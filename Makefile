# tested with: g++-11
CC 		= g++
CFLAGS  = -g -Wall -std=c++11
SRC 	= src
TARGET 	= $(SRC)/main.cc 

main: $(TARGET)
	$(CC) $(CFLAGS) -o main.o $(TARGET)
#	$(CC) $(CFLAGS) -fverbose-asm -S $(TARGET)

all: main

clean:
	$(RM) $(TARGET)