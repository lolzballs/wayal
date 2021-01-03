#ifndef _WAYAL_INPUT_H
#define _WAYAL_INPUT_H

#include <stdbool.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

struct wayal_window;

enum wayal_input_repeat_state {
    KEYREPEAT_NONE,
    KEYREPEAT_TIMEOUT,
    KEYREPEAT_REPEAT,
};

struct wayal_input {
    struct {
        struct xkb_context *context;
        struct xkb_keymap *keymap;
        struct xkb_state *state;

        int repeat_timerfd;
        int32_t repeat_rate;
        int32_t repeat_delay;
        uint32_t repeat_char;
        enum wayal_input_repeat_state repeat_state;
    } keyboard;
};

extern const struct wl_keyboard_listener input_keyboard_listener;

struct wayal_input* input_create();
void input_destroy(struct wayal_input *input);

bool input_repeat_handler(struct wayal_input *input, struct wayal_window *window);

#endif
