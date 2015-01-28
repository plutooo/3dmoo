#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([3dmoo],[0.1.0],[https://github.com/plutooo/3dmoo/issues])
AC_CONFIG_SRCDIR([src/config.c])

AM_INIT_AUTOMAKE([1.10])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_MKDIR_P

dnl Check for SDL
SDL2_VERSION=2.0.0
AM_PATH_SDL2($SDL2_VERSION,
          :,
          AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!])
)
CFLAGS="$CFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"

case "$host" in
   *-*-mingw*)
      LIBS="$LIBS -lws2_32 -mconsole"
      ;;
esac

AC_ARG_ENABLE(gdb-stub,AC_HELP_STRING([--enable-gdb-stub],[enable the gdb stub]),,enable_gdb_stub="no")
AC_ARG_ENABLE(debug-messages,AC_HELP_STRING([--disable-debug-messages],[disable the debug messages]),,enable_debug_messages="yes")

AM_CONDITIONAL(DISABLE_DEBUG_MESSAGES, test "$enable_debug_messages" = "no")
AM_CONDITIONAL(ENABLE_GDB,test "$enable_gdb_stub" = "yes")

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
