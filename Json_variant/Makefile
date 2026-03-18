CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude
LDFLAGS = -pthread -lssl -lcrypto

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

SERVER_SRC = \
	$(SRC_DIR)/server/server.cpp \
	$(SRC_DIR)/server/request_handler.cpp \
	$(SRC_DIR)/auth/auth.cpp \
	$(SRC_DIR)/tasks/tasks.cpp \
	$(SRC_DIR)/tasks/task_executor.cpp \
	$(SRC_DIR)/common/json_utils.cpp \
	$(SRC_DIR)/common/logger.cpp \
	$(SRC_DIR)/common/config.cpp \
	$(SRC_DIR)/common/protocol_validator.cpp \
	$(SRC_DIR)/security/rate_limiter.cpp \
	$(SRC_DIR)/session/session_registry.cpp

CLIENT_SRC = $(SRC_DIR)/client/client.cpp

SERVER_OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SERVER_SRC))
CLIENT_OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CLIENT_SRC))

SERVER_BIN = $(BIN_DIR)/server
CLIENT_BIN = $(BIN_DIR)/client

.PHONY: all clean rebuild run-server run-client

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(SERVER_OBJ) -o $@ $(LDFLAGS)

$(CLIENT_BIN): $(CLIENT_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CLIENT_OBJ) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run-server: $(SERVER_BIN)
	./$(SERVER_BIN)

run-client: $(CLIENT_BIN)
	./$(CLIENT_BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

rebuild: clean all