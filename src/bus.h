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

timer timer_init();
void timer_tick(cpu* c, timer* t);

void timer_write(timer* t, u16 address, u8 value);
u8 timer_read(timer* t, u16 address);

typedef struct bus {
	cpu* c;
	ppu* p;
	timer* t;
	u8* wram;
	u8* hram;
} bus;

bus bus_init(cpu* c, ppu* p, timer* t);
void bus_unload(bus* b);
u8 bus_read(bus* b, u16 addr);
void bus_write(bus* b, u8 value, u16 addr);
void bus_step(bus* b);