#include "apu.h"
#include "platform.h"

#include <stdlib.h>
#include <string.h>

static const u8 duty_table[4][8] = {
    {0,1,0,0,0,0,0,0}, // 12.5%
    {0,1,1,0,0,0,0,0}, // 25%
    {0,1,1,1,1,0,0,0}, // 50%
    {1,0,0,1,1,1,1,1}  // 75%
};

apu* apu_global = NULL;
u32 apu_sampling_rate = 48000;

static u32 apu_clock_div = 0;
static u32 sample_counter = 0;

void apu_init() {
    apu_global = (apu*)malloc(sizeof(apu));
    memset(&apu_global[0], 0, sizeof(apu));

    apu_global->master_enable = true;

    // Correct initialization: CH4 LFSR
    apu_global->ch4.lfsr = 0x7FFF;
}

void apu_unload() {
    free(apu_global);
}

static void frame_sequencer_tick() {
    apu* a = apu_global;

    switch (a->frame_seq_step) {
    case 0:
    case 2:
    case 4:
    case 6:
        // Length counters first
        if (a->ch1.length_enable && a->ch1.length_counter)
            if (--a->ch1.length_counter == 0) a->ch1.enabled = false;
        if (a->ch2.length_enable && a->ch2.length_counter)
            if (--a->ch2.length_counter == 0) a->ch2.enabled = false;
        if (a->ch3.length_enable && a->ch3.length_counter)
            if (--a->ch3.length_counter == 0) a->ch3.enabled = false;
        if (a->ch4.length_enable && a->ch4.length_counter)
            if (--a->ch4.length_counter == 0) a->ch4.enabled = false;

        // Sweep only on steps 2 & 6
        if (a->frame_seq_step == 2 || a->frame_seq_step == 6) {
            square_channel* ch = &a->ch1;
            u8 sweep = ch->sweep;
            if ((sweep >> 4) & 7) { // sweep time > 0
                if (ch->sweep_timer == 0) ch->sweep_timer = (sweep >> 4) & 7;
                if (--ch->sweep_timer == 0) {
                    ch->sweep_timer = (sweep >> 4) & 7;
                    u16 delta = ch->timer >> (sweep & 7);
                    if (GETBIT(sweep, 3)) ch->timer += delta;
                    else ch->timer -= delta;
                    if (ch->timer > 2047) ch->enabled = false;
                }
            }
        }
        break;

    case 7:
        // Envelope
        if (a->ch1.envelope_timer && --a->ch1.envelope_timer == 0) {
            a->ch1.envelope_timer = a->ch1.envelope & 7;
            if (a->ch1.envelope_inc && a->ch1.volume < 15) a->ch1.volume++;
            if (!a->ch1.envelope_inc && a->ch1.volume > 0) a->ch1.volume--;
        }
        if (a->ch2.envelope_timer && --a->ch2.envelope_timer == 0) {
            a->ch2.envelope_timer = a->ch2.envelope & 7;
            if (a->ch2.envelope_inc && a->ch2.volume < 15) a->ch2.volume++;
            if (!a->ch2.envelope_inc && a->ch2.volume > 0) a->ch2.volume--;
        }
        if (a->ch4.envelope_timer && --a->ch4.envelope_timer == 0) {
            a->ch4.envelope_timer = a->ch4.envelope & 7;
            if (a->ch4.envelope_inc && a->ch4.volume < 15) a->ch4.volume++;
            if (!a->ch4.envelope_inc && a->ch4.volume > 0) a->ch4.volume--;
        }
        break;
    }

    a->frame_seq_step = (a->frame_seq_step + 1) & 7;
}

static INLINE void square_tick(square_channel* ch) {
    if (!ch->enabled) return;

    if (--ch->timer == 0) {
        ch->timer = (2048 - (((ch->freq_hi & 7) << 8) | ch->freq_lo)) * 4;
        ch->duty_pos = (ch->duty_pos + 1) & 7;
    }
}

static INLINE void wave_tick(wave_channel* ch) {
    if (!ch->enabled) return;

    if (--ch->timer == 0) {
        ch->timer = (2048 - (((ch->freq_hi & 7) << 8) | ch->freq_lo)) * 2;
        ch->wave_pos = (ch->wave_pos + 1) & 31;
    }
}

static INLINE void noise_tick(noise_channel* ch) {
    if (!ch->enabled) return;

    if (--ch->timer == 0) {
        u8 r = ch->poly & 7;
        u8 s = (ch->poly >> 4) & 1;
        ch->timer = (r ? (r << 4) : 8) << s;
        ch->timer *= 2; // important!

        u16 xor = (ch->lfsr ^ (ch->lfsr >> 1)) & 1;
        ch->lfsr = (ch->lfsr >> 1) | (xor << 14);
        if (ch->poly & 0x08) {
            ch->lfsr = (ch->lfsr & ~(1 << 6)) | (xor << 6);
        }
    }
}

static INLINE f32 channel_output_square(square_channel* ch) {
    if (!ch->enabled || !ch->dac_enabled) return 0.0f;
    return duty_table[ch->duty_length >> 6][ch->duty_pos]
        ? (ch->volume / 15.0f)
        : 0.0f;
}

static INLINE f32 channel_output_wave(wave_channel* ch) {
    if (!ch->enabled || !ch->dac_enabled) return 0.0f;

    u8 sample = ch->wave_ram[ch->wave_pos >> 1];
    if (!(ch->wave_pos & 1)) sample >>= 4;
    sample &= 0xF;

    switch ((ch->volume >> 5) & 3) {
    case 0: return 0.0f;
    case 1: sample <<= 0; break; // 100%
    case 2: sample >>= 1; break; // 50%
    case 3: sample >>= 2; break; // 25%
    }

    return (f32)sample / 15.0f;
}

static INLINE f32 channel_output_noise(noise_channel* ch) {
    if (!ch->enabled || !ch->dac_enabled) return 0.0f;
    return (~ch->lfsr & 1) ? (ch->volume / 15.0f) : 0.0f;
}

void apu_tick() {
    apu* a = apu_global;
    if (!a->master_enable) return;

    square_tick(&a->ch1);
    square_tick(&a->ch2);
    wave_tick(&a->ch3);
    noise_tick(&a->ch4);

    if (++a->frame_seq_counter >= 8192) {
        a->frame_seq_counter = 0;
        frame_sequencer_tick();
    }

    // Sampling
    static f64 sample_acc = 0;
    sample_acc += (f64)apu_sampling_rate / APU_CLOCK;
    while (sample_acc >= 1.0) {
        sample_acc -= 1.0;

        f32 l = 0, r = 0;

        f32 s1 = channel_output_square(&a->ch1);
        f32 s2 = channel_output_square(&a->ch2);
        f32 s3 = channel_output_wave(&a->ch3);
        f32 s4 = channel_output_noise(&a->ch4);

        if (a->nr51 & 0x01) r += s1;
        if (a->nr51 & 0x10) l += s1;
        if (a->nr51 & 0x02) r += s2;
        if (a->nr51 & 0x20) l += s2;
        if (a->nr51 & 0x04) r += s3;
        if (a->nr51 & 0x40) l += s3;
        if (a->nr51 & 0x08) r += s4;
        if (a->nr51 & 0x80) l += s4;

        l *= ((a->nr50 >> 4) & 7) / 7.0f;
        r *= (a->nr50 & 7) / 7.0f;

        platform_audio_push(l * 0.25f, r * 0.25f);
    }
}

u8 apu_read(u16 addr) {
    apu* a = apu_global;

    switch (addr) {
        // CH1
    case 0xFF10: return a->ch1.sweep | 0x80;
    case 0xFF11: return a->ch1.duty_length | 0x3F;
    case 0xFF12: return a->ch1.envelope;
    case 0xFF13: return 0xFF;
    case 0xFF14: return a->ch1.freq_hi | 0xBF;

        // CH2
    case 0xFF16: return a->ch2.duty_length | 0x3F;
    case 0xFF17: return a->ch2.envelope;
    case 0xFF18: return 0xFF;
    case 0xFF19: return a->ch2.freq_hi | 0xBF;

        // CH3
    case 0xFF1A: return a->ch3.dac_enabled ? 0x80 : 0x00;
    case 0xFF1B: return 0xFF;
    case 0xFF1C: return a->ch3.volume | 0x9F;
    case 0xFF1D: return 0xFF;
    case 0xFF1E: return a->ch3.freq_hi | 0xBF;

        // CH4
    case 0xFF20: return 0xFF;
    case 0xFF21: return a->ch4.envelope;
    case 0xFF22: return a->ch4.poly;
    case 0xFF23: return a->ch4.control | 0xBF;

        // Control
    case 0xFF24: return a->nr50;
    case 0xFF25: return a->nr51;
    case 0xFF26:
        return (a->master_enable ? 0x80 : 0x00)
            | (a->ch4.enabled << 3)
            | (a->ch3.enabled << 2)
            | (a->ch2.enabled << 1)
            | (a->ch1.enabled);

    default:
        if (addr >= 0xFF30 && addr <= 0xFF3F)
            return a->ch3.wave_ram[addr - 0xFF30];
        return 0xFF;
    }
}

void apu_write(u16 addr, u8 v) {
    apu* a = apu_global;

    // Master disable gate
    if (!a->master_enable && addr != 0xFF26)
        return;

    switch (addr) {

        // ========= CH1 =========
    case 0xFF10: a->ch1.sweep = v; break;

    case 0xFF11:
        a->ch1.duty_length = v;
        a->ch1.length_counter = 64 - (v & 0x3F);
        break;

    case 0xFF12:
        a->ch1.envelope = v;
        a->ch1.dac_enabled = (v & 0xF8) != 0;
        if (!a->ch1.dac_enabled) a->ch1.enabled = false;
        break;

    case 0xFF13: a->ch1.freq_lo = v; break;

    case 0xFF14:
        a->ch1.freq_hi = v;
        a->ch1.length_enable = GETBIT(v, 6);
        if (GETBIT(v, 7)) {
            if (a->ch1.length_counter == 0)
                a->ch1.length_counter = 64;
            a->ch1.enabled = a->ch1.dac_enabled;
            a->ch1.timer = (2048 - (((v & 7) << 8) | a->ch1.freq_lo)) * 4;
            a->ch1.duty_pos = 0;
            a->ch1.envelope_timer = a->ch1.envelope & 7;
            if (a->ch1.envelope_timer == 0) a->ch1.envelope_timer = 8;
            a->ch1.volume = a->ch1.envelope >> 4;
            a->ch1.envelope_inc = GETBIT(a->ch1.envelope, 3);
        }
        break;

        // ========= CH2 =========
    case 0xFF16:
        a->ch2.duty_length = v;
        a->ch2.length_counter = 64 - (v & 0x3F);
        break;

    case 0xFF17:
        a->ch2.envelope = v;
        a->ch2.dac_enabled = (v & 0xF8) != 0;
        if (!a->ch2.dac_enabled) a->ch2.enabled = false;
        break;

    case 0xFF18: a->ch2.freq_lo = v; break;

    case 0xFF19:
        a->ch2.freq_hi = v;
        a->ch2.length_enable = GETBIT(v, 6);
        if (GETBIT(v, 7)) {
            if (a->ch2.length_counter == 0)
                a->ch2.length_counter = 64;
            a->ch2.enabled = a->ch2.dac_enabled;
            a->ch2.timer = (2048 - (((v & 7) << 8) | a->ch2.freq_lo)) * 4;
            a->ch2.duty_pos = 0;
            a->ch2.envelope_timer = a->ch2.envelope & 7;
            if (a->ch2.envelope_timer == 0) a->ch2.envelope_timer = 8;
            a->ch2.volume = a->ch2.envelope >> 4;
            a->ch2.envelope_inc = GETBIT(a->ch2.envelope, 3);
        }

        break;

        // ========= CH3 =========
    case 0xFF1A:
        a->ch3.dac_enabled = GETBIT(v, 7);
        if (!a->ch3.dac_enabled) a->ch3.enabled = false;
        break;

    case 0xFF1B:
        a->ch3.length_counter = 256 - v;
        break;

    case 0xFF1C:
        a->ch3.volume = v;
        break;

    case 0xFF1D: a->ch3.freq_lo = v; break;

    case 0xFF1E:
        a->ch3.freq_hi = v;
        a->ch3.length_enable = GETBIT(v, 6);
        if (GETBIT(v, 7)) {
            if (a->ch3.length_counter == 0)
                a->ch3.length_counter = 256;
            a->ch3.enabled = a->ch3.dac_enabled;
            a->ch3.timer = (2048 - (((v & 7) << 8) | a->ch3.freq_lo)) * 2;
            a->ch3.wave_pos = 0;
        }
        break;

        // ========= CH4 =========
    case 0xFF20:
        a->ch4.length_counter = 64 - (v & 0x3F);
        break;

    case 0xFF21:
        a->ch4.envelope = v;
        a->ch4.dac_enabled = (v & 0xF8) != 0;
        if (!a->ch4.dac_enabled) a->ch4.enabled = false;
        break;

    case 0xFF22:
        a->ch4.poly = v;
        break;

    case 0xFF23:
        a->ch4.control = v;
        a->ch4.length_enable = GETBIT(v, 6);
        if (GETBIT(v, 7)) {
            if (a->ch4.length_counter == 0)
                a->ch4.length_counter = 64;
            a->ch4.enabled = a->ch4.dac_enabled;
            a->ch4.lfsr = 0x7FFF;
            a->ch4.envelope_timer = a->ch4.envelope & 7;
            if (a->ch4.envelope_timer == 0) a->ch4.envelope_timer = 8;
            a->ch4.volume = a->ch4.envelope >> 4;
            a->ch4.envelope_inc = GETBIT(a->ch4.envelope, 3);
        }
        break;

        // ========= CONTROL =========
    case 0xFF24: a->nr50 = v; break;
    case 0xFF25: a->nr51 = v; break;

    case 0xFF26:
        a->master_enable = GETBIT(v, 7);
        if (!a->master_enable) {
            memset(&a->ch1, 0, sizeof(square_channel));
            memset(&a->ch2, 0, sizeof(square_channel));
            memset(&a->ch3, 0, sizeof(wave_channel));
            memset(&a->ch4, 0, sizeof(noise_channel));
        }
        break;

    default:
        if (addr >= 0xFF30 && addr <= 0xFF3F)
            a->ch3.wave_ram[addr - 0xFF30] = v;
        break;
    }
}