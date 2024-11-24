#include "gui_loop.hxx"
#include "polaris_gui.hxx"

extern "C" {
    #include <common/logging.h>
}

int main() {
    // polaris_gui() has the real UI code, in main() we only "kick off" the
    // program, and gui_main() handles the main loop.
    if (!gui_main(polaris_gui)) {
        LOG_MSG(error, "Failed to start up!\n");
    }

    return 0;
}
