dnl Checking for socket
AC_DEFUN([MRJ_CHECK_SOCKET_T],
[
AC_MSG_CHECKING([socket length])
CFLAGS_SAVE="$CFLAGS"
CXXFLAGS_SAVE="$CXXFLAGS"

for i in socklen_t size_t int ; do
CFLAGS="$CFLAGS_SAVE -D SIRENA_SOCKLEN_T_=$i"
CXXFLAGS="$CXXFLAGS_SAVE -D SIRENA_SOCKLEN_T_=$i"
    AC_TRY_COMPILE([#include <sys/types.h> 
    		    #include <sys/socket.h> 
    		    #include <netinet/in.h>
    		    #include <arpa/inet.h>],
		[recvfrom(0,0,0,0,0,(SIRENA_SOCKLEN_T_*)0)]
    , [found_socklen=yes SIRENA_SOCKLEN_T_=$i break], [found_socklen=no])
done
CFLAGS="$CFLAGS_SAVE"
CXXFLAGS="$CXXFLAGS_SAVE"

if test "$found_socklen" = no; then
      AC_MSG_ERROR([Couldn't compile and run a simpile recvfrom app. See config.log for details!])
fi
AC_DEFINE_UNQUOTED(SIRENA_SOCKLEN_T_, $SIRENA_SOCKLEN_T_, [socket length])
AC_MSG_RESULT([$SIRENA_SOCKLEN_T_])
])
