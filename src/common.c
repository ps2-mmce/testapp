#include <tamtypes.h>
#include <time.h>

#include "include/common.h"

void delay(int seconds)
{
    struct timespec tv;
    tv.tv_sec = seconds;
    tv.tv_nsec = 0;
    nanosleep(&tv, NULL);
}

// not vsynced, but somewhere in the ballpack
// of 60fps so we don't overwhelm the pad
void delayframe()
{
    struct timespec tv;
    tv.tv_sec = 0;
    tv.tv_nsec = 16000000;
    nanosleep(&tv, NULL);
}