#ifndef MMCE_UTILS.H
#define MMCE_UTILS .H

#include <stdint.h>
#include <stdio.h>

#define CRCPOLY 0xEDB88320
static u32 crcTable[256];

#define LFSR_SEED 0x12345678
#define SECTOR_SIZE 2048
#define TEST_ISO_FILE_SIZE 1024 * 1024 * 1024
#define TEST_ISO_NUM_SECTORS TEST_ISO_FILE_SIZE / SECTOR_SIZE

static u32 lfsr = 0;
static u32 lfsr_seed = 0x00;

static void crc_init_table()
{

    u32 crc = 0;
    int i = 0;
    int j = 0;

    for (i = 0; i < 256; i++)
    {
        crc = i;
        for (j = 8; j > 0; j--)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ CRCPOLY;
            }
            else
            {
                crc >>= 1;
            }
        }
        crcTable[i] = crc;
    }
}

u32 crc_calc(const u8 *inData, u32 inLength)
{
    // settings:
    // init with 0xFFFFFFFF
    // xor with 0xFFFFFFFF

    u32 crc = 0xFFFFFFFF;
    int i = 0;
    u8 tableIndex = 0;

    for (i = 0; i < inLength; i++)
    {
        tableIndex = (crc ^ inData[i]) & 0xFF;
        crc = (crc >> 8) ^ crcTable[tableIndex];
    }
    return crc ^ 0xFFFFFFFF;
}


void lfsr_reset()
{
    lfsr = lfsr_seed;
}

uint32_t lfsr_random(uint32_t bits)
{
    // likely not the best lfsr ever written lol
    uint32_t bit = 0;
    lfsr = ((lfsr >> 1) ^ (lfsr >> 7) ^ (lfsr + bits) ^ (lfsr >> 15));
    return lfsr;
}

#endif