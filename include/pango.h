#ifndef _WAYAL_PANGO_H
#define _WAYAL_PANGO_H

#include <pango/pangocairo.h>

PangoLayout *get_pango_layout(cairo_t *cairo, const char *font, const char *text);
void pango_printf(cairo_t *cairo, const char *font, const char *fmt, ...);

#endif

