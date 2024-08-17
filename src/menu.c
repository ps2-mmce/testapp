#include <stdio.h>
#include <string.h>

#include "include/common.h"
#include "include/menu.h"
#include "include/pad.h"

static int cursor_pos = 0;

static menu_t *last_menu;

void menu_draw(menu_t *menu) {
    
    int header_len = strlen(menu->header);
    int header_x = header_x = 40 - (header_len/2);

    scr_clear();

    //Reset cursor pos on menu change
    if (last_menu != menu) {
        last_menu = menu;
        cursor_pos = 0;
    }

    //Draw header and page controls
    xprintf("<-L1");
    scr_setXY(header_x, 0);
    xprintf("%s", menu->header);
    scr_setXY(71, 0);
    xprintf("R1->");
    xprintf("\n");

    char cursor = ' ';
    for (int i = 0; i < menu->items; i++) {
        if (i == cursor_pos)
            cursor = '>';
        else
            cursor = ' ';
        
        //Draw cursor in color
        scr_setfontcolor(0x66ff00);
        xprintf("%c", cursor);
        scr_setfontcolor(0xffffff);

        //Draw menu item
        xprintf("%s", menu->menu_items[i].text);
        //Draw menu item arg
        if (menu->menu_items[i].arg != 0x0) {        
            void *arg = menu->menu_items[i].arg;
            int value = *(int*)arg;
            xprintf("%i", value);
        }

        xprintf("\n");
    }
}

void menu_update_input(menu_t *menu)
{    
    //Process global menu keybinds
    if (released(PAD_UP) && cursor_pos > 0)
        cursor_pos--;

    if (released(PAD_DOWN) && cursor_pos < menu->items - 1)
        cursor_pos++;

    //Val increment/decrement funcs
    if (released(PAD_RIGHT) && menu->menu_items[cursor_pos].func_inc != NULL) {
        menu->menu_items[cursor_pos].func_inc(menu->menu_items[cursor_pos].arg);
    }

    if (released(PAD_LEFT) && menu->menu_items[cursor_pos].func_dec != NULL) {
        menu->menu_items[cursor_pos].func_dec(menu->menu_items[cursor_pos].arg);
    }

    //Process menu specific keybinds
    for (int i = 0; i < menu->inputs; i++) {
        if (released(menu->menu_inputs[i].mask) && menu->menu_inputs[i].func != NULL) {
            menu->menu_inputs[i].func();
        }
    }

    //Call func at cursor
    if (released(PAD_CROSS) && menu->menu_items[cursor_pos].func != NULL) {
        scr_clear();
        if (menu->menu_items[cursor_pos].arg != NULL)
            menu->menu_items[cursor_pos].func(menu->menu_items[cursor_pos].arg);
        else
            menu->menu_items[cursor_pos].func();

        xprintf("\n");
        xprintf("Press () to continue...\n");
        while (!released(PAD_CIRCLE)) {
            delayframe();
            update_pad();
        }
    }
}