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
 * @param *cpu The CPU to print
 * @param y The upper left corner of the data display
 * @param x The upper left corner of the data display
 */
void print_cpu_regs(CPU_t *cpu, int y, int x)
{
    // Data headers/labels
    attrset(A_NORMAL);
    mvprintw(y, x, "C:   X:   Y:   SP:");
    mvprintw(y+3, x, "DBR: PBR: PC:");
    mvprintw(y+6, x, "RST: IRQ: NMI: STP: CRASH:");
    mvprintw(y, x+22, "NVMXDIZC|E");
    mvprintw(y+3, x+22, "Cycles:");

    // Data itself
    attron(A_BOLD);
    mvprintw(y+1, x, "%04x %04x %04x %04x",
             cpu->C, cpu->X, cpu->Y, cpu->SP);
    mvprintw(y+4, x, "%02x   %02x   %04x",
             cpu->DBR, cpu->PBR, cpu->PC);
    mvprintw(y+7, x, "%d    %d    %d    %d    %d",
             cpu->P.RST, cpu->P.IRQ, cpu->P.NMI, cpu->P.STP, cpu->P.CRASH);
    mvprintw(y+1, x+22, "%d%d%d%d%d%d%d%d|%d",
             cpu->P.N, cpu->P.V, cpu->P.M, cpu->P.XB, cpu->P.D,
             cpu->P.I, cpu->P.Z, cpu->P.C, cpu->P.E);
    mvprintw(y+4, x+22, "%010ld", cpu->cycles);
             
    attrset(A_NORMAL);

}


int main(int argc, char *argv[])
{	
    int c, prev_c;          // User key press (c = current, prev_c = previous)
    int scrw, scrh;         // Width and height of screen
    state_t sim_state = STATE_IDLE;
    status_t status_id = STATUS_NONE;
    
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
            printw("F7");
            break;
        case KEY_F(9):
            resetCPU(&cpu);
            printw("F9");
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
        getmaxyx(stdscr, scrh, scrw); // Get screen dimensions
        print_header(scrw, sim_state, status_id);
        print_cpu_regs(&cpu, 2, scrw/2);
        refresh();
        status_id = STATUS_NONE;
        prev_c = c;
        c = getch();
    }
    

    printw("Hello World !!!");	/* Print Hello World		  */

    endwin();			// Clean up curses mode
    return 0;
}
