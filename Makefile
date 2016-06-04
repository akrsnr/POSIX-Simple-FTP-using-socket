SOURCE = server.c client.c
CFLAGS = 
LIBS = -lm -pthread
OBJS = server.o client.o
CC = gcc
OUT_DIR := .
EXE = server client

all:	$(EXE) 

server: server.o
	$(CC) server.o -o server $(LIBS)

server.o: server.c 
	$(CC) $(CFLAGS) -c server.c

client: client.o
	$(CC) client.o -o client $(LIBS)

client.o: client.c 
	$(CC) $(CFLAGS) -c client.c

clean:
	rm -f *.o $(EXE)
