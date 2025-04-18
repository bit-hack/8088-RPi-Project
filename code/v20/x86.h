#pragma once

#include <cstdint>
#include <fstream>
#include <string>

void     pi86Reset      (void);
void     pi86Start      (void);
void     pi86Stop       (void);
bool     pi86Running    (void);
bool     pi86LoadBios   (const std::string &Bios_file);
uint8_t *pi86MemPtr     (uint32_t addr);
uint8_t  pi86MemRead8   (uint32_t addr);
void     pi86MemReadPtr (uint32_t addr, uint8_t *dst, uint32_t size);
void     pi86MemWrite8  (uint32_t addr, uint8_t data);
void     pi86MemWritePtr(uint32_t addr, const uint8_t *src, uint32_t size);
void     pi86IoWrite8   (uint32_t addr, uint8_t  data);
uint8_t  pi86IoRead8    (uint32_t addr);
void     pi86Irq        (uint8_t line);
void     pi86BusCycle   (uint32_t cycles);
void     pi86Trace      (uint32_t cycles);
