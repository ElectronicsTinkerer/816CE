/**
 * 65(c)816 emulator/simulator
 * (C) Ray Clemens 2023
 * Started: 2021-12-31 Ray Clemens
 * Updated: 2023-01-07
 */

#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#include "65816.h"
#include "debugger.h"

/**
 * Prints the banner across the top of the screen
 * 
 * @param width The width in characters of the terminal
 * @param sim_state The current simulation state
 * @param status_id The identifier of the interface status message to print
 */
void print_header(int width, state_t sim_state, status_t status_id)
{
    wmove(stdscr, 0, 0);
    attron(A_REVERSE | A_BOLD);
    for (size_t i = 0; i < width; ++i) {
        waddch(stdscr, ' ');
    }
    mvwprintw(stdscr,0, 0, " 65816 Debugger | %s", status_msgs[status_id]);
    mvwprintw(stdscr, 0, width - strlen(state_msgs[sim_state] - 1), "%s", state_msgs[sim_state]);
    attroff(A_REVERSE | A_BOLD);
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
}


int main(int argc, char *argv[])
{	
    int c, prev_c;          // User key press (c = current, prev_c = previous)
    int scrw, scrh;         // Width and height of screen
    state_t sim_state = STATE_IDLE;
    status_t status_id = STATUS_NONE;
    WINDOW *win_watch1, *win_watch2, *win_cpu, *win_cmd, *win_inst_hist;
    
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
    win_watch1 = newwin(scrh/2, scrw/2, 1, 0);
    win_watch2 = newwin(scrh/2, scrw/2, scrh/2, 0);
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
        default:
            // printw("Default '%d' \"%d\" '%d' %d", c, KEY_F(12), prev_c, KEY_F(12) == prev_c);
            break;
        }

        // Handle exiting
        if (c == KEY_F(12)) {
            status_id = STATUS_F12;
        } else if (cpu.P.CRASH) {
            status_id = STATUS_CRASH;
        } else if (cpu.P.RST) {
            status_id = STATUS_RESET;
        }

        // Update screen
        // getmaxyx(stdscr, scrh, scrw); // Get screen dimensions
        print_header(scrw, sim_state, status_id);
        print_cpu_regs(win_cpu, &cpu, 1, 2);

        // Window borders
        wborder(win_watch1, '|', '|', '=', '=', '+', '+', '+', '+');
        wborder(win_watch2, '|', '|', '=', '=', '+', '+', '+', '+');
        wborder(win_cpu, '|', '|', '=', '=', '+', '+', '+', '+');
        wborder(win_cmd, '|', '|', '=', '=', '+', '+', '+', '+');
        wborder(win_inst_hist, '|', '|', '=', '=', '+', '+', '+', '+');

        // Order of refresh matters - layering of title bars
        wrefresh(win_cpu);
        wrefresh(win_inst_hist);
        wrefresh(win_cmd);
        wrefresh(win_watch1);
        wrefresh(win_watch2);
        refresh();
        status_id = STATUS_NONE;
        prev_c = c;
        c = getch();
    }
    

    printw("Hello World !!!");	/* Print Hello World		  */

    delwin(win_watch1);
    delwin(win_watch2);
    delwin(win_cpu);
    delwin(win_cmd);
    delwin(win_inst_hist);
    endwin();			// Clean up curses mode
    return 0;
}
