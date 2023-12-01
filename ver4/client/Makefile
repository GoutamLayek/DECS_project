CC = gcc
LDFLAGS = -lsqlite3 -lpthread -luuid -lm

CLIENT_SRC_DIR = ./src
CLIENT_SRC_HELPER_DIR = $(CLIENT_SRC_DIR)/helper
CLIENT_OBJ_DIR = ./obj
CLIENT_BIN_DIR = ./bin
CLIENT_INC_DIR = ./include
CLIENT_LOG_DIR = ./logs
CLIENT_TESTS_DIR = ./tests
CLIENT_SRC_HELPER_FILES = $(wildcard $(CLIENT_SRC_HELPER_DIR)/*.c)
CLIENT_OBJ_FILES = $(patsubst $(CLIENT_SRC_HELPER_DIR)/%.c, $(CLIENT_OBJ_DIR)/%.o, $(CLIENT_SRC_HELPER_FILES))

CFLAGS = -Wall -Wextra -g -I$(CLIENT_INC_DIR)

all: $(CLIENT_BIN_DIR)/submit $(CLIENT_BIN_DIR)/load-generating-submit

$(CLIENT_BIN_DIR)/submit: $(CLIENT_SRC_DIR)/client.c  $(CLIENT_OBJ_FILES) $(CLIENT_BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<  $(CLIENT_OBJ_FILES) $(LDFLAGS)

$(CLIENT_BIN_DIR)/load-generating-submit: $(CLIENT_SRC_DIR)/load-generating-client.c $(CLIENT_OBJ_FILES) $(CLIENT_BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<  $(CLIENT_OBJ_FILES) $(LDFLAGS)

$(CLIENT_OBJ_DIR)/%.o: $(CLIENT_SRC_HELPER_DIR)/%.c $(CLIENT_INC_DIR)/%.h $(CLIENT_OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(CLIENT_BIN_DIR):
	mkdir -p $(CLIENT_BIN_DIR)
	cp $(CLIENT_TESTS_DIR)/* $(CLIENT_BIN_DIR)

$(CLIENT_OBJ_DIR):
	mkdir -p $(CLIENT_OBJ_DIR)

$(CLIENT_LOG_DIR):
	mkdir -p $(CLIENT_LOG_DIR)

clean:
	rm -rf $(CLIENT_OBJ_DIR) $(CLIENT_BIN_DIR)

clean-data:
	rm -rf $(CLIENT_LOG_DIR)

.PHONY: all clean clean-data
