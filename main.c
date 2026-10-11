#include "ch32v003.h"
#include "uart.h"
#include "flash.h"
#include "crc.h"

#define BEGIN 1u
#define DATA  2u
#define END   3u

#define READY       1u
#define ACK         2u
#define DONE        3u

#define BAD_PARAMS  4u
#define BAD_CRC     5u
#define FLASH_ERROR 6u
#define BAD_STATE   7u

static void button_init(void)
{
    RCC_APB2PCENR |= RCC_APB2PCENR_IOPCEN;
    GPIOC_OUTDR &= ~GPIOC_PC0_BIT;
    GPIOC_CFGLR &= ~GPIOC_PC0_MASK;
    GPIOC_CFGLR |= GPIOC_PC0_INPUT_PUPD;
}

static uint8_t button_is_pressed(void){return ((GPIOC_INDR & GPIOC_PC0_BIT) != 0u);}

static uint8_t load_page(uint8_t *buf, uint8_t size)
{
    if (!buf || size > 64u) return 0;

    for(uint8_t i=0; i < size; i++) buf[i] = uart_getc();
    for (uint8_t i = size; i < 64u; i++) buf[i] = 0xFFu;

    return 1;    
}

static uint16_t receive_u16(void)
{
    uint16_t value = (uint16_t)uart_getc();
    value |= (uint16_t)uart_getc() << 8;

    return value;
}

static uint32_t receive_u32(void)
{
    uint32_t value = 0;
    for(uint8_t i=0; i < 4u; i++) value |= (uint32_t)uart_getc() << (i * 8u);
    return value;
}

static void send_u16(uint16_t value)
{
    uart_putc((uint8_t)value);
    uart_putc((uint8_t)(value >> 8));
}

static void send_response(uint8_t status, uint16_t page_number)
{
    uart_putc(status);
    send_u16(page_number);
}

static uint8_t firmware_receive(void)
{
    uint32_t image_size = receive_u32();
    uint32_t host_crc = receive_u32();
    uint32_t capacity = FLASH_ALLOWED_END - FLASH_ALLOWED_BEGIN;
    if(image_size == 0u || image_size > capacity)
    {
        send_response(BAD_PARAMS, 0u);
        return 0;
    }

    uint32_t full_pages = image_size / 64u;
    uint8_t tail_size = (uint8_t)(image_size % 64u);

    uint32_t pages_count = full_pages + (uint32_t)(tail_size != 0u);
    uint32_t erase_sectors = pages_count / 16u;
    uint8_t remain_pages = (uint8_t)(pages_count % 16u);
    uint32_t address = FLASH_ALLOWED_BEGIN;

    for(uint32_t i=0; i < erase_sectors; i++)
    {
        if(!flash_erase_sector(address))
        {
            send_response(FLASH_ERROR, 0u);
            return 0;
        }
        address += 1024u;
    }
    for(uint32_t i=0; i < remain_pages; i++)
    {
        if(!flash_erase_page(address))
        {
            send_response(FLASH_ERROR, 0u);
            return 0;
        }
        address += 64u;
    }
    send_response(READY, 0u);

    uint8_t buf[64];
    address = FLASH_ALLOWED_BEGIN;

    if(uart_getc() != DATA)
    {
        send_response(BAD_STATE, 0u);
        return 0;
    }

    for (uint32_t i = 0; i < pages_count; i++)
    {
        uint8_t size = 64u;

        if (i == full_pages) size = tail_size;

        uint16_t page_number = receive_u16();

        if (page_number != i)
        {
            send_response(BAD_PARAMS, page_number);
            return 0;
        }

        if (!load_page(buf, size))
        {
            send_response(BAD_PARAMS, page_number);
            return 0;
        }

        uint32_t packet_host_crc = receive_u32();
        uint32_t packet_crc =crc32_update(CRC32_INIT, buf, size);
        packet_crc = crc32_finish(packet_crc);
        if (packet_crc != packet_host_crc)
        {
            send_response(BAD_CRC, page_number);
            return 0;
        }

        if (!flash_write_page(address, buf))
        {
            send_response(FLASH_ERROR, page_number);
            return 0;
        }

        address += 64u;

        send_response(ACK, page_number);
    }

    if(uart_getc() != END)
    {
        send_response(BAD_STATE, 0u);
        return 0;
    }

    const uint8_t *image = (const uint8_t *)FLASH_ALLOWED_BEGIN;
    uint32_t image_crc = crc32_update(CRC32_INIT, image, image_size);
    image_crc = crc32_finish(image_crc);
    if (image_crc != host_crc)
    {
        send_response(BAD_CRC, 0u);
        return 0;
    }

    send_response(DONE,0u);

    return 1;
}

static void user_start(void)
{
    FLASH_BOOT_MODEKEYR = 0x45670123u;
    FLASH_BOOT_MODEKEYR = 0xCDEF89ABu;
    FLASH_STATR = 0u;
    RCC_RSTSCKR |= 0x01000000u;
    PFIC_CFGR = PFIC_CFGR_KEY3 | PFIC_CFGR_RESETSYS;
}


int main(void)
{
    button_init();

    if (button_is_pressed())
    {
        uart_init();

        if (uart_getc() != BEGIN)
        {
            send_response(BAD_STATE, 0u);
        }
        else if (firmware_receive())
        {
            while ((USART1_STATR & USART1_STATR_TC) == 0u) {}
            user_start();
        }

        while (1) {}
    }

    user_start();
    while (1) {}
}