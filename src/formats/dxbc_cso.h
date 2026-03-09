#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// Structures for DirectX bytecode (DXBC) files produced by Microsoft's HLSL
// compiler "fxc.exe". These often have the file extension ".cso" or ".o".
//
// http://timjones.io/blog/archive/2015/09/02/parsing-direct3d-shader-bytecode
// https://llvm.org/docs/DirectX/DXContainer.html
#include "data_types.h"

// https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_shader_cbuffer_flags
typedef enum {
    D3D_CBF_USERPACKED = 1,
    D3D10_CBF_USERPACKED,
    D3D_CBF_FORCE_DWORD = 0x7fffffff
} D3D_SHADER_CBUFFER_FLAGS;

// https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ff476097(v=vs.85)
typedef enum {
    D3D11_CT_CBUFFER,
    D3D11_CT_TBUFFER,
    D3D11_CT_INTERFACE_POINTERS,
    D3D11_CT_RESOURCE_BIND_INFO,
} D3D11_CBUFFER_TYPE;

typedef enum {
    D3D_SVC_SCALAR = 0,
    D3D_SVC_VECTOR,
    D3D_SVC_MATRIX_ROWS,
    D3D_SVC_MATRIX_COLUMNS,
    D3D_SVC_OBJECT,
    D3D_SVC_STRUCT,
    D3D_SVC_INTERFACE_CLASS,
    D3D_SVC_INTERFACE_POINTER,
    D3D10_SVC_SCALAR,
    D3D10_SVC_VECTOR,
    D3D10_SVC_MATRIX_ROWS,
    D3D10_SVC_MATRIX_COLUMNS,
    D3D10_SVC_OBJECT,
    D3D10_SVC_STRUCT,
    D3D11_SVC_INTERFACE_CLASS,
    D3D11_SVC_INTERFACE_POINTER,
    D3D_SVC_FORCE_DWORD = 0x7fffffff
} D3D_SHADER_VARIABLE_CLASS;

typedef enum {
    D3D_SVT_VOID = 0,
    D3D_SVT_BOOL = 1,
    D3D_SVT_INT = 2,
    D3D_SVT_FLOAT = 3,
    D3D_SVT_STRING = 4,
    D3D_SVT_TEXTURE = 5,
    D3D_SVT_TEXTURE1D = 6,
    D3D_SVT_TEXTURE2D = 7,
    /* snip... */
    D3D_SVT_UINT = 19,
    D3D_SVT_UINT8 = 20,
    /* snip... */
    D3D_SVT_BUFFER = 25,
    D3D_SVT_CBUFFER = 26,
    D3D_SVT_TBUFFER = 27,
    /* snip... */
    D3D_SVT_DOUBLE = 39,
    /* snip... */
    D3D_SVT_FORCE_DWORD = 0x7fffffff
} D3D_SHADER_VARIABLE_TYPE;

typedef struct {
    u8 Magic[4]; // "DXBC"
    u8 Digest[16];
    u16 MajorVersion; // 1
    u16 MinorVersion; // 0
    u32 FileSize;
    u32 PartCount;
    u32 PartOffsets[];
} CSOHeader;

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
} ISGNElement;

// ISGN == "Input Signature"
typedef struct {
    u32 ElementCount;
    u32 Unknown;
    ISGNElement elements[];
} ISGNPart;

// RDEF == "Resource Definition"
typedef struct {
    u32 ConstBufferCount;
    u32 ConstBufferOffset;
    u32 ResBindingCount;
    u32 ResBindingOffset;
    u8 VersionMinor;
    u8 VersionMajor;
    u16 ProgramType;
    u32 Flags; // 256 == NoPreshader
    u32 CreatorNameOffset;
} RDEFPart;

typedef struct {
    u32 NameOffset;
    u32 VariableCount;
    u32 VariablesOffset;
    u32 BufferSize;
    u32 Flags; // D3D_SHADER_CBUFFER_FLAGS
    u32 BufferType; // D3D11_CBUFFER_TYPE
} RDEFConstBuffer;

typedef struct {
    u32 NameOffset; // Offset from start of part
    u32 BufferOffset; // Offset of the variable within the buffer
    u32 Size; // Size of variable
    u32 Flags; // D3D10_SHADER_VARIABLE_FLAGS (2 == used in the shader)
    u32 TypeOffset; // Offset from start of part to variable type
    u32 DefaultValOffset; // 0 means no default value
} RDEFVariable;

typedef struct {
    u16 Class; // D3D10_SHADER_VARIABLE_CLASS (3 == column-major matrix)
    u16 Type; // D3D10_SHADER_VARIABLE_TYPE (3 == float)

    // For matrices
    u16 MatrixRows;
    u16 MatrixCols;

    // For arrays
    u16 ArrayElements;

    // For structs
    u16 StructMemberCount;
    u16 FirstMemberOffset; // From start of part
} RDEFVariableType;

typedef struct {
    u32 NameOffset; // Offset from start of part
    u32 InputType; // 0 == cbuffer
    u32 ResReturnType; // 0 == N/A
    u32 ResViewDimension; // 0 == N/A
    u32 SampleNum;
    u32 BindPoint;
    u32 BindCount;
    u32 InputFlags;
} RDEFResDesc;

typedef struct {
    char Name[4];
    u32 Size;
} CSOPart;

#ifdef __cplusplus
}
#endif