AC_DEFUN([ACX_SOCI3],
[

AC_ARG_WITH(soci,
    [AC_HELP_STRING([--with-soci=<path>],[prefix of SOCI installation. e.g. /usr/local or /usr])],
    [SOCI_PREFIX=$with_soci],
    AC_MSG_ERROR([You must call configure with the --with-soci option.
    This tells configure where to find the SOCI library and headers.
    e.g. --with-soci=/usr/local or --with-soci=/usr])
)

AC_SUBST(SOCI_PREFIX)
SOCI_LIBS="-L${SOCI_PREFIX}/lib -lsoci_core"
SOCI_LIBS_ORA="-L${SOCI_PREFIX}/lib -lsoci_oracle"
SOCI_LIBS_PG="-L${SOCI_PREFIX}/lib -lsoci_postgresql"
SOCI_CPPFLAGS="-I${SOCI_PREFIX}/include/soci"
SOCI_CPPFLAGS2="-I${SOCI_PREFIX}/include"
AC_SUBST(SOCI_LIBS)
AC_SUBST(SOCI_CPPFLAGS)
AC_SUBST(SOCI_CPPFLAGS2)
AC_SUBST(SOCI_LIBS_ORA)
AC_SUBST(SOCI_LIBS_PG)
SOCI_CFLAGS="$SOCI_CPPFLAGS"
AC_SUBST(SOCI_CFLAGS)
AC_DEFINE(HAVE_SOCI,,[define if soci library avaiable])
])
