#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <loadfile.h>
#include <libpad.h>
#include <debug.h>
#include <ps2ip.h>
#include <netman.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <sifrpc.h>
#include <sbv_patches.h>
#include <libmc.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#include "include/common.h"
#include "include/pad.h"

#include "include/menu.h"

#include "include/mmce_cmd_tests.h"
#include "include/mmce_fs_tests.h"
#include "include/mmce_mc_tests.h"

#define IRX_DEFINE(mod)                     \
    extern unsigned char mod##_irx[];       \
    extern unsigned int size_##mod##_irx

extern unsigned int test_256_bin_len;
extern unsigned char test_256_bin[];

#define IRX_LOAD(mod)                                                           \
    if (SifExecModuleBuffer(mod##_irx, size_##mod##_irx, 0, NULL, NULL) < 0)    \
    printf("Could not load ##mod##\n")

IRX_DEFINE(sio2man);
IRX_DEFINE(mmceman);
IRX_DEFINE(mcman);
IRX_DEFINE(mcserv);
IRX_DEFINE(padman);

//For network printf
IRX_DEFINE(ps2dev9);
IRX_DEFINE(netman);
IRX_DEFINE(smap);
IRX_DEFINE(ps2ip_nm);
IRX_DEFINE(udptty);

IRX_DEFINE(iomanX);
IRX_DEFINE(fileXio);

#define PAGES 3

typedef struct page_t {
    menu_t *menu;
} page_t;
page_t page[PAGES]; //CMD tests, FS tests, MC tests
static int page_idx = 0;

/// @return a button was pressed on the pad, interrupting the countdown.
static bool countdown()
{
    xprintf("Auto test will run in 3 seconds, press the Any key for manual operation\n");

    for (int i = 4; i > 0; i--)
    {
        xprintf("%d...\n", i-1);
        for (int j = 0; j < 60; j++)
        {
            delayframe();
            update_pad();
            if (all_released())
            {
                return true;
            }
        }
    }
    return false;
}

static int eth_wait(int seconds)
{
    for (int timeout = 0; timeout < seconds; timeout++) {
        if (NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0) == NETMAN_NETIF_ETH_LINK_STATE_UP)
            return 0;

        delay(1);
    }
    return -1;
}

int main()
{
    struct ip4_addr IP, NM, GW, DNS;
	int EthernetLinkMode;

    SifExitIopHeap();
    SifLoadFileExit();
    SifExitRpc();
    SifInitRpc(0);

    while(!SifIopReset(NULL, 0)){};
    while(!SifIopSync()) {};
    
    SifInitRpc(0);
    SifInitIopHeap();
    SifLoadFileInit();
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check(); // Disable the MODLOAD module black/white list, allowing executables to be freely loaded from any device.
    sbv_patch_fileio();               // Prevent fileio calling mkdir after remove

    init_scr();
    scr_clear();

    //Load network modules
    IRX_LOAD(ps2dev9);
    IRX_LOAD(netman);
    IRX_LOAD(smap);
    IRX_LOAD(ps2ip_nm);

    //Example taken from PS2SDK
    //Initialize IP address.
	IP4_ADDR(&IP, 192, 168, 2, 10);
	IP4_ADDR(&NM, 255, 255, 255, 0);
	IP4_ADDR(&GW, 192, 168, 2, 1);

	//Initialize the TCP/IP protocol stack.
	ps2ipInit(&IP, &NM, &GW);

    //Wait for the link to become ready.
	xprintf("Waiting for connection...\n");
    if(eth_wait(5) != 0) {
		xprintf("Error: failed to get valid link status.\n");
	}

    IRX_LOAD(udptty);

    IRX_LOAD(iomanX);    
    IRX_LOAD(fileXio);

    fileXioInit();

    xprintf("Loading sio2man\n");
    IRX_LOAD(sio2man);
    delay(1);

    xprintf("Loading mmceman\n");
    IRX_LOAD(mmceman);
    delay(1);

    xprintf("Loading mcman\n");
    IRX_LOAD(mcman);

    xprintf("Loading mcserv\n");
    IRX_LOAD(mcserv);

    xprintf("Loading padman\n");
    IRX_LOAD(padman);
    
    //Init MC RPC
	if(mcInit(MC_TYPE_XMC) < 0) {
		printf("Failed to initialise memcard server!\n");
	}

    bool padInited = init_pad();

    if (!padInited)
    {
        xprintf("Pad init failed...\n");
        mmce_fs_auto_tests();
        mmce_cmd_auto_tests();
        return 0;
    }

    bool interrupted = countdown();

    if (!interrupted)
    {
        xprintf("No button press, running auto test...\n");
        mmce_fs_auto_tests();
        mmce_cmd_auto_tests();
        return 0;
    }

    //Assign menus to pages
    page[0].menu = &mmce_fs_menu;
    page[1].menu = &mmce_cmd_menu;
    page[2].menu = &mmce_mc_menu;

    scr_setCursor(0); //disable cursor

    while (true)
    {
        menu_draw(page[page_idx].menu);

        update_pad();

        while (!all_released())
        {
            delayframe();
            update_pad();
        }

        if (released(PAD_R1) && page_idx < (PAGES -1))
            page_idx++;

        if (released(PAD_L1) && page_idx > 0)
            page_idx--;

        menu_update_input(page[page_idx].menu);
    }

    return 0;
}
