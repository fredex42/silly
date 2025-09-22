#include <types.h>
#include "controller.h"
#include "keymap.h"
#include "builtin_keymap.h"
#include <panic.h>

char *active_keymap_ptr = builtin_keymap;

void set_active_keymap(char *keymap, size_t len)
{
    if(!keymap) {
        kputs("ERROR set_active_keymap called with invalid keymap value\r\n");
        return;
    }

    uint16_t keymap_len = *(uint16_t *) keymap; //first 2 bytes are the map length
    if(keymap_len != (uint16_t) len) {
        kputs("ERROR keymap provided to set_active_keymap had inconsistent length\r\n");
        kprintf("Keymap said 0x%x, provided length was 0x%x\r\n", keymap_len, len);
        return;
    }

    active_keymap_ptr = keymap;
}

void set_builtin_keymap()
{
    kputs("INFO Resetting to built-in keymap\r\n");
    active_keymap_ptr = builtin_keymap;
}

/**
 * Returns the ASCII code (if available) for the given scancode using the given shiftmask (state of CTRL, ALT, SHIFT etc. keys)
 * Conversion is carried out by the currently active keymap
*/
char lookup_scancode(char scancode, struct PS2KeyboardState* kbd_state)
{
    if(!active_keymap_ptr) {
        k_panic("lookup_scancode called without an active keymap!\r\n");
    }

    uint16_t keymap_len = *(uint16_t *) active_keymap_ptr;  //first 2 bytes are the map length
    size_t keymap_pos = ( (scancode & 0xFF) * 2); //code 1 is at position 3+4 (0-based byte 2 + 3), there is no code 0
    if(kbd_state->shift_state || kbd_state->caps_lock) keymap_pos+=1;  //shifted equivalent is at +1
    if(keymap_pos>keymap_len) {
        //kprintf("ERROR scancode 0x%x out of range, limit was 0x%x\r\n", (uint32_t)(scancode & 0xFF), (uint32_t)keymap_len);
        return 0;
    }
    return active_keymap_ptr[keymap_pos];
}