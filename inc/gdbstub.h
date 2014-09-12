/*
	Copyright (C) 2006 Ben Jaques
	Copyright (C) 2008-2009 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>

struct armcpu_memory_iface {
    /** the 32 bit instruction prefetch */
    u32 (*prefetch32)(void *data, u32 adr);

    /** the 16 bit instruction prefetch */
    u16 (*prefetch16)(void *data, u32 adr);

    /** read 8 bit data value */
    u8 (*read8)(void *data, u32 adr);
    /** read 16 bit data value */
    u16 (*read16)(void *data, u32 adr);
    /** read 32 bit data value */
    u32 (*read32)(void *data, u32 adr);

    /** write 8 bit data value */
    void (*write8)(void *data, u32 adr, u8 val);
    /** write 16 bit data value */
    void (*write16)(void *data, u32 adr, u16 val);
    /** write 32 bit data value */
    void (*write32)(void *data, u32 adr, u32 val);

    void *data;
};

struct armcpu_ctrl_iface {
    /** stall the processor */
    void(*stall)(void *instance);

    /** unstall the processor */
    void(*unstall)(void *instance);

    /** read a register value */
    u32(*read_reg)(void *instance, u32 reg_num);

    /** set a register value */
    void(*set_reg)(void *instance, u32 reg_num, u32 value);

    /** install the post execute function */
    void(*install_post_ex_fn)(void *instance,
        void(*fn)(void *, u32 adr, int thumb),
        void *fn_data);

    /** remove the post execute function */
    void(*remove_post_ex_fn)(void *instance);

    /** the private data passed to all interface functions */
    void *data;
};

#ifndef _GDBSTUB_H_
#define _GDBSTUB_H_ 1

typedef void *gdbstub_handle_t;

/*
 * The function interface
 */
#ifdef __cplusplus
extern "C"
#endif
gdbstub_handle_t
createStub_gdb( u16 port,
                struct armcpu_memory_iface **cpu_memio,
                struct armcpu_memory_iface *direct_memio);

void
destroyStub_gdb( gdbstub_handle_t stub);
#ifdef __cplusplus
extern "C"
#endif
void
activateStub_gdb( gdbstub_handle_t stub,
                  struct armcpu_ctrl_iface *cpu_ctrl);

  /*
   * An implementation of the following functions is required
   * for the GDB stub to function.
   */


void *
createThread_gdb( void (WINAPI *thread_function)( void *data),
                  void *thread_data);
void
joinThread_gdb( void *thread_handle);

#ifdef __GNUC__
#define UNUSED_PARM( parm) parm __attribute__((unused))
#else
#define UNUSED_PARM( parm) parm
#endif

static void
break_execution(void *data, UNUSED_PARM(uint32_t addr), UNUSED_PARM(int thunmb));

#endif /* End of _GDBSTUB_H_ */

