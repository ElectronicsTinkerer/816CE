/**
 * 65(c)816 simulator/emulator (816CE)
 * Copyright (C) 2023 Ray Clemens
 */

// For sys/stat operations
#define _FILE_OFFSET_BITS 64

// Needed for sigaction
// TODO: Figure out a "correct" number for this
#define _POSIX_C_SOURCE 200000L

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <ncurses.h>
#include <limits.h>

#include <sys/stat.h> // For getting file sizes
#include <signal.h> // For ^Z support
#include <errno.h>

#include "disassembler.h"
#include "../cpu/65816.h"
#include "../cpu/65816-util.h"
#include "../hw/16C750.h"
#include "debugger.h"


// Not a fan of globals but here we are...
volatile bool break_hit = false;

// Messages to print in the status bar at the top of the screen
// Keep in sync with status_msgs
static char *status_msgs[] = {
    "Normal",
    "Press F12 again to exit. Any other key will cancel.",
    "Press q to exit. Any other key will cancel.",
    "Press ^C to exit. Any other key will cancel.",
    "CPU Reset",
    "CPU Crashed - internal error",
    "Running"
};


// Global error message buffer. Used in conjunction
// with the cmd_err_msgs return values.
// To signal use of this buffer, return
// cmd_err_t = CMD_SPECIAL
char global_err_msg_buf[1024];

// Error messages for command parsing/execution
// Keep in sync with the cmd_err_t enum in debugger.h
cmd_err_msg cmd_err_msgs[] = {
    {"",0,0,""}, // OK
    {"ERROR!", 3, 4, global_err_msg_buf},
    {"ERROR!", 3, 34, "Expected argument for command."},
    {"ERROR!", 3, 27, "Expected register name."},
    {"ERROR!", 3, 19, "Expected value."},
    {"ERROR!", 3, 21, "Unknown argument."},
    {"ERROR!", 3, 20, "Unknown command."},
    {"HELP", 18, 43, "Available commands\n"
     " > exit|quit ... Close simulator\n"
     " > mw[1|2] [mem|asm] (pc|addr)\n"
     " > mw[1|2] aaaaaa\n"
     " > irq [set|clear]\n"
     " > nmi [set|clear]\n"
     " > aaaaaa: xx yy zz\n"
     " > save [mem|cpu] filename\n"
     " > load mem (mos) (offset) filename\n"
     " > load cpu filename\n"
     " > cpu [reg] xxxx\n"
     " > cpu [option] [enable|disable|status]\n"
     " > bp aaaaaa\n"
     " > uart [type] aaaaaa (pppp)\n"
     " ? ... Help Menu\n"
     " ^G to clear command input"},
    {"HELP?", 3, 13, "Not help."},
    {"ERROR!", 3, 34, "Unknown character encountered."},
    {"ERROR!", 3, 30, "Overflow in numeric value."},
    {"ERROR!", 3, 22, "Expected filename."},
    {"ERROR!", 3, 24, "Unable to open file."},
    {"ERROR!", 3, 20, "File too large."},
    {"ERROR!", 3, 41, "File will wrap due to offset address."},
    {"ERROR!", 3, 27, "File permission denied."},
    {"ERROR!", 3, 29, "Too many symbolic links."},
    {"ERROR!", 3, 22, "Filename too long."},
    {"ERROR!", 3, 24, "File does not exist."},
    {"ERROR!", 3, 33, "Unhandled file-related error."},
    {"ERROR!", 3, 19, "Corrupt file."},
    {"ERROR!", 4, 40, "Corrupt data format during CPU load.\nCPU may be in an unexpected state."},
    {"INFO",   3, 39, "CPU option cop_vect_enable ENABLED."},
    {"INFO",   3, 40, "CPU option cop_vect_enable DISABLED."},
    {"ERROR!", 3, 44, "Unable to allocate memory for operation."},
    {"ERROR!", 3, 23, "Unsupported device."},
    {"ERROR!", 3, 24, "Invalid port number."},
    {"INFO",   3, 18, "UART disabled."}
};


/**
 * Prints the banner across the top of the screen
 * 
 * @param width The width in characters of the terminal
 * @param status_id The identifier of the interface status message to print
 * @param alert true if the status bar should blink
 */
void print_header(int width, status_t status_id, bool alert)
{
    wmove(stdscr, 0, 0);
    attron(A_REVERSE);
    for (size_t i = 0; i < width; ++i) {
        waddch(stdscr, ' ');
    }
    mvwprintw(stdscr, 0, 0, " 65816 Debugger | ");

    if (alert) {
        attron(A_BLINK);
    }
    wprintw(stdscr, "%s", status_msgs[status_id]);
    if (alert) {
        attroff(A_BLINK);
    }

    attroff(A_REVERSE);
}


/**
 * Print the current state of the CPU's registers at the 
 * location that the cursor currently is on screen
 * 
 * @param *win The curses WINDOW to print the display in
 * @param *cpu The CPU to print
 * @param y The upper left corner of the data display
 * @param x The upper left corner of the data display
 */
void print_cpu_regs(WINDOW *win, CPU_t *cpu, int y, int x)
{
    // Data headers/labels
    wattrset(win, A_NORMAL);
    mvwprintw(win, y, x, "C:   X:   Y:   SP:");
    mvwprintw(win, y+3, x, "DBR: PBR: PC:  D:");
    mvwprintw(win, y+6, x, "RST: IRQ: NMI: STP: CRASH:");
    mvwprintw(win, y, x+22, "NVMXDIZC|E");
    mvwprintw(win, y+3, x+22, "Cycles:");

    // Data itself
    wattron(win, A_BOLD);
    mvwprintw(win, y+1, x, "%04x %04x %04x %04x",
              cpu->C, cpu->X, cpu->Y, cpu->SP);
    mvwprintw(win, y+4, x, "%02x   %02x   %04x %04x",
              cpu->DBR, cpu->PBR, cpu->PC, cpu->D);
    mvwprintw(win, y+7, x, "%d    %d    %d    %d    %d",
              cpu->P.RST, cpu->P.IRQ, cpu->P.NMI, cpu->P.STP, cpu->P.CRASH);
    mvwprintw(win, y+1, x+22, "%d%d%d%d%d%d%d%d|%d",
              cpu->P.N, cpu->P.V, cpu->P.M, cpu->P.XB, cpu->P.D,
              cpu->P.I, cpu->P.Z, cpu->P.C, cpu->P.E);
    mvwprintw(win, y+4, x+22, "%010ld", cpu->cycles);
    wattroff(win, A_BOLD);
}


/**
 * Maintains a history of instructions and register value changes
 * in the CPU. Call this once per CPU step, after the operation
 * has been run.
 * 
 * @param *hist The history structure to reference and update
 * @param *cpu The CPU to conitor
 * @param *mem The memory to use for instruction disassembly
 * @param replace If true, then the last instruction on the history
 *                buffer will be replaced with the current instruction
 */
void update_cpu_hist(hist_t *hist, CPU_t *cpu, memory_t *mem, bool replace)
{
    uint32_t val;
    size_t i;
    int lines;

    // Clear the history list if the CPU was reset
    if (cpu->P.RST) {
        hist->entry_count = 1; // 1 for RESET
        hist->entry_start = 0;
        i = 0;
        wclear(hist->win);
    }
    else if (cpu->P.CRASH) { // Don't increment if CPU has crashed
        return;
    }
    else { // Otherwise, increment the circular buffer's pointer
        lines = CMD_HIST_ENTRIES;
        if (hist->entry_count < lines) {
            i = hist->entry_count;

            if (!replace) {
                ++hist->entry_count;
            } else {
                --i;
            }
        }
        else {
            i = hist->entry_start;

            if (!replace) {
                hist->entry_start += 1;
                hist->entry_start %= lines;
            } else {
                // Subtract 1 from i, wrapping if needed
                i = (i == 0) ? (hist->entry_count - 1) : (i - 1);
            }
        }
    }
    
    // Insert the current CPU state into the array
    memcpy(hist->cpu + i, cpu, sizeof(*cpu));

    // Copy in memory in case there is self-modifying asm code
    hist->mem[i][0].val = _get_mem_byte(mem, _cpu_get_effective_pc(cpu), false);
    val = _cpu_get_immd_long(cpu, mem, false);
    hist->mem[i][1].val = val & 0xff;
    hist->mem[i][2].val = (val >> 8) & 0xff;
    hist->mem[i][3].val = (val >> 16) & 0xff;
}


/**
 * Prints the instruction history text to a window based on
 * data stored in the hist element.
 * 
 * @param *hist The history to print
 */
void print_cpu_hist(hist_t *hist)
{
    size_t i, j, j_1, str_index, row, row_mod, row_prev;
    char buf[100];
    CPU_t *pcpu, *ccpu;
    bool prev_has_diff, curr_has_diff;
    prev_has_diff = false;

    row = hist->win_height - 2;
    row_prev = row;
    j = hist->entry_start;

    // Start with last entry in history table.
    // This handles underflow wrapping
    if (hist->entry_count > 0) {
        j = (j == 0) ? (hist->entry_count - 1) : (j - 1);
    }
    
    for (i = 0; i < hist->entry_count && row > 0; ++i) {

        row_mod = 1;

        if (hist->cpu[j].P.RST) {
            mvwprintw(hist->win, row, 2, ">>> RESET <<<");
        }
        else {
            // Print a diff of the registers
            // This assumes that not "too many" registers change in a given instruction
            // otherwise, it will wrap the text and things won't look nice...

            str_index = 0;
            
            // j-1 with wrapping since the diff prints the previous instruction's changes
            if (hist->entry_count > 0) {
                j_1 = (j == 0) ? (hist->entry_count - 1) : (j - 1);
            }
                
            pcpu = &(hist->cpu[j_1]);
            ccpu = &(hist->cpu[j]);
            if (pcpu->C != ccpu->C) {
                str_index += sprintf(buf + str_index,  " C:%04x->%04x", pcpu->C, ccpu->C);
            }
            if (pcpu->X != ccpu->X) {
                str_index += sprintf(buf + str_index, " X:%04x->%04x", pcpu->X, ccpu->X);
            }
            if (pcpu->Y != ccpu->Y) {
                str_index += sprintf(buf + str_index, " Y:%04x->%04x", pcpu->Y, ccpu->Y);
            }
            if (pcpu->D != ccpu->D) {
                str_index += sprintf(buf + str_index, " D:%04x->%04x", pcpu->D, ccpu->D);
            }
            if (pcpu->DBR != ccpu->DBR) {
                str_index += sprintf(buf + str_index, " DBR:%02x->%02x", pcpu->DBR, ccpu->DBR);
            }
            if (pcpu->PBR != ccpu->PBR) {
                str_index += sprintf(buf + str_index, " PBR:%02x->%02x", pcpu->PBR, ccpu->PBR);
            }
            if (_cpu_get_sr(pcpu) != _cpu_get_sr(ccpu)) {
                str_index += sprintf(buf + str_index, " SR:%02x->%02x", _cpu_get_sr(pcpu), _cpu_get_sr(ccpu));
            }

            curr_has_diff = (str_index > 0);
                
            // Give some margin so that there is not a dangling register
            // diff at the top of the window without an instruction above it.
            if (row >= 3 && curr_has_diff) {
                
                // Clear the line first
                wmove(hist->win, row - 1, 2);
                wclrtoeol(hist->win);
                
                // The print the changes
                mvwprintw(hist->win, row - 1, 4, "%s", buf);
                row_mod = 2;
            }

            if (row >= 2 || !prev_has_diff || (prev_has_diff && row_prev >= 3)) {
                // Print address (PC)
                wattron(hist->win, A_DIM);
                mvwprintw(hist->win, row, 2, "%06x:", hist->cpu[j].PC);
                wattroff(hist->win, A_DIM);

                // Clear rest of line
                wclrtoeol(hist->win);

                // Print the current opcode
                get_opcode_by_addr((memory_t*)&(hist->mem[j]), &(hist->cpu[j]), buf, 0);
                mvwprintw(hist->win, row, 10, "%s", buf);
            }
            else {
                wmove(hist->win, row, 2);
                wclrtoeol(hist->win);
            }

            prev_has_diff = curr_has_diff;
        }

        // Decrement j with wrapping
        if (hist->entry_count > 0) {
            j = (j == 0) ? (hist->entry_count - 1) : (j - 1);
        }
        row_prev = row;
        row -= row_mod;
    }
    
}


/**
 * Check if a string is a decimal string and parse it if so
 * 
 * @param *str The string to parse
 * @param *val A pointer to the variable to store the parsed value in
 * @return false if the string is not decimal (val will not be modified if so)
 *         true if the string is decimal and was successfully parsed
 */
bool is_dec_do_parse(char *str, uint32_t *val)
{
    // Determine if this is actually a hex address
    char *tmp = str;
    while (isdigit(*tmp)) {
        /* scan */
        ++tmp;
    }

    // If the entire argument is hex, use it as an address
    if (*tmp == '\0' || *tmp == '\n') {
        *val = strtoul(str, NULL, 10);
        return true;
    }
    return false;
}


/**
 * Check if a string is a hex string and parse it if so
 * 
 * @param *str The string to parse
 * @param *val A pointer to the variable to store the parsed value in
 * @return false if the string is not hex (val will not be modified if so)
 *         true if the string is hex and was successfully parsed
 */
bool is_hex_do_parse(char *str, uint32_t *val)
{
    // Determine if this is actually a hex address
    char *tmp = str;
    while (isxdigit(*tmp)) {
        /* scan */
        ++tmp;
    }

    // If the entire argument is hex, use it as an address
    if (*tmp == '\0' || *tmp == '\n') {
        *val = strtoul(str, NULL, 16);
        return true;
    }
    return false;
}


/**
 * Load a file into memory
 * 
 * @param *filename The path of the file to load
 * @param *mem The memory to store the data into
 * @param base_addr The base address to load the file at
 * @param memory_format Sets the formatting of the data file to read
 * @return A status code indicating errors if any occur
 */
cmd_err_t load_file_mem(char *filename, memory_t *mem, uint32_t base_addr, memory_fmt_t memory_format)
{
    // Get the size of the file
    struct stat finfo;

    // Lots of error values!
    if (stat(filename, &finfo) != 0) {
        switch (errno) {
        case EACCES:
            return CMD_FILE_PERM_DENIED;
            break;
        case ELOOP:
            return CMD_FILE_LOOP;
            break;
        case ENAMETOOLONG:
            return CMD_FILE_NAME_TOO_LONG;
            break;
        case ENOTDIR:
        case ENOENT:
            return CMD_FILE_NOT_EXIST;
            break;
        default:
            return CMD_FILE_UNKNOWN_ERROR;
        }
    }

    size_t size = finfo.st_size;

    // All good, let's open the file
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return CMD_FILE_IO_ERROR;
    }

    uint8_t *tmp = NULL;
    
    switch (memory_format) {
    default:
    case MF_BASIC_BIN_BLOCK: {
        // Check file size
        if (size / sizeof(*mem) > 0x1000000) {
            return CMD_FILE_TOO_LARGE;
        }

        // Make sure the file won't wrap
        if ((size / sizeof(*mem)) + base_addr > 0x1000000) {
            return CMD_FILE_WILL_WRAP;
        }

        size = size / sizeof(*tmp);

        // We need a temporary buffer since there
        // is a wrapper needed to access the CPU's
        // memory structure
        tmp = malloc(sizeof(*tmp) * size);

        if (!tmp) {
            fclose(fp);
            return CMD_OUT_OF_MEM;
        }

        if (fread(tmp, sizeof(*tmp), size, fp) != size) {
            free(tmp);
            fclose(fp);
            return CMD_FILE_IO_ERROR;
        }

        // Copy data into the memory
        _init_mem_arr(mem, tmp, base_addr, size);
    }
        break;

    case MF_LLVM_MOS_SIM: {
        size_t next_byte_loc = 0;
        size_t max_len = 0;
        int tmp_value = 0;

        while (true) {
            int base_addr_low, base_addr_high, len_low, len_high;
            int base_addr, len;
            if ((base_addr_low = fgetc(fp)) == EOF) {
                // Done with file
                break;
            }
            if ((base_addr_high = fgetc(fp)) == EOF) {
                fclose(fp);
                return CMD_FILE_CORRUPT;
            }
            base_addr = (base_addr_high << 8) | base_addr_low;

            if ((len_low = fgetc(fp)) == EOF) {
                fclose(fp);
                return CMD_FILE_CORRUPT;
            }
            if ((len_high = fgetc(fp)) == EOF) {
                fclose(fp);
                return CMD_FILE_CORRUPT;
            }
            len = (len_high << 8) | len_low;

            if (len == 0) {
                continue;
            }

            printf("\nSection base: %x Section length: %x", base_addr, len);
            
            if (base_addr + len > 0x1000000) {
                fclose(fp);
                return CMD_FILE_WILL_WRAP;
            }
            
            if (max_len < len) {
                // Force memory buffer reallocation
                if (tmp) {
                    free(tmp);
                    tmp = NULL;
                }
                max_len = len;
            }

            if (!tmp) {
                tmp = malloc(sizeof(*tmp) * max_len);
                if (!tmp) {
                    fclose(fp);
                    return CMD_OUT_OF_MEM;
                }
            }

            next_byte_loc = 0;
            while (next_byte_loc < len) {
                tmp_value = fgetc(fp);
                if (tmp_value == EOF) {
                    free(tmp);
                    fclose(fp);
                    return CMD_FILE_CORRUPT;
                }
                tmp[next_byte_loc] = tmp_value;

                next_byte_loc++;
            }

            // Copy data into the memory
            _init_mem_arr(mem, tmp, base_addr, len);
        }
    }
        break;
    }

    fclose(fp);

    if (tmp) {
        free(tmp);
    }

    return CMD_OK;
}


/**
 * Load a file's contents into the CPU state
 * 
 * @param *filename The path of the file to load
 * @param *cpu The CPU to save the file's contents into
 * @return The error status of the load operation
 */
cmd_err_t load_file_cpu(char *filename, CPU_t *cpu)
{
    // Get the size of the file
    struct stat finfo;

    // Lots of error values!
    if (stat(filename, &finfo) != 0) {
        switch (errno) {
        case EACCES:
            return CMD_FILE_PERM_DENIED;
            break;
        case ELOOP:
            return CMD_FILE_LOOP;
            break;
        case ENAMETOOLONG:
            return CMD_FILE_NAME_TOO_LONG;
            break;
        case ENOTDIR:
        case ENOENT:
            return CMD_FILE_NOT_EXIST;
            break;
        default:
            return CMD_FILE_UNKNOWN_ERROR;
        }
    }
    
    size_t size = finfo.st_size;

    if (size > 1024) { // Arbitrary max file size limit (should not need to be increased!)
        return CMD_FILE_TOO_LARGE;
    }

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        return CMD_FILE_IO_ERROR;
    }

    char buf[size];
    fread(&buf, sizeof(*buf), size, fp);

    if (fromstrCPU(cpu, (char*)&buf) != CPU_ERR_OK) {
        return CMD_CPU_CORRUPT_FILE;
    }
    fclose(fp);
    return CMD_OK;
}


/**
 * Clear the command input buffer and onscreen text
 * 
 * @param *win The command window
 * @param *cmdbuf The command input buffer
 * @param *cmdbuf_index The current command input buffer index
 */
void command_clear(WINDOW *win, char *cmdbuf, size_t *cmdbuf_index)
{
    wmove(win, 1, CMD_DISP_X_OFFS);
    wclrtoeol(win);

    for (size_t i = 1; i < MAX_CMD_LEN + 1; ++i) {
        cmdbuf[i] = '\0';
    }
    *cmdbuf_index = 0;

    // Redraw cursor
    wattron(win, A_BOLD);
    mvwaddch(win, 1, CMD_DISP_X_OFFS, '_');
    wattroff(win, A_BOLD);

    wattron(win, A_DIM);
    wprintw(win, " ? to view command list");
    wattroff(win, A_DIM);
}


/**
 * Send a command to the command window
 * 
 * @param *win The curses window for the command entry line
 * @param *cmdbuf The current command buffer
 * @param cmdbuf_index The index into the current command buffer
 * @param c The last typed character
 * @return true if command should be executed
 *         false if command not complete
 */
bool command_entry(WINDOW *win, char *cmdbuf, size_t *cmdbuf_index, int c)
{
    if (c == KEY_CR) {
        cmdbuf[*cmdbuf_index] = '\0';
        return true;
    }

    // Make sure the command line is cleared if typing
    if (*cmdbuf_index == 0) {
        wmove(win, 1, CMD_DISP_X_OFFS);
        wclrtoeol(win);
    }

    wattron(win, A_BOLD);

    if (c == KEY_BACKSPACE || c == KEY_CTRL_H || c == KEY_DELETE) {
        if (*cmdbuf_index > 0) {
            mvwaddch(win, 1, (*cmdbuf_index) + CMD_DISP_X_OFFS, ' '); // Erase cursor
            cmdbuf[*cmdbuf_index] = '\0';
            --*cmdbuf_index;
        }
    }
    else if (*cmdbuf_index < MAX_CMD_LEN - 1) {
        cmdbuf[*cmdbuf_index] = (char)c;
        mvwaddch(win, 1, (*cmdbuf_index) + CMD_DISP_X_OFFS, cmdbuf[*cmdbuf_index]);
        ++*cmdbuf_index;
    }

    mvwaddch(win, 1, (*cmdbuf_index) + CMD_DISP_X_OFFS, '_');

    wattroff(win, A_BOLD);

    // Print basic help
    if (*cmdbuf_index == 0) {
        wattron(win, A_DIM);
        wprintw(win, " ? to view command list");
        wattroff(win, A_DIM);
    }
    return false;
}


/**
 * Parse and execute a command for a memory watch modifier
 * 
 * @param *watch The watch structure to control
 * @param tok The input command buffer
 * @return The error code from execution of the command
 */
cmd_err_t command_execute_watch(watch_t *watch, char* tok)
{
    tok = strtok(NULL, " \t\n\r");

    if (!tok) {
        return CMD_EXPECTED_ARG;
    }

    // Secondary level command
    if (strcmp(tok, "mem") == 0) {
        watch->disasm_mode = false;
    }
    else if (strcmp(tok, "asm") == 0) {
        watch->disasm_mode = true;
        wclear(watch->win);
    }
    else {
        // Check if user is setting watch start address
        if (is_hex_do_parse(tok, &(watch->addr_s))) {

            if (watch->addr_s > 0xffffff) {
                return CMD_VAL_OVERFLOW;
            }
        }
        else {
            return CMD_UNKNOWN_ARG; // Syntax error!
        }
    }

    tok = strtok(NULL, " \t\n\r");
        
    if (!tok) {
        return CMD_OK;
    }
        
    // Tertiary level command
    if (strcmp(tok, "pc") == 0) {
        watch->follow_pc = true;
    }
    else if (strcmp(tok, "addr") == 0) {
        watch->follow_pc = false;
    }
    else {
        return CMD_UNKNOWN_ARG; // Error!
    }

    return CMD_OK; // Success?
}


/**
 * Execute a command from the prompt window.
 * This has a very primitive parser.
 * 
 * @param *status The error code from the command
 * @param *_cmdbuf The buffer containing the command
 * @param cmdbuf_index The length of the command + 1
 * @param *watch1 Watch window 1 structure
 * @param *watch2 Watch window 2 structure
 * @param *cpu The CPU to modify if a command needs to
 * @param *mem The memory to modify if a command needs to
 * @return True if an error occured, false otherwise
 */
cmd_status_t command_execute(cmd_err_t *status, char *_cmdbuf, int cmdbuf_index, watch_t *watch1, watch_t *watch2, CPU_t *cpu, memory_t *mem, tl16c750_t *uart)
{
    if (cmdbuf_index == 0) {
        *status = CMD_OK; // No command
        return STAT_OK; // No Error
    }

    while (isspace(*_cmdbuf)) {
        ++_cmdbuf; // Remove whitespace
    }

    // Make string lowercase
    char *buf_ptr = _cmdbuf;
    while (*buf_ptr) {
        *buf_ptr = tolower(*buf_ptr);
        ++buf_ptr;
    }
    
    char *tok = strtok(_cmdbuf, " \t\n\r");

    if (!tok) {
        *status = CMD_OK; // No command
        return STAT_OK; // No Error
    }

    if (*tok == '#') { // Ignore comments
        *status = CMD_OK;
        return STAT_OK; // No Error
    }
    
    if (strcmp(tok, "?") == 0) { // Main level help
        *status = CMD_HELP_MAIN;
        return STAT_INFO;
    }
    else if (strcmp(tok, "???") == 0) { // Not help
        *status = CMD_HELP_NOT;
        return STAT_INFO;
    }
    else if (strcmp(tok, "irq") == 0) { // Force IRQ status change

        tok = strtok(NULL, " \t\n\r");

        if (!tok) {
            *status = CMD_EXPECTED_ARG;
            return STAT_ERR;
        }

        // Secondary level command
        if (strcmp(tok, "set") == 0) {
            cpu->P.IRQ = 1;
        }
        else if (strcmp(tok, "clear") == 0) {
            cpu->P.IRQ = 0;
        }
        else {
            *status = CMD_UNKNOWN_ARG;
            return STAT_ERR;
        }
        *status = CMD_OK;
        return STAT_OK;
    }
    else if (strcmp(tok, "nmi") == 0) { // Force NMI status change

        tok = strtok(NULL, " \t\n\r");

        if (!tok) {
            *status = CMD_EXPECTED_ARG;
            return STAT_ERR;
        }

        // Secondary level command
        if (strcmp(tok, "set") == 0) {
            cpu->P.NMI = 1;
        }
        else if (strcmp(tok, "clear") == 0) {
            cpu->P.NMI = 0;
        }
        else {
            *status = CMD_UNKNOWN_ARG;
            return STAT_ERR;
        }
        *status = CMD_OK;
        return STAT_OK;
    }
    else if (strcmp(tok, "mw1") == 0) { // Memory Watch 1
        return command_execute_watch(watch1, tok);
    }
    else if (strcmp(tok, "mw2") == 0) { // Memory Watch 2
        return command_execute_watch(watch2, tok);
    }
    else if (strcmp(tok, "exit") == 0 ||
             strcmp(tok, "quit") == 0) { // Exit
        *status = CMD_EXIT;
        return STAT_OK;
    }
    else if (strcmp(tok, "save") == 0) { // Save

        tok = strtok(NULL, " \t\n\r");
        
        if (!tok) {
            *status = CMD_EXPECTED_ARG;
            return STAT_ERR;
        }

        char *filename = strtok(NULL, " \t\n\r");

        if (!filename) {
            *status = CMD_EXPECTED_FILENAME;
            return STAT_ERR;
        }

        // Secondary level command
        if (strcmp(tok, "mem") == 0) {

            FILE *fp = fopen(filename, "wb");
            if (!fp) {
                *status = CMD_FILE_IO_ERROR;
                return STAT_ERR;
            }

            uint8_t *tmp = malloc(sizeof(*tmp) * MEMORY_SIZE);

            if (!tmp) {
                fclose(fp);
                *status = CMD_OUT_OF_MEM;
                return STAT_ERR;
            }

            // Get the memory contents
            _save_mem_arr(mem, tmp, 0, MEMORY_SIZE);
            
            if (fwrite(tmp, sizeof(*tmp), MEMORY_SIZE, fp) != MEMORY_SIZE) {
                free(tmp);
                *status = CMD_FILE_IO_ERROR;
                return STAT_ERR;
            }
            fclose(fp);

            free(tmp);
        }
        else if (strcmp(tok, "cpu") == 0) { // Write the CPU directly to a file

            FILE *fp = fopen(filename, "w");
            if (!fp) {
                *status = CMD_FILE_IO_ERROR;
                return STAT_ERR;
            }
            char buf[160];
            tostrCPU(cpu, (char*)&buf);
            fprintf(fp, "%s", buf);
            fclose(fp);
        }
        else {
            *status = CMD_EXPECTED_ARG;
            return STAT_ERR;
        }

        *status = CMD_OK;
        return STAT_OK;
    }
    else if (strcmp(tok, "load") == 0) {

        tok = strtok(NULL, " \t\n\r");
        
        if (!tok) {
            *status = CMD_EXPECTED_ARG;
            return STAT_ERR;
        }

        // Secondary level command
        if (strcmp(tok, "mem") == 0) {

            tok = strtok(NULL, " \t\n\r");
        
            if (!tok) {
                *status = CMD_EXPECTED_ARG;
                return STAT_ERR;
            }

            memory_fmt_t mf = MF_BASIC_BIN_BLOCK;
            
            if (strcmp(tok, "mos") == 0) {
                tok = strtok(NULL, " \t\n\r");
                mf = MF_LLVM_MOS_SIM;
            }

            uint32_t base_addr = 0;

            // If a load offset is given, parse it
            if (is_hex_do_parse(tok, &base_addr)) {

                if (base_addr > 0xffffff) {
                    *status = CMD_VAL_OVERFLOW;
                    return STAT_ERR;
                }

                tok = strtok(NULL, " \t\n\r");

                if (!tok) {
                    *status = CMD_EXPECTED_FILENAME;
                    return STAT_ERR;
                }
            }

            *status = load_file_mem(tok, mem, base_addr, mf);

            if (*status != CMD_OK) {
                return STAT_ERR;
            } else {
                return STAT_OK;
            }
        }
        else if (strcmp(tok, "cpu") == 0) { // Write the CPU directly to a file

            // Get filename
            tok = strtok(NULL, " \t\n\r");

            if (!tok) {
                *status = CMD_EXPECTED_FILENAME;
                return STAT_ERR;
            }

            return load_file_cpu(tok, cpu);
        }
        else {
            *status = CMD_UNKNOWN_ARG;
            return STAT_ERR;
        }

        *status = CMD_OK;
        return STAT_OK;
    }
    else if (strcmp(tok, "cpu") == 0) {

        tok = strtok(NULL, " \t\n\r");

        if (!tok) {
            *status = CMD_EXPECTED_REG;
            return STAT_ERR;
        }

        // Check for COP option
        if (strcmp(tok, "cop") == 0) {

            tok = strtok(NULL, " \t\n\r");

            // If no arg given, report the current status
            // of the COP option
            if (!tok) {
                *status = CMD_EXPECTED_ARG;
                return STAT_ERR;
            }

            if (strcmp(tok, "enable") == 0) {
                cpu->cop_vect_enable = true;
                *status = CMD_CPU_OPTION_COP_VEC_ENABLED;
                return STAT_INFO;
            }
            else if (strcmp(tok, "disable") == 0) {
                cpu->cop_vect_enable = true;
                *status = CMD_CPU_OPTION_COP_VEC_DISABLED;
                return STAT_INFO;
            }
            else if (strcmp(tok, "status") == 0) {
                if (cpu->cop_vect_enable) {
                    *status = CMD_CPU_OPTION_COP_VEC_ENABLED;
                    return STAT_INFO;
                } else {
                    *status = CMD_CPU_OPTION_COP_VEC_DISABLED;
                    return STAT_INFO;
                }
            }
            *status = CMD_UNKNOWN_ARG;
            return STAT_ERR;
        }

        // Else, it's a register assignment
        
        char *hexval = strtok(NULL, " \t\n\r");

        if (!hexval) {
            *status = CMD_EXPECTED_VALUE;
            return STAT_ERR;
        }

        uint32_t val = 0;
        
        // Make sure it's hex
        if (!is_hex_do_parse(hexval, &val)) {
            *status = CMD_EXPECTED_VALUE;
            return STAT_ERR;
        }

        if (val > 0xffffff) {
            *status = CMD_VAL_OVERFLOW;
            return STAT_ERR;
        }

        if (strcmp(tok, "c") == 0) {
            if (val > 0xffff) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->C = val;
        }
        else if (strcmp(tok, "x") == 0) {
            if (val > 0xffff) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->X = val;
        }
        else if (strcmp(tok, "y") == 0) {
            if (val > 0xffff) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->Y = val;
        }
        else if (strcmp(tok, "sp") == 0) {
            if (val > 0xffff) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->SP = val;
        }
        else if (strcmp(tok, "dbr") == 0) {
            if (val > 0xff) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->DBR = val;
        }
        else if (strcmp(tok, "pbr") == 0) {
            if (val > 0xff) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->PBR = val;
        }
        else if (strcmp(tok, "pc") == 0) {
            if (val > 0xffff) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->PC = val;
        }
        else if (strcmp(tok, "d") == 0) {
            if (val > 0xffff) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->D = val;
        }
        else if (strcmp(tok, "p") == 0) {
            if (val > 0xff) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            _cpu_set_sp(cpu, (uint8_t)val);
        }
        else if (strcmp(tok, "p.n") == 0) {
            if (val > 0x1) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->P.N = val;
        }
        else if (strcmp(tok, "p.v") == 0) {
            if (val > 0x1) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->P.V = val;
        }
        else if (strcmp(tok, "p.m") == 0) {
            if (val > 0x1) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->P.M = val;
        }
        else if (strcmp(tok, "p.x") == 0) {
            if (val > 0x1) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->P.XB = val;
        }
        else if (strcmp(tok, "p.d") == 0) {
            if (val > 0x1) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->P.D = val;
        }
        else if (strcmp(tok, "p.i") == 0) {
            if (val > 0x1) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->P.I = val;
        }
        else if (strcmp(tok, "p.z") == 0) {
            if (val > 0x1) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->P.Z = val;
        }
        else if (strcmp(tok, "p.c") == 0) {
            if (val > 0x1) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->P.C = val;
        }
        else if (strcmp(tok, "p.e") == 0) {
            if (val > 0x1) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->P.E = val;
        }
        else if (strcmp(tok, "rst") == 0) {
            if (val > 0x1) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->P.RST = val;
        }
        else if (strcmp(tok, "irq") == 0) {
            if (val > 0x1) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->P.IRQ = val;
        }
        else if (strcmp(tok, "nmi") == 0) {
            if (val > 0x1) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->P.NMI = val;
        }
        else if (strcmp(tok, "stp") == 0) {
            if (val > 0x1) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->P.STP = val;
        }
        else if (strcmp(tok, "crash") == 0) {
            if (val > 0x1) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->P.CRASH = val;
        }
        else if (strcmp(tok, "cycles") == 0) {
            if (val > 0x80000000) {
                *status = CMD_VAL_OVERFLOW;
                return STAT_ERR;
            }
            cpu->cycles = val;
        }
        else {
            *status = CMD_UNKNOWN_ARG;
            return STAT_ERR;
        }
        *status = CMD_OK;
        return STAT_OK;
    }
    else if (strcmp(tok, "b") == 0 ||
             strcmp(tok, "br") == 0 ||
             strcmp(tok, "bre") == 0 ||
             strcmp(tok, "break") == 0) { // Break point

        tok = strtok(NULL, " \t\n\r");

        if (!tok) {
            *status = CMD_EXPECTED_VALUE;
            return STAT_ERR;
        }

        uint32_t addr;

        if (!is_hex_do_parse(tok, &addr)) {
            *status = CMD_EXPECTED_VALUE;
            return STAT_ERR;
        }

        // Toggle breakpoint
        if (_test_mem_flags(mem, addr).B == 0) {
            _set_mem_flags(mem, addr, MEM_FLAG_B);
        }
        else {
            _reset_mem_flags(mem, addr, MEM_FLAG_B);
        }

        *status = CMD_OK;
        return STAT_OK;
    }
    else if (strcmp(tok, "uart") == 0) {

        tok = strtok(NULL, " \t\n\r");

        if (!tok) {
            *status = CMD_EXPECTED_ARG;
            return STAT_ERR;
        }

        char *tmp = strtok(NULL, " \t\n\r");

        if (!tmp) {
            *status = CMD_EXPECTED_VALUE;
            return STAT_ERR;
        }

        // Get base address for UART device
        uint32_t addr;

        if (!is_hex_do_parse(tmp, &addr)) {
            *status = CMD_EXPECTED_VALUE;
            return STAT_ERR;
        }

        // Get port for UART to listen on (for network connections)
        // Optional parameter
        tmp = strtok(NULL, " \t\n\r");
        
        uint32_t port;

        if (!tmp) {
            port = UART_SOCK_PORT;
        } else if (!is_dec_do_parse(tmp, &port)) {
            *status = CMD_EXPECTED_VALUE;
            return STAT_ERR;
        } // else is covered by is_dec_do_parse call

        if (port > 0xffff) { // Max port number since ports are 16-bit
            *status = CMD_PORT_NUM_INVALID;
            return STAT_ERR;
        }

        if (strcmp(tok, "c750") == 0) {

            uart->addr = addr;

            int err;
            if ((err = init_port_16c750(uart, port))) {
                sprintf(global_err_msg_buf, "%s (port: %d)", strerror(err), port);
                uart->enabled = false;
                
                *status = CMD_SPECIAL;
                return STAT_ERR;
            }

            // If port is 0, disable UART
            if (port == 0) {
                uart->enabled = false;
                
                *status = CMD_UART_DISABLED;
                return STAT_INFO;
            }
            
            uart->enabled = true;
            
            *status = CMD_OK;
            return STAT_OK;
        }
        else {
            *status = CMD_UNSUPPORTED_DEVICE;
            return STAT_ERR;
        }
    }

    // Not a named command, maybe it's a memory access?
    static uint32_t addr = 0; // Retain the previous value
    uint32_t val;
    char *next = _cmdbuf;
    bool found_store_delim = false;
    bool valid_character = true;

    // Remove null char from strtok()
    _cmdbuf[strlen(tok)] = ' ';
    
    while ((next != NULL) && (*next != '\0') && valid_character) {

        if (isspace(*next)) {
            ++next; // Eat whitespace
        }
        else if (*next == ':') {
            found_store_delim = true;
            ++next;
        }
        else {
            val = strtoul(next, &next, 16);
            if (!found_store_delim) {
                addr = val;
            }
            else {
                _set_mem_byte(mem, addr, val, true); // DO tell I/O devices that a value has been written here!
                ++addr;
            }
        }
        valid_character = isxdigit(*next) || isspace(*next) || (*next == ':') || (*next == '\0');
    }

    if (!valid_character && found_store_delim) {
        *status = CMD_INVALID_CHAR;
        return STAT_ERR;
    }
    else if (valid_character && found_store_delim) {
        *status = CMD_OK;
        return STAT_OK;
    }

    *status = CMD_UNKNOWN_CMD; // Not success
    return STAT_ERR;
}


/**
 * Prints the memory in the window for the watch 
 * 
 * @param *w The watch to use
 * @param *mem The CPU's memory to view
 * @param *cpu, The CPU to use
 */
void mem_watch_print(watch_t *w, memory_t *mem, CPU_t *cpu)
{
    size_t cols, col, row;
    uint32_t i, pc;
    char buf[32];
    
    pc = _cpu_get_effective_pc(cpu);

    cols = (w->win_width - 10) / 3;
    if (cols < 16) {
        cols = 8;
    } else if (cols < 32) {
        cols = 16;
    } else {
        cols = 32; // WIDE! screen
    }

    if (w->disasm_mode) { // Show disassembly
        
        CPU_t cpu_dup = *cpu;
        uint32_t effective_pc;
    
        if (!w->follow_pc) {
            cpu_dup.PC = (uint16_t)w->addr_s;
            cpu_dup.PBR = (uint8_t)((w->addr_s >> 16) & 0xff);
        }
        else {
            cpu_dup.PC = cpu->PC;
            cpu_dup.PBR = cpu->PBR;
        }

        // Print as many lines as will fit in the window
        i = _cpu_get_effective_pc(&cpu_dup);
        for (row = 1; row < w->win_height - 1; ++row) {

            i = _addr_add_val_bank_wrap(i, get_opcode(mem, &cpu_dup, buf));
            effective_pc = _cpu_get_effective_pc(&cpu_dup);
            
            wmove(w->win, row, 1);
            wclrtoeol(w->win);

            // Print break points as '@'
            if (_test_mem_flags(mem, effective_pc).B == 1) {
                wattron(w->win, A_BOLD);
                mvwprintw(w->win, row, 1, "@");
                wattroff(w->win, A_BOLD);
            }
            
            // Print the address
            wattron(w->win, (effective_pc == pc) ? A_BOLD : A_DIM);
            mvwprintw(w->win, row, 2, "%06x:", effective_pc);
            wattroff(w->win, (effective_pc == pc) ? A_BOLD : A_DIM);

            // Print the instruction
            mvwprintw(w->win, row, 10, "%s", buf);

            // Print the bytes
            if (cols > 8) {
                wmove(w->win, row, 24);
                while (effective_pc < i) {
                    wprintw(w->win, " %02x", _get_mem_byte(mem, effective_pc, false));

                    cpu_dup.PC = _addr_add_val_bank_wrap(cpu_dup.PC, 1);
                    effective_pc = _cpu_get_effective_pc(&cpu_dup);
                }
            }
            else {
                cpu_dup.PC = i;
            }
        }
    }
    else {     // Just show memory contents
        // Print the column numbers across the top
        wmove(w->win, 1, 1);
        wclrtoeol(w->win);
        wmove(w->win, 1, 9);
        wattron(w->win, A_DIM);
        for (i = 0, col = 0; col < cols; ++col, ++i) {
            wprintw(w->win, " ");
            wprintw(w->win, "%02x", i);
        }
        wattroff(w->win, A_DIM);

        // Since cols will always be a multiple of 8, we
        // can subtract 1 and use it's complement as a mask
        // for making sure that we start on an address that
        // is a multiple of the number of columns (this keeps
        // the column address numbers that we just printed
        // aligned with the correct locations)
        i = (w->follow_pc ? pc : w->addr_s) & ~(cols-1);

        // Then print the actual memory contents
        for (row = 1; row < w->win_height - 2; ++row) {
            wattron(w->win, A_DIM);
            mvwprintw(w->win, 1 + row, 2, "%06x:", i);
            wattroff(w->win, A_DIM);
            for (col = 0; col < cols; ++col) {
                wprintw(w->win, " ");

                // If the CPU's PC is at this address, highlight it
                if (i == pc) {
                    wattron(w->win, A_BOLD | A_UNDERLINE);
                }
                wprintw(w->win, "%02x", _get_mem_byte(mem, i, false));
                if (i == pc) {
                    wattroff(w->win, A_BOLD | A_UNDERLINE);
                }

                i = (i + 1) & 0xffffff;
            }
        }
    }
}


/**
 * Create a message box
 * 
 * @param *win The message box window
 * @param *msg The message to put in the window
 * @param *title The title of the message box
 * @param height The height of the message box window
 * @param width The width of the message box window
 * @param scrh The height of the screen
 * @param scrw The width of the screen
 */
void msg_box(WINDOW **win, char *msg, char *title, int height, int width, int scrh, int scrw)
{
    *win = newwin(height, width, (scrh/2)-(height/2), (scrw/2)-(width/2));

    wrefresh(*win);
    wattron(*win, A_BOLD);

    box(*win, 0, 0);

    // Title
    mvwprintw(*win, 0, 2, " %s ", title);
    wattroff(*win, A_BOLD);

    // User message
    // Need to malloc and copy since string literals
    // are stored in RO region of memory
    char *msg_dup = malloc(sizeof(*msg) * strlen(msg));
    strcpy(msg_dup, msg);
    
    char *tok = strtok(msg_dup, "\n"); // Print each line separately
    int row = 1;
    while (tok) {
        mvwprintw(*win, row, 2, "%s", tok);
        tok = strtok(NULL, "\n");
        ++row;
    }

    free(msg_dup);

    // OK "box"
    wattron(*win, A_REVERSE);
    mvwprintw(*win, height-1, width-6, " OK ");
    wattroff(*win, A_REVERSE);
}


/**
 * Initialize a watch struct
 * 
 * @param *w The watch struct to initialize
 * @param disasm_mode True to switch to disassembly mode
 * @param follow_pc True to follow the CPU's PC
 */
void watch_init(watch_t *w, bool disasm_mode, bool follow_pc)
{
    w->win = NULL;
    w->addr_s = 0;
    w->win_height = 0;
    w->win_width = 0;
    w->disasm_mode = disasm_mode;
    w->follow_pc = follow_pc;
}


/**
 * Initialize a history struct to be in a cleared state
 * 
 * @param *h The history struct to clean
 */
void hist_init(hist_t *h)
{
    h->win = NULL;
    h->win_height = 0;
    h->win_width = 0;
    h->entry_count = 0;
    h->entry_start = 0;
    memset(&(h->cpu), 0, sizeof(h->cpu));
    memset(&(h->mem), 0, sizeof(h->mem));
}


void print_help_and_exit()
{
    printf(
        "65816 Simulator (C) Ray Clemens 2022-2023\n"
        "USAGE:\n"
        " $ 816ce (--cpu filename) (--mem (offset) filename)\n"
        "\n"
        "Args:\n"
        " --cpu-file filename ...... Preload the CPU with a saved state\n"
        " --mem (offset) filename .. Load memory at offset (in hex) with a file\n"
        " --mem-mos filename ....... Load a binary file formatted for the LLVM MOS simulator into memory\n"
        " --cmd \"[command here]\" . Run a command during initialization\n"
        " --cmd-file filename ...... Run commands from a file during initialization\n"
        "\n"
        );
    exit(EXIT_SUCCESS);
}


void handle_suspend(int signal)
{
    struct sigaction sigact;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = SIG_DFL;
    sigemptyset(&sigact.sa_mask);
    sigaction(SIGTSTP, &sigact, NULL);

    // Stop curses then send the suspend signal to ourself
    endwin();
    fprintf(stderr, "\nSimulator be returned to with 'fg'\n");
    raise(SIGTSTP);
}


void handle_continue(int signal)
{
    struct sigaction sigact;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = handle_suspend;
    sigemptyset(&sigact.sa_mask);
    sigaction(SIGTSTP, &sigact, NULL);

    // Start back up curses mode
    initscr();
    refresh();
}


void handle_break(int signal)
{
    break_hit = true;
}


int main(int argc, char *argv[])
{
    int c, prev_c;          // User key press (c = current, prev_c = previous)
    int scrw, scrh;         // Width and height of screen
    status_t status_id = STATUS_NONE;
    bool alert = true;
    bool cmd_exit = false;
    bool in_run_mode = false;
    int run_mode_step_count = 0;
    WINDOW *win_cpu, *win_cmd, *win_msg = NULL;
    char cmdbuf[MAX_CMD_LEN];
    char cmdbuf_dup[MAX_CMD_LEN];
    char *_cmdbuf = cmdbuf;
    char *_cmdbuf_dup = cmdbuf_dup;
    size_t cmdbuf_index = 0;
    cmd_err_t cmd_err;
    cmd_status_t cmd_stat;
    struct sigaction sigact;
    watch_t watch1, watch2;
    watch_init(&watch1, false, false);
    watch_init(&watch2, true, true); // Disasm & follow PC

    hist_t inst_hist;
    hist_init(&inst_hist);

    CPU_t cpu;
    initCPU(&cpu);
    resetCPU(&cpu);
    cpu.setacc = true; // Enable CPU to update access flags

    tl16c750_t uart;
    init_16c750(&uart);
    uart.enabled = false;

    memory_t *memory = calloc(MEMORY_SIZE, sizeof(*memory));

    if (!memory) {
        printf("Unable to allocate system memory!\n");
        exit(EXIT_FAILURE);
    }

    // Command line parsing
    printf("Loading simulator...\n");

    {
        uint32_t base_addr = 0;
        int cli_pstate = 0;
        for (size_t i = 1; i < argc; ++i) {
            switch (cli_pstate) {
            case 0:
                if (strcmp(argv[i], "--cpu-file") == 0) {
                    cli_pstate = 1;
                }
                else if (strcmp(argv[i], "--mem") == 0) {
                    cli_pstate = 2;
                }
                else if (strcmp(argv[i], "--mem-mos") == 0) {
                    cli_pstate = 5;
                }
                else if (strcmp(argv[i], "--cmd") == 0) {
                    cli_pstate = 3;
                }
                else if (strcmp(argv[i], "--cmd-file") == 0) {
                    cli_pstate = 4;
                }
                else if (strcmp(argv[i], "--help") == 0) {
                    print_help_and_exit();
                }
                else {
                    printf("Unknown argument: '%s'\n", argv[i]);
                    exit(EXIT_FAILURE);
                }
                break;
            case 1: // CPU load
                if ((cmd_err = load_file_cpu(argv[i], &cpu)) > 0) {
                    printf("Error! (%s) %s\n", argv[i], cmd_err_msgs[cmd_err].msg);
                    exit(EXIT_FAILURE);
                }
                cli_pstate = 0;
                break;
            case 2: // MEM load
                // If the argument is hex, use it as an address
                // Otherwise just load the file
                if (!is_hex_do_parse(argv[i], &base_addr)) {
                    if ((cmd_err = load_file_mem(argv[i], memory, base_addr, MF_BASIC_BIN_BLOCK)) > 0) {
                        printf("Error! (%s) %s\n", argv[i], cmd_err_msgs[cmd_err].msg);
                        exit(EXIT_FAILURE);
                    }
                    cli_pstate = 0;
                }
                break;
            case 5:
                // Takes and loads an executable formatted for the llvm-mos simulator
                if ((cmd_err = load_file_mem(argv[i], memory, base_addr, MF_LLVM_MOS_SIM)) > 0) {
                    printf("Error! (%s) %s\n", argv[i], cmd_err_msgs[cmd_err].msg);
                    exit(EXIT_FAILURE);
                }
                cli_pstate = 0;
                break;
            case 3: // Execute a command directly
                cmd_stat = command_execute(
                    &cmd_err,
                    argv[i],
                    strlen(argv[i]),
                    &watch1, &watch2,
                    &cpu,
                    memory,
                    &uart
                    );

                if (cmd_stat != STAT_OK) {

                    if (cmd_err == CMD_EXIT) {
                        printf("'exit' encountered.\n");
                        exit(EXIT_SUCCESS);
                    }
                    else if (cmd_stat == STAT_INFO) {
                        // Print info messages
                        printf("Info (%s) %s\n", argv[i], cmd_err_msgs[cmd_err].msg);
                    }
                    else {
                        // If there is errors, print an appropiate message
                        printf("Error! (%s) %s\n", argv[i], cmd_err_msgs[cmd_err].msg);
                        exit(EXIT_FAILURE);
                    }
                }
                cli_pstate = 0;
                break;
            case 4: { // Execute each line in a file as a command

                FILE *fp = fopen(argv[i], "r");

                if (!fp) {
                    printf("Error! Unable to open file '%s':\n%s\n", argv[i], strerror(errno));
                    exit(EXIT_FAILURE);
                }

                char buf[2048]; // I hope that's long enough!

                while (!feof(fp)) {

                    // Get command
                    fgets(buf, 2048, fp);

                    // Parse command
                    cmd_stat = command_execute(
                        &cmd_err,
                        buf,
                        strlen(buf),
                        &watch1, &watch2,
                        &cpu,
                        memory,
                        &uart
                        );

                    if (cmd_stat != STAT_OK) {

                        if (cmd_err == CMD_EXIT) {
                            printf("'exit' encountered.\n");
                            exit(EXIT_SUCCESS);
                        }
                        else if (cmd_stat == STAT_INFO) {
                            // Print info messages
                            printf("Info (%s, %s) %s\n", argv[i], buf, cmd_err_msgs[cmd_err].msg);
                        }
                        else {
                            // If there is errors, print an appropiate message
                            printf("Error! (%s) %s\n", buf, cmd_err_msgs[cmd_err].msg);
                            exit(EXIT_FAILURE);
                        }
                    }
                }

                fclose(fp);

                cli_pstate = 0;
            }
                break;
            default:
                printf(
                    "Internal cli parser error!\ni=%ld, argv[%ld]='%s', cli_pstate=%d\n",
                    i, i, argv[i], cli_pstate);
                exit(EXIT_FAILURE);
                break;
            }
            // printf("i=%ld, argv[%ld]='%s', cli_pstate=%d\n",
                    // i, i, argv[i], cli_pstate);
        }

        if (cli_pstate != 0) {
            printf("Mising argument to --");
            switch (cli_pstate) {
            case 1: // CPU load
                printf("cpu\n");
                break;
            case 2: // MEM load
                printf("mem\n");
                break;
            case 3: // CMD execute
                printf("cmd\n");
                break;
            case 4: // CMD file execute
                printf("cmd_file\n");
                break;
            case 5: // MEM load, but in the LLVM-MOS simulator format
                printf("exe\n");
                break;
            default:
                printf("Unhandled cli_pstate in missing arg handler\n");
                break;
            }
            exit(EXIT_FAILURE);
        }
    }

    // Trap SIGINT and SIGTSTP
    memset(&sigact, 0, sizeof(struct sigaction));
    sigact.sa_handler = handle_break;
    sigaction(SIGINT, &sigact, NULL);

    sigact.sa_handler = handle_suspend;
    sigaction(SIGTSTP, &sigact, NULL);

    sigact.sa_handler = handle_continue;
    sigaction(SIGCONT, &sigact, NULL);
    
    initscr();              // Start curses mode
    getmaxyx(stdscr, scrh, scrw); // Get screen dimensions
    cbreak();               // Disable line buffering but pass through signals (ex. ^C/^Z)
    keypad(stdscr, TRUE);   // Enable handling of function and other special keys
    noecho();               // Disable echoing of user-typed characters
    leaveok(stdscr, TRUE);  // Don't care where the cursor is left on screen
    curs_set(0);            // Invisible cursor

    // Set up each window (height, width, starty, startx)
    watch1.win_height = scrh/2 - 1;
    watch1.win_width = scrw/2;
    watch2.win_height = ceil(scrh/2.0);
    watch2.win_width = scrw/2;
    watch1.win = newwin(watch1.win_height, watch1.win_width, 1, 0);
    watch2.win = newwin(watch2.win_height, watch2.win_width, scrh/2, 0);
    win_cpu = newwin(10, scrw/2, 1, scrw/2);
    win_cmd = newwin(3, scrw/2, scrh-3, scrw/2);
    inst_hist.win_height = scrh - 11 - 3;
    inst_hist.win_width = scrw/2;
    inst_hist.win = newwin(inst_hist.win_height, inst_hist.win_width, 11, scrw/2);

    refresh();  // Necessary for window borders

    update_cpu_hist(&inst_hist, &cpu, memory, PUSH_INST);

    // Set up command input
    command_clear(win_cmd, _cmdbuf, &cmdbuf_index);

    // Event loop
    prev_c = c = EOF;
    break_hit = false;
    // F12 F12 = exit
    while (!cmd_exit &&
           !(c == KEY_F(12) && prev_c == KEY_F(12)) /* F-keys exit */ &&
           !(c == 'q' && prev_c == KEY_ESCAPE) /* Vim-style exit */ &&
           !(c == KEY_CTRL_C && prev_c == KEY_CTRL_X) /* Emacs-style exit */) {

        // Handle key press
        switch (c) {
        case KEY_CTRL_C:
        case KEY_CTRL_X:
        case ERR: // During run mode, ERR is returned from getch if no key is available
            break;
        case KEY_F(1): { // Toggle Breakpoint
            uint32_t addr = _cpu_get_effective_pc(&cpu);
            if (_test_mem_flags(memory, addr).B == 0) {
                _set_mem_flags(memory, addr, MEM_FLAG_B);
            }
            else {
                _reset_mem_flags(memory, addr, MEM_FLAG_B);
            }
        }
            break;
        case KEY_F(2): // IRQ
            cpu.P.IRQ = !cpu.P.IRQ;
            break;
        case KEY_F(3): // NMI
            cpu.P.NMI = !cpu.P.NMI;
            break;
        case KEY_F(4): // Halt
            in_run_mode = false;
            timeout(-1); // Enable keypress waiting
            break;
        case KEY_F(5): // Run (until BRK)
            in_run_mode = true;
            run_mode_step_count = 0;
            timeout(0); // Disable waiting for keypresses
            status_id = STATUS_RUN;
            break;
        case KEY_F(6): // Skip instruction
            if (!in_run_mode) {
                cpu.PC += get_opcode(memory, &cpu, NULL);
                update_cpu_hist(&inst_hist, &cpu, memory, REPLACE_INST);
            }
            break;
        case KEY_F(7): // Step
            if (!in_run_mode) {
                stepCPU(&cpu, memory);
                update_cpu_hist(&inst_hist, &cpu, memory, PUSH_INST);
            }
            break;
        case KEY_F(9):
            resetCPU(&cpu);
            update_cpu_hist(&inst_hist, &cpu, memory, PUSH_INST);
            in_run_mode = false;
            timeout(-1); // Enable keypress waiting
            break;
        case KEY_F(12):
            break; // Handled below
        case KEY_CTRL_G:
            command_clear(win_cmd, _cmdbuf, &cmdbuf_index);
            break;
        case '?': {
            cmd_err_msg *msg = &cmd_err_msgs[CMD_HELP_MAIN];
            msg_box(&win_msg, msg->msg, msg->title, msg->win_h, msg->win_w, scrh, scrw);
        }
            break;
        default:
            if (c == EOF) {
                break;
            }

            // Handle message box clean up if user pressed "Enter"
            if (win_msg) {
                if (c == KEY_CR || c == KEY_ESCAPE) {
                    wborder(win_msg, ' ', ' ', ' ',' ',' ',' ',' ',' ');
                    wrefresh(win_msg);
                    delwin(win_msg);
                    win_msg = NULL;
                }
            }
            // If the user pressed "Enter" (CR), execute the command
            else if (command_entry(win_cmd, _cmdbuf, &cmdbuf_index, c)) {

                // Why duplicate the input string?
                // This is needed since command_execute mutates
                // the input command string and it may fail.
                strncpy(_cmdbuf_dup, _cmdbuf, MAX_CMD_LEN);

                // Check for errors in the command input and execute it if none
                cmd_stat = command_execute(
                    &cmd_err,
                    _cmdbuf_dup,
                    cmdbuf_index,
                    &watch1,
                    &watch2,
                    &cpu,
                    memory,
                    &uart
                    );

                if (cmd_err == CMD_EXIT) {
                    cmd_exit = true;
                }
                else if (cmd_stat == STAT_OK) {
                    // Only clear the command input if the command was successful
                    command_clear(win_cmd, _cmdbuf, &cmdbuf_index);
                    update_cpu_hist(&inst_hist, &cpu, memory, REPLACE_INST);
                }
                else {
                    // Print a message box with the err status value
                    if (cmd_stat == STAT_INFO) { // Not an error
                        command_clear(win_cmd, _cmdbuf, &cmdbuf_index);
                    }

                    // If there is errors, print an appropiate message
                    cmd_err_msg *msg = &cmd_err_msgs[cmd_err];

                    // Most cases will have the string length pre determined
                    int win_w = msg->win_w;

                    // For custom "special" error messages, we have to
                    // figure out the length
                    if (cmd_err == CMD_SPECIAL) {
                        win_w = strlen(msg->msg) + 4; // 2 chars of passing on each side
                    }

                    // Finally, update the box's content
                    msg_box(&win_msg, msg->msg, msg->title, msg->win_h, win_w, scrh, scrw);
                }
            }

            break;
        }

        // RUN mode
        if (in_run_mode) {
            stepCPU(&cpu, memory);
            update_cpu_hist(&inst_hist, &cpu, memory, PUSH_INST);

            ++run_mode_step_count;
            if (run_mode_step_count == RUN_MODE_STEPS_UNTIL_DISP_UPDATE) {
                run_mode_step_count = 0;
            }
        }

        // Check for break points
        if (_test_mem_flags(memory, _cpu_get_effective_pc(&cpu)).B == 1) {
            in_run_mode = false;
            timeout(-1); // Back to waiting for key handling
        }

        // Handle UART updating & control
        if (uart.enabled) {
            if (step_16c750(&uart, memory)) {
                cpu.P.IRQ = 1;
            } else {
                cpu.P.IRQ = 0;
            }
        }

        // Handle exiting
        if (c == KEY_F(12)) {
            status_id = STATUS_F12;
            alert = true;
        }
        else if (c == KEY_ESCAPE) {
            status_id = STATUS_ESCQ;
            alert = true;
        }
        else if (c == KEY_CTRL_X) {
            status_id = STATUS_XC;
            alert = true;
        }
        else if (cpu.P.CRASH) {
            status_id = STATUS_CRASH;
            alert = true;
            in_run_mode = false;
        }
        else if (cpu.P.RST) {
            status_id = STATUS_RESET;
            alert = true;
            in_run_mode = false;
        }

        // Update screen
        // getmaxyx(stdscr, scrh, scrw); // Get screen dimensions
        if (!in_run_mode || (run_mode_step_count == 0)) {

            print_header(scrw, status_id, alert);
            print_cpu_regs(win_cpu, &cpu, 1, 2);
            mem_watch_print(&watch1, memory, &cpu);
            mem_watch_print(&watch2, memory, &cpu);
            print_cpu_hist(&inst_hist);

            mvwprintw(win_cmd, 1, 2, ">"); // Command prompt

            // Window borders (DIM)
            wattron(watch1.win, A_DIM);
            wattron(watch2.win, A_DIM);
            wattron(win_cpu, A_DIM);
            wattron(win_cmd, A_DIM);
            wattron(inst_hist.win, A_DIM);
            box(watch1.win, 0, 0);
            box(watch2.win, 0, 0);
            box(win_cpu, 0, 0);
            box(win_cmd, 0, 0);
            box(inst_hist.win, 0, 0);

            // Window Titles (Normal)
            wattroff(watch1.win, A_DIM);
            wattroff(watch2.win, A_DIM);
            wattroff(win_cpu, A_DIM);
            wattroff(win_cmd, A_DIM);
            wattroff(inst_hist.win, A_DIM);
            mvwprintw(watch1.win, 0, 3, " MEM WATCH 1 ");
            mvwprintw(watch2.win, 0, 3, " MEM WATCH 2 ");
            mvwprintw(win_cpu, 0, 3, " CPU STATUS ");
            mvwprintw(win_cmd, 0, 3, " COMMAND ");
            mvwprintw(inst_hist.win, 0, 3, " INSTRUCTION HISTORY ");

            // If message box, prevent the other windows from updating
            if (win_msg) {
                wrefresh(win_msg);
            }
            else {
                // Order of refresh matters - layering of title bars
                wrefresh(win_cpu);
                wrefresh(inst_hist.win);
                wrefresh(win_cmd);
                wrefresh(watch1.win);
                wrefresh(watch2.win);
            }
        }

        refresh();
        if (!in_run_mode) {
            status_id = STATUS_NONE;
            alert = false;
        }

        if (c != ERR) {
            prev_c = c;
        }

        if (break_hit) {
            c = KEY_CTRL_C;
            break_hit = false;
        }
        else if (!cmd_exit) {
            c = getch();
        }
    }

    delwin(watch1.win);
    delwin(watch2.win);
    delwin(win_cpu);
    delwin(win_cmd);
    delwin(inst_hist.win);
    endwin();           // Clean up curses mode

    free(memory);

    if (uart.enabled) {
        stop_16c750(&uart);
    }

    printf("Stopped simulator\n");
    
    return 0;
}

