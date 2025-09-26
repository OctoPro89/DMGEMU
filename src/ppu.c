#include "ppu.h"
#include "lcd.h"
#include "platform.h"
#include "interrupts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static __forceinline void inc_ly() {
    ++lcd_global->ly;

    if (lcd_global->ly == lcd_global->ly_compare) {
        LCDS_LYC_SET(1);

        if (LCDS_STAT_INT(SS_LYC)) {
            cpu_global->IF |= INT_LCD_STAT;
        }
    }
    else {
        LCDS_LYC_SET(0);
    }
}

// -- STATE MACHINE --

static void ppu_mode_oam() {
    if (ppu_global->line_ticks >= 80) {
        LCDS_MODE_SET(MODE_TRANSFER);
    }
}

static void ppu_mode_transfer() {
    if (ppu_global->line_ticks >= 252) {
        LCDS_MODE_SET(MODE_HBLANK);
    }
}

static void ppu_mode_vblank() {
    if (ppu_global->line_ticks >= TICKS_PER_LINE) {
        inc_ly();

        if (lcd_global->ly >= LINES_PER_FRAME) {
            LCDS_MODE_SET(MODE_OAM);
            lcd_global->ly = 0;
        }

        ppu_global->line_ticks = 0;
    }
}

static u32 target_frame_time_ms = 1000 / 60;
static u64 prev_frame_time = 0;
static u64 start_timer = 0;
static u64 frame_count = 0;

static void ppu_mode_hblank() {
    if (ppu_global->line_ticks >= TICKS_PER_LINE) {
        inc_ly();

        if (lcd_global->ly >= PPU_HEIGHT) {
            LCDS_MODE_SET(MODE_VBLANK);
            cpu_global->IF |= INT_VBLANK;

            if (LCDS_STAT_INT(SS_VBLANK)) cpu_global->IF |= INT_LCD_STAT;

            ++ppu_global->current_frame;
        }
        else {
            LCDS_MODE_SET(MODE_OAM);
        }

        ppu_global->line_ticks = 0;
    }
}

// -- STATE MACHINE --

ppu* ppu_global;

void ppu_init() {
    ppu_global = malloc(sizeof(ppu));
    memset(ppu_global, 0, sizeof(ppu));
    ppu_global->video_buffer = malloc(sizeof(u8) * PPU_WIDTH * PPU_HEIGHT);
    memset(&ppu_global->video_buffer[0], 0, sizeof(u8) * PPU_WIDTH * PPU_HEIGHT);

    lcd_init();
    LCDS_MODE_SET(MODE_OAM);
}

void ppu_unload() {
    if (ppu_global) {
        if (ppu_global->video_buffer) free(ppu_global->video_buffer);
        free(ppu_global);
    }
}

void ppu_tick() {
    ++ppu_global->line_ticks;

    switch (LCDS_MODE) {
        case MODE_OAM:
            ppu_mode_oam();
            break;
        case MODE_TRANSFER:
            ppu_mode_transfer();
            break;
        case MODE_VBLANK:
            ppu_mode_vblank();
            break;
        case MODE_HBLANK:
            ppu_mode_hblank();
            break;
    }
} 

void ppu_write_oam(u16 addr, u8 value) {
    if (addr >= 0xFE00) {
        addr -= 0xFE00;
    }

    u8* oram = (u8*)ppu_global->oam_ram;
    oram[addr] = value;
}

u8 ppu_read_oam(u16 addr) {
    if (addr >= 0xFE00) {
        addr -= 0xFE00;
    }

    u8* oram = (u8*)ppu_global->oam_ram;
    return oram[addr];
}

void ppu_write_vram(u16 addr, u8 value) {
    ppu_global->vram[addr - 0x8000] = value;
}

u8 ppu_read_vram(u16 addr) {
    return ppu_global->vram[addr - 0x8000];
}