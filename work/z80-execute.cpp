#include "z80.hpp"


// #define CPU_DEBUG

static int verboseMode = 0;

void MSG( int mode, const char* format, ...);

static bool load( unsigned char *memory, long memorySize, char *binPath );

#define NOP_OPCODE   0x00
#define HALT_OPCODE  0x76
#define FLOATING_BUS 0xFF


int main(int argc, char *argv[]) {

    unsigned char RAM[ 0x10000 ]; //  64 KByte memory
    const unsigned short RAMMASK = 0xFFFF;

    unsigned char IO[ 0x100 ];    // 256 Byte IO space
    const unsigned short IOMASK = 0xFF;

    // here we can simulate different power-on content or bus behaviour
    memset( RAM, NOP_OPCODE, sizeof( RAM ) );
    memset( IO, FLOATING_BUS, sizeof( IO ) );

    char* binPath = NULL;
    unsigned int origin = 0x0000;

    int result = 0;
    for (int i = 1; i < argc; i++) {
        if ('-' == argv[i][0]) {
            switch (argv[i][1]) {
                case 'c':
                    origin = 0x0100; // CP/M *.com files start at address 0x0100
                    break;
                case 'o': // origin
                    puts( argv[i] ); 
                    if ( argv[i][2] )
                        result = sscanf( argv[i]+2, "%x", &origin );
                    else if ( i < argc - 1 )
                            result = sscanf( argv[++i], "%x", &origin );
                    if ( result )
                        origin &= 0xFFFF; // limit to 64K
                    else {
                        fprintf( stderr,"Error: option -o needs a hexadecimal argument\n" );
                        return 1;
                    }
                    break;
                case 'v':
                    ++verboseMode;
                    break;
                default:
                    printf(
                        "Usage: %s [-c] [-oXXXX] <binfile>\n"
                        "  -c      origin = 0x100\n"
                        "  -oXXXX  origin = 0x0000 .. 0xFFFF\n",
                    argv[0]
                    );
                    return 1;
            }
        } else {
            binPath = argv[i];
        }
    }
    

    if (binPath) {
        if ( !load( RAM + origin, sizeof( RAM ) - origin, binPath ) ) {
            // fprintf( stderr, "Cannot initialize\n" );
            return -1;
        }
    } else
        RAM[ origin ] = HALT_OPCODE; // stop execution immediately for uninitialised RAM


    Z80 z80( // create processor object with memory and IO callback functions
        // read one byte from RAM
        [&RAM]( void* arg, unsigned short addr ) {
            MSG( 2, "  RD  $%04X $%02X\n", addr, RAM[ addr & RAMMASK ] );
            return RAM[ addr & RAMMASK ];
        }, 
        // write one byte to RAM
        [&RAM]( void* arg, unsigned short addr, unsigned char value ) {
            MSG( 2, "  WR  $%04X $%02X\n", addr, value );
            RAM[ addr & RAMMASK ] = value;
        },
        // input one byte from IO space
        [&IO]( void* arg, unsigned short port ) {
            MSG( 2, "  IN  $%04X $%02X\n", port, IO[ port & IOMASK ] );
            return IO[ port & IOMASK ];
        },
        // output one byte to IO space
        [&IO]( void* arg, unsigned short port, unsigned char value ) {
            MSG( 2, "  OUT $%04X $%02X\n", port, value );
            IO[ port & IOMASK ] = value;
        }, 
        &z80, // *arg
        true  // provide 16 bit IO addresses
    );

    z80.setDebugMessage([]( void* arg, const char* msg) { puts(msg); } );


    z80.reg.PC = origin;


#if 0
    // HALT instruction triggers a breakpoint (other instuction are also possible)
    
    z80.addBreakOperand( HALT_OPCODE, []( void* arg, unsigned char* opcode, int opcodeLength ) {
    	MSG( 2, "  HALT -> break\n" );
    	((Z80*)arg)->requestBreak();
    } );

    z80.execute(); // until requestBreakFlag is set

#else
    // check internal Z80 HALT status
    int ticks = 0;

    do {
        ticks += z80.execute( 1 );
    } while ( !(z80.reg.IFF & 0b10000000) ); // HACK: mask is private and can change

    MSG( 1, " Execution took %d ticks\n", ticks );

#endif

        
    return 0;
}


void MSG( int mode, const char* format, ...) {
    if ( verboseMode >= mode ) {
        va_list argptr;
        va_start(argptr, format);
        vfprintf(stderr, format, argptr);
        va_end(argptr);
    }
}


static bool load( unsigned char *memory, long memorySize, char *path ) {
    MSG( 2, "  Open file %s\n", path );
    // read bin file
    FILE* fp = fopen( path, "rb" );
    if ( !fp ) {
        fprintf( stderr, "File not found: %s\n", path );
        return false;
    }
    fseek( fp, 0, SEEK_END );
    long size = ftell( fp );
    if ( size < 1 || size > memorySize ) {
        fprintf( stderr, "File size (%ld bytes) exceeds RAM size (%ld bytes)\n", size, memorySize );
        fclose( fp );
        return false;
    }
    fseek( fp, 0, SEEK_SET);
    MSG( 1, " Load %d bytes from file %s\n", size, path );
    if ( size != (long)fread( memory, 1, (size_t)size, fp ) ) {
        fprintf( stderr, "Cannot read file: %s\n", path);
        fclose( fp );
        return false;
    }
    fclose( fp );
    return true;
}

