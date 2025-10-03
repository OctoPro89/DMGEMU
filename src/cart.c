#include "cart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

cart cart_global;

const u32 GB_HEADER_SIZE = 0x14D - 0x104;

const u8 NINTENDO_LOGO_BYTE_ARRAY[] = {
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

const char* NEW_LICENSEE_CODES[] = {
    "None",
    "Nintendo Research & Development",
    "Capcom",
    "EA Electronic Arts",
    "Hudson Soft",
    "B-AI",
    "KSS",
    "Planning Office WADA",
    "PCM Complete",
    "San-X",
    "Kemco",
    "SETA Corporation",
    "Viacom",
    "Nintendo",
    "Bandai",
    "Ocean Software/Accaim Entertainment",
    "Konami",
    "HectorSoft",
    "Taito",
    "Hudson Soft",
    "Banpresto",
    "Ubi Soft",
    "Atlus",
    "Malibu Interactive",
    "Angel",
    "BulletProof Software",
    "IRem",
    "Absolute",
    "Acclaim Entertainment",
    "Activision",
    "Sammy USA Corporation",
    "Konami",
    "Hi Tech Expressions",
    "LJN",
    "Matchbox",
    "Mattel",
    "Milton Bradley Company",
    "Titus Interactive",
    "Virgin Games Ltd",
    "Lucasfilm Games",
    "Ocean Software",
    "EA Electronic Arts",
    "Infogames",
    "Interplay Entertainment",
    "Broderbund",
    "Sculptured Software",
    "The Sales Curve Limited",
    "THQ",
    "Accolade",
    "Misawa Entertainment",
    "lozc",
    "Tokuma Shoten",
    "Tsukuda Origina",
    "Chunsoft Co",
    "Video System",
    "Ocean Software/Acclaim Entertainment",
    "Varie",
    "Yonezawa",
    "Kaneko",
    "Pack-In-Video",
    "Bottom Up",
    "Konami",
    "MTO",
    "Kodansha"
};

const char* NEW_LICENSEE_CODE_KEYS[] = {
    "00",
    "01", 
    "08", 
    "13", 
    "18", 
    "19", 
    "20", 
    "22", 
    "24", 
    "25", 
    "28", 
    "29", 
    "30", 
    "31", 
    "32", 
    "33", 
    "34", 
    "35", 
    "37", 
    "38", 
    "39", 
    "41", 
    "42", 
    "44", 
    "46", 
    "47", 
    "49", 
    "50", 
    "51", 
    "52", 
    "53", 
    "54", 
    "55", 
    "56", 
    "57", 
    "58", 
    "59", 
    "60", 
    "61", 
    "64", 
    "67", 
    "69", 
    "70", 
    "71", 
    "72", 
    "73", 
    "75", 
    "78", 
    "79", 
    "80", 
    "83", 
    "86", 
    "87", 
    "91", 
    "92", 
    "93", 
    "95", 
    "96", 
    "97", 
    "99", 
    "9H", 
    "A4", 
    "BL", 
    "DK"
};

const char* CARTRIDGE_TYPES[] = {
    "ROM ONLY",
    "MBC1",
    "MBC1 + RAM",
    "MBC1 + RAM + BATTERY",
    "MBC2",
    "MBC2 + BATTERY",
    "ROM + RAM",
    "ROM + RAM + BATTERY",
    "MMM01",
    "MMM01 + RAM",
    "MMM01 + RAM + BATTERY",
    "MBC3 + TIMER + BATTERY",
    "MBC3 + TIMER + RAM + BATTERY",
    "MBC3",
    "MBC3 + RAM",
    "MBC3 + RAM + BATTERY",
    "MBC5",
    "MBC5 + RAM",
    "MBC5 + RAM + BATTERY",
    "MBC5 + RUMBLE",
    "MBC5 + RUMBLE + RAM",
    "MBC5 + RUMBLE + RAM + BATTERY",
    "MBC6",
    "MBC7 + SENSOR + RUMBLE + RAM + BATTERY",
    "POCKET CAMERA",
    "BANDAI TAMA5",
    "HuC3",
    "HuC1 + RAM + BATTERY"
};

const u8 CARTRIDGE_TYPE_KEYS[] = {
    0x00,
    0x01,
    0x02,
    0x03,
    0x05,
    0x06,
    0x08,
    0x09,
    0x0B,
    0x0C,
    0x0D,
    0x0F,
    0x10,
    0x11,
    0x12,
    0x13,
    0x19,
    0x1A,
    0x1B,
    0x1C,
    0x1D,
    0x1E,
    0x20,
    0x22,
    0xFC,
    0xFD,
    0xFE,
    0xFF
};

static u8* read_file(const char* filepath, size_t* out_size) {
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        printf("Failed to open file!\n");
        return NULL;
    }

    // Seek to end to determine file size
    fseek(f, 0, SEEK_END);
    size_t filesize = ftell(f);
    rewind(f); // go back to start

    // Allocate buffer
    u8* buffer = malloc(filesize);
    if (!buffer) {
        printf("Failed to allocate memory!\n");
        fclose(f);
        return NULL;
    }

    // Read entire file
    size_t read_bytes = fread(buffer, 1, filesize, f);
    if (read_bytes != filesize) {
        printf("Failed to read entire file! Only read %zu bytes\n", read_bytes);
        free(buffer);
        fclose(f);
        return NULL;
    }

    fclose(f);

    if (out_size) {
        *out_size = filesize;
    }

    return buffer;
}

static INLINE bool mbc1() {
    return BETWEEN(cart_global.type, 1, 3);
}

static INLINE bool battery() {
    return cart_global.type == 3;
}

void cart_open(const char* filepath) {
    memset(&cart_global, 0, sizeof(cart));
    cart_global.rom = read_file(filepath, &cart_global.rom_size);
    if (!cart_global.rom) {
        printf("cart_open(): Failed to read file!\n");
        free(cart_global.rom);
        exit(1);
    }

    u32 offset = 0x104;

    // -- NINTENDO LOGO --
    for (u32 i = 0; i < 48; ++i) {
        if (cart_global.rom[offset] != NINTENDO_LOGO_BYTE_ARRAY[i]) {
            printf("cart_open(): Failed to find Nintendo logo at address $0104-$0133!\n");
            free(cart_global.rom);
            exit(1);
        }

        ++offset;
    }

    printf("Successfully found Nintendo Logo at address $0104-$0133\n");

    // Apparently 0x13F-0x142 are the manufacturer code, but they are not useful / used anywhere

    // -- TITLE --
    for (u8 i = 0; i < 16; ++i) {
        cart_global.title[i] = cart_global.rom[offset];
        ++offset;
    }
    cart_global.title[15] = '\0';

    printf("Game title: %s\n", (const char*)&cart_global.title[0]);

    // -- CGB FLAG --
    offset = 0x143;
    u8 CGBFlag = cart_global.rom[offset];
    const char* meaning_str = CGBFlag == 0x80 ? "Game support CGB enhancements, but is backwards compatible" : (CGBFlag == 0xC0 ? "Game support CGB only" : "Unknown CGB Flag");
    printf("CGB Flag: %u [%s]\n", CGBFlag, meaning_str);

    offset = 0x144;

    // -- NEW LICENSEE CODE --
    const char licenseeCode[3] = { cart_global.rom[offset], cart_global.rom[offset + 1], '\0' };
    ++offset;

    int licensee_index = -1;
    for (u32 i = 0; i < 64; ++i)
    {
        if (strcmp(NEW_LICENSEE_CODE_KEYS[i], (const char*)&licenseeCode[0]) == 0)
        {
            licensee_index = i;
        }
    }

    const char* licensee = licensee_index == -1 ? "Licensee not found" : NEW_LICENSEE_CODES[licensee_index];
    printf("Licensee code: %s, Licensee: %s\n", (const char*)&licenseeCode[0], licensee);

    // -- SGB FLAG --
    offset = 0x146;
    bool SGBFlag = cart_global.rom[offset] == 0x03;
    meaning_str = SGBFlag == true ? "Game supports SGB functions" : "Game doesn't support SGB functions";
    printf("SGB Flag: %s [%s]\n", meaning_str, SGBFlag ? "true" : "false");

    // -- CARTRIDGE TYPE --
    offset = 0x147;
    int cart_type_index = -1;
    for (u8 i = 0; i < 28; ++i)
    {
        if (CARTRIDGE_TYPE_KEYS[i] == cart_global.rom[offset])
        {
            cart_type_index = i;
        }
    }

    const char* cart_type = cart_type_index == -1 ? "Cartridge type not found" : CARTRIDGE_TYPES[cart_type_index];
    printf("Cartridge type: %s\n", cart_type);

    offset = 0x148;
    u8 rom_size_value = cart_global.rom[offset];
    u8 rom_size_kib = 32 * (1 << rom_size_value);
    u8 rom_bank_count = (u8)((rom_size_kib * 1024) / 16384);

    printf("ROM Size Value: %u, ROM Size (KiB): %u, ROM Bank Count: %u\n", rom_size_value, rom_size_kib, rom_bank_count);

    offset = 0x149;
    u8 ram_size_code = cart_global.rom[offset];
    u32 ram_size = 0x00;
    const char* ram_description = NULL;
    switch (ram_size_code)
    {
        case 0x02:
        {
            ram_size = 8192;
            ram_description = "1 bank";
            break;
        }
        case 0x03:
        {
            ram_size = 32768;
            ram_description = "4 banks of 8 KiB each";
            break;
        }
        case 0x04:
        {
            ram_size = 131072;
            ram_description = "16 banks of 8 KiB each";
            break;
        }
        case 0x05:
        {
            ram_size = 65536;
            ram_description = "8 banks of 8 KiB each";
            break;
        }
        default:
        {
            ram_size = 0;
            ram_description = "Unused or no RAM";
            break;
        }
    }

    printf("RAM Size: %u [%s]\n", ram_size, ram_description);

    offset = 0x14A;
    const char* dest_code = (cart_global.rom[offset] == 0x00 ? "Japan" : cart_global.rom[offset] == 0x01 ? "Overseas" : "Unknown");
    printf("Destination code: %s\n", dest_code);

    offset = 0x14B;
    u8 old_licensee_code = cart_global.rom[offset];
    printf("Old Licensee code (mostly irrelevant): %u\n", old_licensee_code);

    offset = 0x14C;
    u8 mask_rom_ver_num = cart_global.rom[offset];
    printf("Mask ROM version number: %u\n", mask_rom_ver_num);

    u32 checksum = 0;
    for (u32 address = 0x134; address <= 0x14C; ++address) {
        checksum = checksum - cart_global.rom[address] - 1;
    }

    offset = 0x14D;
    bool checksum_passed = cart_global.rom[offset] == (checksum & 0xFF);

    printf("Checksum %s\n", checksum_passed ? "passed" : "failed");

    for (u8 i = 0; i < 16; ++i) {
        cart_global.ram_banks[i] = 0;
        if ((ram_size_code == 2 && i == 0) || (ram_size_code == 3 && i < 4) || (ram_size_code == 4 && i < 16) || (ram_size_code == 5 && i < 8)) {
            cart_global.ram_banks[i] = malloc(sizeof(u8) * 0x2000);
            memset(cart_global.ram_banks[i], 0, 0x2000);
        }
    }

    cart_global.ram_bank = cart_global.ram_banks[0];
    cart_global.rom_bank_x = cart_global.rom + 0x4000;

    cart_global.type = (u8)cart_type_index;
    cart_global.battery = battery();
    cart_global.need_save = false;
    cart_global.rom = cart_global.rom;

    if (cart_global.battery) {
        // load_battery();
    }
}

void cart_unload() {
    if (cart_global.rom) {
        free(cart_global.rom);
    }

    for (u8 i = 0; i < 16; ++i) {
        if (cart_global.ram_banks[i]) free(cart_global.ram_banks[i]);
    }
}

u8 cart_read(u16 address) {
    if (!mbc1() || address < 0x4000) {
        return cart_global.rom[address];
    }

    if ((address & 0xE000) == 0xA000) {
        if (!cart_global.ram_enabled) {
            return 0xFF;
        }

        if (!cart_global.ram_bank) {
            return 0xFF;
        }

        return cart_global.ram_bank[address - 0xA000];
    }

    return cart_global.rom_bank_x[address - 0x4000];
}

void cart_write(u16 address, u8 value) {
    if (!mbc1()) {
        return;
    }

    if (address < 0x2000) {
        cart_global.ram_enabled = ((value & 0xF) == 0xA);
    }

    if ((address & 0xE000) == 0x2000) {
        // ROM bank number
        if (value == 0) {
            value = 1;
        }

        value &= 0b11111;

        cart_global.rom_bank_value = value;
        cart_global.rom_bank_x = cart_global.rom + (0x4000 * cart_global.rom_bank_value);
    }

    if ((address & 0xE000) == 0x4000) {
        // RAM bank number
        cart_global.ram_bank_value = value & 0b11;

        if (cart_global.ram_banking) {
            cart_global.ram_bank = cart_global.ram_banks[cart_global.ram_bank_value];
        }
    }

    if ((address & 0xE000) == 0x6000) {
        // Banking mode select
        cart_global.banking_mode = value & 1;

        cart_global.ram_banking = cart_global.banking_mode;

        if (cart_global.ram_banking) {
            cart_global.ram_bank = cart_global.ram_banks[cart_global.ram_bank_value];
        }
    }

    if ((address & 0xE000) == 0xA000) {
        if (!cart_global.ram_enabled) {
            return;
        }

        if (!cart_global.ram_bank) {
            return;
        }

        cart_global.ram_bank[address - 0xA000] = value;

        if (cart_global.battery) {
            cart_global.need_save = true;
        }
    }
}

void cart_battery_save(const char* filepath) {
    if (!cart_global.ram_bank) return;

    char filename[1048];
    sprintf(filename, "%s.save\000", filepath);
    FILE* fp = fopen(filename, "wb");

    if (!fp) {
        printf("ERROR: Failed to save game to '%s'!\n", filepath);
        return;
    }

    fwrite(cart_global.ram_bank, 0x2000, 1, fp);
    printf("Saved game to save file: '%s'\n", filepath);
    fclose(fp);
}

void cart_battery_load(const char* filepath) {
    if (!cart_global.ram_bank) return;

    char filename[1048];
    sprintf(filename, "%s.save\000", filepath);
    FILE* fp = fopen(filename, "rb");

    if (!fp) {
        printf("ERROR: Failed to load game from '%s'!\n", filepath);
        return;
    }

    fread(cart_global.ram_bank, 0x2000, 1, fp);
    printf("Loaded game from save file: '%s'\n", filepath);
    fclose(fp);
}