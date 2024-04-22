# Omar Kanj
# Equal Safe

# Compiler settings - Can be customized.
CXX = g++
CC = gcc
INCLUDES = -I/opt/homebrew/include
CXXFLAGS = -Wall -std=c++11
CFLAGS = -Wall -std=c11
LDFLAGS = -lpthread -lpigpio -I/opt/homebrew/Cellar/libpaho-mqtt/1.3.13/include -L/opt/homebrew/Cellar/libpaho-mqtt/1.3.13/lib -lpaho-mqtt3c

# Directories
SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Targets
EXEC = $(BIN_DIR)/system_daemon

# Find all C and C++ files in SRC_DIR (including subdirectories)
SRC_CPP = $(shell find $(SRC_DIR) -name '*.cpp')
SRC_C = $(shell find $(SRC_DIR) -name '*.c')

# Replace src directory with obj directory in paths and change .cpp/.c extension to .o
OBJ_CPP = $(SRC_CPP:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
OBJ_C = $(SRC_C:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

OBJ = $(OBJ_CPP) $(OBJ_C)

# Default target
all: $(EXEC)
	@echo =============================================================================
	@echo The system daemon has been created successfully under ./build/bin/system_daemon
	@echo =============================================================================

$(EXEC): $(OBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(OBJ_CPP) -o $@ $(OBJ_C) $(LDFLAGS) $(INCLUDES)

# Rule for C++ object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Rule for C object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR)

install:
	mkdir -p /usr/share/system_daemon
	cp -r ./build/bin/* /usr/share/system_daemon/
	cp -r rootfs/* /

	systemctl daemon-reload
	systemctl enable system_daemon.service

# Phony targets
.PHONY: all clean
