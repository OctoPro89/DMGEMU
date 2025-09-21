#include "bus.h"
#include "ram.h"
#include "iomem.h"
#include <stdio.h>
#include <stdlib.h>

bus bus_init(cpu* c, ppu* p, timer* t) {
	bus b;
	b.c = c;
    b.c->_bus = &b;
    b.p = p;
    b.t = t;
    b.hram = malloc(0x80);
    b.wram = malloc(0x2000);
	return b;
}

void bus_unload(bus* b) {
    if (b) {
        if (b->hram) { free(b->hram); }
        if (b->wram) { free(b->wram); }
    }
}

u8 bus_read(bus* b, u16 addr) {
    if (addr <= 0x7FFF) {
        // Fixed bank 0
        return b->c->cart[addr];
    }
    else if (addr <= 0x9FFF) {
        //printf("UNSUPPORTED bus_read()!\n");
        //exit(1);
        // VRAM
        return 0x0; // b->c->memory[addr];
    }
    else if (addr <= 0xBFFF) {
        // Cartridge RAM (stubbed)
        return b->c->memory[addr];
    }
    else if (addr <= 0xDFFF) {
        // Work RAM
        return wram_read(b, addr);
    }
    else if (addr <= 0xFDFF) {
        return 0;
    }
    else if (addr <= 0xFE9F) {
        // OAM
        //printf("UNSUPPORTED bus_read()!\n");
        //exit(1);
        return 0x0; // b->c->memory[addr];
    }
    else if (addr <= 0xFEFF) {
        // Unusable area
        //printf("UNSUPPORTED bus_read()!\n");
        //exit(1);
        return 0x0;
    }
    else if (addr <= 0xFF7F) {
        // I/O Registers
        return io_read(b, addr);
    }
    else if (addr == 0xFFFF) {
        // Interrupt Enable
        return b->c->IE;
    }

    return hram_read(b, addr); // default
}

void bus_write(bus* b, u8 value, u16 addr) {
    if (addr <= 0x1FFF) {
        // RAM enable (ignore for now)
        return;
    }
    else if (addr <= 0x3FFF) {
        // ROM bank select
        u8 bank = value & 0x1F;
        if (bank == 0) bank = 1;
        b->c->rom_bank = bank;
        return;
    }
    else if (addr <= 0x7FFF) {
        // Switchable ROM bank (MBC, stubbed)
        // printf("UNSUPPORTED bus_write() to address %u!\n", addr);
        // exit(1);
        b->c->memory[addr] = value;
        return;
    }
    else if (addr <= 0x9FFF) {
        // VRAM
        //printf("UNSUPPORTED bus_write() to address %u!\n", addr);
        //exit(1);
        return;
    }
    else if (addr <= 0xBFFF) {
        // Cartridge RAM (stubbed)
        b->c->memory[addr] = value;
        return;
    }
    else if (addr <= 0xDFFF) {
        // Work RAM
        wram_write(b, addr, value);
        return;
    }
    else if (addr <= 0xFDFF) {
        // Echo RAM
        return;
    }
    else if (addr <= 0xFE9F) {
        // OAM
        //printf("UNSUPPORTED bus_write() to address %u!\n", addr);
        // exit(1);
        // b->c->memory[addr] = value;
        return;
    }
    else if (addr <= 0xFEFF) {
        // Unusable
        //printf("UNSUPPORTED bus_write() to address %u!\n", addr);
        // exit(1);
        return;
    }
    else if (addr <= 0xFF7F) {
        // I/O Registers
        io_write(b, addr, value);
    }
    else if (addr == 0xFFFF) {
        // Interrupt Enable
        b->c->IE = value;
        return;
    }
    else {
        hram_write(b, addr, value);
    }
}

static char dbg_msg[1024];
static int msg_size = 0;

#include <string.h>

static void dbg_info(bus* b) {
    if (bus_read(b, 0xFF02) == 0x81) {
        char c = bus_read(b, 0xFF01);
        bus_write(b, 0, 0xFF02);
        dbg_msg[msg_size++] = c;
    }

    dbg_msg[1023] = '\0';

    if (dbg_msg[0]) {
        printf("SERIAL: %s\n", dbg_msg);
    }
}

void bus_step(bus* b) {
    // Fetch
    u8 opcode = bus_read(b, b->c->pc);
    const instruction* instr = &b->c->optable[opcode];
    cpu_print_dbg_info(b, b->c, instr);
    b->c->pc++;

    if (instr->func == NULL) {
        printf("Unknown opcode 0x%02X at PC=0x%04X\n", opcode, b->c->pc - 1);
        exit(1);
    }

    // Execute
    u8 cycles = instr->func(b->c);

    b->c->cycles += cycles;

    for (u8 i = 0; i < cycles; ++i) {
        timer_tick(b->c, b->t);
    }
    
    if (b->c->ints_enabled) {
        cpu_handle_interrupts(b->c);
        b->c->int_next = false;
    }

    if (b->c->int_next) {
        b->c->ints_enabled = true;
    }

    ppu_step(b->p);
    dbg_info(b);
}