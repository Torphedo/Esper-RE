// Need this define to use operators on ImGui vector types
#define IMGUI_DEFINE_MATH_OPERATORS
#include "editor_alr.hxx"
#include <imgui_internal.h>
#include <nfd.h>

#include <common/file.h>
#include <common/vfile.h>
#include <common/vmem.h>
#include <common/logging.h>
#include <common/crc32.h>

#include <formats/pd_common.h>
#include <formats/alr.h>

#include "alr_texture.hxx"
#include "alr_dump.hxx"
#include "pd_mesh.hxx"

#include "imgui_utils.hxx"
#include "validation.hxx"
#include "alr_dump.hxx"
#include "alr_imgui.hxx"

namespace al {

resource::chunk::chunk(u32 id, s32 size, uintptr_t offset) noexcept {
    this->id = id;
    this->size = size;
    this->offset = offset;
}

bool resource::load(const char* path) noexcept {
    if (!file_exists(path)) {
        LOG_MSG(error, "I couldn't find an ALR file named \"%s\".\n", path);
        return false;
    }

    const s64 size = file_size(path);
    if (size < 8) {
        // Smallest possible ALR chunk is 8 bytes
        LOG_MSG(error, "\"%s\" is only %d bytes, but an ALR must be at least 8 bytes.\n", path, size);
        return false;
    }

    // Expand reservation if needed
    if (size > reserve_size) {
        // If our reservation needs resizing, we're dealing with a
        // truly massive file. Just add its size to the old size,
        // more space can never hurt.
        this->expand_reservation(reserve_size + size);
    }

    // Load the file into the buffer.
    if (!file_load_existing(path, data, size)) {
        // Some loading failure, an error message should've been printed
        return false;
    }
    alr_size = size;
    chunks = shatter_alr(data, alr_size);
    tex_manager.atlasheader_offset = first_chunk_by_id(0x10).offset;
    tex_manager.texheader_offset = first_chunk_by_id(0x15).offset;
    return true;
}

bool resource::save(const char* path) const noexcept {
    FILE* out = fopen(path, "wb");
    if (out == nullptr) {
        return false;
    }

    bool result = true;
    if (fwrite(data, alr_size, 1, out) != 1) {
        // Incomplete write
        result = false;
    }
    fclose(out);

    return result;
}

std::vector<resource::chunk> resource::shatter_alr(const u8* buf, s64 size) noexcept {
    // Technically we cast away const here, but we don't write any data so it's
    // fine.
    vfile vf = vfile_open((void*)buf, size);
    std::vector<resource::chunk> out;

    // Loop until we exhaust the buffer or exit early
    u32 prev_id = -1;
    while (!vfile_eof(vf)) {
        // Read chunk data. We have to copy it over 1 field at a time because we
        // don't actually want/need any more of the chunk data.
        const uintptr_t offset = vf.pos; // It's important to save offset before reading
        const u32 id = VFILE_READ(u32, &vf);
        const s32 chunk_size = VFILE_READ(s32, &vf);
        resource::chunk chunk(id, chunk_size, offset);

        if (chunk.id == 0 && prev_id == 0) {
            // There's never multiple consecutive chunks with ID 0. This means
            // we've hit an empty area, probably the end of the chunk data.
            break;
        }

        if (chunk.id == 0x11) {
            // This chunk has the resource buffer offset, save it for later
            // Our layout structure includes the id & size, so we have to seek
            // back for that...
            vf.pos -= sizeof(chunk_generic);
            chunk_layout layout = VFILE_READ(chunk_layout, &vf);
            resbuf_offset = layout.texbuf_offset;

            // Reset the position to what it used to be
            vf.pos -= sizeof(layout);
        }

        // Advance to the next chunk & add to output
        vf.pos = chunk.offset + chunk.size;
        out.push_back(chunk);
        prev_id = chunk.id; // Save current ID
    }

    return out;
}

resource::chunk resource::first_chunk_by_id(u32 id) const noexcept {
    return first_chunk_in_range(id, 0, alr_size);
}

resource::chunk resource::prev_chunk_by_id(u32 id, u32 high, u32 low) const noexcept {
    assert(low < high && "Low bound must be < high bound!");
    for (s64 i = chunks.size() - 1; i > 0; i--) {
        const chunk& c = chunks[i];
        if (high < c.offset) {
            continue; // Skip to starting offset
        }
        if (c.offset < low) {
            break; // We passed the low bound
        }
        if (c.id == id) {
            return c; // Found it!
        }
    }

    return chunk(0, 0, 0); // Nothin...
}

resource::chunk resource::first_chunk_in_range(u32 id, u32 low, u32 high) const noexcept {
    // TODO: Add an overload to find a chunk within an offset range. Since the list is sorted we can do a sort of binary search by starting @ the middle
    assert(low < high && "Low bound must be < high bound!");

    for (const auto & c : chunks) {
        if (high < c.offset) {
            break;
        }
        if (c.offset < low) {
            continue;
        }
        if (c.id == id) {
            return c; // Found it!
        }
    }

    return chunk(0, 0, 0); // Nothin...
}

void resource::draw_file() noexcept {
    if (ImGui::CollapsingHeader("Textures")) {
        const chunk tex_chunk = this->first_chunk_by_id(0x15);
        vfile vf = vf_from_chunk(tex_chunk, true);

        const u32 num_textures = VFILE_READ(u32, &vf);
        const auto* entries = (texture_entry*)vfile_cur(vf);

        for (u32 i = 0; i < num_textures; i++) {
            decoded_text name = decode_double(entries[i].text1, entries[i].text2);
            std::string label;
            str_format_append(label, "%s##%d", name.data, i);
            if (ImGui::Selectable(label.c_str())) {
                selected_tex = i;
                active_type = TYPE_TEXTURE;
            }
        }
    }

    if (ImGui::CollapsingHeader("Models")) {
        const chunk vert_chunk = this->first_chunk_by_id(0x16);
        vfile mesh_vf = vf_from_chunk(vert_chunk, true);
        const u32 num_entries = VFILE_READ(u32, &mesh_vf);
        const vertbuf_entry* entries = (vertbuf_entry*)vfile_cur(mesh_vf);

        for (u32 i = 0; i < num_entries; i++) {
            std::string entry_label;
            str_format_append(entry_label, "%d##%p", i, entries);
            if (ImGui::Selectable(entry_label.c_str())) {
                selected_mesh = i;
                active_type = TYPE_MESH;
            }
        }
    }
}
void resource::draw() noexcept {
    switch (active_type) {
    case TYPE_TEXTURE: {
        const chunk tex_chunk = this->first_chunk_by_id(0x15);
        vfile tex_vf = vf_from_chunk(tex_chunk, true);

        const u32 num_textures = VFILE_READ(u32, &tex_vf);
        auto* entries = (texture_entry*)vfile_cur(tex_vf);
        ImGui::BeginTabBar("Texture content tabs");
        if (ImGui::BeginTabItem("Texture View")) {
            gl_obj texture = tex_manager.get(*this, selected_tex);
            ImGui::draw_image(texture, 512, 512, &tex_auto_scale, &tex_manual_scale, "Active Texture");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("0x15 Chunk Entry")) {
            if (selected_tex >= num_textures) {
                ImGui::Text("Selected texture %d is out of bounds (there are only %d entries)", selected_tex, num_textures);
            } else {
                al::edit_texture_entry(entries[selected_tex]);
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
        break;
    }
    case TYPE_MESH:
        ImGui::Text("Unimplemented mesh content view");
        break;
    }
}

void resource::expand_reservation(s64 new_size) noexcept {
    if (new_size < reserve_size) {
        LOG_MSG(error, "No reason to shrink reservation from 0x%X -> 0x%X, ignoring!\n", reserve_size, new_size);
        return;
    }

    u8* new_buf = (u8*)vmem_reserve(new_size);
    if (new_buf == nullptr) {
        return;
    }

    // Commit the entire region. This ensures it's all accessible, but doesn't
    // comsume any physical memory until accessed. (May still create page file
    // entries)
    vmem_commit(new_buf, new_size);

    // Free old buffer and update our state
    if (data != nullptr) {
        vmem_free(data, reserve_size);
    }
    data = new_buf;
    reserve_size = new_size;
}

resource::resource() noexcept {
    // "Expand" our reservation from 0 bytes to... not 0.
    this->expand_reservation(reserve_size);
}

resource::~resource() noexcept {
    vmem_free(data, reserve_size);
}

} // namespace al
