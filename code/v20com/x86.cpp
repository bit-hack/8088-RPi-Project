#include <unistd.h>
#include <wiringPi.h>

#include "x86.h"

#define PIN_CLK   29
#define PIN_RESET 27
#define PIN_ALE   26
#define PIN_IO_M  10
#define PIN_DTR   11
#define PIN_BHE   6
#define PIN_INTR  28
#define PIN_INTA  31
#define PIN_AD0   25
#define PIN_AD1   24
#define PIN_AD2   23
#define PIN_AD3   22
#define PIN_AD4   21
#define PIN_AD5   30
#define PIN_AD6   14
#define PIN_AD7   13
#define PIN_A8    12
#define PIN_A9    3
#define PIN_A10   2
#define PIN_A11   0
#define PIN_A12   7
#define PIN_A13   9
#define PIN_A14   8
#define PIN_A15   15
#define PIN_A16   16
#define PIN_A17   1
#define PIN_A18   4
#define PIN_A19   5


static bool running;

pi86MemRead8_t   pi86ExtMemRead8;
pi86MemWrite8_t  pi86ExtMemWrite8;
pi86IoRead8_t    pi86ExtIoRead8;
pi86IoWrite8_t   pi86ExtIoWrite8;
pi86IntAck_t     pi86ExtIntAck;

static uint64_t cycles = 0;
static uint32_t tstate = 0;

static void pi86MemWrite8(uint32_t addr, uint8_t data) {
  if (pi86ExtMemWrite8) {
    pi86ExtMemWrite8(addr, data);
  }
}

static uint8_t pi86MemRead8(uint32_t addr) {
  if (pi86ExtMemRead8) {
    return pi86ExtMemRead8(addr);
  }
  return 0x90;
}

static void pi86IoWrite8(uint32_t addr, uint8_t data) {
  if (pi86ExtIoWrite8) {
    pi86ExtIoWrite8(addr, data);
  }
}

static uint8_t pi86IoRead8(uint32_t addr) {
  if (pi86ExtIoRead8) {
    return pi86ExtIoRead8(addr);
  }
  return 0;
}

void pi86Irq(void) {
  digitalWrite(PIN_INTR, HIGH);
}

//////////////////////////////
// System Bus
///////////////////////////////

static uint32_t addrBusRead(void) {
  uint32_t addr = 0;
  addr |= digitalRead(PIN_AD0) << 0;
  addr |= digitalRead(PIN_AD1) << 1;
  addr |= digitalRead(PIN_AD2) << 2;
  addr |= digitalRead(PIN_AD3) << 3;
  addr |= digitalRead(PIN_AD4) << 4;
  addr |= digitalRead(PIN_AD5) << 5;
  addr |= digitalRead(PIN_AD6) << 6;
  addr |= digitalRead(PIN_AD7) << 7;
  addr |= digitalRead(PIN_A8)  << 8;
  addr |= digitalRead(PIN_A9)  << 9;
  addr |= digitalRead(PIN_A10) << 10;
  addr |= digitalRead(PIN_A11) << 11;
  addr |= digitalRead(PIN_A12) << 12;
  addr |= digitalRead(PIN_A13) << 13;
  addr |= digitalRead(PIN_A14) << 14;
  addr |= digitalRead(PIN_A15) << 15;
  addr |= digitalRead(PIN_A16) << 16;
  addr |= digitalRead(PIN_A17) << 17;
  addr |= digitalRead(PIN_A18) << 18;
  addr |= digitalRead(PIN_A19) << 19;
  return addr;
}

// Reads the IO_M, RD, WR pins
static uint8_t ctrlBusRead(void) {
  uint8_t ctrl = 0;
  ctrl |= digitalRead(PIN_DTR)  << 0;
  ctrl |= digitalRead(PIN_IO_M) << 1;
  ctrl |= digitalRead(PIN_INTA) << 2;
  return ctrl;
}

// Sets the Data Port direction for read and writes
static void dataBusDir(int mode) {
  pinMode(PIN_AD0, mode);
  pinMode(PIN_AD1, mode);
  pinMode(PIN_AD2, mode);
  pinMode(PIN_AD3, mode);
  pinMode(PIN_AD4, mode);
  pinMode(PIN_AD5, mode);
  pinMode(PIN_AD6, mode);
  pinMode(PIN_AD7, mode);
}

// Writes Data to Data Port 0-7
void dataBusWrite(uint8_t data) {
  digitalWrite(PIN_AD0, (data >> 0) & 1);
  digitalWrite(PIN_AD1, (data >> 1) & 1);
  digitalWrite(PIN_AD2, (data >> 2) & 1);
  digitalWrite(PIN_AD3, (data >> 3) & 1);
  digitalWrite(PIN_AD4, (data >> 4) & 1);
  digitalWrite(PIN_AD5, (data >> 5) & 1);
  digitalWrite(PIN_AD6, (data >> 6) & 1);
  digitalWrite(PIN_AD7, (data >> 7) & 1);
}

// Reads Data to Data Port 0-7
static uint8_t dataBusRead(void) {
  uint8_t ret = 0;
  ret |= digitalRead(PIN_AD0) << 0;
  ret |= digitalRead(PIN_AD1) << 1;
  ret |= digitalRead(PIN_AD2) << 2;
  ret |= digitalRead(PIN_AD3) << 3;
  ret |= digitalRead(PIN_AD4) << 4;
  ret |= digitalRead(PIN_AD5) << 5;
  ret |= digitalRead(PIN_AD6) << 6;
  ret |= digitalRead(PIN_AD7) << 7;
  return ret;
}

// Clicks the clk pin
static void clk() {
  static const uint32_t clocks = 12;
  for (uint32_t i=0; i<clocks; ++i) {
    digitalWrite(PIN_CLK, HIGH);
  }
  for (uint32_t i=0; i<clocks; ++i) {
    digitalWrite(PIN_CLK, LOW);
  }
  ++cycles;
  ++tstate;
}

// Sets up Raspberry PI pins in the begining
static void Setup(void) {  
  running = true;
  
  wiringPiSetup();

  pinMode(PIN_CLK,   OUTPUT);
  pinMode(PIN_RESET, OUTPUT);
  pinMode(PIN_ALE,   INPUT);
  pinMode(PIN_IO_M,  INPUT);
  pinMode(PIN_DTR,   INPUT);
  pinMode(PIN_BHE,   INPUT);

  pinMode(PIN_INTR,  OUTPUT);
  pinMode(PIN_INTA,  INPUT);
  digitalWrite(PIN_INTR, LOW);

  pinMode(PIN_AD0, INPUT);
  pinMode(PIN_AD1, INPUT);
  pinMode(PIN_AD2, INPUT);
  pinMode(PIN_AD3, INPUT);
  pinMode(PIN_AD4, INPUT);
  pinMode(PIN_AD5, INPUT);
  pinMode(PIN_AD6, INPUT);
  pinMode(PIN_AD7, INPUT);
  pinMode(PIN_A8,  INPUT);
  pinMode(PIN_A9,  INPUT);
  pinMode(PIN_A10, INPUT);
  pinMode(PIN_A11, INPUT);
  pinMode(PIN_A12, INPUT);
  pinMode(PIN_A13, INPUT);
  pinMode(PIN_A14, INPUT);
  pinMode(PIN_A15, INPUT);
  pinMode(PIN_A16, INPUT);
  pinMode(PIN_A17, INPUT);
  pinMode(PIN_A18, INPUT);
  pinMode(PIN_A19, INPUT);
}

static void busCycleMemRead(uint32_t addr) {

  dataBusDir(OUTPUT);
  const uint8_t data = pi86MemRead8(addr);
  dataBusWrite(data);
  clk();  // T3

  clk();  // T4
  dataBusDir(INPUT);
}

static void busCycleMemWrite(uint32_t addr) {

  const uint8_t data = dataBusRead();
  pi86MemWrite8(addr, data);
  clk();  // T3

  clk();  // T4
}

static void busCycleIoRead(uint32_t addr) {

  dataBusDir(OUTPUT);
  const uint8_t data = pi86IoRead8(addr);  
  dataBusWrite(data);
  clk();  // T3

  clk();  // T4
  dataBusDir(INPUT);
}

static void busCycleIoWrite(uint32_t addr) {

  const uint8_t data = dataBusRead();
  pi86IoWrite8(addr, data);
  clk();  // T3

  clk();  // T4
}

static void busCycleInterrupt(void) {

  // get interrupt type
  uint8_t type = 0x8;
  if (pi86ExtIntAck) {
    type = pi86ExtIntAck();
  }

  // Waits for second INTA bus cycle 4 CLKS 8088
  clk();  // T3
  clk();  // T4

  clk();  // T1
  clk();  // T2

  dataBusDir(OUTPUT);
  dataBusWrite(type); 
  clk();  // T3

  clk();  // T4

  dataBusDir(INPUT);
  digitalWrite(PIN_INTR, LOW);
}

static void busCycle(void) {

    // todo: if no ALE is detected after X cycles the CPU might be in a hlt state

    tstate = 0;
    clk();  // T1
    if (digitalRead(PIN_ALE) != 1) {
      return;
    }

    const uint32_t addr = addrBusRead();
    clk();  // T2

    switch (ctrlBusRead()) {  // {INTA, IO_M, DTR}
    case 0b100: busCycleMemRead  (addr); break;
    case 0b101: busCycleMemWrite (addr); break;
    case 0b110: busCycleIoRead   (addr); break;
    case 0b111: busCycleIoWrite  (addr); break;
    case 0b010: busCycleInterrupt();
      break;
    }
}

void pi86BusCycle(uint32_t count) {
  while (count--) {
    busCycle();
  }
}

void pi86Reset(void) {
  digitalWrite(PIN_RESET, HIGH);
  for (uint32_t i=8; i--;) {
    clk();
  }
  digitalWrite(PIN_RESET, LOW);
}

void pi86Start() {
  Setup();
  pi86Reset();
}

void pi86Stop(void) {
  running = false;
}

bool pi86running(void) {
  return running;
}

uint64_t pi86CycleCount(void) {
  return cycles;
}

uint32_t pi86TState(void) {
  return tstate;
}