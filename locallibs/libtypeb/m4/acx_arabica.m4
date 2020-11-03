# Defines: ARABICA_CFLAGS ARABICA_LIBS

AC_DEFUN([ACX_ARABICA],
        [

        AM_PATH_XML2(2.0.0, [AC_MSG_NOTICE($XML_LIBS)], [AC_MSG_ERROR([not found])])

        AC_ARG_WITH([arabica],
            AS_HELP_STRING([--with-arabica@<:@=DIR@:>@], [specify the lib directory of the arabica]),
            [
            if test "$withval" = "no"; then
                want_arabica="no"
            elif test "$withval" = "yes"; then
                want_arabica="yes"
                ax_user_arabica=""
            else
                want_arabica="yes"
                ax_user_arabica="$withval"
            fi
            ],
            [want_arabica="yes"]
            )

        if test "x$want_arabica" = "xyes"; then
            AC_REQUIRE([AC_PROG_CC])
            if test "$ax_user_arabica" != ""; then
                ARABICA_CFLAGS="-I$ax_user_arabica/include"
                ARABICA_LIBS="-L$ax_user_arabica/lib -larabica"


                CPPFLAGS_SAVED="$CPPFLAGS"
                CPPFLAGS="$CPPFLAGS $ARABICA_CFLAGS"
                export CPPFLAGS

                LDFLAGS_SAVED="$LDFLAGS"
                LDFLAGS="$LDFLAGS $ARABICA_LIBS"
                export LDFLAGS

                AC_CACHE_CHECK(whether Arabica library is available, ax_cv_arabica,
                                [
                                AC_LANG_PUSH([C++])
                                    AC_LINK_IFELSE([AC_LANG_PROGRAM([[@%:@include <XML/XMLCharacterClasses.hpp>]], [[return Arabica::XML::is_digit(L'1');]])],
                                        ax_cv_arabica=yes, ax_cv_arabica=no)
                                AC_LANG_POP([C++])
                                ]
                              )

                if test "x$ax_cv_arabica" = "xyes"; then
                    AC_SUBST(ARABICA_CFLAGS)
                    AC_SUBST(ARABICA_LIBS)
                    AC_DEFINE(HAVE_ARABICA,,[define if the arabica library is available])
                else
                    AC_MSG_ERROR(Arabica not found on specified location)
                fi

                CPPFLAGS="$CPPFLAGS_SAVED"
                LDFLAGS="$LDFLAGS_SAVED"
            else
                AC_DEFINE(HAVE_ARABICA,,[define if the arabica library is available])
                PKG_CHECK_MODULES(ARABICA, arabica, [AC_MSG_NOTICE($ARABICA_LIBS)],[AC_MSG_ERROR([not found])]) 
            fi
        fi
    ])
