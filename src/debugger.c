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
 */
void print_header(int width, state_t sim_state)
{
    wmove(stdscr, 0, 0);
    attron(A_REVERSE | A_BOLD);
    for (size_t i = 0; i < width; ++i) {
        waddch(stdscr, ' ');
    }
    mvwprintw(stdscr,0, 0, " 65816 Debugger");
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
    wmove(stdscr, y, x);
    wprintw(stdscr, "Cycles: %010ld", cpu->cycles);
    wmove(stdscr, y + 1, x); // Next line
    wprintw(stdscr, "RST:%d IRQ:%d NMI:%d STP:%d CRSH: %d",
            cpu->P.RST, cpu->P.IRQ, cpu->P.NMI, cpu->P.STP, cpu->P.CRASH);
    wmove(stdscr, y + 2, x); // Next line
    wprintw(stdscr, "C:%04x X:%04x Y:%04x SP:%04x",
            cpu->C, cpu->X, cpu->Y, cpu->SP);
    wmove(stdscr, y + 3, x); // Next line
    wprintw(stdscr, "DBR:%02x PBR:%02x PC:%04x",
            cpu->DBR, cpu->PBR, cpu->PC);
    wmove(stdscr, y + 4, x); // Next line
    wprintw(stdscr, "Status: NVMXDIZC|E");
    wmove(stdscr, y + 5, x); // Next line
    wprintw(stdscr, "        %d%d%d%d%d%d%d%d|%d",
            cpu->P.N, cpu->P.V, cpu->P.M, cpu->P.XB, cpu->P.D,
            cpu->P.I, cpu->P.Z, cpu->P.C, cpu->P.E);
}


int main(int argc, char *argv[])
{	
    char c, prev_c;         // User key press (c = current, prev_c = previous)
    int scrw, scrh;         // Width and height of screen
    state_t sim_state = STATE_IDLE;

    CPU_t cpu;
    resetCPU(&cpu);

    memory_t *memory = malloc(sizeof(*memory) * 0x10000);

    if (!memory) {
        printf("Unable to allocate system memory!\n");
        exit(EXIT_FAILURE);
    }
    
    initscr();              // Start curses mode
    getmaxyx(stdscr, scrh, scrw); // Get screen dimensions
    cbreak(); // Disable line buffering
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
            printw("Default '%d' \"%d\" '%d' %d", c, KEY_F(12), prev_c, KEY_F(12) == prev_c);
            break;
        }

        // Update screen
        print_header(scrw, sim_state);
        print_cpu_regs(&cpu, 2, scrw/2);
        refresh();
        prev_c = c;
        c = getch();
    }
    

    printw("Hello World !!!");	/* Print Hello World		  */

    endwin();			// Clean up curses mode
    return 0;
}
