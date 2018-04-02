# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT(BLK-BK-11-EMU, 2018
AC_CONFIG_SRCDIR([terakdisk.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

case "$GCC" in
yes) CFLAGS="-Wall $CFLAGS" ;;
esac

# Checks for system type
AC_CANONICAL_TARGET
case "${target_os}" in
*darwin* )
	LDFLAGS="-framework Cocoa $LDFLAGS"
	LIBS="$LIBS -lSDLmain"
	;;
*)
	LIBS="$LIBS -lSDL"
	;;
esac


# Checks for libraries.
if test ! -f /usr/include/libintl.h -a \
	! -f /usr/local/include/libintl.h -a \
	-f /sw/include/libintl.h
then
	CFLAGS="$CFLAGS -I/sw/include"
	LDFLAGS="$LDFLAGS -L/sw/lib"
fi
AC_SEARCH_LIBS(SDL_Init, SDL)
AC_SEARCH_LIBS([bindtextdomain], [intl])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h libintl.h locale.h sgtty.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h unistd.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_OFF_T
AC_HEADER_TIME
AC_C_VOLATILE

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([atexit gettimeofday memset munmap select setlocale strcasecmp strchr strdup strspn])

AC_CONFIG_FILES([Makefile po/Makefile.in])
AC_OUTPUT

GETTEXT_PACKAGE=bk
