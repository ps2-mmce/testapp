#include <tamtypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NEWLIB_PORT_AWARE
#include <fileio.h>
#include <io_common.h>

#include "include/pad.h"
#include "include/common.h"
#include "include/mmce_fs_tests.h"

static int random_count;

static int test_fs_verify_data(uint8_t *buffer, uint32_t size, uint32_t offset)
{
    int res = 0;

    uint8_t count = 0;
    uint8_t pattern_found = 0;

    uint8_t mismatch_data[8];
    uint32_t mismatch_offset[8];

    for (int i = 0; i < size; i++) {
        if (buffer[i] != pattern_256_bin[i + offset]) {
 
            mismatch_data[count] = buffer[i];
            mismatch_offset[count] = i + offset;

            count++;

            if (count >= 8)
                break;

            xprintf("Mismatch @ 0x%.6x: 0x%.2x : 0x%.2x\n", i, pattern_256_bin[i + offset], buffer[i]);
            res = -1;
        }
    }

    if (count != 0) {
        for (int i = 0; i < pattern_256_bin_len; i++) {

            for (int j = 0; j < count; j++) {
                if (mismatch_data[j] == pattern_256_bin[i + j]) {
                    pattern_found = 1;
                } else {
                    pattern_found = 0;
                    break;
                }
            }

            if (pattern_found) {
                xprintf("Data read found in test file @ 0x%x, should be data from 0x%x\n", i, mismatch_offset[0]);
                xprintf("Off by %i\n", (int)(i - mismatch_offset[0]));
                break;
            }
        }
    }

    return res;
}

/* Open 256.bin w/ O_RDONLY
 * Close 256.bin
*/
static void test_fs_open_close(void)
{
    int fd;
    int res;

    xprintf("Testing: 0x40 - Open\n");
    delay(2);

    fd = open("mmce:/256.bin", O_RDONLY);
    if (fd != -1) {
        xprintf("[PASS] fd: %i\n", fd);
    } else {
        xprintf("[FAIL] error: %i\n", fd);
        return;
    }

    xprintf("Testing: 0x41 - Close\n");
    delay(2);

    res = close(fd);
    if (res != -1) {
        xprintf("[PASS] closed: %i\n", fd);
    } else {
        xprintf("[FAIL] error: %i\n", fd);
        return;
    }

    xprintf("\n");
}

/* Open 256.bin w/ O_RDONLY
 * Read 256KB
 * Verify data against pattern_256
 * Close
*/
static void test_fs_read(uint32_t read_size)
{
    int fd;
    int res;
    int msec;
    uint8_t *buffer = NULL;

    clock_t clk_start, clk_end;

    fd = open("mmce:/256.bin", O_RDONLY);
    if (fd < 0) {
        xprintf("Failed to open 256.bin, %i\n", fd);
        return;
    }
    
    buffer = malloc(read_size);
    if (buffer == NULL) {
        xprintf("Failed malloc 0x%x bytes\n", read_size);
        close(fd);
        return;
    }

    xprintf("Testing: 0x42 - Read\n");
    delay(2);

    clk_start = clock();
    res = read(fd, buffer, read_size);
    clk_end = clock();
    
    msec = (int)((clk_end - clk_start) * 1000 / CLOCKS_PER_SEC);
    xprintf("\n");
    xprintf("Read %dKB (%d bytes) in %dms, speed = %dKB/s\n", read_size/1024, read_size, msec, read_size/msec);
    xprintf("Verifying data...\n");

    if (res != read_size) {
        xprintf("[Warn] expected %i, got %i\n", read_size, res);
    }

    res = test_fs_verify_data(buffer, read_size, 0);
    if (res != -1) {
        xprintf("[PASS] Data valid\n");
    } else {
        xprintf("[FAIL] Data invalid\n");
    }

    res = close(fd);
    if (res == -1) {
        xprintf("Failed to close\n");
    }

    free(buffer);

    xprintf("\n");
}

/* Open test_file.bin w/ O_CREAT O_RDWR
 * Write 128KB from pattern_256
 * Close and reopen
 * Read back 128KB
 * Verify data against pattern_256
 * Close
*/
static void test_fs_write(uint32_t write_size)
{
    int fd;
    int res;
    int msec;
    uint8_t *buffer = NULL;

    clock_t clk_start, clk_end;

    fd = open("mmce:/test_file.bin", O_CREAT | O_RDWR);
    if (fd < 0) {
        xprintf("Failed to open test_file.bin, %i\n", fd);
        return;
    }
    
    buffer = malloc(write_size);
    if (buffer == NULL) {
        xprintf("Failed malloc 0x%x bytes\n", write_size);
        close(fd);
        return;
    }

    xprintf("Testing: 0x43 - Write\n");
    delay(2);

    clk_start = clock();
    res = write(fd, pattern_256_bin, write_size);
    clk_end = clock();
    
    msec = (int)((clk_end - clk_start) * 1000 / CLOCKS_PER_SEC);
    
    if (res != write_size) {
        xprintf("[WARN] expected %i, got %i\n", write_size, res);
    }
        
    xprintf("\n");
    xprintf("Wrote %dKB (%d bytes) in %dms, speed = %dKB/s\n", write_size/1024, write_size, msec, write_size/msec);
    xprintf("Verifying data...\n");

    //Close and reopen
    res = close(fd);
    if (res == -1) {
        xprintf("Failed to close\n");
    }

    fd = open("mmce:/test_file.bin", O_RDONLY);
    if (fd < 0) {
        xprintf("Failed to open test_file.bin, %i\n", fd);
        return;
    }

    res = read(fd, buffer, write_size);
    if (res != write_size) {
        xprintf("[WARN] expected %i, got %i\n", write_size, res);
    }

    res = test_fs_verify_data(buffer, write_size, 0);
    if (res != -1) {
        xprintf("[PASS] Data valid\n");
    } else {
        xprintf("[FAIL] Data invalid\n");
    }

    res = close(fd);
    if (res == -1) {
        xprintf("Failed to close\n");
    }

    free(buffer);

    xprintf("\n");
}


/* Open 256.bin
 * SEEK_SET 0x2000
 * Read 1KB
 * Verify data against pattern_256[0x2000]
 * SEEK_CUR 0x100
 * Read 1KB
 * Verify data against pattern_256[0x2100]
 * SEEK_END 0x0
 * Close
*/
static void test_fs_lseek(void)
{
    int fd;
    int res;
    int read_size = 0x1000;
    uint8_t *buffer = NULL;

    fd = open("mmce:/256.bin", O_RDONLY);
    if (fd < 0) {
        xprintf("Failed to open 256.bin, %i\n", fd);
        return;
    }
    
    buffer = malloc(read_size);
    if (buffer == NULL) {
        xprintf("Failed malloc 0x%x bytes\n", read_size);
        close(fd);
        return;
    }

    xprintf("Testing: 0x44 - lseek SEEK_SET 0x2000\n");
    delay(2);

    res = lseek(fd, 0x2000, SEEK_SET);
    if (res != 0x2000) {
        xprintf("[WARN] expected pos %i, got %i\n", 0x2000, res);
    }

    read(fd, buffer, read_size);
    res = test_fs_verify_data(buffer, read_size, 0x2000); 
    if (res != -1) {
        xprintf("[PASS] Data @ 0x2000 valid\n");
    } else {
        xprintf("[FAIL] Data @ 0x2000 invalid\n");
    }

    xprintf("\n");

    xprintf("Testing: 0x44 - lseek SEEK_CUR 0x100\n");
    delay(2);

    res = lseek(fd, 0x100, SEEK_CUR);

    //0x3100 since we read 4KB
    if (res != 0x3100) {
        xprintf("[WARN] expected pos %i, got %i\n", 0x3100, res);
    }

    read(fd, buffer, read_size);
    res = test_fs_verify_data(buffer, read_size, 0x3100); 
    if (res != -1) {
        xprintf("[PASS] Data @ 0x3100 valid\n");
    } else {
        xprintf("[FAIL] Data @ 0x3100 invalid\n");
    }

    xprintf("\n");

    xprintf("Testing: 0x44 - lseek SEEK_END\n");
    delay(2);

    res = lseek(fd, 0x0, SEEK_END);
    xprintf("[PASS] \n");

    xprintf("\n");

    res = close(fd);
    if (res == -1) {
        xprintf("Failed to close\n");
    }

    free(buffer);

    xprintf("\n");
}

/* Remove test_file.bin */
static void test_fs_remove(void)
{
    int res;

    xprintf("Testing: 0x46 - remove\n");
    delay(2);

    res = remove("mmce:/test_file.bin");
    if (res != -1) {
        xprintf("[PASS]\n");
    } else {
        xprintf("[FAIL] res %i\n", res);
    }

    xprintf("\n");
}

/* Mkdir test_dir */
static void test_fs_mkdir(void)
{
    int res;

    xprintf("Testing: 0x47 - mkdir\n");
    delay(2);

    res = mkdir("mmce:/test_dir", 0);
    if (res != -1) {
        xprintf("[PASS]\n");
    } else {
        xprintf("[FAIL] res %i\n", res);
    }

    xprintf("\n");
}

/* Rmdir test_dir */
static void test_fs_rmdir(void)
{
    int res;

    xprintf("Testing: 0x48 - rmdir\n");
    delay(2);

    res = rmdir("mmce:/test_dir");
    if (res != -1) {
        xprintf("[PASS]\n");
    } else {
        xprintf("[FAIL] res %i\n", res);
    }

    xprintf("\n");
}

/* Dopen / 
 * Dclose
*/
static void test_fs_dopen_dclose(void)
{
    int fd;
    int res;

    xprintf("Testing: 0x49 - dopen\n");
    delay(2);

    fd = fioDopen("mmce:/");
    if (fd != -1) {
        xprintf("[PASS] fd: %i\n", fd);
    } else {
        xprintf("[FAIL] error: %i\n", fd);
        return;
    }
    xprintf("\n");

    xprintf("Testing: 0x4A - dclose\n");
    delay(2);

    res = close(fd);
    if (res != -1) {
        xprintf("[PASS] closed: %i\n", fd);
    } else {
        xprintf("[FAIL] error: %i\n", fd);
        return;
    }
    xprintf("\n");
}

/* Dopen /
 * Dread, print each entry
 * Dclose
*/
static void test_fs_dread(void)
{
    int fd;
    int res = 0;
    fd = fioDopen("mmce:/");
    if (fd < 0) {
        xprintf("Failed to open /, %i\n", fd);
        return;
    }

    io_dirent_t dirent;

    xprintf("Testing: 0x4B - dread\n");
    delay(2);

    while (fioDread(fd, &dirent) != -1)
    {
        xprintf("File: %s\n", dirent.name);
        xprintf("Mode: 0x%x, Attr: 0x%x, Size: 0x%x\n", dirent.stat.mode, dirent.stat.attr, dirent.stat.size);
        xprintf("ctime: %i/%i/%i %i:%i:%i\n", dirent.stat.ctime[5],
                                              dirent.stat.ctime[4], 
                                              ((dirent.stat.ctime[7] << 8) | dirent.stat.ctime[6]),
                                              dirent.stat.ctime[3],
                                              dirent.stat.ctime[2], 
                                              dirent.stat.ctime[1]);

        xprintf("atime: %i/%i/%i %i:%i:%i\n", dirent.stat.atime[5],
                                              dirent.stat.atime[4], 
                                              ((dirent.stat.atime[7] << 8) | dirent.stat.atime[6]),
                                              dirent.stat.atime[3],
                                              dirent.stat.atime[2], 
                                              dirent.stat.atime[1]);

        xprintf("mtime: %i/%i/%i %i:%i:%i\n", dirent.stat.mtime[5],
                                              dirent.stat.mtime[4], 
                                              ((dirent.stat.mtime[7] << 8) | dirent.stat.mtime[6]),
                                              dirent.stat.mtime[3],
                                              dirent.stat.mtime[2], 
                                              dirent.stat.mtime[1]);
        xprintf("\n");
        res++;
    }

    if (res > 0) {
        xprintf("[PASS] Read %i dirents\n", res);
    } else {
        xprintf("[FAIL] Failed to read any dirents\n");
    }
    xprintf("\n");
}

void mmce_fs_auto_tests()
{
    xprintf("Performing MMCE FS auto test sequence...\n");
    delay(2);
 
    test_fs_open_close();
    delay(2);

    test_fs_read(262144);
    delay(2);

    test_fs_write(262144);
    delay(2);

    test_fs_lseek();
    delay(2);

    test_fs_remove();
    delay(2);

    test_fs_mkdir();
    delay(2);

    test_fs_rmdir();
    delay(2);

    test_fs_dopen_dclose();
    delay(2);

    test_fs_dread();
    delay(2);

    xprintf("Auto test complete\n");
}

static void print_tests_menu()
{
    xprintf(" \n");
    xprintf("--------------------\n");
    xprintf("><  = Open & Close 256.bin\n");
    xprintf("/\\  = Read 256KB from 256.bin\n");
    xprintf("[]  = Write 256KB to test_file.bin \n");
    xprintf("()  = Test lseek w/ 256.bin\n");
    xprintf(" \n");
    xprintf("^   = Run auto test sweep\n");
    xprintf(">   = Increase count\n");
    xprintf("<   = Decrease count\n");
    xprintf("->  = Read random amount from 256.bin x%i\n", random_count);
    xprintf("--  = Write random amount to test_file.bin x%i\n", random_count);
    xprintf(" \n");
    xprintf("L1 = mkdir test_dir\n");
    xprintf("L2 = rmdir test_dir\n");
    xprintf("L3 = Remove test_file.bin\n");
    xprintf("R1 = Dopen & Dclose /\n");
    xprintf("R2 = Dread /\n");
    xprintf("R3 = Return to Main Menu\n");
    xprintf("--------------------\n");
}

void menu_mmce_fs_tests()
{
    int count_updated = 0;

    time_t t;
    srand((unsigned)time(&t));

    random_count = 10;

    while (true)
    {
        scr_clear();
        print_tests_menu();

        update_pad();

        // Prevent spamming the sio every frame 
        // as if we were printing to the screen.
        while (!all_released())
        {
            delayframe();
            update_pad();
        }

        xprintf("Processing...\n");
        scr_clear();

        if (released(PAD_CROSS))
        {
            test_fs_open_close();
        }
        else if (released(PAD_TRIANGLE))
        {
            test_fs_read(262144);
        }
        else if (released(PAD_SQUARE))
        {
            test_fs_write(262144);
        }
        else if (released(PAD_CIRCLE))
        {
            test_fs_lseek();
        }
        else if (released(PAD_LEFT))
        {
            if (random_count != 0) {
                random_count--;
                count_updated = 1;
            }
        } 
        else if (released(PAD_RIGHT))
        {
            random_count++;
            count_updated = 1;
        }
        else if (released(PAD_START))
        {
            for (int i = 0; i < random_count; i++) {
                test_fs_read((rand() % 261120) + 1024);
            }
        }
        else if (released(PAD_SELECT))
        {
            for (int i = 0; i < random_count; i++) {
                test_fs_write((rand() % 261120) + 1024);
            }
        }
        else if (released(PAD_L1))
        {
            test_fs_mkdir();
        }
        else if (released(PAD_L2))
        {
            test_fs_rmdir();
        }
        else if (released(PAD_L3))
        {
            test_fs_remove();
        }
        else if (released(PAD_R1))
        {
            test_fs_dopen_dclose();
        }
        else if (released(PAD_R2))
        {
            test_fs_dread();
        }
        else if (released(PAD_R3))
        {
            break;
        }

        if (count_updated != 1) {
            xprintf("Push to continue... \n" );
            update_pad();
            while (!all_released())
            {
                delayframe();
                update_pad();
            }

            xprintf("Done...\n");
        } else {
            count_updated = 0;
        }
        
    } // while (true);
}