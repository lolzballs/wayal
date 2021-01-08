#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/timerfd.h>
#include <wayland-client.h>

#include "input.h"
#include "wayal.h"

static int set_timer_msec(int tfd, int initial, int interval) {
    struct itimerspec timerspec = {
        .it_value = {
            .tv_sec = initial / 1000,
            .tv_nsec = (initial % 1000) * 1000000,
        },
        .it_interval = {
            .tv_sec = interval / 1000,
            .tv_nsec = (interval % 1000) * 1000000,
        },
    };
    return timerfd_settime(tfd, 0, &timerspec, NULL);
}

static void keyboard_keymap_listener(void *data, struct wl_keyboard *keyboard,
        uint32_t format, int32_t fd, uint32_t size) {
    struct wayal *app = data;
    struct wayal_input *input = app->input;

    char *keymap_string = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    input->keyboard.keymap = xkb_keymap_new_from_string(
            input->keyboard.context, keymap_string,
            XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(keymap_string, size);
    close(fd);

    input->keyboard.state = xkb_state_new(input->keyboard.keymap);
}

static void keyboard_enter_listener(void *data, struct wl_keyboard *keyboard,
        uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {
    printf("enter\n");
}

static void keyboard_leave_listener(void *data, struct wl_keyboard *keyboard,
        uint32_t serial, struct wl_surface *surface) {
    printf("leave\n");
}

static void keyboard_key_listener(void *data, struct wl_keyboard *keyboard,
        uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    struct wayal *app = data;
    uint32_t character = xkb_state_key_get_utf32(app->input->keyboard.state, key + 8);

    if (key == 1) {
        app->running = false;
    }

    bool dirty = false;
    if (state) {
        if (!character) {
            uint32_t keysym = xkb_state_key_get_one_sym(app->input->keyboard.state, key + 8);
            dirty = window_key_listener(&app->window, keysym, true);
        } else {
            dirty = window_key_listener(&app->window, character, false);

            app->input->keyboard.repeat_state = KEYREPEAT_TIMEOUT;
            app->input->keyboard.repeat_char = character;

            set_timer_msec(app->input->keyboard.repeat_timerfd,
                    app->input->keyboard.repeat_delay, 0);
        }
    } else {
        if (app->input->keyboard.repeat_state != KEYREPEAT_NONE &&
                app->input->keyboard.repeat_char == character) {
            app->input->keyboard.repeat_char = 0;
            app->input->keyboard.repeat_state = KEYREPEAT_NONE;
            set_timer_msec(app->input->keyboard.repeat_timerfd, 0, 0);
        }
    }

    if (dirty)
        wayal_render(app);
}

static void keyboard_modifiers_listener(void *data, struct wl_keyboard *keyboard,
        uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
        uint32_t mods_locked, uint32_t group) {
    struct wayal *app = data;
    xkb_state_update_mask(app->input->keyboard.state, mods_depressed,
            mods_latched, mods_locked, 0, 0, group);
}

static void keyboard_repeat_info_listener(void *data, struct wl_keyboard *keyboard,
        int32_t rate, int32_t delay) {
    struct wayal *app = data;
    app->input->keyboard.repeat_rate = rate;
    app->input->keyboard.repeat_delay = delay;
}

const struct wl_keyboard_listener input_keyboard_listener = {
    keyboard_keymap_listener,
    keyboard_enter_listener,
    keyboard_leave_listener,
    keyboard_key_listener,
    keyboard_modifiers_listener,
    keyboard_repeat_info_listener,
};

struct wayal_input* input_create() {
    struct wayal_input *input =
        calloc(1, sizeof(struct wayal_input));

    input->keyboard.repeat_timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    input->keyboard.context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    return input;
}

void input_destroy(struct wayal_input *input) {
    xkb_context_unref(input->keyboard.context);
    xkb_keymap_unref(input->keyboard.keymap);
    xkb_state_unref(input->keyboard.state);

    input->keyboard.context = NULL;
    input->keyboard.keymap = NULL;
    input->keyboard.state = NULL;

    free(input);
}

bool input_repeat_handler(struct wayal_input *input, struct wayal_window *window) {
    uint64_t expirations;
    int res = read(input->keyboard.repeat_timerfd, &expirations, sizeof(uint64_t));
    assert(res == sizeof(uint64_t));

    switch (input->keyboard.repeat_state) {
        case KEYREPEAT_NONE:
            return false;
        case KEYREPEAT_TIMEOUT:
            assert(expirations == 1);

            set_timer_msec(input->keyboard.repeat_timerfd,
                    input->keyboard.repeat_rate,
                    input->keyboard.repeat_rate);
            input->keyboard.repeat_state = KEYREPEAT_REPEAT;
            return false;
        case KEYREPEAT_REPEAT:
            return window_key_listener(window, input->keyboard.repeat_char, false);
        default:
            return false;
    }
}
