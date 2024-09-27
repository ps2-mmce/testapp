#ifndef MMCE_UTILS.H
#define MMCE_UTILS.H

#include <stdint.h>
#include <stdio.h>

#define CRCPOLY 0xEDB88320
static u32 crcTable[256];

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

static u32 lfsr_start_value = 0x00;



#endif