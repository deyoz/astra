# ===========================================================================
#      http://www.gnu.org/software/autoconf-archive/ax_boost_python.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_BOOST_PYTHON
#
# DESCRIPTION
#
#   This macro checks to see if the Boost.Python library is installed. It
#   also attempts to guess the currect library name using several attempts.
#   It tries to build the library name using a user supplied name or suffix
#   and then just the raw library.
#
#   If the library is found, HAVE_BOOST_PYTHON is defined and
#   BOOST_PYTHON_LIB is set to the name of the library.
#
#   This macro calls AC_SUBST(BOOST_PYTHON_LIB).
#
#   In order to ensure that the Python headers are specified on the include
#   path, this macro requires AX_PYTHON to be called.
#
# LICENSE
#
#   Copyright (c) 2008 Michael Tindal
#
#   This program is free software; you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation; either version 2 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <http://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Archive. When you make and distribute a
#   modified version of the Autoconf Macro, you may extend this special
#   exception to the GPL to apply to your modified version as well.

#serial 11

AC_DEFUN([AX_BOOST_PYTHON],
[AC_REQUIRE([AX_PYTHON])dnl
AC_CACHE_CHECK(whether the Boost::Python library is available,
ac_cv_boost_python,
[AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 CPPFLAGS_SAVE=$CPPFLAGS
 if test "x$PYTHON_INCLUDE_DIR" != "x"; then
   CPPFLAGS="$PYTHON_INCLUDE_DIR $CPPFLAGS $BOOST_CPPFLAGS"
 fi
 export CPPFLAGS
 LDFLAGS_SAVED="$LDFLAGS"
 LDFLAGS="$PYTHON_LIB $LDFLAGS $BOOST_LDFLAGS"
 export LDFLAGS


 AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
 #include <boost/python/module.hpp>
 using namespace boost::python;
 BOOST_PYTHON_MODULE(test) { throw "Boost::Python test."; }]],
			   [[return 0;]])],
			   ac_cv_boost_python=yes, ac_cv_boost_python=no)
 AC_LANG_RESTORE
])

if test "$ac_cv_boost_python" = "yes"; then
  AC_DEFINE(HAVE_BOOST_PYTHON,,[define if the Boost::Python library is available])
  AC_LANG_CPLUSPLUS
  ax_python_lib=boost_python
  AC_ARG_WITH([boost-python],AS_HELP_STRING([--with-boost-python],[specify the boost python library or suffix to use]),
  [if test "x$with_boost_python" != "xno"; then
     ax_python_lib=$with_boost_python
     ax_boost_python_lib=boost_python-$with_boost_python
   fi])
  for ax_lib in $ax_python_lib $ax_boost_python_lib boost_python; do
    AC_CHECK_LIB($ax_lib, exit, [BOOST_PYTHON_LIB=-l$ax_lib break])
  done
  AC_SUBST(BOOST_PYTHON_LIB)
fi

CPPFLAGS=$CPPFLAGS_SAVE
LDFLAGS=$LDFLAGS_SAVE
export CPPFLAGS
export LDFLAGS

AC_MSG_RESULT([  results of the Boost::Python check:])
AC_MSG_RESULT([    Library: $BOOST_PYTHON_LIB])


])dnl
