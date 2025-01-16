#include <nfd.h>
#include <common/int.h>

typedef bool (*path_callback)(const char* path);

/// Launch a native file picker, calling the provided callback when finished.
bool nativepicker_open(path_callback callback, const nfdu8filteritem_t* filters = nullptr, u32 num_filters = 0, const char* default_filename = nullptr) noexcept;
