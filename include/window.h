#ifndef _WAYAL_WINDOW_H
#define _WAYAL_WINDOW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <cairo/cairo.h>

#define SEARCH_MAX 256

struct wayal_window {
    struct wayal *wayal;

    char search_buf[SEARCH_MAX];
    size_t search_idx;
};

void window_init(struct wayal_window *window, struct wayal *wayal);
void window_render(struct wayal_window *window, cairo_t *cairo);

bool window_key_listener(struct wayal_window *window, uint8_t key);

#endif

