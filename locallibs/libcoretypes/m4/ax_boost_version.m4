# ===========================================================================
#       http://www.gnu.org/software/autoconf-archive/ax_boost_base.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_BOOST_VERSION([MINIMUM-VERSION], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# DESCRIPTION
#
#   Test for version the Boost C++ libraries and execute actions
#
#

AC_DEFUN([AX_BOOST_VERSION],
[

if test "x$1" != "x"; then
    boost_lib_version_want="$1"
    boost_lib_version_want_shorten=`expr $boost_lib_version_want : '\([[0-9]]*\.[[0-9]]*\)'`
    boost_lib_version_want_major=`expr $boost_lib_version_want : '\([[0-9]]*\)'`
    boost_lib_version_want_minor=`expr $boost_lib_version_want : '[[0-9]]*\.\([[0-9]]*\)'`
    boost_lib_version_want_sub_minor=`expr $boost_lib_version_want : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
    if test "x$boost_lib_version_want_sub_minor" = "x" ; then
        boost_lib_version_want_sub_minor="0"
        fi
    WANT_BOOST_VERSION=`expr $boost_lib_version_want_major \* 100000 \+  $boost_lib_version_want_minor \* 100 \+ $boost_lib_version_want_sub_minor`
    AC_MSG_CHECKING(if boost version >= $boost_lib_version_want)
    succeeded=no

    CPPFLAGS_SAVED="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
    export CPPFLAGS

    LDFLAGS_SAVED="$LDFLAGS"
    LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
    export LDFLAGS

    AC_REQUIRE([AC_PROG_CXX])
    AC_LANG_PUSH(C++)
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    @%:@include <boost/version.hpp>
    ]], [[
    #if BOOST_VERSION >= $WANT_BOOST_VERSION
    // Everything is okay
    #else
    #  error Boost version is too old
    #endif
    ]])],[
        AC_MSG_RESULT(yes)
        ifelse([$2], , :, [$2])
        ],[
        AC_MSG_RESULT(no)
        ifelse([$3], , :, [$3])
        ])
    AC_LANG_POP([C++])
    CPPFLAGS="$CPPFLAGS_SAVED"
    LDFLAGS="$LDFLAGS_SAVED"
fi

])
