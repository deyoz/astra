AC_DEFUN([AX_WCHAR_SIZE],
[
    AC_MSG_CHECKING(is wchar_size == 4)
    AC_LANG_PUSH(C++)
    AC_COMPILE_IFELSE(
            [AC_LANG_PROGRAM(
[[@%:@include <wchar.h>]],
[[#if WCHAR_MAX == 2147483647
    //All is ok
#else
    blablabla
#endif]]
)],
    [wchar_t_wide_size=yes],
    [wchar_t_wide_size=no])
    AC_LANG_POP(C++)
    if test $wchar_t_wide_size = no; then
        AC_DEFINE([LOCALE_WCHAR_T_2], [], [wchar_t size == 2])
    fi
    AC_MSG_RESULT($wchar_t_wide_size)
]) 
