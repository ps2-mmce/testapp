#include "include/menu.h"
#include "include/mmce_mc_tests.h"

//TODO: Memory Card Tests
static void stub()
{

};


menu_item_t mmce_mc_menu_items[] = {
    {
        .text = "Not implemented",
        .func = &stub,
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


