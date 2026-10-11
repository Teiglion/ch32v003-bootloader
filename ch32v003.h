#ifndef CH32V003_H
#define CH32V003_H

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;

#define REG32(address) (*(volatile uint32_t *)(address))


#define RCC_BASE                  0x40021000u
#define RCC_APB2PCENR             REG32(RCC_BASE + 0x18u)
#define RCC_RSTSCKR               REG32(RCC_BASE + 0x24u)

#define RCC_APB2PCENR_AFIOEN      (1u << 0)
#define RCC_APB2PCENR_IOPDEN      (1u << 5)
#define RCC_APB2PCENR_IOPCEN      (1u << 4)
#define RCC_APB2PCENR_USART1EN    (1u << 14)


#define AFIO_BASE                 0x40010000u
#define AFIO_PCFR1                REG32(AFIO_BASE + 0x04u)
#define AFIO_PCFR1_USART1_REMAP_MASK ((1u << 2) | (1u << 21))

#define GPIOC_BASE                0x40011000u
#define GPIOC_CFGLR               REG32(GPIOC_BASE + 0x00u)
#define GPIOC_INDR                REG32(GPIOC_BASE + 0x08u)
#define GPIOC_OUTDR               REG32(GPIOC_BASE + 0x0Cu)

#define GPIOC_PC0_SHIFT           0u
#define GPIOC_PC0_MASK            (0xFu << GPIOC_PC0_SHIFT)
#define GPIOC_PC0_BIT             (1u << 0)
#define GPIOC_PC0_INPUT_PUPD      (0x8u << GPIOC_PC0_SHIFT)

#define GPIOD_BASE                0x40011400u
#define GPIOD_CFGLR               REG32(GPIOD_BASE + 0x00u)

#define GPIOD_PD5_SHIFT           20u
#define GPIOD_PD5_MASK            (0xFu << GPIOD_PD5_SHIFT)
#define GPIOD_PD5_USART1_TX       (0xAu << GPIOD_PD5_SHIFT)

#define GPIOD_PD6_SHIFT           24u
#define GPIOD_PD6_MASK            (0xFu << GPIOD_PD6_SHIFT)
#define GPIOD_PD6_USART1_RX       (0x4u << GPIOD_PD6_SHIFT)

#define USART1_BASE               0x40013800u
#define USART1_STATR              REG32(USART1_BASE + 0x00u)
#define USART1_DATAR              REG32(USART1_BASE + 0x04u)
#define USART1_BRR                REG32(USART1_BASE + 0x08u)
#define USART1_CTLR1              REG32(USART1_BASE + 0x0Cu)

#define USART1_STATR_TXE          (1u << 7)
#define USART1_STATR_RXNE         (1u << 5)
#define USART1_STATR_TC           (1u << 6)
#define USART1_CTLR1_UE           (1u << 13)
#define USART1_CTLR1_TE           (1u << 3)
#define USART1_CTLR1_RE           (1u << 2)

#define USART1_BRR_VALUE          69u

#define FLASH_BASE                0x40022000
#define FLASH_CTLR                REG32(FLASH_BASE + 0x10)
#define FLASH_CTRL_PG             (1u << 0)
#define FLASH_CTRL_PER            (1u << 1)
#define FLASH_CTRL_STRT           (1u << 6)
#define FLASH_CTRL_LOCK           (1u << 7)
#define FLASH_CTRL_FLOCK          (1u << 15)
#define FLASH_CTRL_FTPG           (1u << 16)
#define FLASH_CTRL_FTER           (1u << 17)
#define FLASH_CTRL_BUFLOAD        (1u << 18)
#define FLASH_CTRL_BUFRST         (1u << 19)

#define FLASH_STATR               REG32(FLASH_BASE + 0x0C)
#define FLASH_STATR_BSY           (1u << 0)
#define FLASH_STATR_WRPRTERR      (1u << 4)
#define FLASH_STATR_EOP           (1u << 5)

#define FLASH_ADDR                REG32(FLASH_BASE + 0x14)
#define FLASH_KEYR                REG32(FLASH_BASE + 0x04)
#define FLASH_MODEKEYR            REG32(FLASH_BASE + 0x24)
#define FLASH_BOOT_MODEKEYR       REG32(FLASH_BASE + 0x28)

#define PFIC_BASE                 0xE000E000u
#define PFIC_CFGR                 REG32(PFIC_BASE + 0x48)
#define PFIC_CFGR_KEY3            0xBEEF0000u
#define PFIC_CFGR_RESETSYS        (1u << 7)

#endif