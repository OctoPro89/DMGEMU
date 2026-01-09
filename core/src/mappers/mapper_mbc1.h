#pragma once

#include "mapper.h"

mapper* mapper_mbc1_create(u8* rom, size_t size, u8 ram_size_code, bool battery);