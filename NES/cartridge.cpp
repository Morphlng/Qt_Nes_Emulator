#include "cartridge.h"
#include <QByteArray>
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QMessageBox>

Cartridge::Cartridge()
{
    reset();
}

bool Cartridge::read_from_file(QString input_file)
{
    // 1. Read file and check validity
    QFile file(input_file);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(nullptr, QStringLiteral("ERROR"), QStringLiteral("Can't Open file"));
        return false;
    }

    quint8 *nes_data = new quint8[file.size()];
    file.read((char *) nes_data, file.size());
    if (nes_data[0] != 'N' || nes_data[1] != 'E' || nes_data[2] != 'S' || nes_data[3] != '\x1A') {
        qDebug() << "First 4 bytes in file must be NES\\x1A!";
        QMessageBox::critical(nullptr,
                              QStringLiteral("ERROR"),
                              QStringLiteral("This is not a NES rom"));
        return false;
    }
    game_title = input_file.toLower().split("/").last().remove(".nes");

    file.seek(0); // get MD5 value
    md5_val = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Md5).toHex();
    file.close();

    // 2. Deal with the header
    rom_num = nes_data[4];
    vrom_num = nes_data[5];
    quint8 nametable_mirror = nes_data[6] & 0xb;
    mapper_id = (nes_data[7] & 0xf0) | ((nes_data[6] >> 4) & 0x0f);
    prg_ram_size = nes_data[8];

    // 3. Deal with Mapper infos
    switch (mapper_id) {
    case 0:
        mapper_ptr = new Mapper0(rom_num, vrom_num);
        mapper_ptr->nametable_mirror = nametable_mirror;
        break;
    case 1:
        mapper_ptr = new Mapper1(rom_num, vrom_num);
        mapper_ptr->nametable_mirror = nametable_mirror;
        break;
    case 2:
        mapper_ptr = new Mapper2(rom_num, vrom_num);
        mapper_ptr->nametable_mirror = nametable_mirror;
        break;

    case 3:
        mapper_ptr = new Mapper3(rom_num, vrom_num);
        mapper_ptr->nametable_mirror = nametable_mirror;
        break;

    case 4:
        mapper_ptr = new Mapper4(rom_num, vrom_num);
        mapper_ptr->nametable_mirror = nametable_mirror;
        break;

    case 66:
        mapper_ptr = new Mapper66(rom_num, vrom_num);
        mapper_ptr->nametable_mirror = nametable_mirror;
        break;

    default:
        qDebug() << "Unsupported Mapper = " << mapper_id;
        QMessageBox::critical(nullptr,
                              QStringLiteral("ERROR"),
                              QStringLiteral(
                                  "The Mapper this game used aren't currently supported"));
        return false;
    }

    // 4. read PRG_Data and CHR_Data
    quint32 rom_start_dx = 16;
    quint32 vrom_start_dx = rom_num * 16384 + 16;
    program_data = new quint8[16384 * rom_num];
    memcpy(program_data, &nes_data[rom_start_dx], 16384 * rom_num);
    vrom_data = new quint8[8192 * vrom_num];
    memcpy(vrom_data, &nes_data[vrom_start_dx], 8192 * vrom_num);
    return true;
}

void Cartridge::reset()
{
    if (program_data) {
        delete[] program_data;
    }
    program_data = NULL;

    if (vrom_data) {
        delete[] vrom_data;
    }
    vrom_data = NULL;

    game_title.clear();
    md5_val.clear();
    rom_num = 0;
    vrom_num = 0;
    mapper_id = 0;
    prg_ram_size = 0;
    if (mapper_ptr)
        delete mapper_ptr;
    mapper_ptr = NULL;
}

void Cartridge::CpuWrite(quint16 addr, quint8 data)
{
    quint32 mapped_addr = 0;
    if (addr >= 0x6000 && addr < 0x8000) {
        mapped_addr = mapper_ptr->cpu_write_addram(addr, data);
        if (mapped_addr != 0xFFFF)
            mapper_ptr->addram[mapped_addr] = data;
    } else if (addr >= 0x8000 && addr <= 0xFFFF) {
        // Mapper won't change program_data
        // but will modify it's own registers
        mapper_ptr->cpu_write_prg(addr, data);
    }
}

quint8 Cartridge::CpuRead(quint16 addr)
{
    quint32 mapped_addr = 0;
    if (addr >= 0x6000 && addr < 0x8000) {
        mapped_addr = mapper_ptr->cpu_read_addram(addr);
        if (mapped_addr != 0xFFFF)
            return mapper_ptr->addram[mapped_addr];
    } else if (addr >= 0x8000 && addr <= 0xFFFF) {
        mapped_addr = mapper_ptr->cpu_read_prg(addr);
        return program_data[mapped_addr];
    }

    return 0;
}

void Cartridge::PpuWrite(quint16 addr, quint8 data)
{
    quint32 mapped_addr = 0;

    mapped_addr = mapper_ptr->ppu_write_pt(addr, data);

    if (vrom_num == 0)
        mapper_ptr->character_ram_ptr[mapped_addr] = data;
    else
        qDebug() << "cartridge's vrom is ReadOnly";
}

quint8 Cartridge::PpuRead(quint16 addr)
{
    quint32 mapped_addr = 0;

    mapped_addr = mapper_ptr->ppu_read_pt(addr);

    if (vrom_num != 0)
        return vrom_data[mapped_addr];
    else
        return mapper_ptr->character_ram_ptr[mapped_addr];
}
