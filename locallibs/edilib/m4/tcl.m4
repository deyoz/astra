dnl mrj
dnl check if we have "tcl" then test it. otherwise, RESULT no.
AC_DEFUN([CHECK_TCL],
[
  AC_MSG_CHECKING([for tcl])

  dnl the default
  need_tcl=yes
  TCLPATH=""
  AC_ARG_WITH(tcl,
  [  --with-tcl=DIR       directory to "Tcl" ],
  [
    if test $withval = no; then
      have_tcl=no
      need_tcl=no
    elif test $withval != yes; then
      TCLPATH=$withval
    fi
  ], )

lib_sub_dir="lib"
sys_arch=`uname -m`
if test $sys_arch = x86_64 -o $sys_arch = ppc64 -o $sys_arch = s390x -o $sys_arch = sparc64 && test -d /usr/lib64; then
  if ! echo $CFLAGS | grep -w "\-m32" > /dev/null ; then
   lib_sub_dir="lib64"
  fi
fi

if test "$need_tcl" = yes ; then
  for v in 8.2 8.3 8.4 8.5 8.6 ; do 
    for d in $TCLPATH /usr /usr/local /usr/gnu ; do
        if [[ -d $d/$lib_sub_dir/tcl$v ]] ; then
            LDL=""
            LDL=-L$d/$lib_sub_dir
            TCL_SYS_LIB="$LDL -ltcl$v -lm"
            TCLINCLUDE="-I$d/include/tcl$v"
            if [[ ! -d $d/include/tcl$v ]] ; then
               TCLINCLUDE="-I$d/include"
            fi 
            TCLSH=$d/bin/tclsh$v
            have_tcl=yes
        fi
    done
  done
  if test "$have_tcl" != yes; then
    lib_sub_dir=lib
    for v in 8.2 8.3 8.4 8.5 8.6; do 
      for d in $TCLPATH /usr /usr/local /usr/gnu ; do
          if [[ -d $d/$lib_sub_dir/tcl$v ]] ; then
              LDL=""
              LDL=-L$d/$lib_sub_dir
              TCL_SYS_LIB="$LDL -ltcl$v -lm"
              TCLINCLUDE="-I$d/include/tcl$v"
              if [[ ! -d $d/include/tcl$v ]] ; then
                 TCLINCLUDE="-I$d/include"
              fi 
              TCLSH=$d/bin/tclsh$v
              have_tcl=yes
          fi
      done
    done
  fi

  tcllib_cflags="$TCLINCLUDE"
  tcllib_ldflags="$TCL_SYS_LIB"
  AC_SUBST(TCLSH)
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


