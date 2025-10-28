#pragma once
#include <common/int.h>

#include <imgui.h>
#include <imgui_hex_editor.h>

#include "fileclass.hxx"
#include <formats/st00.h>

// This class represents ".dat" map files in memory
struct mapdata : public fileclass {
public:
    const char* filepath = nullptr;
    bool initialized = false;

    MemoryEditor hex_edit;
    u32 selected_ps00 = 0;
    u32 selected_ps01 = 0;
    s32 selected_offset = -1;
    u64 selected_size = 0;

    // fileclass overrides
    virtual bool load_verify() const noexcept override;

    mapdata() = default;
    mapdata(const char* filepath);
    mapdata& operator=(mapdata&& other);
    ~mapdata();

    st00_t* get_header() {
        return (st00_t*)data;
    }

    void draw_custom_editor();

    /// @brief Render and update all the UI
    void do_gui() noexcept;
};
