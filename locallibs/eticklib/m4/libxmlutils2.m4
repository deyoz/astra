# Defines: XMLUTILS_LIBS and XMLUTILS_CPPFLAGS

AC_DEFUN([AX_XMLUTILS2],
[
   AM_PATH_XML2(2.0.0, [AC_MSG_NOTICE($XML_LIBS)], [AC_MSG_ERROR([not found])])

   AC_ARG_WITH([xmlutils],
      AS_HELP_STRING([--with-xmlutils@<:@=DIR@:>@], [specify the lib directory of the xmlutils]),
      [
         if test "$withval" = "no"; then
            want_xmlutils="no"
         elif test "$withval" = "yes"; then
            want_xmlutils="yes"
            ax_user_xmlutils=""
         else
            want_xmlutils="yes"
            ax_user_xmlutils="$withval"
         fi
      ],
      [want_xmlutils="yes"]
   )

      if test "x$want_xmlutils" = "xyes"; then
      AC_REQUIRE([AC_PROG_CC])
      if test "$ax_user_xmlutils" != ""; then
            XMLUTILS_CFLAGS="-I$ax_user_xmlutils/include"
            XMLUTILS_LIBS="-L$ax_user_xmlutils/lib"

            CPPFLAGS_SAVED="$CPPFLAGS"
            CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS $XMLUTILS_CFLAGS"
            export CPPFLAGS

            LDFLAGS_SAVED="$LDFLAGS"
            XMLUTILS_LIBS="$XMLUTILS_LDFLAGS -lxmlutils"
            LDFLAGS="$LDFLAGS $BOOST_LDFLAGS $XMLUTILS_LIBS"
            export LDFLAGS

            AC_CACHE_CHECK(whether xmlutils library is available, ax_cv_xmlutils,
               [AX_CXX_LT_LINK_IFELSE([AC_LANG_PROGRAM([[@%:@include <xmlutils/helpers.hpp>]], [[return 0;]])],
                                 ax_cv_xmlutils=yes, ax_cv_xmlutils=no)]
            )


           if test "x$ax_cv_xmlutils" = "xyes"; then
               AC_SUBST(XMLUTILS_CFLAGS)
               AC_SUBST(XMLUTILS_LIBS)
               AC_DEFINE(HAVE_XMLUTILS,,[define if the xmlutils is available])
           else
               AC_MSG_ERROR(Nothing about xmlutils where is it?)
           fi

           CPPFLAGS="$CPPFLAGS_SAVED"
           LDFLAGS="$LDFLAGS_SAVED"
      else
         AC_DEFINE(HAVE_XMLUTILS,,[define if the xmlutils is available])
         PKG_CHECK_MODULES(XMLUTILS, xmlutils, [AC_MSG_NOTICE($XMLUTILS_LIBS)], [AC_MSG_ERROR([not found])])
      fi
   fi
])
