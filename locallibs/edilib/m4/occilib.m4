AC_DEFUN([AX_OCCILIB],
[
        AC_ARG_WITH([occilib],
        AS_HELP_STRING(
                   [--with-occilib@<:@=DIR@:>@],
                   [use the occilib - it is possible to specify root directory of the library]
        ),
        [
        if test "$withval" = "no"; then
            want_occilib="no"
        elif test "$withval" = "yes"; then
            want_occilib="yes"
            ax_user_occilib=""
        else
            want_occilib="yes"
            ax_user_occilib="$withval"
        fi
        ],
        [want_occilib="yes"]
        )

        if test "x$want_occilib" = "xyes"; then
          AC_REQUIRE([AC_PROG_CC])
          if test "x$ax_user_occilib" != "x"; then
            OCCILIB_CPPFLAGS="-I$ax_user_occilib/sdk/include"
            OCCILIB_LDFLAGS="-L$ax_user_occilib"
          fi
          OCCILIB_LDFLAGS="$OCCILIB_LDFLAGS -locci"
          CPPFLAGS_SAVED="$CPPFLAGS"
          CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS $OCCILIB_CPPFLAGS"
          export CPPFLAGS

          LDFLAGS_SAVED="$LDFLAGS"
          LDFLAGS="$LDFLAGS $BOOST_LDFLAGS $OCCILIB_LDFLAGS"
          export LDFLAGS

          AC_CACHE_CHECK(whether the OCCI library is available, ax_cv_occilib,
                [AX_CXX_LT_LINK_IFELSE(
                   [AC_LANG_PROGRAM([[@%:@include <occi.h>]],[[return 0;]])],
                   ax_cv_occilib=yes, ax_cv_occilib=no)]
          )

          if test "x$ax_cv_occilib" = "xyes"; then
                        AC_SUBST(OCCILIB_CPPFLAGS)
                        AC_SUBST(OCCILIB_LDFLAGS)
                        AC_DEFINE(HAVE_OCCILIB,,[define if the occi library is available])
          else
                        AC_MSG_ERROR(We could not detect the occi library or not symbol link to occi.so)
          fi

          CPPFLAGS="$CPPFLAGS_SAVED"
          LDFLAGS="$LDFLAGS_SAVED"
        fi
])

