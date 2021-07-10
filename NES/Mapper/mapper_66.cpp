#include "mapper_66.h"
#include <QDebug>

Mapper66::Mapper66(quint8 rnum, quint8 vrnum) : Mapper(rnum, vrnum)
{
    if (vrom_num == 0)
        character_ram_ptr = new quint8[8192];
    else
        character_ram_ptr = nullptr;

    nCHRBankSelect = 0;
    nPRGBankSelect = 0;
}

Mapper66::~Mapper66()
{
    memset(addram, 0, sizeof(quint8) * 0x2000);
    if (character_ram_ptr)
        delete[] character_ram_ptr;
    character_ram_ptr = NULL;
}

quint32 Mapper66::cpu_read_prg(quint16 addr)
{
    quint32 prg_addr = nPRGBankSelect * 0x8000 + (addr & 0x7FFF);
    return prg_addr;
}

quint32 Mapper66::cpu_write_prg(quint16 addr, quint8 data)
{
    Q_UNUSED(addr);
    nCHRBankSelect = data & 0x03;
    nPRGBankSelect = (data >> 4) & 0x03;
    return 0;
}

quint32 Mapper66::cpu_read_addram(quint16 addr)
{
    qDebug() << "Mapper66 doesn't have extra ram on" << QString::number(addr, 16);
    return 0xFFFF;
}

quint32 Mapper66::cpu_write_addram(quint16 addr, quint8 data)
{
    Q_UNUSED(data);
    qDebug() << "Mapper66 doesn't have extra ram on" << QString::number(addr, 16);
    return 0xFFFF;
}

quint32 Mapper66::ppu_read_pt(quint16 addr)
{
    if (vrom_num == 0) {
        return addr;
    } else {
        return (quint32)(nCHRBankSelect * 0x2000 + addr);
    }
}

quint32 Mapper66::ppu_write_pt(quint16 addr, quint8 data)
{
    Q_UNUSED(data);
    return addr;
}

void Mapper66::write_to_stream(QDataStream &stream)
{
    if (vrom_num == 0) {
        for (int i = 0; i < 8192; i++)
            stream << character_ram_ptr[i];
    }

    stream << nPRGBankSelect;
    stream << nCHRBankSelect;
}

void Mapper66::read_from_stream(QDataStream &stream)
{
    if (vrom_num == 0) {
        for (int i = 0; i < 8192; i++)
            stream >> character_ram_ptr[i];
    }
    stream >> nPRGBankSelect;
    stream >> nCHRBankSelect;
}
