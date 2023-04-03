#include "x86.h"


uint8_t RAM[0x100000];
uint8_t IO [ 0x10000];

bool Stop_Flag = false;
bool IRQ0_Flag = false;
bool IRQ1_Flag = false;

void IRQ0(void) {
  IRQ0_Flag = true;
  digitalWrite(PIN_INTR, HIGH);
}

void IRQ1(void) {
  IRQ1_Flag = true;
  digitalWrite(PIN_INTR, HIGH);
}

uint8_t Read_Interrupts(void) {
  uint8_t intr = 0;
  intr |= IRQ0_Flag << 0;
  intr |= IRQ1_Flag << 1;
  return intr;
}

//////////////////////////////
// System Bus
///////////////////////////////

uint32_t Read_Address(void) {
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
uint8_t Read_Control_Bus(void) {
  uint8_t Control_Bus = 0;
  Control_Bus |= digitalRead(PIN_DTR)  << 0;
  Control_Bus |= digitalRead(PIN_IO_M) << 1;
  Control_Bus |= digitalRead(PIN_INTA) << 2;
  return Control_Bus;
}

// Reads the selected Memory Bank, High/Low 8086 only
uint8_t Read_Memory_Bank() {
  uint8_t Memory_Bank = 0;
  Memory_Bank |= digitalRead(AD0)     << 0;
  Memory_Bank |= digitalRead(PIN_BHE) << 1;
  return Memory_Bank;
}

// Sets the Data Port direction for read and writes
void Data_Bus_Direction_8088_IN(void) {
  pinMode(AD0, INPUT);
  pinMode(AD1, INPUT);
  pinMode(AD2, INPUT);
  pinMode(AD3, INPUT);
  pinMode(AD4, INPUT);
  pinMode(AD5, INPUT);
  pinMode(AD6, INPUT);
  pinMode(AD7, INPUT);
}

void Data_Bus_Direction_8088_OUT(void) {
  pinMode(AD0, OUTPUT);
  pinMode(AD1, OUTPUT);
  pinMode(AD2, OUTPUT);
  pinMode(AD3, OUTPUT);
  pinMode(AD4, OUTPUT);
  pinMode(AD5, OUTPUT);
  pinMode(AD6, OUTPUT);
  pinMode(AD7, OUTPUT);
}

void Data_Bus_Direction_8086_IN(void) {
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

void Data_Bus_Direction_8086_OUT(void) {
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
void CLK() {
  static const uint32_t clocks = 12;
  for (uint32_t i=0; i<clocks; ++i) {
    digitalWrite(PIN_CLK, HIGH);
  }
  for (int i=0; i<clocks; ++i) {
    digitalWrite(PIN_CLK, LOW);
  }
}

// Sets up Raspberry PI pins in the begining
void Setup(void) {
  Stop_Flag = false;
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

static void Start_System_Bus_88(void) {
  uint32_t Address        = 0;
  uint8_t  Control_Bus    = 0;
  uint8_t  Memory_IO_Bank = 0;

  while (!Stop_Flag) {
    CLK();
    if (digitalRead(PIN_ALE) == 1) {
      Address = Read_Address();
      CLK();
      switch (Read_Control_Bus()) {
      // Read Mem
      case 0x04:
        Data_Bus_Direction_8088_OUT();
        Write_To_Data_Port_0_7(RAM[Address]);
        CLK();
        CLK();
        Data_Bus_Direction_8088_IN();
        break;
      // Write Mem
      case 0x05:
        RAM[Address] = Read_From_Data_Port_0_7();
        CLK();
        CLK();
        break;
      // Read IO
      case 0x06:
        Data_Bus_Direction_8088_OUT();
        Write_To_Data_Port_0_7(IO[Address]);
        printf("Read IO %#X, ", Address);
        printf("Data %#X \n", IO[Address]);
        CLK();
        CLK();
        Data_Bus_Direction_8088_IN();
        break;
      // Write IO
      case 0x07:
        IO[Address] = Read_From_Data_Port_0_7();
        printf("Write IO %#X, ", Address);
        printf("Data %#X \n", Read_From_Data_Port_0_7());
        CLK();
        CLK();
        break;
      // Interrupt
      case 0x02:
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
        default:
          printf("Default Interrupt %#X \n", Read_Interrupts());
          break;
        }
        break;
      default:
        printf("Default \n");
        break;
      }
    }
  }
}

static void Start_System_Bus_86(void) {
  uint32_t Address        = 0;
  uint8_t  Control_Bus    = 0;
  uint8_t  Memory_IO_Bank = 0;

  while (!Stop_Flag) {
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
        default:
          printf("Default Interrupt %#X \n", Read_Interrupts());
          break;
        }
        break;
      default:
        printf("Default \n");
        break;
      }
    }
  }
}

// System Bus decoder
static void Start_System_Bus(int Processor) {
  if (Processor == 88) {
    Start_System_Bus_88();
  }
  if (Processor == 86) {
    Start_System_Bus_86();
  }
}

void Write_Memory_Array(uint32_t Address, const uint8_t *code_for_8088, uint32_t Length) {
  for (int i = 0; i < Length; i++) {
    RAM[Address] = code_for_8088[i];
    Address++;
  }
}

void Read_Memory_Array(uint32_t Address, uint8_t *char_Array, uint32_t Length) {
  for (int i = 0; i < Length; ++i) {
    char_Array[i] = RAM[Address];
    Address++;
  }
}

void Write_Memory_Byte(uint32_t Address, uint8_t byte_for_8088) {
  RAM[Address] = byte_for_8088;
}

uint8_t Read_Memory_Byte(uint32_t Address) {
  return RAM[Address];
}

void Write_Memory_Word(uint32_t Address, uint16_t word_for_8088) {
  RAM[Address + 0] = word_for_8088;
  RAM[Address + 1] = word_for_8088 >> 8;
}

void Write_IO_Byte(uint32_t Address, uint8_t byte_for_8088) {
  IO[Address] = byte_for_8088;
}

uint8_t Read_IO_Byte(uint64_t Address) {
  return IO[Address];
}

void Write_IO_Word(uint32_t Address, uint16_t word_for_8088) {
  IO[Address + 0] = word_for_8088 >> 0;
  IO[Address + 1] = word_for_8088 >> 8;
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

void pi86LoadBios(const string &path) {

#if 0
  FILE *fd = fopen(path.c_str(), "rb");
  if (!fd) {
    return false
  }
#endif

  std::ifstream MemoryFile;              // New ifstream
  MemoryFile.open(path);                 // Open Rom.bin
  MemoryFile.seekg(0, ios::end);         // Find the end of the file
  size_t FileSize = MemoryFile.tellg();  // Get the size of the file
  MemoryFile.seekg(0, MemoryFile.beg);   // Start reading at the begining
  uint8_t Rom[FileSize];                 // New char array the size of the rom file
  MemoryFile.read((char*)Rom, FileSize); // Read the file
  MemoryFile.close();                    // Close the file

  // Jump code to be written to 0xFFFFF, =JMP FAR 0xF000:0X0100
  static const uint8_t FFFF0[] = {
    0XEA, 0X00, 0X01, 0X00, 0XF8, 'E', 'M', ' ', '0',  '4',  '/',  '1',  '0',  '/', '2', '0'
  };

  Write_Memory_Array(0xFFFF0, FFFF0, sizeof(FFFF0)); // Jump Code
  Write_Memory_Array(0xF8000, Rom,   FileSize);     // The Rom file
  Write_Memory_Byte (0xF80FF, 0xFF); // Make sure STOP byte is not zero 0x00 = Stop
  Write_Memory_Byte (0xF8000, 0xFF); // Make sure int13 command port is 0xFF
  Write_Memory_Byte (0xF80F0, 0x03); // Video mode

  // Video port something...?? makes it work
  Write_IO_Byte(0X3DA, 0xFF);
}

uint8_t *memPtr(uint32_t addr) {
  return &RAM[addr];
}
