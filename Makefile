all: clean server client

server: hashtable server-utils.o server-main.o
	$(CC) $(SERVER_UTILS) $(HASHTABLE_OUT) server-main.o $(LFLAGS) -o $(SERVER_BIN)

client: client-utils.o client-main.o
	$(CC) client-utils.o client-main.o $(LFLAGS) -o $(CLIENT_BIN)

hashtable: hashtable.c
	$(CC) $(CFLAGS) hashtable.c -o $(HASHTABLE_OUT)

server-main.o: server-main.c
	$(CC) $(CFLAGS) server-main.c -o server-main.o

server-utils.o:
	$(CC) $(CFLAGS) $(LFLAGS) server-utils.c -o $(SERVER_UTILS)

client-main.o: client-main.c
	$(CC) $(CFLAGS) client-main.c -o client-main.o

client-utils.o:
	$(CC) $(CFLAGS) $(LFLAGS) client-utils.c -o client-utils.o

clean:
	rm -rf *o $(SERVER_BIN) $(CLIENT_BIN)

#---------------------------------------------
# vars
CC=gcc
CFLAGS=-c
LFLAGS=-lpthread -lrt
SERVER_UTILS=server-utils.o
CLIENT_UTILS=client-utils.o
HASHTABLE_OUT=hashtable.o
SERVER_BIN=runserver
CLIENT_BIN=runclient