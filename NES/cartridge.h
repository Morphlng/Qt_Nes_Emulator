#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "Mapper/mapper.h"
#include "Mapper/mapper_0.h"
#include "Mapper/mapper_1.h"
#include "Mapper/mapper_2.h"
#include "Mapper/mapper_3.h"
#include "Mapper/mapper_4.h"
#include "Mapper/mapper_66.h"
#include <QString>
#include <QtGlobal>

class Cartridge
{
public:
    quint8 rom_num;       // PRG_ROM num（16KB each block）
    quint8 vrom_num;      // CHR_ROM num（8KB each block）
    quint8 *program_data; // PRG_Data
    quint8 *vrom_data;    // CHR_Data
    QString game_title;   // game_title for Savefile
    QByteArray md5_val;   // MD5 value for Savefile verify

    // Mapper infos
    quint8 mapper_id;    // Which Mapper
    quint8 prg_ram_size; // rarely used, you can ignore it for now
    Mapper *mapper_ptr;  // Mapper pointer

public:
    Cartridge();
    bool read_from_file(QString input_file);
    void reset();

    void CpuWrite(quint16 addr, quint8 data);
    quint8 CpuRead(quint16 addr);

    void PpuWrite(quint16 addr, quint8 data);
    quint8 PpuRead(quint16 addr);
};

#endif // CARTRIDGE_H
