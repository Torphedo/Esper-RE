#include "gui_loop.hxx"

#include <common/logging.h>

int main() {
    // In main() we only "kick off" the program, and gui_main() handles the
    // setup/main loop/teardown.

    // It feels a little silly to have one main() that basically just calls
    // another, but I also want to support a "headless" mode later. That'll be
    // much easier to handle if this function isn't full of GUI bootstrapping.

    // polaris::do_gui() has the real UI code, and is basically the real entry
    // point. Sorry for the kind of unintuitive structure.
    enable_win_ansi();
    if (!gui_main()) {
        // Actual error message is reported at the failure point
        LOG_MSG(error, "Failed to start up!\n");
    }

    return 0;
}