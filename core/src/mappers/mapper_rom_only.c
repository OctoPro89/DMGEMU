#include "mapper.h"
#include <stdlib.h>

typedef struct {
    mapper base;
} mapper_rom_only;

static u8 read(mapper* m, u16 addr) {
    return m->rom[addr];
}

static void write(mapper* m, u16 addr, u8 value) {
    (void)m; (void)addr; (void)value;
}

static void destroy(mapper* m) {
    free(m);
}

mapper* mapper_rom_only_create(u8* rom, size_t size) {
    mapper_rom_only* m = (mapper_rom_only*)calloc(1, sizeof(*m));
    m->base.rom = rom;
    m->base.rom_size = size;
    m->base.read = read;
    m->base.write = write;
    m->base.destroy = destroy;
    return &m->base;
}