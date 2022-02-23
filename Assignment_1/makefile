# Author: Hilton Truong
# CCID: truong
# ID: 1615505

target = submit
files = msh379.cpp Makefile 379_Assignment_1_Report.pdf

all: msh 379

msh 379: msh379.o
	g++ -o msh379 msh379.o

msh379.o: msh379.cpp
	g++ -c msh379.cpp

clean:
	rm -f msh379.o msh379

tar: 
	tar -cvf$(target).tar $(files)
	gzip $(target).tar
