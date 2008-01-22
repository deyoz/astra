dnl mrj
dnl check if we have "jxtlib" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_JXTLIB],
[
  AC_MSG_CHECKING([for jxtlib])

  dnl the default
  have_jxtlib=yes
  JXTLIB=""
  AC_ARG_WITH(jxtlib,
  [  --with-jxtlib=DIR       enable support for "jxtlib" (default $(LIBROOT)/work/jxtlib)],
  [
    if test $withval = no; then
      have_jxtlib=no
    elif test $withval != yes; then
      JXTLIB=$withval
    fi
  ], )
  if test "x$JXTLIB" = "x"; then
  	for d in  $LIBROOT/jxtlib ../jxtlib ../../jxtlib ; do
  	if test -d $d; then
  		JXTLIB=`(cd $d; pwd)`
  		break
  	fi
  	done
  fi
  
  jxtlib_cflags="-I $JXTLIB"
  jxtlib_ldflags="-L $JXTLIB"
  jxtlib_lib="$JXTLIB/jxtlib.a"

  AC_SUBST(JXTLIB_CXXFLAGS, $jxtlib_cflags)
  AC_SUBST(JXTLIB_LDFLAGS, $jxtlib_ldflags)
  AC_SUBST(JXTLIB_LIBS, $jxtlib_lib)
  AC_SUBST(JXTLIB_PATH, $JXTLIB)


  if test ! -d $JXTLIB; then
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile jxtlib module])
  else
	enable_jxtlib=yes
	AC_MSG_RESULT(yes)
  fi
])


