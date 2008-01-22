dnl mrj
dnl check if we have "tclmon" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_TCLMON],
[
  AC_MSG_CHECKING([for tclmon])

  dnl the default
  have_tclmon=yes
  TCLMON=""
  AC_ARG_WITH(tclmon,
  [  --with-tclmon=DIR       enable support for "tclmon" (default $(HOME)/work/tclmon)],
  [
    if test $withval = no; then
      have_tclmon=no
    elif test $withval != yes; then
      TCLMON=$withval
    fi
  ], )
  if test "x$TCLMON" = "x"; then
  	for d in  $HOME/tclmon ../tclmon ../../tclmon ; do
  	if test -d $d; then
  		TCLMON=`(cd $d; pwd)`
  		break
  	fi
  	done
  fi
  tclmon_cflags="-I $TCLMON"
  tclmon_ldflags="-L $TCLMON"
  tclmon_lib="$TCLMON/tclmonlib.a"

  AC_SUBST(TCLMON_CXXFLAGS, $tclmon_cflags)
  AC_SUBST(TCLMON_LDFLAGS, $tclmon_ldflags)
  AC_SUBST(TCLMON_LIBS, $tclmon_lib)
  AC_SUBST(TCLMON_PATH, $TCLMON)


  if test ! -d $TCLMON; then
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile tclmon module])
  else
	enable_tclmon=yes
	AC_MSG_RESULT(yes)
  fi
])


