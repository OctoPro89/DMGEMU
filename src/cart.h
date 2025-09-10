#pragma once

#include "common.h"

u8* cart_open(const char* filepath);
void cart_unload(u8* cart);