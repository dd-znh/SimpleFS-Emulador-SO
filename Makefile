GXX=g++
CXXFLAGS = -std=c++17
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system

interface: gui.o fs.o disk.o fileops.o
	$(GXX) $(CXXFLAGS) $(LDFLAGS) gui.o fs.o disk.o fileops.o -o interface

gui.o: gui.cc gui.h
	$(GXX) -Wall gui.cc -c -o gui.o -g

simplefs: shell.o fs.o disk.o
	$(GXX) shell.o fs.o disk.o -o simplefs

fileops.o: fileops.cc fileops.h
	$(GXX) -Wall fileops.cc -c -o fileops.o -g

shell.o: shell.cc
	$(GXX) -Wall shell.cc -c -o shell.o -g

fs.o: fs.cc fs.h
	$(GXX) -Wall fs.cc -c -o fs.o -g

disk.o: disk.cc disk.h
	$(GXX) -Wall disk.cc -c -o disk.o -g

clean:
	rm simplefs disk.o fs.o shell.o interface gui.o fileops.o