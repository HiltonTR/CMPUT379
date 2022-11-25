all: server client

compile: server.o client.o

server: server.cpp server.h
	g++ server.cpp -Wall -o server

client: client.cpp client.h
	g++ client.cpp -Wall -o client

compress:
	tar -cvf assignment3.tar Makefile README *.cpp *.man *.c

clean:
	rm -f *.o ug* server client