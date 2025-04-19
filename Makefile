SOURCES=drives.cpp pi86.cpp vga.cpp x86.cpp keyboard.cpp
LIBS=`sdl-config --cflags --libs` -pthread -lwiringPi
CFLAGS=-O2 -g

.PHONY: clean

all: $(SOURCES)
	g++ $(CFLAGS) $(SOURCES) -o pi86 $(LIBS)

clean:
	rm -rdf pi86
