#ifndef _WAYAL_INPUT_H
#define _WAYAL_INPUT_H

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

struct wayal_input {
    struct {
        struct xkb_context *context;
        struct xkb_keymap *keymap;
        struct xkb_state *state;
    } keyboard;
};

extern const struct wl_keyboard_listener input_keyboard_listener;

struct wayal_input* input_create();
void input_destroy(struct wayal_input *input);

#endif
