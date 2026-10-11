#ifndef UART_H
#define UART_H

#include "ch32v003.h"

void uart_init(void);
void uart_putc(uint8_t c);
void uart_puts(const char *text);
uint8_t uart_getc(void);

#endif