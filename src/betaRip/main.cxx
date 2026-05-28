#include <string>

#include <common/int.h>
#include <common/path.h>
#include <common/file.h>
#include <common/logging.h>
#include "polaris/alr/alr_file.hxx"
#include "formats/ssb.h"
#include <formats/alm.h>

void ripIndexbufALM(const idxbuf_header* idxHeader, const material_header* matHeader, u32 idxOffset, FILE* out) {
    const u32 matIdx = idxHeader->texture_idx;
    const material_entry* mat = &matHeader->entries[matIdx];
    decoded_text dec = decode_double(mat->text1, mat->text2);
    const std::string matName = std::string(dec.data) + std::to_string(matIdx);
    fprintf(out, "usemtl %s\n", matName.c_str());

    if (idxHeader->primitive_type != IDX_TYPE_NORMAL) {
        LOG_MSG(debug, "Found alternate indexing mode (%d) on vertbuf %d\n", idxHeader->primitive_type, idxHeader->vertex_buf);
    }
    const u32 step = (idxHeader->primitive_type == IDX_TYPE_STRIP) ? 1 : 3;
    for (u32 i = 0; i < idxHeader->num_indices - 2; i += step) {
        const u16 idx0 = idxHeader->indices[i + 0] + 1 + idxOffset;
        const u16 idx1 = idxHeader->indices[i + 1] + 1 + idxOffset;
        const u16 idx2 = idxHeader->indices[i + 2] + 1 + idxOffset;

        // fprintf(out, "f %hu %hu %hu\n", idx0, idx1, idx2);
        fprintf(out, "f %hu/%hu %hu/%hu %hu/%hu\n", idx0, idx0, idx1, idx1, idx2, idx2);
    }
}

void ripVertexbufALM(const alm_vertbuf* vertHeader, FILE* obj) {
    const u32 vertbufSize = vertHeader->vert_size * vertHeader->vert_count;
    vfile vf = vfile_open((void*)vertHeader->vertices, vertbufSize);
    for (u32 i = 0; i < vertHeader->vert_count; i++) {
        const vec3f* pos = (vec3f*)vfile_cur(vf);
        const u16* uv = (u16*)((uintptr_t)&pos[1] + 4);
        const u16 maxU = INT16_MAX;
        const u16 maxV = maxU;
        fprintf(obj, "v %f %f %f\n", pos->x, pos->y, pos->z);
        fprintf(obj, "vt %f %f\n", (float)uv[0] / maxU, (float)uv[1] / maxV);
        vfile_seek(&vf, vertHeader->vert_size);
    }
}

bool ripALM(const char* path) {
    assert(path_has_extension(path, "alm"));
    std::string basename = path;
    basename.replace(basename.find(".alm"), 0, "");
    const std::string objPath = basename + ".obj";
    const std::string mtlPath = basename + ".mtl";

    alr::file alr;
    alr.load(path);
    vfile texVF = alr.vf_from_chunk(alr.first_chunk_by_id(4));
    vfile matVF = alr.vf_from_chunk(alr.first_chunk_by_id(ALR_ID_MATERIAL));
    vfile vertVF = alr.vf_from_chunk(alr.first_chunk_by_id(6));
    vfile idxVF = alr.vf_from_chunk(alr.first_chunk_by_id(ALR_ID_INDICES));

    const alm_texture_header* texHeader = (alm_texture_header*)vfile_cur(texVF);
    const material_header* matHeader = (material_header*)vfile_cur(matVF);

    std::vector<alm_vertbuf*> vertbufs;
    std::vector<idxbuf_header*> idxbufs;

    for (const auto& c : alr.chunks) {
        vfile vf = alr.vf_from_chunk(c);
        const void* data = vfile_cur(vf);

        if (c.id == 6) {
            vertbufs.push_back((alm_vertbuf*)data);
        } else if (c.id == ALR_ID_INDICES) {
            idxbufs.push_back((idxbuf_header*)data);
        }
    }

    FILE* mtl = fopen(mtlPath.c_str(), "wb");
    FILE* obj = fopen(objPath.c_str(), "wb");
    if (!mtl || !obj) {
        LOG_MSG(error, "Failed to open OBJ or MTL output file\n");
        fclose(mtl);
        fclose(obj);
        return false;
    }

    // Save MTL file
    for (u32 i = 0; i < matHeader->num_entries; i++) {
        const material_entry* mat = &matHeader->entries[i];
        decoded_text dec = decode_double(mat->text1, mat->text2);
        const std::string matName = std::string(dec.data) + std::to_string(i);

        const alm_texture_entry* tex = &texHeader->entries[mat->texture_idx];
        decoded_text dec2 = decode_double(tex->text1, tex->text2);

        std::string texName = std::string(dec2.data) + ".dds";
        if (!file_exists(texName.c_str())) {
            for (char& c : texName) {
                c = std::toupper(c);
            }
        } else {
            LOG_MSG(debug, "'%s' exists, so it won't be capitalized\n", texName.c_str());
        }


        fprintf(mtl, "newmtl %s\n", matName.c_str());
        fprintf(mtl, "map_Kd %s\n", texName.c_str());
        fprintf(mtl, "map_d %s\n\n", texName.c_str());
    }
    fclose(mtl);

    // Save OBJ file
    fprintf(obj, "mtllib %s\n", mtlPath.c_str());

    u32 curIdx = 0;
    for (u32 i = 0; i < vertbufs.size(); i++) {
        fprintf(obj, "o vertbuf_%d\n", i);
        const alm_vertbuf* v = vertbufs[i];
        ripVertexbufALM(v, obj);
        for (const idxbuf_header* idxbuf : idxbufs) {
            if (idxbuf->vertex_buf == i) {
                ripIndexbufALM(idxbuf, matHeader, curIdx, obj);
            }
        }
        curIdx += v->vert_count;
        fprintf(obj, "\n\n");
    }

    fclose(obj);
    return true;
}

bool ripFile(const char* path) {
    if (path_has_extension(path, "alm")) {
        return ripALM(path);
    }

    return false;
}

int main(int argc, char** argv) {
    for (u32 i = 1; i < argc; i++) {
        if (ripFile(argv[i])) {
            LOG_MSG(info, "Ripped '%s'\n", argv[i]);
        } else {
            LOG_MSG(info, "Failed to rip '%s'\n", argv[i]);
        }
    }
}