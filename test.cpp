#include "z80.hpp"

// 利用プログラム側でMMUを実装します。
// ミニマムでは 64KB の RAM と 256バイト の I/O ポートが必要です。
class MMU
{
  public:
    unsigned char RAM[0x10000];
    unsigned char IO[0x100];
};

// CPUからメモリ読み込み要求発生時のコールバック
unsigned char
readByte(void* arg, unsigned short addr)
{
    return ((MMU*)arg)->RAM[addr];
}

// CPUからメモリ書き込み要求発生時のコールバック
void writeByte(void* arg, unsigned short addr, unsigned char value)
{
    ((MMU*)arg)->RAM[addr] = value;
}

// 入力命令（IN）発生時のコールバック
unsigned char inPort(void* arg, unsigned char port)
{
    return ((MMU*)arg)->IO[port];
}

// 出力命令（OUT）発生時のコールバック
void outPort(void* arg, unsigned char port, unsigned char value)
{
    ((MMU*)arg)->IO[port] = value;
}

int main()
{
    // MMUインスタンスを作成
    MMU mmu;
    mmu.RAM[0] = 0b01000111; // LD B, A

    // CPUインスタンスを作成
    // コールバック、コールバック引数、デバッグ出力設定を行う
    // - コールバック引数: コールバックに渡す任意の引数（ここでは、MMUインスタンスのポインタを指定）
    // - デバッグ出力設定: 省略するかNULLを指定するとデバッグ出力しない（ここでは、標準出力を指定）
    Z80 z80(readByte, writeByte, inPort, outPort, &mmu, stdout);

    z80.reg.pair.A = 0x12;
    z80.reg.pair.B = 0x34;

    // 32Hz実行
    z80.execute(32);

    // レジスタダンプを表示
    z80.registerDump();
    return 0;
}