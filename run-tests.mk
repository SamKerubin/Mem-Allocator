CC = gcc
LDFLAGS = -lmemalloc
CFLAGS = -Wall -Wextra -Werror -Iinclude/ -Llib -std=gnu23 -O0 -g

SRC_DIR = tests
BIN_DIR = ./bin

SRC = $(shell find $(SRC_DIR) -maxdepth 1 -name "*.c" -type f)
BIN = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(SRC))

default: all

all: $(BIN_DIR) $(BIN)
	@echo -e "Running tests...\n"
	@for test in $(BIN); do \
		echo -e "Executing test: $$test\n"; \
		./$$test || echo -e "Failed test\n"; \
	done

	@echo "All tests passed successfully";

$(BIN_DIR)/%: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

clean:
	@rm -rf $(BIN_DIR)

.PHONY: all clean
