#pragma once

#include "common.h"

typedef struct {
	bool start;
	bool select;
	bool a;
	bool b;
	bool up;
	bool down;
	bool left;
	bool right;
} gamepad;

typedef struct {
	bool button_select;
	bool direction_select;
	gamepad controller;
} gamepad_context;

bool gamepad_button_select();
bool gamepad_direction_select();
void gamepad_set_select(u8 value);
u8 gamepad_get_output();

extern gamepad_context gamepad_global;