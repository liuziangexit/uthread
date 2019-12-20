CFLAGS = -std=c11 -Wall -Wno-deprecated-declarations

DEBUG_FLAGS = -O0 -g -pg
RELEASE_FLAGS = -O3

TEST_SRC = test
TEST_OUTPUT = build/test
LIB_OUTPUT = build

all:: build

build: 
		@printf "\n\n***** Building libuthread *****\n\n"
		@mkdir -p $(LIB_OUTPUT)
		@cc $(CFLAGS) $(RELEASE_FLAGS) -c uthread.c -o $(LIB_OUTPUT)/uthread.o
		@ar rs $(LIB_OUTPUT)/libuthread.a $(LIB_OUTPUT)/uthread.o
		@rm -rf $(LIB_OUTPUT)/uthread.o

test: build
		@printf "\n\n***** Building Test *****\n\n"
		@mkdir -p $(TEST_OUTPUT)
		@cc $(CFLAGS) $(DEBUG_FLAGS) -c $(TEST_SRC)/yield.c -o $(TEST_OUTPUT)/yield.o
		@cc $(TEST_OUTPUT)/yield.o $(LIB_OUTPUT)/libuthread.a -o $(TEST_OUTPUT)/yield.dll
		@rm -rf $(TEST_OUTPUT)/yield.o

clean:
	rm -rf $(LIB_OUTPUT)
