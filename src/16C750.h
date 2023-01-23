/**
 * 65(c)816 simulator/emulator (816CE)
 * Copyright (C) 2023 Ray Clemens
 */

#ifndef UART_16C750_H
#define UART_16C750_H

#include <netinet/in.h>

// Not sure how/why this would not be 1
// for this particular use case
#define UART_MAX_CONNECTIONS 1

#define UART_FIFO_LEN 64

// IER
enum {
    IER_ERBI = 0,
    IER_ETBEI,
    IER_ELSI,
    IER_EDSSI,
    IER_SME,
    IER_LPME
};

// IIR
enum {
    IIR_IPN = 0, // Interrupt PeNding
    IIR_IID1,
    IIR_IID2,
    IIR_ITOP,    // Interrupt Time Out Pending
    IIR_NC,
    IIR_FOS5,    // FifO Setup
    IIR_FOS6,
    IIR_FOS7
};

// FCR
enum {
    FCR_FIFOEN = 0,
    FCR_RXFRST,  // Receieve FIFO reset
    FCR_TXFRST,
    FCR_DMA,
    FCR_NC,
    FCR_64FEN,
    FCR_RXTRIGL,
    FCR_RXTRIGM
};

// LCR
enum {
    LCR_WLS0,
    LCR_WLS1,
    LCR_STB,
    LCR_PEN,
    LCR_EPS,
    LCR_SP,
    LCR_BC,
    LCR_DLAB
};

// MCR
enum {
    MCR_DTR,
    MCR_RTS,
    MCR_OUT1,
    MCR_OUT2,
    MCR_LOOP,
    MCR_AFE,
    MCR_NC6, // 0
    MCR_NC7  // 0
};

// LSR
enum {
    LSR_DR,
    LSR_OE,
    LSR_PE,
    LSR_FE,
    LSR_BI,
    LSR_THRE,
    LSR_TEMT,
    LSR_ERFIFO
};

// MSR
enum {
    MSR_DCTS,
    MSR_DDSR,
    MSR_TERI,
    MSR_DDCD,
    MSR_CTS,
    MSR_DSR,
    MSR_RI,
    MSR_DCD
};

// SCR
// No named bits

// DLL
// No named bits

// DLM
// No named bits

typedef enum tl16c750_regs_t {
    // Indicies into the regs[] array
    TL_RBR = 0,
    TL_THR = 1,
    TL_IER = 2,
    TL_IIR = 3,
    TL_FCR = 4,
    TL_LCR = 5,
    TL_MCR = 6,
    TL_LSR = 7,
    TL_MSR = 8,
    TL_SCR = 9,
    TL_DLL = 10,
    TL_DLM = 11,
    // Address offsets
    TLA_RBR = 0,
    TLA_THR = 0,
    TLA_IER = 1,
    TLA_IIR = 2,
    TLA_FCR = 2,
    TLA_LCR = 3,
    TLA_MCR = 4,
    TLA_LSR = 5,
    TLA_MSR = 6,
    TLA_SCR = 7,
    TLA_DLL = 0,
    TLA_DLM = 1
} tl16c750_regs_t;

typedef struct tl16c750_t {
    bool enabled;
    uint32_t addr;    // Base address
    uint8_t regs[12]; // tl16c750_regs_t is index
    int sock_fd;
    struct sockaddr_in sock_name;
    int data_socket;
    int data_rx_fifo_read;
    int data_rx_fifo_write;
    uint8_t data_rx_buf[UART_FIFO_LEN];
    int data_tx_fifo_read;
    int data_tx_fifo_write;
    uint8_t data_tx_buf[UART_FIFO_LEN];
} tl16c750_t;

void reset_16c750(tl16c750_t *);
int init_16c750(tl16c750_t *, uint16_t);
void stop_16c750(tl16c750_t *);
bool step_16c750(tl16c750_t *, memory_t *);

#endif

