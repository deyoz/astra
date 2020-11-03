dnl mrj
dnl check if we have "libavs" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_LIBAVS],
[
  AC_MSG_CHECKING([for libavs])

  dnl the default
  have_libavs=yes
  LIBAVS=""
  AC_ARG_WITH(libavs,
  [  --with-libavs=DIR       enable support for libavs (default $(LIBROOT)/libavs)],
  [
    if test $withval = no; then
      have_libavs=no
    elif test $withval != yes; then
      LIBAVS=$withval
    fi
  ], )

  if test "x$LIBAVS" = "x"; then
  	for d in  $LIBROOT/libavs ../libavs ../../libavs ; do
  	if test -d $d; then
  		LIBAVS=`(cd $d; pwd)`
  		break
  	fi
  	done
  elif ! test -d "$LIBAVS"; then
  	AC_MSG_RESULT(no)
  	AC_MSG_ERROR([Wrong libavs directiry name])
  	LIBAVS="";
  fi

  libavs_cflags="$($LIBAVS/libavs-config --cflags)"
  libavs_libs="$($LIBAVS/libavs-config --libs)"
  libavs_path="$LIBAVS"

  AC_SUBST(LIBAVS_CXXFLAGS, $libavs_cflags)
  AC_SUBST(LIBAVS_LIBS, $libavs_libs)
  AC_SUBST(LIBAVS_PATH, $libavs_path)


  if test "x$LIBAVS" != "x"; then
  	enable_libavs=yes
	AC_MSG_RESULT(yes)
	echo libavs=$LIBAVS
  else
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile libavs module])
  fi
])


