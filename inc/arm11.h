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

void arm11_Init();
void arm11_SetPCSP(u32 pc, u32 sp);
u32  arm11_R(u32 n);
void arm11_SetR(u32 n, u32 val);
bool arm11_Step();
void arm11_Dump();

void arm11_LoadContext(u32 r_in[18]);
void arm11_SaveContext(u32 r_out[18]);
