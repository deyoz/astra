AC_INIT([edilib], [0.1])
AM_INIT_AUTOMAKE

AC_CONFIG_MACRO_DIR([m4])
if test "x$LIBROOT" = "x" ; then
        export LIBROOT=$HOME
fi
echo "use LIBROOT=$LIBROOT"

AC_PREFIX_DEFAULT(${HOME}/local)

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_LANG_C
AC_LANG_CPLUSPLUS
AC_PROG_CXX
AC_PROG_CC
# ᮧ� Shared ⥪
AC_DISABLE_SHARED
# � (quoting):
# This macro adds many new tests to the configure script
# so that the generated libtool script will understand
# the characteristics of the host
AC_PROG_LIBTOOL
#AC_PROG_RANLIB
#LT_INIT

CCLD=`echo $CC | sed 's/ccache//' | sed 's/distcc//' | sed 's/\ //'`
CXXLD=`echo $CXX | sed 's/ccache//' | sed 's/distcc//' | sed 's/\ //'`

AC_MSG_CHECKING(edisession support)
AC_ARG_WITH(edisession,
  [  --with-edisession=yes/no       enable support for "EdiSession" (need ServerLib/Oracle) default - yes ],
    [
    if test $withval = no; then
      want_edisession=no
      AC_MSG_RESULT(no)
    elif test $withval != yes; then
      AC_MSG_ERROR([Edisession support: Only yes/no possible])
    else
	want_edisession=yes
	AC_MSG_RESULT(yes)
    fi
    ], [want_edisession=yes
	AC_MSG_RESULT(yes)
	]
)

#check for oracle
MRJ_CHECK_ORACLE
#check for serverlib
MRJ_CHECK_SERVERLIB
#check for tcl
CHECK_TCL
#check boost
AX_BOOST_BASE(1.31, , [AC_MSG_ERROR([Need boost!])])
#check libtlg
MRJ_CHECK_LIBTLG
MRJ_CHECK_GLIBCXX_DEBUG

edi_cflags="-D_POINT_WARNING_ -D_DUAL_CHAR_SET_SUPPORT_=1"
AC_SUBST(EDILIB_CFLAGS, $edi_cflags)

AM_CONDITIONAL(ENABLE_SESSION_MNG, test "$enable_serverlib" = yes)

CXXFLAGS="$CXXFLAGS $SERVERLIB_CXXFLAGS"
CFLAGS="$CFLAGS $SERVERLIB_CFLAGS"

AC_CONFIG_FILES([Makefile src/Makefile edi_messages/Makefile])
AC_OUTPUT

if ! test -d lib ; then
	mkdir lib
fi

