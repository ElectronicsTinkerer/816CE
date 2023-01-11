/**
 * 65(c)816 emulator/simulator
 * (C) Ray Clemens 2023
 * Started: 2021-12-31 Ray Clemens
 * Updated: 2023-01-07
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <ncurses.h>

#include "disassembler.h"
#include "65816.h"
#include "65816-util.h"
#include "debugger.h"


// Error messages for command parsing/execution
// Keep in sync with the cmd_err_t enum in debugger.h
char cmd_err_msgs[][32] = {
    "",
    "Expected argument for command.",
    "Unknown argument.",
    "Unknown command."
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
    mvwprintw(win, y+3, x, "DBR: PBR: PC:");
    mvwprintw(win, y+6, x, "RST: IRQ: NMI: STP: CRASH:");
    mvwprintw(win, y, x+22, "NVMXDIZC|E");
    mvwprintw(win, y+3, x+22, "Cycles:");

    // Data itself
    wattron(win, A_BOLD);
    mvwprintw(win, y+1, x, "%04x %04x %04x %04x",
              cpu->C, cpu->X, cpu->Y, cpu->SP);
    mvwprintw(win, y+4, x, "%02x   %02x   %04x",
              cpu->DBR, cpu->PBR, cpu->PC);
    mvwprintw(win, y+7, x, "%d    %d    %d    %d    %d",
              cpu->P.RST, cpu->P.IRQ, cpu->P.NMI, cpu->P.STP, cpu->P.CRASH);
    mvwprintw(win, y+1, x+22, "%d%d%d%d%d%d%d%d|%d",
              cpu->P.N, cpu->P.V, cpu->P.M, cpu->P.XB, cpu->P.D,
              cpu->P.I, cpu->P.Z, cpu->P.C, cpu->P.E);
    mvwprintw(win, y+4, x+22, "%010ld", cpu->cycles);
    wattroff(win, A_BOLD);
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
    for (size_t i = 1; i < MAX_CMD_LEN + 1; ++i) {
        mvwaddch(win, 1, i + CMD_DISP_X_OFFS, ' ');
        cmdbuf[i] = '\0';
    }
    *cmdbuf_index = 0;

    // Redraw cursor
    wattron(win, A_BOLD);
    mvwaddch(win, 1, CMD_DISP_X_OFFS, '_');
    wattroff(win, A_BOLD);
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

    wattron(win, A_BOLD);

    if (c == KEY_BACKSPACE || c == KEY_CTRL_H) {
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
    
    return false;
}


/**
 * Execute a command from the prompt window.
 * This has a very primitive parser.
 * 
 * @param *_cmdbuf The buffer containing the command
 * @param cmdbuf_index The length of the command + 1
 * @param *watch1 Watch window 1 structure
 * @param *watch2 Watch window 2 structure
 * @return The error code from the command
 */
cmd_err_t command_execute(char *_cmdbuf, int cmdbuf_index, watch_t *watch1, watch_t *watch2)
{
    if (cmdbuf_index == 0) {
        return CMD_ERR_OK; // No command
    }

    char *tok = strtok(_cmdbuf, " ");

    if (!tok) {
        return CMD_ERR_OK; // No command
    }

    
    if (strcmp(tok, "mw1") == 0) { // Memory Watch 1

        tok = strtok(NULL, " ");
        mvwprintw(watch2->win, 2, 2, "'%s'      ", tok);

        if (!tok) {
            return CMD_ERR_EXPECTED_ARG;
        }

        // Secondary level command
        if (strcmp(tok, "mem") == 0) {
            watch1->disasm_mode = false;
        }
        else if (strcmp(tok, "asm") == 0) {
            watch1->disasm_mode = true;
            wclear(watch1->win);
        }
        else if (strcmp(tok, "pc") == 0) {
            watch1->follow_pc = true;
        }
        else {
            return CMD_ERR_UNKNOWN_ARG; // Syntax error!
        }

        tok = strtok(NULL, " ");
        
        if (!tok) {
            return CMD_ERR_OK;
        }
        
        // Tertiary level command
        if (watch1->disasm_mode && strcmp(tok, "pc") == 0) {
            watch1->follow_pc = true;
        }
        else if (watch1->disasm_mode && strcmp(tok, "addr") == 0) {
            watch1->follow_pc = false;
        }
        else {
            return CMD_ERR_UNKNOWN_ARG; // Error!
        }

        return CMD_ERR_OK; // Success?
    }
    else if (strcmp(tok, "mw2") == 0) { // Memory Watch 1

        tok = strtok(NULL, " ");

        if (!tok) {
            return CMD_ERR_EXPECTED_ARG;
        }
    
        // Secondary level command
        if (strcmp(tok, "mem") == 0) {
            watch2->disasm_mode = false;
        }
        else if (strcmp(tok, "asm") == 0) {
            watch2->disasm_mode = true;
            wclear(watch2->win);
        }
        else {
            return CMD_ERR_UNKNOWN_ARG; // Syntax error!
        }

        tok = strtok(NULL, " ");

        if (!tok) {
            return CMD_ERR_OK;
        }
        
        // Tertiary level command
        if (watch2->disasm_mode && strcmp(tok, "pc") == 0) {
            watch2->follow_pc = true;
        }
        else if (watch2->disasm_mode && strcmp(tok, "addr") == 0) {
            watch2->follow_pc = false;
        }
        else {
            return CMD_ERR_UNKNOWN_ARG; // Error!
        }

        return CMD_ERR_OK; // Success?
    }

    return CMD_ERR_UNKNOWN_CMD; // Not success
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

    if (w->disasm_mode) { // Show disassembly
        
        CPU_t cpu_dup;
    
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
        for (row = 0; row < w->win_height - 2; ++row) {

            i = _addr_add_val_bank_wrap(i, get_opcode(mem, &cpu_dup, buf));

            wattron(w->win, (cpu_dup.PC == pc) ? A_BOLD : A_DIM);
            mvwprintw(w->win, 1 + row, 2, "%06x:                         ", cpu_dup.PC);
            wattroff(w->win, (cpu_dup.PC == pc) ? A_BOLD : A_DIM);

            mvwprintw(w->win, 1 + row, 10, "%s", buf);

            cpu_dup.PC = i;
        }
    }
    else { // Just show memory contents
        cols = (w->win_width - 10) / 3;
        if (cols < 16) {
            cols = 8;
        } else if (cols < 32) {
            cols = 16;
        } else {
            cols = 32; // WIDE! screen
        }
        i = w->addr_s;
        for (row = 0; row < w->win_height - 2; ++row) {
            wattron(w->win, A_DIM);
            mvwprintw(w->win, 1 + row, 2, "%06x:", i);
            wattroff(w->win, A_DIM);
            for (col = 0; col < cols; ++col, ++i) {
                wprintw(w->win, " ");

                // If the CPU's PC is at this address, highlight it
                if (i == pc) {
                    wattron(w->win, A_BOLD | A_UNDERLINE);
                }
                wprintw(w->win, "%02x", mem[i]);
                if (i == pc) {
                    wattroff(w->win, A_BOLD | A_UNDERLINE);
                }
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
    box(*win, 0, 0);

    // Title
    mvwprintw(*win, 0, 2, " %s ", title);

    // User message
    mvwprintw(*win, 1, 2, "%s", msg);

    // OK "box"
    wattron(*win, A_REVERSE);
    mvwprintw(*win, height-1, width-6, " OK ");
    wattroff(*win, A_REVERSE);
}


int main(int argc, char *argv[])
{	
    int c, prev_c;          // User key press (c = current, prev_c = previous)
    int scrw, scrh;         // Width and height of screen
    status_t status_id = STATUS_NONE;
    bool alert = true;
    WINDOW *win_cpu, *win_cmd, *win_inst_hist, *win_msg = NULL;
    char cmdbuf[MAX_CMD_LEN];
    char cmdbuf_dup[MAX_CMD_LEN];
    char *_cmdbuf = cmdbuf;
    char *_cmdbuf_dup = cmdbuf_dup;
    size_t cmdbuf_index = 0;
    cmd_err_t cmd_err;
    watch_t watch1, watch2;
    watch1.addr_s = 0;
    watch2.addr_s = 0;
    
    CPU_t cpu;
    resetCPU(&cpu);

    memory_t *memory = malloc(sizeof(*memory) * 0x10000);
    memory_t prog[] = {0xea, 0x1a, 0xe8, 0x88, 0x4c, 0x00, 0x00};
    memcpy(memory, &prog, 7);

    if (!memory) {
        printf("Unable to allocate system memory!\n");
        exit(EXIT_FAILURE);
    }

    initscr();              // Start curses mode
    getmaxyx(stdscr, scrh, scrw); // Get screen dimensions
    raw();                  // Disable line buffering
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
    win_inst_hist = newwin(scrh - 11 - 3, scrw/2, 11, scrw/2);
    
    refresh();  // Necessary for window borders

    // Set up command input
    command_clear(win_cmd, _cmdbuf, &cmdbuf_index);

    // Event loop
    prev_c = c = EOF;
    // F12 F12 = exit
    while (!(c == KEY_F(12) && prev_c == KEY_F(12))) {

        // Handle key press
        switch (c) {
        // case KEY_F(4): // Halt
        // case KEY_F(5): // Run (until BRK)
        // case KEY_F(6): // Run until breakpoint
        case KEY_F(7): // Step
            stepCPU(&cpu, memory);
            break;
        case KEY_F(9):
            resetCPU(&cpu);
            break;
        case KEY_F(12):
            break; // Handled below
        case KEY_CTRL_C:
            command_clear(win_cmd, _cmdbuf, &cmdbuf_index);
            break;
        default:
            if (c == EOF) {
                break;
            }
            
            // If the user pressed "Enter" (CR), execute the command
            if (command_entry(win_cmd, _cmdbuf, &cmdbuf_index, c)) {
                
                // Check for errors in the command input and execute it if none
                strncpy(_cmdbuf_dup, _cmdbuf, MAX_CMD_LEN);
                if (0 != (cmd_err = command_execute(_cmdbuf_dup, cmdbuf_index, &watch1, &watch2))) {

                    // If there is errors, print an appropiate message
                    char *msg = cmd_err_msgs[cmd_err];
                    msg_box(&win_msg, msg, "ERROR!", 3, strlen(msg) + 4, scrh, scrw);
                }
                else { // Only clear the command input if the command was successful
                    command_clear(win_cmd, _cmdbuf, &cmdbuf_index);
                }
            }

            break;
        }

        // Handle exiting
        if (c == KEY_F(12)) {
            status_id = STATUS_F12;
            alert = true;
        }
        else if (cpu.P.CRASH) {
            status_id = STATUS_CRASH;
            alert = true;
        }
        else if (cpu.P.RST) {
            status_id = STATUS_RESET;
            alert = true;
        }

        // Update screen
        // getmaxyx(stdscr, scrh, scrw); // Get screen dimensions
        print_header(scrw, status_id, alert);
        print_cpu_regs(win_cpu, &cpu, 1, 2);
        mem_watch_print(&watch1, memory, &cpu);
        mem_watch_print(&watch2, memory, &cpu);

        mvwprintw(win_cmd, 1, 2, ">"); // Command prompt

        // Window borders (DIM)
        wattron(watch1.win, A_DIM);
        wattron(watch2.win, A_DIM);
        wattron(win_cpu, A_DIM);
        wattron(win_cmd, A_DIM);
        wattron(win_inst_hist, A_DIM);
        box(watch1.win, 0, 0);
        box(watch2.win, 0, 0);
        box(win_cpu, 0, 0);
        box(win_cmd, 0, 0);
        box(win_inst_hist, 0, 0);

        // Window Titles (Normal)
        wattroff(watch1.win, A_DIM);
        wattroff(watch2.win, A_DIM);
        wattroff(win_cpu, A_DIM);
        wattroff(win_cmd, A_DIM);
        wattroff(win_inst_hist, A_DIM);
        mvwprintw(watch1.win, 0, 3, " MEM WATCH 1 ");
        mvwprintw(watch2.win, 0, 3, " MEM WATCH 2 ");
        mvwprintw(win_cpu, 0, 3, " CPU STATUS ");
        mvwprintw(win_cmd, 0, 3, " COMMAND ");
        mvwprintw(win_inst_hist, 0, 3, " INSTRUCTION HISTORY ");
        
        // Order of refresh matters - layering of title bars
        wrefresh(win_cpu);
        wrefresh(win_inst_hist);
        wrefresh(win_cmd);
        wrefresh(watch1.win);
        wrefresh(watch2.win);

        if (win_msg) {
            wrefresh(win_msg);
        }
        
        refresh();
        status_id = STATUS_NONE;
        alert = false;
        prev_c = c;
        c = getch();

        // Clean up message box
        if (win_msg) {
            // wborder(win_msg, ' ', ' ', ' ',' ',' ',' ',' ',' ');
            // wrefresh(win_msg);
            delwin(win_msg);
            win_msg = NULL;
            // c = getch(); // Get the character again so the user can pres "Enter" to close the box
            // refresh();
        }
        
    }
    
    delwin(watch1.win);
    delwin(watch2.win);
    delwin(win_cpu);
    delwin(win_cmd);
    delwin(win_inst_hist);
    endwin();			// Clean up curses mode
    return 0;
}

