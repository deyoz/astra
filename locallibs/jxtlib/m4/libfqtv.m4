dnl mrj
dnl check if we have "libfqtv" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_LIBFQTV],
[
  AC_MSG_CHECKING([for libfqtv])

  dnl the default
  have_libfqtv=yes
  LIBFQTV=""
  AC_ARG_WITH(libfqtv,
  [  --with-libfqtv=DIR       enable support for libfqtv (default $(LIBROOT)/libfqtv)],
  [
    if test $withval = no; then
      have_libfqtv=no
    elif test $withval != yes; then
      LIBFQTV=$withval
    fi
  ], )

  if test "x$LIBFQTV" = "x"; then
  	for d in  $LIBROOT/libfqtv ../libfqtv ../../libfqtv ; do
  	if test -d $d; then
  		LIBFQTV=`(cd $d; pwd)`
  		break
  	fi
  	done
  elif ! test -d "$LIBFQTV"; then
  	AC_MSG_RESULT(no)
  	AC_MSG_ERROR([Wrong libfqtv directiry name])
  	LIBFQTV="";
  fi

  libfqtv_cflags="$($LIBFQTV/libfqtv-config --cflags)"
  libfqtv_libs="$($LIBFQTV/libfqtv-config --libs)"
  libfqtv_path="$LIBFQTV"

  AC_SUBST(LIBFQTV_CXXFLAGS, $libfqtv_cflags)
  AC_SUBST(LIBFQTV_LIBS, $libfqtv_libs)
  AC_SUBST(LIBFQTV_PATH, $libfqtv_path)


  if test "x$LIBFQTV" != "x"; then
  	enable_libfqtv=yes
	AC_MSG_RESULT(yes)
	echo libfqtv=$LIBFQTV
  else
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile libfqtv module])
  fi
])


