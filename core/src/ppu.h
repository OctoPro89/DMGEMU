#pragma once

#include "common.h"
#include "cpu.h"

static const u16 LINES_PER_FRAME = 154;
static const u16 TICKS_PER_LINE = 456;
static const u16 PPU_YRES = 144;
static const u16 PPU_XRES = 160;

typedef enum {
	FETCH_STATE_TILE,
	FETCH_STATE_DATA0,
	FETCH_STATE_DATA1,
	FETCH_STATE_IDLE,
	FETCH_STATE_PUSH
} fetch_state;

#define FIFO_CAPACITY 16

typedef struct {
	u8 buffer[FIFO_CAPACITY];
	u32 head;
	u32 tail;
	u32 size;
} fifo;

typedef struct {
	fetch_state current_fetch_state;
	fifo pixel_fifo;
	u8 line_x;
	u8 pushed_x;
	u8 fetch_x;
	u8 bgw_fetch_data[3];
	u8 fetch_entry_data[6];
	u8 map_y;
	u8 map_x;
	u8 tile_y;
	u8 fifo_x;
} pixel_fifo;

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

typedef struct _oam_line_entry {
	oam entry;
	struct _oam_line_entry* next;
} oam_line_entry;

typedef struct {
	u64 mode_clock;
	u32 line;
	oam oam_ram[40];
	u8 line_sprite_count; // 0 - 10
	oam_line_entry* line_sprites;
	oam_line_entry line_entry_array[10];
	u8 fetched_entry_count;
	oam fetched_entries[3];
	pixel_fifo pf;
	u8 window_line;
	u8* vram;
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