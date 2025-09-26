#include "timer.h"
#include "cpu.h"
#include "interrupts.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

timer* timer_global;

void timer_init() {
    timer_global = (timer*)malloc(sizeof(timer));
    memset(timer_global, 0, sizeof(timer));
    timer_global->div = 0xAC00;
}

void timer_tick() {
    u16 prev_div = timer_global->div;

    ++timer_global->div;
    bool timer_update = false;

    switch (timer_global->tac & (0b11)) {
        case 0b00:
            timer_update = (prev_div & (1 << 9)) && (!(timer_global->div & (1 << 9)));
            break;
        case 0b01:
            timer_update = (prev_div & (1 << 3)) && (!(timer_global->div & (1 << 3)));
            break;
        case 0b10:
            timer_update = (prev_div & (1 << 5)) && (!(timer_global->div & (1 << 5)));
            break;
        case 0b11:
            timer_update = (prev_div & (1 << 7)) && (!(timer_global->div & (1 << 7)));
            break;
    }

    if (timer_update && timer_global->tac & (1 << 2)) {
        ++timer_global->tima;

        if (timer_global->tima == 0xFF) {
            timer_global->tima = timer_global->tma;
            cpu_global->IF |= INT_TIMER;
        }
    }
}

void timer_write(u16 address, u8 value) {
    switch (address) {
    case 0xFF04:
        //DIV
        timer_global->div = 0;
        break;

    case 0xFF05:
        //TIMA
        timer_global->tima = value;
        break;

    case 0xFF06:
        //TMA
        timer_global->tma = value;
        break;

    case 0xFF07:
        //TAC
        timer_global->tac = value;
        break;
    }
}

u8 timer_read(u16 address) {
    switch (address) {
        case 0xFF04:
            return timer_global->div >> 8;
        case 0xFF05:
            return timer_global->tima;
        case 0xFF06:
            return timer_global->tma;
        case 0xFF07:
            return timer_global->tac;
    }

    printf("DISSALOWED timer_read() ADDRESS!\n");
    exit(1);
    return 0;
}