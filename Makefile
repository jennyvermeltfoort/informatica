# tested with: g++-11
CC 		= gcc
CFLAGS  = -Wall -g
SRC 	= src
TARGET 	= main

bla: $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) src/main.c

all: bla

clean:
	$(RM) $(TARGET)