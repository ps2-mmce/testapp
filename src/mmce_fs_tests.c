#include <tamtypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ps2sdkapi.h>

#define NEWLIB_PORT_AWARE
#include <fileio.h>
#include <fileXio_rpc.h>
#include <io_common.h>

#include "include/pad.h"
#include "include/common.h"
#include "include/mmce_fs_tests.h"
#include "include/mmce_utils.h"

static int read_size = 256;
static int write_size = 256;

static const char *prefix[] = {"mmce0:", "mmce1:"};
static int prefix_idx = 0;

static char path[64] = "mmce0:";

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

static void prefix_inc(void *arg)
{
    int num = *(int*)arg;
    if (num < 1)
        num++;
    *(int*)arg = num;
}

static void prefix_dec(void *arg)
{
    int num = *(int*)arg;
    if (num > 0)
        num--;
    *(int*)arg = num;
}

static void pow_two_size_inc(void *arg)
{
    int size = *(int*)arg;
    size = size * 2;
    if (size > 262144)
        size = 262144;

    *(int*)arg = size;
}

static void pow_two_size_dec(void *arg)
{
    int size = *(int*)arg;
    size = size / 2;
    if (size <= 0)
        size = 1;

    *(int*)arg = size;
}

static void lfsr_size_inc(void * arg){
    uint32_t val = *(uint32_t*)arg;
    val += 1;
    *(uint32_t*)arg = val;
}

static void lfsr_size_dec(void * arg){
    // let it wrap around, why not
    uint32_t val = *(uint32_t*)arg;
    val -= 1;
    *(uint32_t*)arg = val;
}

/* Open 256.bin w/ O_RDONLY
 * Close 256.bin
*/
static void test_fs_open_close()
{
    int fd;
    int iop_fd;
    int res;

    sprintf(path, "%s/256.bin", prefix[prefix_idx]);

    xprintf("Testing: 0x40 - Open\n");
    delay(2);

    fd = open(path, O_RDONLY);
    if (fd != -1) {
        iop_fd = ps2sdk_get_iop_fd(fd);
        xprintf("[PASS] fd: %i\n", iop_fd);
    } else {
        xprintf("[FAIL] error: %i\n", fd);
        return;
    }

    xprintf("Testing: 0x41 - Close\n");
    delay(2);

    res = close(fd);
    if (res != -1) {
        xprintf("[PASS] closed: %i\n", iop_fd);
    } else {
        xprintf("[FAIL] error: %i\n", fd);
        return;
    }

    xprintf("\n");
}

/* Open 256.bin w/ O_RDONLY
 * Read read_write_size
 * Verify data against pattern_256
 * Close
*/
static void test_fs_read(void *arg)
{
    int fd;
    int res;
    uint8_t *buffer = NULL;
    struct timeval tv_start, tv_end;

    int size = *(int*)arg;

    sprintf(path, "%s/256.bin", prefix[prefix_idx]);

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        xprintf("Failed to open 256.bin, %i\n", fd);
        return;
    }
    
    buffer = malloc(size);
    if (buffer == NULL) {
        xprintf("Failed malloc 0x%x bytes\n", size);
        close(fd);
        return;
    }

    xprintf("Testing: 0x42 - Read %i bytes\n", size);
    delay(2);

    gettimeofday(&tv_start, 0);
    res = read(fd, buffer, size);
    gettimeofday(&tv_end, 0);

    long elapsed = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec; 
    float msec = (float)elapsed / 1000.0f;

    xprintf("\n");
    xprintf("Read %dKB (%d bytes) in %.2fms, speed = %.2fKB/s\n", size/1024, size, msec, (float)size/msec);
    xprintf("Verifying data...\n");

    if (res != size) {
        xprintf("[Warn] expected %i, got %i\n", size, res);
    }

    delay(1);
    res = test_fs_verify_data(buffer, size, 0);
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

/* Open 256.bin w/ O_RDONLY
 * Attempt to read 512 bytes beyond length of file
 * Close
*/
static void test_fs_read_beyond()
{
    int fd;
    int res;
    uint8_t *buffer = NULL;
    struct timeval tv_start, tv_end;

    int size = 512;

    sprintf(path, "%s/256.bin", prefix[prefix_idx]);

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        xprintf("Failed to open 256.bin, %i\n", fd);
        return;
    }

    buffer = malloc(size);
    if (buffer == NULL) {
        xprintf("Failed malloc 0x%x bytes\n", size);
        close(fd);
        return;
    }

    xprintf("Seeking to end\n");
    res = lseek(fd, 0, SEEK_END);
    xprintf("Got %i\n", res);

    xprintf("Seeking back 150 bytes\n");
    res = lseek(fd, -150, SEEK_CUR);
    xprintf("Got %i\n", res);

    xprintf("Testing: 0x42 - Read (362 bytes over filesize)\n");
    delay(2);

    gettimeofday(&tv_start, 0);
    res = read(fd, buffer, size);
    gettimeofday(&tv_end, 0);

    long elapsed = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec; 
    float msec = (float)elapsed / 1000.0f;

    xprintf("\n");
    xprintf("Read %dKB (%d bytes) in %.2fms, speed = %.2fKB/s\n", size/1024, size, msec, (float)size/msec);
    xprintf("Verifying data...\n");

    if (res == 150) {
        xprintf("[PASS] Only read %i bytes\n", res);
    } else {
        xprintf("[FAIL] Read more than expected, %i\n", res);
    }

    res = close(fd);
    if (res == -1) {
        xprintf("Failed to close\n");
    }

    free(buffer);

    xprintf("\n");
}

void test_fs_read_sweep()
{
    int size = 256;

    test_fs_read(&size);
    delay(2);
    scr_clear();

    size = 512;
    test_fs_read(&size);
    delay(2);
    scr_clear();

    size = 1024;
    test_fs_read(&size);
    delay(2);
    scr_clear();

    size = 2048;
    test_fs_read(&size);
    delay(2);
    scr_clear();

    size = 4096;
    test_fs_read(&size);
    delay(2);
    scr_clear();

    size = 8192;
    test_fs_read(&size);
    delay(2);
    scr_clear();

    size = 16384;
    test_fs_read(&size);
    delay(2);
    scr_clear();

    size = 32768;
    test_fs_read(&size);
    delay(2);
    scr_clear();

    size = 65536;
    test_fs_read(&size);
    delay(2);
    scr_clear();

    size = 131072;
    test_fs_read(&size);
    delay(2);
    scr_clear();

    size = 262144;
    test_fs_read(&size);
    delay(2);
    scr_clear();
}

/* Open test_file.bin w/ O_CREAT O_RDWR
 * Write read_write_size from pattern_256
 * Close and reopen
 * Read back read_write_size
 * Verify data against pattern_256
 * Close
*/
static void test_fs_write(void *arg)
{
    int fd;
    int res;
    int msec;
    uint8_t *buffer = NULL;

    clock_t clk_start, clk_end;

    int size = *(int*)arg;

    sprintf(path, "%s/testfile.bin", prefix[prefix_idx]);

    fd = open(path, O_CREAT | O_RDWR);
    if (fd < 0) {
        xprintf("Failed to open test_file.bin, %i\n", fd);
        return;
    }
    
    buffer = malloc(size);
    if (buffer == NULL) {
        xprintf("Failed malloc 0x%x bytes\n", size);
        close(fd);
        return;
    }

    xprintf("Testing: 0x43 - Write %i bytes\n", size);
    delay(2);

    clk_start = clock();
    res = write(fd, pattern_256_bin, size);
    clk_end = clock();
    
    msec = (int)((clk_end - clk_start) * 1000 / CLOCKS_PER_SEC);
    
    if (res != size) {
        xprintf("[WARN] expected %i, got %i\n", size, res);
    }

    xprintf("\n");
    xprintf("Wrote %dKB (%d bytes) in %dms, speed = %dKB/s\n", size/1024, size, msec, size/msec);
    xprintf("Verifying data...\n");

    //Close and reopen
    res = close(fd);
    if (res == -1) {
        xprintf("Failed to close\n");
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        xprintf("Failed to open test_file.bin, %i\n", fd);
        return;
    }

    res = read(fd, buffer, size);
    if (res != size) {
        xprintf("[WARN] expected %i, got %i\n", size, res);
    }

    res = test_fs_verify_data(buffer, size, 0);
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
 * Read 4KB
 * Verify data against pattern_256[0x2000]
 * SEEK_CUR 0x100
 * Read 4KB
 * Verify data against pattern_256[0x2100]
 * SEEK_END 0x0
 * Close
*/
static void test_fs_lseek()
{
    int fd;
    u32 res;
    int read_size = 0x1000;
    uint8_t *buffer = NULL;

    sprintf(path, "%s/256.bin", prefix[prefix_idx]);

    fd = open(path, O_RDONLY);
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
        xprintf("[WARN] expected pos %i, got %li\n", 0x2000, res);
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
        xprintf("[WARN] expected pos %i, got %li\n", 0x3100, res);
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
    xprintf("[PASS] %lli\n", res);

    xprintf("\n");

    res = close(fd);
    if (res == -1) {
        xprintf("Failed to close\n");
    }

    free(buffer);

    xprintf("\n");
}

static void test_fs_lseek64()
{
    int fd;
    s64 res;
    int read_size = 0x1000;
    uint8_t *buffer = NULL;

    sprintf(path, "%s/256.bin", prefix[prefix_idx]);

    fd = open(path, O_RDONLY);
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

    res = lseek64(fd, 0x2000, SEEK_SET);
    if (res != 0x2000) {
        xprintf("[WARN] expected pos %i, got %lli\n", 0x2000, res);
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

    res = lseek64(fd, 0x100, SEEK_CUR);

    //0x3100 since we read 4KB
    if (res != 0x3100) {
        xprintf("[WARN] expected pos %i, got %lli\n", 0x3100, res);
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

    res = lseek64(fd, 0x0, SEEK_END);
    xprintf("[PASS] %lli\n", res);

    xprintf("\n");

    res = close(fd);
    if (res == -1) {
        xprintf("Failed to close\n");
    }

    free(buffer);

    xprintf("\n");
}

/* Remove test_file.bin */
static void test_fs_remove()
{
    int res;

    sprintf(path, "%s/test_file.bin", prefix[prefix_idx]);

    xprintf("Testing: 0x46 - remove\n");
    delay(2);

    res = remove(path);
    if (res != -1) {
        xprintf("[PASS]\n");
    } else {
        xprintf("[FAIL] res %i\n", res);
    }

    xprintf("\n");
}

/* Mkdir test_dir */
static void test_fs_mkdir()
{
    int res;
    sprintf(path, "%s/test_dir", prefix[prefix_idx]);

    xprintf("Testing: 0x47 - mkdir\n");
    delay(2);

    res = mkdir(path, 0);
    if (res != -1) {
        xprintf("[PASS]\n");
    } else {
        xprintf("[FAIL] res %i\n", res);
    }

    xprintf("\n");
}

/* Rmdir test_dir */
static void test_fs_rmdir()
{
    int res;

    sprintf(path, "%s/test_dir", prefix[prefix_idx]);

    xprintf("Testing: 0x48 - rmdir\n");
    delay(2);

    res = rmdir(path);
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
static void test_fs_dopen_dclose()
{
    int res;
    DIR *fd;
    int iop_fd;
    
    xprintf("Testing: 0x49 - dopen\n");
    delay(2);

    sprintf(path, "%s/", prefix[prefix_idx]);

    fd = opendir(path);
    if (fd != -1) {
        iop_fd = ps2sdk_get_iop_fd(fd->dd_fd);
        xprintf("[PASS] fd: %i\n", iop_fd);
    } else {
        xprintf("[FAIL] error: %i\n", fd);
        return;
    }
    xprintf("\n");

    xprintf("Testing: 0x4A - dclose\n");
    delay(2);

    res = closedir(fd);
    if (res >= 0) {
        xprintf("[PASS] closed: %i\n", iop_fd);
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
    io_dirent_t dirent;

    sprintf(path, "%s/", prefix[prefix_idx]);
    
    fd = fioDopen(path);
    if (fd < 0) {
        xprintf("Failed to open /, %i\n", fd);
        return;
    }

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

    xprintf("closing\n");
    delay(2);

    fioDclose(fd);
    if (res > 0) {
        xprintf("[PASS] Read %i dirents\n", res);
    } else {
        xprintf("[FAIL] Failed to read any dirents\n");
    }
    xprintf("\n");
}

/* getstat /256.bin
*/
static void test_fs_getstat()
{
    int res;
    struct stat st;

    sprintf(path, "%s/256.bin", prefix[prefix_idx]);

    xprintf("Testing: 0x4C - getstat 256.bin\n");
    delay(2);

    res = stat(path, &st);
    if (res != 0) {
        xprintf("[FAIL] Failed to get stat for 256.bin\n");
    } else {
        xprintf("[PASS]\n");
        xprintf("Mode: 0x%x, Size: 0x%x\n", st.st_mode, st.st_size);
        xprintf("st_ctime: %jd\n", st.st_ctime);
        xprintf("st_mtime: %jd\n", st.st_mtime);
        xprintf("st_atime: %jd\n", st.st_atime);
        xprintf("\n");
    }
}

static int read_sector(int fd, int transfer_type, uint32_t sector, uint32_t num_sectors, uint8_t * outBuffer)
{
    struct mmce_read_sector_args args;
    // __TESTING__
    // TODO: this is a nasty hack, i'll need some help with this
    // A: for the file descriptor
    // B: to read more than one sector at a time fia fileXioDevctl
    args.fd = fd-1;
    args.type = transfer_type;
    args.start_sector = sector;
    args.num_sectors = num_sectors;
    
    return fileXioDevctl(path, MMCE_CMD_FS_READ_SECTOR, &args, sizeof(args),  outBuffer, num_sectors * 2048);
}

// Checks that the first 4 bytes match the sector number
// and that bytes 0->2044 match the CRC32 in the last 4 bytes
static int validate_sector(u8 *buffer, u32 expected_sector)
{
    int res = 0;

    // the first four bytes should be the actual sector number
    u32 read_sector = *(uint32_t*)&buffer[0];

    if (expected_sector != read_sector) {
        xprintf("Invalid sector header bytes (sector number)\n");
        xprintf("Expected 0x%x (%d)\n", expected_sector, expected_sector);
        xprintf("Got 0x%x (%d)\n", read_sector, read_sector);
        return -1;
    }

    //xprintf("Sector %d index valid\n", expected_sector);

    u32 read_crc = *(uint32_t*)&buffer[2048-4];
    u32 calc_crc = crc_calc(buffer, 2048-4);
    
    if ( read_crc != calc_crc ){
        xprintf("Invalid CRC32 on sector %d\n", expected_sector);
        xprintf("Expected CRC32 %x \n", read_crc);
        xprintf("Calced CRC32 %x \n", calc_crc);
        return -1;
    }

    return res;
}

static void test_fs_sectors()
{

    // should probs cache/precalc
    crc_init_table();

    int iop_fd;

    xprintf("Starting sector read from test.iso\n");
    delay(1);

    sprintf(path, "%s/test.iso", prefix[prefix_idx]);

    int fd = open(path, O_RDONLY);

    if (fd != -1)
    {
        iop_fd = ps2sdk_get_iop_fd(fd);
        xprintf("[PASS] fd: %i\n", iop_fd);
    }
    else
    {
        xprintf("[FAIL] error: %i\n", fd);
        return;
    }

    xprintf("Opened %s with fd\n", path, fd);

    // e.g. a block of 16 sectors
    int sectors_per_block = 16;

    // how many of these big blocks should we read?
    int file_size = 1024 * 1024 * 1024;
    int file_num_sectors = file_size / 2048;
    int max_block = file_num_sectors - sectors_per_block;

    int num_blocks = max_block / 4;

    u32 bufferSize = sectors_per_block * 2048;
    // would probs be fine on the stack
    // but saves hunting for issues later
    u8 *buffer = malloc(2048);

    if (buffer == NULL)
    {
        xprintf("Failed malloc 0x%x bytes\n", bufferSize);
        goto cleanup;
    }

    u32 total_sectors_read = 0;

    for (u32 block = 0; block < num_blocks; block++)
    {

        u32 block_start = block + 7;
        //xprintf("Block %d of %d: read %d sectors 0x%x-0x%x\n", block, num_blocks, sectors_per_block, block_start, block_start + sectors_per_block);

        for (u32 v = 0; v < sectors_per_block; v++)
        {

            u32 sector = block_start + v;

            int num_to_read = 1;
            int num_read = read_sector(fd, 0, sector, num_to_read, buffer);

            if (num_read != num_to_read)
            {   
                xprintf("ERRROR on sector %d (read %d so far)\n", sector, total_sectors_read);
                xprintf("Expected %d sectors, got %d\n", num_to_read, sector, num_read);
                goto cleanup;
            }
            total_sectors_read += num_read;

            int isValid = validate_sector(buffer, sector);
            if (isValid != 0)
            {

                xprintf("Re-reading the same sector for comparison\n");
                delay(1);
                num_read = read_sector(fd, 0, sector, num_to_read, buffer);

                if (num_read != num_to_read)
                {
                    xprintf("Failed to re-read\n");
                    goto cleanup;
                }

                validate_sector(buffer, sector);

                xprintf("Sector %d at address 0x%x failed, exiting\n", sector, sector * 2048);
                goto cleanup;
            }
        }
        
    }

    cleanup:
    free(buffer);
    close(fd);
}

void mmce_fs_auto_tests()
{
    int read_write_size = 262144; //256KB

    xprintf("Performing MMCE FS auto test sequence...\n");
    delay(2);
 
    test_fs_open_close();
    delay(2);

    test_fs_read(&read_write_size);
    delay(2);

    test_fs_write(&read_write_size);
    delay(2);

    test_fs_lseek();
    delay(2);

    test_fs_lseek64();
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

    test_fs_getstat();
    delay(2);

    xprintf("Auto test complete\n");
}

menu_item_t mmce_fs_menu_items[] = {
    {
        .text = "MMCE Slot:",
        .func = NULL,
        .func_inc = &prefix_inc,
        .func_dec = &prefix_dec,
        .arg = &prefix_idx
    },
    {
        .text = "Open & Close 256.bin",
        .func = &test_fs_open_close,
        .arg = NULL
    },
    {
        .text = "Read amount from 256.bin: ",
        .func = &test_fs_read,
        .func_inc = &pow_two_size_inc,
        .func_dec = &pow_two_size_dec,
        .arg = &read_size
    },
    {
        .text = "Read 512 bytes beyond 256.bin",
        .func = &test_fs_read_beyond,
        .arg = NULL
    },
    {
        .text = "Read sweep (256 - 256KB)",
        .func = &test_fs_read_sweep,
        .arg = NULL
    },
    {
        .text = "Write amount to 256.bin: ",
        .func = &test_fs_write,
        .func_inc = &pow_two_size_inc,
        .func_dec = &pow_two_size_dec,
        .arg = &write_size
    },
    {
        .text = "Test lseek w/ 256.bin",
        .func = &test_fs_lseek,
        .arg = NULL
    },
    {
        .text = "Test lseek64 w/ 256.bin",
        .func = &test_fs_lseek64,
        .arg = NULL
    },
    {
        .text = "Mkdir test_dir",
        .func = &test_fs_mkdir,
        .arg = NULL
    },
    {
        .text = "Rmdir test_dir",
        .func = &test_fs_rmdir,
        .arg = NULL
    },
    {
        .text = "Remove test_file.bin",
        .func = &test_fs_remove,
        .arg = NULL
    },
    {
        .text = "Dopen & Dclose /",
        .func = &test_fs_dopen_dclose,
        .arg = NULL
    },
    {
        .text = "Dread /",
        .func = &test_fs_dread,
        .arg = NULL
    },
    {
        .text = "Get stat 256.bin",
        .func = &test_fs_getstat,
        .arg = NULL
    },
    {
        .text = "Test reads from test.iso",
        .func = &test_fs_sectors,
        .func_inc = &lfsr_size_inc,
        .func_dec = &lfsr_size_dec,
        .arg = &lfsr_start_value
    },
};

menu_input_t mmce_fs_menu_inputs[] = {
};

menu_t mmce_fs_menu = {
    .header = "MMCE FS Tests",
    .items = (sizeof(mmce_fs_menu_items) / sizeof(menu_item_t)),
    //.inputs = (sizeof(mmce_fs_menu_inputs) / sizeof(menu_input_t)),
    .inputs = 0,
    .menu_inputs = &mmce_fs_menu_inputs[0],
    .menu_items = &mmce_fs_menu_items[0],
};
