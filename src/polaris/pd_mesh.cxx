#include "pd_mesh.hxx"

#include <common/vfile.h>

u16 gl_type_size(u16 type) {
    switch (type) {
    case GL_FLOAT:
    case GL_UNSIGNED_INT:
    case GL_INT:
        return 4;
    case GL_UNSIGNED_SHORT:
    case GL_SHORT:
        return 2;
    case GL_UNSIGNED_BYTE:
    case GL_BYTE:
        return 1;
    default:
        return 0;
    }
}

u32 gl_type_max(u16 type) {
    switch (type) {
    case GL_UNSIGNED_INT:
        return UINT32_MAX;
    case GL_INT:
        return INT32_MAX;
    case GL_UNSIGNED_SHORT:
        return UINT16_MAX;
    case GL_SHORT:
        return INT16_MAX;
    case GL_UNSIGNED_BYTE:
        return UINT8_MAX;
    case GL_BYTE:
        return INT8_MAX;
    default:
        return 1;
    }
}

// Table of known vertex formats we can look up by size. If an attribute
// doesn't have an offset listed, that means it comes immediately after the
// last attribute. This can be overidden by setting an explicit offset.
mesh_view known_formats[] = {
    {
        .attributes = {
            { // Position
                .type = GL_FLOAT,
                .components = 3,
            },
            { // Texcoord
                .empty = true,
            },
        },
        .vertex_size = 12,
    },
    {
        .attributes = {
            { // Position
                .type = GL_FLOAT,
                .components = 3,
            },
            { // Texcoord
                .type = GL_SHORT,
                .components = 2,
            },
        },
        .vertex_size = 24,
        .use_type_divisor = true,
    },
    {
        .attributes = {
            { // Position
                .type = GL_FLOAT,
                .components = 3,
            },
            { // Texcoord
                .type = GL_SHORT,
                .offset = 16,
                .components = 2,
            },
        },
        .vertex_size = 32,
        .uv_divisor = 4096,
        .use_type_divisor = false,
    },
};

std::optional<mesh_view> find_format_by_size(u8 size) {
    for (u32 i = 0; i < ARRAY_SIZE(known_formats); i++) {
        if (known_formats[i].vertex_size == size) {
            // This format is a match!
            return known_formats[i];
        }
    }

    // Return blank optional
    const std::optional<mesh_view> result;
    return result;
}

bool has_uvs(u8 vert_size) {
    // Known formats with UVs
    return vert_size == 24 || vert_size == 32 || vert_size == 20;
}

// TODO: Make this also use the format table.
std_vertex standardize_pd_vertex(void* vertbuf, u8 vert_size) {
    std_vertex output = {};
    // Get a virtual file for the buffer
    vfile vf = vfile_open(vertbuf, vert_size);

    // The only thing consistent across formats is that they always start with
    // the 3D position.
    output.pos = VFILE_READ(vec3s, &vf);

    // Handle all known vertex formats
    switch (vert_size) {
    case 24:
        // Convert UVs from 16-bit to floating-point
        output.texcoord = {
            VFILE_READ(u16, &vf) / (float)INT16_MAX,
            VFILE_READ(u16, &vf) / (float)INT16_MAX,
        };
        break;
    case 32:
        // This format has UVs in a slightly different place
        vfile_seek(&vf, 4);
        // This format only uses values up to 4096 in UVs (why?)
        output.texcoord = {
            VFILE_READ(u16, &vf) / 4096.0f,
            VFILE_READ(u16, &vf) / 4096.0f,
        };
        break;
    case 12: // This format is only position
    default:
        break;
    }

    // Fix vertically flipped UVs to match what Blender expects
    if (output.texcoord.has_value()) {
        output.texcoord.value().y = reflect(output.texcoord.value().y, 0.5f);
    }

    return output;
}

void get_vert_attribute(mesh_view* out, vertbuf_entry vert_header) {

    // Search our table of known formats
    std::optional<mesh_view> format = find_format_by_size(vert_header.vertex_size);

    if (!format.has_value()) {
        // Couldn't find a matching format... try our best guess.
        format = known_formats[0];
    }

    // Copy format data to the output
    out->vertex_size = vert_header.vertex_size;
    out->use_type_divisor = format->use_type_divisor;
    out->uv_divisor = format->uv_divisor;
    for (u32 i = 0; i < ARRAY_SIZE(out->attributes); i++) {
        out->attributes[i] = format->attributes[i];
        if (i > 0 && out->attributes[i].offset == 0) {
            // Guess the correct offset based on the last one
            const vertex_attribute prev_attr = format->attributes[i - 1];
            vertex_attribute& cur_attr = out->attributes[i];
            cur_attr.offset = prev_attr.offset + (prev_attr.components * gl_type_size(prev_attr.type));
        }
    }
}
