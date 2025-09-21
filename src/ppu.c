#include "ppu.h"
#include <stdlib.h>

ppu* ppu_init(cpu* c) {
    ppu* p = malloc(sizeof(ppu));
    p->c = c;
    return p;
}

void ppu_unload(ppu* p) {
    if (p) {
        free(p);
    }
}

// call this once per machine cycle (1 CPU cycle = 1 PPU cycle on DMG)
void ppu_step(ppu* p) {
    p->mode_clock++;

    // One scanline = 456 cycles
    if (p->mode_clock >= 456) {
        p->mode_clock = 0;
        p->line++;
        if (p->line == 144) {
            // Enter VBlank
            p->c->IF |= 0x01; // Request VBlank interrupt
        }
        else if (p->line > 153) {
            // Restart frame
            p->line = 0;
        }
        // Update LY register (0xFF44)
        p->c->memory[0xFF44] = (u8)p->line;
    }
}