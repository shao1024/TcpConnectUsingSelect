CC=g++
CFLAGS=-Wall -Wextra -std=c++11

all: server client_send client_recv

server: server.cpp
	$(CC) $(CFLAGS) server.cpp -o server

client_send: client_send.cpp
	$(CC) $(CFLAGS) client_send.cpp -o client_send

client_recv: client_recv.cpp
	$(CC) $(CFLAGS) client_recv.cpp -o client_recv

clean:
	rm -f server client_send client_recv