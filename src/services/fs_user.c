#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "arm11.h"
#include "handles.h"
#include "mem.h"
#include "SrvtoIO.h"


#define CPUsvcbuffer 0xFFFF0000


u32 fs_user_SyncRequest()
{
    u32 cid = mem_Read32(CPUsvcbuffer + 0x80);

    // Read command-id.
    switch (cid) {
    }

    ERROR("NOT IMPLEMENTED, cid=%08x\n", cid);
    arm11_Dump();
    PAUSE();
    return 0;
}