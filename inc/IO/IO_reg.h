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
u8 IO_Read8(u32 addr);
u16 IO_Read16(u32 addr);
u32 IO_Read32(u32 addr);
void IO_Write8(u32 addr, u8 v);
void IO_Write16(u32 addr, u16 v);
void IO_Write32(u32 addr, u32 v);
u32 hwfireInterrupt(u32 id);