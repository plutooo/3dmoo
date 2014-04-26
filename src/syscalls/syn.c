/*
* Copyright (C) 2014 - plutoo
* Copyright (C) 2014 - ichfly
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "arm11.h"
#include "handles.h"
#include "mem.h"
#include "svc.h"


u32 mutex_SyncRequest(handleinfo* h, bool *locked)
{
    // XXX: insert real mutex here!
    mutex_WaitSynchronization(h,locked);

    DEBUG("locking mutex..\n");
    PAUSE();

    h->locked = true;
    return 0;
}
u32 mutex_WaitSynchronization(handleinfo* h, bool *locked)
{
    DEBUG("waiting for mutex to unlock..\n");
    PAUSE();

    *locked = h->locked;

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
u32 svcDuplicateHandle()
{
    u32 todclone = arm11_R(1);
    u32 handle;
    if (todclone == HANDLE_CURRENT_THREAD)todclone = threads_getcurrenthandle();
    if (todclone == HANDLE_CURRENT_PROCESS)todclone = curprocesshandle;
    handle = handle_New(HANDLE_TYPE_REDIR, todclone);
    handleinfo* h = handle_Get(handle);
    if (h == NULL) {
        DEBUG("failed to get newly created copy\n");
        PAUSE();
        return -1;
    }
}