AC_DEFUN([AX_WINDOWS_OR_PTHREAD],
[
AC_ARG_WITH([windows-version],
	    [AS_HELP_STRING([--with-windows-version=0x0***],
             [windows version to build for (default=0x0500 (windows 2000))])],
	     [windows_version="$withval"] ,
	     [windows_version='0x0500']
	    )

AC_CACHE_CHECK(whether Windows build, ax_cv_windowsbuild,
           [AS_IF( [ expr "$host_os"  : ".*mingw" > /dev/null ],
                [
                ax_cv_windowsbuild=yes
                CFLAGS_THREAD="$CFLAGS"
                CXXFLAGS_THREAD="$CXXFLAGS"
                LDFLAGS_THREAD="$LDFLAGS"
                CFLAGS="$CFLAGS -mthreads"
                CXXFLAGS="$CXXFLAGS -mthreads"
                LDFLAGS="$LDFLAGS -mthreads "
                CPPFLAGS="$CPPFLAGS -D_WIN32_WINNT=$windows_version -DWINVER=$windows_version"
                AX_CXX_LT_LINK_IFELSE([AC_LANG_PROGRAM([[ ]], [[]])],
                    [need_mthreads=yes], [need_mthreads=no])
                AS_IF( [ test x$need_mthreads = xno ], [CXXFLAGS="$CXXFLAGS_THREAD"; LDFLAGS="$LDFLAGS_THREAD" ])

                AC_LANG_PUSH([C])
                ac_link="libtool --mode=link --tag=C $ac_link"
                AC_LINK_IFELSE([AC_LANG_PROGRAM([[ ]], [[]])],
                    [need_mthreads=yes], [need_mthreads=no])
                rmdir .libs
                AC_LANG_POP([C])
                AS_IF( [ test x$need_mthreads = xno ], [CFLAGS="$CFLAGS_THREAD"])
                AS_IF( [ test x$host_vendor = xw64 ], [CPPFLAGS="$CPPFLAGS -D__MINGW64__ " ])
                ],
                [
                ax_cv_windowsbuild=no;
                ACX_PTHREAD
                ]
                )
           AM_CONDITIONAL([WIN_PLATFORM], [[test x$ax_cv_windowsbuild = xyes] ])
           ]
      )

])
