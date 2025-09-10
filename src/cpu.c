#include "cpu.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

cpu cpu_init(u8* cart) {
    cpu _cpu;
    memset(&_cpu, 0, sizeof(cpu));

    // Start at 0x0100
    _cpu.pc = 0x0100;
}