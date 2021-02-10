CC = gcc
MYCFLAGS = -O2 -Wall

all: lvis_release_reader

lvis_release_reader: lvis_release_reader.c lvis_release_structures.h
	$(CC) $(MYCFLAGS) lvis_release_reader.c -o lvis_release_reader -lm

clean: 
	rm -f *.o core lvis_release_reader
