#pragma once

#include "common.h"

typedef struct {
    // 8 bit regiser
    u8 a, f; // F = flags
    u8 b, c;
    u8 d, e;
    u8 h, l;

    // 16 bit program counter and stack pointer
    u16 pc;
    u16 sp;

    u8* cart;
    u32 cart_size;

    // RAM
    u8* memory;

    u8 rom_bank;

    u64 cycles;

    bool halted;
    bool ints_enabled;
    bool int_next;
    u8 IE; // Interrupt Enable (0xFFFF)
    u8 IF; // Interrupt Flags (0xFF0F)

    u8 serial_data;
    u8 serial_control;

    struct bus* _bus;
    struct instruction* optable;
} cpu;

// helpers macros for 16-bit registers
#define AF(cpu) (((cpu)->a << 8) | (cpu)->f)
#define BC(cpu) (((cpu)->b << 8) | (cpu)->c)
#define DE(cpu) (((cpu)->d << 8) | (cpu)->e)
#define HL(cpu) (((cpu)->h << 8) | (cpu)->l)

#define SET_AF(cpu, val) { u16 tmp = val; ((cpu)->a = (tmp) >> 8, (cpu)->f = (tmp) & 0xF0); }
#define SET_BC(cpu, val) { u16 tmp = val; ((cpu)->b = (tmp) >> 8, (cpu)->c = tmp & 0xFF);   }
#define SET_DE(cpu, val) { u16 tmp = val; ((cpu)->d = (tmp) >> 8, (cpu)->e = tmp & 0xFF);   }
#define SET_HL(cpu, val) { u16 tmp = val; ((cpu)->h = (tmp) >> 8, (cpu)->l = tmp & 0xFF);   }

typedef u8(*cpu_inst)(cpu* _cpu);

typedef struct instruction {
    cpu_inst func;
    const char* name; // For debugging purposes
} instruction;

typedef enum {
    CPU_FLAGS_Z = (1 << 7),
    CPU_FLAGS_N = (1 << 6),
    CPU_FLAGS_H = (1 << 5),
    CPU_FLAGS_C = (1 << 4)
} cpu_flags;

cpu* cpu_init(u8* cart, size_t cart_size);
void cpu_unload(cpu* _cpu);
void cpu_handle_interrupts(cpu* c);
void cpu_print_dbg_info(struct bus* b, cpu* c, const instruction* cur_instr);