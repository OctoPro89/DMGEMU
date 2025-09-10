function log(msg)
{
    const e = document.createElement("h4");
    e.textContent = msg;
    document.body.appendChild(e);
}

const binaryString = atob(POKEMON_YELLOW_BASE64);

const len = binaryString.length;
const uint8Array = new Uint8Array(len);

for (let i = 0; i < len; i++) {
    uint8Array[i] = binaryString.charCodeAt(i); // Get the Unicode value of each character (which corresponds to the byte value)
}

const NINTENDO_LOGO_BYTE_ARRAY = new Uint8Array([
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
]);

const NEW_LICENSEE_CODES = [
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
];

const NEW_LICENSEE_CODE_KEYS = [
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
];

const CARTRIDGE_TYPES = [
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
];

const CARTRIDGE_TYPE_KEYS = new Uint8Array([
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
]);

function loadCartFromBytes(u8Array)
{
    let offset = 0x104;

    // -- NINTENDO LOGO --
    for (let i = 0; i < NINTENDO_LOGO_BYTE_ARRAY.length; ++i)
    {
        if (u8Array[offset] != NINTENDO_LOGO_BYTE_ARRAY[i])
        {
            log("Failed to find Nintendo logo at address $0104-$0133!");
            return null;
        }

        offset++;
    }

    log("Successfully found Nintendo Logo at address $0104-$0133");

    // Apparently 0x13F-0x142 are the manufacturer code, but they are not useful / used anywhere

    // -- TITLE --
    let title = new Uint8Array(16);
    for (let i = 0; i < 16; ++i)
    {
        title[i] = u8Array[offset];
        offset++;
    }

    log(`Game title: ${new TextDecoder("utf-8").decode(title)}`);

    // In older cartridges only
    // -- CGB FLAG --
    offset = 0x143;
    const CGBFlag = u8Array[offset]; 
    let meaningStr = CGBFlag == 0x80 ? "Game support CGB enhancements, but is backwards compatible" : (CGBFlag == 0xC0 ? "Game support CGB only" : "Unknown CGB Flag");
    log(`[OLDER CARTRIDGES ONLY] CFB Flag: ${CGBFlag} (${meaningStr})`);

    offset = 0x144;

    // -- NEW LICENSEE CODE --
    const licenseeCodeStr = new Uint8Array([u8Array[offset], u8Array[++offset]]);
    const licenseeCode = new TextDecoder("utf-8").decode(licenseeCodeStr);
    const licenseeIndex = NEW_LICENSEE_CODE_KEYS.indexOf(licenseeCode);
    const licensee = licenseeIndex == -1 ? "Licensee not found" : NEW_LICENSEE_CODES[licenseeIndex];
    log(`Licensee code: ${licenseeCode}, Lincensee: ${licensee}`);


    // -- SGB FLAG --
    offset = 0x146;
    const SGBFlag = u8Array[offset] == 0x03;
    meaningStr = SGBFlag == true ? "Game supports SGB functions" : "Game doesn't support SGB functions";
    log(`SGB Flag: ${SGBFlag} (${meaningStr})`);

    // -- CARTRIDGE TYPE --
    offset = 0x147;
    const cartTypeIndex = CARTRIDGE_TYPE_KEYS.indexOf(u8Array[offset]);
    const cartType = cartTypeIndex == -1 ? "Cartridge type not found" : CARTRIDGE_TYPES[cartTypeIndex];
    log(`Cartridge type: ${cartType}`);

    offset = 0x148;
    const romSizeValue = u8Array[offset];
    const romSizeKiB = 32 * (1 << romSizeValue);
    const romBankCount = (romSizeKiB * 1024) / 16384; 

    log(`ROM Size Value: ${romSizeValue}, ROM Size (KiB): ${romSizeKiB}, ROM Bank Count: ${romBankCount}`);

    offset = 0x149;
    const ramSizeCode = u8Array[offset];
    let ramSize = 0x00;
    let ramString = "";
    switch (ramSizeCode)
    {
        case 0x02:
        {
            ramSize = 8192;
            ramString = "1 bank";
            break;
        }
        case 0x03:
        {
            ramSize = 32768;
            ramString = "4 banks of 8 KiB each";
            break;
        }
        case 0x04:
        {
            ramSize = 131072;
            ramString = "16 banks of 8 KiB each";
            break;
        }
        case 0x05:
        {
            ramSize = 65536;
            ramString = "8 banks of 8 KiB each";
            break;
        }
        default:
        {
            ramSize = 0;
            ramString = "Unused or no RAM";
            break;
        }
    }

    log(`RAM Size: ${ramSize}, (INFO: ${ramString})`);

    offset = 0x14A;
    const destCode = u8Array[offset] == 0x00 ? "Japan" : u8Array[offset] == 0x01 ? "Overseas" : "Unknown";
    log(`Destination code: ${destCode}`);

    offset = 0x14B;
    const oldLicenseeCode = u8Array[offset];
    log(`Old licensee code (mostly irrelevant): ${oldLicenseeCode}`);

    offset = 0x14C;
    const maskROMverNum = u8Array[offset];
    log(`Mask ROM version number: ${maskROMverNum}`);

    let checksumarr = new Uint8Array(1);
    for (let address = 0x134; address <= 0x14C; ++address)
    {
        checksumarr[0] = checksumarr[0] - u8Array[address] - 1;
    }

    offset = 0x14D;
    const checksumPassed = u8Array[offset] == (checksumarr[0] & 0xFF);

    if (!checksumPassed)
    {
        log("Checksum failed!");
    }
    else
    {
        log("Checksum passed");
    }
}

loadCartFromBytes(uint8Array);