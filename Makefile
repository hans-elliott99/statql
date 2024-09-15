#
# Globals
#
SRC_DIR := ./src
OBJ_DIR := ./obj
BIN_DIR := .
INCLUDE_DIR := ./src #eventually put headers in include/, in src for intellisense
MAIN_NAME := main
SRC_SUB_DIRS := $(shell find $(SRC_DIR) -type d)
SQLITE_DIR = ./src/sqlite


# Determine key files/dirs
SRC = $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/**/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/$(MAIN_NAME)

OBJ_SUB_DIRS := $(filter-out $(SRC_DIR), $(SRC_SUB_DIRS))
OBJ_SUB_DIRS := $(shell basename -a $(OBJ_SUB_DIRS))
OBJ_SUB_DIRS_NOSQLITE := $(filter-out $(shell basename $(SQLITE_DIR)), $(OBJ_SUB_DIRS))

# Compilation and linking parameters
CC := gcc
CPPFLAGS := -I$(INCLUDE_DIR) -MMD -MP #c preprocessor
CFLAGS := -Wformat -Werror=format-security -Wdate-time -D_FORTIFY_SOURCE=2 \
		   -UNDEBUG -Wall -pedantic -g -O0 -fdiagnostics-color=always
LDFLAGS := -Llib #linker flag
LDLIBS := -lm -ldl -lpthread

# Rules
.PHONY: default all clean
.PRECIOUS: $(TARGET) $(OBJ)

default: $(TARGET)
all: default

# Linking
$(TARGET): $(OBJ)
	$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

# Compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(OBJ_SUB_DIRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


# Directory setup
#$(OBJ_DIR)/%.o: | $(@D)
#	mkdir -p $(@D)

## make ./bin and ./obj
$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

## make src sub-dirs in obj dir
$(OBJ_SUB_DIRS):
	mkdir -p $(OBJ_DIR)/$@




clean:
	rm -f  $(OBJ_DIR)/*.o
	rm -f  $(OBJ_DIR)/*.d
	rm -f $(TARGET)
	cd $(OBJ_DIR) && rm -rf $(OBJ_SUB_DIRS_NOSQLITE)

# sqlite takes longer to build and is not typically modified so no need to
#   rebuild usually



# I like this makefile: https://stackoverflow.com/questions/30573481/how-to-write-a-makefile-with-separate-source-and-header-directories
# Ideal testing setup? https://stackoverflow.com/questions/17896751/makefile-use-multiple-makefiles
