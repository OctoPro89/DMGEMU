#pragma once

#include "common.h"

u8* cart_open(const char* filepath, size_t* out_size);
void cart_unload(u8* cart);