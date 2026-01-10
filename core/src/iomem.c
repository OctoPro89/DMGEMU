#include "iomem.h"
#include "dma.h"
#include "lcd.h"
#include "apu.h"
#include "gamepad.h"
#include <stdio.h>
#include <stdlib.h>

static char serial_data[2];

u8 io_read(u16 addr) {
	switch (addr) {
		case 0xFF00: return gamepad_get_output();
		case 0xFF01: return serial_data[0];
		case 0xFF02: return serial_data[1];
		case 0xFF0F: return cpu_global->IF;
		default: {
			if (BETWEEN(addr, 0xFF04, 0xFF07)) {
				return timer_read(addr);
			}

			if (BETWEEN(addr, 0xFF10, 0xFF3F) || addr == 0xFF26) {
				return apu_read(addr);
			}

			if (BETWEEN(addr, 0xFF40, 0xFF4B)) {
				return lcd_read(addr);
			}

			break;
		}
	}

	return 0;
}

void io_write(u16 addr, u8 value) {
	switch (addr) {
		case 0xFF00: gamepad_set_select(value); break;
		case 0xFF01: serial_data[0] = value; break;
		case 0xFF02: serial_data[1] = value; break;
		case 0xFF0F: cpu_global->IF = value; break;
		default: {
			if (BETWEEN(addr, 0xFF04, 0xFF07)) {
				timer_write(addr, value);
			}

			if (BETWEEN(addr, 0xFF10, 0xFF3F) || addr == 0xFF26) {
				apu_write(addr, value);
				return;
			}

			if (BETWEEN(addr, 0xFF40, 0xFF4B)) {
				lcd_write(addr, value);
			}

			break;
		}
	}
}