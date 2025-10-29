#pragma once
#include <common/int.h>

#include <imgui.h>
#include <imgui_hex_editor.h>

#include "fileclass.hxx"
#include <formats/st00.h>

struct polaris;

// This class represents ".dat" map files in memory
struct mapdata : public fileclass {
public:
    polaris& pol;
    const char* filepath = nullptr;
    bool initialized = false;

    MemoryEditor hex_edit;

    // fileclass overrides
    virtual bool load_verify() const noexcept override;

    mapdata(polaris& pol) : pol(pol) {
        return;
    }
    mapdata(const char* filepath, polaris& pol);
    mapdata& operator=(mapdata&& other);
    ~mapdata();

    const char* name_at_idx(u32 idx) const noexcept;
    st00_t* get_header() {
        return (st00_t*)data;
    }
    void edit_ps01_entry(u32 idx, ps01_entry* entry, u32 max_id) noexcept;
    void edit_cp00_entries(s32 offset) noexcept;

    void draw_custom_editor();

    /// @brief Render and update all the UI
    void do_gui() noexcept;
};
