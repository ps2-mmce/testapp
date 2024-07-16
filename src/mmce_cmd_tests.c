
#include <tamtypes.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define NEWLIB_PORT_AWARE
#include <fileio.h>
#include <io_common.h>

#include "include/pad.h"
#include "include/common.h"
#include "include/mmce_cmd_tests.h"

/* TODO: using read/write with the actual gameid length
 * can cause fileio to break the transfer down into multiple
 * calls to mmceman_fs_read/write which causes a second read 
 * or write with the current setup. For now use 
 * read/write(fd, buff, MAX_GAMEID_LEN) */
#define MAX_GAMEID_LEN 256

static int mmce_dev_fd;
static int mmce_gameid_fd;

static void wait_for_card(int timeout)
{
    int res;

    xprintf("Polling card with ping, you can safely ignore the following failed ping msgs:\n");

    for (int i = 0; i < timeout; i++) {
        res = fioIoctl(mmce_dev_fd, MMCEMAN_CMD_PING, NULL);
        if (res != -1)
            break;

        delay(1);
    }
}

static void test_cmd_ping(void)
{
    int res;

    xprintf("Testing: 0x01 - Ping\n");
    res = fioIoctl(mmce_dev_fd, MMCEMAN_CMD_PING, NULL);
    if (res != -1) {
        xprintf("[PASS] proto ver: 0x%x, prod id: 0x%x, rev id: 0x%x\n", res >> 16, (res >> 8) & 0xff, res & 0xff);
    } else {
        xprintf("[FAIL] error: %i\n", res);
    }
    xprintf("\n");
}

static void test_cmd_get_status(void)
{
    xprintf("Not implemented\n");
}

static void test_cmd_get_card(void)
{
    int res;

    xprintf("Testing: 0x3 - Get Card\n");
    res = fioIoctl(mmce_dev_fd, MMCEMAN_CMD_GET_CARD, NULL);
    if (res != -1) {
        xprintf("[PASS] current card: %i\n", res);
    } else {
        xprintf("[FAIL] error: %i\n", res);
    }
    xprintf("\n");
}

static void test_cmd_set_card(uint8_t type, uint8_t mode, uint16_t num)
{
    int res;
    int card;
    uint32_t param;

    param  = type << 24;
    param |= mode << 16;
    param |= num;

    if (mode == 0) {
        xprintf("Testing: 0x4 - Set Card [NUM] (%i)\n", num);
    } else if (mode == 1) {
        xprintf("Testing: 0x4 - Set Card [NEXT]\n");
    } else if (mode == 2) {
        xprintf("Testing: 0x4 - Set Card [PREV]\n");
    }

    card = fioIoctl(mmce_dev_fd, MMCEMAN_CMD_GET_CARD, NULL);
    if (card == -1) {
        xprintf("[FAIL] error getting current card: %i\n", card);
        return;
    }
    
    res = fioIoctl(mmce_dev_fd, MMCEMAN_CMD_SET_CARD, &param);
    if (res == -1) {
        xprintf("[FAIL] error setting card: %i\n", res);
        return;
    }

    wait_for_card(10); //Wait 10 seconds for card to resp to ping

    res = fioIoctl(mmce_dev_fd, MMCEMAN_CMD_GET_CARD, NULL);
    if (card == -1) {
        xprintf("[FAIL] error getting current card: %i\n", card);
        return;
    }

    if (card == res) {
        xprintf("[FAIL] card not changed: %i, %i\n", card, res);
    } else {
        xprintf("[PASS] card: %i -> %i\n", card, res);
    }

    xprintf("\n");  
}

static void test_cmd_get_channel(void)
{
    int res;

    xprintf("Testing: 0x5 - Get Channel\n");
    res = fioIoctl(mmce_dev_fd, MMCEMAN_CMD_GET_CHANNEL, NULL);
    if (res != -1) {
        xprintf("[PASS] current channel: %i\n", res);
    } else {
        xprintf("[FAIL] error: %i\n", res);
    }
    xprintf("\n");
}

static void test_cmd_set_channel(uint8_t mode, uint16_t num)
{
    int res;
    int card;
    uint32_t param;

    param  = mode << 16;
    param |= num;

    if (mode == 0) {
        xprintf("Testing: 0x6 - Set Channel [NUM] (%i)\n", num);
    } else if (mode == 1) {
        xprintf("Testing: 0x6 - Set Channel [NEXT]\n");
    } else if (mode == 2) {
        xprintf("Testing: 0x6 - Set Channel [PREV]\n");
    }

    card = fioIoctl(mmce_dev_fd, MMCEMAN_CMD_GET_CHANNEL, NULL);
    if (card == -1) {
        xprintf("[FAIL] error getting current channel: %i\n", card);
        return;
    }
    
    res = fioIoctl(mmce_dev_fd, MMCEMAN_CMD_SET_CHANNEL, &param);
    if (res == -1) {
        xprintf("[FAIL] error setting channel: %i\n", res);
        return;
    }

    wait_for_card(10); //Wait 10 seconds for card to resp to ping

    res = fioIoctl(mmce_dev_fd, MMCEMAN_CMD_GET_CHANNEL, NULL);
    if (card == -1) {
        xprintf("[FAIL] error getting current channel: %i\n", card);
        return;
    }

    if (card == res) {
        xprintf("[FAIL] channel not changed: %i, %i\n", card, res);
    } else {
        xprintf("[PASS] channel: %i -> %i\n", card, res);
    }

    xprintf("\n");  
}

static void test_cmd_get_gameid()
{
    int res;
    char gameid[MAX_GAMEID_LEN];

    xprintf("Testing: 0x7 - Get GameID\n");
    res = read(mmce_gameid_fd, gameid, MAX_GAMEID_LEN);

    if (res != -1) {
        xprintf("[PASS] GameID: %s\n", gameid);
    } else {
        xprintf("[FAIL] error: %i\n", res);
    }

    xprintf("\n");
}

static void test_cmd_set_gameid(char *gameid)
{
    int res;
    char old_gameid[MAX_GAMEID_LEN];
    char new_gameid[MAX_GAMEID_LEN];

    memset(old_gameid, 0, sizeof(old_gameid));
    memset(new_gameid, 0, sizeof(new_gameid));

    xprintf("Testing: 0x8 - Set GameID (%s)\n", gameid);
    res = read(mmce_gameid_fd, old_gameid, MAX_GAMEID_LEN);
    if (res == -1) {
        xprintf("[FAIL] error getting GameID: %i\n", res);
        return;
    }

    res = write(mmce_gameid_fd, gameid, MAX_GAMEID_LEN);
    if (res == -1) {
        xprintf("[FAIL] error setting GameID: %i\n", res);
        return;
    }

    wait_for_card(10); //Wait 10 seconds for card to resp to ping

    res = read(mmce_gameid_fd, new_gameid, MAX_GAMEID_LEN);
    if (res == -1) {
        xprintf("[FAIL] error getting GameID: %i\n", res);
        return;
    }

    if (strcmp(old_gameid, new_gameid) != 0) {
        xprintf("[PASS] gameid: %s -> %s\n", old_gameid, new_gameid);
    } else {
        xprintf("[FAIL] gameid: %s, %s\n", old_gameid, new_gameid);
    }

    xprintf("\n");
}

static int mmce_cmd_open_dev_fd()
{
    mmce_dev_fd = open("mmce:dev", O_RDWR);
    if (mmce_dev_fd < 0) {
        xprintf("failed to open mmce dev\n");
    } else {
        xprintf("opened dev fd: %i\n", mmce_dev_fd);
    }
}

static int mmce_cmd_open_reserved_fd()
{
    mmce_dev_fd = open("mmce:dev", O_RDWR);
    if (mmce_dev_fd < 0) {
        xprintf("failed to open mmce:dev\n");
        return -1;
    }

    mmce_gameid_fd = open("mmce:gameid", O_RDWR);
    if (mmce_gameid_fd < 0) {
        xprintf("failed to open mmce:gameid\n");
        return -1;
    }

    return 0;
}

static void mmce_cmd_close_reserved_fd()
{
    close(mmce_dev_fd);
    close(mmce_gameid_fd);
}

void mmce_cmd_auto_tests()
{
    time_t t;
    srand((unsigned)time(&t));

    char gameid[0x250] = {'S', 'L', 'U', 'S', '-', '2', '0', '9', '1', '5', '\0'};

    mmce_cmd_open_reserved_fd();

    xprintf("Performing MMCE CMD auto test sequence...\n");
    delay(2);
    
    test_cmd_ping();
    delay(2);

    test_cmd_get_card();
    delay(2);

    test_cmd_set_card(0, 1, 0);
    delay(2);

    test_cmd_set_card(0, 2, 0);
    delay(2);

    test_cmd_set_card(0, 0, rand() % 1024);
    delay(2);

    test_cmd_get_channel();
    delay(2);

    test_cmd_set_channel(1, 0);
    delay(2);

    test_cmd_set_channel(2, 0);
    delay(2);

    test_cmd_set_channel(0, (rand() % 7) + 1);
    delay(2);

    test_cmd_set_gameid(gameid);
    delay(2);

    test_cmd_get_gameid();
    delay(2);

    mmce_cmd_close_reserved_fd();

    xprintf("Auto test complete\n");
}

static void print_tests_menu()
{
    xprintf(" \n");
    xprintf("--------------------\n");
    xprintf("><  = Ping\n");
    xprintf("/\\  = Get GameID\n");
    xprintf("[]  = Set Game = Katamari (SLUS-21230) \n");
    xprintf("()  = Set Game = GT3 (PBPX-95503)\n");
    xprintf(" \n");
    xprintf("^   = Run auto test sweep\n");
    xprintf("->  = Get Card\n");
    xprintf("--  = Get Status\n");
    xprintf(" \n");
    xprintf("R1 = Next Chan\n");
    xprintf("L1 = Prev Chan\n");
    xprintf("R2 = Next Card\n");
    xprintf("L2 = Prev Card\n");
    xprintf("R3 = Return to Main Menu\n");
    xprintf("--------------------\n");
}

void menu_mmce_cmd_tests()
{
    mmce_cmd_open_reserved_fd();

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
        
        // Ping & GameID

        if (released(PAD_CROSS))
        {
            test_cmd_ping();
        }
        else if (released(PAD_CIRCLE))
        {
            // Gran Turismo 3 - A-Spec
            char gameid[0x250] = {'S', 'L', 'U', 'S', '-', '2', '1', '2', '3', '0', '\0'};
            test_cmd_set_gameid(gameid);
        }
        else if (released(PAD_SQUARE))
        {
            // Gran Turismo 3 - A-Spec
            char gameid[0x250] = {'P', 'B', 'P', 'X', '-', '9', '5', '5', '0', '3', '\0'};
            test_cmd_set_gameid(gameid);
        }
        else if (released(PAD_TRIANGLE))
        {
            test_cmd_get_gameid();
        }


        // Chan and Card Switch:

        if (released(PAD_L1))
        {
            test_cmd_set_channel(2, 0);
        }
        else if (released(PAD_L2))
        {
            test_cmd_set_card(0, 2, 0);
        }
        else if (released(PAD_L3))
        {
            //
        }
        else if (released(PAD_R1))
        {
            test_cmd_set_channel(1, 0);
        }
        else if (released(PAD_R2))
        {
            test_cmd_set_card(0, 1, 0);
        }
        else if (released(PAD_R3))
        {
            break;
        }

        // Read info:

        if (released(PAD_START))
        {
            test_cmd_get_card();
        }
        else if (released(PAD_SELECT))
        {
            test_cmd_get_status();
        }

        xprintf("Push to continue... \n" );
        update_pad();
        while (!all_released())
        {
            delayframe();
            update_pad();
        }

        xprintf("Done...\n");
        
    } // while (true);

    mmce_cmd_close_reserved_fd();
}