#ifndef XDG_APP_H
#define XDG_APP_H

#include <gio/gdesktopappinfo.h>
#include <wayland-client.h>

struct xdg_app {
    GDesktopAppInfo *app_info;
    struct wl_list link;
};

struct wl_list *xdg_app_search(char *string);
void xdg_app_destroy(struct xdg_app *app);

char *xdg_app_get_name(struct xdg_app *app);
void xdg_app_launch(struct xdg_app *app);

#endif
