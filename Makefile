# tested with: g++-11
CC 		= g++
CFLAGS  = -g -Wall -std=c++11 -O1
SRC 	= src
TARGET 	= $(SRC)/main.cc 

main: $(TARGET)
	$(CC) $(CFLAGS) -o main.o $(TARGET)
#	$(CC) -Wall -std=c++11 -S $(TARGET)

all: main

clean:
	$(RM) $(TARGET)