#ifndef MAPPER_H
#define MAPPER_H

#include <QDataStream>
#include <QtGlobal>

// The function of Mapper is to map addresses from cartridge to CPU&PPU
// There are mainly 4 ways of mapping：Horizontal、Vertical、OneScreen_Lo、OneScreen_Hi
enum MirrorMode { HORIZONTAL = 0, VERTICAL = 1, ONESCREEN_LO = 9, ONESCREEN_HI = 10 };

class Mapper
{
public:
    Mapper(quint8 rnum, quint8 vrnum) : rom_num(rnum), vrom_num(vrnum) {}
    virtual ~Mapper() {}

public:
    // CPU relevant virtual functions
    // read/write 0x6000-0xffff
    virtual quint32 cpu_read_addram(quint16 addr) = 0;
    virtual quint32 cpu_write_addram(quint16 addr, quint8 data) = 0;
    // prg: program
    virtual quint32 cpu_read_prg(quint16 addr) = 0;
    virtual quint32 cpu_write_prg(quint16 addr, quint8 data) = 0;

public:
    // PPU relevant virtual functions
    // read/write pattern table, switch ways of mapping
    // pt: pattern table
    virtual quint32 ppu_read_pt(quint16 addr) = 0;
    virtual quint32 ppu_write_pt(quint16 addr, quint8 data) = 0;

public:
    // IRQ Interface (for example, Mapper4 would use it)
    virtual bool irqState() { return false; }
    virtual void irqClear() {}

    // Scanline Counting
    virtual void scanline() {}

public:
    // for save game
    virtual void write_to_stream(QDataStream &) = 0;
    // for load game
    virtual void read_from_stream(QDataStream &) = 0;

public:
    quint8 nametable_mirror;
    quint8 addram[0x2000]; // not every mapper has add ram, but still, it could be easiser this way
    quint8 *character_ram_ptr; // if there is not pattern table on board, create a 8KB one

    quint8 rom_num;  // PRG_Bank_num
    quint8 vrom_num; // CHR_Bank_num
};

#endif // MAPPER_H
