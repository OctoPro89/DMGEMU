#include "timer.h"
#include "cpu.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

timer timer_init() {
    timer t;
    memset(&t, 0, sizeof(timer));
    t.div = 0xAC00;
    return t;
}

void timer_tick(cpu* c, timer* t) {
    u16 prev_div = t->div;

    ++t->div;
    bool timer_update = false;

    switch (t->tac & (0b11)) {
        case 0b00:
            timer_update = (prev_div & (1 << 9)) && (!(t->div & (1 << 9)));
            break;
        case 0b01:
            timer_update = (prev_div & (1 << 3)) && (!(t->div & (1 << 3)));
            break;
        case 0b10:
            timer_update = (prev_div & (1 << 5)) && (!(t->div & (1 << 5)));
            break;
        case 0b11:
            timer_update = (prev_div & (1 << 7)) && (!(t->div & (1 << 7)));
            break;
    }

    if (timer_update && t->tac & (1 << 2)) {
        ++t->tima;

        if (t->tima == 0xFF) {
            t->tima = t->tma;
            c->IF |= 4;
        }
    }
}

void timer_write(timer* t, u16 address, u8 value) {
    switch (address) {
    case 0xFF04:
        //DIV
        t->div = 0;
        break;

    case 0xFF05:
        //TIMA
        t->tima = value;
        break;

    case 0xFF06:
        //TMA
        t->tma = value;
        break;

    case 0xFF07:
        //TAC
        t->tac = value;
        break;
    }
}

u8 timer_read(timer* t, u16 address) {
    switch (address) {
        case 0xFF04:
            return t->div >> 8;
        case 0xFF05:
            return t->tima;
        case 0xFF06:
            return t->tma;
        case 0xFF07:
            return t->tac;
    }

    printf("DISSALOWED timer_read() ADDRESS!\n");
    exit(1);
    return 0;
}