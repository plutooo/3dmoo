#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([3dmoo],[0.1.0],[davem@devkitpro.org])
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

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
