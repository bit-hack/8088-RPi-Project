#include <atomic>
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


static std::atomic_bool Running   { false };
static std::atomic_bool IRQ0_Flag { false };
static std::atomic_bool IRQ1_Flag { false };

pi86MemRead8_t   pi86ExtMemRead8;
pi86MemWrite8_t  pi86ExtMemWrite8;
pi86IoRead8_t    pi86ExtIoRead8;
pi86IoWrite8_t   pi86ExtIoWrite8;
pi86IntAck_t     pi86ExtIntAck;

static uint64_t cycles = 0;

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

void pi86Irq0(void) {
  IRQ0_Flag = true;
  digitalWrite(PIN_INTR, HIGH);
}

void pi86Irq1(void) {
  IRQ1_Flag = true;
  digitalWrite(PIN_INTR, HIGH);
}

static uint8_t Read_Interrupts(void) {
  uint8_t intr = 0;
  intr |= IRQ0_Flag << 0;
  intr |= IRQ1_Flag << 1;
  return intr;
}

//////////////////////////////
// System Bus
///////////////////////////////

static uint32_t Read_Address(void) {
  uint32_t Address = 0;
  Address |= digitalRead(AD0) << 0;
  Address |= digitalRead(AD1) << 1;
  Address |= digitalRead(AD2) << 2;
  Address |= digitalRead(AD3) << 3;
  Address |= digitalRead(AD4) << 4;
  Address |= digitalRead(AD5) << 5;
  Address |= digitalRead(AD6) << 6;
  Address |= digitalRead(AD7) << 7;
  Address |= digitalRead(A8)  << 8;
  Address |= digitalRead(A9)  << 9;
  Address |= digitalRead(A10) << 10;
  Address |= digitalRead(A11) << 11;
  Address |= digitalRead(A12) << 12;
  Address |= digitalRead(A13) << 13;
  Address |= digitalRead(A14) << 14;
  Address |= digitalRead(A15) << 15;
  Address |= digitalRead(A16) << 16;
  Address |= digitalRead(A17) << 17;
  Address |= digitalRead(A18) << 18;
  Address |= digitalRead(A19) << 19;
  return Address;
}

// Reads the IO_M, RD, WR pins
static uint8_t Read_Control_Bus(void) {
  uint8_t Control_Bus = 0;
  Control_Bus |= digitalRead(PIN_DTR)  << 0;
  Control_Bus |= digitalRead(PIN_IO_M) << 1;
  Control_Bus |= digitalRead(PIN_INTA) << 2;
  return Control_Bus;
}

// Sets the Data Port direction for read and writes
static void Data_Bus_Direction_8088_IN(void) {
  pinMode(AD0, INPUT);
  pinMode(AD1, INPUT);
  pinMode(AD2, INPUT);
  pinMode(AD3, INPUT);
  pinMode(AD4, INPUT);
  pinMode(AD5, INPUT);
  pinMode(AD6, INPUT);
  pinMode(AD7, INPUT);
}

static void Data_Bus_Direction_8088_OUT(void) {
  pinMode(AD0, OUTPUT);
  pinMode(AD1, OUTPUT);
  pinMode(AD2, OUTPUT);
  pinMode(AD3, OUTPUT);
  pinMode(AD4, OUTPUT);
  pinMode(AD5, OUTPUT);
  pinMode(AD6, OUTPUT);
  pinMode(AD7, OUTPUT);
}

// Writes Data to Data Port 0-7
void Write_To_Data_Port_0_7(uint8_t Byte) {
  digitalWrite(AD0, (Byte >> 0) & 1);
  digitalWrite(AD1, (Byte >> 1) & 1);
  digitalWrite(AD2, (Byte >> 2) & 1);
  digitalWrite(AD3, (Byte >> 3) & 1);
  digitalWrite(AD4, (Byte >> 4) & 1);
  digitalWrite(AD5, (Byte >> 5) & 1);
  digitalWrite(AD6, (Byte >> 6) & 1);
  digitalWrite(AD7, (Byte >> 7) & 1);
}

// Reads Data to Data Port 0-7
static uint8_t Read_From_Data_Port_0_7(void) {
  uint8_t ret = 0;
  ret |= digitalRead(AD0) << 0;
  ret |= digitalRead(AD1) << 1;
  ret |= digitalRead(AD2) << 2;
  ret |= digitalRead(AD3) << 3;
  ret |= digitalRead(AD4) << 4;
  ret |= digitalRead(AD5) << 5;
  ret |= digitalRead(AD6) << 6;
  ret |= digitalRead(AD7) << 7;
  return ret;
}

// Clicks the CLK pin
static void CLK() {
  static const uint32_t clocks = 12;
  for (uint32_t i=0; i<clocks; ++i) {
    digitalWrite(PIN_CLK, HIGH);
  }
  for (uint32_t i=0; i<clocks; ++i) {
    digitalWrite(PIN_CLK, LOW);
  }
  ++cycles;
}

// Sets up Raspberry PI pins in the begining
static void Setup(void) {  
  Running = true;
  
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

  pinMode(AD0, INPUT);
  pinMode(AD1, INPUT);
  pinMode(AD2, INPUT);
  pinMode(AD3, INPUT);
  pinMode(AD4, INPUT);
  pinMode(AD5, INPUT);
  pinMode(AD6, INPUT);
  pinMode(AD7, INPUT);
  pinMode(A8,  INPUT);
  pinMode(A9,  INPUT);
  pinMode(A10, INPUT);
  pinMode(A11, INPUT);
  pinMode(A12, INPUT);
  pinMode(A13, INPUT);
  pinMode(A14, INPUT);
  pinMode(A15, INPUT);
  pinMode(A16, INPUT);
  pinMode(A17, INPUT);
  pinMode(A18, INPUT);
  pinMode(A19, INPUT);
}

static void Bus_Cycle_Mem_Read_88(uint32_t Address) {
  Data_Bus_Direction_8088_OUT();
  const uint8_t data = pi86MemRead8(Address);
  Write_To_Data_Port_0_7(data);
  CLK();
  CLK();
  Data_Bus_Direction_8088_IN();
}

static void Bus_Cycle_Mem_Write_88(uint32_t Address) {
  const uint8_t data = Read_From_Data_Port_0_7();
  pi86MemWrite8(Address, data);
  CLK();
  CLK();
}

static void Bus_Cycle_Io_Read_88(uint32_t Address) {
  Data_Bus_Direction_8088_OUT();
  const uint8_t data = pi86IoRead8(Address);  
  Write_To_Data_Port_0_7(data);
  CLK();
  CLK();
  Data_Bus_Direction_8088_IN();
}

static void Bus_Cycle_Io_Write_88(uint32_t Address) {
  const uint8_t data = Read_From_Data_Port_0_7();
  pi86IoWrite8(Address, data);
  CLK();
  CLK();
}

static void Int_Ack(void) {
  if (pi86ExtIntAck) {
    pi86ExtIntAck();
  }
}

static void Bus_Cycle_Interrupt_88(void) {
  // Waits for second INTA bus cycle 4 CLKS 8088
  CLK();
  CLK();
  CLK();
  CLK();
  switch (Read_Interrupts()) {
  case 0x01:
  case 0x03:  // note: IRQ0 has priority
    Data_Bus_Direction_8088_OUT();
    Write_To_Data_Port_0_7(0x08);
    CLK();
    CLK();
    Data_Bus_Direction_8088_IN();
    IRQ0_Flag = false;
    digitalWrite(PIN_INTR, LOW);
    break;
  case 0x02:
    Data_Bus_Direction_8088_OUT();
    Write_To_Data_Port_0_7(0x09);
    CLK();
    CLK();
    Data_Bus_Direction_8088_IN();
    IRQ1_Flag = false;
    digitalWrite(PIN_INTR, LOW);
    break;
  }
  Int_Ack();
}

static void Bus_Cycle_88(void) {

    // todo: if no ALE is detected after X cycles the CPU might be in a hlt state

    CLK();
    if (digitalRead(PIN_ALE) != 1) {
      return;
    }

    const uint32_t Address = Read_Address();
    CLK();
    switch (Read_Control_Bus()) {
    // Read Mem
    case 0b100:
      Bus_Cycle_Mem_Read_88(Address);
      break;
    // Write Mem
    case 0b101:
      Bus_Cycle_Mem_Write_88(Address);
      break;
    // Read IO
    case 0b110:
      Bus_Cycle_Io_Read_88(Address);
      break;
    // Write IO
    case 0b111:
      Bus_Cycle_Io_Write_88(Address);
      break;
    // Interrupt
    case 0b010:
      Bus_Cycle_Interrupt_88();
      break;
    }
}

void pi86BusCycle(uint32_t count) {
  while (count--) {
    Bus_Cycle_88();
  }
}

// Resest the x86
void pi86Reset(void) {
  digitalWrite(PIN_RESET, HIGH);
  CLK();
  CLK();
  CLK();
  CLK();
  CLK();
  CLK();
  CLK();
  CLK();
  digitalWrite(PIN_RESET, LOW);
}

void pi86Start() {
  Setup();
  pi86Reset();
}

void pi86Stop(void) {
  Running = false;
}

bool pi86Running(void) {
  return Running;
}

uint64_t pi86CycleCount(void) {
  return cycles;
}