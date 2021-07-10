#ifndef MAPPER_0_H
#define MAPPER_0_H

#include "mapper.h"

// Mapper0 is the very basic one, which has no implements
class Mapper0 : public Mapper
{
public:
    Mapper0(quint8 rom_num, quint8 vrom_num);
    ~Mapper0();

    quint32 cpu_read_addram(quint16 addr) override;
    quint32 cpu_write_addram(quint16 addr, quint8 data) override;
    quint32 cpu_read_prg(quint16 addr) override;
    quint32 cpu_write_prg(quint16 addr, quint8 data) override;

    quint32 ppu_read_pt(quint16 addr) override;
    quint32 ppu_write_pt(quint16 addr, quint8 data) override;

    void write_to_stream(QDataStream &) override;
    void read_from_stream(QDataStream &) override;
};

#endif // MAPPER_0_H
