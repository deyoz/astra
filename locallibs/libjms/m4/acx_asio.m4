AC_DEFUN([ACX_ASIO],
[
        AC_ARG_WITH([asio],
        AS_HELP_STRING([--with-asio@<:@=DIR@:>@],
                   [use the asio library - it is possible to specify root directory of the library]),
        [
        if test "$withval" = "yes"; then
            want_asio="yes"
            ax_user_asio=""
        else
            want_asio="yes"
            ax_user_asio="$withval"
        fi
        ],
        [want_asio="yes"]
        )

        if test "x$want_asio" = "xyes"; then
        AC_REQUIRE([AC_PROG_CC])
                if test "$ax_user_asio" != ""; then 
                   ASIO_CPPFLAGS="-I$ax_user_asio"
                fi
                CPPFLAGS_SAVED="$CPPFLAGS"
                CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS $ASIO_CPPFLAGS"
                export CPPFLAGS

                LDFLAGS_SAVED="$LDFLAGS"
                LDFLAGS="$LDFLAGS $LDFLAGS"
                export LDFLAGS

        AC_CACHE_CHECK(whether the asio library is available, ax_cv_asio,
        [AC_LANG_PUSH([C++])
                         AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <asio.hpp>
                                                                                                ]],
                                   [[return 0;]]),
                   ax_cv_asio=yes, ax_cv_asio=no)
         AC_LANG_POP([C++])
                ])
                if test "x$ax_cv_asio" = "xyes"; then
                        AC_SUBST(ASIO_CPPFLAGS)
                        AC_DEFINE(HAVE_ASIO,,[define if the asio library is available])
                else
                        AC_MSG_ERROR(We could not detect the asio library)
                fi            

        CPPFLAGS="$CPPFLAGS_SAVED"
        LDFLAGS="$LDFLAGS_SAVED"
        fi
])

