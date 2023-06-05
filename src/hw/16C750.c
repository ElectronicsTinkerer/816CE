/**
 * 65(c)816 simulator/emulator (816CE)
 * Copyright (C) 2023 Ray Clemens
 * 
 * This file implments a 16C750-like API based on the data from:
 * https://www.ti.com/lit/ds/symlink/tl16c750.pdf
 * 
 * There is an EXTREMELY HIGH change that there are BUGS
 * in this emulation of the 16C750. You have been warned!
 * It is, quite frankly, a mess :(
 */

// #include <stdio.h> // DEBUG
// #include <ncurses.h>

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <string.h> // memset
#include <stdbool.h>
#include <math.h>

#include "../cpu/65816.h" // memory_t
#include "../cpu/65816-util.h" // memory_t access functions
#include "16C750.h"

// Defined in datasheet
int trigger_levels[2][4] = {
    {1, 4, 8, 14}, // 16-BYTE RX
    {1, 16, 32, 56} // 64-BYTE RX
};


/*
 * "Hardware" reset UART
 * 
 * @param *uart The UART to reset
 */
void reset_16c750(tl16c750_t *uart)
{
    // Page 22
    uart->regs[TL_IER] = 0;
    uart->regs[TL_IIR] = 1;
    uart->regs[TL_FCR] = 0;
    uart->regs[TL_LCR] = 0;
    uart->regs[TL_MCR] = 0;
    uart->regs[TL_LSR] = 0x60;
    uart->regs[TL_MSR] = 0;

    uart->data_rx_fifo_read = 0;
    uart->data_rx_fifo_write = 0;
    uart->data_tx_fifo_read = 0;
    uart->data_tx_fifo_write = 0;
}


/**
 * Initialize a UART by performing a reset and setting up a socket listener
 * 
 * @param *uart The UART to set up
 */
void init_16c750(tl16c750_t *uart)
{
    reset_16c750(uart);

    uart->sock_fd = -1;
    uart->sock_timeout = 1000; // in ms
    uart->data_socket = -1;
}


/**
 * Initialize a UART by performing a reset and setting up a socket listener.
 * This closes any open ports (file descriptors)
 * 
 * @note This should ONLY be called on UART structs which have been
 *       run through the init function.
 * @param *uart The UART to set up
 * @param port The port to listen on
 * @return errno Describing the error encountered
 */
int init_port_16c750(tl16c750_t *uart, uint16_t port)
{
    stop_16c750(uart);

    // Don't initialize if port 0
    if (port == 0) {
        return 0;
    }

    // Set up socket
    uart->sock_fd = socket(AF_INET, SOCK_STREAM, 0); // 0 = choose protocol automatically

    if (uart->sock_fd < 0) {
        return errno;
    }

    int flags = fcntl(uart->sock_fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(uart->sock_fd, F_SETFL, flags | O_NONBLOCK);
    }
    // TODO: Error value?

    // According to the man7.org man page, some implementations
    // have "nonstandard" fields, so make sure everything is
    // zeroed out
    memset(&(uart->sock_name), 0, sizeof(uart->sock_name));

    uart->sock_name.sin_family = AF_INET;
    uart->sock_name.sin_port = htons(port);

    // Attempt to bind to the interface
    int ret = bind(
        uart->sock_fd,
        (const struct sockaddr *) &(uart->sock_name),
        sizeof(uart->sock_name)
        );

    if (ret < 0) {
        return errno;
    }

    // Listen for connections
    ret = listen(uart->sock_fd, UART_MAX_CONNECTIONS);

    if (ret < 0) {
        return errno;
    }

    // Set maximum unack timeout for the TCP connection
    ret = setsockopt(
        uart->sock_fd,
        IPPROTO_TCP,
        TCP_USER_TIMEOUT,
        &(uart->sock_timeout),
        sizeof(uart->sock_timeout) // ???? optLen
        );

    
    if (ret < 0) {
        return errno;
    }
    
    uart->data_socket = -1;
    uart->enabled = false;
    uart->tx_empty_edge = false;

    return 0;
}
    

/*
 * Close network connections
 * 
 * @param *uart The UART to close
 */
void stop_16c750(tl16c750_t *uart)
{
    if (uart->data_socket >= 0) {
        close(uart->data_socket);
    }
    if (uart->sock_fd >= 0) {
        close(uart->sock_fd);
    }
}


/**
 * Cycle the UART to update its memory locations.
 * @note This makes the assumption that a max of one memory location
 *       will be modified between calls to this function
 * 
 * @param *uart The UART to update
 * @param *mem The memory that the UART is shadowing
 * @return True if an interrupt is active, false if no interrupts are active.
 */
bool step_16c750(tl16c750_t *uart, memory_t *mem)
{
    bool irq = false;
    bool sock_closed = false;

    // Attempt to accept an incomming connection if one is
    // not already established
    if (uart->data_socket < 0) {
        uart->data_socket = accept(uart->sock_fd, NULL, NULL);

        // If the accept was successfult, attempt to set the socket
        // into a nonblocking mode
        if (uart->data_socket >= 0) {
            // endwin(); fprintf(stdout, "Accepted\n"); refresh(); // THIS LINE DEBUG
            int flags = fcntl(uart->data_socket, F_GETFL, 0);
            if (flags != -1) {
                fcntl(uart->data_socket, F_SETFL, flags | O_NONBLOCK);
            }
        }
    }
    
    // Check the socket for characters
    // But be sure to not overflow the RX buffer
    if (uart->data_socket >= 0 && abs(uart->data_rx_fifo_write - uart->data_rx_fifo_read) < UART_FIFO_LEN - 1) {
        char buf;
        int read_len = read(uart->data_socket, &buf, 1);

        if (read_len > 0) {
            uart->data_rx_buf[uart->data_rx_fifo_write] = buf;
            uart->data_rx_fifo_write += 1;
            uart->data_rx_fifo_write %= UART_FIFO_LEN;
        }
        else if (read_len == -1 && errno != EAGAIN && errno != EWOULDBLOCK) { // Error
            sock_closed = true;
        }
    }
        
    // SCR is scratch reg, ignore its contents

    // Keep a local copy of the IER
    uart->regs[TL_IER] = _get_mem_byte(mem, uart->addr + TLA_IER, false);

    // LCR
    uart->regs[TL_LCR] = _get_mem_byte(mem, uart->addr + TLA_LCR, false);

    // MCR
    uart->regs[TL_MCR] = _get_mem_byte(mem, uart->addr + TLA_MCR, false);

    // Disable TX empty flag if the CPU read the IIR
    // (can be enabled if TX is written to this cycle)
    if (_test_and_reset_mem_flags(mem, uart->addr + TLA_IIR, MEM_FLAG_R).R == 1) {
        uart->tx_empty_edge = false;
    }
    
    // Handle writing to divisor latches
    if (uart->regs[TL_LCR] & (1u << LCR_DLAB)) {
        if (_test_and_reset_mem_flags(mem, uart->addr + TLA_DLL, MEM_FLAG_W).W == 1) {
            uart->regs[TL_DLL] = _get_mem_byte(mem, uart->addr + TLA_DLL, false);
        } else {
            _set_mem_byte(mem, uart->addr + TLA_DLL, uart->regs[TL_DLL], false);
        }
        if (_test_and_reset_mem_flags(mem, uart->addr + TLA_DLM, MEM_FLAG_W).W == 1) {
            uart->regs[TL_DLM] = _get_mem_byte(mem, uart->addr + TLA_DLM, false);
        } else {
            _set_mem_byte(mem, uart->addr + TLA_DLM, uart->regs[TL_DLM], false);
        }
    }
    // Otherwise, handle communication with the rx/tx regs
    else {
        // THR
        if (_test_and_reset_mem_flags(mem, uart->addr + TLA_THR, MEM_FLAG_W).W == 1) {

            uart->tx_empty_edge = false; // Write into TX reg resets IRQ for empty tx
            
            // Loopback
            if (uart->regs[TL_MCR] & (1u << MCR_LOOP)) {
                // Add value to queue
                uart->data_rx_buf[uart->data_rx_fifo_write] = _get_mem_byte(mem, uart->addr + TLA_THR, false);
                uart->data_rx_fifo_write += 1;
                uart->data_rx_fifo_write %= UART_FIFO_LEN;
            }
            else {
                // SEND CHAR OVER SOCKET?
                uint8_t val = _get_mem_byte(mem, uart->addr + TLA_THR, false);
                if (uart->data_socket >= 0 && !sock_closed) {
                    if (send(uart->data_socket, &val, 1, MSG_NOSIGNAL) == -1) {
                        // If the pipe was closed, errno should be EPIPE
                        sock_closed = true;
                    }
                }
            }

            // If tx buffer is empty, enable signaling of TX empty IRQ
            if (uart->data_tx_fifo_read == uart->data_tx_fifo_write) {
                uart->tx_empty_edge = true;
            }
        }

        // RHR
        // Always keep the last char from the RX FIFO available
        if (abs(uart->data_rx_fifo_write - uart->data_rx_fifo_read) == 0) {
            // Reading the RHR after all characters have been read
            // in will result in just reading the last char received.
            // This is accomplished by "subtracting 1" from the index
            // and performing wrapping on it
            _set_mem_byte(
                mem,
                uart->addr + TLA_RBR,
                uart->data_rx_buf[(uart->data_rx_fifo_read + UART_FIFO_LEN - 1) % UART_FIFO_LEN],
                false
                );
        } else {
            _set_mem_byte(
                mem,
                uart->addr + TLA_RBR,
                uart->data_rx_buf[uart->data_rx_fifo_read],
                false
                );
        }
        
        if (_test_and_reset_mem_flags(mem, uart->addr + TLA_RBR, MEM_FLAG_R).R == 1) {
            
            // Update read buffer pointer
            if (abs(uart->data_rx_fifo_write - uart->data_rx_fifo_read) > 0) {
                uart->data_rx_fifo_read += 1;
                uart->data_rx_fifo_read %= UART_FIFO_LEN;
            }

        }
    }

    // If the FIFO Control Register was updated, keep a local copy
    if (_test_and_reset_mem_flags(mem, uart->addr + TLA_FCR, MEM_FLAG_W).W == 1) {
        uart->regs[TL_FCR] = _get_mem_byte(mem, uart->addr + TLA_FCR, false);

        // Changing the FIFO ENable bit clears the FIFOs
        if (uart->regs[TL_FCR] & (1u << FCR_FIFOEN)) {
            uart->data_rx_fifo_read = 0;
            uart->data_tx_fifo_read = 0;
            uart->data_rx_fifo_write = 0;
            uart->data_tx_fifo_write = 0;
        }
        else if (uart->regs[TL_FCR] & (1u << FCR_RXFRST)) {
            uart->data_rx_fifo_read = 0;
            uart->data_rx_fifo_write = 0;
        }
        else if (uart->regs[TL_FCR] & (1u << FCR_TXFRST)) {
            uart->data_tx_fifo_read = 0;
            uart->data_tx_fifo_write = 0;
        }
    }

    // LSR
    // If RX FIFO is empty
    if (uart->data_rx_fifo_read == uart->data_rx_fifo_write) {
        uart->regs[TL_LSR] &= ~(1u << LSR_DR); // Clear bit
    } else {
        uart->regs[TL_LSR] |= (1u << LSR_DR); // Set bit
    }
    
    // If TX FIFO is empty
    if (uart->data_tx_fifo_read == uart->data_tx_fifo_write) {
        uart->regs[TL_LSR] |= ((1u << LSR_THRE) | (1u << LSR_TEMT));
    } else {
        uart->regs[TL_LSR] &= ~((1u << LSR_THRE) | (1u << LSR_TEMT));
    }
    uart->regs[TL_LSR] &= ~((1u << LSR_OE) | (1u << LSR_PE) | (1u << LSR_FE) | (1u << LSR_BI) | (1u << LSR_ERFIFO));

    _set_mem_byte(mem, uart->addr + TLA_LSR, uart->regs[TL_LSR], false);

    // IIR
    // If there is received data available, set bit 2
    if (
        (!(uart->regs[TL_FCR] & (1u << FCR_FIFOEN)) &&
         abs(uart->data_rx_fifo_write - uart->data_rx_fifo_read) > 0) ||
        ((uart->regs[TL_FCR] & (1u << FCR_FIFOEN)) &&
         abs(uart->data_rx_fifo_write - uart->data_rx_fifo_read) >=
         trigger_levels[(uart->regs[TL_FCR] >> FCR_64FEN) & 0x1][(uart->regs[TL_FCR] >> FCR_RXTRIGL) & 0x3])
        ) {
        uart->regs[TL_IIR] &= ~(0x2 << 1);
        uart->regs[TL_IIR] |= 0x2 << 1;

        // If RX data IRQ enabled, signal it
        if (uart->regs[TL_IER] & (1u << IER_ERBI)) {
            irq = true;
        }
    }
    else if (uart->tx_empty_edge) {
        uart->regs[TL_IIR] &= ~(0x1 << 1);
        uart->regs[TL_IIR] |= 0x1 << 1;

        // If TX data IRQ enabled, signal it
        if (uart->regs[TL_IER] & (1u << IER_ETBEI)) {
            irq = true;
        }
    }
    else {
        uart->regs[TL_IIR] &= ~(0x3 << 1);
        // no irq
    }
        
    // Bits 5..7 indicate FIFO operation mode
    if (uart->regs[TL_FCR] & (1u << FCR_FIFOEN)) {
        
        uart->regs[TL_IIR] |= ((1u << IIR_FOS6) | (1u << IIR_FOS7));

        if  (uart->regs[TL_FCR] & (1u << FCR_64FEN)) {
            uart->regs[TL_IIR] |= (1u << IIR_FOS5);
        }
        else {
            uart->regs[TL_IIR] &= ~(1u << IIR_FOS5);
        }
    }
    else { // 16450 mode
        uart->regs[TL_IIR] &= ~((1u << IIR_FOS5) | (1u << IIR_FOS6) | (1u << IIR_FOS7));
    }
                 
    // Determine if an interrupt is pending
    if (irq) {
        uart->regs[TL_IIR] &= ~(1u << IIR_IPN); // Clear bit
    } else {
        uart->regs[TL_IIR] |= (1u << IIR_IPN);  // Set bit
    }
    
    // Update the state of the Interrupt Identification Register
    _set_mem_byte(mem, uart->addr + TLA_IIR, uart->regs[TL_IIR], false);

    // MSR
    if (uart->data_socket != -1) {
        uart->regs[TL_MSR] |= 1u << MSR_DCD; // DELTA DCD not implemented! TODO
    } else {
        uart->regs[TL_MSR] &= ~(1u << MSR_DCD);
    }
    
    _set_mem_byte(mem, uart->addr + TLA_MSR, uart->regs[TL_MSR], false);

    // Allow new connections
    if (sock_closed) {
        close(uart->data_socket);
        uart->data_socket = -1;
    }

    return irq;
}


