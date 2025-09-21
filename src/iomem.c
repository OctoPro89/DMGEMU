#include "iomem.h"
#include <stdio.h>
#include <stdlib.h>

static char serial_data[2];

u8 io_read(bus* b, u16 addr) {
	switch (addr) {
		case 0xFF01: return serial_data[0];
		case 0xFF02: return serial_data[1];
		case 0xFF04: break;
		case 0xFF05: break;
		case 0xFF06: break;
		case 0xFF07: break;
		case 0xFF0F: break;
		default: break;
			// printf("UNSUPPORTED bus_read(%04X)\n", addr);
	}
	// exit(1);
	return 0;
}

void io_write(bus* b, u16 addr, u8 value) {
	switch (addr) {
		case 0xFF01: serial_data[0] = value; break;
		case 0xFF02: serial_data[1] = value; break;
		case 0xFF04: timer_write(b->t, addr, value);
		case 0xFF05: timer_write(b->t, addr, value);
		case 0xFF06: timer_write(b->t, addr, value);
		case 0xFF07: timer_write(b->t, addr, value);
		case 0xFF0F: b->c->IF = value; break;
		default: break;
			// printf("UNSUPPORTED bus_write(%04X)\n", addr);
	}

	// exit(1);
}