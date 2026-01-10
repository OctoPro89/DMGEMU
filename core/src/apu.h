#pragma once
#include "common.h"

#define APU_CLOCK 4194304

typedef struct {
    bool enabled;
    bool dac_enabled;
    bool length_enable;

    u8 sweep;        // NR10
    u8 duty_length;  // NR11
    u8 envelope;     // NR12
    u8 freq_lo;      // NR13
    u8 freq_hi;      // NR14

    u16 timer;
    u8 duty_pos;

    u8 volume;
    u8 envelope_timer;
    u8 length_counter;
    bool envelope_inc;
    u8 sweep_timer;
} square_channel;

typedef struct {
    bool enabled;
    bool dac_enabled;
    bool length_enable;

    u8 length;   // NR31
    u8 volume;   // NR32
    u8 freq_lo;  // NR33
    u8 freq_hi;  // NR34

    u16 timer;
    u8 wave_pos;
    u8 length_counter;
    u8 wave_ram[16];
} wave_channel;

typedef struct {
    bool enabled;
    bool dac_enabled;
    bool length_enable;

    u8 length;     // NR41
    u8 envelope;   // NR42
    u8 poly;       // NR43
    u8 control;    // NR44

    u16 timer;
    u16 lfsr;
    u8 volume;
    u8 envelope_timer;
    u8 length_counter;
    bool envelope_inc;
} noise_channel;

typedef struct {
    bool master_enable;

    square_channel ch1;
    square_channel ch2;
    wave_channel   ch3;
    noise_channel  ch4;

    u8 nr50;
    u8 nr51;
    u8 nr52;

    u16 frame_seq_counter;
    u8 frame_seq_step;

    f32 sample_accumulator;
} apu;

void apu_init();
void apu_unload();

void apu_tick();

u8 apu_read(u16 addr);
void apu_write(u16 addr, u8 value);

extern apu* apu_global;

extern u32 apu_sampling_rate;