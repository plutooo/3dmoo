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

#include "util.h"
#include "arm11.h"
#include "mem.h"
#include "handles.h"

u32 svcClearEvent()
{
    u32 handleorigin = arm11_R(0);
    handleinfo* h = handle_Get(handleorigin);
    if (h == NULL) {
        DEBUG("failed to get Event\n");
        PAUSE();
        return -1;
    }
    h->locked = true;
    return 0;
}

u32 svcCreateEvent()
{
    u32 handleorigin = arm11_R(0);
    u32 type = arm11_R(1);
    u32 handle = handle_New(HANDLE_TYPE_EVENT, 0);

    handleinfo* h = handle_Get(handle);
    if (h == NULL) {
        DEBUG("failed to get newly created Event\n");
        PAUSE();
        return -1;
    }
    if (type > LOCK_TYPE_MAX) {
        DEBUG("unknown event type\n");
        PAUSE();
        return -1;
    }

    h->locked = true;
    h->locktype = type;
    arm11_SetR(1, handle); // handle_out

    DEBUG("handleoriginal=%x, resettype=%x.\n", handleorigin, type);
    PAUSE();
    return 0;
}
u32 Event_WaitSynchronization(handleinfo* h, bool *locked)
{
    *locked = h->locked;
    if (h->locktype != LOCK_TYPE_STICKY) h->locked = true;
    DEBUG("waiting for event to happen..\n");
    PAUSE();
    return 0;

    /*
    while (h->locked);
    if (h->subtype != LOCK_TYPE_STICKY)h->locked = true;
    return 0;
    */
}
