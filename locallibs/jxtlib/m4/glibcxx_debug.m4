AC_DEFUN([MRJ_CHECK_GLIBCXX_DEBUG],
[
  AC_MSG_CHECKING([for GLIBCXX_DEBUG])

  AC_ARG_ENABLE(
      [glibcxx-debug],
      AC_HELP_STRING([--enable-glibcxx-debug],[Enable GLIBCXX_DEBUG]),
      [ 
            if test x$enable_glibcxx_debug = xyes; then 
                      GLIBCXX_CFLAGS="-D_GLIBCXX_DEBUG" 
                      CFLAGS="$CFLAGS $GLIBCXX_CFLAGS"
                      AC_SUBST(GLIBCXX_CFLAGS)
                      AC_MSG_RESULT(yes); 
            fi 
      ],
      [ AC_MSG_RESULT(no) ]
      )
])
