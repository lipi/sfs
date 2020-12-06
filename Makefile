
CC=gcc

all: server client

server: server.c
	$(CC) -pthread -o $@ $?

client: client.c
	$(CC) -o $@ $?

clean: server client
	rm -rf $?
