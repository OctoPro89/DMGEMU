#pragma once

#include "common.h"

typedef struct {
	u8 lcdc;
	u8 lcdstat;
	u8 scroll_y;
	u8 scroll_x;
	u8 ly;
	u8 ly_compare;
	u8 dma;
	u8 bg_palette;
	u8 obj_palette[2];
	u8 win_y;
	u8 win_x;

	u8 bg_colors[4];
	u8 sp1_colors[4];
	u8 sp2_colors[4];
} lcd;

typedef enum {
	MODE_HBLANK = 0x00,
	MODE_VBLANK = 0x01,
	MODE_OAM = 0x02,
	MODE_TRANSFER = 0x03
} lcd_mode;

extern lcd* lcd_global;

#define LCDC_BGW_ENABLE (GETBIT(lcd_global->lcdc, 0))
#define LCDC_OBJ_ENABLE (GETBIT(lcd_global->lcdc, 1))
#define LCDC_OBJ_HEIGHT (GETBIT(lcd_global->lcdc, 2) ? 16 : 8)
#define LCDC_BG_TILE_MAP (GETBIT(lcd_global->lcdc, 3) ? 0x9C00 : 0x9800)
#define LCDC_BG_WINDOW_TILES (GETBIT(lcd_global->lcdc, 4) ? 0x8000 : 0x8800)
#define LCDC_WINDOW_ENABLE (GETBIT(lcd_global->lcdc, 5))
#define LCDC_WINDOW_TILE_MAP_AREA (GETBIT(lcd_global->lcdc, 6) ? 0x9C00 : 0x9800)
#define LCDC_LCD_PPU_ENABLE (GETBIT(lcd_global->lcdc, 7))

#define LCDS_MODE ((lcd_mode)(lcd_global->lcdstat & 0b11))
#define LCDS_MODE_SET(mode) { lcd_global->lcdstat &= ~0b11; lcd_global->lcdstat |= mode; }

#define LCDS_LYC (GETBIT(lcd_global->lcdstat, 2))
#define LCDS_LYC_SET(b) { if (b) SETBIT(lcd_global->lcdstat, 2); else CLEARBIT(lcd_global->lcdstat, 2); }

typedef enum {
	SS_HBLANK = (1 << 3),
	SS_VBLANK = (1 << 4),
	SS_OAM = (1 << 5),
	SS_LYC = (1 << 6)
} status_source;

#define LCDS_STAT_INT(src) (lcd_global->lcdstat & src)

void lcd_init();
void lcd_unload();
u8 lcd_read(u16 addr);
void lcd_write(u16 addr, u8 value);