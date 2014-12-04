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

#ifndef _SVC_H_
#define _SVC_H_

#include "../src/arm11/armdefs.h"

#define error_not_a_mutex 0x81234567 //correct me fixme

// svc.c
void svc_Execute(ARMul_State * state, u8 num);

// arm11/threads.c
u32 svcCreateThread();
u32 svcGetThreadId();

// syscalls/memory.c
u32 svcControlMemory();
u32 svcMapMemoryBlock();
u32 svcCreateMemoryBlock();

// syscalls/ports.c
u32 svcConnectToPort();

// syscalls/events.c
u32 svcCreateEvent();
u32 svcClearEvent();

// syscalls/syn.c
u32 svcCreateMutex();
u32 svcReleaseMutex();
u32 svcCreateSemaphore();
u32 svcReleaseSemaphore();

// syscalls/arb.c
u32 svcCreateAddressArbiter();
u32 svcArbitrateAddress();

// Generics (handles.c)
u32 svcSendSyncRequest();
u32 svcCloseHandle();
u32 svcWaitSynchronization1();
u32 svcWaitSynchronizationN();
u32 svcDuplicateHandle();

//syscalls/Interrupt.c
u32 svcBindInterrupt();

//mem
u32 svcmapMemoryBlock();

//screen.c
u32 svcsleep();

//srv.c
u32 svcReplyAndReceive();
u32 svcSignalEvent();
u32 svcCreatePort();
u32 svcAcceptSession();

//syscalls/timer.c
u32 svccreatetimer();

//syscalls/resource_limit.c
u32 svcGetResourceLimitCurrentValues();

#endif
