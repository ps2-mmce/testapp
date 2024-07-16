#ifndef MMCE_CMD_TESTS_H
#define MMCE_CMD_TESTS_H

enum mmceman_cmds {
    MMCEMAN_CMD_PING = 0x1,
    MMCEMAN_CMD_GET_STATUS,
    MMCEMAN_CMD_GET_CARD,
    MMCEMAN_CMD_SET_CARD,
    MMCEMAN_CMD_GET_CHANNEL,
    MMCEMAN_CMD_SET_CHANNEL,
    MMCEMAN_CMD_GET_GAMEID,
    MMCEMAN_CMD_SET_GAMEID,
    MMCEMAN_IOCTL_PROBE_PORT,
};

void mmce_cmd_auto_tests();
void menu_mmce_cmd_tests();

#endif