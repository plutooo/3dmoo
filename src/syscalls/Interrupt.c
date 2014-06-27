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
#include "threads.h"



u32 svcBindInterrupt()
{
    u32 name = arm11_R(0);
    u32 syncObject = arm11_R(1);
    u32 priority = arm11_R(2);
    u32 isManualClear = arm11_R(3);
    DEBUG("svcBindInterrupt %08x %08x %08x %08x\n", name, syncObject, priority, isManualClear);
    return 0;
}
