# Defines: LIBLOG_LDFLAGS and LIBLOG_CPPFLAGS

AC_DEFUN([AX_LOGGER_V2],
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
      LIBLOG_LDFLAGS="$LIBLOG_LDFLAGS -llog $BOOST_LOG_LIB"
      CPPFLAGS_SAVED="$CPPFLAGS"
      CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS $LIBLOG_CPPFLAGS"
      export CPPFLAGS

      LDFLAGS_SAVED="$LDFLAGS"
      LDFLAGS="$LDFLAGS $BOOST_LDFLAGS $BOOST_SYSTEM_LIB $BOOST_FILESYSTEM_LIB $BOOST_THREAD_LIB $BOOST_LOG_LIB $LIBLOG_LDFLAGS"
      export LDFLAGS

      AC_CACHE_CHECK(whether logging_v2 library is available, ax_cv_liblog_v2,
           [AX_CXX_LT_LINK_IFELSE([AC_LANG_PROGRAM([[@%:@include <log/log.hpp>]],
                                 [[@%:@include <boost/log/core.hpp>]],[[return 0;]])],
                                 ax_cv_liblog_v2=yes, ax_cv_liblog_v2=no)]
      )

      if test "x$ax_cv_liblog_v2" = "xyes"; then
         AC_SUBST(LIBLOG_CPPFLAGS)
         AC_SUBST(LIBLOG_LDFLAGS)
         AC_DEFINE(HAVE_LIBLOG,,[define if the logger is available])
         AC_DEFINE(LIBLOG_V2,,[define if the second version of logger is used])
      else
         AC_MSG_ERROR(Nothing about logger_v2 where is it?)
      fi

      CPPFLAGS="$CPPFLAGS_SAVED"
      LDFLAGS="$LDFLAGS_SAVED"
   fi
])
