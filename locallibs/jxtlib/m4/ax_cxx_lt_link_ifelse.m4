AC_DEFUN([AX_CXX_LT_LINK_IFELSE],
[
AC_LANG_PUSH([C++])
ac_link="libtool --mode=link --tag=CXX $ac_link"
AC_LINK_IFELSE([$1],[$2],[$3])
rmdir .libs
AC_LANG_POP([C++])
])
