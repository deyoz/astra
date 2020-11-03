dnl mrj
dnl check if we have "libnsi" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_LIBNSI],
[
  AC_MSG_CHECKING([for libnsi])

  dnl the default
  have_libnsi=yes
  LIBNSI=""
  AC_ARG_WITH(libnsi,
  [  --with-libnsi=DIR       enable support for libnsi (default $(LIBROOT)/libnsi)],
  [
    if test $withval = no; then
      have_libnsi=no
    elif test $withval != yes; then
      LIBNSI=$withval
    fi
  ], )

  if test "x$LIBNSI" = "x"; then
  	for d in  $LIBROOT/libnsi ../libnsi ../../libnsi ; do
  	if test -d $d; then
  		LIBNSI=`(cd $d; pwd)`
  		break
  	fi
  	done
  elif ! test -d "$LIBNSI"; then
  	AC_MSG_RESULT(no)
  	AC_MSG_ERROR([Wrong libnsi directiry name])
  	LIBNSI="";
  fi

  libnsi_cflags="$($LIBNSI/libnsi-config --cflags)"
  libnsi_libs="$($LIBNSI/libnsi-config --libs)"
  libnsi_path="$LIBNSI"

  AC_SUBST(LIBNSI_CXXFLAGS, $libnsi_cflags)
  AC_SUBST(LIBNSI_LIBS, $libnsi_libs)
  AC_SUBST(LIBNSI_PATH, $libnsi_path)


  if test "x$LIBNSI" != "x"; then
  	enable_libnsi=yes
	AC_MSG_RESULT(yes)
	echo libnsi=$LIBNSI
  else
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile libnsi module])
  fi
])


