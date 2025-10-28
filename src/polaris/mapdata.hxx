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

    // fileclass overrides
    virtual bool load_verify() const noexcept override;

    mapdata() = default;
    mapdata(const char* filepath);
    mapdata& operator=(mapdata&& other);
    ~mapdata();

    const char* name_at_idx(u32 idx) const noexcept;
    st00_t* get_header() {
        return (st00_t*)data;
    }
    void edit_ps01_entry(u32 idx, ps01_entry* entry, u32 max_id) noexcept;

    void draw_custom_editor();

    /// @brief Render and update all the UI
    void do_gui() noexcept;
};
