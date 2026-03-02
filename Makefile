# Variables
CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0` -lsqlite3
TARGET = vip_pos
SRC = vip_pos.c

# Default rule: Jab aap sirf 'make' likhenge
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LIBS)

# Clean rule: Purani compiled file delete karne ke liye
clean:
	rm -f $(TARGET)

