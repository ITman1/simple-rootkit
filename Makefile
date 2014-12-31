# Course:   Bezpeènost informaèních systémù
# Project:  Projekt è.1: jednoduchý rootkit
# Author:   Radim Loskot, xlosko01@stud.fit.vutbr.cz
# Date:     17. 10. 2012
# 
# Usage:
#	- make               compile project - release version
#   - make run           runs rootkit and inserts kernel module which hides this process
#   - make stop          stops rootkit and tries to unload kernel module
#   - make insert-module inserts kernel module which hides this process
#   - make run-rootkit   runs rootkit only
#	- make pack          packs all required files to compile this project    
#	- make clean         clean temp compilers files    
#	- make clean-all     clean all compilers files - includes project    
#	- make clean-outp    clean output project files 
#

# output project and package filename
SRC_DIR=src
OBJ_DIR=objs
TARGET=xlosko01_rootkit
PACKAGE_NAME=xlosko01
MODULE_PATH=src/module
MODULE_NAME=hide_process.ko
PACKAGE_FILES=src/server_lib.cpp src/server_lib.h src/regexp.cpp src/regexp.h src/rootkit.cpp src/module/hide_process.c src/module/Makefile Makefile dokumentace.pdf

# C++ compiler and flags
CXX=g++
CXXFLAGS=$(CXXOPT) -std=c++98 -Wall -pedantic -W
LIBS=-lpthread

# Project files
OBJ_FILES=rootkit.o server_lib.o regexp.o
SRC_FILES=rootkit.cpp server_lib.cpp regexp.cpp server_lib.h regexp.h

# Substitute the path
SRC=$(patsubst %,$(SRC_DIR)/%,$(SRC_FILES))

OBJ=$(patsubst %,$(OBJ_DIR)/%,$(OBJ_FILES))

# Universal rule
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

# START RULE
all: | $(OBJ_DIR) $(TARGET) module

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Linking of modules into release program
$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(FLAGS) $(LIBS)

# Building kernel module
module: $(OBJ)
	make -C $(MODULE_PATH)

.PHONY: clean clean-all clean-outp pack run stop insert-module run-rootkit

pack:
	zip $(PACKAGE_NAME).zip $(PACKAGE_FILES)

clean:
	rm -rf $(OBJ_DIR)
	make clean -C $(MODULE_PATH)

clean-outp:								# project doesnt produce any
	

clean-all: clean clean-outp
	rm -rf $(TARGET)

debug:
	make -B all CXXOPT=-g3
	
release:
	make -B all CXXOPT=-O3

run:
	-make insert-module
	make run-rootkit

insert-module:
	insmod $(MODULE_PATH)/$(MODULE_NAME)

run-rootkit:
	./$(TARGET)

stop: # pkill only substring because of task structure in kernel etc...
	-rmmod $(MODULE_NAME)
	-pkill xlosko01_rootki
