#include <tamtypes.h>
#include <stdbool.h>
#include <libpad.h>


#include "include/common.h"
#include "include/pad.h"

static struct padButtonStatus padStat;
static char padBuf[256] __attribute__((aligned(64)));

// pad values from this poll
// and from the previous 'frame'
static uint32_t rawPadMask = 0x0000;
static uint32_t lastPadMask = 0x0000;

/// @brief Read pad vals and update prev to determine new button presses/releases
/// @returns successful read
bool update_pad()
{
    int port = 0;
    int slot = 0;

    lastPadMask = rawPadMask;

    int rv = padRead(port, slot, &padStat);

    if (rv == 0)
    {
        xprintf("padRead failed: %d\n", rv);
        rawPadMask = 0x0000;
        lastPadMask = 0x0000;
        return false;
    }

    rawPadMask = 0xFFFF ^ padStat.btns;
    
    return true;
}

bool released(int button)
{
    return ((lastPadMask & button) == button) && ((rawPadMask & button) != button);
}

bool all_released()
{
    return rawPadMask == 0x0000 && lastPadMask != 0x0000;
}

bool init_pad()
{
    // First pad only
    int port = 0;
    int slot = 0;

    int rv = 0;

    rv = padInit(port);
    if (rv != 1)
    {
        xprintf("padInit failed: %d\n", rv);
        return false;
    }

    rv = padPortOpen(port, slot, &padBuf);
    if (rv == 0)
    {
        xprintf("padPortOpen failed: %d\n", rv);
        return false;
    }

    // Wait for the pad to become ready
    rv = padGetState(port, slot);
    while ((rv != PAD_STATE_STABLE) && (rv != PAD_STATE_FINDCTP1))
    {
        if (rv == PAD_STATE_DISCONN)
        {
            xprintf("Pad port%d/slot%d) is disconnected\n", port, slot);
            return false;
        }
        rv = padGetState(port, slot);
    }

    // not necessary, but the LED is a useful indicator
    padSetMainMode(port, slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);

    // It doesn't like being rushed.
    for (int i = 0; i < 30; i++)
    {
        delayframe();
    }

    // Pre-read pad vals
    update_pad();

    xprintf("Pad %d ready in slot %d\n", port, slot);

    return true;
}