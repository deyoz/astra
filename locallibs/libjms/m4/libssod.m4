dnl mrj
dnl check if we have "libssod" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_LIBSSOD],
[
  AC_MSG_CHECKING([for libssod])

  dnl the default
  have_libssod=yes
  LIBSSOD=""
  AC_ARG_WITH(libssod,
  [  --with-libssod=DIR       enable support for libssod (default $(LIBROOT)/libssod)],
  [
    if test $withval = no; then
      have_libssod=no
    elif test $withval != yes; then
      LIBSSOD=$withval
    fi
  ], )

  if test "x$LIBSSOD" = "x"; then
  	for d in  $LIBROOT/libssod ../libssod ../../libssod ; do
  	if test -d $d; then
  		LIBSSOD=`(cd $d; pwd)`
  		break
  	fi
  	done
  elif ! test -d "$LIBSSOD"; then
  	AC_MSG_RESULT(no)
  	AC_MSG_ERROR([Wrong libssod directiry name])
  	LIBSSOD="";
  fi

  libssod_cflags="$($LIBSSOD/libssod-config --cflags)"
  libssod_libs="$($LIBSSOD/libssod-config --libs)"
  libssod_path="$LIBSSOD"

  AC_SUBST(LIBSSOD_CXXFLAGS, $libssod_cflags)
  AC_SUBST(LIBSSOD_LIBS, $libssod_libs)
  AC_SUBST(LIBSSOD_PATH, $libssod_path)


  if test "x$LIBSSOD" != "x"; then
  	enable_libssod=yes
	AC_MSG_RESULT(yes)
	echo libssod=$LIBSSOD
  else
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile libssod module])
  fi
])


