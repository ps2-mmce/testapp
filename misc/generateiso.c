
//
// Generate a 1GB .iso file
// it's not really an iso though
// it's 2kb sectors, of which the first 4 bytes are the sector number
// and the last 4 bytes are a checksum
//

#include <stdio.h>
#include <stdint.h>

#define LFSR_SEED 0x12345678
#define CRCPOLY 0xEDB88320
#define SECTOR_SIZE 2048
#define FILE_SIZE 1024 * 1024 * 1024
#define NUM_SECTORS FILE_SIZE / SECTOR_SIZE

static uint32_t crcTable[256];
static uint32_t lfsr = 0;

void InitCRCTable()
{

    // using tinycc, oldschool var declaration
    uint32_t crc = 0;
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

uint32_t CalcCRC(const uint8_t *inData, uint32_t inLength)
{
    // settings:
    // init with 0xFFFFFFFF
    // xor with 0xFFFFFFFF

    uint32_t crc = 0xFFFFFFFF;
    int i = 0;
    uint8_t tableIndex = 0;

    for (i = 0; i < inLength; i++)
    {
        tableIndex = (crc ^ inData[i]) & 0xFF;
        crc = (crc >> 8) ^ crcTable[tableIndex];
    }
    return crc ^ 0xFFFFFFFF;
}

void ResetLFSR()
{
    lfsr = LFSR_SEED;
}

uint32_t Random(uint32_t bits)
{
    // likely not the best lfsr ever written lol
    uint32_t bit = 0;
    lfsr = ((lfsr >> 1) ^ (lfsr >> 7) ^ (lfsr + bits) ^ (lfsr >> 15));
    return lfsr;
}

void WriteLittleEndian(uint8_t *buffer, uint32_t value)
{
    int i = 0;
    for (i = 0; i < 4; i++)
    {
        buffer[i] = (value >> (i * 8)) & 0xFF;
    }
}

uint32_t ReadLittleEndian(const uint8_t *buffer)
{
    uint32_t value = 0;
    int i = 0;
    for (i = 0; i < 4; i++)
    {
        value |= buffer[i] << (i * 8);
    }
    return value;
}

int GenerateISO()
{

    printf("Generating ISO...\n");

    FILE *fp;
    uint8_t sectorData[SECTOR_SIZE];
    uint32_t sectorNumber = 0;
    uint32_t crc = 0;
    uint32_t sectorNum = 0;
    uint32_t j = 0;

    fp = fopen("test.iso", "wb");
    if (fp == NULL)
    {
        printf("Error opening file test.iso!\n");
        return 1;
    }

    ResetLFSR();

    for (sectorNum = 0; sectorNum < NUM_SECTORS; sectorNum++)
    {

        memset(sectorData, 0, SECTOR_SIZE);

        // sector num
        WriteLittleEndian(sectorData, sectorNum);

        // rando noise
        for (j = 4; j < SECTOR_SIZE; j += 4)
        {
            // checksum every 256 bytes
            if (j % 256 == 252)
            {
                crc = CalcCRC(sectorData, j);
                WriteLittleEndian(sectorData + j, crc);
            }
            else
            {
                WriteLittleEndian(sectorData + j, Random(lfsr));
            }
        }

        // crc of the noise

        fwrite(sectorData, 1, SECTOR_SIZE, fp);
    }

    fclose(fp);
    return 0;
}

int ValidateISO()
{

    printf("Validating ISO...\n");

    FILE *fp;
    uint8_t sector_data[SECTOR_SIZE];
    uint32_t crc = 0;
    uint32_t sectorNum = 0;
    uint32_t j = 0;
    uint32_t crcCalc = 0;
    uint32_t crcRead = 0;

    fp = fopen("test.iso", "rb");
    if (fp == NULL)
    {
        printf("Error opening file test.iso!\n");
        return 1;
    }

    // for each sector, there's a checksum at the end
    // but there's also a checksum before each 256 byte boundary
    // let's check them all.
    for (sectorNum = 0; sectorNum < FILE_SIZE / SECTOR_SIZE; sectorNum++)
    {
        fread(sector_data, 1, SECTOR_SIZE, fp);

        int sectorNumRead = ReadLittleEndian(sector_data);
        if (sectorNumRead != sectorNum)
        {
            printf("Error: sector number mismatch on sector %d\n", sectorNum);
            printf("Expected: 0x%x, (%d)\n", sectorNum, sectorNum);
            printf("Got: 0x%x (%d)\n", sectorNumRead, sectorNumRead);
            fclose(fp);
            return 1;
        }

        // for each packet (8x256 packets in a sector)
        for (j = 0; j < 8; j++)
        {

            // the crc will be the checksum up to the current point in the data
            crcCalc = CalcCRC(sector_data, (j * 256) + 252);
            crcRead = ReadLittleEndian(sector_data + (j * 256) + 252);

            if (crcCalc != crcRead)
            {
                printf("Error: CRC mismatch on sector %d/%d at address 0x%x (%d)\n", sectorNum, j, sectorNum * SECTOR_SIZE, sectorNum * SECTOR_SIZE);
                printf("Expected: %08X\n", crcCalc);
                printf("Got: %08X\n", crcRead);
                fclose(fp);
                return 1;
            }
        }
        // crcCalc = CalcCRC(sector, SECTOR_SIZE - 4);
        // crcRead = ReadLittleEndian(sector + SECTOR_SIZE - 4);
    }

    fclose(fp);
    return 0;
}

void PrintUsage()
{
    printf("Usage:\n");
    printf("generateiso.exe generate <-- make the test.iso \n");
    printf("generateiso.exe validate <-- validate the test.iso \n");
    printf("generateiso.exe both     <-- make and validate the test.iso \n");
}

int main(int argc, char **argv)
{

    printf("Hello World\n");

    if (argc != 2)
    {
        PrintUsage();
        return 1;
    }

    // bools are for the weak
    int doValidate = 0;
    int doGenerate = 0;

    if (strcmp(argv[1], "generate") == 0)
    {
        doGenerate = 1;
    }
    else if (strcmp(argv[1], "validate") == 0)
    {
        doValidate = 1;
    }
    else if (strcmp(argv[1], "both") == 0)
    {
        doGenerate = 1;
        doValidate = 1;
    }
    else
    {
        PrintUsage();
        return 1;
    }

    InitCRCTable();

    // Example
    // uint8_t crcTest[] = {0x01, 0x02, 0x03, 0x04};
    // uint32_t crc = CalcCRC(crcTest, 4);
    // printf("crc: %08X\n", crc);

    // Example
    // ResetLFSR();
    // uint32_t random = Random(0);
    // printf("random 1a: %08X\n", random);
    // random = Random(random);
    // printf("random 2a: %08X\n", random);
    // should match
    // ResetLFSR();
    // random = Random(0);
    // printf("random 1b: %08X\n", random);
    // random = Random(random);
    // printf("random2b: %08X\n", random);

    if (doGenerate)
    {
        int isoResult = GenerateISO();
        if (isoResult != 0)
        {
            printf("Error generating iso\n");
            return 1;
        }
    }

    if (doValidate)
    {
        int isoResult = ValidateISO();
        if (isoResult != 0)
        {
            printf("Error validating iso\n");
            return 1;
        }
    }

    printf("bye\n");
}
