#ifndef MAPPER_1_H
#define MAPPER_1_H

#include "mapper.h"

class REG_CTRL
{
public:
    quint8 data;
    bool get_pattern_bank_mode()
    {
        if (data & 0x10)
            return 1;
        else
            return 0;
    }
    quint8 get_nametable_mirror()
    {
        return (data & 0x03);
    }
    quint8 get_program_bank_mode()
    {
        return ((data >> 2) & 0x03);
    }
    void set_program_bank_mode(quint8 prg_mode)
    {
        data &= 0x13;
        data |= ((prg_mode & 0x3) << 2);
    }
};

class Mapper1 : public Mapper
{
public:
    Mapper1(quint8 rom_num, quint8 vrom_num);
    ~Mapper1();

    quint32 cpu_read_addram(quint16 addr) override;
    quint32 cpu_write_addram(quint16 addr, quint8 data) override;
    quint32 cpu_read_prg(quint16 addr) override;
    quint32 cpu_write_prg(quint16 addr, quint8 data) override;

    quint32 ppu_read_pt(quint16 addr) override;
    quint32 ppu_write_pt(quint16 addr, quint8 data) override;

public:
    // For Save/Load Game
    void write_to_stream(QDataStream &) override;
    void read_from_stream(QDataStream &) override;

private:
    quint8 num_write = 0;
    quint8 reg_load = 0;
    REG_CTRL reg_ctrl;

    quint8 pt_select_4kb_lo;
    quint8 pt_select_4kb_hi;
    quint8 pt_select_8kb;

    quint8 prg_select_16kb_lo;
    quint8 prg_select_16kb_hi;
    quint8 prg_select_32kb;
};

#endif // MAPPER_1_H
