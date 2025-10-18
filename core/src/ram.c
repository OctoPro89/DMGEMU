#include "ram.h"
#include "bus.h"

u8 wram_read(u16 address) {
	address -= 0xC000;
	return bus_global->wram[address];
}

void wram_write(u16 address, u8 value) {
	address -= 0xC000;
	bus_global->wram[address] = value;
}

u8 hram_read(u16 address) {
	address -= 0xFF80;
	return bus_global->hram[address];
}

void hram_write(u16 address, u8 value) {
	address -= 0xFF80;
	bus_global->hram[address] = value;
}