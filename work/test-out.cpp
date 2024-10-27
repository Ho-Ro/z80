#include "z80.hpp"

int main()
{
    unsigned char ROM[256] = {
        0x01, 0x34, 0x12, // LD BC, $1234
        0x3E, 0x01,       // LD A, $01
        0xED, 0x79,       // OUT (C), A
        0xED, 0x78,       // IN A, (C)
        0x3E, 0x56,       // LD A, $56
        0xDB, 0x78,       // OUT (0x78), A
        0x3E, 0x9A,       // LD A, $9A
        0xDB, 0xBC,       // IN A, (0xBC)
        0xc3, 0x09, 0x00, // JMP $0009
    };

    unsigned char IO[0x10000];

    {
        puts("=== 8bit port mode ===");
        Z80 z80(
            [&ROM](void* arg, unsigned short addr) {
                // read byte from ROM
                return ROM[addr & 0xFF];
            },
            [&ROM](void* arg, unsigned short addr, unsigned char value) {
                // nothing to do
                ROM[addr & 0xFF] = value;
            },
            [&IO](void* arg, unsigned short port) {
                // the bottom half (A0 through A7) of the address bus to select one of 256 I/O devices
                printf("IN  $%04X $%02X\n", port, IO[port & 0xFF]);
                return IO[port & 0xFF];
            },
            [&IO](void* arg, unsigned short port, unsigned char value) {
                // the bottom half (A0 through A7) of the address bus to select one of 256 I/O devices
                printf("OUT $%04X $%02X\n", port, value);
                IO[port & 0xFF] = value;
            },
            &z80, false);
        z80.setDebugMessage([](void* arg, const char* msg) { puts(msg); });
        int hz = z80.execute(80);
        printf("Hz: %d\n", hz);
    }

    {
        puts("=== 16bit port mode ===");
        Z80 z80([&ROM](void* arg, unsigned short addr) { return ROM[addr & 0xFF]; }, [&ROM](void* arg, unsigned short addr, unsigned char value) {
            // nothing to do
            ROM[addr&0xFF] = value; }, [&IO](void* arg, unsigned short port) {
            printf("IN  $%04X $%02X\n",
                port, IO[port] // the full address bus to select the I/O device at one of 65536 possible ports
            );
            return IO[port & 0xFF]; }, [&IO](void* arg, unsigned short port, unsigned char value) {
            printf("OUT $%04X $%02X\n",
                port, // the full address bus to select the I/O device at one of 65536 possible ports
                value
            );
            IO[port] = value; }, &z80, true);
        z80.setDebugMessage([](void* arg, const char* msg) { puts(msg); });
        z80.execute(80);
    }
    return 0;
}
