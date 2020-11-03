dnl mrj
dnl check if we have "typebparser" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_TYPEBPARSER],
[
  AC_MSG_CHECKING([for typebparser])

  dnl the default
  have_typebparser=yes
  TYPEBPARSER=""
  AC_ARG_WITH(typebparser,
  [  --with-typebparser =DIR       enable support for typebparser (default $(LIBROOT)/typebparser)],
  [
    if test $withval = no; then
      have_typebparser=no
    elif test $withval != yes; then
      TYPEBPARSER=$withval
    fi
  ], )

  if test "x$TYPEBPARSER" = "x"; then
  	for d in  $LIBROOT/typebparser ../typebparser ../../typebparser ; do
  	if test -d $d; then
  		TYPEBPARSER=`(cd $d; pwd)`
  		break
  	fi
  	done
  elif ! test -d "$TYPEBPARSER"; then
  	AC_MSG_RESULT(no)
  	AC_MSG_ERROR([Wrong typebparser directiry name])
  	TYPEBPARSER="";
  fi

  typebparser_cflags="$($TYPEBPARSER/typebparser-config --cflags)"
  typebparser_libs="$($TYPEBPARSER/typebparser-config --libs)"
  typebparser_path="$TYPEBPARSER"

  AC_SUBST(TYPEBPARSER_CXXFLAGS, $typebparser_cflags)
  AC_SUBST(TYPEBPARSER_LIBS, $typebparser_libs)
  AC_SUBST(TYPEBPARSER_PATH, $typebparser_path)


  if test "x$TYPEBPARSER" != "x"; then
  	enable_typebparser=yes
	AC_MSG_RESULT(yes)
	echo typebparser=$TYPEBPARSER
  else
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile typebparser module])
  fi
])


