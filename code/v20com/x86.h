#pragma once
#include <cstdint>

typedef uint8_t  (*pi86MemRead8_t  )(uint32_t addr);
typedef void     (*pi86MemWrite8_t )(uint32_t addr, uint8_t  data);
typedef uint8_t  (*pi86IoRead8_t   )(uint32_t addr);
typedef void     (*pi86IoWrite8_t  )(uint32_t addr, uint8_t  data);
typedef void     (*pi86IntAck_t    )();

extern pi86MemRead8_t   pi86ExtMemRead8;
extern pi86MemWrite8_t  pi86ExtMemWrite8;
extern pi86IoRead8_t    pi86ExtIoRead8;
extern pi86IoWrite8_t   pi86ExtIoWrite8;
extern pi86IntAck_t     pi86ExtIntAck;

void     pi86Reset     (void);
void     pi86Start     (void);
void     pi86Stop      (void);
bool     pi86Running   (void);
void     pi86Irq0      (void);
void     pi86Irq1      (void);
void     pi86BusCycle  (uint32_t count);
uint64_t pi86CycleCount(void);
