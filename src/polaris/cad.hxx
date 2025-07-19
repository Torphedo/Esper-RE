#pragma once
#include <imgui.h>
#include <imgui_hex_editor.h>

#include <common/int.h>
#include "fileclass.hxx"

struct cad : public fileclass {
    MemoryEditor hex_edit;

    // fileclass overrides
    virtual bool load_verify() const noexcept override;

    void do_gui() noexcept;
};