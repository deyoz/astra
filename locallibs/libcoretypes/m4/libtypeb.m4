dnl mrj
dnl check if we have "edilib" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_LIBTYPEB],
[
  AC_MSG_CHECKING([for libtypeb])

  dnl the default
  have_libtypeb=yes
  LIBTYPEB=""
  AC_ARG_WITH(libtypeb,
  [  --with-libtypeb=DIR       enable support for "libtypeb" (default $(LIBROOT)/libtypeb)],
  [
    if test $withval = no; then
      have_libtypeb=no
    elif test $withval != yes; then
      LIBTYPEB=$withval
    fi
  ], )
  if test "x$LIBTYPEB" = "x"; then
  	for d in  $LIBROOT/libtypeb ../libtypeb ../../libtypeb ; do
  	if test -d $d; then
  		LIBTYPEB=`(cd $d; pwd)`
  		break
  	fi
  	done
  fi

  libtypeb_cflags="-I $LIBTYPEB/include"
  libtypeb_ldflags=""
#"-L $LIBTYPEB/lib"
  libtypeb_lib="$LIBTYPEB/src/libtypeb.la"
  libtypeb_path="$LIBTYPEB"

  AC_SUBST(LIBTYPEB_CXXFLAGS,   $libtypeb_cflags)
  AC_SUBST(LIBTYPEB_LDFLAGS,    $libtypeb_ldflags)
  AC_SUBST(LIBTYPEB_LIBS,       $libtypeb_lib)
  AC_SUBST(LIBTYPEB_PATH,       $libtypeb_path)


  if test ! -d $LIBTYPEB; then
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile libtypeb module])
  else
	enable_libtypeb=yes
	AC_MSG_RESULT(yes)
  fi
])


