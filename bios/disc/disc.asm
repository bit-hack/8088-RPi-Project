[BITS 16]
CPU 186
ORG 0X0000

;Header so it's recognized as an option card
DB 0x55
DB 0xAA
; Uses 8 512-byte pages. Expand if this grows over 4kb.
DB 0x08

    ; Code starts here. Save everything before we start.
    PUSH AX
    PUSH BX
    PUSH CX
    PUSH DX

    PUSH DI
    PUSH SI
    PUSH DS
    PUSH ES

    MOV AX, 0X0000
    MOV DS, AX
    MOV WORD [0X004E], 0XF800
    MOV AX, INT13
    MOV [0X004C], AX

    POP ES
    POP DS
    POP SI
    POP DI

    POP DX
    POP CX
    POP BX
    POP AX
    RETF            ;RETURN

MESSAGE_1:
    MOV AX, 0X0002
    INT 0X10

    MOV AX, CS
    MOV DS, AX
    MOV AH, 0X0E
    MOV CX, .END - .MESSAGE
    MOV BX, .MESSAGE
  .LOOP:
    MOV AL, [BX]
    INT 0X10
    INC BX
    LOOP .LOOP
    RET
  .MESSAGE:
    DB 'BIOS 2.0 Devloped by EMM APR 2023', 0X0D, 0X0A
    DB '8086/V30 or 8088/V20 CPU', 0X0D, 0X0A
    DB '1MB RAM, 64K IO Ports, VGA Graphics', 0X0D, 0X0A
    DB 'bios.bin loaded at 0xF800:0x0000', 0X0D, 0X0A
    DB 'floppy.img mounted - A:', 0X0D, 0X0A
    DB 'hdd.img mounted - C:', 0X0D, 0X0A
  .END:


INT13: ;Diskette/Disk
;;;;;;JUMP TABLE;;;;;
    CLI
    CMP DL, 0X00
    JE .START_INT13

    ;THESE 2 LINES OF CODE ENABLE THE HARD DRIVE!!!!
    CMP DL, 0X80
    JE .START_INT13

    JNE .NOT_A_DRIVE

  .START_INT13:
    CMP AH, 0X00
    JE .RESET_DISK_SYSTEM ;GET STATUS OF LAST OPERATION
    CMP AH, 0X01
    JE .GET_STATUS_LAST_OPERATION
    CMP AH, 0X02
    JE .READ_WRITE_DISK
    CMP AH, 0X03
    JE .READ_WRITE_DISK
    CMP AH, 0X08
    JE .PARAMETERS
    CMP AH, 0X15
    JE .GET_DISK_TYPE

    PUSH AX
    MOV AH, 0X0E
    MOV AL, 'I'
    INT 0X10
    MOV AL, '1'
    INT 0X10
    MOV AL, '3'
    INT 0X10
    MOV AL, ' '
    INT 0X10
    MOV AL, 'e'
    INT 0X10
    MOV AL, 'r'
    INT 0X10
    MOV AL, 'r'
    INT 0X10
    POP AX

    HLT

  .NOT_A_DRIVE:
    MOV AH, 0X01    ;STATUS 0X00 SUCCESSFULL
    STC             ;CLEAR CARRY FLAG SUCCESFUL
    JMP .INT13_END_WITH_CARRY_FLAG
;;;;;;RESET;;;;;;
  .RESET_DISK_SYSTEM:
    MOV AH, 0X00    ;STATUS 0X00 SUCCESSFULL
    CLC             ;CLEAR CARRY FLAG SUCCESFUL
    JMP .INT13_END_WITH_CARRY_FLAG

;;;;;;GET STATUS OF LAST OPERATION;;;;;
  .GET_STATUS_LAST_OPERATION:
    MOV AH, 0X00    ;STATUS 0X00 SUCCESSFULL
    CLC             ;CLEAR CARRY FLAG SUCCESFUL
    JMP .INT13_END_WITH_CARRY_FLAG

;;;;;;READ AND WRITE;;;;;;
  .READ_WRITE_DISK:

    PUSH DS
    PUSH ES
    PUSH DX
    PUSH CX
    PUSH BX
    PUSH AX
    PUSH AX
    PUSH DX
    PUSH DX

    MOV DX, 0XF001
    OUT DX, AL
    ;MOV [CS:0X0001], AL    ;NUMBER OF SECTORS
    MOV AL, CH
    MOV DX, 0XF002
    OUT DX, AL
    ;MOV [CS:0X0002], AL    ;CYLINDER 0-7
    PUSH CX
    MOV CH, CL
    MOV CL, 0X06
    SHR CH, CL
    AND CH, 0X03
    MOV AL, CH
    MOV DX, 0XF003
    OUT DX, AL
    ;MOV [CS:0X0003], AL    ;CYLINDER 8-9
    POP CX
    AND CL, 0X3F
    MOV AL, CL
    MOV DX, 0XF004
    OUT DX, AL
    ;MOV [CS:0X0004], AL    ;SECTOR
    POP DX
    MOV AL, DH
    MOV DX, 0XF005
    OUT DX, AL
    ;MOV [CS:0X0005], AL    ;HEAD
    POP DX
    MOV AL, DL
    MOV DX, 0XF006
    OUT DX, AL
    ;MOV [CS:0X0006], AL    ;DRIVE
    MOV AX, BX
    MOV DX, 0XF007
    OUT DX, AX
    ;MOV [CS:0X0007], AX    ;BX
    MOV AX, ES
    MOV DX, 0XF009
    OUT DX, AX
    ;MOV [CS:0X0009], AX    ;ES

    POP AX
    MOV AL, AH                              ;AH IS THE COMMAND VALUE
    MOV DX, 0XF000
    OUT DX, AL
    ;MOV [CS:0x0000], AL                ;INT13 COMMAND BYTE 0xF000:0X0000
  .READ_DISK_WAIT:                          ;WAIT FOR RASPBERRY PI LOOP
    IN AL, DX
    CMP AL, 0XFF                            ;CHECK TO SEE IF RASPBERRY PI IS DONE
    JNE .READ_DISK_WAIT                     ;LOOP UNTIL COMPLETE

    POP AX          ;AL NEED TO RETURN NUMBER OF SECTORS
    POP BX          ;RESTORE BX
    POP CX          ;RESTORE CX
    POP DX          ;RESTORE DX
    POP ES          ;RESTORE ES
    POP DS          ;RESTORE DS
    MOV AH, 0X00    ;STATUS 0X00 SUCCESSFULL
    CLC             ;CLEAR CARRY FLAG SUCCESFUL
    JMP .INT13_END_WITH_CARRY_FLAG
;;;;;;PARAMETERS;;;;;;
  .PARAMETERS:      ;0x08 RETRUNS DISK PARAMETERS
    CMP DL, 0X80
    JE .HARD_DRIVE_PARAMETERS
    PUSH DS                         ;STORE DS
    PUSH AX                         ;STORE AX

    MOV AX, CS                      ;GET CODE SEGMENT
    MOV DS, AX                      ;SET DATA SEGMENT TO CS
    MOV ES, AX                      ;FOR DRIVE PARAMETER TABLE ES:DI
    POP AX

    MOV AL, DL                      ;SEND DRIVE NUMBER TO RASPBERRY PI
    MOV DX, 0XF006
    OUT DX, AL
    ;MOV [CS:0X0006], AL                    ;DRIVE

    MOV AL, AH                      ;AH IS THE COMMAND VALUE
    MOV DX, 0XF000
    OUT DX, AL
    ;MOV [CS:0x0000], AL                ;INT13 COMMAND BYTE 0xF000:0X0000
  .PARAMETERS_WAIT:                 ;WAIT FOR RASPBERRY PI LOOP
    IN AL, DX
    CMP AL, 0XFF                    ;CHECK TO SEE IF RASPBERRY PI IS DONE
    JNE .PARAMETERS_WAIT            ;LOOP UNTIL COMPLETE

    MOV DX, 0XF00B
    IN AL, DX
    ;MOV AL, [CS:0X000B]                                    ;DRIVE TYPE/MEDIA DESCRIPTOR
    MOV BL, AL                                  ;BL RETURNS DRIVE TYPE
    MOV DX, 0XF011
    IN AL, DX
    ;MOV AL, [CS:0X0011]                                    ;LOW 8 BITS CYLINDER
    MOV CH, AL                                  ;CH RETURNS LOW 8 BITS CYLINDER
    MOV DX, 0XF010
    IN AL, DX
    ;MOV AL, [CS:0X0010]                                    ;SECTORS PER TRACK AND 2 HIGH BITS OF CYLINDER
    MOV CL, AL                                  ;CL RETURNS SECTORS PER TRACK AND 2 HIGH BITS OF CYLINDER
    MOV DX, 0XF00C
    IN AL, DX
    ;MOV AL, [CS:0X000C]                                    ;NUMBER OF HEADS/CLYINDER
    DEC AL                                      ;STARTS AT 0
    MOV DH, AL                                  ;DL RETURNS HEADS/CLYINDER
    MOV DL, 0X01                                ;NUMBER OF DRIVES
    MOV DX, 0XF010
    IN AL, DX
    ;MOV AL, [CS:0X0010]                                    ;SECTORS PER TRACK AND 2 HIGH BITS OF CYLINDER
    AND AL, 0X3F                                ;REMOVE CYLINDER BITS
    DEC AL                                      ;REDUCE BY ONE
    MOV BYTE [.DISK_PARAMETER_TABLE + 4], AL    ;FOR FLOPPY DRIVES DISK PARAMETER TABLE
    MOV DX, 0XF00F
    IN AL, DX
    ;MOV AL, [CS:0X000F]                                    ;BYTES PER SECTOR HIGH SIDE
    MOV BYTE [.DISK_PARAMETER_TABLE + 3], AL    ;FOR FLOPPY DRIVES DISK PARAMETER TABLE
    MOV DI, INT13.DISK_PARAMETER_TABLE          ;FOR DRIVE PARAMETER TABLE ES:DI

    POP DS                          ;RESTORE DS
    MOV AL, 0X00                    ;ON SOME BIOS RETRUN ZERO
    MOV AH, 0X00                    ;STATUS 0X00 SUCCESSFULL
    CLC                             ;CLEAR CARRY FLAG SUCCESFUL
    JMP .INT13_END_WITH_CARRY_FLAG

  .DISK_PARAMETER_TABLE:
    DB 0X00                         ;00   specify byte 1; step-rate time, head unload time
    DB 0X00                         ;01   specify byte 2; head load time, DMA mode
    DB 0X00                         ;02   timer ticks to wait before disk motor shutoff
    DB 0X00                         ;03   bytes per sector code:
                                    ;0 - 128 bytes  2 - 512 bytes
                                    ;1 - 256 bytes  3 - 1024 bytes
    DB 0X00                         ;04   sectors per track (last sector number)
    DB 0X00                         ;05   inter-block gap length/gap between sectors
    DB 0X00                         ;06   data length, if sector length not specified
    DB 0X00                         ;07   gap length between sectors for format
    DB 0X00                         ;08   fill byte for formatted sectors
    DB 0X00                         ;09   head settle time in milliseconds
    DB 0X00                         ;0A   motor startup time in eighths of a second
;;;;;;HARD DRIVE PARAMETERS;;;;;;
.HARD_DRIVE_PARAMETERS:
    MOV AL, DL                      ;SEND DRIVE NUMBER TO RASPBERRY PI
    MOV DX, 0XF006
    OUT DX, AL
    ;MOV [CS:0X0006], AL                    ;DRIVE
    MOV AL, AH                      ;AH IS THE COMMAND VALUE
    MOV DX, 0XF000
    OUT DX, AL
    ;MOV [CS:0x0000], AL                ;INT13 COMMAND BYTE 0xF000:0X0000
  .HARD_DRIVE_PARAMETERS_WAIT:          ;WAIT FOR RASPBERRY PI LOOP
    IN AL, DX
    ;MOV AL, [CS:0x0000]
    CMP AL, 0XFF                    ;CHECK TO SEE IF RASPBERRY PI IS DONE
    JNE .HARD_DRIVE_PARAMETERS_WAIT ;LOOP UNTIL COMPLETE
    MOV DX, 0XF011
    IN AL, DX
    ;MOV AL, [CS:0X0011]                        ;LOW 8 BITS CYLINDER
    MOV CH, AL                      ;CH RETURNS LOW 8 BITS CYLINDER
    MOV DX, 0XF010
    IN AL, DX
    ;MOV AL, [CS:0X0010]                        ;SECTORS PER TRACK AND 2 HIGH BITS OF CYLINDER
    MOV CL, AL                      ;CL RETURNS SECTORS PER TRACK AND 2 HIGH BITS OF CYLINDER
    MOV DX, 0XF00C
    IN AL, DX
    ;MOV AL, [CS:0X000C]                        ;NUMBER OF HEADS/CLYINDER
    DEC AL                          ;STARTS AT 0
    MOV DH, AL                      ;DL RETURNS HEADS/CLYINDER
    MOV DL, 0X01                    ;NUMBER OF DRIVES
    MOV AL, 0X00                    ;ON SOME BIOS RETRUN ZERO
    MOV AH, 0X00                    ;STATUS 0X00 SUCCESSFULL
    CLC                             ;CLEAR CARRY FLAG SUCCESFUL
    JMP .INT13_END_WITH_CARRY_FLAG
;;;;;;GET_DISK_TYPE;;;;;;
  .GET_DISK_TYPE:
    PUSH DX
    MOV AL, DL                      ;SEND DRIVE NUMBER TO RASPBERRY PI
    MOV DX, 0XF006
    OUT DX, AL
    ;MOV [CS:0X0006], AL                    ;DRIVE
    MOV AL, AH                      ;AH IS THE COMMAND VALUE
    MOV DX, 0XF000
    OUT DX, AL
    ;MOV [CS:0x0000], AL                ;INT13 COMMAND BYTE 0xF000:0X0000
  .GET_DISK_TYPE_WAIT:              ;WAIT FOR RASPBERRY PI LOOP
    IN AL, DX
    ;MOV AL, [CS:0x0000]
    CMP AL, 0XFF                    ;CHECK TO SEE IF RASPBERRY PI IS DONE
    JNE .GET_DISK_TYPE_WAIT         ;LOOP UNTIL COMPLETE
    MOV DX, 0XF015
    IN AL, DX
    ;MOV AL, [CS:0X0012]
    ;MOV DL, AL
    ;MOV AL, [CS:0X0013]
    ;MOV DL, AL
    ;MOV AL, [CS:0X0014]
    ;MOV DL, AL
    ;MOV AL, [CS:0X0015]
    POP DX
    MOV DL, AL
    PUSH DX
    MOV DX, 0XF016
    IN AL, DX
    ;MOV AL, [CS:0X0016]
    POP DX
    MOV AH, AL
    MOV AL, 0X00                    ;JUST BECUASE
    CLC                             ;CLEAR CARRY FLAG SUCCESFUL
    JMP .INT13_END_WITH_CARRY_FLAG
;;;;;;END;;;;;;;;
  .INT13_END_WITH_CARRY_FLAG:   ;THIS IS HOW I RETURN THE CARRY FLAG
    PUSH AX                     ;STORE AX
    PUSHF                       ;STORE FLAGS
    POP AX                      ;GET AX = FLAGS
    PUSH BP                     ;STORE BP
    MOV BP, SP                  ;Copy SP to BP for use as index
    ADD BP, 0X08                ;offset 8
    AND WORD [BP], 0XFFFE       ;CLEAR CF = ZER0
    AND AX, 0X0001              ;ONLY CF
    OR  WORD [BP], AX           ;SET CF AX
    POP BP                      ;RESTORE BASE POINTER
    POP AX                      ;RESTORE AX
    STI                         ;SET INTERRUPS FLAG
    IRET                        ;RETRUN
