typedef struct {
    u8 dataoffset[0x4];
    u8 destoffset[4];
    u8 size[4];
    u8 select[4];
    u8 sha[0x20];

} DSPsegment;

typedef struct {
    u8 signature[0x100];
    u8 magic[4];
    u8 contentsize[4];
    u8 unk1[4];
    u8 unk2;
    u8 unk3; //num mem type 0
    u8 numsec;
    u8 unk5; //num mem type 2
    u8 unk6[4];
    u8 unk7[4];
    u8 zero[8];
    DSPsegment sement[0x9];
} DSPhead;
