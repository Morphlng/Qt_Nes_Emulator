#ifndef BUS_H
#define BUS_H

#include "Simple_Apu.h"
#include "cartridge.h"
#include "controller.h"
#include "cpu.h"
#include "ppu.h"
#include <QDataStream>

class Bus
{
    friend QDataStream &operator<<(QDataStream &stream, const Bus &bus); // Serialize
    friend QDataStream &operator>>(QDataStream &stream, Bus &bus);       // Deserialize
public:
    Bus();
    void reset();
    void clock(); // run 1 cycle

    void save(quint16 addr, quint8 data); // save data to Bus
    quint8 load(quint16 addr);            // load data from Bus
    void SetKeyMap();                     // map keyboard to NES

public:
    quint8 ram_data[2048];

public:
    CPU Cpu;
    PPU Ppu;
    Simple_Apu Apu;
    Cartridge cartridge;
    Controller controller_left;
    Controller controller_right;

private:
    // record clock cycle cound
    // The frequency of the CPU is 1/3 of the PPUs，so call CPU.clock every 3 cycles.
    quint32 clock_count;

    // DMA relevant
    quint8 dma_page = 0x00;    // DMA transfer one page data
    quint8 dma_addr = 0x00;    // page and addr together is a 16bit address
    quint8 dma_data = 0x00;    // Data that will transfer from CPU to OAM
    bool dma_dummy = true;     // You have to wait for clock synchronize while executing DMA
    bool dma_transfer = false; // Flag which tell you DMA is executing
};

#endif // BUS_H
