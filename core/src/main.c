#include "cart.h"
#include "bus.h"
#include "platform.h"
#include "lcd.h"
#include "gamepad.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

platform_window* wnd;       // Main emulator window
platform_window* tile_wnd;  // Tile viewer window

static u8 tile_colors[4] = { 0xFF, 0xAA, 0x55, 0x00 };

static void ppu_display_tile(platform_window* w, u16 addr, u16 tile_num, int x, int y) {
    for (int row = 0; row < 8; ++row) {
        u8 lo_byte = bus_read(addr + tile_num * 16 + (u16)row * 2);
        u8 hi_byte = bus_read(addr + tile_num * 16 + (u16)row * 2 + 1);

        for (int col = 0; col < 8; ++col) {
            u8 lo = (lo_byte >> (7 - col)) & 1;
            u8 hi = (hi_byte >> (7 - col)) & 1;
            u8 color = (hi << 1) | lo;

            platform_put_pixel(w, x + col, y + row,
                tile_colors[color],
                tile_colors[color],
                tile_colors[color]);
        }
    }
}

static void render() {
    // --- Main emulator window ---
    for (int y = 0; y < PPU_YRES; ++y) {
        for (int x = 0; x < PPU_XRES; ++x) {
            int index = x + y * PPU_XRES;
            platform_put_pixel(
                wnd,
                x, y,
                ppu_global->video_buffer[index],
                ppu_global->video_buffer[index],
                ppu_global->video_buffer[index]
            );
        }
    }
    platform_render(wnd);

    // --- Tile viewer window ---
    u16 tile_num = 0;
    u16 addr = 0x8000;
    for (int ty = 0; ty < 24; ++ty) {
        for (int tx = 0; tx < 16; ++tx) {
            int x_draw = tx * 8;
            int y_draw = ty * 8;
            ppu_display_tile(tile_wnd, addr, tile_num, x_draw, y_draw);
            ++tile_num;
        }
    }
    platform_render(tile_wnd);
}

static void input() {
    if (platform_key_down(PLATFORM_KEY_ENTER)) gamepad_global.controller.start = true;
    if (platform_key_down(PLATFORM_KEY_TAB)) gamepad_global.controller.select = true;
    if (platform_key_down(PLATFORM_KEY_Z)) gamepad_global.controller.a = true;
    if (platform_key_down(PLATFORM_KEY_X)) gamepad_global.controller.b = true;

    if (!platform_key_down(PLATFORM_KEY_ENTER)) gamepad_global.controller.start = false;
    if (!platform_key_down(PLATFORM_KEY_TAB)) gamepad_global.controller.select = false;
    if (!platform_key_down(PLATFORM_KEY_Z)) gamepad_global.controller.a = false;
    if (!platform_key_down(PLATFORM_KEY_X)) gamepad_global.controller.b = false;

    if (platform_key_down(PLATFORM_KEY_LEFT)) gamepad_global.controller.left = true;
    if (platform_key_down(PLATFORM_KEY_RIGHT)) gamepad_global.controller.right = true;
    if (platform_key_down(PLATFORM_KEY_UP)) gamepad_global.controller.up = true;
    if (platform_key_down(PLATFORM_KEY_DOWN)) gamepad_global.controller.down = true;

    if (!platform_key_down(PLATFORM_KEY_LEFT)) gamepad_global.controller.left = false;
    if (!platform_key_down(PLATFORM_KEY_RIGHT)) gamepad_global.controller.right = false;
    if (!platform_key_down(PLATFORM_KEY_UP)) gamepad_global.controller.up = false;
    if (!platform_key_down(PLATFORM_KEY_DOWN)) gamepad_global.controller.down = false;
}

int main(int argc, char* argv[]) {
    cart_open(argc == 2 ? argv[1] : "C:/users/vince/downloads/pokemon_blue.gb");
    cpu_init();
    ppu_init();
    timer_init();
    bus_init();

    // Fake bios
    {
        // Turning on double speed here fixes glitches in pokemon
        SET_AF(0x01B0);
        SET_BC(0x0000);
        SET_DE(0xFF56);
        SET_HL(0x000D);
        cpu_global->sp = 0xDFF0;
        cpu_global->pc = 0x0100;

        bus_write(0xFF40, 0x91); // LCDC ON
        bus_write(0xFF41, 0x00); // STAT
        bus_write(0xFF42, 0x00); // SCY
        bus_write(0xFF43, 0x00); // SCX
        bus_write(0xFF44, 0x00); // LY
        bus_write(0xFF40, 0x00); // LCD OFF
        bus_write(0xFF40, 0x91); // LCD ON


        cpu_global->IE = 0;
        cpu_global->IF = 0;

        memset(bus_global->wram, 0, 0x2000);
        memset(bus_global->hram, 0, 0x80);
    }

    platform_init();

    wnd = platform_create_window("DMG Gameboy Emulator", PPU_XRES * 4, PPU_YRES * 4, false, PPU_XRES, PPU_YRES);
    if (!wnd) { printf("Failed to create main window!\n"); exit(1); }

    tile_wnd = platform_create_window("PPU Tiles", 16 * 8 * 4, 24 * 8 * 4, false, 16 * 8, 24 * 8);
    if (!tile_wnd) { printf("Failed to create tile viewer!\n"); exit(1); }

    platform_set_vsync(true);

    render();

    u64 prev_frame = 0;

    while (!platform_window_should_close(wnd) &&
        !platform_window_should_close(tile_wnd)) {
        input();
        //platform_poll_events();
        bus_step();

        if (prev_frame != ppu_global->current_frame) {
            platform_poll_events();
            render();
        }

        if (platform_key_down(PLATFORM_KEY_S) && cart_global.need_save) {
            char filename[1048];
            sprintf(filename, "%s.save\000", (const char*)&cart_global.title[0]);
            cart_battery_save(filename); // Save game
            while (platform_key_down(PLATFORM_KEY_S)) { platform_poll_events(); }
        }

        if (platform_key_down(PLATFORM_KEY_L)) {
            char filename[1048];
            sprintf(filename, "%s.save\000", (const char*)&cart_global.title[0]);
            cart_battery_load(filename); // Load game
            while (platform_key_down(PLATFORM_KEY_L)) { platform_poll_events(); }
        }

        prev_frame = ppu_global->current_frame;
    }
    
    lcd_unload();
    bus_unload();
    timer_unload();
    ppu_unload();
    cpu_unload();
    cart_unload();

    platform_destroy_window(wnd);
    platform_shutdown();

    return 0;
}