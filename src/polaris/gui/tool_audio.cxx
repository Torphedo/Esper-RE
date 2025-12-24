#include "tool_audio.hxx"
#include <stdlib.h>

#include <imgui.h>
#include <nfd.h>
#include <common/vfile.h>
#include <common/platform.h>
#include <common/path.h>

#include <formats/pd_common.h>
#include <formats/sth2.h>
#include <formats/wav.h>
#include "util/imgui_utils.hxx"

static const nfdu8filteritem_t sound_filter[] = { { "Phantom Dust Sound", "bin,stx"} };
static const nfdu8filteritem_t wave_filter[] = { { "Waveform Audio File (WAV)", "wav"} };

void audio_tool::do_gui() noexcept {
    if (!enabled) {
        return;
    }

    ImGui::Begin("Audio Analyzer", &enabled);
    if (data) {
        if (ImGui::Button("Unload audio file")) {
            this->unload();
        }
    } else {
        ImGui::Text("No audio file loaded.");

        if (ImGui::Button("Load Audio")) {
            char* path = nullptr;
            nfdresult_t result_in = NFD_OpenDialogU8(&path, sound_filter, ARRAY_SIZE(sound_filter), nullptr);
            if (result_in == NFD_OKAY && path) {
                this->load(path);
                if (path_has_extension(path, ".stx")) {
                    is_stx = true;
                }
            }
            free(path);
        }
        ImGui::End();
        return;
    }

    if (is_stx) {
        if (ImGui::Button("Dump STX")) {
            char *path = nullptr;
            nfdresult_t result_out = NFD_SaveDialogU8(&path, wave_filter, ARRAY_SIZE(wave_filter), nullptr, nullptr);
            if (result_out == NFD_OKAY && path) {
                dump_stx(path, data, size);
            }
        }
    } else {
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

    ImGui::End();
}

const sth2_wave_header* audio_tool::get_wave_header() const noexcept {
    vfile vf = vfile_open(data, size);
    const sth2_header header = VFILE_READ(sth2_header, &vf);

    vf.pos = header.wave_offset;
    const sth2_wave_header* pd_wave_header = VFILE_READ_PTR(sth2_wave_header, &vf);

    return pd_wave_header;
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