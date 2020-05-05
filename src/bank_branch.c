#include "bank_branch.h"

static char *_build_msg(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  int size = vsnprintf(NULL, 0, fmt, args);
  if (size < 0) {
    return NULL;
  }
  va_end(args);

  char *str = malloc(size + 1);
  va_start(args, fmt);

  vsnprintf(str, size + 1, fmt, args);
  va_end(args);

  return str;
}

static void _append_balance_state(BalanceHistory *history, BalanceState state) {
  uint8_t len = history->s_history_len;
  if (len > 0) {
    BalanceState prev = history->s_history[len - 1];
    timestamp_t elapsed = (state.s_time - prev.s_time - 1);
    for (int i = 0; i < elapsed; i++) {
      prev.s_time++;
      history->s_history[len + i] = prev;
    }
    len += elapsed;
  }
  history->s_history[len] = state;
  history->s_history_len = len + 1;
}

static BalanceState _get_current_state(BalanceHistory *history) {
  uint8_t len = history->s_history_len;
  return history->s_history[len - 1];
}

static BalanceState _update_history(BalanceHistory *history, balance_t delta,
                                    timestamp_t msg_time) {
  BalanceState new_state = _get_current_state(history);
  new_state.s_time = get_lamport_time();
  new_state.s_balance += delta;

  _append_balance_state(history, new_state);
  if (delta > 0) {
    for (int i = msg_time; i < history->s_history_len - 1; i++) {
      history->s_history[i].s_balance_pending_in = delta;
    }
  }

  return new_state;
}

static int _sync(BankBranch *branch, Message *msg) {
  if (send_multicast(branch->client, msg) != 0) {
    fprintf(stderr, "error: branch %1d: %s: %s\n", branch->id, "send_multicast",
            "sync");
    return -1;
  }

  ReceiveAllError err;
  if ((err = receive_from_all(branch->client, msg->s_header.s_type)) !=
      RCV_ALL_OK) {
    fprintf(stderr, "error: branch %1d: %s\n", branch->id,
            str_receive_error(err));
    return -1;
  }
  return 0;
}

BankBranch *new_bank_branch(local_id id, Store *store) {
  IpcClient *client = malloc(sizeof(IpcClient));
  client->id = id;
  client->store = store;

  BalanceHistory *history = malloc(sizeof(BalanceHistory));
  history->s_id = id;
  history->s_history_len = 0;

  BankBranch *branch = malloc(sizeof(BankBranch));
  branch->id = id;
  branch->client = client;
  branch->history = history;

  return branch;
}

int start_bank_branch(BankBranch *branch, balance_t balance, Log *log) {
  init_client(branch->client);

  timestamp_t start_time = get_lamport_time();
  BalanceState start_state = {
      .s_balance = balance, .s_time = start_time, .s_balance_pending_in = 0};
  _append_balance_state(branch->history, start_state);

  increment_lamprot_time();
  start_time = get_lamport_time();
  char *start_str = _build_msg(log_started_fmt, start_time, branch->id,
                               getpid(), getppid(), balance);
  logfmt(log, start_str);

  MessageHeader start_header = {MESSAGE_MAGIC, strlen(start_str), STARTED,
                                start_time};
  Message start_msg = {start_header};
  strcpy(start_msg.s_payload, start_str);

  if (_sync(branch, &start_msg) != 0) {
    return EXIT_FAILURE;
  }
  free(start_str);
  logfmt(log, log_received_all_started_fmt, get_lamport_time(), branch->id);

  MessageHeader header = {MESSAGE_MAGIC, 0, STARTED, 0};
  Message msg = {header};
  while (msg.s_header.s_type != STOP) {
    if (receive_any(branch->client, &msg) != 0) {
      fprintf(stderr, "error: branch %1d: %s: %s\n", branch->id, "receive_any",
              "transport");
      return EXIT_FAILURE;
    }
    align_lamport_time(msg.s_header.s_local_time);

    switch (msg.s_header.s_type) {
      case TRANSFER: {
        increment_lamprot_time();

        TransferOrder *order = (TransferOrder *)msg.s_payload;
        if (order->s_src == branch->id) {
          _update_history(branch->history, -order->s_amount,
                          msg.s_header.s_local_time);

          msg.s_header.s_local_time = get_lamport_time();
          if (send(branch->client, order->s_dst, &msg) != 0) {
            fprintf(stderr, "error: branch %1d: %s: %s\n", branch->id, "send",
                    "transfer");
            return EXIT_FAILURE;
          }
          logfmt(log, log_transfer_out_fmt, msg.s_header.s_local_time,
                 order->s_src, order->s_amount, order->s_dst);
        } else if (order->s_dst == branch->id) {
          BalanceState state = _update_history(branch->history, order->s_amount,
                                               msg.s_header.s_local_time);
          logfmt(log, log_transfer_in_fmt, state.s_time, order->s_dst,
                 order->s_amount, order->s_src);

          msg.s_header.s_type = ACK;
          msg.s_header.s_payload_len = 0;
          msg.s_header.s_local_time = get_lamport_time();
          if (send(branch->client, PARENT_ID, &msg) != 0) {
            fprintf(stderr, "error: branch %1d: %s: %s\n", branch->id, "send",
                    "ack");
            return EXIT_FAILURE;
          }
        } else {
          fprintf(stderr, "error: branch %1d: %s: %s\n", branch->id,
                  "receive_any", "bad message recipient");
          return EXIT_FAILURE;
        }
        break;
      }
      case STOP:
        break;
      default:
        fprintf(stderr, "error: branch %1d: %s: %s\n", branch->id,
                "receive_any", "unexpected message type");
        return EXIT_FAILURE;
    }
  }

  increment_lamprot_time();
  timestamp_t done_time = get_lamport_time();
  _update_history(branch->history, 0, done_time);

  balance_t done_balance = _get_current_state(branch->history).s_balance;
  char *done_str =
      _build_msg(log_done_fmt, done_time, branch->id, done_balance);
  logfmt(log, done_str);

  MessageHeader done_header = {MESSAGE_MAGIC, strlen(done_str), DONE,
                               done_time};
  Message done_msg = {done_header};
  strcpy(done_msg.s_payload, done_str);

  if (_sync(branch, &done_msg) != 0) {
    return EXIT_FAILURE;
  }
  free(done_str);
  logfmt(log, log_received_all_done_fmt, get_lamport_time(), branch->id);

  increment_lamprot_time();
  MessageHeader balance_header;
  balance_header.s_magic = MESSAGE_MAGIC;
  balance_header.s_type = BALANCE_HISTORY;
  balance_header.s_local_time = get_lamport_time();
  balance_header.s_payload_len = sizeof(BalanceHistory);

  Message balance_msg = {balance_header};
  memcpy(balance_msg.s_payload, branch->history, balance_header.s_payload_len);

  if (send(branch->client, PARENT_ID, &balance_msg) != 0) {
    fprintf(stderr, "error: branch %1d: %s: %s\n", branch->id, "send",
            "balance_history");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void free_bank_branch(BankBranch *branch) {
  free(branch->client);
  free(branch->history);
  free(branch);
}
