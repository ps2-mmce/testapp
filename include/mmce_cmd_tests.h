#ifndef MMCE_CMD_TESTS_H
#define MMCE_CMD_TESTS_H

#include "include/menu.h"

extern menu_t mmce_cmd_menu;

enum mmceman_cmds {
    MMCE_CMD_PING = 0x1,
    MMCE_CMD_GET_STATUS,
    MMCE_CMD_GET_CARD,
    MMCE_CMD_SET_CARD,
    MMCE_CMD_GET_CHANNEL,
    MMCE_CMD_SET_CHANNEL,
    MMCE_CMD_GET_GAMEID,
    MMCE_CMD_SET_GAMEID,
    MMCE_CMD_RESET,
    MMCE_SETTINGS_ACK_WAIT_CYCLES,
    MMCE_SETTINGS_SET_ALARMS,
};

void mmce_cmd_auto_tests();
void menu_mmce_cmd_tests();

#endif