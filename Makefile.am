# Makefile.am -- Process this file with automake to produce Makefile.in

SRC_FILES	=	src/mem.c src/screen.c src/handles.c src/loader.c src/utils.c	\
			src/svc.c src/config.c src/gdb/gdbstubchelper.c src/gdb/gdbstub.c \
			src/gdb/gdb.c

ARM11_FILES	=	src/arm11/armsupp.c src/arm11/armemu.c src/arm11/threads.c	\
			src/arm11/thumbemu.c src/arm11/arminit.c src/arm11/armcopro.c	\
			src/arm11/wrap.c src/arm11/vfp/vfpinstr.c src/arm11/vfp/vfp.c	\
			src/arm11/vfp/vfpsingle.c src/arm11/vfp/vfpdouble.c

SYSCALLS_FILES	=	src/syscalls/timer.c src/syscalls/arb.c src/syscalls/resource_limit.c	\
			src/syscalls/memory.c src/syscalls/interrupt.c src/syscalls/syn.c	\
			src/syscalls/ports.c src/syscalls/events.c

SERVICES_FILES	=	src/services/PxiPS9.c src/services/pix_dev.c src/services/cdc_DSP.c	\
			src/services/nim_aoc.c src/services/gsp_gpu.c src/services/cfg_s.c	\
			src/services/hid_user.c src/services/mic_u.c src/services/pdn_d.c	\
			src/services/am_sys.c src/services/apt_u.c src/services/i2cLCD.c	\
			src/services/ps_ps.c src/services/ptm_u.c src/services/ldr_ro.c		\
			src/services/ssl_c.c src/services/csnd_SND.c src/services/cfg_i.c	\
			src/services/cdc_csn.c src/services/mcu_GPU.c src/services/soc_u.c	\
			src/services/apt_s.c src/services/ptm_play.c src/services/nims.c	\
			src/services/am_u.c src/services/hid_SPVR.c src/services/ns_s.c		\
			src/services/ndm_u.c src/services/PxiPM.c src/services/fs_REG.c		\
			src/services/frd_u.c src/services/boss_p.c src/services/boss_u.c	\
			src/services/Pxiam9.c src/services/pdn_g.c src/services/frd_a.c		\
			src/services/ptm_s.c src/services/fs_ldr.c src/services/srv.c		\
			src/services/ir_u.c src/services/ipc_decoder.c src/services/aci.c	\
			src/services/am_app.c src/services/ir_rst.c src/services/dsp_dsp.c	\
			src/services/cecd_u.c src/services/mcuPLS.c src/services/PxiFS0.c	\
			src/services/cfg_u.c src/services/cam_u.c src/services/y2r_u.c		\
			src/services/cfg_nor.c src/services/http_c.c src/services/fs_user.c

FS_FILES	=	src/fs/sysdata.c src/fs/sysdatac.c src/fs/shared_extdata.c src/fs/util.c \
			src/fs/romfs.c src/fs/savedata.c src/fs/extsavedata.c src/fs/sdmc.c

GPU_FILES	=	src/gpu/gpu.c src/gpu/clipper.c src/gpu/io.c src/gpu/rasterizer.c \
			src/gpu/vec4.c src/gpu/color.c src/gpu/commands.c

DSP_FILES	=

CRYPTO_FILES	=	src/lib/crypto/bignum.c src/lib/crypto/aes.c src/lib/crypto/sha2.c src/lib/crypto/rsa.c \
			src/lib/crypto/nin_public_crypt.c

HTTP_FILES	=	src/lib/http/HTTPClientString.c src/lib/http/HTTPClientWrapper.c \
			src/lib/http/HTTPClientAuth.c src/lib/http/HTTPClient.c

MAIN_FILES	=	src/main.c

noinst_HEADERS	=	inc/3dsx.h			\
			inc/arm11.h			\
			inc/config.h			\
			inc/crypto/aes.h		\
			inc/crypto/bignum.h		\
			inc/crypto/bn_mul.h		\
			inc/crypto/config.h		\
			inc/crypto/nin_public_crypto.h	\
			inc/crypto/padlock.h		\
			inc/crypto/rsa.h		\
			inc/crypto/sha2.h		\
			inc/dirent.h			\
			inc/fs.h			\
			inc/gdb/gdbstub.h		\
			inc/gdb/gdbstubchelper.h	\
			inc/gdb/gdbstub_internal.h	\
			inc/gpu.h			\
			inc/handles.h			\
			inc/hid_user.h			\
			inc/http/HTTPClient.h		\
			inc/http/HTTPClientAuth.h	\
			inc/http/HTTPClientCommon.h	\
			inc/http/HTTPClientString.h	\
			inc/http/HTTPClientWrapper.h	\
			inc/init.h			\
			inc/loader.h			\
			inc/mem.h			\
			inc/screen.h			\
			inc/svc.h			\
			inc/threads.h			\
			inc/util.h			\
			inc/vec4.h			\
			src/arm11/armdefs.h		\
			src/arm11/armemu.h		\
			src/arm11/arm_arch_interface.h	\
			src/arm11/arm_regformat.h	\
			src/arm11/vfp/asm_vfp.h		\
			src/arm11/vfp/vfp.h		\
			src/arm11/vfp/vfpdouble.h	\
			src/arm11/vfp/vfp_helper.h	\
			src/gpu/mat4.h			\
			src/services/service_macros.h

AM_CPPFLAGS	=	-iquote$(top_srcdir)/inc -iquote$(top_srcdir)/src/arm11 -iquote$(top_srcdir)/src/arm11/vfp \
			-Wno-format-zero-length -DMODET -DMODE32 -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_SOURCE -DGDB_STUB \
			-std=c99 -DDISABLE_DEBUG

bin_PROGRAMS	=	3dmoo

3dmoo_SOURCES	=	$(MAIN_FILES) $(SRC_FILES) $(ARM11_FILES) $(SYSCALLS_FILES) $(SERVICES_FILES) $(FS_FILES) $(GPU_FILES) $(DSP_FILES) $(CRYPTO_FILES) $(HTTP_FILES)

EXTRA_DIST	=	autogen.sh
