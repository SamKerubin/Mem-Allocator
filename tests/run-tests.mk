CC = gcc
LDFLAGS = -lmemalloc
CFLAGS = -fPIC -Wall -Wextra -Werror -std=gnu23 -O0 -g

SRC_DIR = .
BIN_DIR = ./bin

SRC = $(shell find $(SRC_DIR) -maxdepth 1 -name "*.c" -type f)
BIN = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%, $(SRC))

default: all

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -MMD -MP -MF $(@:.o=.d) $< -o $@

-include $(OBJ_DIR)/*.d

all: $(BIN_DIR) $(BIN)
	@echo -e "Running tests...\n"
	@for test in $(BIN); do \
		echo -e "Executing test: $$test\n"; \
		./$$test || exit 1; \
	done

	@echo "All tests passed successfully";

$(BIN_DIR)/%: $(SRC_DIR)%.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

clean:
	@rm -rf $(BIN_DIR)

.PHONY: all clean
