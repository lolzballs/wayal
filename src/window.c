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

static void render_search_box(struct wayal_window *window, cairo_t *cairo,
        PangoLayout *layout) {
    struct wayal_theme theme = window->wayal->theme;
    window->search_buf[window->search_idx] = '\0';

    set_source_rgba_u32(cairo, theme.font_color);
    cairo_move_to(cairo, theme.border_size, theme.border_size);

    pango_layout_set_text(layout, window->search_buf, window->search_idx);
    pango_cairo_update_layout(cairo, layout);
    pango_cairo_show_layout(cairo, layout);

    cairo_rel_move_to(cairo, 0, theme.search_height);
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

static void render_labels(struct wayal_window *window, cairo_t *cairo,
        PangoLayout *layout) {
    struct wayal_theme theme = window->wayal->theme;

    for (size_t i = 0; i < window->label_count; i++) {
        pango_layout_set_text(layout, window->labels[i].text, -1);
        pango_cairo_update_layout(cairo, layout);
        pango_cairo_show_layout(cairo, layout);

        cairo_rel_move_to(cairo, 0, theme.line_height);
    }
}

void window_init(struct wayal_window *window, struct wayal *wayal) {
    struct wayal_theme theme = wayal->theme;

    window->search_idx = 0;
    window->wayal = wayal;

    window->inner_height = theme.height - theme.border_size * 2;
    window->inner_width = theme.width - theme.border_size * 2;

    window->label_count = (window->inner_height - theme.search_height) /
        theme.line_height;
    window->labels = calloc(window->label_count, sizeof(struct wayal_label));

    for (size_t i = 0; i < window->label_count; i++) {
        window->labels[i].text = "a";
    }
}

void window_render(struct wayal_window *window, cairo_t *cairo) {
    struct wayal_theme theme = window->wayal->theme;

    PangoLayout *layout = pango_cairo_create_layout(cairo);
    PangoFontDescription *desc = pango_font_description_from_string(theme.font);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_MIDDLE);
    pango_layout_set_single_paragraph_mode(layout, TRUE);
    pango_layout_set_width(layout, PANGO_SCALE * window->inner_width);
    pango_layout_set_height(layout, -1);

    render_frame(window, cairo);
    render_search_box(window, cairo, layout);
    render_labels(window, cairo, layout);

    g_object_unref(layout);
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

