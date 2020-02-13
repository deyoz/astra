AC_INIT([serverlib], [0.1])
##AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

if test "x$LIBROOT" = "x" ; then 
    export LIBROOT=$HOME
fi
echo "use LIBROOT=$LIBROOT"

AC_PREFIX_DEFAULT(${HOME}/local)

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_LANG_CPLUSPLUS

AC_PROG_CXX

AC_DISABLE_SHARED

AC_PROG_LIBTOOL

ACX_PTHREAD


AC_DEFUN([AX_CXX_FLAG_CHECK_ADD],
         [
          AC_LANG_PUSH([C++])
          CXXFLAGS_SAVE=$CXXFLAGS
          CXXFLAGS=["$1 -Werror"]
          AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])], [$2="$$2 $1"],[])
          CXXFLAGS=$CXXFLAGS_SAVE
          AC_LANG_POP([C++])
          ])


MRJ_CHECK_ORACLE

CHECK_SSL(yes)
if ! test "$SSL_LIBS" = ""; then
    AC_SUBST(WITH_CRYPT_CFLAGS, ["-DWITH_CRYPT"])
fi

if ! test "$WITH_MESPRO" = ""; then
    AC_SUBST(MESPRO_CFLAGS, ["$MESPRO_CFLAGS"])
    AC_SUBST(MESPRO_CXXFLAGS, ["$MESPRO_CXXFLAGS"])
    AC_SUBST(MESPRO_LDFLAGS, ["$MESPRO_LDFLAGS"])        
fi

CHECK_TCL
AX_BOOST_BASE([1.55], [], AC_MSG_ERROR([Boost C++ libraries not found]))
AX_BOOST_SYSTEM
AX_BOOST_DATE_TIME
AX_BOOST_SERIALIZATION
AX_BOOST_REGEX
MRJ_CHECK_GLIBCXX_DEBUG

PKG_CHECK_MODULES([AMQP_CPP], [amqp-cpp > 4], have_amqp_cpp=true, have_amqp_cpp=false)
AS_IF(["$have_amqp_cpp"], [ AC_DEFINE([HAVE_AMQP_CPP], [1], [Have amqp-cpp library])])
AM_CONDITIONAL([HAVE_AMQP_CPP], ["$have_amqp_cpp"])

AC_ARG_ENABLE([xp-testing],
              [AS_HELP_STRING([--enable-xp-testing],[Enable xp-testing])],
              [ 
                  [if test x$no_check != xyes; then enable_xp_testing=yes; fi]
                  AM_CONDITIONAL([XP_TESTING], [test x$enable_xp_testing = xyes])
              ],\
              [AM_CONDITIONAL([XP_TESTING], [test 1 = 0])])

if test x$enable_xp_testing = xyes; then
    AC_DEFINE([XP_TESTING], , [XP_TESTING support])
    PKG_CHECK_MODULES([CHECK], [check], AC_MSG_NOTICE($CHECK_LIBS), AC_MSG_ERROR([libcheck not found]))
fi
PKG_CHECK_MODULES([MEMCACHED], [libmemcached >= 1],
                  [AC_DEFINE([HAVE_MEMCACHED], , [libmemcached found])],
                  [AC_MSG_NOTICE(libmemcached not found)])
PKG_CHECK_MODULES(XML, libxml-2.0, [AC_MSG_NOTICE($XML_LIBS)], AC_MSG_ERROR([libxml2 not found]))
PKG_CHECK_MODULES(XSLT, libxslt, [AC_MSG_NOTICE($XSLT_LIBS)], AC_MSG_ERROR([libxslt not found]))
PKG_CHECK_MODULES(ZLIB, zlib, 
              [
                 AC_MSG_NOTICE($ZLIB_LIBS)
                 AC_DEFINE(WITH_ZLIB, , [zlib enabled])
              ], AC_MSG_ERROR([zlib not found]))

AX_RISC_ORDER

AC_ARG_ENABLE([pg],
    [AS_HELP_STRING([--enable-pg],[Enable PostgreSQL support])],
    [enable_pg=yes],
    [enable_pg=no])
if test "x$enable_pg" = xyes; then
    ENABLE_PG="-DENABLE_PG"
    PKG_CHECK_MODULES([LIBPQ], [libpq], AC_MSG_NOTICE($LIBPQ_LIBS), AC_MSG_ERROR([libpq not found]))
fi

AC_ARG_ENABLE([pg-tests],
    [AS_HELP_STRING([--enable-pg-tests],[Enable PostgreSQL tests])],
    [enable_pg_tests=yes],
    [enable_pg_tests=no])
if test "x$enable_pg_tests" = xyes && test "x$enable_pg" = xyes; then
    ENABLE_PG_TESTS="-DENABLE_PG_TESTS"
    PKG_CHECK_MODULES([LIBPQ], [libpq], AC_MSG_NOTICE($LIBPQ_LIBS), AC_MSG_ERROR([libpq not found]))
fi

AC_ARG_VAR(CPP_STD_VERSION, [Set c++11 or c++14 or c++17])
AS_IF( [test x"$CPP_STD_VERSION" = x"c++17" ], [ AX_CXX_COMPILE_STDCXX(17,[noext],[mandatory]) ],
       [test x"$CPP_STD_VERSION" = x"c++14" ], [ AX_CXX_COMPILE_STDCXX(14,[noext],[mandatory]) ],
                                               [ AX_CXX_COMPILE_STDCXX(11,[noext],[mandatory]) ])
ALLWARNX="-Wnon-virtual-dtor -Wignored-qualifiers"

AX_CXX_FLAG_CHECK_ADD(["-Wformat"], [ALLWARNX])
AX_CXX_FLAG_CHECK_ADD(["-Winfinite-recursion"], [ALLWARNX])
AX_CXX_FLAG_CHECK_ADD(["-Wdeprecated-declarations"], [ALLWARNX])
AX_CXX_FLAG_CHECK_ADD(["-Wunused-but-set-parameter"], [ALLWARNX])
AX_CXX_FLAG_CHECK_ADD(["-Wno-unused-local-typedefs"], [ALLWARNX])
AX_CXX_FLAG_CHECK_ADD(["-Wno-unused-parameter"], [ALLWARNX])
AX_CXX_FLAG_CHECK_ADD(["-Wno-missing-field-initializers"], [ALLWARNX])
AX_CXX_FLAG_CHECK_ADD(["-Wunreachable-code"], [ALLWARNX])
AX_CXX_FLAG_CHECK_ADD(["-Wunused-function"], [ALLWARNX])
AX_CXX_FLAG_CHECK_ADD(["-Wunused-result"], [ALLWARNX])
AX_CXX_FLAG_CHECK_ADD(["-Wcast-qual"], [ALLWARNX])
AX_CXX_FLAG_CHECK_ADD(["-Werror=non-virtual-dtor"], [ALLWARNX])
AX_CXX_FLAG_CHECK_ADD(["-Werror=constant-conversion"], [ALLWARNX])
AX_CXX_FLAG_CHECK_ADD(["-Werror=format-overflow"], [ALLWARNX])

ALLWARN="-g -pipe -Wall -Wextra -Werror=format -Wno-error=format-security -Wno-error=format-nonliteral -Wtype-limits -Werror=uninitialized -Werror=init-self -Werror=return-type -Wempty-body -Werror=switch"
ALLWARNC="" # -std=c99
CXXFLAGS="$CFLAGS $ALLWARN $ALLWARNX $ENABLE_PG $ENABLE_PG_TESTS $MY_LOCAL_CFLAGS"
CFLAGS="$CFLAGS $ALLWARN $ALLWARNC $ENABLE_PG $MY_LOCAL_CFLAGS"

AC_SUBST(SERVERLIB_CFLAGS, ["$CFLAGS"])
AC_SUBST(SERVERLIB_CXXFLAGS, ["$CXXFLAGS"])

AC_SUBST(LOGDIR, ["$PWD"])
AC_OUTPUT(serverlibconfig Makefile src/Makefile src/rsyslog.conf)

if ! test -d lib ; then
    mkdir lib
    cd lib
    ln -s ../src/libserver.la libserver.la
elif ! test -f lib/libserver.la ; then 
    cd lib
    ln -s ../src/libserver.la libserver.la
fi

#rm -f src/rsyslog.conf
#sed -s "s/LOGDIR/$PWD/g" src/rsyslog.conf.in > src/rsyslog.conf

echo 
echo Configuration for $PACKAGE $VERSION :
echo --------------------------------
echo 
echo Source code location ....: $srcdir
echo Host System Type ........: $host
echo CFLAGS  .................: $CFLAGS
echo CXXFLAGS  ...............: $CXXFLAGS
if test x$enable_xp_testing = xyes; then
    echo XP_TESTING enabled
fi
if test x$enable_pg = xyes; then
    echo PostgreSQL enabled
fi
echo Now type 'make' to build $PACKAGE $VERSION
echo
