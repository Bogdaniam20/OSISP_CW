CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_GNU_SOURCE -DHAVE_STRDUP=0
LDFLAGS = -lssl -lcrypto
TARGET = deduplicator
SOURCES = main.c utils.c file_utils.c deduplication.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
