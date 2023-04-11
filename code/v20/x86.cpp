#include <atomic>

#include "x86.h"


static uint8_t RAM[0x100000];
static uint8_t IO [ 0x10000];

static std::atomic_bool Running   { false };
static std::atomic_bool IRQ0_Flag { false };
static std::atomic_bool IRQ1_Flag { false };

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

// Reads the selected Memory Bank, High/Low 8086 only
static uint8_t Read_Memory_Bank() {
  uint8_t Memory_Bank = 0;
  Memory_Bank |= digitalRead(AD0)     << 0;
  Memory_Bank |= digitalRead(PIN_BHE) << 1;
  return Memory_Bank;
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

static void Data_Bus_Direction_8086_IN(void) {
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
}

static void Data_Bus_Direction_8086_OUT(void) {
  pinMode(AD0, OUTPUT);
  pinMode(AD1, OUTPUT);
  pinMode(AD2, OUTPUT);
  pinMode(AD3, OUTPUT);
  pinMode(AD4, OUTPUT);
  pinMode(AD5, OUTPUT);
  pinMode(AD6, OUTPUT);
  pinMode(AD7, OUTPUT);
  pinMode(A8,  OUTPUT);
  pinMode(A9,  OUTPUT);
  pinMode(A10, OUTPUT);
  pinMode(A11, OUTPUT);
  pinMode(A12, OUTPUT);
  pinMode(A13, OUTPUT);
  pinMode(A14, OUTPUT);
  pinMode(A15, OUTPUT);
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

// Writes Data to Data Port 8-15 8086 only
static void Write_To_Data_Port_8_15(uint8_t Byte) {
  digitalWrite(A8,  (Byte >> 0) & 1);
  digitalWrite(A9,  (Byte >> 1) & 1);
  digitalWrite(A10, (Byte >> 2) & 1);
  digitalWrite(A11, (Byte >> 3) & 1);
  digitalWrite(A12, (Byte >> 4) & 1);
  digitalWrite(A13, (Byte >> 5) & 1);
  digitalWrite(A14, (Byte >> 6) & 1);
  digitalWrite(A15, (Byte >> 7) & 1);
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

// Reads Data to Data Port 8-15 8086 only
static uint8_t Read_From_Data_Port_8_15(void) {
  uint8_t ret = 0;
  ret |= digitalRead(A8)  << 0;
  ret |= digitalRead(A9)  << 1;
  ret |= digitalRead(A10) << 2;
  ret |= digitalRead(A11) << 3;
  ret |= digitalRead(A12) << 4;
  ret |= digitalRead(A13) << 5;
  ret |= digitalRead(A14) << 6;
  ret |= digitalRead(A15) << 7;
  return ret;
}

// Clicks the CLK pin
static void CLK() {
  static const uint32_t clocks = 12;
  for (uint32_t i=0; i<clocks; ++i) {
    digitalWrite(PIN_CLK, HIGH);
  }
  for (int i=0; i<clocks; ++i) {
    digitalWrite(PIN_CLK, LOW);
  }
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
  Write_To_Data_Port_0_7(RAM[Address]);
  CLK();
  CLK();
  Data_Bus_Direction_8088_IN();
}

static void Bus_Cycle_Mem_Write_88(uint32_t Address) {
  RAM[Address] = Read_From_Data_Port_0_7();
  CLK();
  CLK();
}

static void Bus_Cycle_Io_Read_88(uint32_t Address) {
  Data_Bus_Direction_8088_OUT();
  Write_To_Data_Port_0_7(IO[Address]);
  CLK();
  CLK();
  Data_Bus_Direction_8088_IN();
}

static void Bus_Cycle_Io_Write_88(uint32_t Address) {
  IO[Address] = Read_From_Data_Port_0_7();
  CLK();
  CLK();
}

static void Bus_Cycle_Interrupt_88(void) {
  // Waits for second INTA bus cycle 4 CLKS 8088
  CLK();
  CLK();
  CLK();
  CLK();
  switch (Read_Interrupts()) {
  case 0x01:
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
  case 0x03:
    Data_Bus_Direction_8088_OUT();
    Write_To_Data_Port_0_7(0x08);
    CLK();
    CLK();
    Data_Bus_Direction_8088_IN();
    IRQ0_Flag = false;
    break;
  }
}

static void Bus_Cycle_88(void) {

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

static void Start_System_Bus_88(void) {

  while (pi86Running()) {
    CLK();
    if (digitalRead(PIN_ALE) != 1) {
      continue;
    }
    Bus_Cycle_88();
  }
}

static void Start_System_Bus_86(void) {
  uint32_t Address        = 0;
  uint8_t  Control_Bus    = 0;
  uint8_t  Memory_IO_Bank = 0;

  while (pi86Running()) {
    CLK();
    if (digitalRead(PIN_ALE) == 1) {
      Address = Read_Address();
      Memory_IO_Bank = Read_Memory_Bank();
      CLK();
      switch (Read_Control_Bus() + (Memory_IO_Bank << 4)) {
      // Write Mem
      case 0x07:
        RAM[Address + 0] = Read_From_Data_Port_0_7();
        RAM[Address + 1] = Read_From_Data_Port_8_15();
        CLK();
        CLK();
        break;
      case 0x17:
        RAM[Address] = Read_From_Data_Port_8_15();
        CLK();
        CLK();
        break;
      case 0x27:
        RAM[Address] = Read_From_Data_Port_0_7();
        CLK();
        CLK();
        break;
      // Read Mem
      case 0x06:
        Data_Bus_Direction_8086_OUT();
        Write_To_Data_Port_0_7 (RAM[Address + 0]);
        Write_To_Data_Port_8_15(RAM[Address + 1]);
        CLK();
        CLK();
        Data_Bus_Direction_8086_IN();
        break;
      case 0x16:
        Data_Bus_Direction_8086_OUT();
        Write_To_Data_Port_8_15(RAM[Address]);
        CLK();
        CLK();
        Data_Bus_Direction_8086_IN();
        break;
      case 0x26:
        Data_Bus_Direction_8086_OUT();
        Write_To_Data_Port_0_7(RAM[Address]);
        CLK();
        CLK();
        Data_Bus_Direction_8086_IN();
        break;
      // Write IO
      case 0x05:
        IO[Address + 0] = Read_From_Data_Port_0_7();
        IO[Address + 1] = Read_From_Data_Port_8_15();
        CLK();
        CLK();
        break;
      case 0x15:
        IO[Address] = Read_From_Data_Port_8_15();
        CLK();
        CLK();
        break;
      case 0x25:
        IO[Address] = Read_From_Data_Port_0_7();
        CLK();
        CLK();
        break;
      // Read IO
      case 0x04:
        Data_Bus_Direction_8086_OUT();
        Write_To_Data_Port_0_7 (IO[Address + 0]);
        Write_To_Data_Port_8_15(IO[Address + 1]);
        CLK();
        CLK();
        Data_Bus_Direction_8086_IN();
        break;
      case 0x14:
        Data_Bus_Direction_8086_OUT();
        Write_To_Data_Port_8_15(IO[Address]);
        CLK();
        CLK();
        Data_Bus_Direction_8086_IN();
        break;
      case 0x24:
        Data_Bus_Direction_8086_OUT();
        Write_To_Data_Port_0_7(IO[Address]);
        CLK();
        CLK();
        Data_Bus_Direction_8086_IN();
        break;
        // Interrupt
      case 0x00:
        // Waits for second INTA bus cycle 7 CLKS 8086
        CLK();
        CLK();
        CLK();
        CLK();
        CLK();
        CLK();
        CLK();
        switch (Read_Interrupts()) {
        // System Timer
        case 0x01:
          Data_Bus_Direction_8086_OUT();
          Write_To_Data_Port_0_7(0x08);
          CLK();
          CLK();
          Data_Bus_Direction_8086_IN();
          IRQ0_Flag = false;
          digitalWrite(PIN_INTR, LOW);
          break;
        // Keyboard
        case 0x02:
          Data_Bus_Direction_8086_OUT();
          Write_To_Data_Port_0_7(0x09);
          CLK();
          CLK();
          Data_Bus_Direction_8086_IN();
          IRQ1_Flag = false;
          digitalWrite(PIN_INTR, LOW);
          break;
        // System Timer and Keyboard, System Timer is handled
        case 0x03:
          Data_Bus_Direction_8086_OUT();
          Write_To_Data_Port_0_7(0x08);
          CLK();
          CLK();
          Data_Bus_Direction_8086_IN();
          IRQ0_Flag = false;
          break;
        }
        break;
      }
    }
  }
}

// System Bus decoder
static void Start_System_Bus(int model) {
  printf("Starting 80%02d bus\n", model);
  if (model == 88) {
    Start_System_Bus_88();
  }
  if (model == 86) {
    Start_System_Bus_86();
  }
  printf("%s shutdown\n", __func__);
}

void pi86MemWritePtr(uint32_t addr, const uint8_t *src, uint32_t size) {
  for (int i = 0; i < size; i++) {
    RAM[addr] = src[i];
    addr++;
  }
}

void pi86MemReadPtr(uint32_t addr, uint8_t *dst, uint32_t size) {
  for (int i = 0; i < size; ++i) {
    dst[i] = RAM[addr];
    addr++;
  }
}

void pi86MemWrite8(uint32_t addr, uint8_t data) {
  RAM[addr] = data;
}

uint8_t pi86MemRead8(uint32_t addr) {
  return RAM[addr];
}

void pi86MemWrite16(uint32_t addr, uint16_t data) {
  RAM[addr + 0] = data;
  RAM[addr + 1] = data >> 8;
}

void pi86IoWrite8(uint32_t addr, uint8_t data) {
  IO[addr] = data;
}

uint8_t pi86IoRead8(uint64_t addr) {
  return IO[addr];
}

void pi86IoWrite16(uint32_t addr, uint16_t data) {
  IO[addr + 0] = data >> 0;
  IO[addr + 1] = data >> 8;
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

void pi86Start(int Processor) {
  // Sets up Ports
  Setup();
  pi86Reset();
  // Starts the x86 system bus in a thread
  thread System_Bus(Start_System_Bus, Processor);
  // Detach the thread to continue in the program
  System_Bus.detach();
}

bool pi86LoadBios(const string &path) {

  FILE *fd = fopen(path.c_str(), "rb");
  if (!fd) {
    printf("unable to load '%s'\n", path.c_str());
    return false;
  }

  fseek(fd, 0, SEEK_END);
  const size_t biosSize = ftell(fd);
  fseek(fd, 0, SEEK_SET);

  uint8_t *ptr = pi86MemPtr(0xF8000);
  fread(ptr, 1, biosSize, fd);

  fclose(fd);

  // Jump code to be written to 0xFFFFF, =JMP FAR 0xF000:0X0100
  static const uint8_t FFFF0[] = {
    0XEA, 0X00, 0X01, 0X00, 0XF8, 'E', 'M', ' ', '0',  '4',  '/',  '1',  '0',  '/', '2', '0'
  };

  pi86MemWritePtr(0xFFFF0, FFFF0, sizeof(FFFF0)); // Jump Code
  pi86MemWrite8(0xF80FF, 0xFF); // Make sure STOP byte is not zero 0x00 = Stop
  pi86MemWrite8(0xF8000, 0xFF); // Make sure int13 command port is 0xFF
  pi86MemWrite8(0xF80F0, 0x03); // Video mode
  pi86IoWrite8 (0X3DA,   0xFF);

  return true;
}

uint8_t *pi86MemPtr(uint32_t addr) {
  return &RAM[addr];
}

void pi86Stop(void) {
  Running = false;
}

bool pi86Running(void) {
  return Running;
}
