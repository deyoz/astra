dnl @synopsis AX_PION([MINIMUM-VERSION], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl
dnl @description  Tests for the Pion C++ libraries of a particular version (or newer)
dnl
dnl    This macro calls:
dnl
dnl      AC_SUBST(PION_CXXFLAGS) / AC_SUBST(PION_LDFLAGS) / AC_SUBST(PION_ARFLAGS)
dnl      AC_DEFINE(HAVE_PION)
dnl

AC_DEFUN([AX_PION],
[AC_MSG_CHECKING(for pion build information)
    AC_MSG_RESULT([])
    AC_ARG_WITH([pion],
              AS_HELP_STRING([--with-pion@<:@=DIR@:>@], [use this pion installation path]),
              [
                if test "$withval" = "no"; then
                    want_pion="no"
                    ax_pion_install="no"
                else
                    want_pion="yes"
                    ax_pion_install="$withval"
                fi
              ],
              [want_pion="yes"]
        )
    AC_CANONICAL_BUILD
    if test "x$want_pion" = "xyes" ; then
        AC_REQUIRE([AC_PROG_CXX])
        lib_version_req=ifelse([$1], , 0.0.1, $1)
        WANT_PION_VERSION=$lib_version_req
        AC_MSG_CHECKING(for pionlib >= $lib_version_req)

        if test "$ax_pion_install" != "" ; then
            PION_ARFLAGS="$ax_pion_install/lib/libpion.a"
            PION_LDFLAGS="-L$ax_pion_install/lib -lpion"
            PION_CXXFLAGS="-I$ax_pion_install/include"
        else
            for d in /usr /usr/local /opt ~/work/local ~/local ; do
                if test -d "$d/include/pion" && test -r "$d/include/pion"; then
                    PION_ARFLAGS="$d/lib/libpion.a"
                    PION_LDFLAGS="-L$d/lib -lpion"
                    PION_CXXFLAGS="-I$d/include"
                    break;
                fi
            done
        fi

        CPPFLAGS_SAVED="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS $PION_CXXFLAGS"
        export CPPFLAGS

        LDFLAGS_SAVED="$LDFLAGS"
        LDFLAGS="$LDFLAGS $PION_LDFLAGS"
        export LDFLAGS

        AC_TRY_RUN([
#include <pion/config.hpp>
#include <stdio.h>

int main()
{
  unsigned int wv_major=0, wv_minor=0, wv_micro=0;
  if(sscanf("$WANT_PION_VERSION", "%u.%u.%u", &wv_major, &wv_minor, &wv_micro) != 3)
  {
      fprintf(stderr,"%s bad wanted version string for pion\n", "$WANT_PION_VERSION");
      return 1;
  }

  unsigned int pv_major=0, pv_minor=0, pv_micro=0;
  if(sscanf(PION_VERSION, "%u.%u.%u", &pv_major, &pv_minor, &pv_micro) != 3)
  {
      fprintf(stderr,"%s bad wanted version string for pion\n", PION_VERSION);
      return 1;
  }

  if(wv_major>pv_major or wv_minor>pv_minor or wv_micro>pv_micro)
      return 1;
  else
      return 0;
}
        ],
        [
            AC_MSG_RESULT(yes)
            ifelse([$2], , :, [$2])
            AC_SUBST(PION_CXXFLAGS)
            AC_SUBST(PION_LDFLAGS)
            AC_SUBST(PION_ARFLAGS)
            AC_DEFINE(HAVE_PION,,[define if the Pion library is available])
        ],
        [
            AC_MSG_RESULT(no)
            ifelse([$3], , :, [$3])
        ])

        CPPFLAGS="$CPPFLAGS_SAVED"
        LDFLAGS="$LDFLAGS_SAVED"
    fi
])

