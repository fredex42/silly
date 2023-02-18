#include <types.h>
#ifndef __PS2_CONTROLLER_H
#define __PS2_CONTROLLER_H

struct PS2KeyboardState {
    uint8_t caps_lock : 1;
    uint8_t scroll_lock : 1;
    uint8_t num_lock : 1;
    uint8_t shift_state : 1;
};

struct KeyboardBufferHeader {
    uint32_t write_ptr;
    uint32_t read_ptr;
    uint32_t buffer_size;
    char *content;
};

struct PS2DriverState {
    struct PS2KeyboardState kbd;

    uint8_t mouse_channel;
    uint8_t kbd_channel;
    
    struct KeyboardBufferHeader buffer;
};

extern struct PS2DriverState *ps2DriverState;   //this is a global, initialised in ps2_initialise

#endif