#include "bus.h"
#include "ram.h"
#include "iomem.h"
#include "dma.h"
#include "cart.h"
#include <stdio.h>
#include <stdlib.h>

bus* bus_global;

void bus_init() {
    bus_global = (bus*)malloc(sizeof(bus));
    bus_global->hram = malloc(0x80);
    bus_global->wram = malloc(0x2000);
}

void bus_unload() {
    if (bus_global) {
        if (bus_global->hram) { free(bus_global->hram); }
        if (bus_global->wram) { free(bus_global->wram); }
        free(bus_global);
    }
}

u8 bus_read(u16 addr) {
    if (addr <= 0x7FFF) {
        return cart_read(addr);
    }
    else if (addr <= 0x9FFF) {
        // VRAM
        return ppu_read_vram(addr);
    }
    else if (addr <= 0xBFFF) {
        // Cartridge RAM
        return cart_read(addr);
    }
    else if (addr <= 0xDFFF) {
        // Work RAM
        return wram_read(addr);
    }
    else if (addr <= 0xFDFF) {
        // Echo RAM (E000-FDFF maps to C000-DDFF)
        return wram_read(addr - 0x2000);
    }
    else if (addr <= 0xFE9F) {
        // OAM
        if (dma_transferring()) return 0xFF;

        return ppu_read_oam(addr);
    }
    else if (addr <= 0xFEFF) {
        // Unusable area
        // printf("UNSUPPORTED bus_write()\n");
        return 0xFF;
    }
    else if (addr <= 0xFF7F) {
        // I/O Registers
        return io_read(addr);
    }
    else if (addr == 0xFFFF) {
        // Interrupt Enable
        return cpu_global->IE;
    }

    return hram_read(addr); // default
}

void bus_write(u16 addr, u8 value) {
    if (addr <= 0x7FFF) {
        // ROM data
        // cpu_global->cart[addr] = value;
        cart_write(addr, value);
        return;
    }
    else if (addr <= 0x9FFF) {
        // VRAM
        ppu_write_vram(addr, value);
    }
    else if (addr <= 0xBFFF) {
        cart_write(addr, value);
    }
    else if (addr <= 0xDFFF) {
        // Work RAM
        wram_write(addr, value);
    }
    else if (addr <= 0xFDFF) {
        // Echo RAM (E000-FDFF maps to C000-DDFF)
        wram_write(addr - 0x2000, value);
    }
    else if (addr <= 0xFE9F) {
        // OAM
        if (dma_transferring()) return;

        ppu_write_oam(addr, value);
    }
    else if (addr <= 0xFEFF) {
        // Unusable
        // printf("UNSUPPORTED bus_write()\n");
        return;
    }
    else if (addr <= 0xFF7F) {
        // I/O Registers
        io_write(addr, value);
    }
    else if (addr == 0xFFFF) {
        // Interrupt Enable
        cpu_global->IE = value;
    }
    else {
        hram_write(addr, value);
    }
}

static char dbg_msg[1024];
static int msg_size = 0;

#include <string.h>

static void dbg_info() {
    if (bus_read(0xFF02) == 0x81) {
        char c = bus_read(0xFF01);
        bus_write(0xFF02, 0);
        dbg_msg[msg_size++] = c;
    }

    dbg_msg[1023] = '\0';

    if (dbg_msg[0]) {
        printf("SERIAL: %s\n", dbg_msg);
    }
}

static void cycle(u8 m_cycles) {
    for (u8 i = 0; i < m_cycles; ++i) {
        for (u8 n = 0; n < 4; ++n) {
            timer_tick();
            ppu_tick();
        }

        dma_tick();
    }
}

#define _CPU_DEBUG 1

#if _CPU_DEBUG
    #include "platform.h"
#endif

void bus_step() {
    if (!cpu_global->halted) {
        // Fetch
        u8 opcode = bus_read(cpu_global->pc);
        const instruction* instr = &cpu_global->optable[opcode];
#if _CPU_DEBUG
        if (platform_key_down(PLATFORM_KEY_F))
            cpu_print_dbg_info(instr);

        if (instr->func == NULL) {
            printf("Unknown opcode 0x%02X at PC=0x%04X\n", opcode, cpu_global->pc - 1);
            __debugbreak();
            exit(1);
        }
#endif
        cpu_global->pc++;

        // Execute
        u8 cycles = instr->func();

        cpu_global->cycles += cycles;
        cycle(cycles / 4);
    }
    else {
        cycle(1);

        if (cpu_global->IF) cpu_global->halted = false;
    }
    
    if (cpu_global->ints_enabled) {
        cpu_handle_interrupts();
        cpu_global->int_next = false;
    }

    if (cpu_global->int_next) {
        cpu_global->ints_enabled = true;
    }
}