#ifndef MAPPER_3_H
#define MAPPER_3_H

#include "mapper.h"

// Mapper3 is the opposite of Mapper2
// Mapper2 expand PRG_Bank
// Mapper3 expand CHR_Bank
class Mapper3 : public Mapper
{
public:
    Mapper3(quint8 rom_num, quint8 vrom_num);
    ~Mapper3();

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
    quint8 nCHRBankSelect;
};

#endif // MAPPER_3_H
