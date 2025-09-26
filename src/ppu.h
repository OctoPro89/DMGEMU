#pragma once

#include "common.h"
#include "cpu.h"

static const u16 LINES_PER_FRAME = 154;
static const u16 TICKS_PER_LINE = 456;
static const u16 PPU_HEIGHT = 144;
static const u16 PPU_WIDTH = 160;

typedef struct {
	u8 y;
	u8 x;
	u8 tile_index;
	
	struct {
		u8 CGB_PALLETE_NUMBER : 3;
		u8 CGB_VRAM_BANK : 1;
		u8 DMG_PALLETE_NUMBER : 1;
		u8 XFLIP : 1;
		u8 YFLIP : 1;
		u8 PRIORITY : 1;
	} flags;
} oam;

typedef struct {
	u64 mode_clock;
	u32 line;
	cpu* c;
	oam oam_ram[40];
	u8 vram[0x2000];
	u32 current_frame;
	u32 line_ticks;
	u8* video_buffer;
} ppu;

void ppu_init();
void ppu_unload();
void ppu_tick();
void ppu_write_oam(u16 addr, u8 value);
u8 ppu_read_oam(u16 addr);

void ppu_write_vram(u16 addr, u8 value);
u8 ppu_read_vram(u16 addr);

extern ppu* ppu_global;