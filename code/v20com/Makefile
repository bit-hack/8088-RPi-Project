SOURCES=pi86.cpp x86.cpp
LIBS=-pthread -lwiringPi
CFLAGS=-O2

all: $(SOURCES)
	g++ $(CFLAGS) $(SOURCES) -o pi86 $(LIBS)

test.com: test.s
	nasm -f bin -o test.com test.s
	ndisasm -b 32 -o 256 test.com | tee test.dis

clean:
	rm -rdf pi86 test.com test.dif
