#pragma once

#include "common.h"
#include "cpu.h"

typedef struct {
	u64 mode_clock;
	u32 line;
	cpu* c;
} ppu;

ppu* ppu_init(cpu* c);
void ppu_unload(ppu* p);
void ppu_step(ppu* p);