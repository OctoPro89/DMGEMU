#include "lcd.h"
#include "ppu.h"
#include "dma.h"

#include <stdlib.h>
#include <string.h>

lcd* lcd_global;

static u8 colors[4] = { 0xFF, 0xAA, 0x55, 0x00 };

void lcd_init() {
	lcd_global = (lcd*)malloc(sizeof(lcd));
	memset(lcd_global, 0, sizeof(lcd));

	// Defaults
	lcd_global->lcdc = 0x91;
	lcd_global->bg_palette = 0xFC;
	lcd_global->obj_palette[0] = 0xFF;
	lcd_global->obj_palette[1] = 0xFF;
	
	for (u8 i = 0; i < 4; ++i) {
		lcd_global->bg_colors[i] = colors[i];
		lcd_global->sp1_colors[i] = colors[i];
		lcd_global->sp2_colors[i] = colors[i];
	}
}

void lcd_unload() {
	if (lcd_global) {
		free(lcd_global);
	}
}

u8 lcd_read(u16 addr) {
	u8 offset = (u8)(addr - 0xFF40);
	u8* p = (u8*)lcd_global;

	// Only works if struct is in order
	return p[offset];
}

static void update_palette(u8 palette_data, u8 pal) {
	u8* p_colors = lcd_global->bg_colors;

	switch (pal) {
		case 1: {
			p_colors = lcd_global->sp1_colors;
			break;
		}
		case 2: {
			p_colors = lcd_global->sp2_colors;
			break;
		}
	}

	p_colors[0] = colors[palette_data & 0b11];
	p_colors[1] = colors[(palette_data >> 2) & 0b11];
	p_colors[2] = colors[(palette_data >> 4) & 0b11];
	p_colors[3] = colors[(palette_data >> 6) & 0b11];
}

void lcd_write(u16 addr, u8 value) {
	u8 offset = (u8)(addr - 0xFF40);
	u8* p = (u8*)lcd_global;

	// Only works if struct is in order
	p[offset] = value;

	if (offset == 6) {
		// 0xFF46 - DMA
		dma_start(value);
	}

	if (addr == 0xFF47) {
		update_palette(value, 0);
	}
	else if (addr == 0xFF48) {
		update_palette(value & 0b11111100, 1);
	}
	else if (addr == 0xFF49) {
		update_palette(value & 0b11111100, 2);
	}
}