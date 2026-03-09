#pragma once
#include <miniaudio.h>
#include <imgui.h>
#include "imgui_hex_editor.h"
#include <util/fileclass.hxx>

#include <formats/stx.h>
#include <formats/sth2.h>

/// ImGui tool menu for audio files
struct audio_tool : fileclass {
    bool enabled = false; // Whether the window is showing
    u32 sample_rate = PD_SAMPLE_RATE_UWP;
    bool is_stx = false;
    u32 sth2_selected_reat = 0;
    u32 sth2_selected_trat = 0;
    u32 sth2_selected_evnt = 0;
    MemoryEditor evnt_hex;

    stx_reader stx_player;
    ma_device device;

    void do_gui() noexcept;
    void do_gui_sth2() noexcept;
    void do_gui_sth2_simple() noexcept;
    void do_gui_sth2_advanced() noexcept;
    void do_gui_stx() noexcept;
    void setup_player();

    bool load(const char* path) noexcept override;


    // Shortcut to parse the file and get the main header
    const sth2_header* get_sth2_header() const noexcept;

    // Shortcut to parse the file and get the 'WAVE' section header
    const sth2_wave_header* get_wave_header() const noexcept;

    // Shortcut to parse the file and get the 'REAL' section header
    sth2_real_header* get_real_header() noexcept;

    // Shortcut to parse the file and get the 'REAT' section header
    reat_header* get_reat_header(u32 idx, sth2_real_header* header = nullptr) noexcept;

    trat_header* get_trat_header(u32 idx, reat_header& header) noexcept;

    /// @brief Shortcut to get audio data.
    /// @param audio_size_out Location to receive audio size
    /// @param audio_ptr_out Location to receive audio data pointer. Cannot be NULL.
    void get_audio(u32& audio_size_out, const u8** audio_ptr_out) const noexcept;

    bool dump_entire_to_wav(const char* outpath) const noexcept;
    bool dump_clips_to_wav(const char* out_dir, const char* prefix) const noexcept;
};

/// @brief Shortcut to calculate size of the audio buffer
u32 audio_size_from_header(const sth2_wave_header& header);
