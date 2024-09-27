#ifndef MMCE_FS_TESTS_H
#define MMCE_FS_TESTS_H

#include "include/menu.h"

extern menu_t mmce_fs_menu;

enum mmce_cmds_fs {
    MMCE_CMD_FS_OPEN = 0x40,
    MMCE_CMD_FS_CLOSE = 0x41,
    MMCE_CMD_FS_READ = 0x42,
    MMCE_CMD_FS_WRITE = 0x43,
    MMCE_CMD_FS_LSEEK = 0x44,
    MMCE_CMD_FS_IOCTL = 0x45,
    MMCE_CMD_FS_REMOVE = 0x46,
    MMCE_CMD_FS_MKDIR = 0x47,
    MMCE_CMD_FS_RMDIR = 0x48,
    MMCE_CMD_FS_DOPEN = 0x49,
    MMCE_CMD_FS_DCLOSE = 0x4a,
    MMCE_CMD_FS_DREAD = 0x4b,
    MMCE_CMD_FS_GETSTAT = 0x4c,
    MMCE_CMD_FS_CHSTAT = 0x4d,
    MMCE_CMD_FS_LSEEK64 = 0x53,
    MMCE_CMD_FS_READ_SECTOR = 0x58,
    MMCE_CMD_FS_RESET = 0x59,
};

struct mmce_read_sector_args {
    u8 fd;
    u8 reserved_1;
    u8 reserved_2;
    u8 reserved_3;
    int type;
    u32 start_sector;
    u32 num_sectors;    
};

void mmce_fs_auto_tests();
void menu_mmce_fs_tests();

#endif