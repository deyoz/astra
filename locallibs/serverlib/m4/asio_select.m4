AC_DEFUN([AX_ASIO_SELECT],
[

AC_ARG_ENABLE(boost-asio, [  --enable-boost-asio          use Asio from boost [default=no]],, enable_boost_asio=no)
if test "x$enable_boost_asio" = "xyes";
then
AC_MSG_RESULT(checking BOOST_ASIO library)
AX_BOOST_BASE(1.38, , [AC_MSG_ERROR([boost not found])])
AX_BOOST_SYSTEM
AX_BOOST_ASIO
else
AC_MSG_RESULT(checking ASIO library)
ACX_ASIO
fi
AM_CONDITIONAL(HAVE_BOOST_ASIO, test "x$enable_boost_asio" = "xyes")

])
