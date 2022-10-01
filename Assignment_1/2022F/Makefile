# Author: Hilton Truong
# CCID: truong
# ID: 1615505


all: link

link: Shell.o Process.o Commands.o 
	g++ -o shell379 Shell.o Process.o Commands.o 

compile: Shell.o Process.o Commands.o 

Shell.o: Shell.cpp 
	g++ -c Shell.cpp

Process.o: Process.cpp Process.h
	g++ -c Process.cpp

Commands.o: Commands.cpp Commands.h
	g++ -c Commands.cpp

compress:
	zip shell379.zip Makefile -r .cpp.h

clean:
	rm -f *.o shell379