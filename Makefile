CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread -O2 -Iinclude

SRC_DIR = src
BUILD_DIR = build
CLIENT_DIR = client


SRCS:= $(wildcard $(SRC_DIR)/*.cpp)

OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))


TARGET = my_redis_server
CLIENT_TARGET = $(CLIENT_DIR)/redis_client

.PHONY: all clean rebuild run client run-client

all: $(TARGET)


$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

client: $(CLIENT_TARGET)

$(CLIENT_TARGET): $(CLIENT_DIR)/redis_client.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $(CLIENT_TARGET)

clean: 
	rm -rf $(BUILD_DIR) $(TARGET) $(CLIENT_TARGET)

rebuild: clean all


run: all
	./$(TARGET)

run-client: client
	./$(CLIENT_TARGET)
