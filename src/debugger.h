/**
 * 65(c)816 simulator/emulator (816CE)
 * Copyright (C) 2023 Ray Clemens
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

// Keep in sync with status_msgs
typedef enum status_t {
    STATUS_NONE,
    STATUS_F12,
    STATUS_RESET,
    STATUS_CRASH,
    STATUS_RUN
} status_t;    

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
    CMD_ERR_EXIT = -1,
    CMD_ERR_OK = 0, // Start of cmd_err_msgs index
    CMD_ERR_EXPECTED_ARG,
    CMD_ERR_EXPECTED_REG,
    CMD_ERR_EXPECTED_VALUE,
    CMD_ERR_UNKNOWN_ARG,
    CMD_ERR_UNKNOWN_CMD,
    CMD_ERR_HELP_MAIN,
    CMD_ERR_HELP_NOT,
    CMD_ERR_INVALID_CHAR,
    CMD_ERR_VAL_OVERFLOW,
    CMD_ERR_EXPECTED_FILENAME,
    CMD_ERR_FILE_IO_ERROR,
    CMD_ERR_FILE_TOO_LARGE,
    CMD_ERR_FILE_WILL_WRAP,
    CMD_ERR_FILE_PERM_DENIED,
    CMD_ERR_FILE_LOOP,
    CMD_ERR_FILE_NAME_TOO_LONG,
    CMD_ERR_FILE_NOT_EXIST,
    CMD_ERR_FILE_UNKNOWN_ERROR,
    CMD_ERR_CPU_CORRUPT_FILE
} cmd_err_t;

// Error message box type
typedef struct cmd_err_msg {
    char *title;
    int win_h;
    int win_w;
    char *msg;
} cmd_err_msg;

#endif
