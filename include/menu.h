#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include <stdio.h>
#include <debug.h>

#define xprintf(f_, ...)         \
    printf((f_), ##__VA_ARGS__); \
    scr_printf("    ");          \
    scr_printf((f_), ##__VA_ARGS__);

typedef struct menu_item_t {
    const char* text;
    void (*func)();
    void (*func_inc)(void *arg);
    void (*func_dec)(void *arg);
    void *arg;
} menu_item_t;

typedef struct menu_input_t {
    uint32_t mask;
    void (*func)();
} menu_input_t;

typedef struct menu_t {
    const char* header;
    int items;
    int inputs;
    menu_input_t *menu_inputs;
    menu_item_t *menu_items;
} menu_t;

void menu_draw(menu_t *menu);
void menu_update_input(menu_t *menu);

#endif