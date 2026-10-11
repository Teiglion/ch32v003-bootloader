CC := gcc

CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2

TARGET := ch32_loader

all: $(TARGET)

$(TARGET): main.c Makefile
	$(CC) $(CFLAGS) main.c -o $@

clean:
	rm -f $(TARGET)

.PHONY: all clean