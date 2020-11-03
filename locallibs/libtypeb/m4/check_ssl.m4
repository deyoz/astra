dnl @synopsis CHECK_SSL
dnl
dnl This macro will check various standard spots for OpenSSL including
dnl a user-supplied directory. The user uses '--with-ssl' or
dnl '--with-ssl=/path/to/ssl' as arguments to configure.
dnl
dnl If OpenSSL is found the include directory gets added to CPPFLAGS and
dnl CXXFLAGS as well as '-DHAVE_SSL', '-lssl' & '-lcrypto' get added to
dnl LIBS, and the libraries location gets added to LDFLAGS. Finally
dnl 'HAVE_SSL' gets set to 'yes' for use in your Makefile.in I use it
dnl like so (valid for gmake):
dnl
dnl     HAVE_SSL = @HAVE_SSL@
dnl     ifeq ($(HAVE_SSL),yes)
dnl         SRCS+= @srcdir@/my_file_that_needs_ssl.c
dnl     endif
dnl
dnl For bsd 'bmake' use:
dnl
dnl     .if ${HAVE_SSL} == "yes"
dnl         SRCS+= @srcdir@/my_file_that_needs_ssl.c
dnl     .endif
dnl
dnl @category InstalledPackages
dnl @author Mark Ethan Trostler <trostler@juniper.net>
dnl @version 2003-01-28
dnl @license AllPermissive
dnl
dnl using: CHECK_SSL
dnl or CHECK_SSL(<default value>)
dnl

AC_DEFUN([CHECK_SSL],
[AC_MSG_CHECKING(if ssl is wanted)
AS_IF( [test -z "$1"], [ with_ssl_val=no ], [with_ssl_val="$1"])
AC_ARG_WITH(ssl,
[  --with-ssl enable ssl [will check /usr/local/ssl
                            /usr/lib/ssl /usr/ssl /usr/pkg /usr/local /usr ]
], [  with_ssl_val="$withval" ])
AS_IF( [ test x$with_ssl_val != xno  ],
[   AC_MSG_RESULT(yes)
    AC_MSG_CHECKING(if ssl is present)

    LDFLAGS_SAVED="$LDFLAGS"
    CPPFLAGS_SAVED="$CPPFLAGS"
    for dir in "$with_ssl_val" /usr/local/ssl /usr/lib/ssl /usr/ssl /usr/pkg /usr/local /usr /usr/lib/i386-linux-gnu /usr/lib/x86_64-linux-gnu ; do
        ssldir="$dir"
       if test -f "$dir/include/openssl/ssl.h"; then
            found_ssl="yes";
            SSL_CPPFLAGS="-I$ssldir/include/openssl -I$ssldir/include";
            break;
        fi
        if test -f "$dir/include/ssl.h"; then
            found_ssl="yes";
            SSL_CPPFLAGS="-I$ssldir/include";
            break
        fi
    done
    if test x_$found_ssl != x_yes; then
        AC_MSG_RESULT(no)
        AC_MSG_ERROR(Cannot find ssl libraries)
    else
        AC_MSG_RESULT( [found in "$ssldir"]) ;
        CPPFLAGS="$CPPFLAGS $SSL_CPPFLAGS";
        SSL_LIBS="-lssl -lcrypto -lz -ldl";
        LDFLAGS="$LDFLAGS -L$ssldir/lib $SSL_LIBS"
        AC_CACHE_CHECK(wheather dl link is needed, ssl_cv_dl_needed,
   	    [
         AC_LANG_PUSH([C++])
      	 AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[return 0;]])],
                        ssl_cv_dl_needed="yes", ssl_cv_dl_needed="no")
   		 AC_LANG_POP([C++])
   	    ])
			
		if test "x$ssl_cv_dl_needed" != "xyes"; then
            SSL_LIBS="-lssl -lcrypto";
   	    fi

        WS2_LDFLAGS="-lws2_32 -lcrypt32"

        LDFLAGS="$LDFLAGS_SAVED -L$ssldir/lib $SSL_LIBS $WS2_LDFLAGS "
        
        AC_CACHE_CHECK(whether winsock2 link is needed, ssl_cv_ws2_needed,
   	    [
         AC_LANG_PUSH([C++])
      	 AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[return 0;]])],
                        ssl_cv_ws2_needed="yes", ssl_cv_ws2_needed="no")
   		 AC_LANG_POP([C++])
   	    ])

        AS_IF( [ test "x$ssl_cv_ws2_needed" = "xyes" ] , [
            SSL_LIBS="$SSL_LIBS $WS2_LDFLAGS";
   	    ])

        SSL_LIBS="-L$ssldir/lib $SSL_LIBS"
        HAVE_SSL=yes

        AC_CACHE_CHECK(whether redhat ssl version, ssl_cv_redhat,
	    [
            if test "x$ssl_cv_ws2_needed" = "xyes"; then
                ssl_cv_redhat="no"
            elif ldd $ssldir/lib/libssl.so* 2>&1 | grep 'krb5'2>&1 >/dev/null ; then
                 ssl_cv_redhat="yes"
            else
                 ssl_cv_redhat="no"
            fi
	    ])

    fi
    CPPFLAGS="$CPPFLAGS_SAVED"
    LDFLAGS="$LDFLAGS_SAVED"
    AC_DEFINE(HAVE_SSL,,[define if the ssl library is available])
    AC_SUBST(SSL_LIBS)
    AC_SUBST(SSL_CPPFLAGS)

# remove when unneeded
    SSL_CFLAGS="$SSL_CPPFLAGS"
    AC_SUBST(SSL_CFLAGS)
],
[
    AC_MSG_RESULT(no)
])

AM_CONDITIONAL([HAVE_SSL], [[test x$HAVE_SSL=xyes ] ])
AM_CONDITIONAL([SSL_REDHAT], [[test x$ssl_cv_redhat = xyes] ])
])dnl
