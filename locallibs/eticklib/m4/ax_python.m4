dnl @synopsis AX_PYTHON
dnl
dnl This macro does a complete Python development environment check.
dnl
dnl It recurses through several python versions (from 2.1 to 2.4 in
dnl this version), looking for an executable. When it finds an
dnl executable, it looks to find the header files and library.
dnl
dnl It sets PYTHON_BIN to the name of the python executable,
dnl PYTHON_INCLUDE_DIR to the directory holding the header files, and
dnl PYTHON_LIB to the name of the Python library.
dnl
dnl This macro calls AC_SUBST on PYTHON_BIN (via AC_CHECK_PROG),
dnl PYTHON_INCLUDE_DIR and PYTHON_LIB.
dnl
dnl @category InstalledPackages
dnl @author Michael Tindal <mtindal@paradoxpoint.com>
dnl @version 2004-09-20
dnl @license GPLWithACException

AC_DEFUN([AX_PYTHON],
[AC_MSG_CHECKING(for python build information)
AC_MSG_RESULT([])
        AC_ARG_WITH([python],
        AS_HELP_STRING(
                   [--with-python@<:@=DIR@:>@],
                   [use this python installation path]
        ),
        [
        if test "$withval" = "no"; then
            want_python="no"
            ax_python_install="no"
        else
            want_python="yes"
            ax_python_install="$withval"
        fi
        ],
        [want_python="yes"]
        )

get_python_prefix(){
    [$]1 << PYPREFIXEOF
from distutils import sysconfig
print sysconfig.PREFIX
PYPREFIXEOF
}

set_python_vars(){
    unset ax_py_include
    local PY_BIN=[$]1
    ax_py_expref=`$PY_BIN << PYEXECPREFIXEOF
from distutils import sysconfig
print sysconfig.EXEC_PREFIX
PYEXECPREFIXEOF
`
    ax_py_include=`$PY_BIN << PYINCLUDEEOF
from distutils import sysconfig
print ' -I' + sysconfig.get_python_inc() + ' -I' + sysconfig.get_python_inc(plat_specific=True)
PYINCLUDEEOF
`
    ax_py_libs=`$PY_BIN << PYLIBSEOF
from distutils import sysconfig
print ' -lpython'+ sysconfig.get_config_var('VERSION')
PYLIBSEOF
`
ax_py_libs_full="-L"`get_python_prefix $PY_BIN`"/lib ""$ax_py_libs"

}

OLDCFLAGS=$CFLAGS
OLDLDFLAGS=$LDFLAGS

if test "$want_python" != "no" ; then

    if test "x$ax_python_install" != "x" ; then

        if test -d "$ax_python_install/bin" ;then
        for python in $(find "$ax_python_install/bin" -maxdepth 1 "(" -type f -o  -type  l ")"  -name "python*" -regex ".*python[[^- ]]*" -perm /111 | sort --reverse) ; do
	    set_python_vars "$python"
	    if test -n "$ax_py_include" ; then
		ax_python_bin=$python
                ax_python_header=yes
	        ax_python_lib=yes
		break;
	    fi
        done
        elif test -f "$ax_python_install" ;then
            ax_python_bin=$ax_python_install
            set_python_vars "$ax_python_bin"
            ax_python_header=yes
            ax_python_lib=yes
        else
	    AC_MSG_ERROR(Python not found on path $ax_python_install/bin )
        fi

    else
        for python in python2.7 python2.6 python2.5 python2.4 python2.3 python2.2 python2.1 python; do
            AC_CHECK_PROGS(PYTHON_BIN, [$python])
            ax_python_bin=$PYTHON_BIN
            if test "x$ax_python_bin" != "x"; then
                set_python_vars $ax_python_bin
                CFLAGS="$ax_py_include"
                LDFLAGS="$ax_py_libs"
		        export CFLAGS
		        export LDFLAGS
                AC_CHECK_LIB($ax_python_bin, main, ax_python_lib=$ax_python_bin, ax_python_lib=no)
                AC_CHECK_HEADER([$ax_python_bin/Python.h],
                    [[ax_python_header="$ax_python_bin/Python.h"]],
                    ax_python_header=no)
		        CFLAGS=$OLDCFLAGS
		        LDFLAGS=$OLDLDFLAGS
		        export CFLAGS
		        export LDFLAGS
                if test "$ax_python_lib" != "no"; then
                    if test "$ax_python_header" != "no"; then
                        break;
                    fi
                fi
            fi
        done
    fi

fi

if test "x$ax_python_bin" = "x"; then
   ax_python_bin=no
fi
if test "x$ax_python_header" = "x"; then
   ax_python_header=no
else
   ax_python_header="$ax_py_include"
   PYTHON_INCLUDE_DIR="$ax_python_header"
   AC_SUBST(PYTHON_INCLUDE_DIR)
fi
if test "x$ax_python_lib" = "x"; then
   ax_python_lib=no
else
   ax_python_lib="$ax_py_libs_full"
   PYTHON_LIB="$ax_python_lib"
   AC_SUBST(PYTHON_LIB)
fi

AC_MSG_RESULT([  results of the Python check:])
AC_MSG_RESULT([    Binary:      $ax_python_bin])
AC_MSG_RESULT([    Library:     $ax_python_lib])
AC_MSG_RESULT([    Include: $ax_python_header])

])dnl
