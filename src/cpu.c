#include "cpu.h"
#include "bus.h"
#include "interrupts.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

cpu* cpu_global;

// -- HELPERS --

static INLINE u16 fetch_d16() {
    u8 lo = bus_read(cpu_global->pc++);
    u8 hi = bus_read(cpu_global->pc++);
    return (hi << 8) | lo;
}

static INLINE u8 fetch() {
    return bus_read(cpu_global->pc++);
}

static INLINE void cpu_set_flag(cpu_flags f) { cpu_global->f |= f; }
static INLINE void cpu_clear_flag(cpu_flags f) { cpu_global->f &= ~f; }
static INLINE int cpu_get_flag(cpu_flags f) { return cpu_global->f & f; }

#define proc static INLINE u8

// -- HELPERS --

// -- ALU OPERATIONS -- //

static u8 alu_add_sub(u8 a, u8 value, bool add, u8 carry) {
    u16 result;
    cpu_global->f = 0;

    if (add) {
        result = (u16)a + (u16)value + carry;

        if ((result & 0xFF) == 0) cpu_global->f |= CPU_FLAGS_Z;
        if (((a & 0xF) + (value & 0xF) + carry) > 0xF) cpu_global->f |= CPU_FLAGS_H;
        if (result > 0xFF) cpu_global->f |= CPU_FLAGS_C;
    }
    else {
        result = (u16)a - (u16)value - carry;

        if ((result & 0xFF) == 0) cpu_global->f |= CPU_FLAGS_Z;
        cpu_global->f |= CPU_FLAGS_N;
        if ((a & 0xF) < ((value & 0xF) + carry)) cpu_global->f |= CPU_FLAGS_H;
        if ((u16)a < (u16)value + carry) cpu_global->f |= CPU_FLAGS_C;
    }

    return (u8)result;
}

// -- ALU OPERATIONS -- //

// -- INSTRUCTIONS --

// 0x00-0x0F

proc NOP() {
    // Do nothing
    return 4;
}

proc LD_BC_d16() {
    SET_BC(fetch_d16());
    return 12;
}

proc LD_BC_A() {
    bus_write(BC(), cpu_global->a);
    return 8;
}

proc INC_BC() {
    SET_BC(BC() + 1);
    return 8;
}

proc INC_B() {
    u8 result = cpu_global->b + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((cpu_global->b & 0x0F) + 1 > 0x0F) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    cpu_global->b = result;

    return 4;
}

proc DEC_B() {
    u8 result = cpu_global->b - 1;

    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_set_flag(CPU_FLAGS_N);

    // Half-carry borrow from bit 4
    if ((cpu_global->b & 0x0F) == 0x00) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    cpu_global->b = result;
    return 4;
}

proc LD_B_d8() {
    cpu_global->b = fetch();
    return 8;
}

proc RLCA() {
    u8 old = cpu_global->a;
    cpu_global->a = (old << 1) | (old >> 7); // rotate
    cpu_global->f = 0; // clear Z, N, H
    if (old & 0x80) cpu_set_flag(CPU_FLAGS_C);
    return 4;
}

proc LD_a16_SP() {
    u16 a16 = fetch_d16();
    bus_write(a16, LOBYTE(cpu_global->sp));
    bus_write(a16 + 1, HIBYTE(cpu_global->sp));

    return 20;
}

proc ADD_HL_BC() {
    u32 result = HL() + BC();
    cpu_clear_flag(CPU_FLAGS_N);

    if (((HL() & 0x0FFF) + (BC() & 0x0FFF)) > 0x0FFF)
        cpu_set_flag(CPU_FLAGS_H);
    else
        cpu_clear_flag(CPU_FLAGS_H);

    if (result > 0xFFFF)
        cpu_set_flag(CPU_FLAGS_C);
    else
        cpu_clear_flag(CPU_FLAGS_C);

    SET_HL((u16)result);
    return 8;
}

proc LD_A_BC() {
    cpu_global->a = bus_read(BC());
    return 8;
}

proc DEC_BC() {
    SET_BC(BC() - 1);
    return 8;
}

proc INC_C() {
    u8 result = cpu_global->c + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((cpu_global->c & 0x0F) + 1 > 0x0F) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    cpu_global->c = result;

    return 4;
}

proc DEC_C() {
    u8 result = cpu_global->c - 1;

    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_set_flag(CPU_FLAGS_N);

    if ((cpu_global->c & 0x0F) == 0x00) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    cpu_global->c = result;
    return 4;
}

proc LD_C_d8() {
    cpu_global->c = fetch();
    return 8;
}

proc RRCA() {
    u8 old = cpu_global->a;
    cpu_global->a = (old >> 1) | (old << 7); // rotate right
    cpu_global->f = 0;
    if (old & 0x01) cpu_set_flag(CPU_FLAGS_C);
    return 4;
}

// 0x00-0x0F

// 0x10-0x1F

proc STOP() {
    // Not sure how to implement yet...
    // TODO
    ++cpu_global->pc;
    return 4;
}

proc LD_DE_d16() {
    SET_DE(fetch_d16());
    return 12;
}

proc LD_DE_A() {
    bus_write(DE(), cpu_global->a);
    return 8;
}

proc INC_DE() {
    SET_DE(DE() + 1);
    return 8;
}

proc INC_D() {
    u8 result = cpu_global->d + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((cpu_global->d & 0x0F) + 1 > 0x0F) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    cpu_global->d = result;

    return 4;
}

proc DEC_D() {
    u8 result = cpu_global->d - 1;

    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_set_flag(CPU_FLAGS_N);

    // Half-carry borrow from bit 4
    if ((cpu_global->d & 0x0F) == 0x00) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    cpu_global->d = result;
    return 4;
}

proc LD_D_d8() {
    cpu_global->d = fetch();
    return 8;
}

proc RLA() {
    u8 carry = cpu_get_flag(CPU_FLAGS_C) ? 1 : 0;
    u8 old = cpu_global->a;

    cpu_global->a = (old << 1) | carry;

    cpu_global->f = 0; // clear Z, N, H
    if (old & 0x80) cpu_set_flag(CPU_FLAGS_C);

    return 4;
}

proc JR_s8() {
    i8 jmp = (i8)fetch();
    cpu_global->pc += jmp;
    return 12;
}

proc ADD_HL_DE() {
    u32 result = HL() + DE();
    cpu_clear_flag(CPU_FLAGS_N);

    if (((HL() & 0x0FFF) + (DE() & 0x0FFF)) > 0x0FFF)
        cpu_set_flag(CPU_FLAGS_H);
    else
        cpu_clear_flag(CPU_FLAGS_H);

    if (result > 0xFFFF)
        cpu_set_flag(CPU_FLAGS_C);
    else
        cpu_clear_flag(CPU_FLAGS_C);

    SET_HL((u16)result);
    return 8;
}

proc LD_A_DE() {
    cpu_global->a = bus_read(DE());
    return 8;
}

proc DEC_DE() {
    SET_DE(DE() - 1);
    return 8;
}

proc INC_E() {
    u8 result = cpu_global->e + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((cpu_global->e & 0x0F) + 1 > 0x0F) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    cpu_global->e = result;

    return 4;
}

proc DEC_E() {
    u8 result = cpu_global->e - 1;

    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_set_flag(CPU_FLAGS_N);

    if ((cpu_global->e & 0x0F) == 0x00) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    cpu_global->e = result;
    return 4;
}

proc LD_E_d8() {
    cpu_global->e = fetch();
    return 8;
}

proc RRA() {
    u8 carry = cpu_get_flag(CPU_FLAGS_C) ? 0x80 : 0;
    u8 old = cpu_global->a;

    cpu_global->a = (old >> 1) | carry;

    cpu_global->f = 0;
    if (old & 0x01) cpu_set_flag(CPU_FLAGS_C);

    return 4;
}

// 0x10-0x1F

// 0x20-0x2F

proc JR_NZ_s8() {
    i8 offset = (i8)fetch();  // signed 8-bit
    if (!cpu_get_flag(CPU_FLAGS_Z)) {
        cpu_global->pc += offset;
        return 12; // jump taken
    }
    return 8; // jump not taken
}

proc LD_HL_d16() {
    SET_HL(fetch_d16());
    return 12;
}

proc LD_HLI_A() {
    bus_write(HL(), cpu_global->a);
    SET_HL(HL() + 1);
    return 8;
}

proc INC_HL() {
    SET_HL(HL() + 1);
    return 8;
}

proc INC_H() {
    u8 result = cpu_global->h + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((cpu_global->h & 0x0F) + 1 > 0x0F) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    cpu_global->h = result;

    return 4;
}

proc DEC_H() {
    u8 result = cpu_global->h - 1;

    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_set_flag(CPU_FLAGS_N);

    // Half-carry borrow from bit 4
    if ((cpu_global->h & 0x0F) == 0x00) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    cpu_global->h = result;
    return 4;
}

proc LD_H_d8() {
    cpu_global->h = fetch();
    return 8;
}

proc DAA() {
    u8 a = cpu_global->a;
    u8 adjust = 0;
    bool carry = cpu_get_flag(CPU_FLAGS_C) ? 1 : 0;

    if (!cpu_get_flag(CPU_FLAGS_N)) {
        // after addition
        if (cpu_get_flag(CPU_FLAGS_H) || (a & 0x0F) > 9)
            adjust |= 0x06;
        if (carry || a > 0x99) {
            adjust |= 0x60;
            carry = true;
        }
        a += adjust;
    }
    else {
        // after subtraction
        if (cpu_get_flag(CPU_FLAGS_H))
            adjust |= 0x06;
        if (cpu_get_flag(CPU_FLAGS_C))
            adjust |= 0x60;
        a -= adjust;
    }

    cpu_global->a = a;

    // Zero flag
    if (cpu_global->a == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    // N flag unchanged
    // H always cleared
    cpu_clear_flag(CPU_FLAGS_H);

    // C updated only on addition, preserved on subtraction
    if (carry) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);
    return 8;
}

proc JR_Z_s8() {
    i8 offset = (i8)fetch();  // signed 8-bit
    if (cpu_get_flag(CPU_FLAGS_Z)) {
        cpu_global->pc += offset;
        return 12; // jump taken
    }
    return 8; // jump not taken
}

proc ADD_HL_HL() {
    u32 result = HL() + HL();
    cpu_clear_flag(CPU_FLAGS_N);

    if (((HL() & 0x0FFF) + (HL() & 0x0FFF)) > 0x0FFF)
        cpu_set_flag(CPU_FLAGS_H);
    else
        cpu_clear_flag(CPU_FLAGS_H);

    if (result > 0xFFFF)
        cpu_set_flag(CPU_FLAGS_C);
    else
        cpu_clear_flag(CPU_FLAGS_C);

    SET_HL((u16)result);
    return 8;
}

proc LD_A_HLI() {
    cpu_global->a = bus_read(HL());   // read from memory into A
    SET_HL(HL() + 1);              // increment HL
    return 8;
}

proc DEC_HL() {
    SET_HL(HL() - 1);
    return 8;
}

proc INC_L() {
    u8 result = cpu_global->l + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((cpu_global->l & 0x0F) + 1 > 0x0F) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    cpu_global->l = result;

    return 4;
}

proc DEC_L() {
    u8 result = cpu_global->l - 1;

    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_set_flag(CPU_FLAGS_N);

    if ((cpu_global->l & 0x0F) == 0x00) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    cpu_global->l = result;
    return 4;
}

proc LD_L_d8() {
    cpu_global->l = fetch();
    return 8;
}

proc CPL() {
    cpu_global->a = ~(cpu_global->a);
    cpu_set_flag(CPU_FLAGS_N);
    cpu_set_flag(CPU_FLAGS_H);
    return 4;
}

// 0x20-0x2F

// 0x30-0x3F

proc JR_NC_s8() {
    i8 offset = (i8)fetch();  // signed 8-bit
    if (!cpu_get_flag(CPU_FLAGS_C)) {
        cpu_global->pc += offset;
        return 12; // jump taken
    }
    return 8; // jump not taken
}

proc LD_SP_d16() {
    cpu_global->sp = fetch_d16();
    return 12;
}

proc LD_HLD_A() {
    bus_write(HL(), cpu_global->a);
    SET_HL(HL() - 1);
    return 8;
}

proc INC_SP() {
    cpu_global->sp = cpu_global->sp + 1;
    return 8;
}

proc INC_IND_HL() {
    u8 value = bus_read(HL());
    u8 result = value + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    // N is cleared for INC
    cpu_clear_flag(CPU_FLAGS_N);

    // Half-carry: set when low nibble goes from 0xF -> 0x0
    if ((value & 0x0F) == 0x0F) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    // C is NOT affected by INC
    bus_write(HL(), result);
    return 12;
}

proc DEC_IND_HL() {
    u8 value = bus_read(HL());
    u8 result = value - 1;

    // Zero flag
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    // N flag always set for DEC
    cpu_set_flag(CPU_FLAGS_N);

    // Half-carry: if lower nibble goes from 0 -> F
    if ((value & 0x0F) == 0x00) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    bus_write(HL(), result);

    return 12;
}

proc LD_HL_d8() {
    bus_write(HL(), fetch());
    return 12;
}

proc SCF() {
    cpu_clear_flag(CPU_FLAGS_N);
    cpu_clear_flag(CPU_FLAGS_H);
    cpu_set_flag(CPU_FLAGS_C);
    return 4;
}

proc JR_C_s8() {
    i8 offset = (i8)fetch();  // signed 8-bit
    if (cpu_get_flag(CPU_FLAGS_C)) {
        cpu_global->pc += offset;
        return 12; // jump taken
    }
    return 8; // jump not taken
}

proc ADD_HL_SP() {
    u32 result = HL() + cpu_global->sp;
    cpu_clear_flag(CPU_FLAGS_N);

    if (((HL() & 0x0FFF) + (cpu_global->sp & 0x0FFF)) > 0x0FFF)
        cpu_set_flag(CPU_FLAGS_H);
    else
        cpu_clear_flag(CPU_FLAGS_H);

    if (result > 0xFFFF)
        cpu_set_flag(CPU_FLAGS_C);
    else
        cpu_clear_flag(CPU_FLAGS_C);

    SET_HL((u16)result);
    return 8;
}

proc LD_A_HLD() {
    cpu_global->a = bus_read(HL());   // read from memory into A
    SET_HL(HL() - 1);              // decrement HL
    return 8;
}

proc DEC_SP() {
    cpu_global->sp = cpu_global->sp - 1;
    return 8;
}

proc INC_A() {
    u8 result = cpu_global->a + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((cpu_global->a & 0x0F) + 1 > 0x0F) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    cpu_global->a = result;

    return 4;
}

proc DEC_A() {
    u8 result = cpu_global->a - 1;

    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_set_flag(CPU_FLAGS_N);

    if ((cpu_global->a & 0x0F) == 0x00) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    cpu_global->a = result;
    return 4;
}

proc LD_A_d8() {
    cpu_global->a = fetch();
    return 8;
}

proc CCF() {
    cpu_clear_flag(CPU_FLAGS_N);
    cpu_clear_flag(CPU_FLAGS_H);
    if (cpu_get_flag(CPU_FLAGS_C)) cpu_clear_flag(CPU_FLAGS_C);
    else cpu_set_flag(CPU_FLAGS_C);
    return 4;
}

// 0x30-0x3F

// 0x40-0x4F

proc LD_B_B() {
    cpu_global->b = cpu_global->b;
    return 4;
}

proc LD_B_C() {
    cpu_global->b = cpu_global->c;
    return 4;
}

proc LD_B_D() {
    cpu_global->b = cpu_global->d;
    return 4;
}

proc LD_B_E() {
    cpu_global->b = cpu_global->e;
    return 4;
}

proc LD_B_H() {
    cpu_global->b = cpu_global->h;
    return 4;
}

proc LD_B_L() {
    cpu_global->b = cpu_global->l;
    return 4;
}

proc LD_B_HL() {
    cpu_global->b = bus_read(HL());
    return 8;
}

proc LD_B_A() {
    cpu_global->b = cpu_global->a;
    return 4;
}

proc LD_C_B() {
    cpu_global->c = cpu_global->b;
    return 4;
}

proc LD_C_C() {
    cpu_global->c = cpu_global->c;
    return 4;
}

proc LD_C_D() {
    cpu_global->c = cpu_global->d;
    return 4;
}

proc LD_C_E() {
    cpu_global->c = cpu_global->e;
    return 4;
}

proc LD_C_H() {
    cpu_global->c = cpu_global->h;
    return 4;
}

proc LD_C_L() {
    cpu_global->c = cpu_global->l;
    return 4;
}

proc LD_C_HL() {
    cpu_global->c = bus_read(HL());
    return 8;
}

proc LD_C_A() {
    cpu_global->c = cpu_global->a;
    return 4;
}

// 0x40-0x4F

// 0x50-0x5F

proc LD_D_B() {
    cpu_global->d = cpu_global->b;
    return 4;
}

proc LD_D_C() {
    cpu_global->d = cpu_global->c;
    return 4;
}

proc LD_D_D() {
    cpu_global->d = cpu_global->d;
    return 4;
}

proc LD_D_E() {
    cpu_global->d = cpu_global->e;
    return 4;
}

proc LD_D_H() {
    cpu_global->d = cpu_global->h;
    return 4;
}

proc LD_D_L() {
    cpu_global->d = cpu_global->l;
    return 4;
}

proc LD_D_HL() {
    cpu_global->d = bus_read(HL());
    return 8;
}

proc LD_D_A() {
    cpu_global->d = cpu_global->a;
    return 4;
}

proc LD_E_B() {
    cpu_global->e = cpu_global->b;
    return 4;
}

proc LD_E_C() {
    cpu_global->e = cpu_global->c;
    return 4;
}

proc LD_E_D() {
    cpu_global->e = cpu_global->d;
    return 4;
}

proc LD_E_E() {
    cpu_global->e = cpu_global->e;
    return 4;
}

proc LD_E_H() {
    cpu_global->e = cpu_global->h;
    return 4;
}

proc LD_E_L() {
    cpu_global->e = cpu_global->l;
    return 4;
}

proc LD_E_HL() {
    cpu_global->e = bus_read(HL());
    return 8;
}

proc LD_E_A() {
    cpu_global->e = cpu_global->a;
    return 4;
}

// 0x50-0x5F

// 0x60-0x6F

proc LD_H_B() {
    cpu_global->h = cpu_global->b;
    return 4;
}

proc LD_H_C() {
    cpu_global->h = cpu_global->c;
    return 4;
}

proc LD_H_D() {
    cpu_global->h = cpu_global->d;
    return 4;
}

proc LD_H_E() {
    cpu_global->h = cpu_global->e;
    return 4;
}

proc LD_H_H() {
    cpu_global->h = cpu_global->h;
    return 4;
}

proc LD_H_L() {
    cpu_global->h = cpu_global->l;
    return 4;
}

proc LD_H_HL() {
    cpu_global->h = bus_read(HL());
    return 8;
}

proc LD_H_A() {
    cpu_global->h = cpu_global->a;
    return 4;
}

proc LD_L_B() {
    cpu_global->l = cpu_global->b;
    return 4;
}

proc LD_L_C() {
    cpu_global->l = cpu_global->c;
    return 4;
}

proc LD_L_D() {
    cpu_global->l = cpu_global->d;
    return 4;
}

proc LD_L_E() {
    cpu_global->l = cpu_global->e;
    return 4;
}

proc LD_L_H() {
    cpu_global->l = cpu_global->h;
    return 4;
}

proc LD_L_L() {
    cpu_global->l = cpu_global->l;
    return 4;
}

proc LD_L_HL() {
    cpu_global->l = bus_read(HL());
    return 8;
}

proc LD_L_A() {
    cpu_global->l = cpu_global->a;
    return 4;
}

// 0x60-0x6F

// 0x70-0x7F

proc LD_HL_B() {
    bus_write(HL(), cpu_global->b);
    return 8;
}

proc LD_HL_C() {
    bus_write(HL(), cpu_global->c);
    return 8;
}

proc LD_HL_D() {
    bus_write(HL(), cpu_global->d);
    return 8;
}

proc LD_HL_E() {
    bus_write(HL(), cpu_global->e);
    return 8;
}

proc LD_HL_H() {
    bus_write(HL(), cpu_global->h);
    return 8;
}

proc LD_HL_L() {
    bus_write(HL(), cpu_global->l);
    return 8;
}

proc HALT() {
    cpu_global->halted = true;
    return 4;
}

proc LD_HL_A() {
    bus_write(HL(), cpu_global->a);
    return 8;
}

proc LD_A_B() {
    cpu_global->a = cpu_global->b;
    return 4;
}

proc LD_A_C() {
    cpu_global->a = cpu_global->c;
    return 4;
}

proc LD_A_D() {
    cpu_global->a = cpu_global->d;
    return 4;
}

proc LD_A_E() {
    cpu_global->a = cpu_global->e;
    return 4;
}

proc LD_A_H() {
    cpu_global->a = cpu_global->h;
    return 4;
}

proc LD_A_L() {
    cpu_global->a = cpu_global->l;
    return 4;
}

proc LD_A_HL() {
    cpu_global->a = bus_read(HL());
    return 8;
}

proc LD_A_A() {
    return 4;
}

// 0x80-0x8F

proc ADD_A_B() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->b, true, 0);
    return 4;
}

proc ADD_A_C() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->c, true, 0);
    return 4;
}

proc ADD_A_D() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->d, true, 0);
    return 4;
}

proc ADD_A_E() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->e, true, 0);
    return 4;
}

proc ADD_A_H() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->h, true, 0);
    return 4;
}

proc ADD_A_L() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->l, true, 0);
    return 4;
}

proc ADD_A_HL() {
    u8 value = bus_read(HL());
    cpu_global->a = alu_add_sub(cpu_global->a, value, true, 0);
    return 8;
}

proc ADD_A_A() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->a, true, 0);
    return 4;
}

proc ADC_A_B() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->b, true, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc ADC_A_C() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->c, true, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc ADC_A_D() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->d, true, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc ADC_A_E() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->e, true, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc ADC_A_H() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->h, true, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc ADC_A_L() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->l, true, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc ADC_A_HL() {
    u8 value = bus_read(HL());
    cpu_global->a = alu_add_sub(cpu_global->a, value, true, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 8;
}

proc ADC_A_A() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->a, true, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

// 0x80-0x8F

// 0x90-0x9F

proc SUB_A_B() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->b, false, 0);
    return 4;
}

proc SUB_A_C() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->c, false, 0);
    return 4;
}

proc SUB_A_D() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->d, false, 0);
    return 4;
}

proc SUB_A_E() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->e, false, 0);
    return 4;
}

proc SUB_A_H() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->h, false, 0);
    return 4;
}

proc SUB_A_L() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->l, false, 0);
    return 4;
}

proc SUB_A_HL() {
    u8 value = bus_read(HL());
    cpu_global->a = alu_add_sub(cpu_global->a, value, false, 0);
    return 8; // cycles
}

proc SUB_A_A() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->a, false, 0);
    return 4;
}

proc SBC_A_B() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->b, false, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc SBC_A_C() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->c, false, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc SBC_A_D() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->d, false, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc SBC_A_E() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->e, false, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc SBC_A_H() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->h, false, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc SBC_A_L() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->l, false, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc SBC_A_HL() {
    u8 value = bus_read(HL());
    cpu_global->a = alu_add_sub(cpu_global->a, value, false, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 8;
}

proc SBC_A_A() {
    cpu_global->a = alu_add_sub(cpu_global->a, cpu_global->a, false, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

// 0x90-0x9F

// 0xA0-0xAF

proc AND_A_B() {
    u8 result = cpu_global->a & cpu_global->b;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);
    cpu_set_flag(CPU_FLAGS_H);
    cpu_clear_flag(CPU_FLAGS_C);

    cpu_global->a = result;
    return 4;
}

proc AND_A_C() {
    u8 result = cpu_global->a & cpu_global->c;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);
    cpu_set_flag(CPU_FLAGS_H);
    cpu_clear_flag(CPU_FLAGS_C);

    cpu_global->a = result;
    return 4;
}

proc AND_A_D() {
    u8 result = cpu_global->a & cpu_global->d;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);
    cpu_set_flag(CPU_FLAGS_H);
    cpu_clear_flag(CPU_FLAGS_C);

    cpu_global->a = result;
    return 4;
}

proc AND_A_E() {
    u8 result = cpu_global->a & cpu_global->e;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);
    cpu_set_flag(CPU_FLAGS_H);
    cpu_clear_flag(CPU_FLAGS_C);

    cpu_global->a = result;
    return 4;
}

proc AND_A_H() {
    u8 result = cpu_global->a & cpu_global->h;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);
    cpu_set_flag(CPU_FLAGS_H);
    cpu_clear_flag(CPU_FLAGS_C);

    cpu_global->a = result;
    return 4;
}

proc AND_A_L() {
    u8 result = cpu_global->a & cpu_global->l;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);
    cpu_set_flag(CPU_FLAGS_H);
    cpu_clear_flag(CPU_FLAGS_C);

    cpu_global->a = result;
    return 4;
}

proc AND_A_HL() {
    u8 result = cpu_global->a & bus_read(HL());
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);
    cpu_set_flag(CPU_FLAGS_H);
    cpu_clear_flag(CPU_FLAGS_C);

    cpu_global->a = result;
    return 8;
}

proc AND_A_A() {
    u8 result = cpu_global->a & cpu_global->a;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);
    cpu_set_flag(CPU_FLAGS_H);
    cpu_clear_flag(CPU_FLAGS_C);

    cpu_global->a = result;
    return 4;
}

proc XOR_A_B() {
    u8 result = cpu_global->a ^ cpu_global->b;
    
    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 4;
}

proc XOR_A_C() {
    u8 result = cpu_global->a ^ cpu_global->c;

    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 4;
}

proc XOR_A_D() {
    u8 result = cpu_global->a ^ cpu_global->d;

    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 4;
}

proc XOR_A_E() {
    u8 result = cpu_global->a ^ cpu_global->e;

    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 4;
}

proc XOR_A_H() {
    u8 result = cpu_global->a ^ cpu_global->h;

    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 4;
}

proc XOR_A_L() {
    u8 result = cpu_global->a ^ cpu_global->l;

    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 4;
}

proc XOR_A_HL() {
    u8 result = cpu_global->a ^ bus_read(HL());

    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 8;
}

proc XOR_A_A() {
    cpu_global->a = 0;
    cpu_global->f = CPU_FLAGS_Z; // only Z is set
    return 4;
}

// 0xA0-0xAF

// 0xB0-0xBF

proc OR_A_B() {
    u8 result = cpu_global->a | cpu_global->b;

    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 4;
}

proc OR_A_C() {
    u8 result = cpu_global->a | cpu_global->c;

    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 4;
}

proc OR_A_D() {
    u8 result = cpu_global->a | cpu_global->d;

    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 4;
}

proc OR_A_E() {
    u8 result = cpu_global->a | cpu_global->e;

    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 4;
}

proc OR_A_H() {
    u8 result = cpu_global->a | cpu_global->h;

    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 4;
}

proc OR_A_L() {
    u8 result = cpu_global->a | cpu_global->l;

    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 4;
}

proc OR_A_HL() {
    u8 result = cpu_global->a | bus_read(HL());

    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 8;
}

proc OR_A_A() {
    u8 result = cpu_global->a | cpu_global->a;  // just A
    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;
    return 4;
}

proc CP_A_B() {
    u8 value = cpu_global->b;
    u16 result = cpu_global->a - value;
    
    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);
    
    // N always set (since it's subtraction)
    cpu_set_flag(CPU_FLAGS_N);
    
    // Half-carry (borrow from bit 4)
    if ((cpu_global->a & 0x0F) < (value & 0x0F)) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);
    
    // Carry (borrow from bit 8)
    if (cpu_global->a < value) cpu_set_flag(CPU_FLAGS_C); 
    else cpu_clear_flag(CPU_FLAGS_C);
    
    return 4; // cycles
}

proc CP_A_C() {
    u8 value = cpu_global->c;
    u16 result = cpu_global->a - value;

    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    // N always set (since it's subtraction)
    cpu_set_flag(CPU_FLAGS_N);

    // Half-carry (borrow from bit 4)
    if ((cpu_global->a & 0x0F) < (value & 0x0F)) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    // Carry (borrow from bit 8)
    if (cpu_global->a < value) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);

    return 4; // cycles
}

proc CP_A_D() {
    u8 value = cpu_global->d;
    u16 result = cpu_global->a - value;

    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    // N always set (since it's subtraction)
    cpu_set_flag(CPU_FLAGS_N);

    // Half-carry (borrow from bit 4)
    if ((cpu_global->a & 0x0F) < (value & 0x0F)) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    // Carry (borrow from bit 8)
    if (cpu_global->a < value) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);

    return 4; // cycles
}

proc CP_A_E() {
    u8 value = cpu_global->e;
    u16 result = cpu_global->a - value;

    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    // N always set (since it's subtraction)
    cpu_set_flag(CPU_FLAGS_N);

    // Half-carry (borrow from bit 4)
    if ((cpu_global->a & 0x0F) < (value & 0x0F)) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    // Carry (borrow from bit 8)
    if (cpu_global->a < value) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);

    return 4; // cycles
}

proc CP_A_H() {
    u8 value = cpu_global->h;
    u16 result = cpu_global->a - value;

    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    // N always set (since it's subtraction)
    cpu_set_flag(CPU_FLAGS_N);

    // Half-carry (borrow from bit 4)
    if ((cpu_global->a & 0x0F) < (value & 0x0F)) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    // Carry (borrow from bit 8)
    if (cpu_global->a < value) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);

    return 4; // cycles
}

proc CP_A_L() {
    u8 value = cpu_global->l;
    u16 result = cpu_global->a - value;

    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    // N always set (since it's subtraction)
    cpu_set_flag(CPU_FLAGS_N);

    // Half-carry (borrow from bit 4)
    if ((cpu_global->a & 0x0F) < (value & 0x0F)) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    // Carry (borrow from bit 8)
    if (cpu_global->a < value) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);

    return 4; // cycles
}

proc CP_A_HL() {
    u8 value = bus_read(HL());
    u16 result = cpu_global->a - value;

    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    // N always set (since it's subtraction)
    cpu_set_flag(CPU_FLAGS_N);

    // Half-carry (borrow from bit 4)
    if ((cpu_global->a & 0x0F) < (value & 0x0F)) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    // Carry (borrow from bit 8)
    if (cpu_global->a < value) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);

    return 8; // cycles
}

proc CP_A_A() {
    u8 value = cpu_global->a;
    u16 result = cpu_global->a - value;

    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    // N always set (since it's subtraction)
    cpu_set_flag(CPU_FLAGS_N);

    // Half-carry (borrow from bit 4)
    if ((cpu_global->a & 0x0F) < (value & 0x0F)) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    // Carry (borrow from bit 8)
    if (cpu_global->a < value) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);

    return 4; // cycles
}

// 0xB0-0xBF

// 0xC0-0xCF

proc RET_NZ() {
    if (!cpu_get_flag(CPU_FLAGS_Z)) {
        // Pop 16 bit address from stack (little endian)
        u16 addr = bus_read(cpu_global->sp) | (bus_read(cpu_global->sp + 1) << 8);
        cpu_global->sp += 2;
        cpu_global->pc = addr;
        return 20; // Return taken
    }

    return 8; // Return not taken
}

proc POP_BC() {
    u8 low = bus_read(cpu_global->sp);
    u8 high = bus_read(cpu_global->sp + 1);
    cpu_global->sp += 2;

    cpu_global->b = high;
    cpu_global->c = low;

    return 12;
}

proc JP_NZ_a16() {
    u16 addr = fetch_d16();
    if (!cpu_get_flag(CPU_FLAGS_Z)) {
        cpu_global->pc = addr;
        return 16; // jump taken
    }
    return 12; // jump not taken
}

proc JP_a16() {
    u16 addr = fetch_d16();
    cpu_global->pc = addr;
    return 16;
}

proc CALL_NZ_a16() {
    u16 addr = fetch_d16();

    if (!cpu_get_flag(CPU_FLAGS_Z)) {
        cpu_global->sp -= 2;
        bus_write(cpu_global->sp, (cpu_global->pc & 0xFF));
        bus_write(cpu_global->sp + 1, (cpu_global->pc >> 8) & 0xFF);

        cpu_global->pc = addr;
        return 24;
    }

    return 12;
}

proc PUSH_BC() {
    cpu_global->sp--;
    bus_write(cpu_global->sp, cpu_global->b);  // high byte
    cpu_global->sp--;
    bus_write(cpu_global->sp, cpu_global->c);  // low byte
    return 16;
}

proc ADD_A_d8() {
    u8 value = fetch();
    cpu_global->a = alu_add_sub(cpu_global->a, value, true, 0);
    return 8;
}

proc RST_00() {
    cpu_global->sp -= 2;
    bus_write(cpu_global->sp, cpu_global->pc & 0xFF);       // low byte
    bus_write(cpu_global->sp + 1, (cpu_global->pc >> 8) & 0xFF); // high byte

    cpu_global->pc = 0x00;
    return 16;
}

proc RET_Z() {
    if (cpu_get_flag(CPU_FLAGS_Z)) {
        // Pop 16 bit address from stack (little endian)
        u16 addr = bus_read(cpu_global->sp) | (bus_read(cpu_global->sp + 1) << 8);
        cpu_global->sp += 2;
        cpu_global->pc = addr;
        return 20; // Return taken
    }

    return 8; // Return not taken
}

proc RET() {
    // Pop 16 bit address from stack (little endian)
    u16 addr = bus_read(cpu_global->sp) | (bus_read(cpu_global->sp + 1) << 8);
    cpu_global->sp += 2;
    cpu_global->pc = addr;
    return 16; // Return taken
}

proc JP_Z_a16() {
    u16 addr = fetch_d16();
    if (cpu_get_flag(CPU_FLAGS_Z)) {
        cpu_global->pc = addr;
        return 16; // jump taken
    }
    return 12; // jump not taken
}

// -- CB PREFIXED --

// Rotate Left Circular (RLC) - bit7 -> bit0 & carry
static inline u8 rlc(u8 val) {
    u8 res = (val << 1) | (val >> 7);

    cpu_clear_flag(CPU_FLAGS_N | CPU_FLAGS_H);
    if (res == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);
    if (val & 0x80) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);

    return res;
}

// Rotate Right Circular (RRC) - bit0 -> bit7 & carry
static inline u8 rrc(u8 val) {
    u8 res = (val >> 1) | (val << 7);

    cpu_clear_flag(CPU_FLAGS_N | CPU_FLAGS_H);
    if (res == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);
    if (val & 0x01) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);

    return res;
}

// Rotate Left through Carry (RL)
static inline u8 rl(u8 val) {
    u8 carry_in = cpu_get_flag(CPU_FLAGS_C) ? 1 : 0;
    u8 carry_out = (val & 0x80) ? 1 : 0;
    u8 res = (val << 1) | carry_in;

    cpu_clear_flag(CPU_FLAGS_N | CPU_FLAGS_H);
    if (res == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);
    if (carry_out) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);

    return res;
}

// Rotate Right through Carry (RR)
static inline u8 rr(u8 val) {
    u8 carry_in = cpu_get_flag(CPU_FLAGS_C) ? 0x80 : 0;
    u8 carry_out = (val & 0x01) ? 1 : 0;
    u8 res = (val >> 1) | carry_in;

    cpu_clear_flag(CPU_FLAGS_N | CPU_FLAGS_H);
    if (res == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);
    if (carry_out) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);

    return res;
}

// Shift Left Arithmetic (SLA) - bit7 -> carry, fill 0
static inline u8 sla(u8 val) {
    u8 carry_out = (val & 0x80) ? 1 : 0;
    u8 res = val << 1;

    cpu_clear_flag(CPU_FLAGS_N | CPU_FLAGS_H);
    if (res == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);
    if (carry_out) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);

    return res;
}

// Shift Right Arithmetic (SRA) - keep bit7, bit0 -> carry
static inline u8 sra(u8 val) {
    u8 carry_out = (val & 0x01) ? 1 : 0;
    u8 res = (val >> 1) | (val & 0x80);

    cpu_clear_flag(CPU_FLAGS_N | CPU_FLAGS_H);
    if (res == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);
    if (carry_out) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);

    return res;
}

// Swap upper/lower nibbles
static inline u8 swap(u8 val) {
    u8 res = (val >> 4) | (val << 4);

    cpu_clear_flag(CPU_FLAGS_N | CPU_FLAGS_H | CPU_FLAGS_C);
    if (res == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    return res;
}

// Shift Right Logical (SRL) - bit0 -> carry, fill 0
static inline u8 srl(u8 val) {
    u8 carry_out = (val & 0x01) ? 1 : 0;
    u8 res = val >> 1;

    cpu_clear_flag(CPU_FLAGS_N | CPU_FLAGS_H);
    if (res == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);
    if (carry_out) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);

    return res;
}

// Test bit b of val
static inline void bit(int b, u8 val) {
    if (val & (1 << b)) cpu_clear_flag(CPU_FLAGS_Z);
    else cpu_set_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);
    cpu_set_flag(CPU_FLAGS_H);
    // C is unchanged
}

// Reset bit b
static inline u8 res(int b, u8 val) {
    return val & ~(1 << b);
}

// Set bit b
static inline u8 set(int b, u8 val) {
    return val | (1 << b);
}

// -- CB PREFIXED --

// Map reg index (0–7) to actual register
static inline u8* decode_reg(int idx) {
    switch (idx) {
        case 0: return &cpu_global->b;
        case 1: return &cpu_global->c;
        case 2: return &cpu_global->d;
        case 3: return &cpu_global->e;
        case 4: return &cpu_global->h;
        case 5: return &cpu_global->l;
        case 6: return NULL; // special: (HL)
        case 7: return &cpu_global->a;
    }
    return NULL; // should never happen
}

proc PREFIX() {
    u8 op = fetch();
    int reg_idx = op & 0x07;
    u8* reg = decode_reg(reg_idx);
    u8 val = reg ? *reg : bus_read(HL());

    int cycles;
    if (reg) cycles = 8;
    else if (op < 0x40 || op >= 0x80) cycles = 16; // modifies memory
    else cycles = 12; // BIT b,(HL)

    if (op < 0x40) {
        int group = (op >> 3) & 7;
        switch (group) {
            case 0: val = rlc(val); break;
            case 1: val = rrc(val); break;
            case 2: val = rl(val);  break;
            case 3: val = rr(val);  break;
            case 4: val = sla(val); break;
            case 5: val = sra(val); break;
            case 6: val = swap(val); break;
            case 7: val = srl(val); break;
        }
    }
    else if (op < 0x80) {
        bit((op >> 3) & 7, val);
    }
    else if (op < 0xC0) {
        val = res((op >> 3) & 7, val);
    }
    else {
        val = set((op >> 3) & 7, val);
    }

    if (reg) *reg = val;
    else if (op < 0x40 || op >= 0x80) bus_write(HL(), val);

    return (u8)cycles;
}

proc CALL_Z_a16() {
    u16 addr = fetch_d16();

    if (cpu_get_flag(CPU_FLAGS_Z)) {
        cpu_global->sp -= 2;
        bus_write(cpu_global->sp, (cpu_global->pc & 0xFF));
        bus_write(cpu_global->sp + 1, (cpu_global->pc >> 8) & 0xFF);

        cpu_global->pc = addr;
        return 24;
    }

    return 12;
}

proc CALL_a16() {
    u16 addr = fetch_d16();

    cpu_global->sp -= 2;
    bus_write(cpu_global->sp, (cpu_global->pc & 0xFF));
    bus_write(cpu_global->sp + 1, (cpu_global->pc >> 8) & 0xFF);

    cpu_global->pc = addr;
    return 24;
}

proc ADC_A_d8() {
    u8 value = fetch();
    cpu_global->a = alu_add_sub(cpu_global->a, value, true, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 8;
}

proc RST_08() {
    cpu_global->sp -= 2;

    bus_write(cpu_global->sp, cpu_global->pc & 0xFF);       // low byte
    bus_write(cpu_global->sp + 1, (cpu_global->pc >> 8) & 0xFF); // high byte

    cpu_global->pc = 0x08;
    return 16;
}

// 0xC0-0xCF

// 0xD0-0xDF

proc RET_NC() {
    if (!cpu_get_flag(CPU_FLAGS_C)) {
        // Pop 16 bit address from stack (little endian)
        u16 addr = bus_read(cpu_global->sp) | (bus_read(cpu_global->sp + 1) << 8);
        cpu_global->sp += 2;
        cpu_global->pc = addr;
        return 20; // Return taken
    }

    return 8; // Return not taken
}

proc POP_DE() {
    u8 low = bus_read(cpu_global->sp);
    u8 high = bus_read(cpu_global->sp + 1);
    cpu_global->sp += 2;

    cpu_global->d = high;
    cpu_global->e = low;

    return 12;
}

proc JP_NC_a16() {
    u16 addr = fetch_d16();
    if (!cpu_get_flag(CPU_FLAGS_C)) {
        cpu_global->pc = addr;
        return 16; // jump taken
    }
    return 12; // jump not taken
}

proc CALL_NC_a16() {
    u16 addr = fetch_d16();

    if (!cpu_get_flag(CPU_FLAGS_C)) {
        cpu_global->sp -= 2;
        bus_write(cpu_global->sp, (cpu_global->pc & 0xFF));
        bus_write(cpu_global->sp + 1, (cpu_global->pc >> 8) & 0xFF);

        cpu_global->pc = addr;
        return 24;
    }

    return 12;
}

proc PUSH_DE() {
    cpu_global->sp--;
    bus_write(cpu_global->sp, cpu_global->d);
    cpu_global->sp--;
    bus_write(cpu_global->sp, cpu_global->e);

    return 16;
}

proc SUB_A_d8() {
    u8 value = fetch();
    cpu_global->a = alu_add_sub(cpu_global->a, value, false, 0);
    return 8;
}

proc RST_10() {
    cpu_global->sp -= 2;
    bus_write(cpu_global->sp, cpu_global->pc & 0xFF);       // low byte
    bus_write(cpu_global->sp + 1, (cpu_global->pc >> 8) & 0xFF); // high byte

    cpu_global->pc = 0x10;
    return 16;
}

proc RET_C() {
    if (cpu_get_flag(CPU_FLAGS_C)) {
        // Pop 16 bit address from stack (little endian)
        u16 addr = bus_read(cpu_global->sp) | (bus_read(cpu_global->sp + 1) << 8);
        cpu_global->sp += 2;
        cpu_global->pc = addr;
        return 20; // Return taken
    }

    return 8; // Return not taken
}

proc RETI() {
    // Pop 16 bit address from stack (little endian)
    u16 addr = bus_read(cpu_global->sp) | (bus_read(cpu_global->sp + 1) << 8);
    cpu_global->sp += 2;
    cpu_global->pc = addr;
    cpu_global->ints_enabled = 1;
    return 16; // Return taken
}

proc JP_C_a16() {
    u16 addr = fetch_d16();
    if (cpu_get_flag(CPU_FLAGS_C)) {
        cpu_global->pc = addr;
        return 16; // jump taken
    }
    return 12; // jump not taken
}

proc CALL_C_a16() {
    u16 addr = fetch_d16();

    if (cpu_get_flag(CPU_FLAGS_C)) {
        cpu_global->sp -= 2;
        bus_write(cpu_global->sp, (cpu_global->pc & 0xFF));
        bus_write(cpu_global->sp + 1, (cpu_global->pc >> 8) & 0xFF);

        cpu_global->pc = addr;
        return 24;
    }

    return 12;
}

proc SBC_A_d8() {
    u8 value = fetch();
    cpu_global->a = alu_add_sub(cpu_global->a, value, false, cpu_get_flag(CPU_FLAGS_C) ? 1 : 0);
    return 8;
}

proc RST_18() {
    cpu_global->sp -= 2;
    bus_write(cpu_global->sp, cpu_global->pc & 0xFF);            // low byte
    bus_write(cpu_global->sp + 1, (cpu_global->pc >> 8) & 0xFF); // high byte

    cpu_global->pc = 0x18;
    return 16;
}

// 0xD0-0xDF

// 0xE0-0xEF

proc LDH_a8_A() {
    u8 value = fetch();
    bus_write(0xFF00 + value, cpu_global->a);
    return 48;
}

proc POP_HL() {
    u8 low = bus_read(cpu_global->sp);
    u8 high = bus_read(cpu_global->sp + 1);
    cpu_global->sp += 2;

    cpu_global->h = high;
    cpu_global->l = low;

    return 12;
}

proc LDH_C_A() {
    bus_write(0xFF00 + cpu_global->c, cpu_global->a);
    return 8;
}

proc PUSH_HL() {
    cpu_global->sp--;
    bus_write(cpu_global->sp, cpu_global->h);
    cpu_global->sp--;
    bus_write(cpu_global->sp, cpu_global->l);

    return 16;
}

proc AND_A_d8() {
    u8 result = cpu_global->a & fetch();
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    cpu_clear_flag(CPU_FLAGS_N);
    cpu_set_flag(CPU_FLAGS_H);
    cpu_clear_flag(CPU_FLAGS_C);

    cpu_global->a = result;
    return 8;
}

proc RST_20() {
    cpu_global->sp -= 2;
    bus_write(cpu_global->sp, cpu_global->pc & 0xFF);       // low byte
    bus_write(cpu_global->sp + 1, (cpu_global->pc >> 8) & 0xFF); // high byte

    cpu_global->pc = 0x20;
    return 16;
}

proc ADD_SP_i8() {
    i8 imm = (i8)fetch();
    u16 sp = cpu_global->sp;
    u16 result = sp + imm;

    cpu_clear_flag(CPU_FLAGS_Z);
    cpu_clear_flag(CPU_FLAGS_N);

    // Half-carry: nibble carry
    if (((sp & 0x0F) + (imm & 0x0F)) > 0x0F)
        cpu_set_flag(CPU_FLAGS_H);
    else
        cpu_clear_flag(CPU_FLAGS_H);

    // Carry: byte carry
    if (((sp & 0xFF) + (imm & 0xFF)) > 0xFF)
        cpu_set_flag(CPU_FLAGS_C);
    else
        cpu_clear_flag(CPU_FLAGS_C);

    cpu_global->sp = result;

    return 16;
}

proc JP_HL() {
    cpu_global->pc = HL();
    return 4;
}

proc LD_a16_A() {
    u16 value = fetch_d16();
    bus_write(value, cpu_global->a);
    return 16;
}

proc XOR_A_d8() {
    u8 result = cpu_global->a ^ fetch();

    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 8;
}

proc RST_28() {
    cpu_global->sp -= 2;
    bus_write(cpu_global->sp, cpu_global->pc & 0xFF);       // low byte
    bus_write(cpu_global->sp + 1, (cpu_global->pc >> 8) & 0xFF); // high byte

    cpu_global->pc = 0x28;
    return 16;
}

// 0xE0-0xEF

// 0xF0-0xFF

proc LDH_A_a8() {
    u8 imm = fetch();
    u8 value = bus_read(0xFF00 + imm);
    cpu_global->a = value;
    return 48;
}

proc POP_AF() {
    u8 low = bus_read(cpu_global->sp);
    u8 high = bus_read(cpu_global->sp + 1);
    cpu_global->sp += 2;
    cpu_global->a = high;
    cpu_global->f = low & 0xF0;

    return 12;
}

proc LDH_A_C() {
    cpu_global->a = bus_read(0xFF00 + cpu_global->c);
    return 8;
}

proc DI() {
    cpu_global->ints_enabled = false;
    cpu_global->int_next = false; // cancel pending EI
    return 4;
}

proc PUSH_AF() {
    cpu_global->sp--;
    bus_write(cpu_global->sp, cpu_global->a);
    cpu_global->sp--;
    bus_write(cpu_global->sp, cpu_global->f & 0xF0);

    return 16;
}

proc OR_A_d8() {
    u8 result = cpu_global->a | fetch();

    cpu_global->f = 0;
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    cpu_global->a = result;

    return 4;
}

proc RST_30() {
    cpu_global->sp -= 2;
    bus_write(cpu_global->sp, cpu_global->pc & 0xFF);       // low byte
    bus_write(cpu_global->sp + 1, (cpu_global->pc >> 8) & 0xFF); // high byte

    cpu_global->pc = 0x30;
    return 16;
}

proc LD_HL_SP_PLUS_i8() {
    i8 offset = (i8)fetch();
    u16 sp = cpu_global->sp;
    u16 result = sp + offset;

    cpu_clear_flag(CPU_FLAGS_Z);
    cpu_clear_flag(CPU_FLAGS_N);

    // Half-carry: check nibble carry from (sp & 0xF) + (offset & 0xF)
    if (((sp & 0x0F) + (offset & 0x0F)) > 0x0F)
        cpu_set_flag(CPU_FLAGS_H);
    else
        cpu_clear_flag(CPU_FLAGS_H);

    // Carry: check byte carry from (sp & 0xFF) + (offset & 0xFF)
    if (((sp & 0xFF) + (offset & 0xFF)) > 0xFF)
        cpu_set_flag(CPU_FLAGS_C);
    else
        cpu_clear_flag(CPU_FLAGS_C);

    SET_HL(result);

    return 12;
}

proc LD_SP_HL() {
    cpu_global->sp = HL();
    return 8;
}

proc LD_A_a16() {
    u16 value = fetch_d16();
    cpu_global->a = bus_read(value);
    return 16;
}

proc EI() {
    cpu_global->int_next = true;
    return 4;
}

proc CP_A_d8() {
    u8 value = fetch();
    u8 a = cpu_global->a;
    u8 result = a - value;

    // Zero flag
    if (result == 0) cpu_set_flag(CPU_FLAGS_Z);
    else cpu_clear_flag(CPU_FLAGS_Z);

    // Subtract flag
    cpu_set_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((a & 0x0F) < (value & 0x0F)) cpu_set_flag(CPU_FLAGS_H);
    else cpu_clear_flag(CPU_FLAGS_H);

    if (a < value) cpu_set_flag(CPU_FLAGS_C);
    else cpu_clear_flag(CPU_FLAGS_C);

    return 8; // 8 T-cycles
}

proc RST_38() {
    cpu_global->sp -= 2;
    bus_write(cpu_global->sp, cpu_global->pc & 0xFF);            // low byte
    bus_write(cpu_global->sp + 1, (cpu_global->pc >> 8) & 0xFF); // high byte
    cpu_global->pc = 0x38;;
    return 16;
}

// 0xF0-0xFF

proc INVALID() {
    printf("INVALID INSTRUCTION EXECUTED!\n");
    exit(1);
    return 4;
}

// -- INSTRUCTIONS --

void cpu_init(u8* cart, size_t cart_size) {
    cpu_global = malloc(sizeof(cpu));
    memset(cpu_global, 0, sizeof(cpu));

    // Store cart separately
    cpu_global->cart = malloc(cart_size);
    memcpy(cpu_global->cart, cart, cart_size);
    cpu_global->cart_size = (u32)cart_size;
    cpu_global->rom_bank = 1;

    // PC starts at 0x0100
    cpu_global->pc = 0x0100;

    // SP defaults to 0xFFFE
    cpu_global->sp = 0xFFFE;
    cpu_global->a = 0x1;
    cpu_global->f |= CPU_FLAGS_Z | CPU_FLAGS_H | CPU_FLAGS_C;
    SET_BC(0x0013);
    SET_DE(0x00D8);
    SET_HL(0x014D);
    
    // setup optable
    cpu_global->optable = (instruction*)malloc(sizeof(instruction) * 512);
    memset(cpu_global->optable, 0, sizeof(instruction) * 512);
    
    cpu_global->optable[0x00] = (instruction){ .func = NOP,       .name = "NOP"       };
    cpu_global->optable[0x01] = (instruction){ .func = LD_BC_d16, .name = "LD_BC_d16" };
    cpu_global->optable[0x02] = (instruction){ .func = LD_BC_A,   .name = "LD_BC_A"   };
    cpu_global->optable[0x03] = (instruction){ .func = INC_BC,    .name = "INC_BC"    };
    cpu_global->optable[0x04] = (instruction){ .func = INC_B,     .name = "INC_B"     };
    cpu_global->optable[0x05] = (instruction){ .func = DEC_B,     .name = "DEC_B"     };
    cpu_global->optable[0x06] = (instruction){ .func = LD_B_d8,   .name = "LD_B_d8"   };
    cpu_global->optable[0x07] = (instruction){ .func = RLCA,      .name = "RLCA"      };
    cpu_global->optable[0x08] = (instruction){ .func = LD_a16_SP, .name = "LD_a16_SP" };
    cpu_global->optable[0x09] = (instruction){ .func = ADD_HL_BC, .name = "ADD_HL_BC" };
    cpu_global->optable[0x0A] = (instruction){ .func = LD_A_BC,   .name = "LD_A_BC"   };
    cpu_global->optable[0x0B] = (instruction){ .func = DEC_BC,    .name = "DEC_BC"    };
    cpu_global->optable[0x0C] = (instruction){ .func = INC_C,     .name = "INC_C"     };
    cpu_global->optable[0x0D] = (instruction){ .func = DEC_C,     .name = "DEC_C"     };
    cpu_global->optable[0x0E] = (instruction){ .func = LD_C_d8,   .name = "LD_C_d8"   };
    cpu_global->optable[0x0F] = (instruction){ .func = RRCA,      .name = "RRCA"      };

    cpu_global->optable[0x10] = (instruction){ .func = STOP,      .name = "STOP"      };
    cpu_global->optable[0x11] = (instruction){ .func = LD_DE_d16, .name = "LD_DE_d16" };
    cpu_global->optable[0x12] = (instruction){ .func = LD_DE_A,   .name = "LD_DE_A"   };
    cpu_global->optable[0x13] = (instruction){ .func = INC_DE,    .name = "INC_DE"    };
    cpu_global->optable[0x14] = (instruction){ .func = INC_D,     .name = "INC_D"     };
    cpu_global->optable[0x15] = (instruction){ .func = DEC_D,     .name = "DEC_D"     };
    cpu_global->optable[0x16] = (instruction){ .func = LD_D_d8,   .name = "LD_D_d8"   };
    cpu_global->optable[0x17] = (instruction){ .func = RLA,       .name = "RLA"       };
    cpu_global->optable[0x18] = (instruction){ .func = JR_s8,     .name = "JR_s8"     };
    cpu_global->optable[0x19] = (instruction){ .func = ADD_HL_DE, .name = "ADD_HL_DE" };
    cpu_global->optable[0x1A] = (instruction){ .func = LD_A_DE,   .name = "LD_A_DE"   };
    cpu_global->optable[0x1B] = (instruction){ .func = DEC_DE,    .name = "DEC_DE"    };
    cpu_global->optable[0x1C] = (instruction){ .func = INC_E,     .name = "INC_E"     };
    cpu_global->optable[0x1D] = (instruction){ .func = DEC_E,     .name = "DEC_E"     };
    cpu_global->optable[0x1E] = (instruction){ .func = LD_E_d8,   .name = "LD_E_d8"   };
    cpu_global->optable[0x1F] = (instruction){ .func = RRA,       .name = "RRA"       };

    cpu_global->optable[0x20] = (instruction){ .func = JR_NZ_s8,  .name = "JR_NZ_s8"  };
    cpu_global->optable[0x21] = (instruction){ .func = LD_HL_d16, .name = "LD_HL_d16" };
    cpu_global->optable[0x22] = (instruction){ .func = LD_HLI_A,  .name = "LD_HLI_A"  };
    cpu_global->optable[0x23] = (instruction){ .func = INC_HL,    .name = "INC_HL"    };
    cpu_global->optable[0x24] = (instruction){ .func = INC_H,     .name = "INC_H"     };
    cpu_global->optable[0x25] = (instruction){ .func = DEC_H,     .name = "DEC_H"     };
    cpu_global->optable[0x26] = (instruction){ .func = LD_H_d8,   .name = "LD_H_d8"   };
    cpu_global->optable[0x27] = (instruction){ .func = DAA,       .name = "DAA"       };
    cpu_global->optable[0x28] = (instruction){ .func = JR_Z_s8,   .name = "JR_Z_s8"   };
    cpu_global->optable[0x29] = (instruction){ .func = ADD_HL_HL, .name = "ADD_HL_HL" };
    cpu_global->optable[0x2A] = (instruction){ .func = LD_A_HLI,  .name = "LD_A_HLI"  };
    cpu_global->optable[0x2B] = (instruction){ .func = DEC_HL,    .name = "DEC_HL"    };
    cpu_global->optable[0x2C] = (instruction){ .func = INC_L,     .name = "INC_L"     };
    cpu_global->optable[0x2D] = (instruction){ .func = DEC_L,     .name = "DEC_L"     };
    cpu_global->optable[0x2E] = (instruction){ .func = LD_L_d8,   .name = "LD_L_d8"   };
    cpu_global->optable[0x2F] = (instruction){ .func = CPL,       .name = "CPL"       };

    cpu_global->optable[0x30] = (instruction){ .func = JR_NC_s8,  .name = "JR_NC_s8"  };
    cpu_global->optable[0x31] = (instruction){ .func = LD_SP_d16, .name = "LD_SP_d16" };
    cpu_global->optable[0x32] = (instruction){ .func = LD_HLD_A,  .name = "LD_HLD_A"  };
    cpu_global->optable[0x33] = (instruction){ .func = INC_SP,    .name = "INC_SP"    };
    cpu_global->optable[0x34] = (instruction){ .func = INC_IND_HL,.name = "INC_IND_HL"};
    cpu_global->optable[0x35] = (instruction){ .func = DEC_IND_HL,.name = "DEC_IND_HL"};
    cpu_global->optable[0x36] = (instruction){ .func = LD_HL_d8,  .name = "LD_HL_d8"  };
    cpu_global->optable[0x37] = (instruction){ .func = SCF,       .name = "SCF"       };
    cpu_global->optable[0x38] = (instruction){ .func = JR_C_s8,   .name = "JR_C_s8"   };
    cpu_global->optable[0x39] = (instruction){ .func = ADD_HL_SP, .name = "ADD_HL_SP" };
    cpu_global->optable[0x3A] = (instruction){ .func = LD_A_HLD,  .name = "LD_A_HLD"  };
    cpu_global->optable[0x3B] = (instruction){ .func = DEC_SP,    .name = "DEC_SP"    };
    cpu_global->optable[0x3C] = (instruction){ .func = INC_A,     .name = "INC_A"     };
    cpu_global->optable[0x3D] = (instruction){ .func = DEC_A,     .name = "DEC_A"     };
    cpu_global->optable[0x3E] = (instruction){ .func = LD_A_d8,   .name = "LD_A_d8"   };
    cpu_global->optable[0x3F] = (instruction){ .func = CCF,       .name = "CCF"       };

    cpu_global->optable[0x40] = (instruction){ .func = LD_B_B, .name = "LD_B_B"       };
    cpu_global->optable[0x41] = (instruction){ .func = LD_B_C, .name = "LD_B_C"       };
    cpu_global->optable[0x42] = (instruction){ .func = LD_B_D, .name = "LD_B_D"       };
    cpu_global->optable[0x43] = (instruction){ .func = LD_B_E, .name = "LD_B_E"       };
    cpu_global->optable[0x44] = (instruction){ .func = LD_B_H, .name = "LD_B_H"       };
    cpu_global->optable[0x45] = (instruction){ .func = LD_B_L, .name = "LD_B_L"       };
    cpu_global->optable[0x46] = (instruction){ .func = LD_B_HL,.name = "LD_B_HL"      };
    cpu_global->optable[0x47] = (instruction){ .func = LD_B_A, .name = "LD_B_A"       };
    cpu_global->optable[0x48] = (instruction){ .func = LD_C_B, .name = "LD_C_B"       };
    cpu_global->optable[0x49] = (instruction){ .func = LD_C_C, .name = "LD_C_C"       };
    cpu_global->optable[0x4A] = (instruction){ .func = LD_C_D, .name = "LD_C_D"       };
    cpu_global->optable[0x4B] = (instruction){ .func = LD_C_E, .name = "LD_C_E"       };
    cpu_global->optable[0x4C] = (instruction){ .func = LD_C_H, .name = "LD_C_H"       };
    cpu_global->optable[0x4D] = (instruction){ .func = LD_C_L, .name = "LD_C_L"       };
    cpu_global->optable[0x4E] = (instruction){ .func = LD_C_HL,.name = "LD_C_HL"      };
    cpu_global->optable[0x4F] = (instruction){ .func = LD_C_A, .name = "LD_C_A"       };

    cpu_global->optable[0x50] = (instruction){ .func = LD_D_B, .name = "LD_D_B"       };
    cpu_global->optable[0x51] = (instruction){ .func = LD_D_C, .name = "LD_D_C"       };
    cpu_global->optable[0x52] = (instruction){ .func = LD_D_D, .name = "LD_D_D"       };
    cpu_global->optable[0x53] = (instruction){ .func = LD_D_E, .name = "LD_D_E"       };
    cpu_global->optable[0x54] = (instruction){ .func = LD_D_H, .name = "LD_D_H"       };
    cpu_global->optable[0x55] = (instruction){ .func = LD_D_L, .name = "LD_D_L"       };
    cpu_global->optable[0x56] = (instruction){ .func = LD_D_HL,.name = "LD_D_HL"      };
    cpu_global->optable[0x57] = (instruction){ .func = LD_D_A, .name = "LD_D_A"       };
    cpu_global->optable[0x58] = (instruction){ .func = LD_E_B, .name = "LD_E_B"       };
    cpu_global->optable[0x59] = (instruction){ .func = LD_E_C, .name = "LD_E_C"       };
    cpu_global->optable[0x5A] = (instruction){ .func = LD_E_D, .name = "LD_E_D"       };
    cpu_global->optable[0x5B] = (instruction){ .func = LD_E_E, .name = "LD_E_E"       };
    cpu_global->optable[0x5C] = (instruction){ .func = LD_E_H, .name = "LD_E_H"       };
    cpu_global->optable[0x5D] = (instruction){ .func = LD_E_L, .name = "LD_E_L"       };
    cpu_global->optable[0x5E] = (instruction){ .func = LD_E_HL,.name = "LD_E_HL"      };
    cpu_global->optable[0x5F] = (instruction){ .func = LD_E_A, .name = "LD_E_A"       };

    cpu_global->optable[0x60] = (instruction){ .func = LD_H_B, .name = "LD_H_B"       };
    cpu_global->optable[0x61] = (instruction){ .func = LD_H_C, .name = "LD_H_C"       };
    cpu_global->optable[0x62] = (instruction){ .func = LD_H_D, .name = "LD_H_D"       };
    cpu_global->optable[0x63] = (instruction){ .func = LD_H_E, .name = "LD_H_E"       };
    cpu_global->optable[0x64] = (instruction){ .func = LD_H_H, .name = "LD_H_H"       };
    cpu_global->optable[0x65] = (instruction){ .func = LD_H_L, .name = "LD_H_L"       };
    cpu_global->optable[0x66] = (instruction){ .func = LD_H_HL,.name = "LD_H_HL"      };
    cpu_global->optable[0x67] = (instruction){ .func = LD_H_A, .name = "LD_H_A"       };
    cpu_global->optable[0x68] = (instruction){ .func = LD_L_B, .name = "LD_L_B"       };
    cpu_global->optable[0x69] = (instruction){ .func = LD_L_C, .name = "LD_L_C"       };
    cpu_global->optable[0x6A] = (instruction){ .func = LD_L_D, .name = "LD_L_D"       };
    cpu_global->optable[0x6B] = (instruction){ .func = LD_L_E, .name = "LD_L_E"       };
    cpu_global->optable[0x6C] = (instruction){ .func = LD_L_H, .name = "LD_L_H" };
    cpu_global->optable[0x6D] = (instruction){ .func = LD_L_L, .name = "LD_L_L"       };
    cpu_global->optable[0x6E] = (instruction){ .func = LD_L_HL,.name = "LD_L_HL"      };
    cpu_global->optable[0x6F] = (instruction){ .func = LD_L_A, .name = "LD_L_A"       };

    cpu_global->optable[0x70] = (instruction){ .func = LD_HL_B,.name = "LD_HL_B"      };
    cpu_global->optable[0x71] = (instruction){ .func = LD_HL_C,.name = "LD_HL_C"      };
    cpu_global->optable[0x72] = (instruction){ .func = LD_HL_D,.name = "LD_HL_D"      };
    cpu_global->optable[0x73] = (instruction){ .func = LD_HL_E,.name = "LD_HL_E"      };
    cpu_global->optable[0x74] = (instruction){ .func = LD_HL_H,.name = "LD_HL_H"      };
    cpu_global->optable[0x75] = (instruction){ .func = LD_HL_L,.name = "LD_HL_L"      };
    cpu_global->optable[0x76] = (instruction){ .func = HALT,   .name = "HALT"         };
    cpu_global->optable[0x77] = (instruction){ .func = LD_HL_A,.name = "LD_HL_A"      };
    cpu_global->optable[0x78] = (instruction){ .func = LD_A_B, .name = "LD_A_B"       };
    cpu_global->optable[0x79] = (instruction){ .func = LD_A_C, .name = "LD_A_C"       };
    cpu_global->optable[0x7A] = (instruction){ .func = LD_A_D, .name = "LD_A_D"       };
    cpu_global->optable[0x7B] = (instruction){ .func = LD_A_E, .name = "LD_A_E"       };
    cpu_global->optable[0x7C] = (instruction){ .func = LD_A_H, .name = "LD_A_H"       };
    cpu_global->optable[0x7D] = (instruction){ .func = LD_A_L, .name = "LD_A_L"       };
    cpu_global->optable[0x7E] = (instruction){ .func = LD_A_HL,.name = "LD_A_HL"      };
    cpu_global->optable[0x7F] = (instruction){ .func = LD_A_A, .name = "LD_A_A"       };

    cpu_global->optable[0x80] = (instruction){ .func = ADD_A_B, .name = "ADD_A_B"     };
    cpu_global->optable[0x81] = (instruction){ .func = ADD_A_C, .name = "ADD_A_C"     };
    cpu_global->optable[0x82] = (instruction){ .func = ADD_A_D, .name = "ADD_A_D"     };
    cpu_global->optable[0x83] = (instruction){ .func = ADD_A_E, .name = "ADD_A_E"     };
    cpu_global->optable[0x84] = (instruction){ .func = ADD_A_H, .name = "ADD_A_H"     };
    cpu_global->optable[0x85] = (instruction){ .func = ADD_A_L, .name = "ADD_A_L"     };
    cpu_global->optable[0x86] = (instruction){ .func = ADD_A_HL,.name = "ADD_A_HL"    };
    cpu_global->optable[0x87] = (instruction){ .func = ADD_A_A, .name = "ADD_A_A"     };
    cpu_global->optable[0x88] = (instruction){ .func = ADC_A_B, .name = "ADC_A_B"     };
    cpu_global->optable[0x89] = (instruction){ .func = ADC_A_C, .name = "ADC_A_C"     };
    cpu_global->optable[0x8A] = (instruction){ .func = ADC_A_D, .name = "ADC_A_D"     };
    cpu_global->optable[0x8B] = (instruction){ .func = ADC_A_E, .name = "ADC_A_E"     };
    cpu_global->optable[0x8C] = (instruction){ .func = ADC_A_H, .name = "ADC_A_H"     };
    cpu_global->optable[0x8D] = (instruction){ .func = ADC_A_L, .name = "ADC_A_L"     };
    cpu_global->optable[0x8E] = (instruction){ .func = ADC_A_HL,.name = "ADC_A_HL"    };
    cpu_global->optable[0x8F] = (instruction){ .func = ADC_A_A, .name = "ADC_A_A"     };

    cpu_global->optable[0x90] = (instruction){ .func = SUB_A_B,  .name = "SUB_A_B"    };
    cpu_global->optable[0x91] = (instruction){ .func = SUB_A_C,  .name = "SUB_A_C"    };
    cpu_global->optable[0x92] = (instruction){ .func = SUB_A_D,  .name = "SUB_A_D"    };
    cpu_global->optable[0x93] = (instruction){ .func = SUB_A_E,  .name = "SUB_A_E"    };
    cpu_global->optable[0x94] = (instruction){ .func = SUB_A_H,  .name = "SUB_A_H"    };
    cpu_global->optable[0x95] = (instruction){ .func = SUB_A_L,  .name = "SUB_A_L"    };
    cpu_global->optable[0x96] = (instruction){ .func = SUB_A_HL, .name = "SUB_A_HL"   };
    cpu_global->optable[0x97] = (instruction){ .func = SUB_A_A,  .name = "SUB_A_A"    };
    cpu_global->optable[0x98] = (instruction){ .func = SBC_A_B,  .name = "SBC_A_B"    };
    cpu_global->optable[0x99] = (instruction){ .func = SBC_A_C,  .name = "SBC_A_C"    };
    cpu_global->optable[0x9A] = (instruction){ .func = SBC_A_D,  .name = "SBC_A_D"    };
    cpu_global->optable[0x9B] = (instruction){ .func = SBC_A_E,  .name = "SBC_A_E"    };
    cpu_global->optable[0x9C] = (instruction){ .func = SBC_A_H,  .name = "SBC_A_H"    };
    cpu_global->optable[0x9D] = (instruction){ .func = SBC_A_L,  .name = "SBC_A_L"    };
    cpu_global->optable[0x9E] = (instruction){ .func = SBC_A_HL, .name = "SBC_A_HL"   };
    cpu_global->optable[0x9F] = (instruction){ .func = SBC_A_A,  .name = "SBC_A_A"    };

    cpu_global->optable[0xA0] = (instruction){ .func = AND_A_B,   .name = "AND_A_B"  };
    cpu_global->optable[0xA1] = (instruction){ .func = AND_A_C,   .name = "AND_A_C"  };
    cpu_global->optable[0xA2] = (instruction){ .func = AND_A_D,   .name = "AND_A_D"  };
    cpu_global->optable[0xA3] = (instruction){ .func = AND_A_E,   .name = "AND_A_E"  };
    cpu_global->optable[0xA4] = (instruction){ .func = AND_A_H,   .name = "AND_A_H"  };
    cpu_global->optable[0xA5] = (instruction){ .func = AND_A_L,   .name = "AND_A_L"  };
    cpu_global->optable[0xA6] = (instruction){ .func = AND_A_HL,  .name = "AND_A_HL" };
    cpu_global->optable[0xA7] = (instruction){ .func = AND_A_A,   .name = "AND_A_A"  };
    cpu_global->optable[0xA8] = (instruction){ .func = XOR_A_B,   .name = "XOR_A_B"  };
    cpu_global->optable[0xA9] = (instruction){ .func = XOR_A_C,   .name = "XOR_A_C"  };
    cpu_global->optable[0xAA] = (instruction){ .func = XOR_A_D,   .name = "XOR_A_D"  };
    cpu_global->optable[0xAB] = (instruction){ .func = XOR_A_E,   .name = "XOR_A_E"  };
    cpu_global->optable[0xAC] = (instruction){ .func = XOR_A_H,   .name = "XOR_A_H"  };
    cpu_global->optable[0xAD] = (instruction){ .func = XOR_A_L,   .name = "XOR_A_L"  };
    cpu_global->optable[0xAE] = (instruction){ .func = XOR_A_HL,  .name = "XOR_A_HL" };
    cpu_global->optable[0xAF] = (instruction){ .func = XOR_A_A,   .name = "XOR_A_A"  };

    cpu_global->optable[0xB0] = (instruction){ .func = OR_A_B,    .name = "OR_A_B"   };
    cpu_global->optable[0xB1] = (instruction){ .func = OR_A_C,    .name = "OR_A_C"   };
    cpu_global->optable[0xB2] = (instruction){ .func = OR_A_D,    .name = "OR_A_D"   };
    cpu_global->optable[0xB3] = (instruction){ .func = OR_A_E,    .name = "OR_A_E"   };
    cpu_global->optable[0xB4] = (instruction){ .func = OR_A_H,    .name = "OR_A_H"   };
    cpu_global->optable[0xB5] = (instruction){ .func = OR_A_L,    .name = "OR_A_L"   };
    cpu_global->optable[0xB6] = (instruction){ .func = OR_A_HL,   .name = "OR_A_HL"  };
    cpu_global->optable[0xB7] = (instruction){ .func = OR_A_A,    .name = "OR_A_A"   };
    cpu_global->optable[0xB8] = (instruction){ .func = CP_A_B,    .name = "CP_A_B"   };
    cpu_global->optable[0xB9] = (instruction){ .func = CP_A_C,    .name = "CP_A_C"   };
    cpu_global->optable[0xBA] = (instruction){ .func = CP_A_D,    .name = "CP_A_D"   };
    cpu_global->optable[0xBB] = (instruction){ .func = CP_A_E,    .name = "CP_A_E"   };
    cpu_global->optable[0xBC] = (instruction){ .func = CP_A_H,    .name = "CP_A_H"   };
    cpu_global->optable[0xBD] = (instruction){ .func = CP_A_L,    .name = "CP_A_L"   };
    cpu_global->optable[0xBE] = (instruction){ .func = CP_A_HL,   .name = "CP_A_H"   };
    cpu_global->optable[0xBF] = (instruction){ .func = CP_A_A,    .name = "CP_A_A"   };

    cpu_global->optable[0xC0] = (instruction){ .func = RET_NZ,     .name = "RET_NZ"       };
    cpu_global->optable[0xC1] = (instruction){ .func = POP_BC,     .name = "POP_BC"       };
    cpu_global->optable[0xC2] = (instruction){ .func = JP_NZ_a16,  .name = "JP_NZ_a16"    };
    cpu_global->optable[0xC3] = (instruction){ .func = JP_a16,     .name = "JP_a16"       };
    cpu_global->optable[0xC4] = (instruction){ .func = CALL_NZ_a16,.name = "CALL_NZ_a16"  };
    cpu_global->optable[0xC5] = (instruction){ .func = PUSH_BC,    .name = "PUSH_BC"      };
    cpu_global->optable[0xC6] = (instruction){ .func = ADD_A_d8,   .name = "ADD_A_d8"     };
    cpu_global->optable[0xC7] = (instruction){ .func = RST_00,     .name = "RST_00"       };
    cpu_global->optable[0xC8] = (instruction){ .func = RET_Z,      .name = "RET_Z"        };
    cpu_global->optable[0xC9] = (instruction){ .func = RET,        .name = "RET"          };
    cpu_global->optable[0xCA] = (instruction){ .func = JP_Z_a16,   .name = "JP_Z_a16"     };
    cpu_global->optable[0xCB] = (instruction){ .func = PREFIX,     .name = "PREFIX"       };
    cpu_global->optable[0xCC] = (instruction){ .func = CALL_Z_a16, .name = "CALL_Z_a16"   };
    cpu_global->optable[0xCD] = (instruction){ .func = CALL_a16,   .name = "CALL_a16"     };
    cpu_global->optable[0xCE] = (instruction){ .func = ADC_A_d8,   .name = "ADC_A_d8"     };
    cpu_global->optable[0xCF] = (instruction){ .func = RST_08,     .name = "RST_08"       };

    cpu_global->optable[0xD0] = (instruction){ .func = RET_NC,     .name = "RET_NC"       };
    cpu_global->optable[0xD1] = (instruction){ .func = POP_DE,     .name = "POP_DE"       };
    cpu_global->optable[0xD2] = (instruction){ .func = JP_NC_a16,  .name = "JP_NC_a16"    };
    cpu_global->optable[0xD3] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    cpu_global->optable[0xD4] = (instruction){ .func = CALL_NC_a16,.name = "CALL_NC_a16"  };
    cpu_global->optable[0xD5] = (instruction){ .func = PUSH_DE,    .name = "PUSH_DE"      };
    cpu_global->optable[0xD6] = (instruction){ .func = SUB_A_d8,   .name = "SUB_A_d8"     };
    cpu_global->optable[0xD7] = (instruction){ .func = RST_10,     .name = "RST_10"       };
    cpu_global->optable[0xD8] = (instruction){ .func = RET_C,      .name = "RET_C"        };
    cpu_global->optable[0xD9] = (instruction){ .func = RETI,       .name = "RETI"         };
    cpu_global->optable[0xDA] = (instruction){ .func = JP_C_a16,   .name = "JP_C_a16"     };
    cpu_global->optable[0xDB] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    cpu_global->optable[0xDC] = (instruction){ .func = CALL_C_a16, .name = "CALL_C_a16"   };
    cpu_global->optable[0xDD] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    cpu_global->optable[0xDE] = (instruction){ .func = SBC_A_d8,   .name = "SBC_A_d8"     };
    cpu_global->optable[0xDF] = (instruction){ .func = RST_18,     .name = "RST_18"       };

    cpu_global->optable[0xE0] = (instruction){ .func = LDH_a8_A,   .name = "LDH_a8_A"     };
    cpu_global->optable[0xE1] = (instruction){ .func = POP_HL,     .name = "POP_HL"       };
    cpu_global->optable[0xE2] = (instruction){ .func = LDH_C_A,    .name = "LD_C_A"       };
    cpu_global->optable[0xE3] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    cpu_global->optable[0xE4] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    cpu_global->optable[0xE5] = (instruction){ .func = PUSH_HL,    .name = "PUSH_HL"      };
    cpu_global->optable[0xE6] = (instruction){ .func = AND_A_d8,   .name = "AND_A_d8"     };
    cpu_global->optable[0xE7] = (instruction){ .func = RST_20,     .name = "RST_20"       };
    cpu_global->optable[0xE8] = (instruction){ .func = ADD_SP_i8,  .name = "ADD_SP_i8"    };
    cpu_global->optable[0xE9] = (instruction){ .func = JP_HL,      .name = "JP_HL"        };
    cpu_global->optable[0xEA] = (instruction){ .func = LD_a16_A,   .name = "LD_a16_A"     };
    cpu_global->optable[0xEB] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    cpu_global->optable[0xEC] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    cpu_global->optable[0xED] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    cpu_global->optable[0xEE] = (instruction){ .func = XOR_A_d8,   .name = "XOR_A_d8"     };
    cpu_global->optable[0xEF] = (instruction){ .func = RST_28,     .name = "RST_28"       };

    cpu_global->optable[0xF0] = (instruction){ .func = LDH_A_a8,          .name = "LDH_A_a8"          };
    cpu_global->optable[0xF1] = (instruction){ .func = POP_AF,            .name = "POP_AF"            };
    cpu_global->optable[0xF2] = (instruction){ .func = LDH_A_C,           .name = "LD_A_C"            };
    cpu_global->optable[0xF3] = (instruction){ .func = DI,                .name = "DI"                };
    cpu_global->optable[0xF4] = (instruction){ .func = INVALID,           .name = "INVALID"           };
    cpu_global->optable[0xF5] = (instruction){ .func = PUSH_AF,           .name = "PUSH_AF"           };
    cpu_global->optable[0xF6] = (instruction){ .func = OR_A_d8,           .name = "OR_A_d8"           };
    cpu_global->optable[0xF7] = (instruction){ .func = RST_30,            .name = "RST_30"            };
    cpu_global->optable[0xF8] = (instruction){ .func = LD_HL_SP_PLUS_i8,  .name = "LD_HL_SP_PLUS_i8"  };
    cpu_global->optable[0xF9] = (instruction){ .func = LD_SP_HL,          .name = "LD_SP_HL"          };
    cpu_global->optable[0xFA] = (instruction){ .func = LD_A_a16,          .name = "LD_A_a16"          };
    cpu_global->optable[0xFB] = (instruction){ .func = EI,                .name = "EI"                };
    cpu_global->optable[0xFC] = (instruction){ .func = INVALID,           .name = "INVALID"           };
    cpu_global->optable[0xFD] = (instruction){ .func = INVALID,           .name = "INVALID"           };
    cpu_global->optable[0xFE] = (instruction){ .func = CP_A_d8,           .name = "CP_A_d8"           };
    cpu_global->optable[0xFF] = (instruction){ .func = RST_38,            .name = "RST_38"            };
}

void cpu_unload() {
    if (cpu_global) {
        if (cpu_global->optable) {
            free(cpu_global->optable);
        }
        free(cpu_global);
    }
}

static void check_interrupts(u16 addr, u16 int_type) {
    if (cpu_global->IF & int_type && cpu_global->IE & int_type) {
        // Push PC onto stack
        cpu_global->sp -= 2;
        bus_write(cpu_global->sp, cpu_global->pc & 0xFF);
        bus_write(cpu_global->sp + 1, (cpu_global->pc >> 8) & 0xFF);

        // Jump to interrupt vector
        cpu_global->pc = addr;

        cpu_global->IF &= ~int_type;
        cpu_global->halted = false;
        cpu_global->ints_enabled = false;
    }
}

void cpu_handle_interrupts() {
    check_interrupts(0x40, INT_VBLANK); // VBLANK
    check_interrupts(0x48, INT_LCD_STAT); // LCD_STAT
    check_interrupts(0x50, INT_TIMER); // TIMER
    check_interrupts(0x58, INT_SERIAL); // SERIAL
    check_interrupts(0x60, INT_JOYPAD); // JOYPAD

    // Interrupt handling takes 20 cycles
    cpu_global->cycles += 20;
}

void cpu_print_dbg_info(const instruction* cur_instr) {
    char flags[16];
    sprintf(flags, "%c%c%c%c",
        cpu_global->f & CPU_FLAGS_Z ? 'Z' : '-',
        cpu_global->f & CPU_FLAGS_N ? 'N' : '-',
        cpu_global->f & CPU_FLAGS_H ? 'H' : '-',
        cpu_global->f & CPU_FLAGS_C ? 'C' : '-'
    );

    printf("%04X: %-12s (%02X %02X %02X) A: %02X F: %s BC: %02X%02X DE: %02X%02X HL: %02X%02X\n",
        cpu_global->pc, cur_instr->name, bus_read(cpu_global->pc),
        bus_read(cpu_global->pc + 1), bus_read(cpu_global->pc + 2), cpu_global->a, flags, cpu_global->b, cpu_global->c,
        cpu_global->d, cpu_global->e, cpu_global->h, cpu_global->l);
}