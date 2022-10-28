all: link

link: compile
	g++ -o main main.o -lpthread
	
compile: main.o

prodcon.o: main.cpp
	g++ -c -Wall main.cpp

