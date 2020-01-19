#include "wayal.h"

int main(int argc, char **argv) {
    struct wayal app = {0};

    // TODO: Read from config file and arguments
    struct wayal_geom geom = {
        .width = 640,
        .height = 480,
    };
    geom.stride = geom.width * 4;
    geom.size = geom.stride * geom.height;
    app.geom = geom;
    app.running = true;

    wayal_setup(&app);
    wayal_run(&app);
    wayal_finish(&app);

    return 0;
}

