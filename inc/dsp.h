typedef struct {
    u8 data_offset[4];
    u8 dest_offset[4];
    u8 size[4];
    u8 select[4];
    u8 sha[0x20];
} dsp_segment;

typedef struct {
    u8 signature[0x100];
    u8 magic[4];
    u8 content_sz[4];
    u8 unk1[4];
    u8 unk2;
    u8 unk3; //num mem type 0
    u8 num_sec;
    u8 unk5; //num mem type 2
    u8 unk6[4];
    u8 unk7[4];
    u8 zero[8];
    dsp_segment segment[0x9];
} dsp_header;
