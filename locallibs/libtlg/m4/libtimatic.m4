dnl mrj
dnl check if we have "libtimatic" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_LIBTIMATIC],
[
  AC_MSG_CHECKING([for libtimatic])

  dnl the default
  have_libtimatic=yes
  LIBTIMATIC=""

  if test "x$LIBTIMATIC" = "x"; then
  	for d in  $LIBROOT/libtimatic ../libtimatic ../../libtimatic ; do
  	if test -d $d; then
  		LIBTIMATIC=`(cd $d; pwd)`
  		break
  	fi
  	done
  elif ! test -d "$LIBTIMATIC"; then
  	AC_MSG_RESULT(no)
  	AC_MSG_ERROR([Wrong "libtimatic" directory name])
  	LIBTIMATIC="";
  fi

  if test "x$LIBTIMATIC" != "x"; then
	libtimatic_cflags="$($LIBTIMATIC/libtimatic-config --cflags)"
	libtimatic_cxxflags="$($LIBTIMATIC/libtimatic-config --cxxflags)"
	libtimatic_libs="$($LIBTIMATIC/libtimatic-config --libs)"
	libtimatic_path="$LIBTIMATIC"
  fi	

  AC_SUBST(LIBTIMATIC_CFLAGS, $libtimatic_cflags)
  AC_SUBST(LIBTIMATIC_CXXFLAGS, $libtimatic_cxxflags)
  AC_SUBST(LIBTIMATIC_LIBS, $libtimatic_libs)
  AC_SUBST(LIBTIMATIC_PATH, $libtimatic_path)


  if test "x$LIBTIMATIC" != "x"; then
  	enable_libtimatic=yes
	AC_MSG_RESULT(yes)
	echo libtimatic=$LIBTIMATIC
  else
   	AC_MSG_RESULT(no)
	if test "x$LIBTIMATIC_OPTIONAL" != "xyes"; then
		AC_MSG_ERROR([Checkout and compile libtimatic module])            
	else  
		AC_MSG_NOTICE([Checkout and compile libtimatic module: result = no])
    fi       
  fi
])


