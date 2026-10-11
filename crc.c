#include "crc.h"

#define CRC32_POLY 0xEDB88320u

uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        crc ^= (uint32_t)data[i];

        for (uint8_t bit = 0; bit < 8u; bit++)
        {
            if ((crc & 1u) != 0u) crc = (crc >> 1) ^ CRC32_POLY;
            else crc >>= 1;
        }
    }

    return crc;
}

uint32_t crc32_finish(uint32_t crc) {return crc ^ 0xFFFFFFFFu;}