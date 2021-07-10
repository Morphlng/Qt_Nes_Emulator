#include "controller.h"
#include <QDebug>

void Controller::init()
{
    strobe = false;
    keystate = 0;
    for (int t = 0; t <= 7; t++)
        cur_keystate[t] = false;
}

void Controller::get_key_states()
{
    keystate = 0;
    for (int key_id = FC_KEY_A; key_id <= FC_KEY_RIGHT; key_id++) {
        if (cur_keystate[key_id])
            keystate |= (1 << key_id);
    }
}

void Controller::write_strobe(quint8 data)
{
    bool strobe_old = strobe;
    strobe = data & 1;
    if (strobe_old && (!(strobe))) {
        // get keystate before closing
        get_key_states();
    }
}

quint8 Controller::output_key_states()
{
    bool is_key_pressed = false;
    if (strobe) {
        // if strobe is on, output real time keystate
        is_key_pressed = cur_keystate[FC_KEY_A];
    } else {
        // if strobe isn't on, output cached keystate
        is_key_pressed = keystate & 1;
        // Prepare for next keystate
        keystate >>= 1;
    }
    return (0x40 | is_key_pressed);
}
