all: server client

compile: server.o client.o

server: server.cpp
	g++ server.cpp -Wall -o server

client: client.cpp
	g++ client.cpp -Wall -o client

compress:
	tar -cvf assignment3.tar Makefile README *.cpp *.1 *.c

clean:
	rm -f *.o ug* server client