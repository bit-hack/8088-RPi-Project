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

using namespace std;

void     pi86Reset      (void);
void     pi86Start      (int Processor);
void     pi86Stop       (void);
bool     pi86Running    (void);
bool     pi86LoadBios   (const string &Bios_file);
uint8_t *pi86MemPtr     (uint32_t addr);
uint8_t  pi86MemRead8   (uint32_t addr);
void     pi86MemReadPtr (uint32_t addr, uint8_t *dst, uint32_t size);
void     pi86MemWrite8  (uint32_t addr, uint8_t data);
void     pi86MemWrite16 (uint32_t addr, uint16_t data);
void     pi86MemWritePtr(uint32_t addr, const uint8_t *src, uint32_t size);
void     pi86IoWrite8   (uint32_t addr, uint8_t  data);
void     pi86IoWrite16  (uint32_t addr, uint16_t data);
uint8_t  pi86IoRead8    (uint32_t addr);
void     pi86Irq0       (void);
void     pi86Irq1       (void);
