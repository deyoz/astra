AC_DEFUN([AX_MONGO_CLIENT],
[
   AC_ARG_WITH([mongo_client],
      AS_HELP_STRING([--with-mongo_client=DIR],
      [use mongo client (default is yes) specify the root directory for mongo client library]),
      [
      if test $withval = "no"; then
         want_mongo_client="no"
      elif test $withval = "yes"; then
         want_mongo_client="yes"
         ax_user_libmongo_client=""
      else
         want_mongo_client="yes"
         ax_user_libmongo_client="$withval"
      fi
      ],
      [want_mongo_client="yes"])

   if test "x$want_mongo_client" = "xyes"; then
      AC_REQUIRE([AC_PROG_CC])
      if test "x$ax_user_libmongo_client" != "x"; then
         LIBMONGO_CLIENT_CPPFLAGS="-I$ax_user_libmongo_client/include"
         LIBMONGO_CLIENT_LIBS="-L$ax_user_libmongo_client/lib"
      fi
      LIBMONGO_CLIENT_LIBS="$LIBMONGO_CLIENT_LIBS -lmongoclient"
      CPPFLAGS_SAVED="$CPPFLAGS"
      CPPFLAGS="$CPPFLAGS_SAVED $LIBMONGO_CLIENT_CPPFLAGS"
      export CPPFLAGS

      LDFLAGS_SAVED="$LDFLAGS"
      LDFLAGS="$LDFLAGS $BOOST_LD_FLAGS $BOOST_SYSTEM_LIB $LIBMONGO_CLIENT_LIBS"
      export LDFLAGS
      AC_CACHE_CHECK(whether the mongo client library is available, ax_cv_libmongo_client,
            [AX_CXX_LT_LINK_IFELSE(
               [AC_LANG_PROGRAM([[@%:@include <mongo/client/dbclient.h>]],
               [[mongo::client::initialize();
               return 0;]])],
               ax_cv_libmongo_client=yes, ax_cv_libmongo_client=no)]
      )

      if test "x$ax_cv_libmongo_client" = "xyes"; then
         AC_SUBST(LIBMONGO_CLIENT_CPPFLAGS)
         AC_SUBST(LIBMONGO_CLIENT_LIBS)
         AC_DEFINE(HAVE_LIBMONGO_CLIENT,,[define if the mongo client library is available])
      fi

      CPPFLAGS="$CPPFLAGS_SAVED"
      LDFLAGS="$LDFLAGS_SAVED"
   fi
])
