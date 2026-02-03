#pragma once
// Structures for DirectX bytecode (DXBC) files produced by the HLSL compiler
// fxc.exe. These often have the file extension ".cso" or ".o".
//
// http://timjones.io/blog/archive/2015/09/02/parsing-direct3d-shader-bytecode
// https://llvm.org/docs/DirectX/DXContainer.html
#include <common/int.h>

typedef struct {
    u8 Magic[4]; // "DXBC"
    u8 Digest[16];
    u16 MajorVersion; // 1
    u16 MinorVersion; // 0
    u32 FileSize;
    u32 PartCount;
    u32 PartOffsets[];
}CSOHeader;

// A single input (i.e, position or texcoord)
typedef struct {
    u32 NameOffset; // Offset from start of part
    u32 SemanticIndex;
    u32 SemanticType;
    u32 ComponentType;
    u32 Register;
    u8 Mask;
    u8 ReadWriteMask;
    u16 unused;
}ISGNElement;

// ISGN == "Input Signature"
typedef struct {
    u32 ElementCount;
    u32 Unknown;
    ISGNElement elements[];
}ISGNPart;

typedef struct {
    char Name[4];
    u32 Size;
}CSOPart;
