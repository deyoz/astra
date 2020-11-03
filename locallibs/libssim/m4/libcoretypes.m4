dnl mrj
dnl check if we have "libcoretypes" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_LIBCORETYPES],
[
  AC_MSG_CHECKING([for libcoretypes])

  dnl the default
  have_libcoretypes=yes
  LIBCORETYPES=""
  AC_ARG_WITH(libcoretypes,
  [  --with-libcoretypes=DIR       enable support for libcoretypes (default $(LIBROOT)/libcoretypes)],
  [
    if test $withval = no; then
      have_libcoretypes=no
    elif test $withval != yes; then
      LIBCORETYPES=$withval
    fi
  ], )

  if test "x$LIBCORETYPES" = "x"; then
  	for d in  $LIBROOT/libcoretypes ../libcoretypes ../../libcoretypes ; do
  	if test -d $d; then
  		LIBCORETYPES=`(cd $d; pwd)`
  		break
  	fi
  	done
  elif ! test -d "$LIBCORETYPES"; then
  	AC_MSG_RESULT(no)
  	AC_MSG_ERROR([Wrong libcoretypes directiry name])
  	LIBCORETYPES="";
  fi

  libcoretypes_cflags="$($LIBCORETYPES/libcoretypes-config --cflags)"
  libcoretypes_libs="$($LIBCORETYPES/libcoretypes-config --libs)"
  libcoretypes_path="$LIBCORETYPES"

  AC_SUBST(LIBCORETYPES_CXXFLAGS, $libcoretypes_cflags)
  AC_SUBST(LIBCORETYPES_LIBS, $libcoretypes_libs)
  AC_SUBST(LIBCORETYPES_PATH, $libcoretypes_path)


  if test "x$LIBCORETYPES" != "x"; then
  	enable_libcoretypes=yes
	AC_MSG_RESULT(yes)
	echo libcoretypes=$LIBCORETYPES
  else
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile libcoretypes module])
  fi
])


