#include <libmc.h>

#include "include/menu.h"
#include "include/mmce_mc_tests.h"

/* Based on the libmc example provided in the ps2sdk
 * TODO: add tests
*/

static int mc_slot = 0;

static void slot_inc()
{
    if (mc_slot < 1)
        mc_slot++;
}

static void slot_dec()
{
    if (mc_slot > 0)
        mc_slot--;
}

static void test_mc_get_info()
{
    int res;
    int type;
    int free;
    int format;

	mcGetInfo(mc_slot, 0, &type, &free, &format);
	mcSync(0, NULL, &res);

    if (res == 0) {
        xprintf("0, Same card from last mcGetInfo call\n");
    } else if (res == -1) {
        xprintf("-1, Switched to a formatted memory card\n");
    } else if (res == -2) {
        xprintf("-2, Switched to an unformatted memory card\n");
    } else {
        xprintf("res %i\n", res);
    }

    xprintf("Type: %i\n", type);
    xprintf("Format %i\n", format);
    xprintf("Free Space: %i\n", free);
};

static void test_mc_list_dirs()
{
    int res;

    const int entries = 64;
    static mcTable mc_dir_table[64] __attribute__((aligned(64)));

    mcGetDir(0, 0, "/*", 0, entries - 10, mc_dir_table);
	mcSync(0, NULL, &res);

    if (res > 0) {
        xprintf("Found entries\n");

        for (int i = 0; i < res; i++) {
            if (mc_dir_table[i].attrFile & MC_ATTR_SUBDIR) {
                xprintf("[DIR] %s\n", mc_dir_table[i].name);
            } else {
                xprintf("[FILE] %s Size:%i\n", mc_dir_table[i].name, mc_dir_table[i].fileSizeByte);
            }
        }
    } else {
        xprintf("Failed to find dir entries\n");
    }
}

menu_item_t mmce_mc_menu_items[] = {
    {
        .text = "Memory Card: ",
        .func = NULL,
        .func_inc = &slot_inc,
        .func_dec = &slot_dec,
        .arg = &mc_slot
    },
    {
        .text = "Get MC info",
        .func = &test_mc_get_info,
        .arg = NULL
    },
    {
        .text = "List MC dirs",
        .func = &test_mc_list_dirs,
        .arg = NULL
    },
};

menu_input_t mmce_mc_menu_inputs[] = {};

menu_t mmce_mc_menu = {
    .header = "MMCE MC Tests",
    .items = (sizeof(mmce_mc_menu_items) / sizeof(menu_item_t)),
    //.inputs = (sizeof(mmce_mc_menu_inputs) / sizeof(menu_input_t)),
    .inputs = 0,
    .menu_inputs = &mmce_mc_menu_inputs[0],
    .menu_items = &mmce_mc_menu_items[0],
};


