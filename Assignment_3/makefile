all: server client

compile: server.o client.o

server: server.cpp server.h
	g++ server.cpp -Wall -o server -O

client: client.cpp client.h
	g++ client.cpp -Wall -o client -O

zip:
	tar -cvf Assign3.tar makefile README *.cpp *.man *.c *.h *.pdf

clean:
	rm -f *.o u* server client