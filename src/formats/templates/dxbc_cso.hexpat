struct Header {
    u8 Magic[4];
    u8 Digest[16];
    u16 MajorVersion;
    u16 MinorVersion;
    u32 FileSize;
    u32 PartCount;
};

struct ISGNElement {
    u32 NameOffset;
    u32 SemanticIndex;
    u32 SemanticType;
    u32 ComponentType;
    u32 Register;
    u8 Mask;
    u8 ReadWriteMask;
    u16 unused;
};

struct PartHeader {
    char Name[4];
    u32 Size;

    if (Name == "ISGN") {
        u32 ElementCount;
        u32 Unknown;
        ISGNElement elements[ElementCount];
    }
};

struct PartOffset {
    u32 offset;
    PartHeader part @ offset;
};

Header header @ 0x0;
PartOffset partOffsets[header.PartCount] @ $;