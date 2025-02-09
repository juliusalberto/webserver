CC=gcc
CFLAGS=-Wall -Wextra -g
LDFLAGS=-pthread

# Directories
SRC_DIR=src
TEST_DIR=tests
OBJ_DIR=obj

# Main program
TARGET=httpd
SRCS=$(wildcard $(SRC_DIR)/*.c)
OBJS=$(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Test program
TEST_TARGET=run_tests
TEST_SRCS=$(wildcard $(TEST_DIR)/*.c)
TEST_OBJS=$(TEST_SRCS:$(TEST_DIR)/%.c=$(OBJ_DIR)/%.o)

.PHONY: all clean test memcheck

all: $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(TARGET): CFLAGS += -U TESTING
$(TARGET): $(OBJ_DIR) $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Test compilation - define TESTING
test: CFLAGS += -DTESTING
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(OBJS) $(TEST_OBJS) 
	$(CC) $(OBJS) $(TEST_OBJS) -o $(TEST_TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) 8080 ./www

clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(TEST_TARGET)