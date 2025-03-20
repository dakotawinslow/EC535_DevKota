
CC=arm-linux-gnueabihf-gcc
TARGET=ktimer
CFLAGS=-static
SRC=ktimer.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
