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

#define LCDCOLORFILLMAIN 0x202204
#define LCDCOLORFILLSUB 0x202A04

#define frameselectoben 0x400478
#define RGBuponeleft 0x400468
#define RGBuptwoleft 0x40046C

#define RGBdownoneleft 0x400494
#define RGBdowntwoleft 0x400498


u8* IObuffer;
u8* LINEmembuffer;
u8* VRAMbuff;
u8* GSPsharedbuff;

#define GSPsharebuffsize 0x1000 //dumped from GSP module in Firm 4.4

void initGPU();
void GPUwritereg32(u32 addr, u32 data);
u32 GPUreadreg32(u32 addr);
void GPUTriggerCmdReqQueue();
void GPURegisterInterruptRelayQueue(u32 Flags, u32 Kevent, u32*threadID, u32*outMemHandle);
u8* get_pymembuffer(u32 addr);
u32 get_py_memrestsize(u32 addr);