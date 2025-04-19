SOURCES=drives.cpp pi86.cpp vga.cpp x86.cpp keyboard.cpp
#LIBS=`sdl2-config --cflags --libs` -pthread -lwiringPi
LIBS=`sdl-config --cflags --libs` -pthread -lwiringPi
CFLAGS=-O2 -g

.PHONY: clean

all: $(SOURCES) bios.bin
	g++ $(CFLAGS) $(SOURCES) -o pi86 $(LIBS)

bios.bin: bios/bios.asm
	cd bios; nasm bios.asm -o bios.bin

clean:
	rm -rdf bios/bios.bin pi86
