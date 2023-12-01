CC = gcc
LDFLAGS = -lsqlite3 -lpthread -luuid -lm

SERVER_SRC_DIR = ./src
SERVER_OBJ_DIR = ./obj
SERVER_BIN_DIR = ./bin
SERVER_INC_DIR = ./include
SERVER_PUB_DIR = ./public
SERVER_LOG_DIR = ./logs
SERVER_TESTS_DIR = ./tests
SERVER_SRC_FILES = $(wildcard $(SERVER_SRC_DIR)/*.c)
SERVER_OBJ_FILES = $(patsubst $(SERVER_SRC_DIR)/%.c, $(SERVER_OBJ_DIR)/%.o, $(SERVER_SRC_FILES))

CFLAGS = -Wall -Wextra -g -I$(SERVER_INC_DIR)

$(SERVER_BIN_DIR)/server: $(SERVER_OBJ_FILES) $(SERVER_BIN_DIR) $(SERVER_PUB_DIR) $(SERVER_LOG_DIR)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJ_FILES) $(LDFLAGS)

$(SERVER_OBJ_DIR)/%.o: $(SERVER_SRC_DIR)/%.c $(SERVER_OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(SERVER_PUB_DIR):
	mkdir -p $(SERVER_PUB_DIR)
	cp $(SERVER_TESTS_DIR)/* $(SERVER_PUB_DIR)

$(SERVER_BIN_DIR):
	mkdir -p $(SERVER_BIN_DIR)

$(SERVER_OBJ_DIR):
	mkdir -p $(SERVER_OBJ_DIR)

$(SERVER_LOG_DIR):
	mkdir -p $(SERVER_LOG_DIR)

clean:
	rm -rf $(SERVER_OBJ_DIR) $(SERVER_BIN_DIR)

clean-data:
	rm -rf $(SERVER_PUB_DIR) $(SERVER_LOG_DIR)

.PHONY: all clean clean-data