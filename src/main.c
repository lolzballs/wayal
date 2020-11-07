#include "wayal.h"

int main(int argc, char **argv) {
    struct wayal app = {0};

    // TODO: Read from config file and arguments
    struct wayal_theme theme = {
        .width = 640,
        .height = 480,
        .border_size = 8,
        .font = "Sans"
    };
    app.theme = theme;
    app.running = true;

    wayal_setup(&app);
    wayal_run(&app);
    wayal_finish(&app);

    return 0;
}

