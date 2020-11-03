AC_DEFUN([CHECK_ETICKLIB],
[
  AC_MSG_CHECKING([for eticklib])

  have_eticklib=yes
  ETICKLIB=""
  AC_ARG_WITH(eticklib,
  [  --with-eticklib=DIR       enable support for "eticklib" (default $(LIBROOT)/eticklib)],
  [
    if test $withval = no; then
      have_etick=no
    elif test $withval != yes; then
      ETICKLIB=$withval
    fi
  ], )
  if test "x$ETICKLIB" = "x"; then
  	for d in  $LIBROOT/eticklib ../eticklib ../../eticklib ; do
  	if test -d $d; then
  		ETICKLIB=`(cd $d; pwd)`
  		break
  	fi
  	done
  fi
  
  eticklib_cflags="-I $ETICKLIB/include"
  eticklib_ldflags="-L $ETICKLIB/src/.libs"
  eticklib_lib="$ETICKLIB/src/libetick.la"

  AC_SUBST(ETICKLIB_CXXFLAGS, $eticklib_cflags)
  AC_SUBST(ETICKLIB_LDFLAGS, $eticklib_ldflags)
  AC_SUBST(ETICKLIB_LIBS, $eticklib_lib)


  if test ! -d $ETICKLIB; then
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile eticklib module])
  else
	enable_ticklib=yes
	AC_MSG_RESULT(yes)
  fi
])


