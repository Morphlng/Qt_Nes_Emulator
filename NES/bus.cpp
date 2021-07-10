#include "bus.h"
#include <QDebug>
#include <QMessageBox>

Bus::Bus()
{
    clock_count = 0;
    this->Cpu.connectToBus(this);
    Ppu.ConnectCartridge(&cartridge);
    controller_left.init();
    controller_right.init();
    SetKeyMap();
}

void Bus::reset()
{
    memset(ram_data, 0, sizeof(quint8) * 2048);
    clock_count = 0;
    Cpu.reset();
    Ppu.reset();
    Apu.reset();
}

void Bus::clock()
{
    Ppu.clock();

    if (clock_count % 3 == 0) {
        if (dma_transfer) {
            // wait for clock sync
            if (dma_dummy) {
                if (clock_count % 2 == 1) {
                    dma_dummy = false;
                }
            } else {
                // On even clock fetch data
                if (clock_count % 2 == 0) {
                    dma_data = load(dma_page << 8 | dma_addr);
                } else {
                    // On odd clock write in
                    Ppu.pOAM[dma_addr] = dma_data;
                    dma_addr++;

                    // after 256 Bytes, address will turn to 0
                    if (dma_addr == 0x00) {
                        dma_transfer = false;
                        dma_dummy = true;
                    }
                }
            }
        } else {
            clock_count %= 0x3FFFFFFF; // avoid out of bound(2^31-1)
            Cpu.clock();
        }
    }

    if (Ppu.nmi) {
        Ppu.nmi = false;
        Cpu.nmi();
    }

    if (cartridge.mapper_ptr->irqState()) {
        cartridge.mapper_ptr->irqClear();
        Cpu.irq();
    }

    clock_count++;
}

void Bus::save(quint16 addr, quint8 data)
{
    if (addr < 0x2000) {
        ram_data[addr & 0x7ff] = data;
    } else if (addr < 0x4000) {
        switch (addr & 0x2007) {
        case 0x2000: // PPU ctrl
            Ppu.write_ctrl(data);
            break;
        case 0x2001: // PPU mask
            Ppu.write_mask(data);
            break;
        case 0x2002: // PPU status
            qDebug() << "can't write to PPU Status!";
            break;
        case 0x2003: // PPU oamaddr
            Ppu.write_oamaddr(data);
            break;
        case 0x2004: // PPU oamdata
            Ppu.write_oamdata(data);
            break;
        case 0x2005: // PPU scroll
            Ppu.write_scroll(data);
            break;
        case 0x2006: // PPU address
            Ppu.write_addr(data);
            break;
        case 0x2007: // PPU data
            Ppu.write_data(data);
            break;
        }
    } else if ((addr >= 0x4000 && addr <= 0x4013) || addr == 0x4015 || addr == 0x4017) {
        // APU write
        Apu.write_register(addr, data);
    } else if (addr == 0x4014) {
        // OAM DMA
        dma_page = data;
        dma_addr = 0x00;
        dma_transfer = true;
    } else if (addr == 0x4016) {
        // Controller Strobe
        controller_left.write_strobe(data);
        controller_right.write_strobe(data);
    } else if (addr >= 0x6000 && addr < 0x8000) {
        // $6000-$7FFF = Battery Backed Save or Work RAM
        cartridge.CpuWrite(addr, data);
    } else {
        // $8000-$FFFF = Usual ROM, commonly with Mapper Registers
        cartridge.CpuWrite(addr, data);
    }
}

quint8 Bus::load(quint16 addr)
{
    if (addr < 0x2000) {
        return ram_data[addr & 0x7ff];
    } else if (addr < 0x4000) {
        switch (addr & 0x2007) {
        case 0x2000: // PPU ctrl
            qDebug("cannot read PPU CTRL\n");
            break;
        case 0x2001: // PPU mask
            qDebug("cannot read PPU MASK\n");
            break;
        case 0x2002: // PPU status
            return Ppu.get_status();
        case 0x2003:
            qDebug("cannot read OAMADDR\n");
            break;
        case 0x2004: // PPU oamdata
            return Ppu.get_oamdata();
        case 0x2005: // PPU scroll
            qDebug("cannot read PPU SCROLL\n");
            break;
        case 0x2006: // PPU addr
            qDebug("cannot read PPU ADDR\n");
            break;
        case 0x2007: // PPU data
            return Ppu.read_data();
        }
    } else if (addr == 0x4015) {
        // APU Status
        return Apu.read_status();
    } else if (addr == 0x4016) {
        // controller 1 key state
        return controller_left.output_key_states();
    } else if (addr == 0x4017) {
        // controller 2 key state
        return controller_right.output_key_states();
    } else if (addr >= 0x4000 && addr < 0x6000) {
        qDebug() << "0x4000 - 0x6000, Cannot read " << QString::number(addr, 16);
    } else if (addr >= 0x6000 && addr < 0x8000) {
        return cartridge.CpuRead(addr);
    } else {
        return cartridge.CpuRead(addr);
    }
    return 0;
}

void Bus::SetKeyMap()
{
    this->controller_left.key_map.clear();
    this->controller_left.key_map.insert(Qt::Key_A, FC_KEY_LEFT);
    this->controller_left.key_map.insert(Qt::Key_S, FC_KEY_DOWN);
    this->controller_left.key_map.insert(Qt::Key_D, FC_KEY_RIGHT);
    this->controller_left.key_map.insert(Qt::Key_W, FC_KEY_UP);
    this->controller_left.key_map.insert(Qt::Key_J, FC_KEY_B);
    this->controller_left.key_map.insert(Qt::Key_K, FC_KEY_A);
    this->controller_left.key_map.insert(Qt::Key_Space, FC_KEY_START);
    this->controller_left.key_map.insert(Qt::Key_Shift, FC_KEY_SELECT);

    this->controller_right.key_map.clear();
    this->controller_right.key_map.insert(Qt::Key_Up, FC_KEY_UP);
    this->controller_right.key_map.insert(Qt::Key_Right, FC_KEY_RIGHT);
    this->controller_right.key_map.insert(Qt::Key_Down, FC_KEY_DOWN);
    this->controller_right.key_map.insert(Qt::Key_Left, FC_KEY_LEFT);
    this->controller_right.key_map.insert(Qt::Key_Z, FC_KEY_B);
    this->controller_right.key_map.insert(Qt::Key_X, FC_KEY_A);
    this->controller_right.key_map.insert(Qt::Key_BracketLeft, FC_KEY_START);
    this->controller_right.key_map.insert(Qt::Key_BracketRight, FC_KEY_SELECT);
}

QDataStream &operator<<(QDataStream &stream, const Bus &bus)
{
    stream << bus.cartridge.md5_val;
    stream << bus.Cpu;
    stream << bus.Ppu;
    stream << bus.Apu;
    bus.cartridge.mapper_ptr->write_to_stream(stream);

    for (int i = 0; i < 2048; i++)
        stream << bus.ram_data[i];

    stream << bus.clock_count;
    stream << bus.dma_page;
    stream << bus.dma_addr;
    stream << bus.dma_data;
    stream << bus.dma_dummy;
    stream << bus.dma_transfer;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, Bus &bus)
{
    QByteArray md5_tmp;
    stream >> md5_tmp;

    if (md5_tmp != bus.cartridge.md5_val) {
        QMessageBox::critical(nullptr,
                              QStringLiteral("ERROR"),
                              QStringLiteral("Savefile isn't compatible with current game!"));
        return stream;
    }

    stream >> bus.Cpu;
    stream >> bus.Ppu;
    stream >> bus.Apu;
    bus.cartridge.mapper_ptr->read_from_stream(stream);

    for (int i = 0; i < 2048; i++)
        stream >> bus.ram_data[i];

    stream >> bus.clock_count;
    stream >> bus.dma_page;
    stream >> bus.dma_addr;
    stream >> bus.dma_data;
    stream >> bus.dma_dummy;
    stream >> bus.dma_transfer;
    return stream;
}
