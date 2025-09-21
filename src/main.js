function main()
{
    let rom = loadCartFromBytes(uint8Array);
    let cpu = new CPU(rom);

    let i = 500;

    while (1 && i > 1) {
        cpu.cycle();
        --i;
    }
}

window.onerror = (e) => log(e);

main();