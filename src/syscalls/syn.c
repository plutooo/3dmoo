#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "arm11.h"
#include "handles.h"
#include "mem.h"
#include "svc.h"


u32 mutex_SyncRequest(handleinfo* h)
{
    // XXX: insert real mutex here!
    mutex_WaitSynchronization(h);

    DEBUG("locking mutex..\n");
    PAUSE();

    h->locked = true;
    return 0;
}
u32 mutex_WaitSynchronization(handleinfo* h)
{
    DEBUG("waiting for mutex to unlock..\n");
    PAUSE();

    while(h->locked);
    return 0;
}

u32 svcCreateMutex()
{
    u32 locked = arm11_R(0);
    u32 handle = handle_New(HANDLE_TYPE_MUTEX, 0);

    handleinfo* h = handle_Get(handle);
    if(h == NULL) {
        DEBUG("failed to get newly created mutex\n");
        PAUSE();
        return -1;
    }

    h->locked = !!locked;
    arm11_SetR(1, handle); // handle_out
    return 0;
}

u32 svcReleaseMutex()
{
    u32 handle = arm11_R(0);
    handleinfo* h = handle_Get(handle);

    if(h == NULL) {
        ERROR("svcReleaseMutex on an invalid handle\n");
        PAUSE();
        return -1;
    }

    if (h->type != HANDLE_TYPE_MUTEX) {
        ERROR("svcReleaseMutex on a handle that is not a MUTEX\n");
        PAUSE();
        return -1;
    }

    h->locked = false;
    return 0;
}
