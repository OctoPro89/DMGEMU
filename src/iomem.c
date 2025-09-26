#include "iomem.h"
#include "dma.h"
#include "lcd.h"
#include <stdio.h>
#include <stdlib.h>

static char serial_data[2];

u8 io_read(u16 addr) {
	switch (addr) {
		case 0xFF01: return serial_data[0];
		case 0xFF02: return serial_data[1];
		case 0xFF0F: return cpu_global->IF;
		default: {
			// printf("UNSUPPORTED bus_read(%04X)\n", addr);

			if (BETWEEN(addr, 0xFF04, 0xFF07)) {
				return timer_read(addr);
			}

			if (BETWEEN(addr, 0xFF40, 0xFF4B)) {
				lcd_read(addr);
			}

			break;
		}
	}
	// exit(1);
	return 0;
}

void io_write(u16 addr, u8 value) {
	switch (addr) {
		case 0xFF01: serial_data[0] = value; break;
		case 0xFF02: serial_data[1] = value; break;
		case 0xFF0F: cpu_global->IF = value; break;
		default: {
			// printf("UNSUPPORTED bus_write(%04X)\n", addr);

			if (BETWEEN(addr, 0xFF04, 0xFF07)) {
				timer_write(addr, value);
			}

			if (BETWEEN(addr, 0xFF40, 0xFF4B)) {
				lcd_write(addr, value);
			}
		}
	}

	// exit(1);
}