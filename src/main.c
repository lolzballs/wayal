#include "wayal.h"

int main(int argc, char **argv) {
    struct wayal app = {0};

    // TODO: Read from config file and arguments
    struct wayal_theme theme = {
        .width = 640,
        .height = 480,
        .background_color = 0xFFFFFF0F,
        .border_size = 8,
        .border_color = 0xFF00FF7F,

        .search_height = 32,
        .line_height = 18,

        .font = "Sans 18",
        .font_color = 0xFFFFFFFF,
    };
    app.theme = theme;
    app.running = true;

    wayal_setup(&app);
    wayal_run(&app);
    wayal_finish(&app);

    return 0;
}

