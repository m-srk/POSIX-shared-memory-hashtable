all: server client

server: hashtable server-main.o server-utils.o
	$(CC) $(SERVER_UTILS) $(HASHTABLE_OUT) server-main.o $(LFLAGS) -o $(SERVER_BIN)

client: client-main.o client-utils.o
	$(CC) client-utils.o client-main.o $(LFLAGS) -o $(CLIENT_BIN)

hashtable: HashTable.cpp
	$(CC) $(CFLAGS) HashTable.cpp -o $(HASHTABLE_OUT)

server-main.o: server-main.cpp
	$(CC) $(CFLAGS) server-main.cpp -o server-main.o

server-utils.o:
	$(CC) $(CFLAGS) $(LFLAGS) server-utils.cpp -o $(SERVER_UTILS)

client-main.o: client-main.cpp
	$(CC) $(CFLAGS) client-main.cpp -o client-main.o

client-utils.o:
	$(CC) $(CFLAGS) $(LFLAGS) client-utils.cpp -o client-utils.o

clean:
	rm -rf *o $(SERVER_BIN) $(CLIENT_BIN)

#---------------------------------------------
# vars
CC=g++
CFLAGS=--std=c++11 -c
LFLAGS=-lpthread -lrt
UTILS=./utils
SERVER_UTILS=server-utils.o
CLIENT_UTILS=client-utils.o
HASHTABLE_OUT=HashTable.o
SERVER_BIN=runserver
CLIENT_BIN=runclient