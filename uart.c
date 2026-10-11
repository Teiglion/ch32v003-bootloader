#include "uart.h"

void uart_init(void)
{
    RCC_APB2PCENR |= RCC_APB2PCENR_AFIOEN |  RCC_APB2PCENR_IOPDEN |  RCC_APB2PCENR_USART1EN;

    AFIO_PCFR1 &= ~AFIO_PCFR1_USART1_REMAP_MASK;

    GPIOD_CFGLR &= ~(GPIOD_PD5_MASK | GPIOD_PD6_MASK);
    GPIOD_CFGLR |= GPIOD_PD5_USART1_TX | GPIOD_PD6_USART1_RX;

    USART1_BRR = USART1_BRR_VALUE;

    USART1_CTLR1 = USART1_CTLR1_UE | USART1_CTLR1_TE | USART1_CTLR1_RE;
}

void uart_putc(uint8_t c)
{
    while ((USART1_STATR & USART1_STATR_TXE) == 0u) {}
    USART1_DATAR = c;
}

void uart_puts(const char *text){while (*text != '\0') uart_putc((uint8_t)*text++);}

uint8_t uart_getc(void)
{
    while ((USART1_STATR & USART1_STATR_RXNE) == 0u) {}
    return (uint8_t)USART1_DATAR;
}