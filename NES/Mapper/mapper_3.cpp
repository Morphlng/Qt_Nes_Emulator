#include "mapper_3.h"
#include <QDebug>

Mapper3::Mapper3(quint8 rnum, quint8 vrnum) : Mapper(rnum, vrnum)
{
    if (vrom_num == 0)
        character_ram_ptr = new quint8[8192];
    else
        character_ram_ptr = nullptr;

    nCHRBankSelect = 0;
}

Mapper3::~Mapper3()
{
    memset(addram, 0, sizeof(quint8) * 0x2000);
    if (character_ram_ptr)
        delete[] character_ram_ptr;
    character_ram_ptr = NULL;
}

quint32 Mapper3::cpu_read_prg(quint16 addr)
{
    quint32 prg_addr = addr & (rom_num > 1 ? 0x7fff : 0x3fff);
    return prg_addr;
}

quint32 Mapper3::cpu_write_prg(quint16 addr, quint8 data)
{
    Q_UNUSED(addr);
    nCHRBankSelect = (data & 0x03) % vrom_num;
    return 0;
}

quint32 Mapper3::cpu_read_addram(quint16 addr)
{
    qDebug() << "Mapper3 doesn't have extra ram on" << QString::number(addr, 16);
    return 0xFFFF;
}

quint32 Mapper3::cpu_write_addram(quint16 addr, quint8 data)
{
    Q_UNUSED(data);
    qDebug() << "Mapper3 doesn't have extra ram on" << QString::number(addr, 16);
    return 0xFFFF;
}

quint32 Mapper3::ppu_read_pt(quint16 addr)
{
    if (vrom_num == 0) {
        return addr;
    } else {
        return (quint32)(nCHRBankSelect * 0x2000 + addr);
    }
}

quint32 Mapper3::ppu_write_pt(quint16 addr, quint8 data)
{
    Q_UNUSED(data);
    return addr;
}

void Mapper3::write_to_stream(QDataStream &stream)
{
    if (vrom_num == 0) {
        for (int i = 0; i < 8192; i++)
            stream << character_ram_ptr[i];
    }

    stream << nCHRBankSelect;
}

void Mapper3::read_from_stream(QDataStream &stream)
{
    if (vrom_num == 0) {
        for (int i = 0; i < 8192; i++)
            stream >> character_ram_ptr[i];
    }
    stream >> nCHRBankSelect;
}
