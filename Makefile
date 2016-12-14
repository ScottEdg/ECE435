CC = gcc
CFLAGS = -O2 -Wall
SERVER_TARGET = chatServer
CLIENT_TARGET = chatClient

all: server client

server: $(SERVER_TARGET).o
	$(CC) -o $(SERVER_TARGET) $(SERVER_TARGET).o

server.o: $(SERVER_TARGET).c
	$(CC) $(CFLAGS) -c $(SERVER_TARGET).c

client: $(CLIENT_TARGET).o
	$(CC) -o $(CLIENT_TARGET) $(CLIENT_TARGET).o -pthread

client.o: $(CLIENT_TARGET).c
	$(CC) $(CFLAGS) -c $(CLIENT_TARGET).c -pthread

clean:
	rm -f $(SERVER_TARGET).o $(SERVER_TARGET) $(CLIENT_TARGET).o $(CLIENT_TARGET)
