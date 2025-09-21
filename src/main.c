#include "cart.h"
#include "bus.h"

int main(int argc, char* argv[]) {
    size_t cart_size = 0;
    u8* cart = cart_open("C:\\Users\\vince\\Downloads\\02-interrupts.gb", &cart_size);
    
    cpu* CPU = cpu_init(cart, cart_size);
    ppu* PPU = ppu_init(CPU);
    timer TIMER = timer_init();
    bus b = bus_init(CPU, PPU, &TIMER);

    int a = 15 & 15;

    while (1) {
        bus_step(&b);
    }
    
    bus_unload(&b);
    ppu_unload(PPU);
    cpu_unload(CPU);
    cart_unload(cart);

    return 0;
}