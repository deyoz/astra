dnl mrj
dnl check if we have "libtlg" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_LIBTLG],
[
  AC_MSG_CHECKING([for libtlg])

  dnl the default
  have_libtlg=yes
  LIBTLG=""
  AC_ARG_WITH(libtlg,
  [  --with-libtlg=DIR       enable support for "libtlg" (default $(LIBROOT)/work/libtlg)],
  [
    if test $withval = no; then
      have_libtlg=no
    elif test $withval != yes; then
      LIBTLG=$withval
    fi
  ], )
  if test "x$LIBTLG" = "x"; then
  	for d in  $LIBROOT/libtlg ../libtlg ../../libtlg ; do
  	if test -d $d; then
  		LIBTLG=`(cd $d; pwd)`
  		break
  	fi
  	done
  fi
  
  libtlg_cflags="-I $LIBTLG/include"
  libtlg_ldflags=""
#"-L $LIBTLG/lib"
  libtlg_lib="$LIBTLG/src/libtlg.la"
  libtlg_path="$LIBTLG"

  AC_SUBST(LIBTLG_CXXFLAGS, $libtlg_cflags)
  AC_SUBST(LIBTLG_LDFLAGS, $libtlg_ldflags)
  AC_SUBST(LIBTLG_LIBS, $libtlg_lib)
  AC_SUBST(LIBTLG_PATH, $libtlg_path)


  if test ! -d $LIBTLG; then
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile libtlg module])
  else
	enable_libtlg=yes
	AC_MSG_RESULT(yes)
  fi
])


