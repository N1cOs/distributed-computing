FORMATTER = clang-format

.PHONY: format
format:
	find $(SRC) -type f -name '*.[ch]' | \
	xargs $(FORMATTER) -i


ARCHIVE = $(PA).tar.gz

.PHONY: archive
archive: format
	mkdir $(PA)
	cp -r $(SRC)/* $(INCLUDE)/* $(PA)
	tar -czf $(ARCHIVE) $(PA)
	rm -rf $(PA)


CC = clang
BIN = bin
SRC = src
INCLUDE = include

PA := pa
OUT = $(BIN)/$(PA)

.PHONY: build
build:
	$(CC) -Wall \
		--pedantic \
		--output $(OUT) \
		$(if $(DEBUG),--debug,) \
		--include-directory $(INCLUDE) $(SRC)/*.c 


.PHONY: run
run:
	@$(OUT) $(ARGS)


VALGRIND = valgrind

.PHONY: memcheck
memcheck:
	$(VALGRIND) --leak-check=yes $(OUT) $(ARGS)


EVENTS_LOG = events.log

.PHONY: clean
clean:
	rm -rf $(EVENTS_LOG) *.tar.gz $(BIN)/*
