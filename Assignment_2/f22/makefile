all: link

optimize: optimization

link: compile
	g++ -o prodcon main.o general.o outfile.o -lpthread 
	
optimization: compile
	g++ -o prodcon main.o general.o outfile.o -lpthread -O

compile: main.o general.o outfile.o

outfile.o: outfile.cpp outfile.h
	g++ -c -Wall outfile.cpp 

general.o: general.cpp general.h
	g++ -c -Wall general.cpp 

main.o: main.cpp tands.c
	g++ -c -Wall main.cpp 

zip: 
	zip prodcon.zip makefile -r *.cpp *.h *.c README

clean:
	rm -f *.o *.log prodcon