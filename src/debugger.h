/**
 * 65(c)816 emulator/simulator
 * (C) Ray Clemens 2023
 */

#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include <ncurses.h> // WINDOW

#define KEY_CTRL_C 3
#define KEY_CTRL_H 8
#define KEY_CR 10
#define KEY_ESCAPE 27

#define MAX_CMD_LEN 40

#define CMD_DISP_X_OFFS 4

#define CMD_HIST_ENTRIES 20

/*
#define MIN(a, b) ({\
    __typeof__(a) _local_a = (a);\
    __typeof__(b) _local_b = (b);\
    ((_local_a < _local_b) ? _local_a : _local_b);});
*/

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

// History structure for execution history tracking
typedef struct hist_t {
    WINDOW *win;
    int win_height;
    int win_width;
    int entry_count;
    int entry_start;
    CPU_t cpu[CMD_HIST_ENTRIES];
    memory_t mem[CMD_HIST_ENTRIES][4];
} hist_t;
    

// Command input error codes
// Keep in sync with the cmd_err_msgs[] array in debugger.c
typedef enum cmd_err_t {
    CMD_ERR_OK = 0,
    CMD_ERR_EXPECTED_ARG,
    CMD_ERR_UNKNOWN_ARG,
    CMD_ERR_UNKNOWN_CMD,
    CMD_ERR_HELP_MAIN,
    CMD_ERR_HELP_NOT,
    CMD_ERR_INVALID_CHAR,
    CMD_ERR_VAL_OVERFLOW
} cmd_err_t;

// Error message box type
typedef struct cmd_err_msg {
    char *title;
    int win_h;
    int win_w;
    char *msg;
} cmd_err_msg;

#endif
