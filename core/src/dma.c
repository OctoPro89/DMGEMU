#include "dma.h"
#include "bus.h"
#include "ppu.h"

typedef struct {
	bool active;
	u8 byte;
	u8 value;
	u8 start_delay;
} dma;

static dma _dma;

void dma_start(u8 start) {
	_dma.active = true;
	_dma.byte = 0;
	_dma.start_delay = 2;
	_dma.value = start;
}

void dma_tick() {
	if (!_dma.active) {
		return;
	}

	if (_dma.start_delay) {
		--_dma.start_delay;
		return;
	}

	ppu_write_oam(_dma.byte, bus_read((_dma.value * 0x100) + _dma.byte));

	++_dma.byte;
	_dma.active = _dma.byte < 0xA0;
}

bool dma_transferring() {
	return _dma.active;
}