/*
 * Copyright (C) 2014 - plutoo
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

#define error_not_a_mutex 0x81234567 //correct me fixme

// svc.c
void svc_Execute(u8 num);

// svc/memory.c
u32 svcControlMemory();
u32 svcMapMemoryBlock();

// svc/ports.c
u32 svcConnectToPort();

// svc/events.c
u32 svcCreateEvent();

// svc/syn.c
u32 svcCreateMutex();
u32 svcReleaseMutex();

// Generics (handles.c)
u32 svcSendSyncRequest();
u32 svcCloseHandle();
u32 svcWaitSynchronization1();

