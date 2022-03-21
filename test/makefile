
target = submit
allFiles = Makefile a3sdn.cpp master.cpp master.h switch.cpp switch.h general.cpp general.h report.pdf

compile:
	g++ -std=c++11 -Wall a2w22.cpp master.cpp master.h switch.cpp switch.h general.cpp general.h -o a2w22

tar:
	tar -cvf $(target).tar $(allFiles)
	gzip $(target).tar

clean:
	rm -f *~ out.* *.o