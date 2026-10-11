#ifndef CRC_H
#define CRC_H

#include "ch32v003.h"

#define CRC32_INIT 0xFFFFFFFFu

uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t size);
uint32_t crc32_finish(uint32_t crc);

#endif