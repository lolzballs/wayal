#include "window.h"

#include "wayal.h"
#include "pango.h"

static void set_source_rgba_u32(cairo_t *cairo, uint32_t rgba) {
    cairo_set_source_rgba(cairo,
            (rgba >> 24 & 0xFF) / 255.0,
            (rgba >> 16 & 0xFF) / 255.0,
            (rgba >> 8 & 0xFF) / 255.0,
            (rgba >> 0 & 0xFF) / 255.0);
}

static void render_search_box(struct wayal_window *window, cairo_t *cairo) {
    struct wayal_theme theme = window->wayal->theme;
    window->search_buf[window->search_idx] = '\0';

    set_source_rgba_u32(cairo, window->wayal->theme.font_color);
    cairo_move_to(cairo, window->wayal->theme.border_size, window->wayal->theme.border_size);
    pango_printf(cairo, theme.font, window->search_buf);
}

static void render_frame(struct wayal_window *window, cairo_t *cairo) {
    struct wayal_theme theme = window->wayal->theme;
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);

    cairo_set_line_width(cairo, theme.border_size);
    cairo_rectangle(cairo, 0, 0, theme.width, theme.height);

    set_source_rgba_u32(cairo, window->wayal->theme.background_color);
    cairo_fill_preserve(cairo);

    set_source_rgba_u32(cairo, window->wayal->theme.border_color);
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

