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

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h> // memset
#include <stdbool.h>
#include <math.h>

#include "65816.h" // memory_t
#include "65816-util.h" // memory_t access functions
#include "16C750.h"

bool trigger_levels[2][4] = {
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
}


/**
 * Initialize a UART by performing a reset and setting up a socket listener
 * 
 * @param *uart The UART to set up
 * @param port The port to listen on
 * @return errno Describing the error encountered
 */
int init_16c750(tl16c750_t *uart, uint16_t port)
{
    reset_16c750(uart);

    uart->data_rx_fifo_read = 0;
    uart->data_rx_fifo_write = 0;
    uart->data_tx_fifo_read = 0;
    uart->data_tx_fifo_write = 0;

    // Set up socket
    uart->sock_fd = socket(AF_INET, SOCK_STREAM, 0); // 0 = choose protocol automatically

    if (uart->sock_fd < 0) {
        return errno;
    }

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

    return 0;
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

    // Attempt to accept an incomming connection if one is
    // not already established
    if (uart->data_socket < 0) {
        uart->data_socket = accept(uart->sock_fd, NULL, NULL);

        // If the accept was successfult, attempt to set the socket
        // into a nonblocking mode
        if (uart->data_socket >= 0) {
            int flags = fcntl(uart->sock_fd, F_GETFL, 0);
            if (flags != -1) {
                fcntl(uart->sock_fd, F_SETFL, flags | O_NONBLOCK);
            }
        }
    }
    
    // Check the socket for characters
    char buf;
    if (uart->data_socket < 0 && read(uart->data_socket, &buf, 1) > 0) {
        uart->data_rx_buf[uart->data_rx_fifo_write] = buf;
        uart->data_rx_fifo_write += 1;
        uart->data_rx_fifo_write %= UART_FIFO_LEN;
    }
        
    // SCR is scratch reg, ignore its contents

    // Keep a local copy of the IER
    uart->regs[TL_IER] = _get_mem_byte(mem, uart->addr + TL_IER, false);

    // LCR
    uart->regs[TL_LCR] = _get_mem_byte(mem, uart->addr + TL_LCR, false);

    // MCR
    uart->regs[TL_MCR] = _get_mem_byte(mem, uart->addr + TL_MCR, false);
    
    // Handle writing to divisor latches
    if (uart->regs[TL_LCR] & (1u << LCR_DLAB)) {
        if (_test_and_reset_mem_flags(mem, uart->addr + TL_DLL - 8, MEM_FLAG_W).W == 1) {
            uart->regs[TL_DLL] = _get_mem_byte(mem, uart->addr + TL_DLL - 8, false);
        }
        if (_test_and_reset_mem_flags(mem, uart->addr + TL_DLM - 8, MEM_FLAG_W).W == 1) {
            uart->regs[TL_DLM] = _get_mem_byte(mem, uart->addr + TL_DLM - 8, false);
        }
    }
    // Otherwise, handle communication with the rx/tx regs
    else {
        // THR
        if (_test_and_reset_mem_flags(mem, uart->addr + TL_THR, MEM_FLAG_W).W == 1) {

            // Loopback
            if (uart->regs[TL_MCR] & MCR_LOOP) {
                // Don't modify memory since the Tx and Rx regs are at same address
            }
            else {
                // SEND CHAR OVER SOCKET?
                // FIXME: ignoring return value
                uint8_t val = _get_mem_byte(mem, uart->addr + TL_THR, false);
                if (uart->data_socket < 0) {
                    write(uart->data_socket, &val, 1);
                }
            }
        }

        // RHR
        if (_test_and_reset_mem_flags(mem, uart->addr + TL_RBR, MEM_FLAG_R).R == 1) {
            
            // Get a character if available from the buffer and store it to memory
            if (abs(uart->data_rx_fifo_write - uart->data_rx_fifo_read) > 0) {
                _set_mem_byte(mem, uart->addr + TL_RBR, uart->data_rx_buf[uart->data_rx_fifo_read], false);
                uart->data_rx_fifo_read += 1;
                uart->data_rx_fifo_read %= UART_FIFO_LEN;
            }

        }
    }

    // If the FIFO Control Register was updated, keep a local copy
    if (_test_and_reset_mem_flags(mem, uart->addr + TL_FCR, MEM_FLAG_W).W == 1) {
        uart->regs[TL_FCR] = _get_mem_byte(mem, uart->addr + TL_FCR, false);

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
        uart->regs[TL_LSR] |= (1u << LSR_DR);
    } else {
        uart->regs[TL_LSR] &= ~(1u << LSR_DR);
    }
    
    // If TX FIFO is empty
    if (uart->data_tx_fifo_read == uart->data_tx_fifo_write) {
        uart->regs[TL_LSR] |= ((1u << LSR_THRE) | (1u << LSR_TEMT));
    } else {
        uart->regs[TL_LSR] &= ~((1u << LSR_THRE) | (1u << LSR_TEMT));
    }
    uart->regs[TL_LSR] &= ~((1u << LSR_OE) | (1u << LSR_PE) | (1u << LSR_FE) | (1u << LSR_BI) | (1u << LSR_ERFIFO));

    _set_mem_byte(mem, uart->addr + TL_LSR, uart->regs[TL_LSR], false);

    // IIR
    // If there is received data available, set bit 2
    if (
        (!(uart->regs[TL_FCR] & (1u << FCR_FIFOEN)) &&
         abs(uart->data_rx_fifo_write - uart->data_rx_fifo_read) > 0) ||
        ((uart->regs[TL_FCR] & (1u << FCR_FIFOEN)) &&
         abs(uart->data_rx_fifo_write - uart->data_rx_fifo_read) >
         trigger_levels[uart->regs[(TL_FCR >> FCR_64FEN) & 0x1]][(TL_FCR >> FCR_RXTRIGL) & 0x3])
        ) {
        uart->regs[TL_IIR] |= 0x2 << 1;
        uart->regs[TL_IIR] &= ~(0x2 << 1);

        // If RX data IRQ enabled, signal it
        if (uart->regs[TL_IER] & (1u << IER_ERBI)) {
            irq = true;
        }
    }
    else if (uart->data_tx_fifo_read == uart->data_tx_fifo_write) {
        uart->regs[TL_IIR] |= 0x1 << 1;
        uart->regs[TL_IIR] &= ~(0x1 << 1);

        // If TX data IRQ enabled, signal it
        if (uart->regs[TL_IER] & (1u << IER_ETBEI)) {
            irq = true;
        }
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
        uart->regs[TL_IIR] &= ((1u << IIR_FOS5) | (1u << IIR_FOS6) | (1u << IIR_FOS7));
    }
                 
    // Determine if an interrupt is pending
    if (irq) {
        uart->regs[TL_IIR] &= ~(1u << IIR_IPN);
    } else {
        uart->regs[TL_IIR] |= (1u << IIR_IPN);
    }
    
    // Update the state of the Interrupt Identification Register
    _set_mem_byte(mem, uart->addr + TL_IIR, uart->regs[TL_IIR], false);

    // bad.
    if (errno != 0) {
        uart->data_socket = -1;
    }

    return irq;
}


