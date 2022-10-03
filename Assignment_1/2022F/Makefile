# Author: Hilton Truong
# CCID: truong
# ID: 1615505

all: group

group: Shell.o Process.o Commands.o 
	g++ -o shell379 Shell.o Process.o Commands.o 

compile: Shell.o Process.o Commands.o 

Shell.o: Shell.cpp Shell.h
	g++ -c Shell.cpp

Process.o: Process.cpp Process.h
	g++ -c Process.cpp

Commands.o: Commands.cpp Commands.h
	g++ -c Commands.cpp

zip:
	zip shell379.zip Makefile -r *.cpp *.h README.txt

clean:
	rm -f *.o shell379