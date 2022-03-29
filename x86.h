#pragma once

#include <string>
#include <thread>
#include <fstream>
#include <cstdint>

#include <unistd.h>


#define V20 	88
#define V30 	86 

#define PIN_CLK   29
#define PIN_RESET 27
#define PIN_ALE   26
#define PIN_IO_M  10
#define PIN_DTR   11
#define PIN_BHE   6

#define PIN_INTR  28
#define PIN_INTA  31

#define AD0   25
#define AD1   24
#define AD2   23
#define AD3   22
#define AD4   21
#define AD5   30
#define AD6   14
#define AD7   13

#define A8    12
#define A9    3
#define A10   2
#define A11   0
#define A12   7
#define A13   9
#define A14   8
#define A15   15

#define A16   16
#define A17   1
#define A18   4 
#define A19   5

extern bool Stop_Flag;

void Reset();	
bool Start(int32_t Processor);
void Load_Bios(std::string Bios_file);

//Memory
void Write_Memory_Array(uint64_t Address, char code_for_8088[], int32_t Length);
void Read_Memory_Array(uint64_t Address, char* char_Array, int32_t Length);
void Write_Memory_Byte(uint64_t Address, char byte_for_8088);
char Read_Memory_Byte(uint64_t Address);		
void Write_Memory_Word(uint64_t Address, uint16_t word_for_8088);

//IO
void Write_IO_Byte(uint64_t Address, char byte_for_8088);
char Read_IO_Byte(uint64_t Address);
void Write_IO_Word(uint64_t Address, uint16_t word_for_8088);

//INT
void IRQ0();
void IRQ1();
