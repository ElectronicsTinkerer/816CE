/**
 * 65(c)816 emulator/simulator
 * (C) Ray Clemens 2023
 */

#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include <ncurses.h>

#define KEY_CTRL_C 3
#define KEY_CTRL_H 8
#define KEY_CR 10

#define MAX_CMD_LEN 40

#define CMD_DISP_X_OFFS 4


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
    

// Memory watch window
typedef struct watch_t {
    WINDOW *win;
    uint32_t addr_s; // Start address of watch
    int win_height;
    int win_width;
    bool disasm_mode;
    bool follow_pc;
} watch_t;

#endif
