#include "cpu.h"
#include "bus.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// -- HELPERS --

static __forceinline u16 fetch_d16(cpu* c) {
    u8 lo = bus_read(c->_bus, c->pc++);
    u8 hi = bus_read(c->_bus, c->pc++);
    return (hi << 8) | lo;
}

static __forceinline u8 fetch(cpu* c) {
    return bus_read(c->_bus, c->pc++);
}

static __forceinline void cpu_set_flag(cpu* c, cpu_flags f) { c->f |= f; }
static __forceinline void cpu_clear_flag(cpu* c, cpu_flags f) { c->f &= ~f; }
static __forceinline int cpu_get_flag(cpu* c, cpu_flags f) { return c->f & f; }

#define proc static __forceinline u8

// -- HELPERS --

// -- ALU OPERATIONS -- //

static u8 alu_add_sub(cpu* c, u8 a, u8 value, bool add, u8 carry) {
    u16 result;
    c->f = 0;

    if (add) {
        result = (u16)a + (u16)value + carry;

        if ((result & 0xFF) == 0) c->f |= CPU_FLAGS_Z;
        if (((a & 0xF) + (value & 0xF) + carry) > 0xF) c->f |= CPU_FLAGS_H;
        if (result > 0xFF) c->f |= CPU_FLAGS_C;
    }
    else {
        result = (u16)a - (u16)value - carry;

        if ((result & 0xFF) == 0) c->f |= CPU_FLAGS_Z;
        c->f |= CPU_FLAGS_N;
        if ((a & 0xF) < ((value & 0xF) + carry)) c->f |= CPU_FLAGS_H;
        if ((u16)a < (u16)value + carry) c->f |= CPU_FLAGS_C;
    }

    return (u8)result;
}

// -- ALU OPERATIONS -- //

// -- INSTRUCTIONS --

// 0x00-0x0F

proc NOP(cpu* c) {
    // Do nothing
    return 4;
}

proc LD_BC_d16(cpu* c) {
    SET_BC(c, fetch_d16(c));
    return 12;
}

proc LD_BC_A(cpu* c) {
    bus_write(c->_bus, c->a, BC(c));
    return 8;
}

proc INC_BC(cpu* c) {
    SET_BC(c, BC(c) + 1);
    return 8;
}

proc INC_B(cpu* c) {
    u8 result = c->b + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((c->b & 0x0F) + 1 > 0x0F) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    c->b = result;

    return 4;
}

proc DEC_B(cpu* c) {
    u8 result = c->b - 1;

    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_set_flag(c, CPU_FLAGS_N);

    // Half-carry borrow from bit 4
    if ((c->b & 0x0F) == 0x00) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    c->b = result;
    return 4;
}

proc LD_B_d8(cpu* c) {
    c->b = fetch(c);
    return 8;
}

proc RLCA(cpu* c) {
    u8 old = c->a;
    c->a = (old << 1) | (old >> 7); // rotate
    c->f = 0; // clear Z, N, H
    if (old & 0x80) cpu_set_flag(c, CPU_FLAGS_C);
    return 4;
}

proc LD_a16_SP(cpu* c) {
    u16 a16 = fetch_d16(c);
    bus_write(c->_bus, LOBYTE(c->sp), a16);
    bus_write(c->_bus, HIBYTE(c->sp), a16 + 1);

    return 20;
}

proc ADD_HL_BC(cpu* c) {
    u32 result = HL(c) + BC(c);
    cpu_clear_flag(c, CPU_FLAGS_N);

    if (((HL(c) & 0x0FFF) + (BC(c) & 0x0FFF)) > 0x0FFF)
        cpu_set_flag(c, CPU_FLAGS_H);
    else
        cpu_clear_flag(c, CPU_FLAGS_H);

    if (result > 0xFFFF)
        cpu_set_flag(c, CPU_FLAGS_C);
    else
        cpu_clear_flag(c, CPU_FLAGS_C);

    SET_HL(c, (u16)result);
    return 8;
}

proc LD_A_BC(cpu* c) {
    c->a = bus_read(c->_bus, BC(c));
    return 8;
}

proc DEC_BC(cpu* c) {
    SET_BC(c, BC(c) - 1);
    return 8;
}

proc INC_C(cpu* c) {
    u8 result = c->c + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((c->c & 0x0F) + 1 > 0x0F) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    c->c = result;

    return 4;
}

proc DEC_C(cpu* c) {
    u8 result = c->c - 1;

    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_set_flag(c, CPU_FLAGS_N);

    if ((c->c & 0x0F) == 0x00) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    c->c = result;
    return 4;
}

proc LD_C_d8(cpu* c) {
    c->c = fetch(c);
    return 8;
}

proc RRCA(cpu* c) {
    u8 old = c->a;
    c->a = (old >> 1) | (old << 7); // rotate right
    c->f = 0;
    if (old & 0x01) cpu_set_flag(c, CPU_FLAGS_C);
    return 4;
}

// 0x00-0x0F

// 0x10-0x1F

proc STOP(cpu* c) {
    // Not sure how to implement yet...
    // TODO
    ++c->pc;
    return 4;
}

proc LD_DE_d16(cpu* c) {
    SET_DE(c, fetch_d16(c));
    return 12;
}

proc LD_DE_A(cpu* c) {
    bus_write(c->_bus, c->a, DE(c));
    return 8;
}

proc INC_DE(cpu* c) {
    SET_DE(c, DE(c) + 1);
    return 8;
}

proc INC_D(cpu* c) {
    u8 result = c->d + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((c->d & 0x0F) + 1 > 0x0F) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    c->d = result;

    return 4;
}

proc DEC_D(cpu* c) {
    u8 result = c->d - 1;

    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_set_flag(c, CPU_FLAGS_N);

    // Half-carry borrow from bit 4
    if ((c->d & 0x0F) == 0x00) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    c->d = result;
    return 4;
}

proc LD_D_d8(cpu* c) {
    c->d = fetch(c);
    return 8;
}

proc RLA(cpu* c) {
    u8 carry = cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0;
    u8 old = c->a;

    c->a = (old << 1) | carry;

    c->f = 0; // clear Z, N, H
    if (old & 0x80) cpu_set_flag(c, CPU_FLAGS_C);

    return 4;
}

proc JR_s8(cpu* c) {
    i8 jmp = (i8)fetch(c);
    c->pc += jmp;
    return 12;
}

proc ADD_HL_DE(cpu* c) {
    u32 result = HL(c) + DE(c);
    cpu_clear_flag(c, CPU_FLAGS_N);

    if (((HL(c) & 0x0FFF) + (DE(c) & 0x0FFF)) > 0x0FFF)
        cpu_set_flag(c, CPU_FLAGS_H);
    else
        cpu_clear_flag(c, CPU_FLAGS_H);

    if (result > 0xFFFF)
        cpu_set_flag(c, CPU_FLAGS_C);
    else
        cpu_clear_flag(c, CPU_FLAGS_C);

    SET_HL(c, (u16)result);
    return 8;
}

proc LD_A_DE(cpu* c) {
    c->a = bus_read(c->_bus, DE(c));
    return 8;
}

proc DEC_DE(cpu* c) {
    SET_DE(c, DE(c) - 1);
    return 8;
}

proc INC_E(cpu* c) {
    u8 result = c->e + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((c->e & 0x0F) + 1 > 0x0F) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    c->e = result;

    return 4;
}

proc DEC_E(cpu* c) {
    u8 result = c->e - 1;

    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_set_flag(c, CPU_FLAGS_N);

    if ((c->e & 0x0F) == 0x00) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    c->e = result;
    return 4;
}

proc LD_E_d8(cpu* c) {
    c->e = fetch(c);
    return 8;
}

proc RRA(cpu* c) {
    u8 carry = cpu_get_flag(c, CPU_FLAGS_C) ? 0x80 : 0;
    u8 old = c->a;

    c->a = (old >> 1) | carry;

    c->f = 0;
    if (old & 0x01) cpu_set_flag(c, CPU_FLAGS_C);

    return 4;
}

// 0x10-0x1F

// 0x20-0x2F

proc JR_NZ_s8(cpu* c) {
    i8 offset = (i8)fetch(c);  // signed 8-bit
    if (!cpu_get_flag(c, CPU_FLAGS_Z)) {
        c->pc += offset;
        return 12; // jump taken
    }
    return 8; // jump not taken
}

proc LD_HL_d16(cpu* c) {
    SET_HL(c, fetch_d16(c));
    return 12;
}

proc LD_HLI_A(cpu* c) {
    bus_write(c->_bus, c->a, HL(c));
    SET_HL(c, HL(c) + 1);
    return 8;
}

proc INC_HL(cpu* c) {
    SET_HL(c, HL(c) + 1);
    return 8;
}

proc INC_H(cpu* c) {
    u8 result = c->h + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((c->h & 0x0F) + 1 > 0x0F) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    c->h = result;

    return 4;
}

proc DEC_H(cpu* c) {
    u8 result = c->h - 1;

    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_set_flag(c, CPU_FLAGS_N);

    // Half-carry borrow from bit 4
    if ((c->h & 0x0F) == 0x00) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    c->h = result;
    return 4;
}

proc LD_H_d8(cpu* c) {
    c->h = fetch(c);
    return 8;
}

proc DAA(cpu* c) {
    u8 a = c->a;
    u8 adjust = 0;
    bool carry = cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0;

    if (!cpu_get_flag(c, CPU_FLAGS_N)) {
        // after addition
        if (cpu_get_flag(c, CPU_FLAGS_H) || (a & 0x0F) > 9)
            adjust |= 0x06;
        if (carry || a > 0x99) {
            adjust |= 0x60;
            carry = true;
        }
        a += adjust;
    }
    else {
        // after subtraction
        if (cpu_get_flag(c, CPU_FLAGS_H))
            adjust |= 0x06;
        if (cpu_get_flag(c, CPU_FLAGS_C))
            adjust |= 0x60;
        a -= adjust;
    }

    c->a = a;

    // Zero flag
    if (c->a == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    // N flag unchanged
    // H always cleared
    cpu_clear_flag(c, CPU_FLAGS_H);

    // C updated only on addition, preserved on subtraction
    if (carry) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);
    return 8;
}

proc JR_Z_s8(cpu* c) {
    i8 offset = (i8)fetch(c);  // signed 8-bit
    if (cpu_get_flag(c, CPU_FLAGS_Z)) {
        c->pc += offset;
        return 12; // jump taken
    }
    return 8; // jump not taken
}

proc ADD_HL_HL(cpu* c) {
    u32 result = HL(c) + HL(c);
    cpu_clear_flag(c, CPU_FLAGS_N);

    if (((HL(c) & 0x0FFF) + (HL(c) & 0x0FFF)) > 0x0FFF)
        cpu_set_flag(c, CPU_FLAGS_H);
    else
        cpu_clear_flag(c, CPU_FLAGS_H);

    if (result > 0xFFFF)
        cpu_set_flag(c, CPU_FLAGS_C);
    else
        cpu_clear_flag(c, CPU_FLAGS_C);

    SET_HL(c, (u16)result);
    return 8;
}

proc LD_A_HLI(cpu* c) {
    c->a = bus_read(c->_bus, HL(c));   // read from memory into A
    SET_HL(c, HL(c) + 1);              // increment HL
    return 8;
}

proc DEC_HL(cpu* c) {
    u8 val = bus_read(c->_bus, HL(c)) - 1;
    bus_write(c->_bus, HL(c), val);
    return 8;
}

proc INC_L(cpu* c) {
    u8 result = c->l + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((c->l & 0x0F) + 1 > 0x0F) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    c->l = result;

    return 4;
}

proc DEC_L(cpu* c) {
    u8 result = c->l - 1;

    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_set_flag(c, CPU_FLAGS_N);

    if ((c->l & 0x0F) == 0x00) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    c->l = result;
    return 4;
}

proc LD_L_d8(cpu* c) {
    c->l = fetch(c);
    return 8;
}

proc CPL(cpu* c) {
    c->a = ~(c->a);
    cpu_set_flag(c, CPU_FLAGS_N);
    cpu_set_flag(c, CPU_FLAGS_H);
    return 4;
}

// 0x20-0x2F

// 0x30-0x3F

proc JR_NC_s8(cpu* c) {
    i8 offset = (i8)fetch(c);  // signed 8-bit
    if (!cpu_get_flag(c, CPU_FLAGS_C)) {
        c->pc += offset;
        return 12; // jump taken
    }
    return 8; // jump not taken
}

proc LD_SP_d16(cpu* c) {
    c->sp = fetch_d16(c);
    return 12;
}

proc LD_HLD_A(cpu* c) {
    bus_write(c->_bus, c->a, HL(c));
    SET_HL(c, HL(c) - 1);
    return 8;
}

proc INC_SP(cpu* c) {
    c->sp = c->sp + 1;
    return 8;
}

proc INC_IND_HL(cpu* c) {
    u8 value = bus_read(c->_bus, HL(c));
    u8 result = value + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    // N is cleared for INC
    cpu_clear_flag(c, CPU_FLAGS_N);

    // Half-carry: set when low nibble goes from 0xF -> 0x0
    if ((value & 0x0F) == 0x0F) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    // C is NOT affected by INC
    bus_write(c->_bus, result, HL(c));
    return 12;
}

proc DEC_IND_HL(cpu* c) {
    u8 value = bus_read(c->_bus, HL(c));
    u8 result = value - 1;

    // Zero flag
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    // N flag always set for DEC
    cpu_set_flag(c, CPU_FLAGS_N);

    // Half-carry: if lower nibble goes from 0 -> F
    if ((value & 0x0F) == 0x00) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    bus_write(c->_bus, result, HL(c));

    return 12;
}

proc LD_HL_d8(cpu* c) {
    bus_write(c->_bus, fetch(c), HL(c));
    return 12;
}

proc SCF(cpu* c) {
    cpu_clear_flag(c, CPU_FLAGS_N);
    cpu_clear_flag(c, CPU_FLAGS_H);
    cpu_set_flag(c, CPU_FLAGS_C);
    return 4;
}

proc JR_C_s8(cpu* c) {
    i8 offset = (i8)fetch(c);  // signed 8-bit
    if (cpu_get_flag(c, CPU_FLAGS_C)) {
        c->pc += offset;
        return 12; // jump taken
    }
    return 8; // jump not taken
}

proc ADD_HL_SP(cpu* c) {
    u32 result = HL(c) + c->sp;
    cpu_clear_flag(c, CPU_FLAGS_N);

    if (((HL(c) & 0x0FFF) + (c->sp & 0x0FFF)) > 0x0FFF)
        cpu_set_flag(c, CPU_FLAGS_H);
    else
        cpu_clear_flag(c, CPU_FLAGS_H);

    if (result > 0xFFFF)
        cpu_set_flag(c, CPU_FLAGS_C);
    else
        cpu_clear_flag(c, CPU_FLAGS_C);

    SET_HL(c, (u16)result);
    return 8;
}

proc LD_A_HLD(cpu* c) {
    c->a = bus_read(c->_bus, HL(c));   // read from memory into A
    SET_HL(c, HL(c) - 1);              // decrement HL
    return 8;
}

proc DEC_SP(cpu* c) {
    c->sp = c->sp - 1;
    return 8;
}

proc INC_A(cpu* c) {
    u8 result = c->a + 1;

    // Zero flag
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((c->a & 0x0F) + 1 > 0x0F) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    c->a = result;

    return 4;
}

proc DEC_A(cpu* c) {
    u8 result = c->a - 1;

    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_set_flag(c, CPU_FLAGS_N);

    if ((c->a & 0x0F) == 0x00) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    c->a = result;
    return 4;
}

proc LD_A_d8(cpu* c) {
    c->a = fetch(c);
    return 8;
}

proc CCF(cpu* c) {
    cpu_clear_flag(c, CPU_FLAGS_N);
    cpu_clear_flag(c, CPU_FLAGS_H);
    if (cpu_get_flag(c, CPU_FLAGS_C)) cpu_clear_flag(c, CPU_FLAGS_C);
    else cpu_set_flag(c, CPU_FLAGS_C);
    return 4;
}

// 0x30-0x3F

// 0x40-0x4F

proc LD_B_B(cpu* c) {
    c->b = c->b;
    return 4;
}

proc LD_B_C(cpu* c) {
    c->b = c->c;
    return 4;
}

proc LD_B_D(cpu* c) {
    c->b = c->d;
    return 4;
}

proc LD_B_E(cpu* c) {
    c->b = c->e;
    return 4;
}

proc LD_B_H(cpu* c) {
    c->b = c->h;
    return 4;
}

proc LD_B_L(cpu* c) {
    c->b = c->l;
    return 4;
}

proc LD_B_HL(cpu* c) {
    c->b = bus_read(c->_bus, HL(c));
    return 8;
}

proc LD_B_A(cpu* c) {
    c->b = c->a;
    return 4;
}

proc LD_C_B(cpu* c) {
    c->c = c->b;
    return 4;
}

proc LD_C_C(cpu* c) {
    c->c = c->c;
    return 4;
}

proc LD_C_D(cpu* c) {
    c->c = c->d;
    return 4;
}

proc LD_C_E(cpu* c) {
    c->c = c->e;
    return 4;
}

proc LD_C_H(cpu* c) {
    c->c = c->h;
    return 4;
}

proc LD_C_L(cpu* c) {
    c->c = c->l;
    return 4;
}

proc LD_C_HL(cpu* c) {
    c->c = bus_read(c->_bus, HL(c));
    return 8;
}

proc LD_C_A(cpu* c) {
    c->c = c->a;
    return 4;
}

// 0x40-0x4F

// 0x50-0x5F

proc LD_D_B(cpu* c) {
    c->d = c->b;
    return 4;
}

proc LD_D_C(cpu* c) {
    c->d = c->c;
    return 4;
}

proc LD_D_D(cpu* c) {
    c->d = c->d;
    return 4;
}

proc LD_D_E(cpu* c) {
    c->d = c->e;
    return 4;
}

proc LD_D_H(cpu* c) {
    c->d = c->h;
    return 4;
}

proc LD_D_L(cpu* c) {
    c->d = c->l;
    return 4;
}

proc LD_D_HL(cpu* c) {
    c->d = bus_read(c->_bus, HL(c));
    return 8;
}

proc LD_D_A(cpu* c) {
    c->d = c->a;
    return 4;
}

proc LD_E_B(cpu* c) {
    c->e = c->b;
    return 4;
}

proc LD_E_C(cpu* c) {
    c->e = c->c;
    return 4;
}

proc LD_E_D(cpu* c) {
    c->e = c->d;
    return 4;
}

proc LD_E_E(cpu* c) {
    c->e = c->e;
    return 4;
}

proc LD_E_H(cpu* c) {
    c->e = c->h;
    return 4;
}

proc LD_E_L(cpu* c) {
    c->e = c->l;
    return 4;
}

proc LD_E_HL(cpu* c) {
    c->e = bus_read(c->_bus, HL(c));
    return 8;
}

proc LD_E_A(cpu* c) {
    c->e = c->a;
    return 4;
}

// 0x50-0x5F

// 0x60-0x6F

proc LD_H_B(cpu* c) {
    c->h = c->b;
    return 4;
}

proc LD_H_C(cpu* c) {
    c->h = c->c;
    return 4;
}

proc LD_H_D(cpu* c) {
    c->h = c->d;
    return 4;
}

proc LD_H_E(cpu* c) {
    c->h = c->e;
    return 4;
}

proc LD_H_H(cpu* c) {
    c->h = c->h;
    return 4;
}

proc LD_H_L(cpu* c) {
    c->h = c->l;
    return 4;
}

proc LD_H_HL(cpu* c) {
    c->h = bus_read(c->_bus, HL(c));
    return 8;
}

proc LD_H_A(cpu* c) {
    c->h = c->a;
    return 4;
}

proc LD_L_B(cpu* c) {
    c->l = c->b;
    return 4;
}

proc LD_L_C(cpu* c) {
    c->l = c->c;
    return 4;
}

proc LD_L_D(cpu* c) {
    c->l = c->d;
    return 4;
}

proc LD_L_E(cpu* c) {
    c->l = c->e;
    return 4;
}

proc LD_L_H(cpu* c) {
    c->l = c->h;
    return 4;
}

proc LD_L_L(cpu* c) {
    c->l = c->l;
    return 4;
}

proc LD_L_HL(cpu* c) {
    c->l = bus_read(c->_bus, HL(c));
    return 8;
}

proc LD_L_A(cpu* c) {
    c->l = c->a;
    return 4;
}

// 0x60-0x6F

// 0x70-0x7F

proc LD_HL_B(cpu* c) {
    bus_write(c->_bus, c->b, HL(c));
    return 8;
}

proc LD_HL_C(cpu* c) {
    bus_write(c->_bus, c->c, HL(c));
    return 8;
}

proc LD_HL_D(cpu* c) {
    bus_write(c->_bus, c->d, HL(c));
    return 8;
}

proc LD_HL_E(cpu* c) {
    bus_write(c->_bus, c->e, HL(c));
    return 8;
}

proc LD_HL_H(cpu* c) {
    bus_write(c->_bus, c->h, HL(c));
    return 8;
}

proc LD_HL_L(cpu* c) {
    bus_write(c->_bus, c->l, HL(c));
    return 8;
}

proc HALT(cpu* c) {
    // TODO
    return 4;
}

proc LD_HL_A(cpu* c) {
    bus_write(c->_bus, c->a, HL(c));
    return 8;
}

proc LD_A_B(cpu* c) {
    c->a = c->b;
    return 4;
}

proc LD_A_C(cpu* c) {
    c->a = c->c;
    return 4;
}

proc LD_A_D(cpu* c) {
    c->a = c->d;
    return 4;
}

proc LD_A_E(cpu* c) {
    c->a = c->e;
    return 4;
}

proc LD_A_H(cpu* c) {
    c->a = c->h;
    return 4;
}

proc LD_A_L(cpu* c) {
    c->a = c->l;
    return 4;
}

proc LD_A_HL(cpu* c) {
    c->a = bus_read(c->_bus, HL(c));
    return 8;
}

proc LD_A_A(cpu* c) {
    return 4;
}

// 0x80-0x8F

proc ADD_A_B(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->b, true, 0);
    return 4;
}

proc ADD_A_C(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->c, true, 0);
    return 4;
}

proc ADD_A_D(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->d, true, 0);
    return 4;
}

proc ADD_A_E(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->e, true, 0);
    return 4;
}

proc ADD_A_H(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->h, true, 0);
    return 4;
}

proc ADD_A_L(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->l, true, 0);
    return 4;
}

proc ADD_A_HL(cpu* c) {
    u8 value = bus_read(c->_bus, HL(c));
    c->a = alu_add_sub(c, c->a, value, true, 0);
    return 8;
}

proc ADD_A_A(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->a, true, 0);
    return 4;
}

proc ADC_A_B(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->b, true, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc ADC_A_C(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->c, true, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc ADC_A_D(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->d, true, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc ADC_A_E(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->e, true, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc ADC_A_H(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->h, true, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc ADC_A_L(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->l, true, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc ADC_A_HL(cpu* c) {
    u8 value = bus_read(c->_bus, HL(c));
    c->a = alu_add_sub(c, c->a, value, true, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 8;
}

proc ADC_A_A(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->a, true, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

// 0x80-0x8F

// 0x90-0x9F

proc SUB_A_B(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->b, false, 0);
    return 4;
}

proc SUB_A_C(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->c, false, 0);
    return 4;
}

proc SUB_A_D(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->d, false, 0);
    return 4;
}

proc SUB_A_E(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->e, false, 0);
    return 4;
}

proc SUB_A_H(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->h, false, 0);
    return 4;
}

proc SUB_A_L(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->l, false, 0);
    return 4;
}

proc SUB_A_HL(cpu* c) {
    u8 value = bus_read(c->_bus, HL(c));
    c->a = alu_add_sub(c, c->a, value, false, 0);
    return 8; // cycles
}

proc SUB_A_A(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->a, false, 0);
    return 4;
}

proc SBC_A_B(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->b, false, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc SBC_A_C(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->c, false, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc SBC_A_D(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->d, false, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc SBC_A_E(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->e, false, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc SBC_A_H(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->h, false, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc SBC_A_L(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->l, false, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

proc SBC_A_HL(cpu* c) {
    u8 value = bus_read(c->_bus, HL(c));
    c->a = alu_add_sub(c, c->a, value, false, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 8;
}

proc SBC_A_A(cpu* c) {
    c->a = alu_add_sub(c, c->a, c->a, false, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 4;
}

// 0x90-0x9F

// 0xA0-0xAF

proc AND_A_B(cpu* c) {
    u8 result = c->a & c->b;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);
    cpu_set_flag(c, CPU_FLAGS_H);
    cpu_clear_flag(c, CPU_FLAGS_C);

    c->a = result;
    return 4;
}

proc AND_A_C(cpu* c) {
    u8 result = c->a & c->c;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);
    cpu_set_flag(c, CPU_FLAGS_H);
    cpu_clear_flag(c, CPU_FLAGS_C);

    c->a = result;
    return 4;
}

proc AND_A_D(cpu* c) {
    u8 result = c->a & c->d;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);
    cpu_set_flag(c, CPU_FLAGS_H);
    cpu_clear_flag(c, CPU_FLAGS_C);

    c->a = result;
    return 4;
}

proc AND_A_E(cpu* c) {
    u8 result = c->a & c->e;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);
    cpu_set_flag(c, CPU_FLAGS_H);
    cpu_clear_flag(c, CPU_FLAGS_C);

    c->a = result;
    return 4;
}

proc AND_A_H(cpu* c) {
    u8 result = c->a & c->h;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);
    cpu_set_flag(c, CPU_FLAGS_H);
    cpu_clear_flag(c, CPU_FLAGS_C);

    c->a = result;
    return 4;
}

proc AND_A_L(cpu* c) {
    u8 result = c->a & c->l;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);
    cpu_set_flag(c, CPU_FLAGS_H);
    cpu_clear_flag(c, CPU_FLAGS_C);

    c->a = result;
    return 4;
}

proc AND_A_HL(cpu* c) {
    u8 result = c->a & bus_read(c->_bus, HL(c));
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);
    cpu_set_flag(c, CPU_FLAGS_H);
    cpu_clear_flag(c, CPU_FLAGS_C);

    c->a = result;
    return 8;
}

proc AND_A_A(cpu* c) {
    u8 result = c->a & c->a;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);
    cpu_set_flag(c, CPU_FLAGS_H);
    cpu_clear_flag(c, CPU_FLAGS_C);

    c->a = result;
    return 4;
}

proc XOR_A_B(cpu* c) {
    u8 result = c->a ^ c->b;
    
    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 4;
}

proc XOR_A_C(cpu* c) {
    u8 result = c->a ^ c->c;

    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 4;
}

proc XOR_A_D(cpu* c) {
    u8 result = c->a ^ c->d;

    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 4;
}

proc XOR_A_E(cpu* c) {
    u8 result = c->a ^ c->e;

    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 4;
}

proc XOR_A_H(cpu* c) {
    u8 result = c->a ^ c->h;

    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 4;
}

proc XOR_A_L(cpu* c) {
    u8 result = c->a ^ c->l;

    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 4;
}

proc XOR_A_HL(cpu* c) {
    u8 result = c->a ^ bus_read(c->_bus, HL(c));

    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 8;
}

proc XOR_A_A(cpu* c) {
    c->a = 0;
    c->f = CPU_FLAGS_Z; // only Z is set
    return 4;
}

// 0xA0-0xAF

// 0xB0-0xBF

proc OR_A_B(cpu* c) {
    u8 result = c->a | c->b;

    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 4;
}

proc OR_A_C(cpu* c) {
    u8 result = c->a | c->c;

    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 4;
}

proc OR_A_D(cpu* c) {
    u8 result = c->a | c->d;

    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 4;
}

proc OR_A_E(cpu* c) {
    u8 result = c->a | c->e;

    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 4;
}

proc OR_A_H(cpu* c) {
    u8 result = c->a | c->h;

    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 4;
}

proc OR_A_L(cpu* c) {
    u8 result = c->a | c->l;

    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 4;
}

proc OR_A_HL(cpu* c) {
    u8 result = c->a | bus_read(c->_bus, HL(c));

    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 8;
}

proc OR_A_A(cpu* c) {
    u8 result = c->a | c->a;  // just A
    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;
    return 4;
}

proc CP_A_B(cpu* c) {
    u8 value = c->b;
    u16 result = c->a - value;
    
    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);
    
    // N always set (since it's subtraction)
    cpu_set_flag(c, CPU_FLAGS_N);
    
    // Half-carry (borrow from bit 4)
    if ((c->a & 0x0F) < (value & 0x0F)) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);
    
    // Carry (borrow from bit 8)
    if (c->a < value) cpu_set_flag(c, CPU_FLAGS_C); 
    else cpu_clear_flag(c, CPU_FLAGS_C);
    
    return 4; // cycles
}

proc CP_A_C(cpu* c) {
    u8 value = c->c;
    u16 result = c->a - value;

    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    // N always set (since it's subtraction)
    cpu_set_flag(c, CPU_FLAGS_N);

    // Half-carry (borrow from bit 4)
    if ((c->a & 0x0F) < (value & 0x0F)) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    // Carry (borrow from bit 8)
    if (c->a < value) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);

    return 4; // cycles
}

proc CP_A_D(cpu* c) {
    u8 value = c->d;
    u16 result = c->a - value;

    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    // N always set (since it's subtraction)
    cpu_set_flag(c, CPU_FLAGS_N);

    // Half-carry (borrow from bit 4)
    if ((c->a & 0x0F) < (value & 0x0F)) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    // Carry (borrow from bit 8)
    if (c->a < value) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);

    return 4; // cycles
}

proc CP_A_E(cpu* c) {
    u8 value = c->e;
    u16 result = c->a - value;

    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    // N always set (since it's subtraction)
    cpu_set_flag(c, CPU_FLAGS_N);

    // Half-carry (borrow from bit 4)
    if ((c->a & 0x0F) < (value & 0x0F)) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    // Carry (borrow from bit 8)
    if (c->a < value) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);

    return 4; // cycles
}

proc CP_A_H(cpu* c) {
    u8 value = c->h;
    u16 result = c->a - value;

    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    // N always set (since it's subtraction)
    cpu_set_flag(c, CPU_FLAGS_N);

    // Half-carry (borrow from bit 4)
    if ((c->a & 0x0F) < (value & 0x0F)) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    // Carry (borrow from bit 8)
    if (c->a < value) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);

    return 4; // cycles
}

proc CP_A_L(cpu* c) {
    u8 value = c->l;
    u16 result = c->a - value;

    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    // N always set (since it's subtraction)
    cpu_set_flag(c, CPU_FLAGS_N);

    // Half-carry (borrow from bit 4)
    if ((c->a & 0x0F) < (value & 0x0F)) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    // Carry (borrow from bit 8)
    if (c->a < value) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);

    return 4; // cycles
}

proc CP_A_HL(cpu* c) {
    u8 value = bus_read(c->_bus, HL(c));
    u16 result = c->a - value;

    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    // N always set (since it's subtraction)
    cpu_set_flag(c, CPU_FLAGS_N);

    // Half-carry (borrow from bit 4)
    if ((c->a & 0x0F) < (value & 0x0F)) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    // Carry (borrow from bit 8)
    if (c->a < value) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);

    return 8; // cycles
}

proc CP_A_A(cpu* c) {
    u8 value = c->a;
    u16 result = c->a - value;

    // Zero flag
    if ((result & 0xFF) == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    // N always set (since it's subtraction)
    cpu_set_flag(c, CPU_FLAGS_N);

    // Half-carry (borrow from bit 4)
    if ((c->a & 0x0F) < (value & 0x0F)) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    // Carry (borrow from bit 8)
    if (c->a < value) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);

    return 4; // cycles
}

// 0xB0-0xBF

// 0xC0-0xCF

proc RET_NZ(cpu* c) {
    if (!cpu_get_flag(c, CPU_FLAGS_Z)) {
        // Pop 16 bit address from stack (little endian)
        u16 addr = bus_read(c->_bus, c->sp) | (bus_read(c->_bus, c->sp + 1) << 8);
        c->sp += 2;
        c->pc = addr;
        return 20; // Return taken
    }

    return 8; // Return not taken
}

proc POP_BC(cpu* c) {
    u8 low = bus_read(c->_bus, c->sp);
    u8 high = bus_read(c->_bus, c->sp + 1);
    c->sp += 2;

    c->b = high;
    c->c = low;

    return 12;
}

proc JP_NZ_a16(cpu* c) {
    u16 addr = fetch_d16(c);
    if (!cpu_get_flag(c, CPU_FLAGS_Z)) {
        c->pc = addr;
        return 16; // jump taken
    }
    return 12; // jump not taken
}

proc JP_a16(cpu* c) {
    u16 addr = fetch_d16(c);
    c->pc = addr;
    return 16;
}

proc CALL_NZ_a16(cpu* c) {
    u16 addr = fetch_d16(c);

    if (!cpu_get_flag(c, CPU_FLAGS_Z)) {
        c->sp -= 2;
        bus_write(c->_bus, (c->pc & 0xFF), c->sp);
        bus_write(c->_bus, (c->pc >> 8) & 0xFF, c->sp + 1);

        c->pc = addr;
        return 24;
    }

    return 12;
}

proc PUSH_BC(cpu* c) {
    c->sp--;
    bus_write(c->_bus, c->b, c->sp);  // high byte
    c->sp--;
    bus_write(c->_bus, c->c, c->sp);  // low byte
    return 16;
}

proc ADD_A_d8(cpu* c) {
    u8 value = fetch(c);
    c->a = alu_add_sub(c, c->a, value, true, 0);
    return 8;
}

proc RST_00(cpu* c) {
    c->sp -= 2;
    bus_write(c->_bus, c->pc & 0xFF, c->sp);       // low byte
    bus_write(c->_bus, (c->pc >> 8) & 0xFF, c->sp + 1); // high byte

    c->pc = 0x00;
    return 16;
}

proc RET_Z(cpu* c) {
    if (cpu_get_flag(c, CPU_FLAGS_Z)) {
        // Pop 16 bit address from stack (little endian)
        u16 addr = bus_read(c->_bus, c->sp) | (bus_read(c->_bus, c->sp + 1) << 8);
        c->sp += 2;
        c->pc = addr;
        return 20; // Return taken
    }

    return 8; // Return not taken
}

proc RET(cpu* c) {
    // Pop 16 bit address from stack (little endian)
    u16 addr = bus_read(c->_bus, c->sp) | (bus_read(c->_bus, c->sp + 1) << 8);
    c->sp += 2;
    c->pc = addr;
    return 16; // Return taken
}

proc JP_Z_a16(cpu* c) {
    u16 addr = fetch_d16(c);
    if (cpu_get_flag(c, CPU_FLAGS_Z)) {
        c->pc = addr;
        return 16; // jump taken
    }
    return 12; // jump not taken
}

// -- CB PREFIXED --

// Rotate Left Circular (RLC) - bit7 -> bit0 & carry
static inline u8 rlc(cpu* c, u8 val) {
    u8 res = (val << 1) | (val >> 7);

    cpu_clear_flag(c, CPU_FLAGS_N | CPU_FLAGS_H);
    if (res == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);
    if (val & 0x80) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);

    return res;
}

// Rotate Right Circular (RRC) - bit0 -> bit7 & carry
static inline u8 rrc(cpu* c, u8 val) {
    u8 res = (val >> 1) | (val << 7);

    cpu_clear_flag(c, CPU_FLAGS_N | CPU_FLAGS_H);
    if (res == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);
    if (val & 0x01) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);

    return res;
}

// Rotate Left through Carry (RL)
static inline u8 rl(cpu* c, u8 val) {
    u8 carry_in = cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0;
    u8 carry_out = (val & 0x80) ? 1 : 0;
    u8 res = (val << 1) | carry_in;

    cpu_clear_flag(c, CPU_FLAGS_N | CPU_FLAGS_H);
    if (res == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);
    if (carry_out) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);

    return res;
}

// Rotate Right through Carry (RR)
static inline u8 rr(cpu* c, u8 val) {
    u8 carry_in = cpu_get_flag(c, CPU_FLAGS_C) ? 0x80 : 0;
    u8 carry_out = (val & 0x01) ? 1 : 0;
    u8 res = (val >> 1) | carry_in;

    cpu_clear_flag(c, CPU_FLAGS_N | CPU_FLAGS_H);
    if (res == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);
    if (carry_out) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);

    return res;
}

// Shift Left Arithmetic (SLA) - bit7 -> carry, fill 0
static inline u8 sla(cpu* c, u8 val) {
    u8 carry_out = (val & 0x80) ? 1 : 0;
    u8 res = val << 1;

    cpu_clear_flag(c, CPU_FLAGS_N | CPU_FLAGS_H);
    if (res == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);
    if (carry_out) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);

    return res;
}

// Shift Right Arithmetic (SRA) - keep bit7, bit0 -> carry
static inline u8 sra(cpu* c, u8 val) {
    u8 carry_out = (val & 0x01) ? 1 : 0;
    u8 res = (val >> 1) | (val & 0x80);

    cpu_clear_flag(c, CPU_FLAGS_N | CPU_FLAGS_H);
    if (res == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);
    if (carry_out) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);

    return res;
}

// Swap upper/lower nibbles
static inline u8 swap(cpu* c, u8 val) {
    u8 res = (val >> 4) | (val << 4);

    cpu_clear_flag(c, CPU_FLAGS_N | CPU_FLAGS_H | CPU_FLAGS_C);
    if (res == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    return res;
}

// Shift Right Logical (SRL) - bit0 -> carry, fill 0
static inline u8 srl(cpu* c, u8 val) {
    u8 carry_out = (val & 0x01) ? 1 : 0;
    u8 res = val >> 1;

    cpu_clear_flag(c, CPU_FLAGS_N | CPU_FLAGS_H);
    if (res == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);
    if (carry_out) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);

    return res;
}

// Test bit b of val
static inline void bit(cpu* c, int b, u8 val) {
    if (val & (1 << b)) cpu_clear_flag(c, CPU_FLAGS_Z);
    else cpu_set_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);
    cpu_set_flag(c, CPU_FLAGS_H);
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
static inline u8* decode_reg(cpu* c, int idx) {
    switch (idx) {
    case 0: return &c->b;
    case 1: return &c->c;
    case 2: return &c->d;
    case 3: return &c->e;
    case 4: return &c->h;
    case 5: return &c->l;
    case 6: return NULL; // special: (HL)
    case 7: return &c->a;
    }
    return NULL; // should never happen
}

proc PREFIX(cpu* c) {
    u8 op = fetch(c);
    int reg_idx = op & 0x07;
    u8* reg = decode_reg(c, reg_idx);
    u8 val = reg ? *reg : bus_read(c->_bus, HL(c));

    int cycles;
    if (reg) cycles = 8;
    else if (op < 0x40 || op >= 0x80) cycles = 16; // modifies memory
    else cycles = 12; // BIT b,(HL)

    if (op < 0x40) {
        int group = (op >> 3) & 7;
        switch (group) {
            case 0: val = rlc(c, val); break;
            case 1: val = rrc(c, val); break;
            case 2: val = rl(c, val);  break;
            case 3: val = rr(c, val);  break;
            case 4: val = sla(c, val); break;
            case 5: val = sra(c, val); break;
            case 6: val = swap(c, val); break;
            case 7: val = srl(c, val); break;
        }
    }
    else if (op < 0x80) {
        bit(c, (op >> 3) & 7, val);
    }
    else if (op < 0xC0) {
        val = res((op >> 3) & 7, val);
    }
    else {
        val = set((op >> 3) & 7, val);
    }

    if (reg) *reg = val;
    else if (op < 0x40 || op >= 0x80) bus_write(c->_bus, val, HL(c));

    return (u8)cycles;
}

proc CALL_Z_a16(cpu* c) {
    u16 addr = fetch_d16(c);

    if (cpu_get_flag(c, CPU_FLAGS_Z)) {
        c->sp -= 2;
        bus_write(c->_bus, (c->pc & 0xFF), c->sp);
        bus_write(c->_bus, (c->pc >> 8) & 0xFF, c->sp + 1);

        c->pc = addr;
        return 24;
    }

    return 12;
}

proc CALL_a16(cpu* c) {
    u16 addr = fetch_d16(c);

    c->sp -= 2;
    bus_write(c->_bus, (c->pc & 0xFF), c->sp);
    bus_write(c->_bus, (c->pc >> 8) & 0xFF, c->sp + 1);

    c->pc = addr;
    return 24;
}

proc ADC_A_d8(cpu* c) {
    u8 value = fetch(c);
    c->a = alu_add_sub(c, c->a, value, true, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 8;
}

proc RST_08(cpu* c) {
    c->sp -= 2;

    bus_write(c->_bus, c->pc & 0xFF, c->sp);       // low byte
    bus_write(c->_bus, (c->pc >> 8) & 0xFF, c->sp + 1); // high byte

    c->pc = 0x08;
    return 16;
}

// 0xC0-0xCF

// 0xD0-0xDF

proc RET_NC(cpu* c) {
    if (!cpu_get_flag(c, CPU_FLAGS_C)) {
        // Pop 16 bit address from stack (little endian)
        u16 addr = bus_read(c->_bus, c->sp) | (bus_read(c->_bus, c->sp + 1) << 8);
        c->sp += 2;
        c->pc = addr;
        return 20; // Return taken
    }

    return 8; // Return not taken
}

proc POP_DE(cpu* c) {
    u8 low = bus_read(c->_bus, c->sp);
    u8 high = bus_read(c->_bus, c->sp + 1);
    c->sp += 2;

    c->d = high;
    c->e = low;

    return 12;
}

proc JP_NC_a16(cpu* c) {
    u16 addr = fetch_d16(c);
    if (!cpu_get_flag(c, CPU_FLAGS_C)) {
        c->pc = addr;
        return 16; // jump taken
    }
    return 12; // jump not taken
}

proc CALL_NC_a16(cpu* c) {
    u16 addr = fetch_d16(c);

    if (!cpu_get_flag(c, CPU_FLAGS_C)) {
        c->sp -= 2;
        bus_write(c->_bus, (c->pc & 0xFF), c->sp);
        bus_write(c->_bus, (c->pc >> 8) & 0xFF, c->sp + 1);

        c->pc = addr;
        return 24;
    }

    return 12;
}

proc PUSH_DE(cpu* c) {
    c->sp--;
    bus_write(c->_bus, c->d, c->sp);
    c->sp--;
    bus_write(c->_bus, c->e, c->sp);

    return 16;
}

proc SUB_A_d8(cpu* c) {
    u8 value = fetch(c);
    c->a = alu_add_sub(c, c->a, value, false, 0);
    return 8;
}

proc RST_10(cpu* c) {
    c->sp -= 2;
    bus_write(c->_bus, c->pc & 0xFF, c->sp);       // low byte
    bus_write(c->_bus, (c->pc >> 8) & 0xFF, c->sp + 1); // high byte

    c->pc = 0x10;
    return 16;
}

proc RET_C(cpu* c) {
    if (cpu_get_flag(c, CPU_FLAGS_C)) {
        // Pop 16 bit address from stack (little endian)
        u16 addr = bus_read(c->_bus, c->sp) | (bus_read(c->_bus, c->sp + 1) << 8);
        c->sp += 2;
        c->pc = addr;
        return 20; // Return taken
    }

    return 8; // Return not taken
}

proc RETI(cpu* c) {
    // Pop 16 bit address from stack (little endian)
    u16 addr = bus_read(c->_bus, c->sp) | (bus_read(c->_bus, c->sp + 1) << 8);
    c->sp += 2;
    c->pc = addr;
    c->ints_enabled = 1;
    return 16; // Return taken
}

proc JP_C_a16(cpu* c) {
    u16 addr = fetch_d16(c);
    if (cpu_get_flag(c, CPU_FLAGS_C)) {
        c->pc = addr;
        return 16; // jump taken
    }
    return 12; // jump not taken
}

proc CALL_C_a16(cpu* c) {
    u16 addr = fetch_d16(c);

    if (cpu_get_flag(c, CPU_FLAGS_C)) {
        c->sp -= 2;
        bus_write(c->_bus, (c->pc & 0xFF), c->sp);
        bus_write(c->_bus, (c->pc >> 8) & 0xFF, c->sp + 1);

        c->pc = addr;
        return 24;
    }

    return 12;
}

proc SBC_A_d8(cpu* c) {
    u8 value = fetch(c);
    c->a = alu_add_sub(c, c->a, value, false, cpu_get_flag(c, CPU_FLAGS_C) ? 1 : 0);
    return 8;
}

proc RST_18(cpu* c) {
    c->sp -= 2;
    bus_write(c->_bus, c->pc & 0xFF, c->sp);       // low byte
    bus_write(c->_bus, (c->pc >> 8) & 0xFF, c->sp + 1); // high byte

    c->pc = 0x18;
    return 16;
}

// 0xD0-0xDF

// 0xE0-0xEF

proc LDH_a8_A(cpu* c) {
    u8 value = fetch(c);
    bus_write(c->_bus, c->a, 0xFF00 + value);
    return 48;
}

proc POP_HL(cpu* c) {
    u8 low = bus_read(c->_bus, c->sp);
    u8 high = bus_read(c->_bus, c->sp + 1);
    c->sp += 2;

    c->h = high;
    c->l = low;

    return 12;
}

proc LDH_C_A(cpu* c) {
    bus_write(c->_bus, c->a, 0xFF00 + c->c);
    return 8;
}

proc PUSH_HL(cpu* c) {
    c->sp--;
    bus_write(c->_bus, c->h, c->sp);
    c->sp--;
    bus_write(c->_bus, c->l, c->sp);

    return 16;
}

proc AND_A_d8(cpu* c) {
    u8 result = c->a & fetch(c);
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    cpu_clear_flag(c, CPU_FLAGS_N);
    cpu_set_flag(c, CPU_FLAGS_H);
    cpu_clear_flag(c, CPU_FLAGS_C);

    c->a = result;
    return 8;
}

proc RST_20(cpu* c) {
    c->sp -= 2;
    bus_write(c->_bus, c->pc & 0xFF, c->sp);       // low byte
    bus_write(c->_bus, (c->pc >> 8) & 0xFF, c->sp + 1); // high byte

    c->pc = 0x20;
    return 16;
}

proc ADD_SP_i8(cpu* c) {
    i8 imm = (i8)fetch(c);
    u16 sp = c->sp;
    u16 result = sp + imm;

    cpu_clear_flag(c, CPU_FLAGS_Z);
    cpu_clear_flag(c, CPU_FLAGS_N);

    // Half-carry: nibble carry
    if (((sp & 0x0F) + (imm & 0x0F)) > 0x0F)
        cpu_set_flag(c, CPU_FLAGS_H);
    else
        cpu_clear_flag(c, CPU_FLAGS_H);

    // Carry: byte carry
    if (((sp & 0xFF) + (imm & 0xFF)) > 0xFF)
        cpu_set_flag(c, CPU_FLAGS_C);
    else
        cpu_clear_flag(c, CPU_FLAGS_C);

    c->sp = result;

    return 16;
}

proc JP_HL(cpu* c) {
    c->pc = HL(c);
    return 4;
}

proc LD_a16_A(cpu* c) {
    u16 value = fetch_d16(c);
    bus_write(c->_bus, c->a, value);
    return 16;
}

proc XOR_A_d8(cpu* c) {
    u8 result = c->a ^ fetch(c);

    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 8;
}

proc RST_28(cpu* c) {
    c->sp -= 2;
    bus_write(c->_bus, c->pc & 0xFF, c->sp);       // low byte
    bus_write(c->_bus, (c->pc >> 8) & 0xFF, c->sp + 1); // high byte

    c->pc = 0x28;
    return 16;
}

// 0xE0-0xEF

// 0xF0-0xFF

proc LDH_A_a8(cpu* c) {
    u8 imm = fetch(c);
    u8 value = bus_read(c->_bus, 0xFF00 + imm);
    c->a = value;
    return 48;
}

proc POP_AF(cpu* c) {
    u8 low = bus_read(c->_bus, c->sp);
    u8 high = bus_read(c->_bus, c->sp + 1);
    c->sp += 2;
    c->a = high;
    c->f = low & 0xF0;

    return 12;
}

proc LDH_A_C(cpu* c) {
    c->a = bus_read(c->_bus, 0xFF00 + c->c);
    return 8;
}

proc DI(cpu* c) {
    c->ints_enabled = false;
    c->int_next = false; // cancel pending EI
    return 4;
}

proc PUSH_AF(cpu* c) {
    c->sp--;
    bus_write(c->_bus, c->a, c->sp);
    c->sp--;
    bus_write(c->_bus, c->f & 0xF0, c->sp);

    return 16;
}

proc OR_A_d8(cpu* c) {
    u8 result = c->a | fetch(c);

    c->f = 0;
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    c->a = result;

    return 4;
}

proc RST_30(cpu* c) {
    c->sp -= 2;
    bus_write(c->_bus, c->pc & 0xFF, c->sp);       // low byte
    bus_write(c->_bus, (c->pc >> 8) & 0xFF, c->sp + 1); // high byte

    c->pc = 0x30;
    return 16;
}

proc LD_HL_SP_PLUS_i8(cpu* c) {
    i8 offset = (i8)fetch(c);
    u16 sp = c->sp;
    u16 result = sp + offset;

    cpu_clear_flag(c, CPU_FLAGS_Z);
    cpu_clear_flag(c, CPU_FLAGS_N);

    // Half-carry: check nibble carry from (sp & 0xF) + (offset & 0xF)
    if (((sp & 0x0F) + (offset & 0x0F)) > 0x0F)
        cpu_set_flag(c, CPU_FLAGS_H);
    else
        cpu_clear_flag(c, CPU_FLAGS_H);

    // Carry: check byte carry from (sp & 0xFF) + (offset & 0xFF)
    if (((sp & 0xFF) + (offset & 0xFF)) > 0xFF)
        cpu_set_flag(c, CPU_FLAGS_C);
    else
        cpu_clear_flag(c, CPU_FLAGS_C);

    SET_HL(c, result);

    return 12;
}

proc LD_SP_HL(cpu* c) {
    c->sp = HL(c);
    return 8;
}

proc LD_A_a16(cpu* c) {
    u16 value = fetch_d16(c);
    c->a = bus_read(c->_bus, value);
    return 16;
}

proc EI(cpu* c) {
    c->int_next = true;
    return 4;
}

proc CP_A_d8(cpu* c) {
    u8 value = fetch(c);
    u8 a = c->a;
    u8 result = a - value;

    // Zero flag
    if (result == 0) cpu_set_flag(c, CPU_FLAGS_Z);
    else cpu_clear_flag(c, CPU_FLAGS_Z);

    // Subtract flag
    cpu_set_flag(c, CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((a & 0x0F) < (value & 0x0F)) cpu_set_flag(c, CPU_FLAGS_H);
    else cpu_clear_flag(c, CPU_FLAGS_H);

    if (a < value) cpu_set_flag(c, CPU_FLAGS_C);
    else cpu_clear_flag(c, CPU_FLAGS_C);

    return 8; // 8 T-cycles
}

proc RST_38(cpu* c) {
    c->sp -= 2;
    bus_write(c->_bus, c->pc & 0xFF, c->sp);       // low byte
    bus_write(c->_bus, (c->pc >> 8) & 0xFF, c->sp + 1); // high byte

    c->pc = 0x38;
    return 16;
}

// 0xF0-0xFF

proc INVALID(cpu* c) {
    printf("INVALID INSTRUCTION EXECUTED!\n");
    exit(1);
    return 4;
}

// -- INSTRUCTIONS --

cpu* cpu_init(u8* cart, size_t cart_size) {
    cpu* _cpu = malloc(sizeof(cpu));
    memset(_cpu, 0, sizeof(cpu));

    // Allocate RAM
    _cpu->memory = malloc(0x10000); // 64KB RAM/IO
    memset(_cpu->memory, 0, 0x10000);

    // Store cart separately
    _cpu->cart = malloc(cart_size);
    memcpy(_cpu->cart, cart, cart_size);
    _cpu->cart_size = (u32)cart_size;
    _cpu->rom_bank = 1;

    // PC starts at 0x0100
    _cpu->pc = 0x0100;

    // SP defaults to 0xFFFE
    _cpu->sp = 0xFFFE;
    _cpu->a = 0x1;
    _cpu->f |= CPU_FLAGS_Z | CPU_FLAGS_H | CPU_FLAGS_C;
    SET_BC(_cpu, 0x0013);
    SET_DE(_cpu, 0x00D8);
    SET_HL(_cpu, 0x014D);
    
    // setup optable
    _cpu->optable = (instruction*)malloc(sizeof(instruction) * 512);
    memset(_cpu->optable, 0, sizeof(instruction) * 512);
    
    _cpu->optable[0x00] = (instruction){ .func = NOP,       .name = "NOP"       };
    _cpu->optable[0x01] = (instruction){ .func = LD_BC_d16, .name = "LD_BC_d16" };
    _cpu->optable[0x02] = (instruction){ .func = LD_BC_A,   .name = "LD_BC_A"   };
    _cpu->optable[0x03] = (instruction){ .func = INC_BC,    .name = "INC_BC"    };
    _cpu->optable[0x04] = (instruction){ .func = INC_B,     .name = "INC_B"     };
    _cpu->optable[0x05] = (instruction){ .func = DEC_B,     .name = "DEC_B"     };
    _cpu->optable[0x06] = (instruction){ .func = LD_B_d8,   .name = "LD_B_d8"   };
    _cpu->optable[0x07] = (instruction){ .func = RLCA,      .name = "RLCA"      };
    _cpu->optable[0x08] = (instruction){ .func = LD_a16_SP, .name = "LD_a16_SP" };
    _cpu->optable[0x09] = (instruction){ .func = ADD_HL_BC, .name = "ADD_HL_BC" };
    _cpu->optable[0x0A] = (instruction){ .func = LD_A_BC,   .name = "LD_A_BC"   };
    _cpu->optable[0x0B] = (instruction){ .func = DEC_BC,    .name = "DEC_BC"    };
    _cpu->optable[0x0C] = (instruction){ .func = INC_C,     .name = "INC_C"     };
    _cpu->optable[0x0D] = (instruction){ .func = DEC_C,     .name = "DEC_C"     };
    _cpu->optable[0x0E] = (instruction){ .func = LD_C_d8,   .name = "LD_C_d8"   };
    _cpu->optable[0x0F] = (instruction){ .func = RRCA,      .name = "RRCA"      };

    _cpu->optable[0x10] = (instruction){ .func = STOP,      .name = "STOP"      };
    _cpu->optable[0x11] = (instruction){ .func = LD_DE_d16, .name = "LD_DE_d16" };
    _cpu->optable[0x12] = (instruction){ .func = LD_DE_A,   .name = "LD_DE_A"   };
    _cpu->optable[0x13] = (instruction){ .func = INC_DE,    .name = "INC_DE"    };
    _cpu->optable[0x14] = (instruction){ .func = INC_D,     .name = "INC_D"     };
    _cpu->optable[0x15] = (instruction){ .func = DEC_D,     .name = "DEC_D"     };
    _cpu->optable[0x16] = (instruction){ .func = LD_D_d8,   .name = "LD_D_d8"   };
    _cpu->optable[0x17] = (instruction){ .func = RLA,       .name = "RLA"       };
    _cpu->optable[0x18] = (instruction){ .func = JR_s8,     .name = "JR_s8"     };
    _cpu->optable[0x19] = (instruction){ .func = ADD_HL_DE, .name = "ADD_HL_DE" };
    _cpu->optable[0x1A] = (instruction){ .func = LD_A_DE,   .name = "LD_A_DE"   };
    _cpu->optable[0x1B] = (instruction){ .func = DEC_DE,    .name = "DEC_DE"    };
    _cpu->optable[0x1C] = (instruction){ .func = INC_E,     .name = "INC_E"     };
    _cpu->optable[0x1D] = (instruction){ .func = DEC_E,     .name = "DEC_E"     };
    _cpu->optable[0x1E] = (instruction){ .func = LD_E_d8,   .name = "LD_E_d8"   };
    _cpu->optable[0x1F] = (instruction){ .func = RRA,       .name = "RRA"       };

    _cpu->optable[0x20] = (instruction){ .func = JR_NZ_s8,  .name = "JR_NZ_s8"  };
    _cpu->optable[0x21] = (instruction){ .func = LD_HL_d16, .name = "LD_HL_d16" };
    _cpu->optable[0x22] = (instruction){ .func = LD_HLI_A,  .name = "LD_HLI_A"  };
    _cpu->optable[0x23] = (instruction){ .func = INC_HL,    .name = "INC_HL"    };
    _cpu->optable[0x24] = (instruction){ .func = INC_H,     .name = "INC_H"     };
    _cpu->optable[0x25] = (instruction){ .func = DEC_H,     .name = "DEC_H"     };
    _cpu->optable[0x26] = (instruction){ .func = LD_H_d8,   .name = "LD_H_d8"   };
    _cpu->optable[0x27] = (instruction){ .func = DAA,       .name = "DAA"       };
    _cpu->optable[0x28] = (instruction){ .func = JR_Z_s8,   .name = "JR_Z_s8"   };
    _cpu->optable[0x29] = (instruction){ .func = ADD_HL_HL, .name = "ADD_HL_HL" };
    _cpu->optable[0x2A] = (instruction){ .func = LD_A_HLI,  .name = "LD_A_HLI"  };
    _cpu->optable[0x2B] = (instruction){ .func = DEC_HL,    .name = "DEC_HL"    };
    _cpu->optable[0x2C] = (instruction){ .func = INC_L,     .name = "INC_L"     };
    _cpu->optable[0x2D] = (instruction){ .func = DEC_L,     .name = "DEC_L"     };
    _cpu->optable[0x2E] = (instruction){ .func = LD_L_d8,   .name = "LD_L_d8"   };
    _cpu->optable[0x2F] = (instruction){ .func = CPL,       .name = "CPL"       };

    _cpu->optable[0x30] = (instruction){ .func = JR_NC_s8,  .name = "JR_NC_s8"  };
    _cpu->optable[0x31] = (instruction){ .func = LD_SP_d16, .name = "LD_SP_d16" };
    _cpu->optable[0x32] = (instruction){ .func = LD_HLD_A,  .name = "LD_HLD_A"  };
    _cpu->optable[0x33] = (instruction){ .func = INC_SP,    .name = "INC_SP"    };
    _cpu->optable[0x34] = (instruction){ .func = INC_IND_HL,.name = "INC_IND_HL"};
    _cpu->optable[0x35] = (instruction){ .func = DEC_IND_HL,.name = "DEC_IND_HL"};
    _cpu->optable[0x36] = (instruction){ .func = LD_HL_d8,  .name = "LD_HL_d8"  };
    _cpu->optable[0x37] = (instruction){ .func = SCF,       .name = "SCF"       };
    _cpu->optable[0x38] = (instruction){ .func = JR_C_s8,   .name = "JR_C_s8"   };
    _cpu->optable[0x39] = (instruction){ .func = ADD_HL_SP, .name = "ADD_HL_SP" };
    _cpu->optable[0x3A] = (instruction){ .func = LD_A_HLD,  .name = "LD_A_HLD"  };
    _cpu->optable[0x3B] = (instruction){ .func = DEC_SP,    .name = "DEC_SP"    };
    _cpu->optable[0x3C] = (instruction){ .func = INC_A,     .name = "INC_A"     };
    _cpu->optable[0x3D] = (instruction){ .func = DEC_A,     .name = "DEC_A"     };
    _cpu->optable[0x3E] = (instruction){ .func = LD_A_d8,   .name = "LD_A_d8"   };
    _cpu->optable[0x3F] = (instruction){ .func = CCF,       .name = "CCF"       };

    _cpu->optable[0x40] = (instruction){ .func = LD_B_B, .name = "LD_B_B"       };
    _cpu->optable[0x41] = (instruction){ .func = LD_B_C, .name = "LD_B_C"       };
    _cpu->optable[0x42] = (instruction){ .func = LD_B_D, .name = "LD_B_D"       };
    _cpu->optable[0x43] = (instruction){ .func = LD_B_E, .name = "LD_B_E"       };
    _cpu->optable[0x44] = (instruction){ .func = LD_B_H, .name = "LD_B_H"       };
    _cpu->optable[0x45] = (instruction){ .func = LD_B_L, .name = "LD_B_L"       };
    _cpu->optable[0x46] = (instruction){ .func = LD_B_HL,.name = "LD_B_HL"      };
    _cpu->optable[0x47] = (instruction){ .func = LD_B_A, .name = "LD_B_A"       };
    _cpu->optable[0x48] = (instruction){ .func = LD_C_B, .name = "LD_C_B"       };
    _cpu->optable[0x49] = (instruction){ .func = LD_C_C, .name = "LD_C_C"       };
    _cpu->optable[0x4A] = (instruction){ .func = LD_C_D, .name = "LD_C_D"       };
    _cpu->optable[0x4B] = (instruction){ .func = LD_C_E, .name = "LD_C_E"       };
    _cpu->optable[0x4C] = (instruction){ .func = LD_C_H, .name = "LD_C_H"       };
    _cpu->optable[0x4D] = (instruction){ .func = LD_C_L, .name = "LD_C_L"       };
    _cpu->optable[0x4E] = (instruction){ .func = LD_C_HL,.name = "LD_C_HL"      };
    _cpu->optable[0x4F] = (instruction){ .func = LD_C_A, .name = "LD_C_A"       };

    _cpu->optable[0x50] = (instruction){ .func = LD_D_B, .name = "LD_D_B"       };
    _cpu->optable[0x51] = (instruction){ .func = LD_D_C, .name = "LD_D_C"       };
    _cpu->optable[0x52] = (instruction){ .func = LD_D_D, .name = "LD_D_D"       };
    _cpu->optable[0x53] = (instruction){ .func = LD_D_E, .name = "LD_D_E"       };
    _cpu->optable[0x54] = (instruction){ .func = LD_D_H, .name = "LD_D_H"       };
    _cpu->optable[0x55] = (instruction){ .func = LD_D_L, .name = "LD_D_L"       };
    _cpu->optable[0x56] = (instruction){ .func = LD_D_HL,.name = "LD_D_HL"      };
    _cpu->optable[0x57] = (instruction){ .func = LD_D_A, .name = "LD_D_A"       };
    _cpu->optable[0x58] = (instruction){ .func = LD_E_B, .name = "LD_E_B"       };
    _cpu->optable[0x59] = (instruction){ .func = LD_E_C, .name = "LD_E_C"       };
    _cpu->optable[0x5A] = (instruction){ .func = LD_E_D, .name = "LD_E_D"       };
    _cpu->optable[0x5B] = (instruction){ .func = LD_E_E, .name = "LD_E_E"       };
    _cpu->optable[0x5C] = (instruction){ .func = LD_E_H, .name = "LD_E_H"       };
    _cpu->optable[0x5D] = (instruction){ .func = LD_E_L, .name = "LD_E_L"       };
    _cpu->optable[0x5E] = (instruction){ .func = LD_E_HL,.name = "LD_E_HL"      };
    _cpu->optable[0x5F] = (instruction){ .func = LD_E_A, .name = "LD_E_A"       };

    _cpu->optable[0x60] = (instruction){ .func = LD_H_B, .name = "LD_H_B"       };
    _cpu->optable[0x61] = (instruction){ .func = LD_H_C, .name = "LD_H_C"       };
    _cpu->optable[0x62] = (instruction){ .func = LD_H_D, .name = "LD_H_D"       };
    _cpu->optable[0x63] = (instruction){ .func = LD_H_E, .name = "LD_H_E"       };
    _cpu->optable[0x64] = (instruction){ .func = LD_H_H, .name = "LD_H_H"       };
    _cpu->optable[0x65] = (instruction){ .func = LD_H_L, .name = "LD_H_L"       };
    _cpu->optable[0x66] = (instruction){ .func = LD_H_HL,.name = "LD_H_HL"      };
    _cpu->optable[0x67] = (instruction){ .func = LD_H_A, .name = "LD_H_A"       };
    _cpu->optable[0x68] = (instruction){ .func = LD_L_B, .name = "LD_L_B"       };
    _cpu->optable[0x69] = (instruction){ .func = LD_L_C, .name = "LD_L_C"       };
    _cpu->optable[0x6A] = (instruction){ .func = LD_L_D, .name = "LD_L_D"       };
    _cpu->optable[0x6B] = (instruction){ .func = LD_L_E, .name = "LD_L_E"       };
    _cpu->optable[0x6C] = (instruction){ .func = LD_L_H, .name = "LD_L_H" };
    _cpu->optable[0x6D] = (instruction){ .func = LD_L_L, .name = "LD_L_L"       };
    _cpu->optable[0x6E] = (instruction){ .func = LD_L_HL,.name = "LD_L_HL"      };
    _cpu->optable[0x6F] = (instruction){ .func = LD_L_A, .name = "LD_L_A"       };

    _cpu->optable[0x70] = (instruction){ .func = LD_HL_B,.name = "LD_HL_B"      };
    _cpu->optable[0x71] = (instruction){ .func = LD_HL_C,.name = "LD_HL_C"      };
    _cpu->optable[0x72] = (instruction){ .func = LD_HL_D,.name = "LD_HL_D"      };
    _cpu->optable[0x73] = (instruction){ .func = LD_HL_E,.name = "LD_HL_E"      };
    _cpu->optable[0x74] = (instruction){ .func = LD_HL_H,.name = "LD_HL_H"      };
    _cpu->optable[0x75] = (instruction){ .func = LD_HL_L,.name = "LD_HL_L"      };
    _cpu->optable[0x76] = (instruction){ .func = HALT,   .name = "HALT"         };
    _cpu->optable[0x77] = (instruction){ .func = LD_HL_A,.name = "LD_HL_A"      };
    _cpu->optable[0x78] = (instruction){ .func = LD_A_B, .name = "LD_A_B"       };
    _cpu->optable[0x79] = (instruction){ .func = LD_A_C, .name = "LD_A_C"       };
    _cpu->optable[0x7A] = (instruction){ .func = LD_A_D, .name = "LD_A_D"       };
    _cpu->optable[0x7B] = (instruction){ .func = LD_A_E, .name = "LD_A_E"       };
    _cpu->optable[0x7C] = (instruction){ .func = LD_A_H, .name = "LD_A_H"       };
    _cpu->optable[0x7D] = (instruction){ .func = LD_A_L, .name = "LD_A_L"       };
    _cpu->optable[0x7E] = (instruction){ .func = LD_A_HL,.name = "LD_A_HL"      };
    _cpu->optable[0x7F] = (instruction){ .func = LD_A_A, .name = "LD_A_A"       };

    _cpu->optable[0x80] = (instruction){ .func = ADD_A_B, .name = "ADD_A_B"     };
    _cpu->optable[0x81] = (instruction){ .func = ADD_A_C, .name = "ADD_A_C"     };
    _cpu->optable[0x82] = (instruction){ .func = ADD_A_D, .name = "ADD_A_D"     };
    _cpu->optable[0x83] = (instruction){ .func = ADD_A_E, .name = "ADD_A_E"     };
    _cpu->optable[0x84] = (instruction){ .func = ADD_A_H, .name = "ADD_A_H"     };
    _cpu->optable[0x85] = (instruction){ .func = ADD_A_L, .name = "ADD_A_L"     };
    _cpu->optable[0x86] = (instruction){ .func = ADD_A_HL,.name = "ADD_A_HL"    };
    _cpu->optable[0x87] = (instruction){ .func = ADD_A_A, .name = "ADD_A_A"     };
    _cpu->optable[0x88] = (instruction){ .func = ADC_A_B, .name = "ADC_A_B"     };
    _cpu->optable[0x89] = (instruction){ .func = ADC_A_C, .name = "ADC_A_C"     };
    _cpu->optable[0x8A] = (instruction){ .func = ADC_A_D, .name = "ADC_A_D"     };
    _cpu->optable[0x8B] = (instruction){ .func = ADC_A_E, .name = "ADC_A_E"     };
    _cpu->optable[0x8C] = (instruction){ .func = ADC_A_H, .name = "ADC_A_H"     };
    _cpu->optable[0x8D] = (instruction){ .func = ADC_A_L, .name = "ADC_A_L"     };
    _cpu->optable[0x8E] = (instruction){ .func = ADC_A_HL,.name = "ADC_A_HL"    };
    _cpu->optable[0x8F] = (instruction){ .func = ADC_A_A, .name = "ADC_A_A"     };

    _cpu->optable[0x90] = (instruction){ .func = SUB_A_B,  .name = "SUB_A_B"    };
    _cpu->optable[0x91] = (instruction){ .func = SUB_A_C,  .name = "SUB_A_C"    };
    _cpu->optable[0x92] = (instruction){ .func = SUB_A_D,  .name = "SUB_A_D"    };
    _cpu->optable[0x93] = (instruction){ .func = SUB_A_E,  .name = "SUB_A_E"    };
    _cpu->optable[0x94] = (instruction){ .func = SUB_A_H,  .name = "SUB_A_H"    };
    _cpu->optable[0x95] = (instruction){ .func = SUB_A_L,  .name = "SUB_A_L"    };
    _cpu->optable[0x96] = (instruction){ .func = SUB_A_HL, .name = "SUB_A_HL"   };
    _cpu->optable[0x97] = (instruction){ .func = SUB_A_A,  .name = "SUB_A_A"    };
    _cpu->optable[0x98] = (instruction){ .func = SBC_A_B,  .name = "SBC_A_B"    };
    _cpu->optable[0x99] = (instruction){ .func = SBC_A_C,  .name = "SBC_A_C"    };
    _cpu->optable[0x9A] = (instruction){ .func = SBC_A_D,  .name = "SBC_A_D"    };
    _cpu->optable[0x9B] = (instruction){ .func = SBC_A_E,  .name = "SBC_A_E"    };
    _cpu->optable[0x9C] = (instruction){ .func = SBC_A_H,  .name = "SBC_A_H"    };
    _cpu->optable[0x9D] = (instruction){ .func = SBC_A_L,  .name = "SBC_A_L"    };
    _cpu->optable[0x9E] = (instruction){ .func = SBC_A_HL, .name = "SBC_A_HL"   };
    _cpu->optable[0x9F] = (instruction){ .func = SBC_A_A,  .name = "SBC_A_A"    };

    _cpu->optable[0xA0] = (instruction){ .func = AND_A_B,   .name = "AND_A_B"  };
    _cpu->optable[0xA1] = (instruction){ .func = AND_A_C,   .name = "AND_A_C"  };
    _cpu->optable[0xA2] = (instruction){ .func = AND_A_D,   .name = "AND_A_D"  };
    _cpu->optable[0xA3] = (instruction){ .func = AND_A_E,   .name = "AND_A_E"  };
    _cpu->optable[0xA4] = (instruction){ .func = AND_A_H,   .name = "AND_A_H"  };
    _cpu->optable[0xA5] = (instruction){ .func = AND_A_L,   .name = "AND_A_L"  };
    _cpu->optable[0xA6] = (instruction){ .func = AND_A_HL,  .name = "AND_A_HL" };
    _cpu->optable[0xA7] = (instruction){ .func = AND_A_A,   .name = "AND_A_A"  };
    _cpu->optable[0xA8] = (instruction){ .func = XOR_A_B,   .name = "XOR_A_B"  };
    _cpu->optable[0xA9] = (instruction){ .func = XOR_A_C,   .name = "XOR_A_C"  };
    _cpu->optable[0xAA] = (instruction){ .func = XOR_A_D,   .name = "XOR_A_D"  };
    _cpu->optable[0xAB] = (instruction){ .func = XOR_A_E,   .name = "XOR_A_E"  };
    _cpu->optable[0xAC] = (instruction){ .func = XOR_A_H,   .name = "XOR_A_H"  };
    _cpu->optable[0xAD] = (instruction){ .func = XOR_A_L,   .name = "XOR_A_L"  };
    _cpu->optable[0xAE] = (instruction){ .func = XOR_A_HL,  .name = "XOR_A_HL" };
    _cpu->optable[0xAF] = (instruction){ .func = XOR_A_A,   .name = "XOR_A_A"  };

    _cpu->optable[0xB0] = (instruction){ .func = OR_A_B,    .name = "OR_A_B"   };
    _cpu->optable[0xB1] = (instruction){ .func = OR_A_C,    .name = "OR_A_C"   };
    _cpu->optable[0xB2] = (instruction){ .func = OR_A_D,    .name = "OR_A_D"   };
    _cpu->optable[0xB3] = (instruction){ .func = OR_A_E,    .name = "OR_A_E"   };
    _cpu->optable[0xB4] = (instruction){ .func = OR_A_H,    .name = "OR_A_H"   };
    _cpu->optable[0xB5] = (instruction){ .func = OR_A_L,    .name = "OR_A_L"   };
    _cpu->optable[0xB6] = (instruction){ .func = OR_A_HL,   .name = "OR_A_HL"  };
    _cpu->optable[0xB7] = (instruction){ .func = OR_A_A,    .name = "OR_A_A"   };
    _cpu->optable[0xB8] = (instruction){ .func = CP_A_B,    .name = "CP_A_B"   };
    _cpu->optable[0xB9] = (instruction){ .func = CP_A_C,    .name = "CP_A_C"   };
    _cpu->optable[0xBA] = (instruction){ .func = CP_A_D,    .name = "CP_A_D"   };
    _cpu->optable[0xBB] = (instruction){ .func = CP_A_E,    .name = "CP_A_E"   };
    _cpu->optable[0xBC] = (instruction){ .func = CP_A_H,    .name = "CP_A_H"   };
    _cpu->optable[0xBD] = (instruction){ .func = CP_A_L,    .name = "CP_A_L"   };
    _cpu->optable[0xBE] = (instruction){ .func = CP_A_HL,   .name = "CP_A_H"   };
    _cpu->optable[0xBF] = (instruction){ .func = CP_A_A,    .name = "CP_A_A"   };

    _cpu->optable[0xC0] = (instruction){ .func = RET_NZ,     .name = "RET_NZ"       };
    _cpu->optable[0xC1] = (instruction){ .func = POP_BC,     .name = "POP_BC"       };
    _cpu->optable[0xC2] = (instruction){ .func = JP_NZ_a16,  .name = "JP_NZ_a16"    };
    _cpu->optable[0xC3] = (instruction){ .func = JP_a16,     .name = "JP_a16"       };
    _cpu->optable[0xC4] = (instruction){ .func = CALL_NZ_a16,.name = "CALL_NZ_a16"  };
    _cpu->optable[0xC5] = (instruction){ .func = PUSH_BC,    .name = "PUSH_BC"      };
    _cpu->optable[0xC6] = (instruction){ .func = ADD_A_d8,   .name = "ADD_A_d8"     };
    _cpu->optable[0xC7] = (instruction){ .func = RST_00,     .name = "RST_00"       };
    _cpu->optable[0xC8] = (instruction){ .func = RET_Z,      .name = "RET_Z"        };
    _cpu->optable[0xC9] = (instruction){ .func = RET,        .name = "RET"          };
    _cpu->optable[0xCA] = (instruction){ .func = JP_Z_a16,   .name = "JP_Z_a16"     };
    _cpu->optable[0xCB] = (instruction){ .func = PREFIX,     .name = "PREFIX"       };
    _cpu->optable[0xCC] = (instruction){ .func = CALL_Z_a16, .name = "CALL_Z_a16"   };
    _cpu->optable[0xCD] = (instruction){ .func = CALL_a16,   .name = "CALL_a16"     };
    _cpu->optable[0xCE] = (instruction){ .func = ADC_A_d8,   .name = "ADC_A_d8"     };
    _cpu->optable[0xCF] = (instruction){ .func = RST_08,     .name = "RST_08"       };

    _cpu->optable[0xD0] = (instruction){ .func = RET_NC,     .name = "RET_NC"       };
    _cpu->optable[0xD1] = (instruction){ .func = POP_DE,     .name = "POP_DE"       };
    _cpu->optable[0xD2] = (instruction){ .func = JP_NC_a16,  .name = "JP_NC_a16"    };
    _cpu->optable[0xD3] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    _cpu->optable[0xD4] = (instruction){ .func = CALL_NC_a16,.name = "CALL_NC_a16"  };
    _cpu->optable[0xD5] = (instruction){ .func = PUSH_DE,    .name = "PUSH_DE"      };
    _cpu->optable[0xD6] = (instruction){ .func = SUB_A_d8,   .name = "SUB_A_d8"     };
    _cpu->optable[0xD7] = (instruction){ .func = RST_10,     .name = "RST_10"       };
    _cpu->optable[0xD8] = (instruction){ .func = RET_C,      .name = "RET_C"        };
    _cpu->optable[0xD9] = (instruction){ .func = RETI,       .name = "RETI"         };
    _cpu->optable[0xDA] = (instruction){ .func = JP_C_a16,   .name = "JP_C_a16"     };
    _cpu->optable[0xDB] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    _cpu->optable[0xDC] = (instruction){ .func = CALL_C_a16, .name = "CALL_C_a16"   };
    _cpu->optable[0xDD] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    _cpu->optable[0xDE] = (instruction){ .func = SBC_A_d8,   .name = "SBC_A_d8"     };
    _cpu->optable[0xDF] = (instruction){ .func = RST_18,     .name = "RST_18"       };

    _cpu->optable[0xE0] = (instruction){ .func = LDH_a8_A,   .name = "LDH_a8_A"     };
    _cpu->optable[0xE1] = (instruction){ .func = POP_HL,     .name = "POP_HL"       };
    _cpu->optable[0xE2] = (instruction){ .func = LDH_C_A,    .name = "LD_C_A"       };
    _cpu->optable[0xE3] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    _cpu->optable[0xE4] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    _cpu->optable[0xE5] = (instruction){ .func = PUSH_HL,    .name = "PUSH_HL"      };
    _cpu->optable[0xE6] = (instruction){ .func = AND_A_d8,   .name = "AND_A_d8"     };
    _cpu->optable[0xE7] = (instruction){ .func = RST_20,     .name = "RST_20"       };
    _cpu->optable[0xE8] = (instruction){ .func = ADD_SP_i8,  .name = "ADD_SP_i8"    };
    _cpu->optable[0xE9] = (instruction){ .func = JP_HL,      .name = "JP_HL"        };
    _cpu->optable[0xEA] = (instruction){ .func = LD_a16_A,   .name = "LD_a16_A"     };
    _cpu->optable[0xEB] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    _cpu->optable[0xEC] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    _cpu->optable[0xED] = (instruction){ .func = INVALID,    .name = "INVALID"      };
    _cpu->optable[0xEE] = (instruction){ .func = XOR_A_d8,   .name = "XOR_A_d8"     };
    _cpu->optable[0xEF] = (instruction){ .func = RST_28,     .name = "RST_28"       };

    _cpu->optable[0xF0] = (instruction){ .func = LDH_A_a8,          .name = "LDH_A_a8"          };
    _cpu->optable[0xF1] = (instruction){ .func = POP_AF,            .name = "POP_AF"            };
    _cpu->optable[0xF2] = (instruction){ .func = LDH_A_C,           .name = "LD_A_C"            };
    _cpu->optable[0xF3] = (instruction){ .func = DI,                .name = "DI"                };
    _cpu->optable[0xF4] = (instruction){ .func = INVALID,           .name = "INVALID"           };
    _cpu->optable[0xF5] = (instruction){ .func = PUSH_AF,           .name = "PUSH_AF"           };
    _cpu->optable[0xF6] = (instruction){ .func = OR_A_d8,           .name = "OR_A_d8"           };
    _cpu->optable[0xF7] = (instruction){ .func = RST_30,            .name = "RST_30"            };
    _cpu->optable[0xF8] = (instruction){ .func = LD_HL_SP_PLUS_i8,  .name = "LD_HL_SP_PLUS_i8"  };
    _cpu->optable[0xF9] = (instruction){ .func = LD_SP_HL,          .name = "LD_SP_HL"          };
    _cpu->optable[0xFA] = (instruction){ .func = LD_A_a16,          .name = "LD_A_a16"          };
    _cpu->optable[0xFB] = (instruction){ .func = EI,                .name = "EI"                };
    _cpu->optable[0xFC] = (instruction){ .func = INVALID,           .name = "INVALID"           };
    _cpu->optable[0xFD] = (instruction){ .func = INVALID,           .name = "INVALID"           };
    _cpu->optable[0xFE] = (instruction){ .func = CP_A_d8,           .name = "CP_A_d8"           };
    _cpu->optable[0xFF] = (instruction){ .func = RST_38,            .name = "RST_38"            };

    return _cpu;
}

void cpu_unload(cpu* _cpu) {
    if (_cpu) {
        if (_cpu->memory) {
            free(_cpu->memory);
        }
        if (_cpu->optable) {
            free(_cpu->optable);
        }
        free(_cpu);
    }
}

static void check_interrupts(cpu* c, u16 addr, u16 int_type) {
    if (c->IF & int_type && c->IE & int_type) {
        // Push PC onto stack
        c->sp -= 2;
        bus_write(c->_bus, c->pc & 0xFF, c->sp);
        bus_write(c->_bus, (c->pc >> 8) & 0xFF, c->sp + 1);

        // Jump to interrupt vector
        c->pc = addr;

        c->IF &= ~int_type;
        c->halted = false;
        c->ints_enabled = false;

        // return true;
    }

    // return false;
}

void cpu_handle_interrupts(cpu* c) {
    check_interrupts(c, 0x40, 1); // VBLANK
    check_interrupts(c, 0x48, 2); // LCD_STAT
    check_interrupts(c, 0x50, 4); // TIMER
    check_interrupts(c, 0x58, 8); // SERIAL
    check_interrupts(c, 0x60, 16); // JOYPAD

    // Interrupt handling takes 20 cycles
    c->cycles += 20;
}

void cpu_print_dbg_info(struct bus* b, cpu* c, const instruction* cur_instr) {
    char flags[16];
    sprintf(flags, "%c%c%c%c",
        c->f & CPU_FLAGS_Z ? 'Z' : '-',
        c->f & CPU_FLAGS_N ? 'N' : '-',
        c->f & CPU_FLAGS_H ? 'H' : '-',
        c->f & CPU_FLAGS_C ? 'C' : '-'
    );

    printf("%04X: %-12s (%02X %02X %02X) A: %02X F: %s BC: %02X%02X DE: %02X%02X HL: %02X%02X\n",
        c->pc, cur_instr->name, bus_read(b, b->c->pc),
        bus_read(b, c->pc + 1), bus_read(b, c->pc + 2), c->a, flags, c->b, c->c,
        c->d, c->e, c->h, c->l);
}