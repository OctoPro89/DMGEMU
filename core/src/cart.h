#pragma once

#include "common.h"

typedef struct {
	size_t rom_size;
	u8* rom;
	u8 title[16];

	// MBC1 data
	bool ram_enabled;
	bool ram_banking; 
	u8* rom_bank_x;
	u8 banking_mode;

	u8 rom_bank_value;
	u8 ram_bank_value;

	u8* ram_bank;
	u8* ram_banks[16];

	u8 type;

	// Battery backed carts
	bool battery;
	bool need_save;
} cart;

void cart_open(const char* filepath);
void cart_unload();

u8 cart_read(u16 addr);
void cart_write(u16 addr, u8 value);

void cart_battery_save(const char* filepath);
void cart_battery_load(const char* filepath);

extern cart cart_global;