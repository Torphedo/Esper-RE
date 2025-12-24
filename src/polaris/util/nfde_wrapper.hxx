#pragma once
#include <vector>
#include <string>

#include <nfd.h>

// A wrapper around NFDE's multi-file dialog that outputs a straight string array
nfdresult_t NFD_OpenDialogMultipleAutoFree(std::vector<std::string>& output, const nfdu8filteritem_t* filterList,
                                     nfdfiltersize_t filterCount, const nfdu8char_t* defaultPath);
