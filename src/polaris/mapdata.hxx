#pragma once
#include <common/int.h>

#include <imgui.h>
#include <imgui_hex_editor.h>

#include "fileclass.hxx"

// This class represents ".dat" map files in memory
struct mapdata : public fileclass {
public:
    MemoryEditor hex_edit;
    u32 selected_chunk = 0;
    const char* filepath = nullptr;

    bool initialized = false;

    // fileclass overrides
    virtual bool load_verify() const noexcept override;

    mapdata() = default;
    mapdata(const char* filepath);
    mapdata& operator=(mapdata&& other);
    ~mapdata();

    /// @brief Render and update all the UI
    void do_gui() noexcept;
};
