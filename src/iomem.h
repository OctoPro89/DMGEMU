#pragma once

#include "common.h"
#include "bus.h"

u8 io_read(bus* b, u16 addr);
void io_write(bus* b, u16 addr, u8 value);