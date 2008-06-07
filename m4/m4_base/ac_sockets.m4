AC_DEFUN([AC_SOCKETS],
[
	SOCKET_LDFLAGS="-lwsock32 -lws2_32"
	LDFLAGS_SAVED="$LDFLAGS"
	LDFLAGS="$LDFLAGS $SOCKET_LDFLAGS"
	export LDFLAGS
	AC_CACHE_CHECK(wheather winsocks are needed, win_socks,
   [AC_LANG_PUSH([C++])
   	AC_LINK_IFELSE(AC_LANG_PROGRAM([[]], [[return 0;]]), 
   							win_socks="yes", win_socks="no")
   AC_LANG_POP([C++])
   ])
	if test "x$win_socks" = "xyes"; then
		AC_MSG_NOTICE([Seems like Windows])
	else
		AC_MSG_NOTICE([Seems like Linux])
		SOCKET_LDFLAGS=""
	fi
	LDFLAGS="$LDFLAGS_SAVED"
	AC_SUBST(SOCKET_LDFLAGS)
])
