#ifndef _WAYAL_H
#define _WAYAL_h

#include <stdbool.h>
#include <stdint.h>

#include <cairo/cairo.h>
#include <wayland-client.h>

#include "input.h"

struct wayal_geom {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t size;
};

struct wayal {
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct wl_output *output;
    struct wl_surface *surface;
    struct wl_shm *shm;
    struct zwlr_layer_shell_v1 *layer_shell;
    struct zwlr_layer_surface_v1 *layer_surface;
    struct wl_seat *seat;
    struct wl_keyboard *keyboard;

    struct wl_buffer *buffer;
    void *shm_buffer;

    cairo_t *cairo;
    struct wayal_input *input;
    bool running;

    struct wayal_geom geom;
};

#endif

