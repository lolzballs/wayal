#include "window.h"

#include "pango.h"

void window_init(struct wayal_window *window) {
    window->search_idx = 0;
}

void window_render(struct wayal_window *window, cairo_t *cairo) {
    window->search_buf[window->search_idx] = '\0';

    cairo_set_source_rgb(cairo, 255, 255, 255);
    pango_printf(cairo, "Monospace 14", window->search_buf);
}

void window_key_listener(struct wayal_window *window, uint8_t key) {
    switch(key) {
        case 0:
            return;
        case 8:
            window->search_idx--;
            break;
        default:
            window->search_buf[window->search_idx++] = key;
    }
}

