#pragma once
#include <common.h>

#define FRAMEBUFFER_WIDTH 256
#define FRAMEBUFFER_HEIGHT 256

typedef struct {
    u8 z, x, enter, shift, up, down, left, right, space, f, c, u;
} keys;

u8 platform_open_window(i32 width, i32 height);
void platform_pump_messages();
void platform_shutdown();
void platform_sleep_ms(u64 ms);
f64 platform_get_elapsed_time_ms();
u8 platform_should_run();
void platform_put_pixel(i32 x, i32 y, u8 r, u8 g, u8 b);
void platform_render();
keys platform_get_keys();