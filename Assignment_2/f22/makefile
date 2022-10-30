all: link

link: compile
	g++ -o prodcon main.o general.o outfile.o -lpthread 
	
compile: main.o general.o outfile.o

outfile.o: outfile.cpp outfile.h
	g++ -c -Wall outfile.cpp 

general.o: general.cpp general.h
	g++ -c -Wall general.cpp 

main.o: main.cpp tands.c
	g++ -c -Wall main.cpp 

clean:
	rm -f *.o main.o