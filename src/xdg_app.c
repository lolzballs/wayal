#include "xdg_app.h"
#include <stdio.h>

struct wl_list *
xdg_app_search(char *string) {
    gchar ***orig_result = g_desktop_app_info_search(string);
    gchar ***results = orig_result;

    struct wl_list *list = calloc(sizeof(struct wl_list), 1);
    wl_list_init(list);

    while (*results != NULL) {
        gchar **temp = *results;

        while (*temp != NULL) {
            gchar *result = *temp;

            struct xdg_app *search_result = calloc(sizeof(struct xdg_app), 1);

            search_result->app_info = g_desktop_app_info_new(result);
            if (search_result->app_info != NULL) {
                wl_list_insert(list, &search_result->link);
            } else {
                free(search_result);
            }

            g_free(result);
            temp += 1;
        }

        g_free(*results);
        results += 1;
    }
    g_free(orig_result);

    return list;
}

void
xdg_app_destroy(struct xdg_app *app) {
    g_object_unref(app->app_info);
}

char *
xdg_app_get_name(struct xdg_app *app) {
    return g_desktop_app_info_get_string(app->app_info, "Name");
}

void
xdg_app_launch(struct xdg_app *app) {
    g_app_info_launch(G_APP_INFO(app->app_info), NULL, NULL, NULL);
}

