#pragma once

#include <common.h>
#include <bus.h>

u8 wram_read(bus* b, u16 address);
void wram_write(bus* b, u16 address, u8 value);

u8 hram_read(bus* b, u16 address);
void hram_write(bus* b, u16 address, u8 value);