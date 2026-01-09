#pragma once
#include "common.h"

typedef struct mapper mapper;

struct mapper {
    u8(*read)(mapper* m, u16 addr);
    void (*write)(mapper* m, u16 addr, u8 value);
    void (*reset)(mapper* m);
    void (*destroy)(mapper* m);

    // Common data every mapper can use
    u8* rom;
    size_t rom_size;

    u8* ram_banks[16];
    u8 ram_bank_count;

    bool ram_enabled;
    bool battery;
};
