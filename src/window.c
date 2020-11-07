#include "window.h"

#include "wayal.h"
#include "pango.h"

static void render_search_box(struct wayal_window *window, cairo_t *cairo) {
    struct wayal_theme theme = window->wayal->theme;
    window->search_buf[window->search_idx] = '\0';

    cairo_set_source_rgb(cairo, 255, 255, 255);
    pango_printf(cairo, theme.font, window->search_buf);
}

static void render_frame(struct wayal_window *window, cairo_t *cairo) {
    struct wayal_theme theme = window->wayal->theme;
    cairo_set_source_rgb(cairo, 255, 0, 0);
    cairo_set_line_width(cairo, theme.border_size);
    cairo_rectangle(cairo, 0, 0, theme.width, theme.height);
    cairo_stroke(cairo);
}

void window_init(struct wayal_window *window, struct wayal *wayal) {
    window->search_idx = 0;
    window->wayal = wayal;
}

void window_render(struct wayal_window *window, cairo_t *cairo) {
    render_frame(window, cairo);
    render_search_box(window, cairo);
}

bool window_key_listener(struct wayal_window *window, uint32_t key, bool control) {
    if (control)
        return false;

    switch (key) {
        case 0:
            return false;
        case 8:
            if (window->search_idx != 0)
                window->search_idx--;
            break;
        default:
            printf("%d\n", key);
            window->search_buf[window->search_idx++] = key;
            break;
    }
    return true;
}

