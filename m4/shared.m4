dnl mrj
dnl check if we have "sharedlib" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_SHARED],
[
  AC_MSG_CHECKING([for shared])

  dnl the default
  have_shared=yes
  SHARED=""
  AC_ARG_WITH(shared,
  [  --with-shared=DIR       enable support for "shared lib" (default $(LIBROOT)/work/shared)],
  [
    if test $withval = no; then
      have_shared=no
    elif test $withval != yes; then
      SHARED=$withval
    fi
  ], )
  if test "x$SHARED" = "x"; then
  	for d in  $LIBROOT/shared ../shared ../../shared ; do
  	if test -d $d; then
  		SHARED=`(cd $d; pwd)`
  		break
  	fi
  	done
  fi
  
  shared_cflags="-I$SHARED/src"
  shared_ldflags="-L$SHARED/src"
  shared_lib="-lshared"

  AC_SUBST(SHARED_CXXFLAGS, $shared_cflags)
  AC_SUBST(SHARED_LDFLAGS, $shared_ldflags)
  AC_SUBST(SHARED_LIBS, $shared_lib)
  AC_SUBST(SHARED_PATH, $SHARED)


  if test ! -d $SHARED; then
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile shared module])
  else
	enable_shared=yes
	AC_MSG_RESULT(yes)
  fi
])


