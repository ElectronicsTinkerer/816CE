/**
 * 65(c)816 emulator/simulator
 * (C) Ray Clemens 2023
 */

#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#define KEY_CTRL_C 3
#define KEY_CTRL_X 24

// Keep in sync with status_msgs
typedef enum status_t {
    STATUS_NONE,
    STATUS_F12,
    STATUS_RESET,
    STATUS_CRASH
} status_t;

static char status_msgs[][64] = {
    "Normal",
    "Press F12 again to exit. Any other key to cancel.",
    "CPU Reset",
    "CPU Crashed - internal error"
};
    

// Keep in sync with state_msgs
typedef enum state_t {
    STATE_IDLE = 0,
    STATE_CRASH
} state_t;

static char state_msgs[][16] = {
    "IDLE",
    "CRASH"
};

#endif
