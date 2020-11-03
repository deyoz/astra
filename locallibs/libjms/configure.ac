AC_INIT(jmscpp, 1.1, yuran@sirena2000.ru)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(src/aq.cpp)

AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

enable_xp_testing=no
#SERVERLIB_OPTIONAL=yes
#MRJ_CHECK_SERVERLIB
AS_IF([test "x$enable_serverlib" = xyes],[ 
    AC_DISABLE_SHARED
    # Switch off tests with static linking
    enable_xp_testing=no
    AC_DEFINE([JMS_SERVERLIB_LOGGING],[1],[Serverlib logger ON]) 
])    
AM_CONDITIONAL([HAVE_SERVERLIB], [test x$enable_serverlib = xyes])

AC_PROG_CXX
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_ARG_VAR(CPP_STD_VERSION, [Set c++11 or c++14 or c++17 or c++20])
AS_IF( [test x"$CPP_STD_VERSION" = x"c++17" ], [ AX_CXX_COMPILE_STDCXX(17,[noext],[mandatory]) ],
       [test x"$CPP_STD_VERSION" = x"c++14" ], [ AX_CXX_COMPILE_STDCXX(14,[noext],[mandatory]) ],
                                               [ AX_CXX_COMPILE_STDCXX(11,[noext],[mandatory]) ])

AS_IF( [ test -z "$NO_ASIO_SSL_INIT" -o "$NO_ASIO_SSL_INIT" = "no" -o "$NO_ASIO_SSL_INIT" = "false" -o "$NO_ASIO_SSL_INIT" = 0  ],
      [ env_enable_asio_ssl_init=yes ], [ env_enable_asio_ssl_init=no ])

AX_BOOST_BASE(1.56, , [AC_MSG_ERROR([Boost requirement])])
MRJ_CHECK_ORACLE

#AS_IF([test "x$enable_oracle" != "xyes"], [AC_MSG_ERROR([Oracle requirement])])
AS_IF([test "$enable_oracle" = "yes"], [
       AC_DEFINE([HAVE_ORACLE_AQ], [1], [Have oracle client library])
       have_oracle_aq=true ], [have_oracle_aq=false])
AM_CONDITIONAL([HAVE_ORACLE_AQ], ["$have_oracle_aq"])

AC_ARG_ENABLE([queue-listen],
    [  --disable-queue-listen       Disable AQ listen (for Oracle 9)],
    [
     AS_IF([test "$enableval" = "yes"], [ enable_queue_listen=yes], [enable_queue_listen=no]  )
    ])
AM_CONDITIONAL([NO_QUEUE_LISTEN], [test x$enable_queue_listen = xno])
AS_IF([test x$enable_queue_listen = xno] , [ AC_DEFINE([NO_QUEUE_LISTEN],[1],[no AQ listen support]) ])

AC_ARG_ENABLE([xp-testing],
    [  --enable-xp-testing       Enable xp testing],
    [
        AS_IF([test "$enableval" = "yes"], [ enable_xp_testing=yes], [enable_xp_testing=no]  )
    ])

AM_CONDITIONAL([XP_TESTING], [test x$enable_xp_testing = xyes])
AS_IF([test x$enable_xp_testing = xyes],[ AC_DEFINE([XP_TESTING],,[XP TESTING support]) ])

AC_ARG_ENABLE([debug-out],
    [  --enable-debug-out       Enable debug output],
    [
        AS_IF([test "$enableval" = "yes"],
              [ AC_DEFINE([JMS_STDERR_LOGGING],[1],[debug output]) ] 
              )
    ])

AC_ARG_ENABLE([mutex-try-lock-for-workaround],
    [  --enable-mutex-try-lock-for-workaround       Enable workaround for libstdc++ bug 54562 ],
    [
        AS_IF([test "$enableval" = "yes"],
              [ AC_DEFINE([STDLIBCXX_MUTEX_TRY_LOCK_FOR_WORKAROUND],[1],[Enable workaround for libstdc++ bug 54562]) ] 
              )
        AS_IF([test "$enableval" = "no"],
              [ AC_DEFINE([NO_STDLIBCXX_MUTEX_TRY_LOCK_FOR_WORKAROUND],[1],[Disable workaround for libstdc++ bug 54562]) ] 
              )
    ])

AC_ARG_ENABLE([examples],
    [  --enable-examples       Enable examples],
    [
        AS_IF([test "$enableval" = "yes"], [ enable_examples=yes], [enable_examples=no]  )
        AC_DEFINE([BUILD_EXAMPLES],,[examples support])
        ACX_PTHREAD([AC_MSG_NOTICE($PTHREAD_LIBS)], [AC_MSG_ERROR([not found])])
    ])
AM_CONDITIONAL([BUILD_EXAMPLES], [test x$enable_examples = xyes])

AX_PYTHON
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_SYSTEM
AX_BOOST_PYTHON
#
MRJ_CHECK_GLIBCXX_DEBUG

AM_CONDITIONAL([BUILD_AGAINST_PYTHON], [[test -n "$BOOST_PYTHON_LIB"] ])

PKG_CHECK_MODULES([ACTIVEMQ], [activemq-cpp], have_activemq=true, have_activemq=false)
AS_IF(["$have_activemq"], [ AC_DEFINE([HAVE_ACTIVEMQ], [1], [Have activemq library])])
AM_CONDITIONAL([HAVE_ACTIVEMQ], ["$have_activemq"])
PKG_CHECK_MODULES([AMQP_CPP], [amqp-cpp > 4], have_amqp_cpp=true, have_amqp_cpp=false)
AS_IF(["$have_amqp_cpp"], [
       AC_DEFINE([HAVE_AMQP_CPP], [1], [Have amqp-cpp library])
       jmslib_need_ssl=yes ] )
AM_CONDITIONAL([HAVE_AMQP_CPP], ["$have_amqp_cpp"])

AS_IF([ "$have_activemq" || "$have_amqp_cpp" || "$have_oracle_aq" ],
      [], [AC_MSG_ERROR([Can't build any backend])])

CHECK_SSL("$jmslib_need_ssl")

AC_ARG_ENABLE(asio-ssl-init, [ Is openssl init should be performed inside Boost::asio [default=yes] ],, enable_asio_ssl_init=$env_enable_asio_ssl_init )

AS_IF([ test "x$HAVE_SSL" = "xyes" ], [
    AS_IF([ test "x$enable_asio_ssl_init" = "xno" ], [
           AC_DEFINE([BOOST_ASIO_NO_OPENSSL_STATIC_INIT], , [define if disabling of Boost::asio openssl init is needed ])
           AC_MSG_RESULT([Boost::Asio internal openssl initialization disabled])
           ], [
           AC_MSG_RESULT([Boost::Asio internal openssl initialization enabled])
           ]
          ) ] )



AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile examples/Makefile libjms-conf])
AC_SUBST(MY_LOCAL_CFLAGS, ["$MY_LOCAL_CFLAGS"])
CFLAGS="$CFLAGS $ALLWARN $MY_LOCAL_CFLAGS"
AC_OUTPUT

echo 
echo Configuration for $PACKAGE $VERSION :
echo --------------------------------
echo
echo Source code location ....: $srcdir
echo Host System Type ........: $host
echo CFLAGS  .................: $CFLAGS
echo CXXFLAGS  ...............: $CXXFLAGS
if test x$enable_xp_testing = xyes; then
    echo XP_TESTING enabled
fi
echo Now type 'make' to build $PACKAGE $VERSION,
echo
    
