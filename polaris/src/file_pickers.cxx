#include "file_pickers.hxx"
#include <stdlib.h>

bool nativepicker_open(path_callback callback, const nfdu8filteritem_t* filters, u32 num_filters, const char* default_filename) noexcept {
    nfdopendialogu8args_t args = {0};
    args.filterList = filters;
    args.filterCount = num_filters;
    char* path = NULL;
    nfdresult_t result = NFD_OpenDialogU8_With(&path, &args);

    if (result == NFD_OKAY && path != NULL) {
        (callback)(path);
    }

    free(path);
    return result == NFD_OKAY;
}
