/*
 * Copyright (C) 2014 - plutoo
 * Copyright (C) 2014 - ichfly
 * Copyright (C) 2014 - Normmatt
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
#include "handles.h"
#include "mem.h"
#include "arm11.h"

#include "service_macros.h"


SERVICE_START(ldr_ro);

SERVICE_CMD(0x100C2)   //Initialize
{
    DEBUG("Initialize -- stubbed -- %08x,%08x,%08x,%08x,%08x\n",
          CMD(1), CMD(2), CMD(3), CMD(4), CMD(5));

    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x20082)   //LoadCRR
{
    DEBUG("LoadCRR -- stubbed -- %08x,%08x,%08x,%08x\n",
          CMD(1), CMD(2), CMD(3), CMD(4));

    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x30042)   //UnloadCCR
{
    DEBUG("UnloadCCR -- stubbed -- %08x,%08x,%08x\n",
          CMD(1), CMD(2), CMD(3));

    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x402C2)   //LoadExeCRO
{
    DEBUG("LoadExeCRO -- stubbed -- %08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x\n",
          CMD(1), CMD(2), CMD(3), CMD(4), CMD(5), CMD(6), CMD(7), CMD(8), CMD(9), CMD(10), CMD(11), CMD(12), CMD(13));

    RESP(1, 0); // Result
    RESP(2, 0); // Unknown
    return 0;
}

SERVICE_CMD(0x500C2)   //LoadCROSymbols
{
    DEBUG("LoadCROSymbols -- stubbed -- %08x,%08x,%08x,%08x,%08x\n",
          CMD(1), CMD(2), CMD(3), CMD(4), CMD(5));

    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x80042)   //Shutdown
{
    DEBUG("Shutdown -- stubbed -- %08x,%08x,%08x\n",
          CMD(1), CMD(2), CMD(3));

    RESP(1, 0); // Result
    return 0;
}

SERVICE_END();
