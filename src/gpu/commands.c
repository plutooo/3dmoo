#include "util.h"
#include "gpu.h"

void gpu_ExecuteCommands(u8* buffer, u32 sizea)
{
    u32 i;
    for (i = 0; i < sizea; i += 8) {
        u32 cmd = *(u32*)(buffer + 4 + i);
        u32 dataone = *(u32*)(buffer + i);
        u16 ID = cmd & 0xFFFF;
        u8 mask = (cmd >> 16) & 0xF;
        u16 size = (cmd >> 20) & 0x7FF;
        u8 grouping = (cmd >> 31);
        u32 datafild[0x800]; //maximal size
        datafild[0] = dataone;
#ifdef GSP_ENABLE_LOG
		GPUDEBUG("cmd %04x mask %01x size %03x (%08x) %s \n", ID, mask, size, dataone, grouping ? "grouping" : "");
#endif
        int j;
        for (j = 0; j < size; j++) {
            datafild[1 + j] = *(u32*)(buffer + 8 + i);
#ifdef GSP_ENABLE_LOG
            GPUDEBUG("data %08x\n", datafild[1 + j]);
#endif
            i += 4;
        }
        if (size & 0x1) {
            u32 data = *(u32*)(buffer + 8 + i);
#ifdef GSP_ENABLE_LOG
            GPUDEBUG("padding data %08x\n", data);
#endif
            i += 4;
        }
        if (mask != 0) {
#ifdef GSP_ENABLE_LOG
            if (size > 0 && mask != 0xF)
                GPUDEBUG("masked data? cmd %04x mask %01x size %03x (%08x) %s \n", ID, mask, size, dataone, grouping ? "grouping" : "");
#endif
            if (grouping) {
                for (j = 0; j <= size; j++)writeGPUID(ID + j, mask, 1, &datafild[j]);
            } else {
                writeGPUID(ID, mask, size + 1, datafild);
            }
        } else {
#ifdef GSP_ENABLE_LOG
            GPUDEBUG("masked out data is ignored cmd %04x mask %01x size %03x (%08x) %s \n", ID, mask, size, dataone, grouping ? "grouping" : "");
#endif
        }

    }
}
