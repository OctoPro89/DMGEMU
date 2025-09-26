#pragma once

#include "common.h"
#include "cpu.h"
#include "ppu.h"
// #include "timer.h"

typedef struct {
	u16 div;
	u8 tima;
	u8 tma;
	u8 tac;
} timer;

void timer_init();
void timer_tick();

void timer_write(u16 address, u8 value);
u8 timer_read(u16 address);

extern timer* timer_global;

typedef struct bus {
	u8* wram;
	u8* hram;
} bus;

void bus_init();
void bus_unload();
u8 bus_read(u16 addr);
void bus_write(u16 addr, u8 value);
void bus_step();

extern bus* bus_global;