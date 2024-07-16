#include <stdio.h>
#include <debug.h>

#define xprintf(f_, ...)         \
    printf((f_), ##__VA_ARGS__); \
    scr_printf("    ");          \
    scr_printf((f_), ##__VA_ARGS__);


void delay(int seconds);
void delayframe();

extern unsigned int pattern_256_bin_len;
extern unsigned char pattern_256_bin[]; 