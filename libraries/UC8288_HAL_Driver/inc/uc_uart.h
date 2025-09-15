#ifndef _UART_H
#define _UART_H

#include "uc_pulpino.h"
#include <stdint.h>

#define UART_REG_RBR ( UART_BASE_ADDR + 0x00) // Receiver Buffer Register (Read Only)
#define UART_REG_DLL ( UART_BASE_ADDR + 0x00) // Divisor Latch (LS)
#define UART_REG_THR ( UART_BASE_ADDR + 0x00) // Transmitter Holding Register (Write Only)
#define UART_REG_DLM ( UART_BASE_ADDR + 0x04) // Divisor Latch (MS)
#define UART_REG_IER ( UART_BASE_ADDR + 0x04) // Interrupt Enable Register
#define UART_REG_IIR ( UART_BASE_ADDR + 0x08) // Interrupt Identity Register (Read Only)
#define UART_REG_FCR ( UART_BASE_ADDR + 0x08) // FIFO Control Register (Write Only)
#define UART_REG_LCR ( UART_BASE_ADDR + 0x0C) // Line Control Register
#define UART_REG_MCR ( UART_BASE_ADDR + 0x10) // MODEM Control Register
#define UART_REG_LSR ( UART_BASE_ADDR + 0x14) // Line Status Register
#define UART_REG_MSR ( UART_BASE_ADDR + 0x18) // MODEM Status Register
#define UART_REG_SCR ( UART_BASE_ADDR + 0x1C) // Scratch Register

#define RBR_UART REGP_8(UART_REG_RBR)
#define DLL_UART REGP_8(UART_REG_DLL)
#define THR_UART REGP_8(UART_REG_THR)
#define DLM_UART REGP_8(UART_REG_DLM)
#define IER_UART REGP_8(UART_REG_IER)
#define IIR_UART REGP_8(UART_REG_IIR)
#define FCR_UART REGP_8(UART_REG_FCR)
#define LCR_UART REGP_8(UART_REG_LCR)
#define MCR_UART REGP_8(UART_REG_MCR)
#define LSR_UART REGP_8(UART_REG_LSR)
#define MSR_UART REGP_8(UART_REG_MSR)
#define SCR_UART REGP_8(UART_REG_SCR)

//uart 1
#define UART1_REG_RBR ( UART1_BASE_ADDR + 0x00) // Receiver Buffer Register (Read Only)
#define UART1_REG_DLL ( UART1_BASE_ADDR + 0x00) // Divisor Latch (LS)
#define UART1_REG_THR ( UART1_BASE_ADDR + 0x00) // Transmitter Holding Register (Write Only)
#define UART1_REG_DLM ( UART1_BASE_ADDR + 0x04) // Divisor Latch (MS)
#define UART1_REG_IER ( UART1_BASE_ADDR + 0x04) // Interrupt Enable Register
#define UART1_REG_IIR ( UART1_BASE_ADDR + 0x08) // Interrupt Identity Register (Read Only)
#define UART1_REG_FCR ( UART1_BASE_ADDR + 0x08) // FIFO Control Register (Write Only)
#define UART1_REG_LCR ( UART1_BASE_ADDR + 0x0C) // Line Control Register
#define UART1_REG_MCR ( UART1_BASE_ADDR + 0x10) // MODEM Control Register
#define UART1_REG_LSR ( UART1_BASE_ADDR + 0x14) // Line Status Register
#define UART1_REG_MSR ( UART1_BASE_ADDR + 0x18) // MODEM Status Register
#define UART1_REG_SCR ( UART1_BASE_ADDR + 0x1C) // Scratch Register

#define DLAB 1<<7   //DLAB bit in LCR reg
#define ERBFI 1     //ERBFI bit in IER reg
#define ETBEI 1<<1  //ETBEI bit in IER reg
#define PE 1<<2     //PE bit in LSR reg
#define THRE 1<<5   //THRE bit in LSR reg
#define DR 1        //DR bit in LSR reg


#define UART_FIFO_DEPTH 16


void uart_set_cfg(int parity, uint16_t clk_counter);

void uart_send(const char* str, unsigned int len);
void uart_sendchar(const char c);
uint8_t uart_get_int_identity();

char uart_getchar();

void uart_wait_tx_done(void);

void uc_uart_init(UART_TYPE* uartx, uint32_t baud_rate, uint8_t data_bits, uint8_t stop_bits, uint8_t parity);
char uc_uart_getchar(UART_TYPE* uartx, uint8_t* get_char);
void uc_uart_sendchar(UART_TYPE* uartx, const char c);
uint8_t uc_uart_get_intrxflag(UART_TYPE* uartx);
void uc_uart_enable_intrx(UART_TYPE* uartx, uint8_t ctrl);
void uc_uartx_wait_tx_done(UART_TYPE *uartx);

#endif
