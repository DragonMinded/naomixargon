#include <stdio.h>
#include <string.h>
#include <naomi/maple.h>
#include <naomi/thread.h>
#include "include/keyboard.h"

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
    // Blank, this is used to set the shift statuses above in the original game, but we
    // set this in our k_pressed handler.
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
            jvs_buttons_t released = maple_buttons_released();

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

            if (pressed.player1.up)
            {
                // Up button pressed, but we also have to queue up
                // the correct scan code so you can hold the joystick
                // to continue moving.
                control_queue('8');
                keydown[0][scan_cursorup] = 1;
            }
            else if (released.player1.up)
            {
                keydown[0][scan_cursorup] = 0;
            }

            if (pressed.player1.down)
            {
                // Down button pressed, but we also have to queue up
                // the correct scan code so you can hold the joystick
                // to continue moving.
                control_queue('2');
                keydown[0][scan_cursordown] = 1;
            }
            else if (released.player1.down)
            {
                keydown[0][scan_cursordown] = 0;
            }

            if (pressed.player1.left)
            {
                // Left button pressed, but we also have to queue up
                // the correct scan code so you can hold the joystick
                // to continue moving.
                control_queue('4');
                keydown[0][scan_cursorleft] = 1;
            }
            else if (released.player1.left)
            {
                keydown[0][scan_cursorleft] = 0;
            }

            if (pressed.player1.right)
            {
                // Right button pressed, but we also have to queue up
                // the correct scan code so you can hold the joystick
                // to continue moving.
                control_queue('6');
                keydown[0][scan_cursorright] = 1;
            }
            else if (released.player1.right)
            {
                keydown[0][scan_cursorright] = 0;
            }

            if (pressed.player1.button1)
            {
                // Fire1
                k_shift = 1;
            }
            else if (released.player1.button1)
            {
                // Fire1
                k_shift = 0;
            }
            if (pressed.player1.button2)
            {
                // Fire2
                k_ctrl = 1;
            }
            else if (released.player1.button2)
            {
                // Fire2
                k_ctrl = 0;
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

void k_reset (void)
{
    // Used to clear keyboard state between some screens.
    while (k_read() != 0) { ; }

    memset(keydown, 0, sizeof(keydown));
    k_rshift = 0;
    k_lshift = 0;
    k_shift = 0;
    k_ctrl = 0;
    k_alt = 0;
    k_numlock = 0;
}

void input_reset (void)
{
    controls_needed = 1;
    controls_available = 0;
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
