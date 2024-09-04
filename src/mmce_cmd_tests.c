
#include <tamtypes.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#include "include/pad.h"
#include "include/common.h"
#include "include/mmce_cmd_tests.h"

#define MAX_GAMEID_LEN 256

menu_t mmce_cmd_menu;

static int channel_num;
static int card_num;

static const char *prefix[] = {"mmce0:", "mmce1:"};
static int prefix_idx = 0;

static char path[256] = "mmce0:";

static void prefix_inc(void *arg)
{
    int num = *(int*)arg;
    if (num < 1) {
        num++;
        sprintf(path, "%s", prefix[num]);
    }
    *(int*)arg = num;
}

static void prefix_dec(void *arg)
{
    int num = *(int*)arg;
    if (num > 0) {
        num--;
        sprintf(path, "%s", prefix[num]);
    }
    *(int*)arg = num;
}

static void channel_num_inc()
{
    if (channel_num < 8)
        channel_num++;
}

static void channel_num_dec()
{
    if (channel_num > 0)
        channel_num--;
}

static void card_num_inc()
{
    if (card_num < 512)
        card_num++;
}

static void card_num_dec()
{
    if (card_num > 0)
        card_num--;
}

static void wait_for_card(int timeout)
{
    int res;

    xprintf("Polling card with ping, you can safely ignore the following failed ping msgs:\n");
  
    for (int i = 0; i < timeout; i++) {
        res = fileXioDevctl(path, MMCEMAN_CMD_PING, NULL, 0, NULL, 0);
        if (res != -1)
            break;

        delay(1);
    }
}

static void test_cmd_ping()
{
    int res;

    xprintf("Testing: 0x01 - Ping\n");
    res = fileXioDevctl(path, MMCEMAN_CMD_PING, NULL, 0, NULL, 0);
    if (res != -1) {
        xprintf("[PASS]\n");

        if (((res & 0xFF00) >> 8) == 1) {
            xprintf("Product id: 1 (SD2PSX)\n");
        } else if (((res & 0xFF00) >> 8) == 2) {
            xprintf("Product id: 2 (MemCard PRO2)\n");
        } else {
            xprintf("Product id: %i (unknown)\n", ((res & 0xFF00) >> 8));
        }
        
        xprintf("Revision id: %i\n", (res & 0xFF));
        xprintf("Protocol Version: %i\n", (res & 0xFF0000) >> 16);
    } else {
        xprintf("[FAIL] error: %i\n", res);
    }
    xprintf("\n");
}

static void test_cmd_get_status()
{
    xprintf("Not implemented\n");
}

static void test_cmd_get_card()
{
    int res;

    xprintf("Testing: 0x3 - Get Card\n");
    res = fileXioDevctl(path, MMCEMAN_CMD_GET_CARD, NULL, 0, NULL, 0);
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

    card = fileXioDevctl(path, MMCEMAN_CMD_GET_CARD, NULL, 0, NULL, 0);
    if (card == -1) {
        xprintf("[FAIL] error getting current card: %i\n", card);
        return;
    }
    
    res = fileXioDevctl(path, MMCEMAN_CMD_SET_CARD, &param, 4, NULL, 0);
    if (res == -1) {
        xprintf("[FAIL] error setting card: %i\n", res);
        return;
    }

    wait_for_card(15); //Wait 15 seconds for card to resp to ping

    res = fileXioDevctl(path, MMCEMAN_CMD_GET_CARD, NULL, 0, NULL, 0);
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

static void test_cmd_set_card_next()
{
    test_cmd_set_card(0, 1, 0);
}

static void test_cmd_set_card_prev()
{
    test_cmd_set_card(0, 2, 0);
}

static void test_cmd_set_card_direct()
{
    test_cmd_set_card(0, 0, card_num);
}

static void test_cmd_get_channel(void)
{
    int res;

    xprintf("Testing: 0x5 - Get Channel\n");
    res = fileXioDevctl(path, MMCEMAN_CMD_GET_CHANNEL, NULL, 0, NULL, 0);
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
    int chan;
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

    chan = fileXioDevctl(path, MMCEMAN_CMD_GET_CHANNEL, NULL, 0, NULL, 0);
    if (chan == -1) {
        xprintf("[FAIL] error getting current channel: %i\n", chan);
        return;
    }
    
    res = fileXioDevctl(path, MMCEMAN_CMD_SET_CHANNEL, &param, 4, NULL, 0);
    if (res == -1) {
        xprintf("[FAIL] error setting channel: %i\n", res);
        return;
    }

    wait_for_card(15); //Wait 15 seconds for chan to resp to ping

    res = fileXioDevctl(path, MMCEMAN_CMD_GET_CHANNEL, NULL, 0, NULL, 0);
    if (chan == -1) {
        xprintf("[FAIL] error getting current channel: %i\n", chan);
        return;
    }

    if (chan == res) {
        xprintf("[FAIL] channel not changed: %i, %i\n", chan, res);
    } else {
        xprintf("[PASS] channel: %i -> %i\n", chan, res);
    }

    xprintf("\n");  
}

static void test_cmd_set_channel_next()
{
    test_cmd_set_channel(1, 0);
}

static void test_cmd_set_channel_prev()
{
    test_cmd_set_channel(2, 0);
}

static void test_cmd_set_channel_direct()
{
    test_cmd_set_channel(0, card_num);
}

static void test_cmd_get_gameid()
{
    int res;
    char gameid[MAX_GAMEID_LEN];

    xprintf("Testing: 0x7 - Get GameID\n");
    fileXioDevctl(path, MMCEMAN_CMD_GET_GAMEID, NULL, 0, &gameid, MAX_GAMEID_LEN);

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

    fileXioDevctl(path, MMCEMAN_CMD_GET_GAMEID, NULL, 0, &old_gameid, MAX_GAMEID_LEN);
    if (res == -1) {
        xprintf("[FAIL] error getting GameID: %i\n", res);
        return;
    }

    fileXioDevctl(path, MMCEMAN_CMD_SET_GAMEID, gameid, MAX_GAMEID_LEN, NULL, 0);
    if (res == -1) {
        xprintf("[FAIL] error setting GameID: %i\n", res);
        return;
    }

    wait_for_card(15); //Wait 15 seconds for card to resp to ping

    fileXioDevctl(path, MMCEMAN_CMD_GET_GAMEID, NULL, 0, &new_gameid, MAX_GAMEID_LEN);
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

static void test_cmd_set_gameid1()
{
    const char *gameid = "SLUS-21230\0"; 
    test_cmd_set_gameid(gameid);
}

static void test_cmd_set_gameid2()
{
    const char *gameid = "PBPX-95503\0"; 
    test_cmd_set_gameid(gameid);
}

void mmce_cmd_auto_tests()
{
    const char *gameid = "SLUS-20915\0"; 

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

    test_cmd_set_card(0, 0, 10);
    delay(2);

    test_cmd_get_channel();
    delay(2);

    test_cmd_set_channel(1, 0);
    delay(2);

    test_cmd_set_channel(2, 0);
    delay(2);

    test_cmd_set_channel(0, 5);
    delay(2);

    test_cmd_set_gameid(gameid);
    delay(2);

    test_cmd_get_gameid();
    delay(2);

    xprintf("Auto test complete\n");
}

menu_item_t mmce_cmd_menu_items[] = {
    {
        .text = "MMCE Slot:",
        .func = NULL,
        .func_inc = &prefix_inc,
        .func_dec = &prefix_dec,
        .arg = &prefix_idx
    },
    {
        .text = "Ping",
        .func = &test_cmd_ping,
        .arg = NULL
    },
    {
        .text = "Get Status",
        .func = &test_cmd_get_status,
        .arg = NULL
    },
    {
        .text = "Get Card",
        .func = &test_cmd_get_card,
        .arg = NULL
    },
    {
        .text = "Set Card Next",
        .func = &test_cmd_set_card_next,
        .arg = NULL
    },
    {
        .text = "Set Card Prev",
        .func = &test_cmd_set_card_prev,
        .arg = NULL
    },
    {
        .text = "Set Card:",
        .func = &test_cmd_set_card_direct,
        .func_inc = &card_num_inc,
        .func_dec = &card_num_dec,
        .arg = &card_num
    },
    {
        .text = "Set Channel Next",
        .func = &test_cmd_set_channel_next,
        .arg = NULL
    },
    {
        .text = "Set Channel Prev",
        .func = &test_cmd_set_channel_prev,
        .arg = NULL
    },
    {
        .text = "Set Channel:",
        .func = &test_cmd_set_channel_direct,
        .func_inc = &channel_num_inc,
        .func_dec = &channel_num_dec,
        .arg = &channel_num
    },
    {
        .text = "Get GameID",
        .func = &test_cmd_get_gameid,
        .arg = NULL
    },
    {
        .text = "Set GameID: Katamari (SLUS-21230)",
        .func = &test_cmd_set_gameid1,
        .arg = NULL
    },
    {
        .text = "Set GameID: GT3 (PBPX-95503)",
        .func = &test_cmd_set_gameid2,
        .arg = NULL
    },
};

menu_input_t mmce_cmd_menu_inputs[] = {};

menu_t mmce_cmd_menu = {
    .header = "MMCE CMD Tests",
    .items = (sizeof(mmce_cmd_menu_items) / sizeof(menu_item_t)),
    //.inputs = (sizeof(mmce_cmd_menu_inputs) / sizeof(menu_input_t)),
    .inputs = 0,
    .menu_inputs = &mmce_cmd_menu_inputs[0],
    .menu_items = &mmce_cmd_menu_items[0],
};