#include "mapper.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    mapper base;

    u8 rom_bank;
    u8 ram_bank;
    bool ram_banking;
    bool need_save; // TODO

    u8* rom_bank_ptr;
    u8* ram_bank_ptr;

    u8 rom_bank_low;   // 5 bits
    u8 rom_bank_high;  // 2 bits
} mapper_mbc1;

static inline u8 mbc1_rom_bank(mapper_mbc1* mbc) {
    u8 bank = mbc->rom_bank_low & 0x1F;
    if (!mbc->ram_banking) {
        bank |= (mbc->rom_bank_high << 5);
    }
    if (bank == 0) bank = 1;
    return bank;
}

static u8 mbc1_read(mapper* m, u16 addr) {
    mapper_mbc1* mbc = (mapper_mbc1*)m;

    if (addr < 0x4000) {
        return mbc->base.rom[addr];
    }

    if ((addr & 0xE000) == 0xA000) {
        if (!mbc->base.ram_enabled) {
            return 0xFF;
        }

        if (!mbc->ram_bank_ptr) {
            return 0xFF;
        }

        return mbc->ram_bank_ptr[addr - 0xA000];
    }

    return mbc->rom_bank_ptr[addr - 0x4000];
}

static void mbc1_write(mapper* m, u16 addr, u8 val) {
    mapper_mbc1* mbc = (mapper_mbc1*)m;

    if (addr < 0x2000) {
        mbc->base.ram_enabled = ((val & 0xF) == 0xA);
    }

    if ((addr & 0xE000) == 0x2000) {
        mbc->rom_bank_low = val & 0x1F;
        if (mbc->rom_bank_low == 0)
            mbc->rom_bank_low = 1;

        mbc->rom_bank_ptr =
            mbc->base.rom + 0x4000 * mbc1_rom_bank(mbc);
    }

    if ((addr & 0xE000) == 0x4000) {
        if (mbc->ram_banking) {
            mbc->ram_bank = val & 0x03;
            mbc->ram_bank_ptr = mbc->base.ram_banks[mbc->ram_bank];
        }
        else {
            mbc->rom_bank_high = val & 0x03;
        }
    }

    if ((addr & 0xE000) == 0x6000) {
        mbc->ram_banking = val & 1;
        mbc->ram_bank_ptr = mbc->ram_banking
            ? mbc->base.ram_banks[mbc->ram_bank]
            : mbc->base.ram_banks[0];
    }

    if ((addr & 0xE000) == 0xA000) {
        if (!mbc->base.ram_enabled) {
            return;
        }

        if (!mbc->ram_bank_ptr) {
            return;
        }

        mbc->ram_bank_ptr[addr - 0xA000] = val;

        if (mbc->base.battery) {
            mbc->need_save = true;
        }
    }
}

static void destroy(mapper* m) {
    if (m->ram_banks) {
        for (u8 i = 0; i < m->ram_bank_count; i++) {
            free(m->ram_banks[i]);
        }
    }

    free(m);
}


mapper* mapper_mbc1_create(u8* rom, size_t size, u8 ram_size_code, bool battery)
{
    mapper_mbc1* mbc = calloc(1, sizeof(*mbc));

    mbc->base.rom = rom;
    mbc->base.rom_size = size;
    mbc->base.read = mbc1_read;
    mbc->base.write = mbc1_write;
    mbc->base.destroy = destroy;
    mbc->base.battery = battery;

    mbc->rom_bank = 1;
    mbc->rom_bank_ptr = rom + 0x4000;

    switch (ram_size_code) {
        case 2: mbc->base.ram_bank_count = 1; break;   // 8KB
        case 3: mbc->base.ram_bank_count = 4; break;   // 32KB
        case 4: mbc->base.ram_bank_count = 16; break;  // 128KB
        case 5: mbc->base.ram_bank_count = 8; break;   // 64KB
        default: mbc->base.ram_bank_count = 0; break;
    }

    for (u8 i = 0; i < 16; ++i) {
        mbc->base.ram_banks[i] = malloc(sizeof(u8) * 0x2000);
        memset(mbc->base.ram_banks[i], 0, 0x2000);
    }

    mbc->ram_bank = 0;
    mbc->ram_bank_ptr = mbc->base.ram_bank_count ? mbc->base.ram_banks[0] : NULL;

    return &mbc->base;
}
