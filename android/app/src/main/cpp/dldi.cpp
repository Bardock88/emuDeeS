#include "dldi.h"
#include "core.h"
#include "settings.h"

Dldi::~Dldi() {
    // Ensure the SD image is closed
    if (sdImage)
        fclose(sdImage);
}

void Dldi::patchRom(uint8_t *rom, size_t offset, size_t size) {
    // Scan the ROM for DLDI drivers and patch them if found
    for (size_t i = 0; i < size; i += 0x40) {
        // Check for the DLDI magic number
        if (U8TO32(rom, i) != 0xBF8DA5ED)
            next:
            continue;

        // Check for the DLDI magic string
        const char *str = " Chishm\0";
        for (size_t j = 0; j < 8; j++) {
            if (rom[i + 4 + j] != str[j])
                goto next;
        }

        // Patch the DLDI driver to use the HLE functions
        rom[i + 0x0F] = 0x0E;                     // Size of driver in terms of 1 << n (16KB)
        uint32_t address = U8TO32(rom, i + 0x40); // Address of driver
        U32TO8(rom, i + 0x64, 0x00000023);        // Feature flags (read, write, NDS slot)
        U32TO8(rom, i + 0x68, address + 0x80);    // Address of startup()
        U32TO8(rom, i + 0x6C, address + 0x84);    // Address of isInserted()
        U32TO8(rom, i + 0x70, address + 0x88);    // Address of readSectors(sector, numSectors, buf)
        U32TO8(rom, i + 0x74,
               address + 0x8C);    // Address of writeSectors(sector, numSectors, buf)
        U32TO8(rom, i + 0x78, address + 0x90);    // Address of clearStatus()
        U32TO8(rom, i + 0x7C, address + 0x94);    // Address of shutdown()
        U32TO8(rom, i + 0x80, DLDI_START);        // startup()
        U32TO8(rom, i + 0x84, DLDI_INSERT);       // isInserted()
        U32TO8(rom, i + 0x88, DLDI_READ);         // readSectors(sector, numSectors, buf)
        U32TO8(rom, i + 0x8C, DLDI_WRITE);        // writeSectors(sector, numSectors, buf)
        U32TO8(rom, i + 0x90, DLDI_CLEAR);        // clearStatus()
        U32TO8(rom, i + 0x94, DLDI_STOP);         // shutdown()

        // Confirm that a driver has been patched
        LOG("Patched DLDI driver at ROM offset 0x%X\n", offset + i);
        patched = true;
    }
}

int Dldi::startup() {
    // Try to open the SD image
    sdImage = fopen(Settings::getSdImagePath().c_str(), "rb+");
    return (sdImage ? 1 : 0);
}

int Dldi::isInserted() {
    // Check if the SD image is opened
    return (sdImage ? 1 : 0);
}

int Dldi::readSectors(bool cpu, int sector, int numSectors, uint32_t buf) {
    if (!sdImage) return 0;

    const int offset = sector * 0x200;
    const int size = numSectors * 0x200;

    // Read data from the SD image
    uint8_t *data = new uint8_t[size];
    fseek(sdImage, offset, SEEK_SET);
    fread(data, sizeof(uint8_t), size, sdImage);

    // Write the data to memory
    for (int i = 0; i < size; i++)
        core->memory.write<uint8_t>(cpu, buf + i, data[i]);

    delete[] data;
    return 1;
}

int Dldi::writeSectors(bool cpu, int sector, int numSectors, uint32_t buf) {
    if (!sdImage) return 0;

    const int offset = sector * 0x200;
    const int size = numSectors * 0x200;

    // Read data from memory
    uint8_t *data = new uint8_t[size];
    for (int i = 0; i < size; i++)
        data[i] = core->memory.read<uint8_t>(cpu, buf + i);

    // Write the data to the SD image
    fseek(sdImage, offset, SEEK_SET);
    fwrite(data, sizeof(uint8_t), size, sdImage);

    delete[] data;
    return 1;
}

int Dldi::clearStatus() {
    // Dummy function
    return (sdImage ? 1 : 0);
}

int Dldi::shutdown() {
    if (!sdImage) return 0;

    // Close the SD image
    fclose(sdImage);
    sdImage = nullptr;
    return 1;
}
