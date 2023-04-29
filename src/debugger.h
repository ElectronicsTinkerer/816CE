/**
 * 65(c)816 simulator/emulator (816CE)
 * Copyright (C) 2023 Ray Clemens
 */

#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include <ncurses.h> // WINDOW

#define MEMORY_SIZE 0x1000000 // 16MiB

#define UART_SOCK_PORT 6501

#define KEY_CTRL_C 3
#define KEY_CTRL_H 8
#define KEY_CR 10
#define KEY_ESCAPE 27
#define KEY_DELETE 127

#define MAX_CMD_LEN 40

#define CMD_DISP_X_OFFS 4

#define CMD_HIST_ENTRIES 20

#define RUN_MODE_STEPS_UNTIL_DISP_UPDATE 1000

#define REPLACE_INST true
#define PUSH_INST false

typedef enum memory_fmt_t {
    MF_BASIC_BIN_BLOCK,
    MF_LLVM_MOS_SIM
} memory_fmt_t;

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
    CMD_EXIT = -1,
    CMD_OK = 0, // Start of cmd_err_msgs index
    CMD_SPECIAL,
    CMD_EXPECTED_ARG,
    CMD_EXPECTED_REG,
    CMD_EXPECTED_VALUE,
    CMD_UNKNOWN_ARG,
    CMD_UNKNOWN_CMD,
    CMD_HELP_MAIN,
    CMD_HELP_NOT,
    CMD_INVALID_CHAR,
    CMD_VAL_OVERFLOW,
    CMD_EXPECTED_FILENAME,
    CMD_FILE_IO_ERROR,
    CMD_FILE_TOO_LARGE,
    CMD_FILE_WILL_WRAP,
    CMD_FILE_PERM_DENIED,
    CMD_FILE_LOOP,
    CMD_FILE_NAME_TOO_LONG,
    CMD_FILE_NOT_EXIST,
    CMD_FILE_UNKNOWN_ERROR,
    CMD_FILE_CORRUPT,
    CMD_CPU_CORRUPT_FILE,
    CMD_CPU_OPTION_COP_VEC_ENABLED,
    CMD_CPU_OPTION_COP_VEC_DISABLED,
    CMD_OUT_OF_MEM,
    CMD_UNSUPPORTED_DEVICE,
    CMD_PORT_NUM_INVALID,
    CMD_UART_DISABLED
} cmd_err_t;

// Error message box type
typedef struct cmd_err_msg {
    char *title;
    int win_h;
    int win_w;
    char *msg;
} cmd_err_msg;

// Status values which represent that state of
// command parsing
typedef enum cmd_status_t {
    STAT_OK,
    STAT_INFO,
    STAT_ERR
} cmd_status_t;


#endif


