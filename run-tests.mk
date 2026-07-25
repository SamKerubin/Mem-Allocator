CC = gcc
LDFLAGS = -lmemalloc
CFLAGS = -Wall -Wextra -Werror -Iinclude/ -Llib -std=gnu23 -O0 -g

SRC_DIR = tests
BIN_DIR = ./bin

SRC = $(shell find $(SRC_DIR) -maxdepth 1 -name "*.c" -type f)
BIN = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(SRC))

default: all

all: $(BIN_DIR) $(BIN)
	@declare tests_failed=0; \
	echo -e "Running tests...\n"; \
	for test in $(BIN); do \
		echo -e "Executing test: $$test\n"; \
		./$$test || { echo -e "Failed test\n"; ((tests_failed+=1)); } \
	done; \
	[ "$$tests_failed" -eq 0 ] && echo "All tests passed successfully" || echo "$$tests_failed" tests failed;

$(BIN_DIR)/%: $(SRC_DIR)/%.c
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

clean:
	@rm -rf $(BIN_DIR)

.PHONY: all clean
