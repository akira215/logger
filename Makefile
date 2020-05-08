ifdef SYSROOT
CXX		:= $(SYSROOT)/bin/arm-linux-gnueabihf-g++
SHARED_DIR	:= $(SYSROOT)/arm-linux-gnueabihf/lib
else
CXX		:= /usr/bin/g++
SHARED_DIR	:= /usr/lib64
endif

CXX_FLAGS := -Wall -Wextra -std=c++17 -ggdb

BIN		:= bin
SRC		:= src
INCLUDE		:= src

LIBRARIES	:= -pthread -lstdc++fs

EXECUTABLE	:= logger


all: $(BIN)/$(EXECUTABLE)

run: clean all
	clear
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): $(SRC)/*.cpp
	$(CXX) $(CXX_FLAGS) -I$(INCLUDE) $^ -o $@ -L$(SHARED_DIR) $(LIBRARIES)

clean:
	-rm $(BIN)/*
