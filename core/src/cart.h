#pragma once
#include "common.h"
#include "mappers/mapper.h"

typedef struct {
    u8 title[16];
    u8 type;

    mapper* mapper;

    bool battery;
    bool need_save;
} cart;

void cart_open(const char* filepath);
void cart_unload(void);

u8 cart_read(u16 addr);
void cart_write(u16 addr, u8 value);

void cart_battery_save(const char* filepath);
void cart_battery_load(const char* filepath);

extern cart cart_global;