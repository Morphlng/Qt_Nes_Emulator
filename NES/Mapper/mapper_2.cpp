#include "mapper_2.h"
#include <QDebug>

Mapper2::Mapper2(quint8 rnum, quint8 vrnum) : Mapper(rnum, vrnum)
{
    if (vrom_num == 0)
        character_ram_ptr = new quint8[8192];
    else
        character_ram_ptr = nullptr;

    prg_select_16kb_lo = 0;
    prg_select_16kb_hi = rom_num - 1;
}

Mapper2::~Mapper2()
{
    memset(addram, 0, 0x2000);
    delete[] character_ram_ptr;
    character_ram_ptr = NULL;
}

quint32 Mapper2::cpu_read_prg(quint16 addr)
{
    // low 16KB
    if (addr <= 0xbfff) {
        quint32 program_addr = 0x4000 * prg_select_16kb_lo + (addr & 0x3fff);
        return program_addr;
    } else {
        // high 16KB
        quint32 program_addr = 0x4000 * prg_select_16kb_hi + (addr & 0x3fff);
        return program_addr;
    }
}

quint32 Mapper2::cpu_write_prg(quint16 addr, quint8 data)
{
    Q_UNUSED(addr);
    prg_select_16kb_lo = data & 0x0f;
    return 0;
}

quint32 Mapper2::cpu_read_addram(quint16 addr)
{
    qDebug() << "Mapper2 doesn't have extra ram in" << QString::number(addr, 16);
    return 0xFFFF;
}

quint32 Mapper2::cpu_write_addram(quint16 addr, quint8 data)
{
    Q_UNUSED(data);
    qDebug() << "Mapper2 doesn't have extra ram in" << QString::number(addr, 16);
    return 0xFFFF;
}

quint32 Mapper2::ppu_read_pt(quint16 addr)
{
    return addr;
}

quint32 Mapper2::ppu_write_pt(quint16 addr, quint8 data)
{
    Q_UNUSED(data);
    return addr;
}

void Mapper2::write_to_stream(QDataStream &stream)
{
    if (vrom_num == 0) {
        for (int i = 0; i < 8192; i++)
            stream << character_ram_ptr[i];
    }

    stream << prg_select_16kb_lo;
    stream << prg_select_16kb_hi;
}

void Mapper2::read_from_stream(QDataStream &stream)
{
    if (vrom_num == 0) {
        for (int i = 0; i < 8192; i++)
            stream >> character_ram_ptr[i];
    }
    stream >> prg_select_16kb_lo;
    stream >> prg_select_16kb_hi;
}
