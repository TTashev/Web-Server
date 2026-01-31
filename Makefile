CC = g++
CFLAGS = -Wall -std=c++20 -g
LDFLAGS = -lpthread

all: server

server: server.cpp
	$(CC) $(CFLAGS) -o server $^ $(LDFLAGS)

clean:
	rm -f server client
