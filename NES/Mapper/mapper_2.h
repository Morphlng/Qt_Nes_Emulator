#ifndef MAPPER_2_H
#define MAPPER_2_H

#include "mapper.h"

// Mapper2 info:
// http://wiki.nesdev.com/w/index.php/UxROM
class Mapper2 : public Mapper
{
public:
    Mapper2(quint8 rom_num, quint8 vrom_num);
    ~Mapper2();

    quint32 cpu_read_addram(quint16 addr) override;
    quint32 cpu_write_addram(quint16 addr, quint8 data) override;
    quint32 cpu_read_prg(quint16 addr) override;
    quint32 cpu_write_prg(quint16 addr, quint8 data) override;

    quint32 ppu_read_pt(quint16 addr) override;
    quint32 ppu_write_pt(quint16 addr, quint8 data) override;

public:
    // For Save/Load game
    void write_to_stream(QDataStream &) override;
    void read_from_stream(QDataStream &) override;

private:
    quint8 prg_select_16kb_lo;
    quint8 prg_select_16kb_hi;
};

#endif // MAPPER_2_H
