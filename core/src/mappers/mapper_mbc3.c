#include "mapper_mbc3.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    u8 seconds;
    u8 minutes;
    u8 hours;
    u16 days;
    bool halt;
    bool carry;
} mapper_rtc;

typedef struct {
    mapper base;

    /* ROM */
    u8 rom_bank;
    u8* rom_bank_ptr;

    /* RAM */
    u8 ram_bank;
    bool ram_selected;

    /* RTC */
    bool rtc_selected;
    u8 rtc_reg;

    mapper_rtc rtc;

    /* Latched copy */
    mapper_rtc rtc_latch;

    u8 latch_state;
    time_t last_timestamp;
} mapper_mbc3;

static void rtc_update(mapper_mbc3* mbc) {
    if (mbc->rtc.halt)
        return;

    time_t now = time(NULL);
    time_t diff = now - mbc->last_timestamp;
    mbc->last_timestamp = now;

    while (diff--) {
        mbc->rtc.seconds++;
        if (mbc->rtc.seconds >= 60) {
            mbc->rtc.seconds = 0;
            mbc->rtc.minutes++;
        }
        if (mbc->rtc.minutes >= 60) {
            mbc->rtc.minutes = 0;
            mbc->rtc.hours++;
        }
        if (mbc->rtc.hours >= 24) {
            mbc->rtc.hours = 0;
            mbc->rtc.days++;
            if (mbc->rtc.days >= 512) {
                mbc->rtc.days &= 0x1FF;
                mbc->rtc.carry = true;
            }
        }
    }
}

static u8 mbc3_read(mapper* m, u16 addr) {
    mapper_mbc3* mbc = (mapper_mbc3*)m;

    if (addr < 0x4000)
        return m->rom[addr];

    if (addr < 0x8000)
        return mbc->rom_bank_ptr[addr - 0x4000];

    if (addr >= 0xA000 && addr < 0xC000) {
        if (!m->ram_enabled)
            return 0xFF;

        rtc_update(mbc);

        if (mbc->rtc_selected) {
            switch (mbc->rtc_reg) {
            case 0x08: return mbc->rtc_latch.seconds;
            case 0x09: return mbc->rtc_latch.minutes;
            case 0x0A: return mbc->rtc_latch.hours;
            case 0x0B: return mbc->rtc_latch.days & 0xFF;
            case 0x0C:
                return ((mbc->rtc_latch.days >> 8) & 1) |
                    (mbc->rtc_latch.halt << 6) |
                    (mbc->rtc_latch.carry << 7);
            }
        }
        else if (mbc->ram_selected) {
            if (mbc->ram_bank >= m->ram_bank_count)
                return 0xFF;

            return m->ram_banks[mbc->ram_bank][addr - 0xA000];
        }
        else {
            return 0xFF; // unmapped
        }
    }

    return 0xFF;
}


static void mbc3_write(mapper* m, u16 addr, u8 val) {
    mapper_mbc3* mbc = (mapper_mbc3*)m;

    if (addr < 0x2000) {
        m->ram_enabled = ((val & 0x0F) == 0x0A);
    }
    else if (addr < 0x4000) {
        u8 max_banks = (u8)(m->rom_size / 0x4000);

        if (val == 0) val = 1;
        val &= 0x7F; // ensure only lower 7 bits
        if (val >= max_banks) val %= max_banks;

        mbc->rom_bank = val;
        mbc->rom_bank_ptr = m->rom + 0x4000 * mbc->rom_bank;
    }
    else if (addr < 0x6000) {
        if (val <= 0x03) {
            mbc->ram_bank = val;
            mbc->rtc_selected = false;
            mbc->ram_selected = true;
        }
        else if (val >= 0x08 && val <= 0x0C) {
            mbc->rtc_selected = true;
            mbc->ram_selected = false;
            mbc->rtc_reg = val;
        }
    }
    else if (addr < 0x8000) {
        if (mbc->latch_state == 0 && val == 1) {
            mbc->rtc_latch = mbc->rtc;
        }
        mbc->latch_state = val;
    }
    else if (addr >= 0xA000 && addr < 0xC000) {
        if (!m->ram_enabled)
            return;

        rtc_update(mbc);

        if (mbc->rtc_selected) {
            switch (mbc->rtc_reg) {
            case 0x08: mbc->rtc.seconds = val % 60; break;
            case 0x09: mbc->rtc.minutes = val % 60; break;
            case 0x0A: mbc->rtc.hours = val % 24; break;
            case 0x0B: mbc->rtc.days = (mbc->rtc.days & 0x100) | val; break;
            case 0x0C:
                mbc->rtc.days = (mbc->rtc.days & 0xFF) | ((val & 1) << 8);
                mbc->rtc.halt = (val >> 6) & 1;
                mbc->rtc.carry = (val >> 7) & 1;
                break;
            }
        }
        else if (mbc->ram_selected) {
            if (mbc->ram_bank >= m->ram_bank_count)
                return;

            m->ram_banks[mbc->ram_bank][addr - 0xA000] = val;
        }
        else {
            return; // unmapped
        }
    }
}

static void mbc3_destroy(mapper* m) {
    if (m->ram_banks) {
        for (u8 i = 0; i < m->ram_bank_count; i++)
            free(m->ram_banks[i]);
    }
    free(m);
}


mapper* mapper_mbc3_create(u8* rom, size_t size, u8 ram_size_code, bool battery)
{
    mapper_mbc3* mbc = calloc(1, sizeof(*mbc));
    if (!mbc)
        return NULL;

    /* ---- base mapper ---- */
    mbc->base.rom = rom;
    mbc->base.rom_size = size;
    mbc->base.read = mbc3_read;
    mbc->base.write = mbc3_write;
    mbc->base.destroy = mbc3_destroy;
    mbc->base.battery = battery;
    mbc->base.ram_enabled = false;

    /* ---- ROM banking ---- */
    mbc->rom_bank = 1;
    mbc->rom_bank_ptr = rom + 0x4000;

    /* ---- RAM allocation ---- */
    switch (ram_size_code) {
    case 0x02: mbc->base.ram_bank_count = 1;  break; // 8 KiB
    case 0x03: mbc->base.ram_bank_count = 4;  break; // 32 KiB
    case 0x04: mbc->base.ram_bank_count = 16; break; // 128 KiB
    case 0x05: mbc->base.ram_bank_count = 8;  break; // 64 KiB
    default:   mbc->base.ram_bank_count = 0;  break;
    }

    if (mbc->base.ram_bank_count > 0) {
        for (u8 i = 0; i < mbc->base.ram_bank_count; i++) {
            mbc->base.ram_banks[i] = calloc(0x2000, 1); // 8 KiB per bank
        }
    }

    mbc->ram_bank = 0;
    mbc->ram_selected = true;
    mbc->rtc_selected = false;

    /* ---- RTC init ---- */
    mbc->rtc.seconds = 0;
    mbc->rtc.minutes = 0;
    mbc->rtc.hours = 0;
    mbc->rtc.days = 0;
    mbc->rtc.halt = false;
    mbc->rtc.carry = false;

    mbc->rtc_latch = mbc->rtc;

    mbc->latch_state = 0;
    mbc->last_timestamp = time(NULL);

    memset(mbc->base.ram_banks[0], 0xAA, 0x2000);

    return &mbc->base;
}