/**
 * 65(c)816 emulator/simulator
 * (C) Ray Clemens 2023
 * Started: 2021-12-31 Ray Clemens
 * Updated: 2023-01-07
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ncurses.h>

#include "65816.h"
#include "debugger.h"

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
    for (size_t i = 0; i < MAX_CMD_LEN + 1; ++i) {
        mvwaddch(win, 1, i + CMD_DISP_X_OFFS, ' ');
    }
    *cmdbuf_index = 0;
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
        cmdbuf[++*cmdbuf_index] = '\0';
        return true;
    }

    wattron(win, A_BOLD);
    
    if (c == KEY_BACKSPACE || c == KEY_CTRL_H) {
        if (*cmdbuf_index > 0) {
            mvwaddch(win, 1, (*cmdbuf_index) + CMD_DISP_X_OFFS + 1, ' '); // Erase cursor
            cmdbuf[--*cmdbuf_index] = ' ';
        }
    }
    else if (c == KEY_CTRL_C) { // Clear command
        command_clear(win, cmdbuf, cmdbuf_index);
    }
    else if (*cmdbuf_index < MAX_CMD_LEN - 1) {
        cmdbuf[++*cmdbuf_index] = (char)c;
        mvwaddch(win, 1, (*cmdbuf_index) + CMD_DISP_X_OFFS, cmdbuf[*cmdbuf_index]);
    }

    mvwaddch(win, 1, (*cmdbuf_index) + CMD_DISP_X_OFFS + 1, '_');

    wattroff(win, A_BOLD);
    
    return false;
}


/**
 * Prints the memory in the window for the watch 
 * 
 * @param *w The watch to use
 * @param *mem The CPU's memory to view
 * @param pc The CPU's current PC value
 */
void mem_watch_print(watch_t *w, memory_t *mem, uint32_t pc)
{
    size_t cols, col, row;
    uint32_t i;

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


int main(int argc, char *argv[])
{	
    int c, prev_c;          // User key press (c = current, prev_c = previous)
    int scrw, scrh;         // Width and height of screen
    status_t status_id = STATUS_NONE;
    bool alert = true;
    WINDOW *win_cpu, *win_cmd, *win_inst_hist;
    char cmdbuf[MAX_CMD_LEN];
    char *_cmdbuf = cmdbuf;
    size_t cmdbuf_index = 0;
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

    // Set up each window (height, width, starty, startx)
    watch1.win_height = scrh/2;
    watch1.win_width = scrw/2;
    watch2.win_height = scrh/2;
    watch2.win_width = scrw/2;
    watch1.win = newwin(watch1.win_height, watch1.win_width, 1, 0);
    watch2.win = newwin(watch2.win_height, watch2.win_width, scrh/2, 0);
    win_cpu = newwin(10, scrw/2, 1, scrw/2);
    win_cmd = newwin(3, scrw/2, scrh-3, scrw/2);
    win_inst_hist = newwin(scrh - 10 - 3 + 1, scrw/2, 10, scrw/2);

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
        default:
            if (c == EOF) {
                break;
            }
            if (command_entry(win_cmd, _cmdbuf, &cmdbuf_index, c)) {
                // command_execute(_cmdbuf, &cmdbuf_index, watch1, watch2);
                command_clear(win_cmd, _cmdbuf, &cmdbuf_index);
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
        mem_watch_print(&watch1, memory, cpu.PC);
        mem_watch_print(&watch2, memory, cpu.PC);

        // Window borders (DIM)
        wattron(watch1.win, A_DIM);
        wattron(watch2.win, A_DIM);
        wattron(win_cpu, A_DIM);
        wattron(win_cmd, A_DIM);
        wattron(win_inst_hist, A_DIM);
        wborder(watch1.win, '|', '|', '=', '=', '+', '+', '+', '+');
        wborder(watch2.win, '|', '|', '=', '=', '+', '+', '+', '+');
        wborder(win_cpu, '|', '|', '=', '=', '+', '+', '+', '+');
        wborder(win_cmd, '|', '|', '=', '=', '+', '+', '+', '+');
        wborder(win_inst_hist, '|', '|', '=', '=', '+', '+', '+', '+');

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

        mvwprintw(win_cmd, 1, 2, ">"); // Command prompt

        // Order of refresh matters - layering of title bars
        wrefresh(win_cpu);
        wrefresh(win_inst_hist);
        wrefresh(win_cmd);
        wrefresh(watch1.win);
        wrefresh(watch2.win);
        refresh();
        status_id = STATUS_NONE;
        alert = false;
        prev_c = c;
        c = getch();
    }
    
    delwin(watch1.win);
    delwin(watch2.win);
    delwin(win_cpu);
    delwin(win_cmd);
    delwin(win_inst_hist);
    endwin();			// Clean up curses mode
    return 0;
}
