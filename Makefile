# tested with: g++-11
CC 		= gcc
CFLAGS  = -mavx2 -O5
SRC 	= src
TARGET 	= life

life: $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)/main.c

all: life

clean:
	$(RM) $(TARGET)