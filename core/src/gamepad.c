#include "gamepad.h"

gamepad_context gamepad_global;

bool gamepad_button_select() {
	return gamepad_global.button_select;
}

bool gamepad_direction_select() {
	return gamepad_global.direction_select;
}

void gamepad_set_select(u8 value) {
	gamepad_global.button_select = value & 0x20;
	gamepad_global.direction_select = value & 0x10;
}

u8 gamepad_get_output() {
	u8 output = 0xCF;

	if (!gamepad_button_select()) {
		if (gamepad_global.controller.start) output &= ~(1 << 3);
		if (gamepad_global.controller.select) output &= ~(1 << 2);
		if (gamepad_global.controller.b) output &= ~(1 << 1);
		if (gamepad_global.controller.a) output &= ~(1 << 0);
	}

	if (!gamepad_direction_select()) {
		if (gamepad_global.controller.right) output &= ~(1 << 0);
		if (gamepad_global.controller.left) output &= ~(1 << 1);
		if (gamepad_global.controller.up) output &= ~(1 << 2);
		if (gamepad_global.controller.down) output &= ~(1 << 3);
	}

	return output;
}