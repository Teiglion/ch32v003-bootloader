#include "flash.h"

uint8_t flash_wait_ready(void)
{
    uint32_t counter = 1000000;
    while(counter-- != 0u)if((FLASH_STATR & FLASH_STATR_BSY) == 0u)return 1;
    return 0;
}

uint8_t flash_unlock(void)
{
    if(!flash_wait_ready())return 0;

    if((FLASH_CTLR & FLASH_CTRL_LOCK) == 0u)return 1;
    FLASH_KEYR = 0x45670123;
    FLASH_KEYR = 0xCDEF89AB;
    return ((FLASH_CTLR & FLASH_CTRL_LOCK) == 0u);
}

uint8_t flash_fast_unlock(void)
{
    if(!flash_unlock())return 0;

    if((FLASH_CTLR & FLASH_CTRL_FLOCK) == 0u)return 1;
    FLASH_MODEKEYR = 0x45670123;
    FLASH_MODEKEYR = 0xCDEF89AB;
    return ((FLASH_CTLR & FLASH_CTRL_FLOCK) == 0u);
}

uint8_t flash_clear_flags(void)
{
    if(!flash_wait_ready())return 0;
    FLASH_STATR = (FLASH_STATR_EOP | FLASH_STATR_WRPRTERR);
    return ((FLASH_STATR & (FLASH_STATR_EOP | FLASH_STATR_WRPRTERR)) == 0u);
}

static uint8_t flash_check_addr(uint32_t addr, uint32_t size)
{
    if ((addr & (size - 1u)) != 0u) return 0;

    if (addr < FLASH_ALLOWED_BEGIN) return 0;
    if (addr >= FLASH_ALLOWED_END) return 0;

    if (size > FLASH_ALLOWED_END - addr) return 0;

    return 1;
}

uint8_t flash_erase_sector(uint32_t addr)
{
    if (!flash_check_addr(addr,1024u)) return 0;
    if (!flash_unlock()) return 0;
    if (!flash_clear_flags()) return 0;

    FLASH_CTLR |= FLASH_CTRL_PER;
    FLASH_ADDR = addr;
    FLASH_CTLR |= FLASH_CTRL_STRT;

    if (!flash_wait_ready()) return 0;

    uint32_t stat = FLASH_STATR;
    FLASH_STATR = FLASH_STATR_EOP;

    uint8_t success = ((stat & FLASH_STATR_WRPRTERR) == 0u) && ((stat & FLASH_STATR_EOP) != 0u);
    if (success) 
    {
        volatile uint16_t *block = (volatile uint16_t *)addr;
        for (uint16_t i = 0; i < 512u; i++) 
        {
            if (block[i] != 0xFFFFu) 
            {
                success = 0;
                break;
            }
        }
    }

    FLASH_CTLR &= ~FLASH_CTRL_PER;
    return success;
}

uint8_t flash_erase_page(uint32_t addr)
{
    if (!flash_check_addr(addr,64u)) return 0;
    if(!flash_fast_unlock()) return 0;
    if(!flash_clear_flags()) return 0;

    FLASH_CTLR |= FLASH_CTRL_FTER;
    FLASH_ADDR = addr;
    FLASH_CTLR |= FLASH_CTRL_STRT;

    if (!flash_wait_ready()) return 0;
    uint32_t stat = FLASH_STATR;
    uint8_t success = ((stat & FLASH_STATR_WRPRTERR) == 0u) && ((stat & FLASH_STATR_EOP) != 0u);

    FLASH_STATR = FLASH_STATR_EOP;
    FLASH_CTLR &= ~FLASH_CTRL_FTER;
    return success;
}

uint8_t flash_write(uint32_t addr, uint16_t data)
{
    if (!flash_check_addr(addr, 2u)) return 0;
    if (!flash_unlock()) return 0;
    if (!flash_clear_flags()) return 0;

    FLASH_CTLR |= FLASH_CTRL_PG;

    volatile uint16_t *a = (volatile uint16_t *)addr;
    *a = data;

    if (!flash_wait_ready()) return 0;

    uint32_t stat = FLASH_STATR;
    FLASH_STATR = FLASH_STATR_EOP;

    uint8_t success = ((stat & FLASH_STATR_WRPRTERR) == 0u) && ((stat & FLASH_STATR_EOP) != 0u);
    
    FLASH_CTLR &= ~FLASH_CTRL_PG;

    return success;
}

uint8_t flash_write_page(uint32_t addr, const uint8_t *data)
{
    if (!data) return 0;
    if (!flash_check_addr(addr, 64u)) return 0;
    if (!flash_fast_unlock()) return 0;
    if (!flash_clear_flags()) return 0;

    FLASH_CTLR |= FLASH_CTRL_FTPG;
    FLASH_CTLR |= FLASH_CTRL_BUFRST;

    if (!flash_wait_ready()) return 0;
    FLASH_STATR = FLASH_STATR_EOP;
    
    for(uint8_t i = 0; i < 64; i+=4u)
    {
        uint32_t word = (uint32_t)data[i] | ((uint32_t)data[i+1]<<8) | ((uint32_t)data[i+2]<<16) | ((uint32_t)data[i+3]<<24);
        volatile uint32_t *a = (volatile uint32_t *)(addr+i);
        *a = word;
        FLASH_CTLR |= FLASH_CTRL_BUFLOAD;

        if (!flash_wait_ready()) return 0;
        FLASH_STATR = FLASH_STATR_EOP;
    }

    FLASH_ADDR = addr;
    FLASH_CTLR |= FLASH_CTRL_STRT;
    if (!flash_wait_ready()) return 0;

    uint32_t stat = FLASH_STATR;
    FLASH_STATR = FLASH_STATR_EOP;

    uint8_t success = ((stat & FLASH_STATR_WRPRTERR) == 0u) && ((stat & FLASH_STATR_EOP) != 0u);
    
    FLASH_CTLR &= ~FLASH_CTRL_FTPG;

    return success;
}