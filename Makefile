CC      = g++
CFLAGS  = -Wall -g -pthread -O2
LDFLAGS = -lSDL2 -lwiringPi
FILES   = vga.cpp drives.cpp pi86.cpp timer.cpp x86.cpp gpio.cpp
EXENAME = pi86

pi88: $(FILES)
	$(CC) $(CFLAGS) $(FILES) $(LDFLAGS) -o $(EXENAME)

pi88_prof: $(FILES)
	$(CC) $(CFLAGS) -pg $(FILES) $(LDFLAGS) -o $(EXENAME)

clean:
	-rm $(EXENAME)
