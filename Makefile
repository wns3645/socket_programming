CC = gcc
CFLAGS = -W -Wall
CLIENT = client
SERVER = server

all : $(CLIENT) $(SERVER)

$(CLIENT) : client.o 
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER) : server.o
	$(CC) $(CFLAGS) -o $@ $^

client.o : client.c protocol.h
server.o : server.c protocol.h
clean : 
	rm *.o client server


