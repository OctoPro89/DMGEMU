#include "ppu.h"
#include "lcd.h"
#include "platform.h"
#include "interrupts.h"
#include "bus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static INLINE bool window_active_on_line() {
    return LCDC_WINDOW_ENABLE &&
        lcd_global->ly >= lcd_global->win_y &&
        lcd_global->win_x >= 7;
}

static INLINE void inc_ly() {
    ++lcd_global->ly;

    if (lcd_global->ly == lcd_global->ly_compare) {
        LCDS_LYC_SET(1);

        if (LCDS_STAT_INT(SS_LYC)) {
            cpu_global->IF |= INT_LCD_STAT;
        }
    }
    else {
        LCDS_LYC_SET(0);
    }
}

// -- PIPELINE --

static void pixel_fifo_push(u8 color) {
    fifo* q = &ppu_global->pf.pixel_fifo;

    if (q->size >= FIFO_CAPACITY) {
#if PPU_DEBUG
        printf("FIFO overflow!\n");
        exit(1);
#endif
        return;
    }

    q->buffer[q->tail] = color;
    q->tail = (q->tail + 1) % FIFO_CAPACITY;
    q->size++;
}

static u8 pixel_fifo_pop() {
    fifo* q = &ppu_global->pf.pixel_fifo;

#if PPU_DEBUG
    if (q->size == 0) {
        printf("ERROR IN PIXEL FIFO!\n");
        exit(1);
    }
#endif

    u8 color = q->buffer[q->head];
    q->head = (q->head + 1) % FIFO_CAPACITY;
    q->size--;
    return color;
}

static u8 fetch_sprite_pixels(int bit, u8 color, u8 bg_color) {
    for (u8 i = 0; i < ppu_global->fetched_entry_count; ++i) {
        u8 sp_x = (ppu_global->fetched_entries[i].x - 8) + ((lcd_global->scroll_x % 8));

        if (sp_x + 8 < ppu_global->pf.fifo_x) {
            // Past pixel point already
            continue;
        }

        int offset = ppu_global->pf.fifo_x - sp_x;

        if (offset < 0 || offset > 7) {
            // Out of bounds
            continue;
        }

        bit = (7 - offset);

        if (ppu_global->fetched_entries[i].flags.XFLIP) {
            bit = offset;
        }

        u8 hi = !!(ppu_global->pf.fetch_entry_data[i * 2] & (1 << bit));
        u8 lo = !!(ppu_global->pf.fetch_entry_data[(i * 2) + 1] & (1 << bit)) << 1;

        bool bg_priority = ppu_global->fetched_entries[i].flags.PRIORITY;

        if (!(hi | lo)) {
            // Transparent
            continue;
        }

        if (!bg_priority || bg_color == 0) {
            color = (ppu_global->fetched_entries[i].flags.DMG_PALLETE_NUMBER) ? lcd_global->sp2_colors[hi | lo] : lcd_global->sp1_colors[hi | lo];

            if (hi | lo) break;
        }
    }

    return color;
}

static bool pipeline_fifo_add() {
    if (ppu_global->pf.pixel_fifo.size >= FIFO_CAPACITY) {
        // Fifo is full
        return false;
    }

    int x = ppu_global->pf.fetch_x - (8 - (lcd_global->scroll_x % 8));

    for (int i = 0; i < 8; ++i) {
        int bit = 7 - i;
        u8 hi = !!(ppu_global->pf.bgw_fetch_data[1] & (1 << bit));
        u8 lo = !!(ppu_global->pf.bgw_fetch_data[2] & (1 << bit)) << 1;
        u8 color = lcd_global->bg_colors[hi | lo];

        if (!LCDC_BGW_ENABLE) {
            color = lcd_global->bg_colors[0];
        }

        if (LCDC_OBJ_ENABLE) {
            color = fetch_sprite_pixels(bit, color, hi | lo);
        }

        if (x >= 0) {
            pixel_fifo_push(color);
            ++ppu_global->pf.fifo_x;
        }
    }

    return true;
}

static void pipeline_load_sprite_tile() {
    oam_line_entry* le = ppu_global->line_sprites;

    while (le) {
        int sp_x = (le->entry.x - 8) + (lcd_global->scroll_x % 8);

        if ((sp_x >= ppu_global->pf.fetch_x && sp_x < ppu_global->pf.fetch_x + 8) || ((sp_x + 8) >= ppu_global->pf.fetch_x && (sp_x + 8) < ppu_global->pf.fetch_x + 8)) {
            // Need to add entry
            ppu_global->fetched_entries[ppu_global->fetched_entry_count++] = le->entry;
        }

        le = le->next;

        if (!le || ppu_global->fetched_entry_count >= 3) {
            // Max checking 3 sprites on pixels
            break;
        }
    }
}

static void pipeline_load_sprite_data(u8 offset) {
    u8 cur_y = lcd_global->ly;
    u8 sprite_height = LCDC_OBJ_HEIGHT;

    for (u8 i = 0; i < ppu_global->fetched_entry_count; ++i) {
        u8 ty = ((cur_y + 16) - ppu_global->fetched_entries[i].y) * 2;

        if (ppu_global->fetched_entries[i].flags.YFLIP) {
            // Flipped upside down
            ty = ((sprite_height * 2) - 2) - ty;
        }

        u8 tile_index = ppu_global->fetched_entries[i].tile_index;

        if (sprite_height == 16) {
            tile_index &= ~(1); // Remove last bit
        }

        ppu_global->pf.fetch_entry_data[(i * 2) + offset] = bus_read(0x8000 + (tile_index * 16) + ty + offset);
    }
}

static void pipeline_load_window_tile() {
    // Window must be enabled and active on this scanline
    if (!window_active_on_line()) return;

    int wx = lcd_global->win_x - 7;

    // Are we fetching pixels that belong to the window?
    if (ppu_global->pf.fetch_x + 7 < wx) return;

    // Window X/Y in tile space
    int win_x = (ppu_global->pf.fetch_x + 7) - wx;
    int win_tile_x = win_x / 8;
    int win_tile_y = ppu_global->window_line / 8;

    // Fetch tile index from window tile map
    u16 tile_map_addr =
        LCDC_WINDOW_TILE_MAP_AREA +
        (win_tile_y * 32) +
        win_tile_x;

    u8 tile = bus_read(tile_map_addr);

    // Handle signed tile indices if using 0x8800
    if (LCDC_BG_WINDOW_TILES == 0x8800) {
        tile += 128;
    }

    ppu_global->pf.bgw_fetch_data[0] = tile;
}

static void pipeline_fetch() {
    switch (ppu_global->pf.current_fetch_state) {
    case FETCH_STATE_TILE: {
        ppu_global->fetched_entry_count = 0;

        if (LCDC_BGW_ENABLE) {
            ppu_global->pf.bgw_fetch_data[0] = bus_read(LCDC_BG_TILE_MAP + (ppu_global->pf.map_x / 8) + (((ppu_global->pf.map_y / 8)) * 32));

            if (LCDC_BG_WINDOW_TILES == 0x8800) {
                ppu_global->pf.bgw_fetch_data[0] += 128;
            }

            pipeline_load_window_tile();
        }

        if (LCDC_OBJ_ENABLE && ppu_global->line_sprites) {
            pipeline_load_sprite_tile();
        }

        ppu_global->pf.current_fetch_state = FETCH_STATE_DATA0;
        ppu_global->pf.fetch_x += 8;
        break;
    }
    case FETCH_STATE_DATA0: {
        ppu_global->pf.bgw_fetch_data[1] = bus_read(LCDC_BG_WINDOW_TILES + (ppu_global->pf.bgw_fetch_data[0] * 16) + ppu_global->pf.tile_y);
        pipeline_load_sprite_data(0);
        ppu_global->pf.current_fetch_state = FETCH_STATE_DATA1;
        break;
    }
    case FETCH_STATE_DATA1: {
        ppu_global->pf.bgw_fetch_data[2] = bus_read(LCDC_BG_WINDOW_TILES + (ppu_global->pf.bgw_fetch_data[0] * 16) + ppu_global->pf.tile_y + 1);
        pipeline_load_sprite_data(1);
        ppu_global->pf.current_fetch_state = FETCH_STATE_IDLE;
        break;
    }
    case FETCH_STATE_IDLE: {
        ppu_global->pf.current_fetch_state = FETCH_STATE_PUSH;
        break;
    }
    case FETCH_STATE_PUSH: {
        if (pipeline_fifo_add()) {
            ppu_global->pf.current_fetch_state = FETCH_STATE_TILE;
        }
        break;
    }
    }
}

static void pipeline_push_pixel() {
    if (ppu_global->pf.pixel_fifo.size > 8) {
        u8 pixel_data = pixel_fifo_pop();
        int screen_x = ppu_global->pf.line_x;
        int wx = lcd_global->win_x - 7;
        bool window_active = window_active_on_line();

        // Skip pixels until the window starts
        if (window_active && !ppu_global->window_drawn_this_line && screen_x < wx) {
            ++ppu_global->pf.line_x;
            return;
        }

        // Skip pixels until BG scroll offset
        if (!window_active && screen_x < (lcd_global->scroll_x % 8)) {
            ++ppu_global->pf.line_x;
            return;
        }

        // Push pixel to framebuffer
        ppu_global->video_buffer[
            ppu_global->pf.pushed_x +
                (lcd_global->ly * PPU_XRES)
        ] = pixel_data;

        ++ppu_global->pf.pushed_x;
        ++ppu_global->pf.line_x;

        // Mark window drawn
        if (window_active && screen_x >= wx) {
            ppu_global->window_drawn_this_line = true;
        }
    }
}

static void pipeline_process() {
    bool using_window = false;

    if (window_active_on_line()) {
        int wx = lcd_global->win_x - 7;
        if (ppu_global->pf.fetch_x >= wx) {
            using_window = true;
        }
    }

    if (using_window) {
        // Window ignores scroll registers
        ppu_global->pf.map_x = ppu_global->pf.fetch_x - (lcd_global->win_x - 7);
        ppu_global->pf.map_y = ppu_global->window_line;
        ppu_global->pf.tile_y = (ppu_global->window_line % 8) * 2;
    }
    else {
        // Normal BG behavior
        ppu_global->pf.map_x = ppu_global->pf.fetch_x + lcd_global->scroll_x;
        ppu_global->pf.map_y = lcd_global->ly + lcd_global->scroll_y;
        ppu_global->pf.tile_y = ((lcd_global->ly + lcd_global->scroll_y) % 8) * 2;
    }

    if (!(ppu_global->line_ticks & 1)) {
        pipeline_fetch();
    }

    pipeline_push_pixel();
}

static void pipeline_fifo_reset() {
    ppu_global->pf.pixel_fifo.size = 0;
    ppu_global->pf.pixel_fifo.head = 0;
    ppu_global->pf.pixel_fifo.tail = 0;
}

// -- PIPELINE --

// -- STATE MACHINE --

void load_line_sprites() {
    u8 cur_y = lcd_global->ly;
    u8 sprite_height = LCDC_OBJ_HEIGHT;
    memset(&ppu_global->line_entry_array[0], 0, sizeof(oam_line_entry) * 10);

    for (int i = 0; i < 40; ++i) {
        oam e = ppu_global->oam_ram[i];

        if (!e.x) {
            // x = 0 | Not visible
            continue;
        }

        if (ppu_global->line_sprite_count >= 10) {
            // Max 10 sprites per line
            break;
        }

        if (e.y <= cur_y + 16 && e.y + sprite_height > cur_y + 16) {
            // Sprite is on the current line
            oam_line_entry* entry = &ppu_global->line_entry_array[ppu_global->line_sprite_count++];

            entry->entry = e;
            entry->next = NULL;

            if (!ppu_global->line_sprites || ppu_global->line_sprites->entry.x > e.x) {
                entry->next = ppu_global->line_sprites;
                ppu_global->line_sprites = entry;
                continue;
            }

            // Sort sprites
            oam_line_entry* le = ppu_global->line_sprites;
            oam_line_entry* prev = le;

            while (le) {
                if (le->entry.x > e.x) {
                    prev->next = entry;
                    entry->next = le;
                    break;
                }

                if (!le->next) {
                    le->next = entry;
                    break;
                }

                prev = le;
                le = le->next;
            }
        }
    }
}

static void ppu_mode_oam() {
    if (ppu_global->line_ticks >= 80) {
        LCDS_MODE_SET(MODE_TRANSFER);

        ppu_global->pf.current_fetch_state = FETCH_STATE_TILE;
        ppu_global->pf.line_x = 0;
        ppu_global->pf.fetch_x = 0;
        ppu_global->pf.pushed_x = 0;
        ppu_global->pf.fifo_x = 0;
    }

    if (ppu_global->line_ticks == 1) {
        // Read OAM on the first tick only
        ppu_global->line_sprites = 0;
        ppu_global->line_sprite_count = 0;

        load_line_sprites();
    }
}

static void ppu_mode_transfer() {
    pipeline_process();

    if (ppu_global->pf.pushed_x >= PPU_XRES) {
        if (ppu_global->window_drawn_this_line) {
            ppu_global->window_line++;
        }

        ppu_global->window_drawn_this_line = false;

        pipeline_fifo_reset();
        LCDS_MODE_SET(MODE_HBLANK);

        if (LCDS_STAT_INT(SS_HBLANK)) {
            cpu_global->IF |= INT_LCD_STAT;
        }
    }
}

static void ppu_mode_vblank() {
    if (ppu_global->line_ticks >= TICKS_PER_LINE) {
        inc_ly();

        if (lcd_global->ly >= LINES_PER_FRAME) {
            LCDS_MODE_SET(MODE_OAM);
            lcd_global->ly = 0;
            ppu_global->window_line = 0;
        }

        ppu_global->line_ticks = 0;
    }
}

static void ppu_mode_hblank() {
    if (ppu_global->line_ticks >= TICKS_PER_LINE) {
        inc_ly();

        if (lcd_global->ly >= PPU_YRES) {
            LCDS_MODE_SET(MODE_VBLANK);
            cpu_global->IF |= INT_VBLANK;

            if (LCDS_STAT_INT(SS_VBLANK)) cpu_global->IF |= INT_LCD_STAT;

            ++ppu_global->current_frame;
        }
        else {
            LCDS_MODE_SET(MODE_OAM);
        }

        ppu_global->line_ticks = 0;
    }
}

// -- STATE MACHINE --

ppu* ppu_global;

void ppu_init() {
    ppu_global = malloc(sizeof(ppu));
    memset(ppu_global, 0, sizeof(ppu));
    ppu_global->vram = malloc(sizeof(u8) * 0x2000);
    memset(&ppu_global->vram[0], 0, sizeof(u8) * 0x2000);
    ppu_global->video_buffer = malloc(sizeof(u8) * PPU_XRES * PPU_YRES);
    memset(&ppu_global->video_buffer[0], 0, sizeof(u8) * PPU_XRES * PPU_YRES);

    ppu_global->pf.line_x = 0;
    ppu_global->pf.pushed_x = 0;
    ppu_global->pf.fetch_x = 0;
    ppu_global->pf.pixel_fifo.size = 0;
    ppu_global->pf.pixel_fifo.head = 0;
    ppu_global->pf.pixel_fifo.tail = 0;
    ppu_global->pf.current_fetch_state = FETCH_STATE_TILE;
    ppu_global->line_sprites = 0;
    ppu_global->fetched_entry_count = 0;
    ppu_global->window_line = 0;

    lcd_init();
    LCDS_MODE_SET(MODE_OAM);
}

void ppu_unload() {
    if (ppu_global) {
        if (ppu_global->vram) free(ppu_global->vram);
        if (ppu_global->video_buffer) free(ppu_global->video_buffer);
        free(ppu_global);
    }
}

void ppu_tick() {
    ++ppu_global->line_ticks;

    switch (LCDS_MODE) {
    case MODE_OAM:
        ppu_mode_oam();
        break;
    case MODE_TRANSFER:
        ppu_mode_transfer();
        break;
    case MODE_VBLANK:
        ppu_mode_vblank();
        break;
    case MODE_HBLANK:
        ppu_mode_hblank();
        break;
    }
}

void ppu_write_oam(u16 addr, u8 value) {
    if (addr >= 0xFE00) {
        addr -= 0xFE00;
    }

    u8* oram = (u8*)ppu_global->oam_ram;
    oram[addr] = value;
}

u8 ppu_read_oam(u16 addr) {
    if (addr >= 0xFE00) {
        addr -= 0xFE00;
    }

    u8* oram = (u8*)ppu_global->oam_ram;
    return oram[addr];
}

void ppu_write_vram(u16 addr, u8 value) {
    ppu_global->vram[addr - 0x8000] = value;
}

u8 ppu_read_vram(u16 addr) {
    return ppu_global->vram[addr - 0x8000];
}