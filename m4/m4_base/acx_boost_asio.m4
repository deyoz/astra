AC_DEFUN([ACX_BOOST_ASIO],
[
        AC_ARG_WITH([boost-asio],
        AS_HELP_STRING([--with-boost-asio@<:@=DIR@:>@],
                   [use the asio library from boost - it is possible to specify root directory of the library]),
        [
        if test "$withval" = "yes"; then
            want_boost="yes"
            ax_boost_user_asio=""
        else
            want_boost="yes"
            ax_boost_user_asio="$withval"
        fi
        ],
        [want_boost="yes"]
        )

        if test "x$want_boost" = "xyes"; then
        AC_REQUIRE([AC_PROG_CC])
                if test "$ax_boost_user_asio" != ""; then 
                   BOOST_ASIO_CPPFLAGS="-I$ax_boost_user_asio"
                fi
                CPPFLAGS_SAVED="$CPPFLAGS"
                CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS $BOOST_ASIO_CPPFLAGS"
                export CPPFLAGS

                LDFLAGS_SAVED="$LDFLAGS"
                LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
                export LDFLAGS

        AC_CACHE_CHECK(whether the boost::asio library is available, ax_cv_boost_asio,
        [AC_LANG_PUSH([C++])
                         AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/asio.hpp>
                                                                                                ]],
                                   [[return 0;]]),
                   ax_cv_boost_asio=yes, ax_cv_boost_asio=no)
         AC_LANG_POP([C++])
                ])
                if test "x$ax_cv_boost_asio" = "xyes"; then
                        AC_SUBST(BOOST_ASIO_CPPFLAGS)
                        AC_DEFINE(HAVE_BOOST_ASIO,,[define if the boost::asio library is available])
                else
                        AC_MSG_ERROR(We could not detect the boost::asio library)
                fi            

        CPPFLAGS="$CPPFLAGS_SAVED"
        LDFLAGS="$LDFLAGS_SAVED"
        fi
])

