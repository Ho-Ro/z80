#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "z80.hpp"

#include "file.h"
#include "kk_ihex_read.h"

static int verboseMode = 0;

static void MSG(int mode, const char* format, ...);

static bool load_bin(char* binPath, unsigned offset, bool z80 = false);
static bool load_hex(char* binPath);

#define NOP_OPCODE 0x00
#define HALT_OPCODE 0x76
#define FLOATING_BUS 0xFF

const unsigned int MAX_MEM = 0x10000;
static unsigned char RAM[MAX_MEM]; //  64 KByte memory
static unsigned int RAM_low_addr = MAX_MEM;
static unsigned int RAM_high_addr = 0x0000;

static unsigned char IO[0x100]; // 256 Byte IO space

void usage(const char* fullpath)
{
    const char* progname = 0;
    while (char c = *fullpath++)
        if (c == '/' || c == '\\')
            progname = fullpath;
    printf(
        "Usage:\n"
        "  %s [-b] [-x] [-fXX] [-c] [-oXXXX] [-sXXXX] <infile>\n"
        "    -b      treat infile as binary\n"
        "    -x      treat infile as hex\n"
        "    -fXX    fill = 0x00 .. 0xFF\n"
        "    -c      offset = start = 0x100 (*.com format)\n"
        "    -oXXXX  offset = 0x0000 .. 0xFFFF\n"
        "    -sXXXX  start  = 0x0000 .. 0xFFFF\n",
        progname);
}

int main(int argc, char* argv[]) {

    const unsigned short MEM_MASK = 0xFFFF;
    const unsigned short IO_MASK = 0xFF;

    char* inPath = NULL;
    unsigned int offset = 0x0000;
    unsigned int start = 0x0000;
    int fill = 0x00;
    int result = 0;

    for (int i = 1, j = 0; i < argc; i++) {
        if ('-' == argv[i][0]) {
            switch (argv[i][++j]) {
                case 'c':
                    offset = start = 0x0100; // CP/M *.com files start at address 0x0100
                    break;
                case 'f':             // fill
                    if (argv[i][++j]) // "-fXX"
                        result = sscanf(argv[i] + j, "%x", &fill);
                    else if (i < argc - 1) // "-f XX"
                        result = sscanf(argv[++i], "%x", &fill);
                    if (result)
                        fill &= 0x00FF; // limit to byte size
                    else {
                        fprintf(stderr, "Error: option -f needs a hexadecimal argument\n");
                        return 1;
                    }
                    j = 0; // end of this arg group
                    break;
                case 'o':             // offset
                    if (argv[i][++j]) // "-oXXXX"
                        result = sscanf(argv[i] + j, "%x", &offset);
                    else if (i < argc - 1) // "-o XXXX"
                        result = sscanf(argv[++i], "%x", &offset);
                    if (result)
                        offset &= 0xFFFF; // limit to 64K
                    else {
                        fprintf(stderr, "Error: option -o needs a hexadecimal argument\n");
                        return 1;
                    }
                    j = 0; // end of this arg group
                    break;
                case 's':             // start
                    if (argv[i][++j]) // "-sXXXX"
                        result = sscanf(argv[i] + j, "%x", &start);
                    else if (i < argc - 1) // "-s XXXX"
                        result = sscanf(argv[++i], "%x", &start);
                    if (result)
                        start &= 0xFFFF; // limit to 64K
                    else {
                        fprintf(stderr, "Error: option -s needs a hexadecimal argument\n");
                        return 1;
                    }
                    j = 0; // end of this arg group
                    break;
                case 'v':
                    ++verboseMode;
                    break;
                default:
                    usage(argv[0]);
                    return 1;
            }

            if (j && argv[i][j + 1]) { // one more arg char
                --i;                   // keep this arg group
                continue;
            }
            j = 0; // start from the beginning in next arg group
        } else {
            if (!inPath)
                inPath = argv[i];
            else {
                usage(argv[0]);
                return 1;
            } // check next arg string
        }
    }

    // here we can simulate different power-on content or bus behaviour
    memset(RAM, fill, sizeof(RAM));
    memset(IO, FLOATING_BUS, sizeof(IO));

    if (inPath) {
        int status;
        // input file type detection
        if ( strlen(inPath) > 4 && !strcmp(inPath + strlen(inPath) - 4, ".hex") )
            status = load_hex(inPath);
        else if ( strlen(inPath) > 4 && !strcmp(inPath + strlen(inPath) - 4, ".z80") )
            status = load_bin(inPath, offset, true);
        else
            status = load_bin(inPath, offset );
        if ( !status ) {
            fprintf(stderr, "Cannot initialize from %s\n", inPath);
            return -1;
        }
        if (!start && start < RAM_low_addr)
            start = RAM_low_addr;
    } else
        RAM[offset] = HALT_OPCODE; // no infile, stop execution immediately for uninitialised RAM

    Z80 z80( // create processor object with memory and IO callback functions
             // read one byte from RAM
        [](void* arg, unsigned short addr) {
            MSG(2, "RD  $%04X $%02X\n", addr, RAM[addr & MEM_MASK]);
            return RAM[addr & MEM_MASK];
        },
        // write one byte to RAM
        [](void* arg, unsigned short addr, unsigned char value) {
            MSG(2, "WR  $%04X $%02X\n", addr, value);
            RAM[addr & MEM_MASK] = value;
        },
        // input one byte from IO space
        [](void* arg, unsigned short port) {
            MSG(2, "IN  $%04X $%02X\n", port, IO[port & IO_MASK]);
            return IO[port & IO_MASK];
        },
        // output one byte to IO space
        [](void* arg, unsigned short port, unsigned char value) {
            MSG(2, "OUT $%04X $%02X\n", port, value);
            IO[port & IO_MASK] = value;
        },
        &z80, // *arg
        true  // provide 16 bit IO addresses
    );

    z80.setDebugMessage([](void* arg, const char* msg) { puts(msg); });

    MSG(1, "Offset: $%04X, start: $%04X\n", offset, start);

    z80.reg.PC = start;

#if 0
    // HALT instruction triggers a breakpoint (other instuction are also possible)
    
    z80.addBreakOperand( HALT_OPCODE, []( void* arg, unsigned char* opcode, int opcodeLength ) {
    	MSG( 2, "HALT -> break\n" );
    	((Z80*)arg)->requestBreak();
    } );

    z80.execute(); // until requestBreakFlag is set

#else
    // check internal Z80 HALT status
    int ticks = 0;

    do {
        ticks += z80.execute(1);
    } while (!(z80.reg.IFF & 0b10000000)); // HACK: mask is private and can change

    MSG(1, "Execution took %d ticks\n", ticks);

#endif

    return 0;
}

void MSG(int mode, const char* format, ...)
{
    if (verboseMode >= mode) {
        while (mode--)
            fprintf(stderr, " ");
        va_list argptr;
        va_start(argptr, format);
        vfprintf(stderr, format, argptr);
        va_end(argptr);
    }
}

static bool load_bin(char* path, unsigned offset, bool z80)
{
    MSG(2, "Open bin file %s\n", path);

    FILE* fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "File not found: %s\n", path);
        return false;
    }
    fseek(fp, 0, SEEK_END);
    unsigned size = (unsigned)ftell(fp);
    if (size < 1 || size > MAX_MEM - offset) {
        fprintf(stderr, "File size (%u bytes) exceeds available RAM size (%u bytes)\n",
                size, MAX_MEM - offset);
        fclose(fp);
        return false;
    }
    if (z80) {
        const char _Z80HEADER[] = "Z80ASM\032\n";
        const unsigned h = strlen( _Z80HEADER );
        char tmp[ h + 1 ];
        tmp[ h ] = 0;
        unsigned char c[2];
        unsigned iii = 0;
        int ret = 0;

        fseek(fp, 0, SEEK_SET);
        if ((fread(tmp, 1, h, fp)) != h)
            ret = 1;
        else if (strcmp(tmp, _Z80HEADER))
            ret = 1;
        else if (fread(c, 1, 2, fp) != 2)
            ret = 1;
        else {
            offset = (c[1] << 8) | c[0];
            iii = h + 2;
        }
        if (fseek(fp, 0, SEEK_END))
            ret = 2;
        else if ( ftell(fp) < iii )
            ret = 2;
        if (fseek(fp, iii, SEEK_SET))
            ret = 3;
        if (ret)
            return ret;
        size -= iii;
    } else
        fseek(fp, 0, SEEK_SET);

    if (size != (unsigned)fread(RAM + offset, 1, (size_t)size, fp)) {
        fprintf(stderr, "Cannot read file: %s (%d)\n", path, size);
        fclose(fp);
        return false;
    }
    RAM_low_addr = offset;
    RAM_high_addr = offset + size - 1;
    MSG(1, "Loaded %d data bytes from binfile into RAM region $%04X .. $%04X\n",
        size, RAM_low_addr, RAM_high_addr);
    fclose(fp);
    return true;
}

static bool load_hex(char* path)
{
    MSG(2, "Open hex file %s\n", path);

    char buffer[256];

    FILE* fp = fopen(path, "r");

    if (!fp) {
        return false;
    }

    struct ihex_state ihex;
    ihex_begin_read(&ihex);

    while (fgets(buffer, sizeof(buffer), fp)) {
        MSG(3, "%s", buffer); // buffer with trailing return
        ihex_read_bytes(&ihex, buffer, strlen(buffer));
    }

    ihex_end_read(&ihex);

    fclose(fp);

    return true;
}

// callback from kk_ihex_read.c when data has arrived
ihex_bool_t ihex_data_read(struct ihex_state* ihex,
                           ihex_record_type_t type,
                           ihex_bool_t error)
{
    static int hex_data_size = 0;
    error = error || (ihex->length < ihex->line_length);
    if (type == IHEX_DATA_RECORD && !error) {
        MSG(4, "IHEX addr: $%04X, data len: %d\n", IHEX_LINEAR_ADDRESS(ihex), ihex->length);
        memcpy(RAM + IHEX_LINEAR_ADDRESS(ihex), ihex->data, ihex->length);
        if (IHEX_LINEAR_ADDRESS(ihex) < RAM_low_addr)
            RAM_low_addr = IHEX_LINEAR_ADDRESS(ihex);
        if (IHEX_LINEAR_ADDRESS(ihex) + ihex->length >= RAM_high_addr)
            RAM_high_addr = IHEX_LINEAR_ADDRESS(ihex) + ihex->length - 1;
        hex_data_size += ihex->length;
    } else if (type == IHEX_END_OF_FILE_RECORD) {
        MSG(4, "IHEX EOF\n");
        MSG(1, "Loaded %d data bytes from hexfile into RAM region $%04X .. $%04X\n",
            hex_data_size, RAM_low_addr, RAM_high_addr);
    }
    return !error;
}
