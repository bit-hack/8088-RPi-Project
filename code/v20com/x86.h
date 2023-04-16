#pragma once
#include <cstdint>

// intAck() types:
//
// https://stanislavs.org/helppc/int_table.html
//
//        0   Divide by zero
//        1   Single step
//        2   Non-maskable  (NMI)
//        3   Breakpoint
//        4   Overflow trap
//        5   BOUND range exceeded                  (186,286,386)
//        6   Invalid opcode                        (186,286,386)
//        7   Coprocessor not available             (286,386)
//  IRQ0  8   timer                                 (55ms intervals, 18.2 per second)
//  IRQ1  9   keyboard service required
//  IRQ2  A   slave 8259 or EGA/VGA vertical retrace
//  IRQ3  B   COM2 or COM4 service required         (COM3-COM8 on MCA PS/2)
//  IRQ4  C   COM1 or COM3 service required
//  IRQ5  D   fixed disk or data request from LPT2
//  IRQ6  E   floppy disk service required
//  IRQ7  F   data request from LPT1                (unreliable on IBM mono)


typedef uint8_t  (*pi86MemRead8_t )(uint32_t addr);
typedef void     (*pi86MemWrite8_t)(uint32_t addr, uint8_t  data);
typedef uint8_t  (*pi86IoRead8_t  )(uint32_t addr);
typedef void     (*pi86IoWrite8_t )(uint32_t addr, uint8_t  data);
typedef uint8_t  (*pi86IntAck_t   )(void);

extern pi86MemRead8_t   pi86ExtMemRead8;
extern pi86MemWrite8_t  pi86ExtMemWrite8;
extern pi86IoRead8_t    pi86ExtIoRead8;
extern pi86IoWrite8_t   pi86ExtIoWrite8;
extern pi86IntAck_t     pi86ExtIntAck;

void     pi86Reset     (void);
void     pi86Start     (void);
void     pi86Stop      (void);
bool     pi86Running   (void);

void     pi86Irq       (void);

void     pi86BusCycle  (uint32_t count);
uint64_t pi86CycleCount(void);
uint32_t pi86TState    (void);