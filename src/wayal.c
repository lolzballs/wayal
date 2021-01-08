#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <wayland-client.h>

#include "wayal.h"
#include "input.h"
#include "window.h"
#include "xdg-shell-client-protocol.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"


static struct wayal_fb *get_free_buffer(struct wayal *app) {
    if (!app->fbs[0].busy) {
        return &app->fbs[0];
    }
    if (!app->fbs[1].busy) {
        return &app->fbs[1];
    }
    return NULL;
}

static void buffer_handle_release(void *data, struct wl_buffer *wl_buffer) {
    struct wayal_fb *fb = data;
    fb->busy = false;
}

static const struct wl_buffer_listener buffer_listener = {
    .release = buffer_handle_release,
};

static void layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface,
        uint32_t serial, uint32_t width, uint32_t height) {
    printf("layer_surface_configure: %d %d\n", width, height);
    zwlr_layer_surface_v1_ack_configure(surface, serial);
}

static void layer_surface_closed(void *output, struct zwlr_layer_surface_v1 *surface) {
    printf("layer_surface_closed\n");
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    layer_surface_configure,
    layer_surface_closed
};

static void shm_format_handler(void *data, struct wl_shm *shm, uint32_t format) {
    printf("shm_format_handler: %d\n", format);
}

static const struct wl_shm_listener shm_listener = {
    shm_format_handler
};

static void global_registry_handler(void *data, struct wl_registry *registry,
        uint32_t id, const char *interface, uint32_t version) {
    struct wayal *app = data;

    printf("Got a registry event for %s id %d v%d\n", interface, id, version);
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        app->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 4);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        app->layer_shell = wl_registry_bind(registry, id, &zwlr_layer_shell_v1_interface, 2);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        app->output = wl_registry_bind(registry, id, &wl_output_interface, 3);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        app->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
        wl_shm_add_listener(app->shm, &shm_listener, data);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        app->seat = wl_registry_bind(registry, id, &wl_seat_interface, 7);
    }
}

static void global_registry_remover(void *data, struct wl_registry *registry, uint32_t id) {
    printf("Got a registry losing event for %d\n", id);
}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover,
};

static void output_frame_handle_done(void *data,
        struct wl_callback *callback, uint32_t time) {
	wl_callback_destroy(callback);

	struct wayal *app = data;
	app->frame_scheduled = false;
	if (app->frame_dirty) {
		wayal_render(app);
		app->frame_dirty = false;
	}
}

static const struct wl_callback_listener output_frame_listener = {
	.done = output_frame_handle_done
};

static struct wayal_fb create_buffer(struct wayal *app) {
    char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (xdg_runtime_dir == NULL) {
        fprintf(stderr, "XDG_RUNTIME_DIR not specified");
        exit(EXIT_FAILURE);
    }

    int fd = open(xdg_runtime_dir, O_TMPFILE | O_RDWR | O_EXCL, 0600);
    if (fd < 0) {
        perror("opening shm file");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, app->geom.size) < 0) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void *data = mmap(NULL, app->geom.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(app->shm, fd, app->geom.size);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, app->geom.width,
            app->geom.height, app->geom.stride, WL_SHM_FORMAT_ARGB8888);
    assert(buffer);
    wl_shm_pool_destroy(pool);

    close(fd);

    cairo_surface_t *cairo_surface = cairo_image_surface_create_for_data(
            data, CAIRO_FORMAT_ARGB32, app->geom.width,
            app->geom.height, app->geom.stride);
    assert(cairo_surface);

    cairo_t *cairo = cairo_create(cairo_surface);
    assert(cairo);

    struct wayal_fb ret = {
        .buffer = buffer,
        .data = data,
        .cairo = cairo,
        .surface = cairo_surface,
        .busy = false,
    };
    return ret;
}

static void flush(struct wayal *app, struct wayal_fb *fb) {
    struct wl_callback *callback = wl_surface_frame(app->surface);
    wl_callback_add_listener(callback, &output_frame_listener, app);
    app->frame_scheduled = true;

    wl_surface_attach(app->surface, fb->buffer, 0, 0);
    wl_surface_damage(app->surface, 0, 0, app->geom.width, app->geom.height);
    wl_surface_commit(app->surface);

    if (wl_display_dispatch(app->display) == -1) {
        exit(EXIT_FAILURE);
    }
}

void wayal_setup(struct wayal *app) {
    app->geom.height = app->theme.height;
    app->geom.width = app->theme.width;
    app->geom.stride = app->geom.width * 4;
    app->geom.size = app->geom.stride * app->geom.height;

    app->input = input_create();

    app->display = wl_display_connect(NULL);
    if (app->display == NULL) {
        fprintf(stderr, "wl_display_connect failed\n");
        exit(EXIT_FAILURE);
    }

    struct wl_registry *registry = wl_display_get_registry(app->display);
    wl_registry_add_listener(registry, &registry_listener, app);
    wl_display_roundtrip(app->display);

    assert(app->compositor && app->output && app->layer_shell && app->shm && app->seat);

    app->surface = wl_compositor_create_surface(app->compositor);
    assert(app->surface);
    // TODO: surface events

    app->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
            app->layer_shell, app->surface, app->output,
            ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "wayal");
    assert(app->layer_surface);
    zwlr_layer_surface_v1_add_listener(app->layer_surface,
            &layer_surface_listener, app);
    zwlr_layer_surface_v1_set_size(app->layer_surface,
            app->geom.width, app->geom.height);
    zwlr_layer_surface_v1_set_anchor(app->layer_surface, 0);
    zwlr_layer_surface_v1_set_keyboard_interactivity(app->layer_surface, true);

    app->keyboard = wl_seat_get_keyboard(app->seat);
    assert(app->keyboard);
    wl_keyboard_add_listener(app->keyboard, &input_keyboard_listener, app);

    wl_registry_destroy(registry);

    app->fbs[0] = create_buffer(app);
    app->fbs[1] = create_buffer(app);
    wl_buffer_add_listener(app->fbs[0].buffer, &buffer_listener, &app->fbs[0]);
    wl_buffer_add_listener(app->fbs[1].buffer, &buffer_listener, &app->fbs[1]);

    window_init(&app->window, app);

    wl_surface_commit(app->surface);
    wl_display_dispatch(app->display);
    wl_display_roundtrip(app->display);
}

void wayal_finish(struct wayal *app) {
    input_destroy(app->input);
    zwlr_layer_surface_v1_destroy(app->layer_surface);
    zwlr_layer_shell_v1_destroy(app->layer_shell);
    wl_surface_destroy(app->surface);
    wl_compositor_destroy(app->compositor);
    wl_shm_destroy(app->shm);
    wl_keyboard_destroy(app->keyboard);
    wl_seat_destroy(app->seat);
    wl_output_destroy(app->output);

    app->input = NULL;

    cairo_surface_destroy(app->fbs[0].surface);
    cairo_destroy(app->fbs[0].cairo);
    wl_buffer_destroy(app->fbs[0].buffer);

    cairo_surface_destroy(app->fbs[1].surface);
    cairo_destroy(app->fbs[1].cairo);
    wl_buffer_destroy(app->fbs[1].buffer);

    app->layer_surface = NULL;
    app->layer_shell = NULL;
    app->surface = NULL;
    app->compositor = NULL;
    app->shm = NULL;
    app->keyboard = NULL;
    app->seat = NULL;
    app->output = NULL;
    
    if (wl_display_roundtrip(app->display) == -1) {
        fprintf(stderr, "wl_display_roundtrip failed\n");
        exit(EXIT_FAILURE);
    }

    wl_display_disconnect(app->display);
}

void wayal_run(struct wayal *app) {
    printf("wayal_run\n");
    wayal_render(app);
    wl_display_dispatch(app->display);

    struct pollfd fds[2] = {
        {
            .fd = wl_display_get_fd(app->display),
            .events = POLLIN,
        },
        {
            .fd = app->input->keyboard.repeat_timerfd,
            .events = POLLIN,
        },
    };

    while (app->running) {
        int res = poll(fds, 2, -1);
        if (res <= 0) {
            fputs("poll() failed", stderr);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < res; i++) {
            if (fds[0].revents) {
                if (fds[0].revents & POLLIN) {
                    if (wl_display_dispatch(app->display) == -1) {
                        exit(EXIT_FAILURE);
                    }
                } else {
                    fputs("wl_display_fd failed", stderr);
                    exit(EXIT_FAILURE);
                }
            }

            if (fds[1].revents) {
                if (fds[1].revents & POLLIN) {
                    if (input_repeat_handler(app->input, &app->window)) {
                        wayal_render(app);
                    }
                }
            }
        }
    }
}

void wayal_render(struct wayal *app) {
    struct wayal_fb *fb = get_free_buffer(app);
    if (!fb)
        return;
    fb->busy = true;

    if (app->frame_scheduled)
        app->frame_dirty = true;

    printf("wayal_render\n");
    cairo_t *cairo = fb->cairo;

    cairo_set_source_rgb(cairo, 0, 0, 0);
    cairo_rectangle(cairo, 0, 0, app->geom.width, app->geom.height);
    cairo_fill(cairo);

    window_render(&app->window, cairo);

    flush(app, fb);
}

