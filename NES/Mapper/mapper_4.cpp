#include "mapper_4.h"
#include <QDebug>

Mapper4::Mapper4(quint8 rnum, quint8 vrnum) : Mapper(rnum, vrnum)
{
    if (vrom_num == 0)
        character_ram_ptr = new quint8[8192];
    else
        character_ram_ptr = nullptr;

    memset(addram, 0, sizeof(quint8) * 0x2000);

    nTargetRegister = 0x00;
    bPRGBankMode = false;
    bCHRInversion = false;

    bIRQActive = false;
    bIRQEnable = false;
    bIRQUpdate = false;
    nIRQCounter = 0x00;
    nIRQReload = 0x00;

    memset(pRegister, 0, sizeof(quint8) * 8);
    memset(pPRGBank, 0, sizeof(quint32) * 4);
    memset(pCHRBank, 0, sizeof(quint32) * 8);

    pPRGBank[0] = 0 * 0x2000;
    pPRGBank[1] = 1 * 0x2000;

    // PRG_Bank is 16KB each, we use 8KB so we need to x2
    pPRGBank[2] = (rom_num * 2 - 2) * 0x2000;
    pPRGBank[3] = (rom_num * 2 - 1) * 0x2000;
}

Mapper4::~Mapper4()
{
    memset(addram, 0, sizeof(quint8) * 0x2000);
    if (character_ram_ptr)
        delete[] character_ram_ptr;
    character_ram_ptr = NULL;
}

quint32 Mapper4::cpu_read_addram(quint16 addr)
{
    return (quint32)(addr & 0x1FFF);
}

quint32 Mapper4::cpu_write_addram(quint16 addr, quint8 data)
{
    Q_UNUSED(data);
    return (quint32)(addr & 0x1FFF);
}

quint32 Mapper4::cpu_read_prg(quint16 addr)
{
    quint32 prg_addr = 0;
    if (addr >= 0x8000 && addr <= 0x9FFF) {
        prg_addr = pPRGBank[0] + (addr & 0x1FFF);
    } else if (addr >= 0xA000 && addr <= 0xBFFF) {
        prg_addr = pPRGBank[1] + (addr & 0x1FFF);
    } else if (addr >= 0xC000 && addr <= 0xDFFF) {
        prg_addr = pPRGBank[2] + (addr & 0x1FFF);
    } else if (addr >= 0xE000 && addr <= 0xFFFF) {
        prg_addr = pPRGBank[3] + (addr & 0x1FFF);
    }

    return prg_addr;
}

quint32 Mapper4::cpu_write_prg(quint16 addr, quint8 data)
{
    if (addr >= 0x8000 && addr <= 0x9FFF) {
        // Bank Select
        if (!(addr & 0x0001)) {
            nTargetRegister = data & 0x07;
            bPRGBankMode = ((data >> 6) & 1);
            bCHRInversion = ((data >> 7) & 1);
        } else {
            // Update target register
            pRegister[nTargetRegister] = data;

            // Update Pointer Table
            if (bCHRInversion) {
                pCHRBank[0] = pRegister[2] * 0x0400;
                pCHRBank[1] = pRegister[3] * 0x0400;
                pCHRBank[2] = pRegister[4] * 0x0400;
                pCHRBank[3] = pRegister[5] * 0x0400;
                pCHRBank[4] = (pRegister[0] & 0xFE) * 0x0400;
                pCHRBank[5] = pRegister[0] * 0x0400 + 0x0400;
                pCHRBank[6] = (pRegister[1] & 0xFE) * 0x0400;
                pCHRBank[7] = pRegister[1] * 0x0400 + 0x0400;
            } else {
                pCHRBank[0] = (pRegister[0] & 0xFE) * 0x0400;
                pCHRBank[1] = pRegister[0] * 0x0400 + 0x0400;
                pCHRBank[2] = (pRegister[1] & 0xFE) * 0x0400;
                pCHRBank[3] = pRegister[1] * 0x0400 + 0x0400;
                pCHRBank[4] = pRegister[2] * 0x0400;
                pCHRBank[5] = pRegister[3] * 0x0400;
                pCHRBank[6] = pRegister[4] * 0x0400;
                pCHRBank[7] = pRegister[5] * 0x0400;
            }

            if (bPRGBankMode) {
                pPRGBank[2] = (pRegister[6] & 0x3F) * 0x2000;
                pPRGBank[0] = (rom_num * 2 - 2) * 0x2000;
            } else {
                pPRGBank[0] = (pRegister[6] & 0x3F) * 0x2000;
                pPRGBank[2] = (rom_num * 2 - 2) * 0x2000;
            }

            pPRGBank[1] = (pRegister[7] & 0x3F) * 0x2000;
            pPRGBank[3] = (rom_num * 2 - 1) * 0x2000;
        }
        return 0;
    }

    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (!(addr & 0x0001)) {
            // Mirroring
            if (data & 0x01)
                nametable_mirror = MirrorMode::HORIZONTAL;
            else
                nametable_mirror = MirrorMode::VERTICAL;
        } else {
            // PRG Ram Protect
            // TODO:
        }
        return 0;
    }

    if (addr >= 0xC000 && addr <= 0xDFFF) {
        if (!(addr & 0x0001)) {
            nIRQReload = data;
        } else {
            nIRQCounter = 0x0000;
        }
        return 0;
    }

    if (addr >= 0xE000 && addr <= 0xFFFF) {
        if (!(addr & 0x0001)) {
            bIRQEnable = false;
            bIRQActive = false;
        } else {
            bIRQEnable = true;
        }
        return 0;
    }

    return 0;
}

quint32 Mapper4::ppu_read_pt(quint16 addr)
{
    if (vrom_num == 0) {
        return addr;
    } else {
        quint32 vrom_addr = 0;
        if (addr >= 0x0000 && addr <= 0x03FF) {
            vrom_addr = pCHRBank[0] + (addr & 0x03FF);
        } else if (addr >= 0x0400 && addr <= 0x07FF) {
            vrom_addr = pCHRBank[1] + (addr & 0x03FF);
        } else if (addr >= 0x0800 && addr <= 0x0BFF) {
            vrom_addr = pCHRBank[2] + (addr & 0x03FF);
        } else if (addr >= 0x0C00 && addr <= 0x0FFF) {
            vrom_addr = pCHRBank[3] + (addr & 0x03FF);
        } else if (addr >= 0x1000 && addr <= 0x13FF) {
            vrom_addr = pCHRBank[4] + (addr & 0x03FF);
        } else if (addr >= 0x1400 && addr <= 0x17FF) {
            vrom_addr = pCHRBank[5] + (addr & 0x03FF);
        } else if (addr >= 0x1800 && addr <= 0x1BFF) {
            vrom_addr = pCHRBank[6] + (addr & 0x03FF);
        } else if (addr >= 0x1C00 && addr <= 0x1FFF) {
            vrom_addr = pCHRBank[7] + (addr & 0x03FF);
        }

        return vrom_addr;
    }
}

quint32 Mapper4::ppu_write_pt(quint16 addr, quint8 data)
{
    Q_UNUSED(data);
    return addr;
}

bool Mapper4::irqState()
{
    return bIRQActive;
}

void Mapper4::irqClear()
{
    bIRQActive = false;
}

void Mapper4::scanline()
{
    if (nIRQCounter == 0) {
        nIRQCounter = nIRQReload;
    } else
        nIRQCounter--;

    if (nIRQCounter == 0 && bIRQEnable) {
        bIRQActive = true;
    }
}

void Mapper4::write_to_stream(QDataStream &stream)
{
    stream << nametable_mirror;

    for (int i = 0; i < 8192; i++)
        stream << addram[i];

    if (vrom_num == 0) {
        for (int i = 0; i < 8192; i++)
            stream << character_ram_ptr[i];
    }

    stream << nTargetRegister;
    stream << bPRGBankMode;
    stream << bCHRInversion;

    for (int i = 0; i < 8; i++) {
        stream << pRegister[i];
        stream << pCHRBank[i];
    }

    for (int i = 0; i < 4; i++)
        stream << pPRGBank[i];

    stream << bIRQActive;
    stream << bIRQEnable;
    stream << bIRQUpdate;

    stream << nIRQCounter;
    stream << nIRQReload;
}

void Mapper4::read_from_stream(QDataStream &stream)
{
    stream >> nametable_mirror;

    for (int i = 0; i < 8192; i++)
        stream >> addram[i];

    if (vrom_num == 0) {
        for (int i = 0; i < 8192; i++)
            stream >> character_ram_ptr[i];
    }

    stream >> nTargetRegister;
    stream >> bPRGBankMode;
    stream >> bCHRInversion;

    for (int i = 0; i < 8; i++) {
        stream >> pRegister[i];
        stream >> pCHRBank[i];
    }

    for (int i = 0; i < 4; i++)
        stream >> pPRGBank[i];

    stream >> bIRQActive;
    stream >> bIRQEnable;
    stream >> bIRQUpdate;

    stream >> nIRQCounter;
    stream >> nIRQReload;
}
