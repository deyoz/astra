dnl mrj
dnl check if we have "edilib" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_EDILIB],
[
  AC_MSG_CHECKING([for edilib])

  dnl the default
  have_edilib=yes
  EDILIB=""
  AC_ARG_WITH(edilib,
  [  --with-edilib=DIR       enable support for "edilib" (default $(LIBROOT)/edilib)],
  [
    if test $withval = no; then
      have_edilib=no
    elif test $withval != yes; then
      EDILIB=$withval
    fi
  ], )
  if test "x$EDILIB" = "x"; then
  	for d in  $LIBROOT/edilib ../edilib ../../edilib ; do
  	if test -d $d; then
  		EDILIB=`(cd $d; pwd)`
  		break
  	fi
  	done
  fi
  
  edilib_cflags="-I $EDILIB/include"
  edilib_ldflags=""
#"-L $EDILIB/lib"
  edilib_lib="$EDILIB/src/libedifact.la $EDILIB/src/loading/libediloading.la"
  edilib_path="$EDILIB"

  AC_SUBST(EDILIB_CXXFLAGS, $edilib_cflags)
  AC_SUBST(EDILIB_LDFLAGS, $edilib_ldflags)
  AC_SUBST(EDILIB_LIBS, $edilib_lib)
  AC_SUBST(EDILIB_PATH, $edilib_path)


  if test ! -d $EDILIB; then
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile edilib module])
  else
	enable_edilib=yes
	AC_MSG_RESULT(yes)
  fi
])


