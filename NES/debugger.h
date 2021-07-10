#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "bus.h"
#include <QMainWindow>
#include <QTimer>

namespace Ui {
class Debugger;
}

class Debugger : public QMainWindow
{
    Q_OBJECT

public:
    explicit Debugger(QWidget *parent = nullptr);
    ~Debugger();
    void InitTable();
    void ConnectToBus(Bus *);

    void updateInfos();
    void updateOpInfo();
    void updateRegInfo();
    void updateMemInfo();

private:
    QTimer *debug_timer;
    int mem_line; // Update 16 lines each time, can be helpful for performance issue

private:
    Ui::Debugger *ui;
    Bus *bus;
};

#endif // DEBUGGER_H
