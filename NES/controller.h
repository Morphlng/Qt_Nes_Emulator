#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QMap>
#include <QtGlobal>

#define FC_KEY_A 0
#define FC_KEY_B 1
#define FC_KEY_SELECT 2
#define FC_KEY_START 3
#define FC_KEY_UP 4
#define FC_KEY_DOWN 5
#define FC_KEY_LEFT 6
#define FC_KEY_RIGHT 7

class Controller
{
private:
    // registers
    bool strobe;     // whether strobe is on or not
    quint8 keystate; // save the keystate

public:
    // interface for CPU to call
    void write_strobe(quint8 data);
    quint8 output_key_states();

public:
    void init();
    void get_key_states();     // return realtime keystate or cached keystate based on strobe
    QMap<int, quint8> key_map; // Key Mapping, maybe should allow users to change
    bool cur_keystate[8];      // real time key state
};

#endif
