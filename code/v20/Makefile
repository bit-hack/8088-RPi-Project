SOURCES=drives.cpp pi86.cpp vga.cpp x86.cpp
LIBS=`sdl2-config --cflags --libs` -pthread -lwiringPi
CFLAGS=-O2

all: $(SOURCES)
	g++ $(CFLAGS) $(SOURCES) -o pi86 $(LIBS)

bios.bin: bios.asm
	nasm bios.asm -o bios.bin

clean:
	rm -rdf bios.bin pi86
