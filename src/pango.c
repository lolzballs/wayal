#include "pango.h"

PangoLayout *get_pango_layout(cairo_t *cairo, const char *font, const char *text) {
    PangoLayout *layout = pango_cairo_create_layout(cairo);
    PangoFontDescription *desc = pango_font_description_from_string(font);

    pango_layout_set_text(layout, text, -1);
    pango_layout_set_font_description(layout, desc);
    pango_layout_set_single_paragraph_mode(layout, 1);

    pango_font_description_free(desc);
    return layout;
}

void pango_printf(cairo_t *cairo, const char *font, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    // Add one since vsnprintf excludes null terminator.
    int length = vsnprintf(NULL, 0, fmt, args) + 1;
    va_end(args);

    char *buf = malloc(length);
    if (buf == NULL) {
        printf("failed to allocate mem\n");
        exit(EXIT_FAILURE);
    }

    va_start(args, fmt);
    vsnprintf(buf, length, fmt, args);
    va_end(args);

    PangoLayout *layout = get_pango_layout(cairo, font, buf);
    cairo_font_options_t *fo = cairo_font_options_create();
    cairo_get_font_options(cairo, fo);
    pango_cairo_context_set_font_options(pango_layout_get_context(layout), fo);
    cairo_font_options_destroy(fo);
    pango_cairo_update_layout(cairo, layout);
    pango_cairo_show_layout(cairo, layout);

    g_object_unref(layout);
    free(buf);
}

