/**
 * 65(c)816 simulator/emulator (816CE)
 * Copyright (C) 2023 Ray Clemens
 */

#ifndef UART_16C750_H
#define UART_16C750_H

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
    TL_RBR = 0,
    TL_THR = 0,
    TL_IER = 1,
    TL_IIR = 2,
    TL_FCR = 2,
    TL_LCR = 3,
    TL_MCR = 4,
    TL_LSR = 5,
    TL_MSR = 6,
    TL_SCR = 7,
    TL_DLL = 8,
    TL_DLM = 9
} tl16c750_regs_t;

typedef struct tl16c750_t {
    uint32_t addr;    // Base address
    uint8_t regs[10]; // tl16c750_regs_t is index
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

#endif

