dnl mrj
dnl check if we have "libssim" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_LIBSSIM],
[
  AC_MSG_CHECKING([for libssim])

  dnl the default
  have_libssim=yes
  LIBSSIM=""
  AC_ARG_WITH(libssim,
  [  --with-libssim=DIR       enable support for libssim (default $(LIBROOT)/libssim)],
  [
    if test $withval = no; then
      have_libssim=no
    elif test $withval != yes; then
      LIBSSIM=$withval
    fi
  ], )

  if test "x$LIBSSIM" = "x"; then
  	for d in  $LIBROOT/libssim ../libssim ../../libssim ; do
  	if test -d $d; then
  		LIBSSIM=`(cd $d; pwd)`
  		break
  	fi
  	done
  elif ! test -d "$LIBSSIM"; then
  	AC_MSG_RESULT(no)
  	AC_MSG_ERROR([Wrong libssim directiry name])
  	LIBSSIM="";
  fi

  libssim_cflags="$($LIBSSIM/libssim-config --cflags)"
  libssim_libs="$($LIBSSIM/libssim-config --libs)"
  libssim_path="$LIBSSIM"

  AC_SUBST(LIBSSIM_CXXFLAGS, $libssim_cflags)
  AC_SUBST(LIBSSIM_LIBS, $libssim_libs)
  AC_SUBST(LIBSSIM_PATH, $libssim_path)


  if test "x$LIBSSIM" != "x"; then
  	enable_libssim=yes
	AC_MSG_RESULT(yes)
	echo libssim=$LIBSSIM
  else
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile libssim module])
  fi
])


