/**
 * 65(c)816 emulator/simulator
 * (C) Ray Clemens 2023
 */

#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#define KEY_CTRL_C 3
#define KEY_CTRL_X 24

typedef enum state_t {
    STATE_IDLE = 0,
    STATE_CRASH
} state_t;

static char state_msgs[][16] = {
    "IDLE",
    "CRASH"
};

#endif
