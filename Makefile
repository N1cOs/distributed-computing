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
LIB = lib/lib64
RUNTIME=runtime

PA := pa
OUT = $(BIN)/$(PA)

.PHONY: build
build:
	mkdir --parents $(BIN)
	$(CC) -Wall \
		--pedantic \
		--output $(OUT) \
		$(if $(DEBUG),--debug,) \
		--include-directory $(INCLUDE) \
		--library-directory $(LIB) \
		-l $(RUNTIME) $(SRC)/*.c 


.PHONY: run
run:
	@LD_LIBRARY_PATH=$(LIB) $(OUT) $(ARGS)


VALGRIND = valgrind

.PHONY: memcheck
memcheck: build
	LD_LIBRARY_PATH=$(LIB) $(VALGRIND) --leak-check=yes $(OUT) $(ARGS)


EVENTS_LOG = events.log

.PHONY: clean
clean:
	rm -rf $(EVENTS_LOG) *.tar.gz $(BIN)/*
