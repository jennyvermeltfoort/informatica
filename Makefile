# tested with: g++-11
CC 		= g++
CFLAGS  = -g -Wall 
SRC 	= src
TARGET 	= $(SRC)/main.cc 

main: $(TARGET)
	$(CC) $(CFLAGS) -o main.o $(TARGET)

all: main

clean:
	$(RM) $(TARGET)