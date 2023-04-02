SOURCES=drives.cpp pi86.cpp timer.cpp vga.cpp x86.cpp
LIBS=`sdl2-config --cflags --libs` -pthread -lwiringPi


all:
	g++ $(SOURCES) -o pi86 $(LIBS)

