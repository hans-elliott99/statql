SRC_DIR := ./src
OBJ_DIR := ./obj
BIN_DIR := .
INCLUDE_DIR := ./src #eventually put headers in include/, in src for intellisense
TEST_DIR := ./tests
MAIN_NAME := main

SRC = $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/sqlite/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/$(MAIN_NAME)

# gcc -I"/usr/share/R/include" -DNDEBUG
# -fpic  -g -O2 -fdebug-prefix-map=/build/r-base-DdVjkr/r-base-4.3.2=.
# -fstack-protector-strong -Wformat -Werror=format-security -Wdate-time -D_FORTIFY_SOURCE=2
# -UNDEBUG -Wall -pedantic -g -O0 -fdiagnostics-color=always -c grepvec.c -o grepvec.o

CC := gcc
CPPFLAGS := -I$(INCLUDE_DIR) -MMD -MP #c preprocessor
CFLAGS := -Wformat -Werror=format-security -Wdate-time -D_FORTIFY_SOURCE=2 \
		   -UNDEBUG -Wall -pedantic -g -O0 -fdiagnostics-color=always
LDFLAGS := -Llib #linker flag
LDLIBS := -lm -ldl -lpthread

.PHONY: default all clean test
.PRECIOUS: $(TARGET) $(OBJ)

default: $(TARGET)
all: default

# Linking
$(TARGET): $(OBJ)
	$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

# Compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


# Other
$(OBJ_DIR)/%.o: | $(@D)
	mkdir -p $(@D)

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	rm -f  $(OBJ_DIR)/*.o
	rm -f  $(OBJ_DIR)/*.d
	rm -rf $(TARGET)
	rm -rf $(BIN_DIR)/test

clean-sqlite:
	find $(OBJ_DIR) -type f -name "*.o" -delete
	find $(OBJ_DIR) -type f -name "*.d" -delete


# I like this makefile: https://stackoverflow.com/questions/30573481/how-to-write-a-makefile-with-separate-source-and-header-directories
# Ideal testing setup: https://stackoverflow.com/questions/17896751/makefile-use-multiple-makefiles
