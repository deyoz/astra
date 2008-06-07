AC_DEFUN([AX_LOGGER],
[
   AC_ARG_WITH([liblog],
      AS_HELP_STRING([--with-liblog@<:@=DIR@:>@], [specify the lib directory of the logger]),
      [
         if test "$withval" = "no"; then
         	want_liblog="no"
         elif test "$withval" = "yes"; then
            want_liblog="yes"
            ax_user_liblog=""
         else
            want_liblog="yes"
            ax_user_liblog="$withval"
         fi
      ],
      [want_liblog="yes"]
   )
   
   if test "x$want_liblog" = "xyes"; then
      AC_REQUIRE([AC_PROG_CC])
      if test "$ax_user_liblog" != ""; then
            LIBLOG_CPPFLAGS="-I$ax_user_liblog/include"
            LIBLOG_LDFLAGS="-L$ax_user_liblog/lib"
      fi
      LIBLOG_LDFLAGS="$LIBLOG_LDFLAGS -llog"
      CPPFLAGS_SAVED="$CPPFLAGS"
      CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS $BOOST_ASIO_CPPFLAGS $LIBLOG_CPPFLAGS"
      export CPPFLAGS

      LDFLAGS_SAVED="$LDFLAGS"
      LDFLAGS="$LDFLAGS $BOOST_LDFLAGS $LIBLOG_LDFLAGS"
      export LDFLAGS
      
      AC_CACHE_CHECK(whether logging library is available, ax_cv_liblog,
           [AC_LANG_PUSH([C++])
            AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <log.hpp>]], [[return 0;]]),
                                 ax_cv_liblog=yes, ax_cv_liblog=no)
            AC_LANG_POP([C++])
           ]
      )

      if test "x$ax_cv_liblog" = "xyes"; then
         AC_SUBST(LIBLOG_CPPFLAGS)
         AC_SUBST(LIBLOG_LDFLAGS)
         AC_DEFINE(HAVE_LIBLOG,,[define if the logger is available])
      else
         AC_MSG_ERROR(Nothing about logger where is it?)
      fi

      CPPFLAGS="$CPPFLAGS_SAVED"
      LDFLAGS="$LDFLAGS_SAVED"
   fi
])