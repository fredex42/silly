#ifndef __KEYMAP_H
#define __KEYMAP_H

// Shiftmask definitions
#define SHIFTMASK_SHIFT     1<<0
#define SHIFTMASK_ALT       1<<1
#define SHIFTMASK_CTRL      1<<2
#define SHIFTMASK_CAPS      1<<3
#define SHIFTMASK_WIN       1<<4

// Scancodes for control keys
#define SC_EXTENDED         0xe0        //Expect another byte that tells us what the actual key is
#define SC_LCRTL_MAKE       0x1d
#define SC_LCTRL_BRK        0x9d
#define SC_LALT_MAKE        0x38
#define SC_LALT_BRK         0xB8
#define SC_ALTGR_MAKE_EXT       0x38    //Note, prepended by SC_EXTENTED
#define SC_ALTGR_BRK_EXT        0xb8
#define SC_RCTRL_MAKE_EXT   0x1d
#define SC_LCTRL_BRK_EXT    0x9d
#define SC_LSHIFT_MAKE      0x2a
#define SC_LSHIFT_BRK       0xaa
#define SC_RSHIFT_MAKE      0x36
#define SC_RSHIFT_BRK       0xb6
#define SC_CAPS_MAKE        0x3a
#define SC_CAPS_BRK         0xba
#define SC_NUMLK_MAKE       0x45
#define SC_NUMLK_BRK        0xc5

char lookup_scancode(char scancode, struct PS2KeyboardState* kbd_state);
void set_builtin_keymap();
void set_active_keymap(char *keymap, size_t len);

#endif