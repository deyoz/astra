AC_DEFUN([AX_JMSLIB],
[
        AC_ARG_WITH([jmslib],
        AS_HELP_STRING(
                   [--with-jmslib@<:@=DIR@:>@],
                   [use the java messaging service library - it is possible to specify root directory of the library]
        ),
        [
        if test "$withval" = "no"; then
            want_jmslib="no"
        elif test "$withval" = "yes"; then
            want_jmslib="yes"
            ax_user_jmslib=""
        else
            want_jmslib="yes"
            ax_user_jmslib="$withval"
        fi
        ],
        [want_jmslib="yes"]
        )

        if test "x$want_jmslib" = "xyes"; then
          AC_REQUIRE([AC_PROG_CC])
          if test "x$ax_user_jmslib" != "x"; then
            JMSLIB_CPPFLAGS="-I$ax_user_jmslib/include"
            JMSLIB_LDFLAGS="-L$ax_user_jmslib/lib -ljms -lpthread"
          fi
          if test "x$JMSLIB_CPPFLAGS" = "x" ; then
            for d in  $LIBROOT/libjms ../libjms ../../libjms ; do                     
            if test -d $d; then
                jmsdir=`(cd $d; pwd)`
                JMSLIB_CPPFLAGS="`$jmsdir/libjms-config --cflags`"
                JMSLIB_LDFLAGS="`$jmsdir/libjms-config --libs`"
                break                                                                    
            fi
            done
          fi                                        
          CPPFLAGS_SAVED="$CPPFLAGS"
          CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS $JMSLIB_CPPFLAGS"
          export CPPFLAGS

          LDFLAGS_SAVED="$LDFLAGS"
          LDFLAGS="$LDFLAGS $BOOST_LDFLAGS $BOOST_SYSTEM_LIB $JMSLIB_LDFLAGS"
          export LDFLAGS

          AC_CACHE_CHECK(whether the java messaging service library is available, ax_cv_jmslib,
                [AX_CXX_LT_LINK_IFELSE(
                   [AC_LANG_PROGRAM([[@%:@include <jms/jms.hpp>]],[[jms::connection c({},true);]])],
                   ax_cv_jmslib=yes, ax_cv_jmslib=no)]
          )

          if test "x$ax_cv_jmslib" = "xyes"; then
                        AC_SUBST(JMSLIB_CPPFLAGS)
                        AC_SUBST(JMSLIB_LDFLAGS)
                        AC_SUBST(LIBJMS_LIBS,$JMSLIB_LDFLAGS)
                        AC_DEFINE(HAVE_JMSLIB,,[define if the java messaging service library is available])
          else
                        AC_MSG_ERROR(We could not detect the java messaging service library)
          fi

          CPPFLAGS="$CPPFLAGS_SAVED"
          LDFLAGS="$LDFLAGS_SAVED"
        fi
])

