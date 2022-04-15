# makefile

CC = gcc
CFLAGS = -Wall

SOURCES = ring.c node.c utils.c communicate.c

OBJECTS = ring.o node.o utils.o communicate.o 

ring: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

ring.o: ring.c ring.h node.h utils.h communicate.h

node.o: node.c ring.h node.h

utils.o: utils.c ring.h node.h

communicate.o: communicate.c ring.h node.h utils.h communicate.h

clean: 
	rm -f *.o  ring 