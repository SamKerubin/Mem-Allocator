CC = gcc
LDFLAGS = -shared
CFLAGS = -fPIC -Wall -Wextra -Werror -Iinclude/ -std=gnu23

ifdef DEBUG
	CFLAGS += -O0 -g
else
	CFLAGS += -O2
endif

SRC_DIR = src
OBJ_DIR = obj
LIB_DIR = lib

SRC = $(SRC_DIR)/memalloc.c
OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

TARGET = $(LIB_DIR)/libmemalloc.so

default: all

$(OBJ_DIR) $(LIB_DIR):
	@mkdir -p $(OBJ_DIR) $(LIB_DIR)

$(OBJ): | $(OBJ_DIR)

$(TARGET): $(OBJ) | $(LIB_DIR)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -MMD -MP -MF $(@:.o=.d) $< -o $@

-include $(OBJ_DIR)/*.d

all: $(TARGET)

clean:
	@rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
