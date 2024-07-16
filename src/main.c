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

#define NEWLIB_PORT_AWARE
#include <fileio.h>
#include <io_common.h>

#include "include/common.h"
#include "include/pad.h"

#include "include/mmce_cmd_tests.h"
#include "include/mmce_fs_tests.h"

#define IRX_DEFINE(mod)                     \
    extern unsigned char mod##_irx[];       \
    extern unsigned int size_##mod##_irx

extern unsigned int test_256_bin_len;
extern unsigned char test_256_bin[];

#define IRX_LOAD(mod)                                                           \
    ID = SifExecModuleBuffer(mod##_irx, size_##mod##_irx, 0, NULL, &RET);       \
    if (ID < 0 || RET == 1) printf("Could not load ##mod## (%d|%d)\n", ID, RET)

#define xprintf(f_, ...)         \
    printf((f_), ##__VA_ARGS__); \
    scr_printf("    ");          \
    scr_printf((f_), ##__VA_ARGS__);

IRX_DEFINE(mcman);
IRX_DEFINE(mmceman);
IRX_DEFINE(sio2man);
IRX_DEFINE(padman);

//For network printf
IRX_DEFINE(ps2dev9);
IRX_DEFINE(netman);
IRX_DEFINE(smap);
IRX_DEFINE(ps2ip_nm);
IRX_DEFINE(udptty);

void print_main_menu()
{
    xprintf(" \n");
    xprintf("--------------------\n");
    xprintf("^   = Run auto test sweep\n");
    xprintf("->  = MMCE Filesystem tests\n");
    xprintf("--  = MMCE Command tests\n");
    xprintf("R3 = Exit\n");
    xprintf("--------------------\n");
}

void menu_main_loop()
{
    while (true) {
        scr_clear();
        print_main_menu();

        update_pad();
        // Prevent spamming the sio every frame 
        // as if we were printing to the screen.
        while (!all_released())
        {
            delayframe();
            update_pad();
        }

        if (released(PAD_UP))
        {
            mmce_fs_auto_tests();
            mmce_cmd_auto_tests();
            delay(2);
        }
        else if (released(PAD_START))
        {
            menu_mmce_fs_tests();
        } 
        else if (released(PAD_SELECT))
        {
            menu_mmce_cmd_tests();
        }
        else if (released(PAD_R3))
        {
            break;
        }

        xprintf("Push to continue... \n" );
        update_pad();
        while (!all_released())
        {
            delayframe();
            update_pad();
        }

        xprintf("Done...\n");
    }
}
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

//TODO: temp
static void probe_port()
{
    int fd;
    int res;

    fd = open("mmce:dev", O_RDONLY);
    if (fd < 0) {
        xprintf("Failed to open mmce:dev\n");
        return;
    }
    res = fioIoctl(fd, MMCEMAN_IOCTL_PROBE_PORT, NULL);
    if (res == 2) {
        xprintf("Found MMCE on port 2 (MC2)\n");
    } else if (res == 3) {
        xprintf("Found MMCE on port 3 (MC2)\n");
    } else {
        xprintf("Failed to find MMCE\n");
    }

    close(fd);
}

int main()
{
    int ID, RET;
    struct ip4_addr IP, NM, GW, DNS;
	int EthernetLinkMode;

    SifInitRpc(0);
    while (!SifIopReset("", 0)) {};
    while (!SifIopSync()) {};

    SifInitIopHeap(); // Initialize SIF services for loading modules and files.
    SifLoadFileInit();
    sbv_patch_enable_lmb(); // The old IOP kernel has no support for LoadModuleBuffer. Apply the patch to enable it.
    sbv_patch_disable_prefix_check(); /* disable the MODLOAD module black/white list, allowing executables to be freely loaded from any device. */
    sbv_patch_fileio();     // Prevent fileio calling mkdir after remove and fix readdir

    init_scr();

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
	//Wait 5 seconds
    if(eth_wait(5) != 0) {
		xprintf("Error: failed to get valid link status.\n");
	}

    IRX_LOAD(udptty);

    xprintf("Loading sio2man\n");
    IRX_LOAD(sio2man);
    delay(1);

    xprintf("Loading mmceman\n");
    IRX_LOAD(mmceman);
    delay(1);
    
    xprintf("Loading mcman\n");
    IRX_LOAD(mcman);

    xprintf("Loading padman\n");
    IRX_LOAD(padman);

    //Temp, attempt to determine port MMCE is connected to
    probe_port();
    
    // Perform a short automatic sequence
    // if we fail to init the pads or
    // interrupt the countdown timer
    
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

    // in case you've been mashing the keys a bit
    update_pad();
    delay(1);

    menu_main_loop();

    return 0;
}
