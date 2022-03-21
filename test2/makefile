
target = submit
allFiles = Makefile a3sdn.cpp controller.cpp controller.h switch.cpp switch.h util.cpp util.h report.pdf

compile:
	g++ -std=c++11 -Wall a3sdn.cpp controller.cpp controller.h switch.cpp switch.h util.cpp util.h -o a2w22

tar:
	tar -cvf $(target).tar $(allFiles)
	gzip $(target).tar

clean:
	rm -f *~ out.* *.o