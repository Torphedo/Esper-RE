#pragma once
#include <vector>
#include <imgui.h>
#include <imgui_hex_editor.h>

extern "C" {
    #include <GLFW/glfw3.h>
    #include <common/gl/input.h>
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
    u8* alr_data = nullptr;
    s64 alr_size = 0;
    std::vector<chunk_desc> chunks;
    // Windows for multiple chunks can be active at once, by adding multiple
    // indices (for the above vector) to this array.
    std::vector<size_t> selected_chunks;
    // Input state from the previous frame
    input_internal prev_input = {};
    // The texture currently being rendered in the background (buffpeep integration)
    img_state img_ctx = {0};

    // State for 0x2 (index buffer) window
    MemoryEditor indexBufHex;

    // State for 0x3 (transform matrix) window
    s32 selected_mat = 0;
    bool mat_slider = false;
    MemoryEditor matrixHex;

    // State for 0x10 chunk window
    u32 selected_atlas = 0;
    u32 selected_atlas_texture = 0;

    // State for 0x15 texture window
    u32 selected_texture = 0;

    // State for 0x16 chunk window
    MemoryEditor vert_bufHex;
    u32 selected_vertex_buf = 0;

    polaris();
    ~polaris();

    /// @brief Supresses input from the rest of the program if needed
    ///
    /// ImGui sometimes wants full mouse and/or keyboard control (for moving
    /// windows around, input fields, etc.). If ImGui is "in control", it sets
    /// a flag to let us know, and this function hides the real input from
    /// other code using the global input data.
    void handle_input_suppression();

    bool save_alr(const char* path);

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

    void dump_idx_buf(chunk_desc chunk, FILE* out);

    void chunk_0x2(chunk_desc chunk);

    /// @brief The menu for 0x3 transform matrix chunks.
    /// @param chunk The chunk to display
    void chunk_0x3(chunk_desc chunk);

    /// @brief The menu for 0x10 texture atlas chunks.
    /// @param chunk The chunk to display
    void chunk_0x10(chunk_desc chunk);

    /// @brief The menu for 0x11 texture chunks.
    /// @param chunk The chunk to display
    void chunk_0x11(chunk_desc chunk);

    /// @brief The menu for 0x15 texture chunks.
    /// @param chunk The chunk to display
    void chunk_0x15(chunk_desc chunk);

    /// @brief The menu for 0x15 mesh info chunks.
    /// @param chunk The chunk to display
    void chunk_0x16(chunk_desc chunk);
};
