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
#include "handles.h"
#include "svc.h"

#include "mem.h"

extern ARMul_State s;


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
    NULL,
    "CreateResourceLimit",
    NULL, NULL,
    "KernelSetState",
    "QueryProcessMemory",
    NULL
};


void svc_Execute(ARMul_State * state, ARMword num)
{
    const char* name = names[num & 0xFF];

    if (name == NULL)
        name = "Unknown";

    LOG("\n>> svc%s (0x%x)\n", name, num);

    switch (num) {
    case 1:
        arm11_SetR(0, svcControlMemory());
        return;
    case 2:
        //Stubbed for now
        //arm11_SetR(0, svcQueryMemory()); //uses R0-R5 r0 = return code r1 = Base process virtual address  r2 = size r3 = perm r4 = MemoryState? r5 = PageInfo
        DEBUG("%x, %x, %x\n", arm11_R(0), arm11_R(1), arm11_R(2));
        arm11_SetR(0, 0);
        arm11_SetR(1, 0x14000000);
        arm11_SetR(2, 0x10000);
        arm11_SetR(4, 6);
        return;
    case 3: //Exit Process
        state->NumInstrsToExecute = 0;
        exit(1);
        return;

    case 0x5:
        DEBUG("SetProcessAffinityMask %08x %08x %08x\n", arm11_R(1), arm11_R(2), arm11_R(3));
        DEBUG("STUBBED\n");
        arm11_SetR(1, 0);
        return;

    case 0x7:
        DEBUG("SetProcessIdealProcessor %08x %08x\n", arm11_R(1), arm11_R(2));
        DEBUG("STUBBED\n");
        arm11_SetR(1, 0);
        return;

    case 8:
        arm11_SetR(0, svcCreateThread());
        return;
    case 9: //Exit Thread
        arm11_SetR(0, 0);
        state->NumInstrsToExecute = 0;
        threads_StopCurrentThread();
        threads_Reschedule();
        return;
    case 0xa:
        arm11_SetR(0, svcsleep());
        state->NumInstrsToExecute = 0;
        threads_Reschedule();
        return;
    case 0xb:
        arm11_SetR(0, svcGetThreadPriority());
        return;
    case 0xc:
        arm11_SetR(0, svcSetThreadPriority());
        return;

    case 0x12:
        arm11_SetR(0, svcRun());
        return;

    case 0x13:
        arm11_SetR(0, svcCreateMutex());
        return;
    case 0x14:
        arm11_SetR(0, svcReleaseMutex());
        return;
    case 0x15:
        arm11_SetR(0, svcCreateSemaphore());
        return;
    case 0x16:
        arm11_SetR(0, svcReleaseSemaphore());
        return;
    case 0x17:
        arm11_SetR(0, svcCreateEvent());
        return;
    case 0x18:
        arm11_SetR(0, svcSignalEvent());
        return;
    case 0x19:
        arm11_SetR(0, svcClearEvent());
        return;
    case 0x1A:
        arm11_SetR(0, svcCreateTimer());
        return;
    case 0x1B:
        arm11_SetR(0, svcSetTimer());
        return;
    case 0x1E:
        arm11_SetR(0, svcCreateMemoryBlock());
        return;
    case 0x1F:
        arm11_SetR(0, svcMapMemoryBlock());
        return;
    case 0x20:
        arm11_SetR(0, svcUnmapMemoryBlock());
        return;
    case 0x21:
        arm11_SetR(0, svcCreateAddressArbiter());
        return;
    case 0x22:
        arm11_SetR(0, svcArbitrateAddress());
        return;
    case 0x23:
        arm11_SetR(0, svcCloseHandle());
        return;
    case 0x24:
        arm11_SetR(0, svcWaitSynchronization1());
        state->NumInstrsToExecute = 0;
        threads_Reschedule();
        return;
    case 0x25:
        arm11_SetR(0, svcWaitSynchronizationN());
        state->NumInstrsToExecute = 0;
        threads_Reschedule();
        return;
    case 0x27:
        arm11_SetR(0, svcDuplicateHandle());
        return;
    case 0x28: //GetSystemTick
        arm11_SetR(0, (u32)s.NumInstrs);
        arm11_SetR(1, (u32)(s.NumInstrs >> 32));
        return;
    case 0x2A:
        arm11_SetR(0, svcGetSystemInfo());
        return;
    case 0x2B:
        DEBUG("svcGetProcessInfo=%08x\n",  arm11_R(2));
        DEBUG("STUBBED\n");
        arm11_SetR(0, 0);
        return;
    case 0x2D:
        arm11_SetR(0, svcConnectToPort());
        return;
    case 0x32:
        arm11_SetR(0, svcSendSyncRequest());
        return;
    case 0x33:
        DEBUG("OpenProcess=%08x\n",arm11_R(1));
        DEBUG("STUBBED\n");
        arm11_SetR(0, 0);
        arm11_SetR(1, handle_New(0, HANDLE_TYPE_PROCESS ));
        return;
    case 0x35:
        DEBUG("GetProcessId=%08x\n", arm11_R(1));
        DEBUG("STUBBED\n");
        arm11_SetR(0, 0);
        arm11_SetR(1, 5);
        return;
    case 0x38:
        DEBUG("resourcelimit=%08x, handle=%08x\n", arm11_R(0), arm11_R(1));
        DEBUG("STUBBED\n");
        PAUSE();
        //arm11_SetR(0, 1);
        arm11_SetR(1, handle_New(0, 0)); // r1 = handle_out
        return;
    case 0x37:
        arm11_SetR(0, svcGetThreadId());
        return;
    case 0x3A:
        DEBUG("values_ptr=%08x, handleResourceLimit=%08x, names_ptr=%08x, nameCount=%d\n",
              arm11_R(0), arm11_R(1), arm11_R(2), arm11_R(3));
        arm11_SetR(0, svcGetResourceLimitCurrentValues());
        return;
    case 0x3C: //Break
        ERROR("Break %x (%d).\n", arm11_R(0), arm11_R(0));
        exit(1);
        return;
    case 0x3D: { //OutputDebugString
        static char   *buffer = NULL;
        static size_t bufferlen = 0;

        size_t sz = arm11_R(1);
        if (sz >= bufferlen) {
            char *tmp = (char*)realloc(buffer, sz+1);
            if(tmp != NULL) {
                buffer    = tmp;
                bufferlen = sz+1;
            }
            else
                sz = bufferlen-1;
        }

        if(buffer != NULL) {
            memset(buffer, 0, bufferlen);
            if(mem_Read(buffer, arm11_R(0), sz) == 0)
                MSG("%s", buffer);
        }
        return;
    }
    case 0x47:
        arm11_SetR(0, svcCreatePort());
        return;
    case 0x4a:
        arm11_SetR(0, svcAcceptSession());
        return;
    case 0x4F:
        arm11_SetR(0, svcReplyAndReceive());
        state->NumInstrsToExecute = 0;
        return;
    case 0x50:
        arm11_SetR(0, svcBindInterrupt());
        return;
    case 0x73:
        arm11_SetR(0, svcCreateCodeSet());
        return;
    case 0x75:
        arm11_SetR(0, svcCreateProcess());
        return;
    case 0x77:
        DEBUG("ResourceLimits %08x %08x\n", arm11_R(1), arm11_R(2));
        DEBUG("STUBBED\n");
        arm11_SetR(0, 0);
        return;
    case 0x78:
        arm11_SetR(0, svcCreateResourceLimit());
        return;
    case 0x79:
        arm11_SetR(0, svcSetResourceLimitValues());
        return;
    case 0x7C:
        arm11_SetR(0, svcKernelSetState());
        return;
    case 0xFF:
        fprintf(stdout, "%c", (u8)arm11_R(0));
        return;

    default:
        //Lets not error yet
        ERROR("Syscall (0x%02X) not implemented.\n", num);
        arm11_SetR(0, -1);
        break;

        arm11_Dump();
        PAUSE();
        //exit(1);
    }
}


u32 svcGetProcessId()
{
    u32 handle = arm11_R(1);

    ERROR("svcGetProcessId (%02X)\n", handle);

    if(handle == 0xffff8000) {
        arm11_SetR(1, 1);
        return 0;
    }
    else {
        THREADDEBUG("svcGetProcessId not supported\n");
        return 0;
    }
}