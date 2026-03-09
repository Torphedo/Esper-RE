#include "tool_audio.hxx"
#include "util/utils.hxx"
#include <string>

#include <nfd.h>

#include <common/file.h>
#include <common/vfile.h>
#include <common/platform.h>
#include <common/path.h>

#include <formats/wav.h>
#include <util/imgui_utils.hxx>

static const nfdu8filteritem_t sound_filter[] = { { "Phantom Dust Sound", "bin,stx"} };
static const nfdu8filteritem_t wave_filter[] = { { "Waveform Audio File (WAV)", "wav"} };

void audio_tool::do_gui_sth2_simple() noexcept {
    ImGui::InputU32("Sample Rate (Hz)", &sample_rate);
    if (ImGui::Button("Dump all audio to WAV")) {
        char* path = nullptr;
        nfdresult_t result_out = NFD_SaveDialogU8(&path, wave_filter, ARRAY_SIZE(wave_filter), nullptr, nullptr);
        if (result_out == NFD_OKAY && path) {
            extract_sth2(this->data, this->size, path, sample_rate);
        }
    }

    if (ImGui::Button("Dump clips to WAV")) {
        char* path = nullptr;
        nfdresult_t result = NFD_PickFolderU8(&path, nullptr);
        if (result == NFD_OKAY && path) {
            dump_clips_to_wav(path, "clip");
        }
    }
}

bool EditEVNT(evnt_header* evnt, MemoryEditor& hexedit) {
    if (!evnt) {
        return false;
    }

    if (ImGui::BeginTabBar("EVNT Editor")) {
        if (ImGui::BeginTabItem("EVNT")) {
            const float duration = (evnt->clip_size / float(evnt->sample_rate)) / 2;
            ImGui::Text("Sample rate: %d Hz", evnt->sample_rate);
            ImGui::Text("Clip ID: %d", evnt->clip_idx);
            ImGui::Text("Clip size: %d", evnt->clip_size);
            ImGui::Text("Clip duration: %f seconds", duration);

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("EVNT Hex Editor")) {
            hexedit.DrawContents(evnt, sizeof(*evnt));
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    return true;
}

void audio_tool::do_gui_sth2_advanced() noexcept {
    auto* real = get_real_header();
    const u32 real_offset = get_sth2_header()->real_offset;

    ImGui::BeginChildFitContent("REAT selector");
    {
        for (u32 i = 0; i < real->num_offsets; i++) {
            const s32 offset = real->offsets[i];
            if (offset < 0) {
                continue;
            }

            std::string label;
            str_format_append(label, "REAT #%d @ 0x%X", i + 1, real_offset + offset);
            if (ImGui::Selectable(label.c_str())) {
                sth2_selected_reat = i;
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();
    int flags = ImGuiChildFlags_AutoResizeX;
    ImGui::BeginChild("EVNT editor", ImVec2(), flags);
    {
        auto* reat = get_reat_header(sth2_selected_reat, real);
        ImGui::InputU32("Selected TRAT Header", &sth2_selected_trat);
        ImGui::InputU32("Selected EVNT", &sth2_selected_evnt);

        ImGui::Text("Current REAT Header: %d/%d", sth2_selected_reat + 1, real->num_offsets);

        if (reat) {
            sth2_selected_trat = MIN(sth2_selected_trat, s32(reat->num_entries) - 1);
            ImGui::Text("Current TRAT Header: %d/%d", sth2_selected_trat + 1, reat->num_entries);

            auto* trat = get_trat_header(sth2_selected_trat, *reat);
            if (trat) {
                ImGui::Text("Current EVNT: %d/%d", sth2_selected_evnt + 1, trat->num_entries);
                sth2_selected_evnt = MIN(sth2_selected_evnt, s32(trat->num_entries) - 1);
                EditEVNT(&trat->events[sth2_selected_evnt], evnt_hex);
            } else {
                sth2_selected_evnt = 0;
            }
        } else {
            sth2_selected_trat = 0;
        }
    }
    ImGui::EndChild();
}

void audio_tool::do_gui_sth2() noexcept {
    if (ImGui::BeginTabBar("sth2_editor")) {
        if (ImGui::BeginTabItem("Simple Editor")) {
            do_gui_sth2_simple();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Advanced Editor")) {
            do_gui_sth2_advanced();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    stx_reader* player = (stx_reader*)pDevice->pUserData;

    stx_read_samples(player, frameCount, pOutput);
}

void audio_tool::setup_player() {
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_s16;  // Set to ma_format_unknown to use the device's native format.
    config.playback.channels = 2;              // Set to 0 to use the device's native channel count.
    config.sampleRate        = (1 + stx_player.header.channels[0].sample_rate);
    config.dataCallback      = data_callback; // This function will be called when miniaudio needs more data.
    config.pUserData         = &stx_player;        // Can be accessed from the device object (device.pUserData).

    ma_device_init(NULL, &config, &device);
}

void audio_tool::do_gui_stx() noexcept {
    if (ImGui::Button("Dump to WAV")) {
        char *path = nullptr;
        nfdresult_t result_out = NFD_SaveDialogU8(&path, wave_filter, ARRAY_SIZE(wave_filter), nullptr, nullptr);
        if (result_out == NFD_OKAY && path) {
            dump_stx(path, data, size);
        }
    }

    if (ImGui::Button("Play")) {
        if (stx_player.initialized) {
            ma_device_start(&device);
        }
    }

    if (stx_player.initialized) {
        ImGui::Text("[Debug] Current STX Block: %d", stx_player.audio_block_idx);
        ImGui::Text("[Debug] STX Total Blocks: %d", stx_player.header.header.block_count);
        ImGui::Text("[Debug] STX Loop Start Block: %d", stx_player.header.header.loop_start_block);
        ImGui::Text("[Debug] STX Loop End Block: %d", stx_player.header.header.loop_end_block);
    }
}

void audio_tool::do_gui() noexcept {
    if (!enabled) {
        return;
    }

    ImGui::Begin("Audio Analyzer", &enabled);
    if (data) {
        if (ImGui::Button("Unload audio file")) {
            this->unload();
            if (is_stx) {
                ma_device_uninit(&device);
                memset(&stx_player, 0, sizeof(stx_player));
            }
        }
    } else {
        ImGui::Text("No audio file loaded.");

        if (ImGui::Button("Load Audio")) {
            char* path = nullptr;
            nfdresult_t result_in = NFD_OpenDialogU8(&path, sound_filter, ARRAY_SIZE(sound_filter), nullptr);
            if (result_in == NFD_OKAY && path) {
                this->load(path);
            }
            free(path);
        }
    }

    if (!data) {
        ImGui::End();
        return;
    }

    if (is_stx) {
        do_gui_stx();
    } else {
        do_gui_sth2();
    }

    ImGui::End();
}

bool audio_tool::load(const char* path) noexcept {
    const bool res = fileclass::load(path);
    is_stx = file_has_magic(path, STX_MAGIC);
    if (is_stx && data) {
        stx_player = stx_reader_init(data, size);
        setup_player();
    }

    return res;
}

const sth2_header* audio_tool::get_sth2_header() const noexcept {
    return (const sth2_header*)data;
}

const sth2_wave_header* audio_tool::get_wave_header() const noexcept {
    vfile vf = vfile_open(data, size);
    const sth2_header header = VFILE_READ(sth2_header, &vf);

    vf.pos = header.wave_offset;
    const sth2_wave_header* pd_wave_header = VFILE_READ_PTR(sth2_wave_header, &vf);

    return pd_wave_header;
}

sth2_real_header* audio_tool::get_real_header() noexcept {
    vfile vf = vfile_open(data, size);
    const sth2_header* header = VFILE_READ_PTR(sth2_header, &vf);

    vf.pos = header->real_offset;
    auto* out = VFILE_READ_PTR(sth2_real_header, &vf);
    return out;
}

reat_header* audio_tool::get_reat_header(u32 idx, sth2_real_header* header) noexcept {
    if (!header) {
        header = get_real_header();
    }

    if (idx >= header->num_offsets) {
        return nullptr;
    }

    const s32 offset = header->offsets[idx];
    if (offset < 0) {
        return nullptr;
    }

    auto* out = (reat_header*)((u8*)header + offset);
    return out;
}

trat_header* audio_tool::get_trat_header(u32 idx, reat_header& header) noexcept {
    if (idx >= header.num_entries) {
        return nullptr;
    }

    const s32 offset = header.offsets[idx];
    if (offset < 0) {
        return nullptr;
    }

    auto* out = (trat_header*)((u8*)&header + offset);
    return out;
}

u32 audio_size_from_header(const sth2_wave_header& header) {
    const u32 offsets_size = header.num_offsets * sizeof(*header.offsets);
    const u32 header_size = sizeof(header) + offsets_size;
    const u32 audio_size = header.size - header_size;

    return audio_size;
}

void audio_tool::get_audio(u32& audio_size_out, const u8** audio_ptr_out) const noexcept {
    assert(audio_ptr_out);
    *audio_ptr_out = nullptr;

    vfile vf = vfile_open(data, size);
    const sth2_header header = VFILE_READ(sth2_header, &vf);

    vf.pos = header.wave_offset;
    const sth2_wave_header pd_wave_header = VFILE_READ(sth2_wave_header, &vf);
    const u32 offsets_size = pd_wave_header.num_offsets * sizeof(*pd_wave_header.offsets);
    vfile_seek(&vf, offsets_size);

    const u32 audio_size = audio_size_from_header(pd_wave_header);
    u8* audio = (u8*)vfile_cur(vf);

    *audio_ptr_out = audio;
    audio_size_out = audio_size;
}

bool audio_tool::dump_entire_to_wav(const char* outpath) const noexcept {
    return extract_sth2(this->data, this->size, outpath, sample_rate);
}

bool audio_tool::dump_clips_to_wav(const char* out_dir, const char* prefix) const noexcept {
    const sth2_wave_header* header = get_wave_header();
    u32 audio_size = 0;
    const u8* audio_data = nullptr;
    get_audio(audio_size, &audio_data);

    for (u32 i = 0; i < header->num_offsets; i++) {
        const u32 clip_begin = header->offsets[i];
        u32 clip_end = audio_size;
        if (i + 1 < header->num_offsets) {
            clip_end = header->offsets[i + 1];
        }

        const u8* clip_data = audio_data + clip_begin;
        const u32 clip_size = clip_end - clip_begin;

        std::string path = out_dir;
        if (path.back() != PLATFORM_DIRSEP) {
            path += PLATFORM_DIRSEP;
        }
        path += prefix + std::to_string(i) + ".wav";

        FILE* f = fopen(path.c_str(), "wb");
        if (!f) {
            LOG_MSG(error, "Failed to open output file '%s'\n", path.c_str());
            continue;
        }

        wav_write_audio(sample_rate, 1, sizeof(u16), WAV_FMT_PCM, clip_data, clip_size, f);
        fclose(f);
    }

    return true;
}