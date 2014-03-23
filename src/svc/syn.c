#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "arm11.h"
#include "handles.h"
#include "mem.h"
#include "svc.h"


bool syn_IsLocked(u32 handle)
{
    handleinfo* h = handle_Get(handle);
    if (h->locked) {
	return true;
    }
    else {
	if (h->locktype != LOCK_TYPE_STICKY) {
	    h->locked = true;
	}
    }
    return false;
}

u32 ReleaseMutex(u32 handle)
{
    handleinfo* h = handle_Get(handle);

    if (h->type != HANDLE_TYPE_MUTEX) {
	DEBUG("ERROR: ReleaseMutex on a handle that is not a MUTEX type");
    }

    if (h->locktype == LOCK_TYPE_PULSE) {
	DEBUG("ERROR LOCK_TYPE_PULSE not supported for MUTEX");
    }

    h->locked = false;
    return 1;
}
