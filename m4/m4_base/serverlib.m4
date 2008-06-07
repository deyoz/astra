dnl mrj
dnl check if we have "serverlib" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_SERVERLIB],
[
  AC_MSG_CHECKING([for serverlib])

  dnl the default
  have_serverlib=yes
  SERVERLIB=""
  AC_ARG_WITH(serverlib,
  [  --with-serverlib=DIR       enable support for "ServerLib" (default $(LIBROOT)/serverlib)],
  [
    if test $withval = no; then
      have_serverlib=no
    elif test $withval != yes; then
      SERVERLIB=$withval
    fi
  ], )

  if test "x$SERVERLIB" = "x"; then
  	for d in  $LIBROOT/serverlib ../serverlib ../../serverlib ; do
  	if test -d $d; then
  		SERVERLIB=`(cd $d; pwd)`
  		break
  	fi
  	done
  elif ! test -d "$SERVERLIB"; then
  	AC_MSG_RESULT(no)
  	AC_MSG_ERROR([Wrong "serverlib" directiry name])
  	SERVERLIB="";
  fi
  
  serverlib_cflags="-I $SERVERLIB"
  serverlib_ldflags="-L $SERVERLIB"
  serverlib_lib="$SERVERLIB/serverlib.a"
  serverlib_path="$SERVERLIB"

  AC_SUBST(SERVERLIB_CXXFLAGS, $serverlib_cflags)
  AC_SUBST(SERVERLIB_LDFLAGS, $serverlib_ldflags)
  AC_SUBST(SERVERLIB_LIBS, $serverlib_lib)
  AC_SUBST(SERVERLIB_PATH, $serverlib_path)


  if test "x$SERVERLIB" != "x"; then
  	enable_serverlib=yes
	AC_MSG_RESULT(yes)
	echo serverlib=$SERVERLIB
  else
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile serverlib module])
  fi
])


