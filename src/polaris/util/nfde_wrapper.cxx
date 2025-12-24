#include <vector>
#include <string>
#include <nfd.h>

#include <common/int.h>

nfdresult_t NFD_OpenDialogMultipleAutoFree(std::vector<std::string>& output, const nfdu8filteritem_t* filterList,
                                     nfdfiltersize_t filterCount, const nfdu8char_t* defaultPath) {
    const nfdpathset_t* pathset;
    auto result = NFD_OpenDialogMultipleU8(&pathset, filterList, filterCount, defaultPath);
    if (result != NFD_OKAY) {
        return result;
    }

    nfdpathsetsize_t numPaths = 0;
    if (NFD_PathSet_GetCount(pathset, &numPaths) != NFD_OKAY) {
        NFD_PathSet_Free(pathset);
        return NFD_ERROR;
    }

    output.reserve(numPaths);
    for (u32 i = 0; i < numPaths; i++) {
        nfdu8char_t* path = nullptr;
        if (NFD_PathSet_GetPathU8(pathset, i, &path) != NFD_OKAY) {
            continue; // Invalid path or something
        }

        output.emplace_back((char*)path);
        NFD_PathSet_FreePathU8(path);
    }

    // Free NFD's stuff
    NFD_PathSet_Free(pathset);
    return NFD_OKAY;
}
