#include <stdio.h>
#include <string.h>
#include <naomi/maple.h>
#include <naomi/thread.h>

// Flags for whether we have new controls.
int controls_needed = 1;
int controls_available = 0;

// Shared with main.c
extern mutex_t control_mutex;

static char queued_controls[256] = { 0 };
static int queued_controls_count = 0;

void readjoy (int *x, int *y)
{
    // Make sure to fake out joypresent() by stating top left for joystick position.
    *x = 0;
    *y = 0;
}

char k_rshift, k_lshift, k_shift, k_ctrl, k_alt, k_numlock;
unsigned char keydown[2][256];

void k_status (void)
{
    // Blank
}

void control_queue(char key)
{
    if (queued_controls_count < (sizeof(queued_controls) / sizeof(queued_controls[0])))
    {
        queued_controls[queued_controls_count++] = key;
    }
}

int k_pressed (void)
{
    // Queue up any new controls. Its safe to read this speculatively as we'll check
    // the real value inside the mutex.
    if (controls_available)
    {
        mutex_lock(&control_mutex);
        if (controls_available)
        {
            controls_available = 0;
            controls_needed = 1;

            jvs_buttons_t pressed = maple_buttons_pressed();

            if (pressed.player1.start)
            {
                // Enter
                control_queue(13);
            }

            if (pressed.player2.start)
            {
                // Escape
                control_queue(27);
            }
        }
        mutex_unlock(&control_mutex);
    }

    return queued_controls_count > 0;
}

int k_read (void)
{
    if (queued_controls_count == 0)
    {
        return 0;
    }

    char control = queued_controls[0];

    if (queued_controls_count == 1)
    {
        queued_controls_count = 0;
    }
    else
    {
        memmove(&queued_controls[0], &queued_controls[1], queued_controls_count - 1);
        queued_controls_count --;
    }

    return control;
}

void installhandler (unsigned char status)
{
    // Blank
}

void removehandler(void)
{
    // Blank
}

void enablebios(void)
{
    // Blank
}

void disablebios(void)
{
    // Blank
}

int biosstatus (void)
{
    // Blank
    return 0;
}
