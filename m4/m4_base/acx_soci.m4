AC_DEFUN([ACX_SOCI],
[

AC_ARG_WITH(soci,
    [AC_HELP_STRING([--with-soci=<path>],[prefix of SOCI installation. e.g. /usr/local or /usr])],
    [SOCI_PREFIX=$with_soci],
    AC_MSG_ERROR([You must call configure with the --with-soci option.
    This tells configure where to find the SOCI library and headers.
    e.g. --with-soci=/usr/local or --with-soci=/usr])
)

AC_SUBST(SOCI_PREFIX)
SOCI_ORACLE_CFLAGS="-I${SOCI_PREFIX}/include/soci/oracle"
SOCI_SQLITE3_CFLAGS="-I${SOCI_PREFIX}/include/soci/sqlite3"
SOCI_ORACLE_LIBS="-lsoci_oracle-gcc-2_2"
SOCI_SQLITE3_LIBS="-lsoci_sqlite3-gcc-2_2" 
SOCI_LIBS="-L${SOCI_PREFIX}/lib -lsoci_core-gcc-2_2 $SOCI_ORACLE_LIBS $SOCI_SQLITE3_LIBS"
SOCI_CFLAGS="-I${SOCI_PREFIX}/include/soci $SOCI_ORACLE_CFLAGS $SOCI_SQLITE3_CFLAGS"
AC_SUBST(SOCI_LIBS)
AC_SUBST(SOCI_CFLAGS)
AC_DEFINE(HAVE_SOCI,,[define if soci library avaiable])
])