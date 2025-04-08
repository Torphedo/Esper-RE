#include "pd_mesh.hxx"

#include <common/vfile.h>

// Table of known vertex formats we can look up by size
mesh_view known_formats[] = {
    {
        .attributes = {
            [ATTRIBUTE_POSITION] = {
                .type = GL_FLOAT,
                .stride = 12,
                .components = 3,
            },
            [ATTRIBUTE_TEXCOORD] = {
                .empty = true,
            },
        },
    },
    {
        .attributes = {
            [ATTRIBUTE_POSITION] = {
                .type = GL_FLOAT,
                .stride = 24,
                .components = 3,
            },
            [ATTRIBUTE_TEXCOORD] = {
                .type = GL_SHORT,
                .stride = 24,
                .offset = 12,
                .components = 2,
            },
        },
        .use_type_divisor = true,
    },
    {
        .attributes = {
            [ATTRIBUTE_POSITION] = {
                .type = GL_FLOAT,
                .stride = 32,
                .components = 3,
            },
            [ATTRIBUTE_TEXCOORD] = {
                .type = GL_UNSIGNED_SHORT,
                .stride = 32,
                .offset = 16,
                .components = 2,
            },
        },
        .uv_divisor = 4096,
        .use_type_divisor = false,
    },
};

std::optional<mesh_view> find_format_by_size(u8 size) {
    for (u32 i = 0; i < ARRAY_SIZE(known_formats); i++) {
        if (known_formats[i].attributes[0].stride == size) {
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
        // Couldn't find a matching format...
        return;
    }

    // Copy format data to the output
    out->use_type_divisor = format.value().use_type_divisor;
    out->uv_divisor = format.value().uv_divisor;
    for (u32 i = 0; i < ARRAY_SIZE(out->attributes); i++) {
        out->attributes[i] = format.value().attributes[i];
    }
}
