#include "pd_mesh.hxx"
#include <cstring>

#include <common/vfile.h>

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

vec4s read_attr(vfile& vf, vertex_attribute attr) {
    vec4s result = {};
    if (!attr.exists) {
        return result;
    }

    vf.pos = attr.offset;
    for (u32 i = 0; i < attr.components; i++) {
        float val = 0.0f;
        switch (attr.type) {
            case GL_FLOAT:
                val = VFILE_READ(float, &vf);
                break;
            case GL_BYTE:
                val = VFILE_READ(s8, &vf);
                break;
            case GL_UNSIGNED_BYTE:
                val = VFILE_READ(u8, &vf);
                break;
            case GL_SHORT:
                val = VFILE_READ(s16, &vf);
                break;
            case GL_UNSIGNED_SHORT:
                val = VFILE_READ(u16, &vf);
                break;
        }

        result.raw[i] = val;
    }

    return result;
}

// TODO: Make this also use the format table.
std_vertex standardize_pd_vertex(void* vertbuf, u8 format_id) {
    vertex_format_t format = format_by_id(format_id);
    std_vertex output = {};
    // Get a virtual file for the buffer
    vfile vf = vfile_open(vertbuf, format.size);

    // The only thing consistent across formats is that they always start with
    // the 3D position.

    vertex_attribute pos_attr = format.attributes[ATTRIBUTE_POSITION];
    if (pos_attr.exists) {
        vec4s pos = read_attr(vf, pos_attr);
        output.pos = vec3s{pos.x, pos.y, pos.z};
    }

    vertex_attribute uv_attr = format.attributes[ATTRIBUTE_TEXCOORD];
    if (uv_attr.exists) {
        vec4s uv = read_attr(vf, uv_attr);
        output.texcoord = vec2s{uv.x, uv.y};
    }

    vertex_attribute normal_attr = format.attributes[ATTRIBUTE_TEXCOORD];
    if (normal_attr.exists) {
        vec4s normal = read_attr(vf, normal_attr);
        output.normal = vec3s{normal.x, normal.y, normal.z};
    }

    // Fix vertically flipped UVs to match what Blender expects
    if (output.texcoord.has_value()) {
        output.texcoord.value().y = reflect(output.texcoord.value().y, 0.5f);
    }

    return output;
}

void get_vert_attribute(mesh_view* out, vertbuf_entry vert_header) {
    // Search our table of known formats
    vertex_format_t format = format_by_id(vert_header.format);

    // Copy format data to the output
    out->vertex_size = format.size;
    // TODO: Make divisors per-attribute instead of UV-only
    out->uv_divisor = format.attributes[ATTRIBUTE_TEXCOORD].divisor;
    memcpy(out->attributes, format.attributes, sizeof(format.attributes));
}
