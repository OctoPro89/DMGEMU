#include "cart.h"
#include "cpu.h"

int main(int argc, char* argv[]) {
    u8* cart = cart_open("pokemon_yellow.gb");
    
    cpu CPU = cpu_init(cart);
    
    cart_unload(cart);

    return 0;
}