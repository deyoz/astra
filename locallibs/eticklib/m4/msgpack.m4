dnl mrj
dnl check if we have "msgpack" then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_MSGPACK],
[
  AC_MSG_CHECKING([for msgpack])

  dnl the default
  have_msgpack=yes
  MSGPACK=""
  AC_ARG_WITH(msgpack,
  [  --with-msgpack=DIR       enable support for "MsgPack" (default $(LIBROOT))],
  [
    if test $withval = no; then
      have_msgpack=no
    elif test $withval != yes; then
      MSGPACK=$withval
    fi
  ], )

  if test "x$MSGPACK" = "x"; then
  	for d in  /usr/local/include/msgpack  ; do
  	if test -d $d; then
  		MSGPACK="/usr/local"
  		break
  	fi
  	done
  elif ! test -d "$MSGPACK"; then
  	AC_MSG_RESULT(no)
  	AC_MSG_ERROR([Wrong "msgpack" directiry name])
  	MSGPACK="";
  fi

  msgpack_cflags=" -I $MSGPACK/include/msgpack"
  msgpack_libs=" -L$MSGPACK/lib -lmsgpack"
  msgpack_path="$MSGPACK"

  AC_SUBST(MSGPACK_CXXFLAGS, $msgpack_cflags)
  AC_SUBST(MSGPACK_LIBS, $msgpack_libs)
  AC_SUBST(MSGPACK_PATH, $msgpack_path)

  if test "x$MSGPACK" != "x"; then
  	enable_msgpack=yes
	AC_MSG_RESULT(yes)
	echo msgpack=$MSGPACK
  else
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Checkout and compile msgpack module])
  fi
])


