target = submit
allFiles = Makefile a2w22.cpp controller.cpp controller.h switch.cpp switch.h

compile:
	g++ -std=c++11 -w -Wall a2w22.cpp controller.cpp controller.h switch.cpp switch.h -o a2w22

tar:
	tar -cvf $(target).tar $(allFiles)
	gzip $(target).tar

clean:
	rm -f *~ out.* *.o