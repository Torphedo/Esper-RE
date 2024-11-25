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
    // State for the overall editor
    // Currently loaded ALR & metadata for all its chunks
    u8* alr_data;
    s64 alr_size;
    std::vector<chunk_desc> chunks;
    // Input state from the previous frame
    input_internal prev_input;
    // The texture currently being rendered in the background (buffpeep integration)
    img_state img_ctx;

    // State for 0x10 chunk window
    u32 selected_atlas;
    u32 selected_atlas_texture;

    // The currently selected chunk to be displayed
    s32 selected_chunk;

    ~polaris();

    /// @brief Effectively the "real" entry point for Polaris, driving the UI
    /// @param window The main window, needed for aspect ratio & input and such
    /// @return Returns false when the user wants to exit (if ever)
    bool do_gui(GLFWwindow* window);

    /// @brief "Shatter" an ALR into all its chunks
    static std::vector<chunk_desc> shatter_alr(const u8* buf, s64 size);

    /// @brief Render a menu for the provided chunk.
    ///
    /// The user might modify the ALR data via the menu.
    /// @param chunk Metadata about the chunk to be edited
    void do_chunk_menu(chunk_desc chunk);

    /// @brief The menu for 0x10 texture atlas chunks.
    /// @param chunk The chunk to display
    void chunk_0x10(chunk_desc chunk);

    /// @brief The menu for 0x11 texture chunks.
    /// @param chunk The chunk to display
    void chunk_0x11(chunk_desc chunk);

    /// @brief The menu for 0x15 texture chunks.
    /// @param chunk The chunk to display
    void chunk_0x15(chunk_desc chunk);
};