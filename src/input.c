#include "input.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/mman.h>
#include <wayland-client.h>

#include "wayal.h"

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

