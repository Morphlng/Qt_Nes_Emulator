#include "debugger.h"
#include "ui_debugger.h"
#include <QDebug>
#include <QGraphicsSimpleTextItem>

Debugger::Debugger(QWidget *parent) :
      QMainWindow(parent),
      ui(new Ui::Debugger)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);
    mem_line = 0;
    InitTable();

    debug_timer = new QTimer(this);
    connect(debug_timer, &QTimer::timeout, this, &Debugger::updateInfos);
    debug_timer->start(100);
}

void Debugger::ConnectToBus(Bus *bus)
{
    this->bus = bus;
    bus->Cpu.isDebugging = true;
}

Debugger::~Debugger()
{
    bus->Cpu.isDebugging = false;
    delete ui;
    delete debug_timer;
    bus = nullptr;
}

void Debugger::InitTable()
{
    ui->MemoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->MemoryTable->setColumnCount(16);
    ui->MemoryTable->setRowCount(128);

    // Set column headers
    ui->MemoryTable->setHorizontalHeaderLabels({"00",
                                                "01",
                                                "02",
                                                "03",
                                                "04",
                                                "05",
                                                "06",
                                                "07",
                                                "08",
                                                "09",
                                                "0A",
                                                "0B",
                                                "0C",
                                                "0D",
                                                "0E",
                                                "0F"});

    // Set row headers
    QStringList vertical = {};
    for (int i = 0; i < 128; i++)
        vertical.append("0x" + QString("%1").arg(i * 16, 3, 16, QLatin1Char('0')).toUpper());
    ui->MemoryTable->setVerticalHeaderLabels(vertical);

    // Initialize (all 0)
    for (int i = 0; i < 128; i++) {
        for (int j = 0; j < 16; j++)
            ui->MemoryTable->setItem(i, j, new QTableWidgetItem(QString::number(0, 16)));
    }

    ui->MemoryTable->resizeColumnsToContents();
    ui->MemoryTable->resizeRowsToContents();
}

void Debugger::updateInfos()
{
    updateOpInfo();
    updateRegInfo();
    updateMemInfo();
}

void Debugger::updateOpInfo()
{
    ui->Curr_Op_label->setText("Current_Operation:" + bus->Cpu.curr_instruction);
}

void Debugger::updateRegInfo()
{
    ui->PC_label->setText("PC:" + QString::number(bus->Cpu.reg_pc, 16).toUpper());
    ui->RegA_label->setText("RegA:" + QString::number(bus->Cpu.reg_a));
    ui->RegX_label->setText("RegX:" + QString::number(bus->Cpu.reg_x));
    ui->RegY_label->setText("RegY:" + QString::number(bus->Cpu.reg_y));
    ui->SP_label->setText("SP:" + QString::number(bus->Cpu.reg_sp, 16).toUpper());
}

void Debugger::updateMemInfo()
{
    for (int i = mem_line; i < mem_line + 16; i++)
        for (int j = 0; j < 16; j++)
            ui->MemoryTable->setItem(i,
                                     j,
                                     new QTableWidgetItem(
                                         QString("%1")
                                             .arg(bus->ram_data[i * 16 + j], 2, 16, QLatin1Char('0'))
                                             .toUpper()));
    mem_line = (mem_line + 16) % 128;
}
