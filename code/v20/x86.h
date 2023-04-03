#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <wiringPi.h>

#define V20 88
#define V30 86

#define PIN_CLK   29
#define PIN_RESET 27
#define PIN_ALE   26
#define PIN_IO_M  10
#define PIN_DTR   11
#define PIN_BHE   6
#define PIN_INTR  28
#define PIN_INTA  31

#define AD0 25
#define AD1 24
#define AD2 23
#define AD3 22
#define AD4 21
#define AD5 30
#define AD6 14
#define AD7 13
#define A8  12
#define A9  3
#define A10 2
#define A11 0
#define A12 7
#define A13 9
#define A14 8
#define A15 15
#define A16 16
#define A17 1
#define A18 4
#define A19 5

extern bool Stop_Flag;

using namespace std;

void pi86Reset   (void);
void pi86Start   (int Processor);
void pi86LoadBios(const string &Bios_file);

// Memory
void    Write_Memory_Array(uint32_t addr, const uint8_t *src, uint32_t size);
void    Read_Memory_Array (uint32_t addr, uint8_t *dst, uint32_t size);
void    Write_Memory_Byte (uint32_t addr, uint8_t data);
uint8_t Read_Memory_Byte  (uint32_t addr);
void    Write_Memory_Word (uint32_t addr, uint16_t data);

// IO
void    Write_IO_Byte(uint32_t addr, uint8_t data);
uint8_t Read_IO_Byte (uint32_t addr);
void    Write_IO_Word(uint32_t addr, uint16_t data);

// INT
void IRQ0(void);
void IRQ1(void);

uint8_t *memPtr(uint32_t addr);
