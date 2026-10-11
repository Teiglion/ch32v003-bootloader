#ifndef FLASH_H
#define FLASH_H

#include "ch32v003.h"

#define FLASH_ALLOWED_BEGIN 0x08000000u
#define FLASH_ALLOWED_END   0x08004000u

uint8_t flash_wait_ready(void);
uint8_t flash_clear_flags(void);

uint8_t flash_unlock(void);
uint8_t flash_fast_unlock(void);

uint8_t flash_erase_sector(uint32_t addr);
uint8_t flash_erase_page(uint32_t addr);

uint8_t flash_write(uint32_t addr, uint16_t data);
uint8_t flash_write_page(uint32_t addr, const uint8_t *data);

#endif