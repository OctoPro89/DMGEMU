const CPU_FLAGS_Z = (1 << 7);
const CPU_FLAGS_N = (1 << 6);
const CPU_FLAGS_H = (1 << 5);
const CPU_FLAGS_C = (1 << 4);

let static_i8 = new Int8Array(1);
let static_u8 = new Uint8Array(1);
let static_u16 = new Uint16Array(1);

function LOBYTE(w) { return (u8(w & 0xFF)) }
function HIBYTE(w) { return (u8((w >> 8) & 0xFF)) }
function GETBIT(x, n) { return u8(x | (1 << n)) }
function SETBIT(x, n) { return u8(x |= (1 << n)) }
function CLEARBIT(x, n) { u8(x &= ~(1 << n)) }
function U16MSBLSB8(msb, lsb) { return u16((msb << 8) | lsb); }

function i8(x) { static_i8[0] = x; return static_i8[0]; }
function u8(x) { static_u8[0] = x; return static_u8[0]; }
function u16(x) { static_u16[0] = x; return static_u16[0]; }

// Helpers
function AF(cpu) { return (((cpu).a << 8) | (cpu).f) }
function BC(cpu) { return (((cpu).b << 8) | (cpu).c) }
function DE(cpu) { return (((cpu).d << 8) | (cpu).e) }
function HL(cpu) { return (((cpu).h << 8) | (cpu).l) }

function SET_AF(cpu, val) { ((cpu).a = (val) >> 8, (cpu).f = (val) & 0xF0) }
function SET_BC(cpu, val) { ((cpu).b = (val) >> 8, (cpu).c = val & 0xFF) }
function SET_DE(cpu, val) { ((cpu).d = (val) >> 8, (cpu).e = val & 0xFF) }
function SET_HL(cpu, val) { ((cpu).h = (val) >> 8, (cpu).l = val & 0xFF) }

// -- INSTRUCTIONS --

function NOP(c) {
    // Do nothing
    return 4;
}

function LD_BC_d16(c) {
    SET_BC(c, c.fetch_d16());
    return 12;
}

function LD_BC_A(c) {
    c.memory[BC(c)] = c.a;
    return 8;
}

function INC_BC(c) {
    SET_BC(c, BC(c) + 1);
    return 8;
}

function INC_B(c) {
    const result = u8(c.b + 1);

    // Zero flag
    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.clear_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((c.b & 0x0F) + 1 > 0x0F) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.b = result;

    return 4;
}

function DEC_B(c) {
    const result = u8(c.b - 1);

    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.set_flag(CPU_FLAGS_N);

    // Half-carry borrow from bit 4
    if ((c.b & 0x0F) == 0x00) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.b = result;
    return 4;
}

function LD_B_d8(c) {
    c.b = c.fetch();
    return 8;
}

function RLCA(c) {
    const old = u8(c.a);
    c.a = (old << 1) | (old >> 7); // rotate
    c.f = 0; // clear Z, N, H
    if (old & 0x80) c.set_flag(CPU_FLAGS_C);
    return 4;
}

function LD_a16_SP(c) {
    const a16 = c.fetch_d16();
    c.memory[a16] = LOBYTE(c.sp);
    c.memory[a16 + 1] = HIBYTE(c.sp);

    return 20;
}

function ADD_HL_BC(c) {
    // u32
    const result = HL(c) + BC(c);
    c.clear_flag(CPU_FLAGS_N);

    if (((HL(c) & 0x0FFF) + (BC(c) & 0x0FFF)) > 0x0FFF)
        c.set_flag(CPU_FLAGS_H);
    else
        c.clear_flag(CPU_FLAGS_H);

    if (result > 0xFFFF)
        c.set_flag(CPU_FLAGS_C);
    else
        c.clear_flag(CPU_FLAGS_C);

    SET_HL(c, u16(result));
    return 8;
}

function LD_A_BC(c) {
    c.a = c.memory[BC(c)];
    return 8;
}

function DEC_BC(c) {
    SET_BC(c, BC(c) - 1);
    return 8;
}

function INC_C(c) {
    const result = u8(c.c + 1);

    // Zero flag
    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.clear_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((c.c & 0x0F) + 1 > 0x0F) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.c = result;

    return 4;
}

function DEC_C(c) {
    const result = u8(c.c - 1);

    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.set_flag(CPU_FLAGS_N);

    if ((c.c & 0x0F) == 0x00) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.c = result;
    return 4;
}

function LD_C_d8(c) {
    c.c = c.fetch();
    return 8;
}

function RRCA(c) {
    const old = u8(c.a);
    c.a = (old >> 1) | (old << 7); // rotate right
    c.f = 0;
    if (old & 0x01) c.set_flag(CPU_FLAGS_C);
    return 4;
}

// 0x00-0x0F

// 0x10-0x1F

function STOP(c) {
    // Not sure how to implement yet...
    return 4;
}

function LD_DE_d16(c) {
    SET_DE(c, c.fetch_d16());
    return 12;
}

function LD_DE_A(c) {
    c.memory[DE(c)] = c.a;
    return 8;
}

function INC_DE(c) {
    SET_DE(c, DE(c) + 1);
    return 8;
}

function INC_D(c) {
    const result = u8(c.d + 1);

    // Zero flag
    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.clear_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((c.d & 0x0F) + 1 > 0x0F) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.d = result;

    return 4;
}

function DEC_D(c) {
    const result = u8(c.d - 1);

    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.set_flag(CPU_FLAGS_N);

    // Half-carry borrow from bit 4
    if ((c.d & 0x0F) == 0x00) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.d = result;
    return 4;
}

function LD_D_d8(c) {
    c.d = c.fetch();
    return 8;
}

function RLA(c) {
    const carry = u8(c.get_flag(CPU_FLAGS_C) ? 1 : 0);
    const old = u8(c.a);

    c.a = (old << 1) | carry;

    c.f = 0; // clear Z, N, H
    if (old & 0x80) c.set_flag(CPU_FLAGS_C);

    return 4;
}

function JR_s8(c) {
    const jmp = c.fetch();
    c.pc += jmp;
    return 12;
}

function ADD_HL_DE(c) {
    const result = HL(c) + DE(c);
    c.clear_flag(CPU_FLAGS_N);

    if (((HL(c) & 0x0FFF) + (DE(c) & 0x0FFF)) > 0x0FFF)
        c.set_flag(CPU_FLAGS_H);
    else
        c.clear_flag(CPU_FLAGS_H);

    if (result > 0xFFFF)
        c.set_flag(CPU_FLAGS_C);
    else
        c.clear_flag(CPU_FLAGS_C);

    SET_HL(c, u16(result));
    return 8;
}

function LD_A_DE(c) {
    c.a = c.memory[DE(c)];
    return 8;
}

function DEC_DE(c) {
    SET_DE(c, DE(c) - 1);
    return 8;
}

function INC_E(c) {
    const result = u8(c.e + 1);

    // Zero flag
    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.clear_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((c.e & 0x0F) + 1 > 0x0F) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.e = result;

    return 4;
}

function DEC_E(c) {
    const result = u8(c.e - 1);

    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.set_flag(CPU_FLAGS_N);

    if ((c.e & 0x0F) == 0x00) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.e = result;
    return 4;
}

function LD_E_d8(c) {
    c.e = c.fetch();
    return 8;
}

function RRA(c) {
    const carry = u8(c.get_flag(CPU_FLAGS_C) ? 0x80 : 0);
    const old = u8(c.a);

    c.a = (old >> 1) | carry;

    c.f = 0;
    if (old & 0x01) c.set_flag(CPU_FLAGS_C);

    return 4;
}

// 0x10-0x1F

// 0x20-0x2F

function JR_NZ_s8(c) {
    const offset = c.fetch();  // signed 8-bit
    if (!c.get_flag(CPU_FLAGS_Z)) {
        c.pc += offset;
        return 12; // jump taken
    }
    return 8; // jump not taken
}

function LD_HL_d16(c) {
    SET_HL(c, c.fetch_d16());
    return 12;
}

function LD_HLI_A(c) {
    c.memory[HL(c)] = c.a;
    SET_HL(c, HL(c) + 1);
    return 8;
}

function INC_HL(c) {
    SET_HL(c, HL(c) + 1);
    return 8;
}

function INC_H(c) {
    const result = u8(c.h + 1);

    // Zero flag
    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.clear_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((c.h & 0x0F) + 1 > 0x0F) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.h = result;

    return 4;
}

function DEC_H(c) {
    const result = u8(c.h - 1);

    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.set_flag(CPU_FLAGS_N);

    // Half-carry borrow from bit 4
    if ((c.h & 0x0F) == 0x00) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.h = result;
    return 4;
}

function LD_H_d8(c) {
    c.h = c.fetch();
    return 8;
}

function DAA(c) {
    // TODO
    return 4;
}

function JR_Z_s8(c) {
    const offset = c.fetch();  // signed 8-bit
    if (c.get_flag(CPU_FLAGS_Z)) {
        c.pc += offset;
        return 12; // jump taken
    }
    return 8; // jump not taken
}

function ADD_HL_HL(c) {
    // u32
    const result = HL(c) + HL(c);
    c.clear_flag(CPU_FLAGS_N);

    if (((HL(c) & 0x0FFF) + (HL(c) & 0x0FFF)) > 0x0FFF)
        c.set_flag(CPU_FLAGS_H);
    else
        c.clear_flag(CPU_FLAGS_H);

    if (result > 0xFFFF)
        c.set_flag(CPU_FLAGS_C);
    else
        c.clear_flag(CPU_FLAGS_C);

    SET_HL(c, u16(result));
    return 8;
}

function LD_A_HLI(c) {
    c.a = c.memory[HL(c)];
    SET_HL(c, HL(c) + 1);
    return 8;
}

function DEC_HL(c) {
    SET_HL(c, HL(c) - 1);
    return 8;
}

function INC_L(c) {
    const result = u8(c.l + 1);

    // Zero flag
    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.clear_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((c.l & 0x0F) + 1 > 0x0F) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.l = result;

    return 4;
}

function DEC_L(c) {
    const result = u8(c.l - 1);

    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.set_flag(CPU_FLAGS_N);

    if ((c.l & 0x0F) == 0x00) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.l = result;
    return 4;
}

function LD_L_d8(c) {
    c.l = c.fetch();
    return 8;
}

function CPL(c) {
    c.a = ~(c.a);
    c.set_flag(CPU_FLAGS_N);
    c.set_flag(CPU_FLAGS_H);
    return 4;
}

// 0x20-0x2F

// 0x30-0x3F

function JR_NC_s8(c) {
    const offset = c.fetch();  // signed 8-bit
    if (!c.get_flag(CPU_FLAGS_C)) {
        c.pc += offset;
        return 12; // jump taken
    }
    return 8; // jump not taken
}

function LD_SP_d16(c) {
    c.sp = c.fetch_d16();
    return 12;
}

function LD_HLD_A(c) {
    c.memory[HL(c)] = c.a;
    SET_HL(c, HL(c) - 1);
    return 8;
}

function INC_SP(c) {
    c.sp = c.sp + 1;
    return 8;
}

function INC_IND_HL(c) {
    const result = u8(c.memory[HL(c)] + 1);

    // Zero flag
    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.clear_flag(CPU_FLAGS_N);

    if ((c.memory[HL(c)] & 0x0FFF) + 1 > 0x0FFF) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.memory[HL(c)] = result;

    return 12;
}

function DEC_IND_HL(c) {
    const result = u8(c.memory[HL(c)] - 1);

    // Zero flag
    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.clear_flag(CPU_FLAGS_N);

    if ((c.memory[HL(c)] & 0x0FFF) == 0x00) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.memory[HL(c)] = result;

    return 12;
}

function LD_HL_d8(c) {
    c.memory[HL(c)] = c.fetch();
    return 12;
}

function SCF(c) {
    c.clear_flag(CPU_FLAGS_N);
    c.clear_flag(CPU_FLAGS_H);
    c.set_flag(CPU_FLAGS_C);
    return 4;
}

function JR_C_s8(c) {
    const offset = c.fetch();  // signed 8-bit
    if (c.get_flag(CPU_FLAGS_C)) {
        c.pc += offset;
        return 12; // jump taken
    }
    return 8; // jump not taken
}

function ADD_HL_SP(c) {
    const result = HL(c) + c.sp;
    c.clear_flag(CPU_FLAGS_N);

    if (((HL(c) & 0x0FFF) + (c.sp & 0x0FFF)) > 0x0FFF)
        c.set_flag(CPU_FLAGS_H);
    else
        c.clear_flag(CPU_FLAGS_H);

    if (result > 0xFFFF)
        c.set_flag(CPU_FLAGS_C);
    else
        c.clear_flag(CPU_FLAGS_C);

    SET_HL(c, u16(result));
    return 8;
}

function LD_A_HLD(c) {
    c.a = c.memory[HL(c)];
    SET_HL(c, HL(c) - 1);
    return 8;
}

function DEC_A(c) {
    const result = u8(c.a - 1);

    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.set_flag(CPU_FLAGS_N);

    if ((c.a & 0x0F) == 0x00) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.a = result;
    return 4;
}

function LD_A_d8(c) {
    c.a = c.memory[c.pc++];
    return 8;
}

function CCF(c) {
    c.clear_flag(CPU_FLAGS_N);
    c.clear_flag(CPU_FLAGS_H);
    if (c.get_flag(CPU_FLAGS_C)) c.clear_flag(CPU_FLAGS_C);
    else c.set_flag(CPU_FLAGS_C);
    return 4;
}

// 0x30-0x3F

// 0x40-0x4F

function LD_B_B(c) {
    c.b = c.b;
    return 4;
}

function LD_B_C(c) {
    c.b = c.c;
    return 4;
}

function LD_B_D(c) {
    c.b = c.d;
    return 4;
}

function LD_B_E(c) {
    c.b = c.e;
    return 4;
}

function LD_B_H(c) {
    c.b = c.h;
    return 4;
}

function LD_B_L(c) {
    c.b = c.l;
    return 4;
}

function LD_B_HL(c) {
    c.b = c.memory[HL(c)];
    return 8;
}

function LD_B_A(c) {
    c.b = c.a;
    return 4;
}

function LD_C_B(c) {
    c.c = c.b;
    return 4;
}

function LD_C_C(c) {
    c.c = c.c;
    return 4;
}

function LD_C_D(c) {
    c.c = c.d;
    return 4;
}

function LD_C_E(c) {
    c.c = c.e;
    return 4;
}

function LD_C_H(c) {
    c.c = c.h;
    return 4;
}

function LD_C_L(c) {
    c.c = c.l;
    return 4;
}

function LD_C_HL(c) {
    c.c = c.memory[HL(c)];
    return 8;
}

function LD_C_A(c) {
    c.c = c.a;
    return 4;
}

// 0x40-0x4F

function LD_D_B(c) {
    c.d = c.b;
    return 4;
}

function LD_D_C(c) {
    c.d = c.c;
    return 4;
}

function LD_D_D(c) {
    c.d = c.d;
    return 4;
}

function LD_D_E(c) {
    c.d = c.e;
    return 4;
}

function LD_D_H(c) {
    c.d = c.h;
    return 4;
}

function LD_D_L(c) {
    c.d = c.l;
    return 4;
}

function LD_D_HL(c) {
    c.d = c.memory[HL(c)];
    return 8;
}

function LD_D_A(c) {
    c.d = c.a;
    return 4;
}

function LD_E_B(c) {
    c.e = c.b;
    return 4;
}

function LD_E_C(c) {
    c.e = c.c;
    return 4;
}

function LD_E_D(c) {
    c.e = c.d;
    return 4;
}

function LD_E_E(c) {
    c.e = c.e;
    return 4;
}

function LD_E_H(c) {
    c.e = c.h;
    return 4;
}

function LD_E_L(c) {
    c.e = c.l;
    return 4;
}

function LD_E_HL(c) {
    c.e = c.memory[HL(c)];
    return 8;
}

function LD_E_A(c) {
    c.e = c.a;
    return 4;
}

// 0x50-0x5F

// 0x50-0x5F

function CP_d8(c) {
    const value = c.fetch();
    const a = u8(c.a);
    const result = u8(a - value);

    // Zero flag
    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    // Subtract flag
    c.set_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((a & 0x0F) < (value & 0x0F)) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    if (a < value) c.set_flag(CPU_FLAGS_C);
    else c.clear_flag(CPU_FLAGS_C);

    return 8; // 8 T-cycles
}

function XOR_A(c) {
    c.a = 0;
    c.f = CPU_FLAGS_Z; // only Z is set
    return 4;
}

function JP_A_a16(c) {
    c.pc = c.fetch_d16();
    return 16;
}

function LDH_a8_A(c) {
    c.memory[U16MSBLSB8(0xFF, c.fetch())] = c.a;
    return 12;
}

function LD_A_C(c) {
    c.a = c.c;
    return 4;
}

function DEC_SP(c) {
    c.sp = c.sp - 1;
    return 8;
}

function INC_A(c) {
    const result = u8(c.a + 1);

    // Zero flag
    if (result == 0) c.set_flag(CPU_FLAGS_Z);
    else c.clear_flag(CPU_FLAGS_Z);

    c.clear_flag(CPU_FLAGS_N);

    // Half-carry flag (borrow from bit 4)
    if ((c.a & 0x0F) + 1 > 0x0F) c.set_flag(CPU_FLAGS_H);
    else c.clear_flag(CPU_FLAGS_H);

    c.a = result;

    return 4;
}

// -- INSTRUCTIONS --

class instruction {
    constructor(func, name) {
        this.func = func;
        this.name = name;
    }
}

class CPU {
    constructor(cart) {
        this.memory = cart;

        this._a = new Uint8Array(1);
        this._f = new Uint8Array(1);
        this._b = new Uint8Array(1);
        this._c = new Uint8Array(1);
        this._d = new Uint8Array(1);
        this._e = new Uint8Array(1);
        this._h = new Uint8Array(1);
        this._l = new Uint8Array(1);

        this._pc = new Uint16Array(1);
        this._sp = new Uint16Array(1);

        this.pc = 0x100;
        this.sp = 0xFFFE;
        this.a = 0x01;
        this.ints_enabled = true;
        this.cycles = 0;

        this.optable = new Array(512);
        this.optable[0x00] = new instruction(NOP,       "NOP"       );
        this.optable[0x01] = new instruction(LD_BC_d16, "LD_BC_d16" );
        this.optable[0x02] = new instruction(LD_BC_A,   "LD_BC_A"   );
        this.optable[0x03] = new instruction(INC_BC,    "INC_BC"    );
        this.optable[0x04] = new instruction(INC_B,     "INC_B"     );
        this.optable[0x05] = new instruction(DEC_B,     "DEC_B"     );
        this.optable[0x06] = new instruction(LD_B_d8,   "LD_B_d8"   );
        this.optable[0x07] = new instruction(RLCA,      "RLCA"      );
        this.optable[0x08] = new instruction(LD_a16_SP, "LD_a16_SP" );
        this.optable[0x09] = new instruction(ADD_HL_BC, "ADD_HL_BC" );
        this.optable[0x0A] = new instruction(LD_A_BC,   "LD_A_BC"   );
        this.optable[0x0B] = new instruction(DEC_BC,    "DEC_BC"    );
        this.optable[0x0C] = new instruction(INC_C,     "INC_C"     );
        this.optable[0x0D] = new instruction(DEC_C,     "DEC_C"     );
        this.optable[0x0E] = new instruction(LD_C_d8,   "LD_C_d8"   );
        this.optable[0x0F] = new instruction(RRCA,      "RRCA"      );

        this.optable[0x10] = new instruction(STOP,      "STOP"      );
        this.optable[0x11] = new instruction(LD_DE_d16, "LD_DE_d16" );
        this.optable[0x12] = new instruction(LD_DE_A,   "LD_DE_A"   );
        this.optable[0x13] = new instruction(INC_DE,    "INC_DE"    );
        this.optable[0x14] = new instruction(INC_D,     "INC_D"     );
        this.optable[0x15] = new instruction(DEC_D,     "DEC_D"     );
        this.optable[0x16] = new instruction(LD_D_d8,   "LD_D_d8"   );
        this.optable[0x17] = new instruction(RLA,       "RLA"       );
        this.optable[0x18] = new instruction(JR_s8,     "JR_s8"     );
        this.optable[0x19] = new instruction(ADD_HL_DE, "ADD_HL_DE" );
        this.optable[0x1A] = new instruction(LD_A_DE,   "LD_A_DE"   );
        this.optable[0x1B] = new instruction(DEC_DE,    "DEC_DE"    );
        this.optable[0x1C] = new instruction(INC_E,     "INC_E"     );
        this.optable[0x1D] = new instruction(DEC_E,     "DEC_E"     );
        this.optable[0x1E] = new instruction(LD_E_d8,   "LD_E_d8"   );
        this.optable[0x1F] = new instruction(RRA,       "RRA"       );

        this.optable[0x20] = new instruction(JR_NZ_s8,  "JR_NZ_s8"  );
        this.optable[0x21] = new instruction(LD_HL_d16, "LD_HL_d16" );
        this.optable[0x22] = new instruction(LD_HLI_A,  "LD_HLI_A"  );
        this.optable[0x23] = new instruction(INC_HL,    "INC_HL"    );
        this.optable[0x24] = new instruction(INC_H,     "INC_H"     );
        this.optable[0x25] = new instruction(DEC_H,     "DEC_H"     );
        this.optable[0x26] = new instruction(LD_H_d8,   "LD_H_d8"   );
        this.optable[0x27] = new instruction(DAA,       "DAA"       );
        this.optable[0x28] = new instruction(JR_Z_s8,   "JR_Z_s8"   );
        this.optable[0x29] = new instruction(ADD_HL_HL, "ADD_HL_HL" );
        this.optable[0x2A] = new instruction(LD_A_HLI,  "LD_A_HLI"  );
        this.optable[0x2B] = new instruction(DEC_HL,    "DEC_HL"    );
        this.optable[0x2C] = new instruction(INC_L,     "INC_L"     );
        this.optable[0x2D] = new instruction(DEC_L,     "DEC_L"     );
        this.optable[0x2E] = new instruction(LD_L_d8,   "LD_L_d8"   );
        this.optable[0x2F] = new instruction(CPL,       "CPL"       );

        this.optable[0x30] = new instruction(JR_NC_s8,  "JR_NC_s8"  );
        this.optable[0x31] = new instruction(LD_SP_d16, "LD_SP_d16" );
        this.optable[0x32] = new instruction(LD_HLD_A,  "LD_HLD_A"  );
        this.optable[0x33] = new instruction(INC_SP,    "INC_SP"    );
        this.optable[0x34] = new instruction(INC_IND_HL,"INC_IND_HL");
        this.optable[0x35] = new instruction(INC_IND_HL,"INC_IND_HL");
        this.optable[0x36] = new instruction(LD_HL_d8,   "LD_HL_d8" );
        this.optable[0x37] = new instruction(SCF,       "SCF"       );
        this.optable[0x38] = new instruction(JR_C_s8,   "JR_C_s8"   );
        this.optable[0x39] = new instruction(ADD_HL_SP, "ADD_HL_SP" );
        this.optable[0x3A] = new instruction(LD_A_HLD,  "LD_A_HLD"  );
        this.optable[0x3B] = new instruction(DEC_SP,    "DEC_SP"    );
        this.optable[0x3C] = new instruction(INC_A,     "INC_A"     );
        this.optable[0x3D] = new instruction(DEC_A,     "DEC_A"     );
        this.optable[0x3E] = new instruction(LD_A_d8,   "LD_A_d8"   );
        this.optable[0x3F] = new instruction(CCF,       "CCF"       );

        this.optable[0x40] = new instruction(LD_B_B,  "JR_NC_s8"    );
        this.optable[0x41] = new instruction(LD_B_C, "LD_SP_d16"    );
        this.optable[0x42] = new instruction(LD_B_D,  "LD_HLD_A"    );
        this.optable[0x43] = new instruction(LD_B_E,    "INC_SP"    );
        this.optable[0x44] = new instruction(LD_B_H,"INC_IND_HL"    );
        this.optable[0x45] = new instruction(LD_B_L,"INC_IND_HL"    );
        this.optable[0x46] = new instruction(LD_B_HL,   "LD_HL_d8"  );
        this.optable[0x47] = new instruction(LD_B_A,       "SCF"    );
        this.optable[0x48] = new instruction(LD_C_B,   "JR_C_s8"    );
        this.optable[0x49] = new instruction(LD_C_C, "ADD_HL_SP"    );
        this.optable[0x4A] = new instruction(LD_C_D,  "LD_A_HLD"    );
        this.optable[0x4B] = new instruction(LD_C_E,    "DEC_SP"    );
        this.optable[0x4C] = new instruction(LD_C_H,     "INC_A"    );
        this.optable[0x4D] = new instruction(LD_C_L,     "DEC_A"    );
        this.optable[0x4E] = new instruction(LD_C_HL,   "LD_A_d8"   );
        this.optable[0x4F] = new instruction(LD_C_A,       "CCF"    );

        this.optable[0x50] = new instruction(LD_D_B,  "JR_NC_s8"    );
        this.optable[0x51] = new instruction(LD_D_C, "LD_SP_d16"    );
        this.optable[0x52] = new instruction(LD_D_D,  "LD_HLD_A"    );
        this.optable[0x53] = new instruction(LD_D_E,    "INC_SP"    );
        this.optable[0x54] = new instruction(LD_D_H,"INC_IND_HL"    );
        this.optable[0x55] = new instruction(LD_D_L,"INC_IND_HL"    );
        this.optable[0x56] = new instruction(LD_D_HL,   "LD_HL_d8"  );
        this.optable[0x57] = new instruction(LD_D_A,       "SCF"    );
        this.optable[0x58] = new instruction(LD_E_B,   "JR_C_s8"    );
        this.optable[0x59] = new instruction(LD_E_C, "ADD_HL_SP"    );
        this.optable[0x5A] = new instruction(LD_E_D,  "LD_A_HLD"    );
        this.optable[0x5B] = new instruction(LD_E_E,    "DEC_SP"    );
        this.optable[0x5C] = new instruction(LD_E_H,     "INC_A"    );
        this.optable[0x5D] = new instruction(LD_E_L,     "DEC_A"    );
        this.optable[0x5E] = new instruction(LD_E_HL,   "LD_A_d8"   );
        this.optable[0x5F] = new instruction(LD_E_A,       "CCF"    );

        this.optable[0x3E] = new instruction(LD_A_d8,   "LD_A_d8"   );
        this.optable[0xC3] = new instruction(JP_A_a16,  "JP_A_a16"  );
        this.optable[0xFE] = new instruction(CP_d8,     "CP_d8"     );
        this.optable[0xAF] = new instruction(XOR_A,     "XOR_A"     );
        this.optable[0xE0] = new instruction(LDH_a8_A,  "LDH_a8_A"  );
        this.optable[0xF3] = new instruction(LD_A_C,    "LD_A_C"    );
    }

    fetch_d16() {
        const lo = this.memory[this.pc++];
        const hi = this.memory[this.pc++];
        return (hi << 8) | lo;
    }

    fetch() {
        return this.memory[this.pc++];
    }

    set_flag(f) { this.f |= f; }
    clear_flag(f) { this.f &= ~f; }
    get_flag(f) { return this.f & f; }

    cycle() {
        const opcode = this.fetch();
        log(`OPCODE: 0x${opcode.toString(16)}`);
        const instr = this.optable[opcode];

        if (instr.func == null) {
            log(`Unknown opcode 0x${opcode.toString(16)}, PC = 0x${this.pc.toString(16) - 1}`);
            throw new Error();
        }
        // Execute
        const cycles = instr.func(this);

        this.cycles += cycles;

        log(`Executed instruction: ${instr.name}`);
        log(`Z=${this.get_flag(CPU_FLAGS_Z)}, N=${this.get_flag(CPU_FLAGS_N)}, H=${this.get_flag(CPU_FLAGS_H)}, C=${this.get_flag(CPU_FLAGS_C)}`);
        log(`A: ${this.a}, B: ${this.b}, C: ${this.c} D: ${this.d}, E: ${this.e}, F: ${this.f}, H: ${this.h}, L: ${this.l}`);
    }
    
    // 8 and 16 bit getters and setters

    get a() {
        return this._a[0];
    }

    set a(val) {
        this._a[0] = val;
    }

    get f() {
        return this._f[0];
    }

    set f(val) {
        this._f[0] = val;
    }

    get b() {
        return this._b[0];
    }

    set b(val) {
        this._b[0] = val;
    }

    get c() {
        return this._c[0];
    }

    set c(val) {
        this._c[0] = val;
    }

    get d() {
        return this._d[0];
    }

    set d(val) {
        this._d[0] = val;
    }

    get e() {
        return this._e[0];
    }

    set e(val) {
        this._e[0] = val;
    }

    get h() {
        return this._h[0];
    }

    set h(val) {
        this._h[0] = val;
    }

    get l() {
        return this._l[0];
    }

    set l(val) {
        this._l[0] = val;
    }

    get pc() {
        return this._pc[0];
    }

    set pc(val) {
        this._pc[0] = val;
    }

    get sp() {
        return this._sp[0];
    }

    set sp(val) {
        this._sp[0] = val;
    }
}