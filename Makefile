CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Iinclude

BUILD = build
BIN = bin

all: | $(BUILD) $(BIN)
all: $(BIN)/server $(BIN)/client

$(BUILD):
	mkdir -p $(BUILD)

$(BIN):
	mkdir -p $(BIN)

$(BIN)/server: $(BUILD)/server.o $(BUILD)/utils.o
	$(CXX) $^ -o $@

$(BIN)/client: $(BUILD)/client.o $(BUILD)/utils.o
	$(CXX) $^ -o $@

$(BUILD)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD) $(BIN)