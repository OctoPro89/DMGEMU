#include "cart.h"
#include "bus.h"
#include "platform.h"
#include "lcd.h"

static u8 tile_colors[4] = { 0xFF, 0xAA, 0x55, 0x00 };

static void ppu_display_tile(u16 addr, u16 tile_num, int x, int y) {
    i32 rx, ry;

    for (int tileY = 0; tileY < 16; tileY += 2) {
        u8 b1 = bus_read(addr + (tile_num * 16) + (u8)tileY);
        u8 b2 = bus_read(addr + (tile_num * 16) + (u8)tileY + 1);

        for (int bit = 7; bit >= 0; bit--) {
            u8 hi = !!(b1 & (1 << bit)) << 1;
            u8 lo = !!(b2 & (1 << bit));

            u8 color = hi | lo;

            rx = x + (7 - bit);
            ry = (y + 7 - (tileY / 2));

            platform_put_pixel(rx, ry, tile_colors[color], tile_colors[color], tile_colors[color]);
        }
    }
}

int main(int argc, char* argv[]) {
    size_t cart_size = 0;
    u8* cart = cart_open("C:/users/vince/downloads/01-special.gb", &cart_size);
    
    cpu_init(cart, cart_size);
    ppu_init();
    timer_init();
    bus_init();

    platform_open_window(1920, 1080);

    while (platform_should_run()) {
        platform_pump_messages();
        bus_step();

        u16 tile_num = 0;
        u32 x_draw = 0, y_draw = 0;
        u16 addr = 0x8000;

        // 384 tiles, 24 * 16
        for (int y = 0; y < 24; ++y) {
            for (int x = 0; x < 16; ++x) {
                int flipped_y = (23 - y) * 8;
                ppu_display_tile(addr, tile_num, x_draw, flipped_y);
                x_draw += 8;
                ++tile_num;
            }

            y_draw += 8;
            x_draw = 0;
        }

        platform_render();
    }
    
    lcd_unload();
    bus_unload();
    ppu_unload();
    cpu_unload();
    cart_unload(cart);

    return 0;
}