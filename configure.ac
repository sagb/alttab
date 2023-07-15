AC_INIT([alttab], [1.7.1])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CC
PKG_CHECK_MODULES([x11], [x11])
PKG_CHECK_MODULES([xft], [xft])
PKG_CHECK_MODULES([xrender], [xrender])
PKG_CHECK_MODULES([xrandr], [xrandr])
PKG_CHECK_MODULES([libpng], [libpng])
PKG_CHECK_MODULES([xpm], [xpm])

AC_MSG_CHECKING(for standalone fts library)
if pkg-config --exists libfts ; then
    AC_MSG_RESULT(libfts)
    AC_SUBST([fts_CFLAGS], [`pkg-config --cflags libfts`])
    AC_SUBST([fts_LIBS], [`pkg-config --libs libfts`])
else
    if pkg-config --exists musl-fts ; then
        AC_MSG_RESULT(musl-fts)
        AC_SUBST([fts_CFLAGS], [`pkg-config --cflags musl-fts`])
        AC_SUBST([fts_LIBS], [`pkg-config --libs musl-fts`])
    else
        AC_MSG_RESULT(no)
        AC_MSG_NOTICE(libfts or musl-fts not found. Assuming that fts functions are included in libc.)
    fi
fi

AC_CHECK_HEADERS([uthash.h],
  [],
  [AC_MSG_ERROR([Cannot find uthash.h, please install uthash])]
  )
AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile test/Makefile])
AC_REQUIRE_AUX_FILE([tap-driver.sh])
AC_OUTPUT
