#include "mapper_0.h"
#include <QDebug>

Mapper0::Mapper0(quint8 rnum, quint8 vrnum) : Mapper(rnum, vrnum)
{
    if (vrom_num == 0)
        character_ram_ptr = new quint8[8192];
    else
        character_ram_ptr = nullptr;
}

Mapper0::~Mapper0()
{
    memset(addram, 0, 0x2000);
    if (character_ram_ptr)
        delete[] character_ram_ptr;
    character_ram_ptr = NULL;
}

quint32 Mapper0::cpu_read_prg(quint16 addr)
{
    quint16 prg_addr = addr & (rom_num > 1 ? 0x7fff : 0x3fff);
    return prg_addr;
}

quint32 Mapper0::cpu_write_prg(quint16 addr, quint8 data)
{
    Q_UNUSED(data);
    qDebug() << "0x8000 - 0xFFFF, cannot write to " << QString::number(addr, 16);
    return 0;
}

quint32 Mapper0::cpu_read_addram(quint16 addr)
{
    qDebug() << "Mapper0 doesn't have extra ram in" << QString::number(addr, 16);
    return 0xFFFF;
}

quint32 Mapper0::cpu_write_addram(quint16 addr, quint8 data)
{
    Q_UNUSED(data);
    qDebug() << "Mapper0 doesn't have extra ram in" << QString::number(addr, 16);
    return 0xFFFF;
}

quint32 Mapper0::ppu_read_pt(quint16 addr)
{
    return addr;
}

quint32 Mapper0::ppu_write_pt(quint16 addr, quint8 data)
{
    Q_UNUSED(data);
    return addr;
}

void Mapper0::write_to_stream(QDataStream &stream)
{
    if (vrom_num == 0) {
        for (int i = 0; i < 8192; i++)
            stream << character_ram_ptr[i];
    }
}

void Mapper0::read_from_stream(QDataStream &stream)
{
    if (vrom_num == 0) {
        for (int i = 0; i < 8192; i++)
            stream >> character_ram_ptr[i];
    }
}
