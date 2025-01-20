#pragma once
#include <optional>

#include <cglm/struct.h>
#include <common/int.h>

// Standardized vertex format that can express all known Phantom Dust vertex
// formats. Will change often as new information is found.
struct std_vertex {
    // 3D position of the vertex. Should always be present.
    std::optional<vec3s> pos;

    // 2D texture coordinates. Should often be present.
    std::optional<vec2s> texcoord;

    // This is incomplete, more will be added here as research progresses
};

/// @brief Convert a single vertex to the standard format.
///
/// @param vertbuf Buffer containing PD vertex data (must be at least [vert_size] bytes)
/// @param vert_size The expected size of the vertex
/// @return A vertex in standard format
std_vertex standardize_pd_vertex(void* vertbuf, u8 vert_size);