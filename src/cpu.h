#pragma once

#include "common.h"

typedef enum {
    CPU_FLAGS_Z = (1 << 7),
    CPU_FLAGS_N = (1 << 6),
    CPU_FLAGS_H = (1 << 5),
    CPU_FLAGS_C = (1 << 4)
} cpu_flags;

typedef struct {
    // 8 bit registers
    u8 a, b, c, d, e, h, l;

    // 16 bit registers
    u16 af, bc, de, hl;

    // flags
    u8 f;

    // program counter
    u16 pc;

    // stack pointer
    u16 sp;

    // 64KB base addressing
    u8* memory;
} cpu;

cpu cpu_init(u8* cart);