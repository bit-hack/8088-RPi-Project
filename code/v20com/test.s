BITS 16
org 100h

    xor ax, ax
start:
    out 10h, ax
    inc ax
    jmp start
