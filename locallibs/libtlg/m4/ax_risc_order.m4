dnl This macro calls AC_SUBST(RISC_ORDER_CFLAGS) and sets RISC_ORDER

AC_DEFUN([AX_RISC_ORDER],
[
    AC_MSG_CHECKING([for risc order])
    AC_TRY_RUN(
        [#include <stdio.h>
        int main(void)
        {
            short a = 1;
            if (*((char*)&a) == 0)
            {
                return -1;
            }
            return 0;
        }
        ],
        [AC_MSG_RESULT(no)],
        [
        if test $? = -1; then 
            AC_MSG_RESULT(yes)
            RISC_ORDER_CFLAGS="-DRISC_ORDER"
            AC_SUBST(RISC_ORDER_CFLAGS)
            AC_DEFINE(RISC_ORDER, , [define if risc order])
        else
            AC_MSG_ERROR(config error. unexpected error)
        fi
        ]
    )
])

