#include "alr_dump.hxx"
#include <vector>
#include <string.h>
#include <common/file.h>

#include <formats/alr_animations.h>
#include <formats/pd_common.h>
#include <version.h>

namespace alr {

void anim_key_info(u32 key_size, data_type& frame_type, data_type& component_type, u32& num_components) {
    frame_type = DATA_TYPE_COUNT;
    component_type = DATA_TYPE_COUNT;

    switch (key_size) {
    // Integer keys
    case 3:
    case 5:
    case 7:
        frame_type = DATA_TYPE_U8;
        component_type = DATA_TYPE_U16;
        break;

    // Floating point keys
    case 8:
    case 12:
    case 16:
        frame_type = component_type = DATA_TYPE_FLOAT;
    default:
        break;
    }

    const u32 component_size = sizeof_type(component_type);
    const u32 frame_size = sizeof_type(frame_type);

    // We know component and frame value size, so we can find out the # of components
    num_components = (key_size - frame_size) / component_size;
}

vec3s anim_read_key(const u8* key, u32 key_size, float& frame_out, const u8** next_key_out) {
    vfile vf = vfile_open(const_cast<u8*>(key), key_size);

    u32 num_components = 0;
    data_type frame_type = DATA_TYPE_COUNT;
    data_type component_type = DATA_TYPE_COUNT;
    anim_key_info(key_size, frame_type, component_type, num_components);

    float frame = 0.0f;
    switch (frame_type) {
        case DATA_TYPE_FLOAT:
            frame = VFILE_READ(float, &vf);
            break;
        case DATA_TYPE_U8:
            frame = VFILE_READ(u8, &vf);
            break;
        default:
            LOG_MSG(warning, "Unknown key format with size %d!\n", key_size);
            break;
    }

    vec3s out = {};
    assert(num_components <= ARRAY_SIZE(out.raw));
    for (u32 i = 0; i < num_components; i++) {
        float component = 0.0f;
        switch (component_type) {
            case DATA_TYPE_FLOAT:
                component = VFILE_READ(float, &vf);
                break;
            case DATA_TYPE_U16:
                component = VFILE_READ(s16, &vf);
                // Map into [0, 1] range
                component /= float(INT16_MAX);

                // Convert to radians
                component *= 2.0f * M_PI;
                break;
            default:
                LOG_MSG(warning, "Unknown key format with size %d!\n", key_size);
                break;
        }
        out.raw[i] = component;
    }

    if (next_key_out) {
        *next_key_out = &key[key_size];
    }
    frame_out = frame;
    return out;
}

s32 animation_by_idx(const u8* alr, u32 alr_size, u32 idx, u32 joint_idx) {
    vfile vf = vfile_open(const_cast<u8*>(alr), alr_size);

    const auto* layout = VFILE_READ_PTR(chunk_layout, &vf);

    const bool invalid_anim_id = (idx >= ALR_NUM_PLAYER_ANIMATIONS);
    const bool no_anims = (layout->offset_array_size < ALR_NUM_PLAYER_ANIMATIONS);
    const bool out_of_bounds = (layout->offset_array_size <= idx);
    if (invalid_anim_id || no_anims || out_of_bounds) {
        return -1;
    }

    s32 offset = layout->offsets[idx];
    if (offset < 0) {
        // Animation doesn't exist
        return -1;
    }

    vf.pos = offset;
    while (true) {
        s32 cur_offset = vf.pos;
        auto* animation_header = VFILE_READ_PTR(anim_header, &vf);
        if (animation_header->id != 0x5) {
            break; // We hit the end of the animation
        }

        if (animation_header->joint_idx == joint_idx) {
            return cur_offset;
        }
        vf.pos = cur_offset + animation_header->size;
    }

    return -1;
}

mat4s anim_xform_for_joint(u8* alr, u32 alr_size, u32 anim_id, s32 joint_idx, float cur_frame) {
    s32 anim_offset = alr::animation_by_idx(alr, alr_size, anim_id, joint_idx);
    if (anim_offset <= 0) {
        return GLMS_MAT4_IDENTITY_INIT;
    }

    vfile afile = vfile_open(alr + anim_offset, alr_size);
    const auto* aheader = VFILE_READ_PTR(anim_header, &afile);

    vec3s position = {};
    vec3s rotation = {};
    const u32 transkey_offset = afile.pos;
    for (u32 i = 0; i < aheader->translation_key_count; i++) {
        float frame = 0.0f;
        const u8* keydata = (const u8*)vfile_cur(afile);
        vec3s key = alr::anim_read_key(keydata, aheader->translation_key_size, frame);
        if (frame > cur_frame) {
            break;
        }
        vfile_seek(&afile, aheader->translation_key_size);

        position = key;
    }
    // Skip all translation keys
    afile.pos = transkey_offset + (aheader->translation_key_count * aheader->translation_key_size);

    const u32 rotkey_offset = afile.pos;
    for (u32 i = 0; i < aheader->rotation_key_count; i++) {
        float frame = 0.0f;
        const u8* keydata = (const u8*)vfile_cur(afile);
        vec3s key = alr::anim_read_key(keydata, aheader->rotation_key_size, frame);
        if (frame > cur_frame) {
            break;
        }
        vfile_seek(&afile, aheader->rotation_key_size);

        rotation = key;
    }
    // Skip all rotation keys
    afile.pos = rotkey_offset + (aheader->rotation_key_count * aheader->rotation_key_size);

    for (u32 i = 0; i < ARRAY_SIZE(rotation.raw); i++) {
        // rotation.raw[i] = glm_rad(rotation.raw[i]);
        // rotation.raw[i] *= (1.0f * M_PI);
        // rotation.raw[i] *= (2.0f * 180.0f);
    }

    mat4s rot_xform = glms_euler_zyx(rotation);
    mat4s pos_xform = glms_translate_make(position);
    mat4s anim_xform = glms_mat4_mul(pos_xform, rot_xform);
    return rot_xform;
}

// IMPORTANT: If Blender complains and won't import the DAE, make sure you
// haven't accidentally made the XML start with a newline. If you do, Blender's
// XML parser will freak out.
// Be careful with the raw string literal, you can't escape newlines.
const char* DAE_HEADER = R"(<?xml version="1.0" encoding="utf-8"?>
<COLLADA xmlns="http://www.collada.org/2005/11/COLLADASchema" version="1.4.1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <asset>
    <contributor>
      <authoring_tool>Polaris</authoring_tool>
    </contributor>
    <unit name="meter" meter="1"/>
    <up_axis>Y_UP</up_axis>
  </asset>
  <library_images/>
  <library_controllers/>
  <library_visual_scenes>
    <visual_scene id="Scene" name="Scene">
)";

const char* DAE_FOOTER = R"(
    </visual_scene>
  </library_visual_scenes>
</COLLADA>
)";

mat4s transform_from_joint(const joint_t & joint) {
    const mat4s pos = glms_translate_make(*(vec3s*)&joint.position);
    const mat4s rot = glms_euler_zyx(*(vec3s*)&joint.rotation);

    mat4s transform = glms_mat4_mul(pos, rot);
    transform = glms_scale(transform, *(vec3s*)&joint.scale);

    return transform;
}

// Just the game's structure, but made so that we can traverse down from the
// root instead of up from the leaves
struct joint_tree {
    joint_t joint;
    std::vector<u32> children;
};

void xml_dump_joint(FILE* f, const joint_tree* joints, u32 idx) {
    const joint_tree& node = joints[idx];
    if (node.joint.name == UINT32_MAX && node.children.empty()) {
        // Unnamed node that won't affect the rest of the skeleton
        return;
    }

    decoded_text name = {0};
    decode_single32(name.data, node.joint.name);
    fprintf(f, R"(<node id="%s_%d" name="%s_%d" sid="%s_%d" type="JOINT">%c)",
        name.data, idx, name.data, idx, name.data, idx, '\n');

    // We need the transform of this bone in the bind pose - the "original"
    // pose of the skeleton (usually a T-pose or A-pose).
    mat4s xform = transform_from_joint(node.joint);

    // 3D software wants the inverse bind pose transform
    // We transpose because DAE is row-major, and we're column-major
    const mat4s inv_bind_xform = glms_mat4_transpose(xform);
    fprintf(f, "<matrix sid=\"transform\">");
    const float* raw = (float*)&inv_bind_xform;
    for (u32 i = 0; i < (sizeof(mat4s) / sizeof(float)); i++) {
        fprintf(f, "%f ", raw[i]);
    }
    fprintf(f, "</matrix>");

    for (u32 i : node.children) {
        xml_dump_joint(f, joints, i);
    }

    fprintf(f, "\n</node>\n");
}

void dump_armature_dae(FILE* f, vfile armature_data) {
    if (!armature_data.ptr || armature_data.size < sizeof(chunk_armature)) {
        LOG_MSG(error, "Not exporting armature because there was nothing to export.\n");
        return;
    }
    fprintf(f, DAE_HEADER);
    fprintf(f, "%s\n", R"(<node id="Armature" name="Armature" type="NODE">)");

    const chunk_armature header = VFILE_READ(chunk_armature, &armature_data);
    auto* joints = (joint_t *) vfile_cur(armature_data);

    std::vector<joint_tree> roots(header.joint_count);
    for (u32 i = 0; i < header.joint_count; i++) {
        joint_t cur_joint = joints[i];
        roots[i].joint = cur_joint;

        // Bounds check
        if (cur_joint.parent_idx < 0 || cur_joint.parent_idx > header.joint_count - 1) {
            continue;
        }

        roots[cur_joint.parent_idx].children.push_back(i);
    }

    for (u32 i = 0; i < roots.size(); i++) {
        const bool no_name = roots[i].joint.name == UINT32_MAX;
        const bool is_root = roots[i].joint.parent_idx < 0;
        const bool out_of_bounds = roots[i].joint.parent_idx > s32(roots.size() - 1);
        const bool has_children = roots[i].children.size() > 0;

        if (!is_root || out_of_bounds || (no_name && !has_children)) {
            continue;
        }

        xml_dump_joint(f, roots.data(), i);
    }

    fprintf(f, "</node>\n");
    fprintf(f, DAE_FOOTER);
}

void fprint_obj_idx(FILE* out, bool uv, bool normal, u16 idx) {
    fprintf(out, "%hu", idx);
    if (uv) {
        fprintf(out, "/%hu", idx);
    }
    if (normal) {
        fprintf(out, "/%hu", idx);
    }
    fprintf(out, " ");
}

void dump_materials_obj(FILE* f, const chunk_0x1_entry* materials, u32 num_mats, const decoded_text* texture_names, u32 num_names) {
    for (u32 i = 0; i < num_mats; i++) {
        const chunk_0x1_entry* mat = &materials[i];
        // This swaps around in 1 specific vertex format that uses a baked light map
        const u32 normal_idx = (mat->vertbuf_format == 0x1F) ? mat->normal_backup_idx : mat->normal_idx;
        if (mat->texture_idx >= num_names) {
            LOG_MSG(warning, "Got out-of-bounds texture ID %d, skipping material %d.\n", mat->texture_idx, i);
            continue;
        }

        const char* diffuse_name = texture_names[mat->texture_idx].data;
        fprintf(f, "newmtl mat_%d\n", i);
        // Specify diffuse (base color)
        fprintf(f, "map_Kd textures/%s.dds\n", diffuse_name);
        // Also use this texture for alpha
        fprintf(f, "map_d textures/%s.dds\n", diffuse_name);

        // Specify normal map if needed
        if (normal_idx != 0 && normal_idx < num_names) {
            const char* normal_name = texture_names[normal_idx].data;
            fprintf(f, "map_Bump -bm 1.0 textures/%s.dds\n", normal_name);
        }
        fprintf(f, "\n");
    }
}

void dump_idx_buf(const u8* alr_data, u32 offset, FILE* out, bool has_uvs) {
    vfile vf = vfile_open((void*)(alr_data + offset), sizeof(idxbuf_header) + 0x8);

    // We use the temporary size until we can get the actual size here
    const idxbuf_header header = VFILE_READ(idxbuf_header, &vf);
    vf.size = header.size; // This just sets the limit of how much we can read
    fprintf(out, "usemtl mat_%d\n", header.texture_idx);

    const u16* indices = (u16*)vfile_cur(vf);
    for (s32 i = 2; i < header.num_indices; i++) {
        // We add 1 because OBJ indices start at 1
        u16 idx1 = indices[i - 2] + 1;
        u16 idx2 = indices[i - 1] + 1;
        u16 idx3 = indices[i] + 1;

        if (idx1 == idx2 || idx1 == idx3 || idx2 == idx3) {
            // Triangle strips will repeat 1 index to create a triangle with an
            // area of 0, which is used to end a strip and start another.
            // We skip these since they're not part of the geometry.
            continue;
        }

        fprintf(out, "f ");
        fprint_obj_idx(out, has_uvs, false, idx1);
        fprint_obj_idx(out, has_uvs, false, idx2);
        fprint_obj_idx(out, has_uvs, false, idx3);
        fprintf(out, "\n");

        if (header.primitive_type != IDX_TYPE_STRIP) {
            // For triangle strips we advance 1 each loop, but for normal
            // triangles we need to make up the difference to advance 3 each loop.
            i += 2;
        }
    }
}

void dump_vertex_buf(const file& alr, const char* path, u32 vertchunk_offset, u32 vert_entry_idx) {
    // Dump to OBJ
    FILE *out = fopen(path, "wb");
    if (out != nullptr) {
        // Open resource buffer
        vfile vf = vfile_open(alr.data, alr.alr_size);
        vfile_seek(&vf, vertchunk_offset);
        const auto genheader = VFILE_READ(chunk_generic, &vf);
        const u32 vert_entries = VFILE_READ(u32, &vf);
        const vertbuf_entry* entries = (vertbuf_entry*)vfile_cur(vf);
        const vertbuf_entry entry = entries[vert_entry_idx];

        // Jump to the appropriate data
        vf.pos = alr.resbuf_offset;
        vfile_seek(&vf, entry.data_ptr);
        bool has_uvs = false;
        for (u32 i = 0; i < entry.vertex_count; i++) {
            const s64 next_pos = vf.pos + entry.vertex_size;
            // Read the vertex (this abstracts away the many different formats)
            const std_vertex vert = standardize_pd_vertex(vfile_cur(vf), entry.format);

            // Save whatever vertex data we got
            if (vert.pos.has_value()) {
                const vec3s pos = vert.pos.value();
                fprintf(out, "v %f %f %f\n", pos.x, pos.y, pos.z);
            }

            if (vert.texcoord.has_value()) {
                has_uvs = true;
                const vec2s uv = vert.texcoord.value();
                fprintf(out, "vt %f %f\n", uv.x, uv.y);
            }

            if (vert.normal.has_value()) {
                const vec3s normal = vert.normal.value();
                fprintf(out, "vn %f %f %f\n", normal.x, normal.y, normal.z);
            }

            // Skip to the next vertex
            vf.pos = next_pos;
        }

        // Vertices are dumped, now for indices
        for (alr::file::chunk idx_chunk : alr.chunks) {
            if (idx_chunk.id == 0x16 && idx_chunk.offset > vertchunk_offset) {
                // We've hit a mesh metadata chunk past our own, so any
                // further index buffers will be garbage data to us. Quit.
                break;
            }

            if (idx_chunk.id != 0x2) {
                // We only want index buffer chunks
                continue;
            }

            if (idx_chunk.offset < vertchunk_offset) {
                // This index buffer is from a previous mesh, so it's
                // garbage data to us. Skip.
                continue;
            }

            // Skip to idx_chunk and skip header
            vf.pos = idx_chunk.offset;
            const idxbuf_header header = VFILE_READ(idxbuf_header, &vf);

            // We only want index buffers meant for this vertex buffer
            if (header.vertex_buf != vert_entry_idx) {
                continue;
            }

            fprintf(out, "\ng idxbuf_0x%lx\n", idx_chunk.offset);
            alr::dump_idx_buf(alr.data, idx_chunk.offset, out, has_uvs);
        }

        // Cleanup
        fclose(out);
    }
}

bool dump_all_textures(const file& alr) {
    file::chunk texture_chunk = alr.first_chunk_by_id(0x15);
    file::chunk atlas_chunk = alr.first_chunk_by_id(0x10);
    if (texture_chunk.size == 0 && atlas_chunk.size == 0) {
        LOG_MSG(warning, "I couldn't find any textures to dump.\n");
        return false;
    }

    system("mkdir textures"); // We need this folder for later

    // Try to find texture and texture atlas metadata, we need both to make a
    // good guess about dimensions.
    u32 textures_dumped = 0;

    // Read texture chunk data
    vfile vf = vfile_open(alr.data + texture_chunk.offset, texture_chunk.size);
    // Skip over the ID and size fields we already have
    vfile_seek(&vf, sizeof(chunk_generic));
    const u32 num_entries = VFILE_READ(u32, &vf);
    const texture_entry* tex_entries = (texture_entry*)vfile_cur(vf);

    // Read atlas chunk data
    const atlas_entry* atlas_entries = nullptr;
    const atlas_name* atlas_names = nullptr;
    atlas_header header_atlas = {0};
    if (atlas_chunk.size > 0) {
        vf = vfile_open(alr.data + atlas_chunk.offset, atlas_chunk.size);

        // Skip over the ID and size fields we already have
        vfile_seek(&vf, sizeof(chunk_generic));
        header_atlas = VFILE_READ(atlas_header, &vf);

        // Skip over names
        atlas_names = (atlas_name*)vfile_cur(vf);
        vfile_seek(&vf, sizeof(atlas_name) * header_atlas.atlas_count);

        atlas_entries = (atlas_entry*)vfile_cur(vf);
    }

    for (u32 i = 0; i < num_entries; i++) {
        // Convert the ALR texture data to our standard texture struct
        texture cur_tex = convert_tex(alr.resource_buffer(), tex_entries[i]);

        // Decode the texture filename
        char decoded_name[0x20] = {0};
        decode_single32(decoded_name, tex_entries[i].text1);
        decode_single32(&decoded_name[ENCODED_CHAR_COUNT], tex_entries[i].text2);
        strncat(decoded_name, ".dds", sizeof(decoded_name) - 1);
        char* name = decoded_name;

        if (atlas_entries != nullptr && header_atlas.atlas_count > i) {
            const atlas_entry entry = atlas_entries[i];
            // We get better dimension info from the atlas headers, so use it!
            // Dimensions from the atlas headers are almost always more
            // accurate, so we always use them unless they're obviously wrong.

            const u32 too_small = 0;
            const u32 too_big = 8192;
            if (entry.width > too_small && entry.width < too_big) {
                cur_tex.width = entry.width;
            }
            if (entry.height > too_small && entry.height < too_big) {
                cur_tex.height = entry.height;
            }

            // Also use the name from the atlas for the filename, because it'll
            // have correct capitalization
            name = (char*)atlas_names[i].name;
        }

        char path[0x30] = {0};
        snprintf(path, sizeof(path) - 1, "textures/%s", name);

        // Save the texture
        img_write(cur_tex, path);
        LOG_MSG(info, "Dumped %s\n", name);
        textures_dumped++;
    }

    if (textures_dumped == 0) {
        // This isn't a *failure*, but might be confusing if we don't say
        // anything and someone expects a texture file to appear.
        LOG_MSG(warning, "I couldn't find any textures to dump.\n");
    }
    return true;
}

bool dump_all_materials(const file& alr, const char* output_path) {
    file::chunk texture_chunk = alr.first_chunk_by_id(0x15);
    file::chunk material_chunk = alr.first_chunk_by_id(0x1);
    if (texture_chunk.size == 0 && material_chunk.size == 0) {
        LOG_MSG(warning, "I couldn't find any materials to dump.\n");
        return false;
    }

    FILE* f = fopen(output_path, "wb");
    if (!f) {
        LOG_MSG(error, "Failed to open output file '%s'\n", output_path);
        return false;
    }

    // Read texture chunk data
    vfile vf = vfile_open(alr.data, alr.alr_size);
    vf.pos = texture_chunk.offset;
    // Skip over the ID and size fields we already have
    vfile_seek(&vf, sizeof(chunk_generic));
    const u32 num_entries = VFILE_READ(u32, &vf);
    const auto* tex_entries = (texture_entry*)vfile_cur(vf);

    std::vector<decoded_text> texture_names;
    for (u32 i = 0; i < num_entries; i++) {
        // Decode the texture filename
        const texture_entry& tex = tex_entries[i];
        texture_names.emplace_back(decode_double(tex.text1, tex.text2));
    }

    vf.pos = material_chunk.offset;
    const auto material_header = VFILE_READ(chunk_0x1_header, &vf);
    const auto* materials = (const chunk_0x1_entry*)vfile_cur(vf);

    dump_materials_obj(f, materials, material_header.num_entries, texture_names.data(), texture_names.size());
    fclose(f);
    return true;
}


const char *anim_boilerplate = R"(animVersion 1.1;
mayaVersion %s; # This is actually the Polaris version
timeUnit film; # Frames
linearUnit m;
angularUnit rad;
startTime 0;
endTime %d;
)";

void fprint_anim_boilerplate(FILE* f, const char* polaris_version, float anim_length) {
    fprintf(f, anim_boilerplate, polaris_version, (u32)anim_length);
}

void fprintf_anim_key(FILE* f, float frame, float val, u32 tan_locked, u32 weight_locked, u32 breakdown) {
    fprintf(f, "    %f %f auto auto %d %d %d;", frame, val, tan_locked, weight_locked, breakdown);
}

enum anim_key_type : u8 {
    KEY_TRANSLATE,
    KEY_ROTATE,
    KEY_SCALE,
    KEY_TYPE_ENUM_MAX,
};

void fprintf_anim_data_start(FILE* f, anim_key_type type, char axis, const char* joint_name) {
    type = MIN(type, KEY_SCALE); // Keep in bounds
    const char* anim_types[KEY_TYPE_ENUM_MAX] = {
        "translate",
        "rotate",
        "scale",
    };
    const char* unit_types[KEY_TYPE_ENUM_MAX] = {
        "linear",
        "angular",
        "unitless",
    };

    const char* type_str = anim_types[type];
    const char* units = unit_types[type];
    s32 attr_idx = (type * 3) - 1;
    switch (axis) {
    case 'Z':
        attr_idx++;
        [[fallthrough]];
    case 'Y':
        attr_idx++;
        [[fallthrough]];
    case 'X':
        attr_idx++;
    default:
        break;
    }

    fprintf(f, "anim %s.%s%c %s%c ", type_str, type_str, axis, type_str, axis);
    fprintf(f, "%s 0 1 %d;\n", joint_name, attr_idx);
    fprintf(f, R"(animData {
  input time;
  output %s;
  weighted 1;
  preInfinity constant;
  postInfinity constant;
  keys {)", units);
    fprintf(f, "\n");
}

void fprintf_anim_data_end(FILE* f) {
    fprintf(f, "\n  }\n}\n\n");
}

void dump_anim_channel(u32 key_size, u32 num_keys, const void* keydata, FILE* f, anim_key_type type, const char* bone_name) {
    u32 num_components = 0;
    data_type frame_type = DATA_TYPE_COUNT;
    data_type component_type = DATA_TYPE_COUNT;
    anim_key_info(key_size, frame_type, component_type, num_components);
    const u32 frame_size = sizeof_type(frame_type);
    const u32 component_size = sizeof_type(component_type);
    const char* axes = "ZYX";

    vfile vf = vfile_open((void*)keydata, num_keys * key_size);
    for (u32 i = 0; i < num_components; i++) {
        if (num_keys == 0) {
            break; // Don't print empty key blocks, they break the parser
        }

        fprintf_anim_data_start(f, type, axes[i], bone_name);
        for (u32 j = 0; j < num_keys; j++) {
            const u32 next_key_pos = vf.pos + key_size;
            float frame = 0.0f;
            switch (frame_type) {
                case DATA_TYPE_FLOAT:
                    frame = VFILE_READ(float, &vf);
                    break;
                case DATA_TYPE_U8:
                    frame = VFILE_READ(u8, &vf);
                    break;
                default:
                    LOG_MSG(warning, "Unknown key format with size %d!\n", key_size);
                    break;
            }

            // Skip to component we want
            float component = 0.0f;
            vfile_seek(&vf, component_size * i);
            switch (component_type) {
                case DATA_TYPE_FLOAT:
                    component = VFILE_READ(float, &vf);
                    break;
                case DATA_TYPE_U16:
                    component = VFILE_READ(s16, &vf);
                    // Map into [0, 1] range
                    component /= float(INT16_MAX);

                    // Convert to radians
                    component *= 2.0f * M_PI;
                    break;
                default:
                    LOG_MSG(warning, "Unknown key format with size %d!\n", key_size);
                    break;
            }

            fprintf_anim_key(f, frame, component, 1, 0, 0);
            if (j < (num_keys - 1)) {
                // This is load-bearing. If we have a trailing newline in our
                // key {} block, the Blender plugin will crash.
                fprintf(f, "\n");
            }
            vf.pos = next_key_pos;
        }

        fprintf_anim_data_end(f);
        vf.pos = 0;
    }
}

bool dump_animation_maya(const anim_header* anim_chunk, const char* outpath, const char* bone_name) {
    const bool exists = file_exists(outpath);
    FILE* f = fopen(outpath, "ab");
    if (!f) {
        return false;
    }

    if (!exists) {
        fprint_anim_boilerplate(f, POLARIS_VERSION, anim_chunk->length);
    }

    vfile vf = vfile_open((void*)anim_chunk, anim_chunk->size);
    vfile_seek(&vf, sizeof(*anim_chunk));

    dump_anim_channel(anim_chunk->translation_key_size, anim_chunk->translation_key_count, vfile_cur(vf), f, KEY_TRANSLATE, bone_name);
    vfile_seek(&vf, anim_chunk->translation_key_size * anim_chunk->translation_key_count);

    dump_anim_channel(anim_chunk->rotation_key_size, anim_chunk->rotation_key_count, vfile_cur(vf), f, KEY_ROTATE, bone_name);
    vfile_seek(&vf, anim_chunk->rotation_key_size * anim_chunk->rotation_key_count);

    fclose(f);
    return true;
}

void obj_get_info(const char* txt, u32& out_vert_count, u32& out_idx_count, bool& out_has_uvs) {
    assert(txt != nullptr);
    u16 vert_count = 0;
    u16 idx_count = 0;
    bool has_uvs = false;

    const char* line = txt;
    while (*line != 0x00) {
        // Find end of line (NUL or newline)
        const char* line_end = strchr(line, '\n');
        if (!line_end) {
            break;
        }

        if (line[0] != '#') {
            // Faces
            if (strncmp(line, "f ", 2) == 0) {
                // Each face requires 3 indices
                idx_count += 3;
            }
            // Vertex position
            if (strncmp(line, "v ", 2) == 0) {
                vert_count++;
            }
            // vt == vertex texture coordinate
            if (strncmp(line, "vt ", 3) == 0) {
                has_uvs = true;
            }
        }

        // Advance to next line
        line = line_end + 1;
    }

    out_has_uvs = has_uvs;
    out_vert_count = vert_count;
    out_idx_count = idx_count;
}

parsed_obj obj_load(const char* text) {
    parsed_obj out;
    obj_get_info(text, out.vert_count, out.idx_count, out.has_uvs);

    u16* indices = (u16*)calloc(1, out.idx_count * sizeof(*indices));
    vec3s* positions = (vec3s*)calloc(1, out.vert_count * sizeof(*positions));
    vec2s* texcoords = (vec2s*)calloc(1, out.vert_count * sizeof(*texcoords));
    if (!positions || !indices) {
        free(indices);
        free(positions);
        free(texcoords);
        return out;
    }

    u32 idx_i = 0; // Pos in index buffer
    u32 pos_i = 0; // Pos in positions array
    u32 uv_i = 0;  // Pos in UV array
    const char* line = text;
    while (*line != 0x00) {
        // Find end of line (NUL or newline)
        const char* line_end = strchr(line, '\n');
        if (!line_end) {
            break;
        }

        if (line[0] != '#') {
            // Faces
            if (strncmp(line, "f ", 2) == 0) {

                u16 slash_count = 0;
                for (const char* temp = line; temp < line_end; temp++) {
                    slash_count += (*temp == '/');
                }

                u16 idx_temp[3] = {};
                if (slash_count > 0 && slash_count <= 3) {
                    u16 temp[3] = {};
                    sscanf(line, "f %hd/%hd %hd/%hd %hd/%hd",
                        &idx_temp[0], &temp[0], &idx_temp[1], &temp[1], &idx_temp[2], &temp[2]);
                }

                sscanf(line, "f %hd %hd %hd", &idx_temp[0], &idx_temp[1], &idx_temp[2]);

                for (u16 idx : idx_temp) {
                    // OBJ indices start @ 1
                    indices[idx_i++] = idx - 1;
                }
            }

            // Vertex position
            if (strncmp(line, "v ", 2) == 0) {
                vec3s pos = {};
                sscanf(line, "v %f %f %f", &pos.x, &pos.y, &pos.z);
                positions[pos_i] = pos;
            }

            // vt == vertex texture coordinate
            if (strncmp(line, "vt ", 3) == 0) {
                vec2s uv = {};
                sscanf(line, "vt %f %f", &uv.x, &uv.y);
                texcoords[uv_i] = uv;
            }
        }

        // Advance to next line
        line = line_end + 1;
    }

    out.indices = indices;
    out.positions = positions;
    out.texcoords = texcoords;

    return out;
}

bool obj_import(const char* txt, alr::file& alr, u32 vertbuf_chunk_offset, u32 entry_idx) {
    bool has_uvs = false;
    u32 vert_count = 0;
    u32 idx_count = 0;
    obj_get_info(txt, vert_count, idx_count, has_uvs);

    vfile vf = vfile_open(alr.data, alr.reserve_size);

    // Parse vertex buffer chunk
    vfile_seek(&vf, vertbuf_chunk_offset);
    const chunk_generic genheader = VFILE_READ(chunk_generic, &vf);
    const u32 num_entries = VFILE_READ(u32, &vf);
    auto* entries = (vertbuf_entry*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*entries) * num_entries);

    // Parse first index buffer chunk
    const u32 idxbuf_offset = vf.pos;
    auto* idx_header = (idxbuf_header*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*idx_header)); // Skip header
    u16* indices = (u16*)vfile_cur(vf);

    vf.pos = idxbuf_offset + idx_header->size; // Skip to next chunk
    if (idx_count > idx_header->num_indices) {
        // Not enough space, need to push the next index buffer forwards
        const u32 diff = idx_count - idx_header->num_indices;
        alr.shift_chunks(vf.pos, diff * sizeof(u16));
    }

    vertbuf_entry* entry = &entries[entry_idx];
    if (vert_count > entry->vertex_count) {
        // Make space for the extra data
        const u32 diff = vert_count - entry->vertex_count;
        if (!alr.shift_vertbuf(entry->data_ptr, diff * entry->vertex_size)) {
            return false;
        }
    }

    vertex_format_t vert_format = format_by_id(entry->format);

    u8* vertices = alr.resource_buffer() + entry->data_ptr;
    const u8* vertices_end = vertices + ((entry->vertex_count + 1) * entry->vertex_size);
    const char* line = txt;
    while (*line != 0x00) {
        // Find end of line (NUL or newline)
        const char* line_end = strchr(line, '\n');
        if (!line_end) {
            break;
        }

        if (line[0] != '#') {
            // Faces
            if (strncmp(line, "f ", 2) == 0) {
                u16 idx_temp[3] = {};
                sscanf(line, "f %hd %hd %hd", &idx_temp[0], &idx_temp[1], &idx_temp[2]);

                for (u16 idx : idx_temp) {
                    // OBJ indices start @ 1
                    *indices = idx - 1;
                    indices++;
                }
            }

            // Vertex position
            if (strncmp(line, "v ", 2) == 0) {
                vec3s pos = {};
                sscanf(line, "v %f %f %f", &pos.x, &pos.y, &pos.z);
                memcpy(vertices, &pos.raw, sizeof(pos.raw));
                vertices += entry->vertex_size;
            }

            // vt == vertex texture coordinate
            if (strncmp(line, "vt ", 3) == 0) {
            }
        }

        // Advance to next line
        line = line_end + 1;
    }

    idx_header->num_indices = idx_count;
    idx_header->vertex_buf = entry_idx;
    while (vertices < vertices_end) {
        // Wipe vertex position data
        memset(vertices, 0, sizeof(vec3s));
        vertices += entry->vertex_size;
    }

    return true;
}

} // namespace al
