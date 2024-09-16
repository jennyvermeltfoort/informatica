# tested with: g++-11
CC 		= g++
CFLAGS  = -g -Wall 
SRC 	= src
TARGET 	= $(SRC)/main.cpp 
TEST 	= ./test.sh 

main: $(TARGET)
	$(CC) $(CFLAGS) -o main.o $(TARGET)

test: main
	$(TEST) >> results.log

all: main

clean:
	$(RM) $(TARGET)