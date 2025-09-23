OBJECTS = rpi_server.o client.o
CC = gcc
CFLAGS = -Wall -g

all: rpi_server client

rpi_server: rpi_server.o
	$(CC) $(CFLAGS) -o $@ $^

client: client.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o rpi_server client
