#pragma once
#include <imgui.h>
#include <imgui_hex_editor.h>

#include "mapdata.hxx"

struct polaris;

// This class represents ".dat" map files in memory
struct mapdata_editor {
public:
    polaris& pol;
    mapdata map;
    bool initialized = false;

    MemoryEditor hex_edit;

    mapdata_editor(polaris& pol) : pol(pol) {
        return;
    }
    mapdata_editor(const char* filepath, polaris& pol);
    mapdata_editor& operator=(mapdata_editor&& other) noexcept;
    ~mapdata_editor();

    void edit_ps01_entry(u32 idx, ps01_entry* entry, u32 max_id) noexcept;
    void edit_ps01_entries(st00_t* header, ps01_entry* entries) noexcept;
    void edit_cp00_entries(s32 offset) noexcept;
    void edit_oc00_entries(s32 offset, s32 end_offset) noexcept;
    void dump_oc00(s32 offset, s32 end_offset) noexcept;

    void draw_custom_editor();

    /// @brief Render and update all the UI
    void do_gui() noexcept;
};
