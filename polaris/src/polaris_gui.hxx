#pragma once
#include <vector>

extern "C" {
    #include <GLFW/glfw3.h>
    #include "common/gl/input.h"
    #include "viewer/render_image.h"
}

typedef struct {
    u32 id;
    s32 size; // Sizes can actually be signed, not sure why.
    uintptr_t offset;

    // A chunk is "orphaned" if it can't be found using the offset table
    bool orphan;
}chunk_desc;

struct polaris {
public:
    input_internal prev_input;
    img_state img_ctx;
    u8* alr_data;
    s64 alr_size;
    std::vector<chunk_desc> chunks;
    s32 selected_chunk;

    /// @brief Effectively the "real" entry point for Polaris, driving the UI
    ///
    /// @param window The window, needed for aspect ratio and input and such
    /// @return Returns false when the user wants to exit (if ever)
    bool do_gui(GLFWwindow* window);

    /// @brief "Shatter" an ALR into all its chunks
    static std::vector<chunk_desc> shatter_alr(const u8* buf, s64 size);

    /// @brief Render a menu for editing the provided chunk.
    ///
    /// The user might modify the ALR data via the menu.
    /// @param chunk Metadata about the chunk to be edited
    void do_chunk_menu(chunk_desc chunk);

    void chunk_0x10(chunk_desc chunk);
};