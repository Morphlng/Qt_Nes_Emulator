#ifndef MAPPER_66_H
#define MAPPER_66_H

#include "mapper.h"

class Mapper66 : public Mapper
{
public:
    Mapper66(quint8 rom_num, quint8 vrom_num);
    ~Mapper66();

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
    quint8 nPRGBankSelect;
};

#endif // MAPPER_66_H
