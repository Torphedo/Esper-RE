#include "pd_mesh.hxx"

#include <common/vfile.h>

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