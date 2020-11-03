dnl mrj
dnl check if we have "edilib" then test it. otherwise, RESULT no.
AC_DEFUN([AX_EDILIB_INSTALLED],
[
  AC_MSG_CHECKING([for edilib])

  dnl the default
  have_edilib=yes
  EDILIB=""
  AC_ARG_WITH(edilib,
  [  --with-edilib=DIR       enable support for "edilib"],
  [
    if test $withval = no; then
      have_edilib=no
    elif test $withval != yes; then
      EDILIB=$withval
    fi
  ], )

  edilib_cflags="-I $EDILIB/include"
  edilib_lib="$EDILIB/lib/libedifact.la"
  
  if test "x$EDILIB" != "x"; then
     AC_SUBST(EDILIB_CXXFLAGS,   $edilib_cflags)
     AC_SUBST(EDILIB_LIBS,       $edilib_lib)
     AC_DEFINE(HAVE_EDILIB,,[define if the edilib library is available])
  fi


  if test ! -d $EDILIB; then
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile edilib module])
  else
	enable_edilib=yes
	AC_MSG_RESULT(yes)
  fi
])


