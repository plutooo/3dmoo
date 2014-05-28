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

#define CMD(n)                                  \
    mem_Read32(0xFFFF0080 + 4*(n))

#define EXTENDED_CMD(n)                         \
    mem_Read32(0xFFFF0180 + 4*(n))


#define RESP(n, w)                              \
    mem_Write32(0xFFFF0080 + 4*(n), w)

#define SERVICE_START(name)                     \
    u32 name ## _SyncRequest() {                \
        switch(CMD(0)) {

#define SERVICE_CMD(id)                         \
        case (id):

#define SERVICE_END()                                    \
        default:                                         \
            ERROR("Not implemented cmd %08x\n", CMD(0)); \
            return -1;                                   \
        }                                                \
        return -1;                                       \
    }
