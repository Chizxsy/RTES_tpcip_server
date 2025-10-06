OBJECTS = raid5.o
CC = gcc
CFLAGS = -Wall -g

all: raid5

raid5: raid5.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o raid5