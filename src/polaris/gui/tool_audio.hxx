#pragma once
#include <memory>

#include "util/fileclass.hxx"
#include "ma_stx_player.h"

#include <formats/stx.h>
#include <formats/sth2.h>

/// ImGui tool menu for audio files
struct audio_tool : fileclass {
    bool enabled = false; // Whether the window is showing
    u32 sample_rate = PD_SAMPLE_RATE_UWP;
    bool is_stx = false;
    ma_stx_player stx_player;

    void do_gui() noexcept;
    void do_gui_bin() noexcept;
    void do_gui_stx() noexcept;

    // Shortcut to parse the file and get the 'WAVE' section header
    const sth2_wave_header* get_wave_header() const noexcept;

    /// @brief Shortcut to get audio data.
    /// @param audio_size_out Location to receive audio size
    /// @param audio_ptr_out Location to receive audio data pointer. Cannot be NULL.
    void get_audio(u32& audio_size_out, const u8** audio_ptr_out) const noexcept;

    bool dump_entire_to_wav(const char* outpath) const noexcept;
    bool dump_clips_to_wav(const char* out_dir, const char* prefix) const noexcept;
};

/// @brief Shortcut to calculate size of the audio buffer
u32 audio_size_from_header(const sth2_wave_header& header);
