AC_DEFUN([AX_STPLIB],
[
        AC_ARG_WITH([stplib],
        AS_HELP_STRING(
                   [--with-stplib@<:@=DIR@:>@],
                   [use the sirena protocol library - it is possible to specify root directory of the library]
        ),
        [
        if test "$withval" = "no"; then
            want_stplib="no"
        elif test "$withval" = "yes"; then
            want_stplib="yes"
            ax_user_stplib=""
        else
            want_stplib="yes"
            ax_user_stplib="$withval"
        fi
        ],
        [want_stplib="yes"]
        )

        if test "x$want_stplib" = "xyes"; then
          AC_REQUIRE([AC_PROG_CC])
          if test "$ax_user_stplib" != ""; then
            STPLIB_CPPFLAGS="-I$ax_user_stplib/include"
            STPLIB_LDFLAGS="-L$ax_user_stplib/lib"
          fi
          STPLIB_LDFLAGS="$STPLIB_LDFLAGS -lstp"
          CPPFLAGS_SAVED="$CPPFLAGS"
          CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS $ASIO_CPPFLAGS $STPLIB_CPPFLAGS"
          export CPPFLAGS

          LDFLAGS_SAVED="$LDFLAGS"
          LDFLAGS="$LDFLAGS $BOOST_LDFLAGS $STPLIB_LDFLAGS"
          export LDFLAGS

          AC_CACHE_CHECK(whether the sirena protocol library is available, ax_cv_stplib,
                [
                 AC_LANG_PUSH([C++])
                 AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <STP/sirena_server.hpp>]],
                                   [[return 0;]]),
                   ax_cv_stplib=yes, ax_cv_stplib=no)
                 AC_LANG_POP([C++])
                ]
          )
          
          if test "x$ax_cv_stplib" = "xyes"; then
                        AC_SUBST(STPLIB_CPPFLAGS)
                        AC_SUBST(STPLIB_LDFLAGS)
                        AC_DEFINE(HAVE_STPLIB,,[define if the sirena protocol library is available])
          else
                        AC_MSG_ERROR(We could not detect the sirena protocol library)
          fi            

          CPPFLAGS="$CPPFLAGS_SAVED"
          LDFLAGS="$LDFLAGS_SAVED"
        fi
])