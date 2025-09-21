#include "ram.h"

u8 wram_read(bus* b, u16 address) {
	address -= 0xC000;
	return b->wram[address];
}

void wram_write(bus* b, u16 address, u8 value) {
	address -= 0xC000;
	b->wram[address] = value;
}

u8 hram_read(bus* b, u16 address) {
	address -= 0xFF80;
	return b->hram[address];
}

void hram_write(bus* b, u16 address, u8 value) {
	address -= 0xFF80;
	b->hram[address] = value;
}