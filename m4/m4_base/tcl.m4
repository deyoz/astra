dnl mrj
dnl check if we have "tcl" then test it. otherwise, RESULT no.
AC_DEFUN([CHECK_TCL],
[
  AC_MSG_CHECKING([for tcl])

  dnl the default
  have_tcl=yes
  TCLPATH=""
  AC_ARG_WITH(tcl,
  [  --with-tcl=DIR       directory to "Tcl" ],
  [
    if test $withval = no; then
      have_tcl=no
    elif test $withval != yes; then
      TCLPATH=$withval
    fi
  ], )

if test "$have_tcl" = yes ; then
  for v in 8.2 8.3 8.4  ; do 
    for d in $TCLPATH /usr /usr/local /usr/gnu ; do
        if [[ -d $d/lib/tcl$v -a -d $d/include/tcl$v ]] ; then
            LDL=""
            if [[ $d = /usr/local ]] ; then LDL="-L/usr/local/lib" ; fi
            TCL_SYS_LIB="$LDL -ltcl$v -lm"
            TCLINCLUDE="-I $d/include/tcl$v"
	    if [[ ! -d $d/include/tcl$v  -a $d = /usr/local ]] ; then
            	TCLINCLUDE="-I $d/include"
	    fi 
            TCLSH=$d/bin/tclsh$v
            have_tcl=yes
        fi
    done
  done

  
  tcllib_cflags="$TCLINCLUDE"
  tcllib_ldflags="$TCL_SYS_LIB"

  AC_SUBST(TCLLIB_CXXFLAGS, $tcllib_cflags)
  AC_SUBST(TCLLIB_LDFLAGS, $tcllib_ldflags)
fi

  if test "$have_tcl" = yes; then
  	enable_tcllib=yes
	AC_MSG_RESULT(yes)
  else
    	AC_MSG_RESULT(no)
    	AC_MSG_ERROR([Tcl package needed])
  fi
])


