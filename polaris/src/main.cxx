#include "gui_loop.hxx"
#include "polaris_gui.hxx"

extern "C" {
    #include <common/logging.h>
}

int main() {
    // In main() we only "kick off" the program, and gui_main() handles the
    // setup/main loop/teardown.

    // polaris_gui() has the real UI code, and is basically the real entry
    // point. Sorry if that's confusing, just trying to tuck away the
    // initialization code so the UI code is more approachable.
    if (!gui_main(polaris_gui)) {
        LOG_MSG(error, "Failed to start up!\n");
    }

    return 0;
}
