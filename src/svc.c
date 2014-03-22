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

#include <stdio.h>
#include <stdlib.h>

#include "util.h"

static const char* names[256] = {
    NULL,
    "ControlMemory",
    "QueryMemory",
    "ExitProcess",
    "GetProcessAffinityMask",
    "SetProcessAffinityMask",
    "GetProcessIdealProcessor",
    "SetProcessIdealProcessor",
    "CreateThread",
    "ExitThread",
    "SleepThread",
    "GetThreadPriority",
    "SetThreadPriority",
    "GetThreadAffinityMask",
    "SetThreadAffinityMask",
    "GetThreadIdealProcessor",
    "SetThreadIdealProcessor",
    "GetCurrentProcessorNumber",
    "Run",
    "CreateMutex",
    "ReleaseMutex",
    "CreateSemaphore",
    "ReleaseSemaphore",
    "CreateEvent",
    "SignalEvent",
    "ClearEvent",
    "CreateTimer",
    "SetTimer",
    "CancelTimer",
    "ClearTimer",
    "CreateMemoryBlock",
    "MapMemoryBlock",
    "UnmapMemoryBlock",
    "CreateAddressArbiter",
    "ArbitrateAddress",
    "CloseHandle",
    "WaitSynchronization1",
    "WaitSynchronizationN",
    "SignalAndWait",
    "DuplicateHandle",
    "GetSystemTick",
    "GetHandleInfo",
    "GetSystemInfo",
    "GetProcessInfo",
    "GetThreadInfo",
    "ConnectToPort",
    "SendSyncRequest1",
    "SendSyncRequest2",
    "SendSyncRequest3",
    "SendSyncRequest4",
    "SendSyncRequest",
    "OpenProcess",
    "OpenThread",
    "GetProcessId",
    "GetProcessIdOfThread",
    "GetThreadId",
    "GetResourceLimit",
    "GetResourceLimitLimitValues",
    "GetResourceLimitCurrentValues",
    "GetThreadContext",
    "Break",
    "OutputDebugString",
    "ControlPerformanceCounter",
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "CreatePort",
    "CreateSessionToPort",
    "CreateSession",
    "AcceptSession",
    "ReplyAndReceive1",
    "ReplyAndReceive2",
    "ReplyAndReceive3",
    "ReplyAndReceive4",
    "ReplyAndReceive",
    "BindInterrupt",
    "UnbindInterrupt",
    "InvalidateProcessDataCache",
    "StoreProcessDataCache",
    "FlushProcessDataCache",
    "StartInterProcessDma",
    "StopDma",
    "GetDmaState",
    "RestartDma",
    NULL,
    "DebugActiveProcess",
    "BreakDebugProcess",
    "TerminateDebugProcess",
    "GetProcessDebugEvent",
    "ContinueDebugEvent",
    "GetProcessList",
    "GetThreadList",
    "GetDebugThreadContext",
    "SetDebugThreadContext",
    "QueryDebugProcessMemory",
    "ReadProcessMemory",
    "WriteProcessMemory",
    "SetHardwareBreakPoint",
    "GetDebugThreadParam",
    NULL, NULL,
    "ControlProcessMemory",
    "MapProcessMemory",
    "UnmapProcessMemory",
    NULL, NULL, NULL,
    "TerminateProcess",
    "No ",
    "CreateResourceLimit",
    NULL, NULL,
    "KernelSetState",
    "QueryProcessMemory",
    NULL
};

void svc_Execute(u8 num)
{
    const char* name = names[num & 0xFF];

    if(name == NULL)
	name = "Unknown";

    DEBUG("-- svc%s (0x%x) --\n", name, num);

    if(num == 1) {
	svcControlMemory();
	arm11_SetR(0, 1);
	return;
    }
    else if(num == 0x21) {
	arm11_SetR(0, 1);
	return;
    }
    else if(num == 0x23) {
	printf("handle=%08x\n", arm11_R(0));
	PAUSE();
	arm11_SetR(0, 1);
	return;
    }
    else if(num == 0x38) {
	printf("resourcelimit=%08x, handle=%08x\n", arm11_R(0), arm11_R(1));
	arm11_SetR(0, 1);
	return;
    }
    else if(num == 0x3a) {
	printf("values_ptr=%08x, handleResourceLimit=%08x, names_ptr=%08x, nameCount=%d\n",
	       arm11_R(0), arm11_R(1), arm11_R(2), arm11_R(3));
	arm11_SetR(0, 1);
	return;
    }

    arm11_Dump();
    PAUSE();
    exit(1);
}
