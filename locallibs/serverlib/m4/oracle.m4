dnl mrj
dnl check if we have oracle then test it. otherwise, RESULT no.
AC_DEFUN([MRJ_CHECK_ORACLE],
[
  AC_MSG_CHECKING([for oracle])

  dnl the default
  have_oracle=yes

  AC_ARG_WITH(oracle,
  [  --with-oracle=DIR       enable support for Oracle (default ORACLE_HOME)],
  [
    if test $withval = no; then
      have_oracle=no
    elif test $withval != yes; then
      ORACLE_HOME=$withval
    fi
  ], )

  oracle_user_inc=
  AC_ARG_WITH(oracle-includes,
  [  --with-oracle-includes=DIR
                          set oracle include dir (default ORACLE_HOME/subdirs)],
  [
    have_oracle=yes
    oracle_user_inc=$withval
    if ! test -d "$oracle_user_inc"; then
              AC_MSG_ERROR([$oracle_user_inc dir doesn't exist. Check --with-oracle-includes.])
    fi
  ], )

  oracle_user_lib=
  AC_ARG_WITH(oracle-libraries,
  [  --with-oracle-libraries=DIR
                          set oracle lib dir (default ORACLE_HOME/lib)],
  [
    have_oracle=yes
    oracle_user_lib=$withval
    if ! test -d "$oracle_user_lib"; then
              AC_MSG_ERROR([$oracle_user_lib dir doesn't exist. Check --with-oracle-libraries.])
    fi
  ], )

  oracle_user_otl_ver=
  AC_ARG_WITH(oci-version,
  [[  --with-oci-version=[8, 8I, 9I, 10G, 11G]
                          this is the version of the client, not the database.]],
  [
    have_oracle=yes
    oracle_user_otl_ver=$withval
  ], )


  if test "x$ORACLE_INSTANT" != "x"; then 
      oracle_user_instant=$ORACLE_INSTANT
  else
      oracle_user_instant=
  fi


  AC_ARG_WITH(instant-client,
  [[  --with-instant-client
                          define if compiling against Oracle Instant Client.
                          Disables testing for ORACLE_HOME and tnsnames.]],
  [
    have_oracle=yes
    oracle_user_instant=$withval
  ], )


  cflags_ora_save=$CFLAGS
  cxxflags_ora_save=$CXXFLAGS
  ldflags_ora_save=$LDFLAGS
  libs_ora_save=$LIBS

  ora_cflags=
  ora_libdir=
  ora_ldflags=
  ora_lib=-lclntsh

  AC_ARG_ENABLE([oracle-support],
    [
      AS_HELP_STRING([--enable-oracle-support],[Enable Oracle support @<:@default=no@:>@])
    ],
    [
      AS_CASE(
        ${enableval}, [yes], [], [no], [],
        [AC_MSG_ERROR([bad value ${enableval} for --enable-oracle-support])]
      )
      if test $enableval = no; then
        have_oracle=no
        enable_oracle_support=no
      elif test $enableval != yes; then
        enable_oracle_support=yes
      fi
    ],
    [
      enable_oracle_support=no
      have_oracle=no
    ]
  )


  if test $have_oracle = no; then
    dnl yeah, this is backwards.
    AC_DEFINE(TO_NO_ORACLE, 1, [Define if you do _not_ have Oracle.])
    AC_MSG_RESULT(no)
  elif test "x$oracle_user_instant" != "x"; then
    dnl user says we're running on the instant client libraries.
    AC_DEFINE(TO_INSTANT_CLIENT, 1, [Define if compiled against Oracle Instant Client])

    if test "x$oracle_user_lib" = "x" && test "x$oracle_user_inc" = "x"; then
      dnl try to find oracle includes for instant client
      dnl these are from the rpm install. they're all i know of so far.
      
      if test -d "$oracle_user_instant" ; then
          if test -d "$oracle_user_instant/sdk/include"; then
              ora_ldflags="-L$oracle_user_instant"
              ora_cflags="-I$oracle_user_instant/sdk/include"
          else
              AC_MSG_ERROR([$oracle_user_instant/sdk/include dir doesn't exist. Please use --with-oracle-includes.])
          fi
          AC_MSG_RESULT($oracle_user_instant)
      else
          AC_MSG_WARN([instant client dir not defined by --with-instant-client or not exists ])
          for dir in `ls /usr/lib/oracle/`; do
            echo "trying $dir" >&5
            if expr $dir \> 10 >/dev/null; then
              oracle_user_otl_ver=10G
            fi
            ora_ldflags="-L/usr/lib/oracle/$dir/client/lib"

            incdir=/usr/include/oracle/$dir/client
            if ! test -d $incdir; then
              AC_MSG_ERROR([$incdir doesn't exist. Please install the sdk package or use --with-oracle-includes.])
            fi
            ora_cflags="-I$incdir"
            break
          done
          AC_MSG_RESULT(ok)
      fi

    else
        ora_cflags="-I$oracle_user_inc"  
        ora_ldflags="-L$oracle_user_lib"


         if test "x$oracle_user_lib" = "x" || test "x$oracle_user_inc" = "x"; then 
             if test -d "$oracle_user_instant" ; then

                 if test "x$oracle_user_lib" = "x"; then 
                    ora_ldflags="-L$oracle_user_instant"
                 fi
                 if test "x$oracle_user_inc" = "x"; then 
                         ora_cflags="-I$oracle_user_instant/sdk/include"
                 fi
             else
                AC_MSG_WARN([instant client dir not defined by --with-instant-client or not exists ])                 
             fi
         fi
         if test "x$oracle_user_lib" = "x"; then
             AC_MSG_RESULT($oracle_user_instant)
         else
             AC_MSG_RESULT($oracle_user_lib)
         fi
    fi
  elif test "x$ORACLE_HOME" != "x"; then
    AC_MSG_RESULT($ORACLE_HOME)

    dnl try to find oracle includes
    ora_check_inc="
      $oracle_user_inc
      $ORACLE_HOME/rdbms/demo
      $ORACLE_HOME/plsql/public
      $ORACLE_HOME/rdbms/public
      $ORACLE_HOME/precomp/public
      $ORACLE_HOME/network/public
      $ORACLE_HOME/sdk/
      $ORACLE_HOME/include/"

    for dir in $ora_check_inc; do
      if test -d $dir; then
        ora_cflags="$ora_cflags -I$dir"
      fi
    done

    ora_check_lib="
      $oracle_user_lib
      $ORACLE_HOME/lib
      $ORACLE_HOME/lib32
      $ORACLE_HOME/lib64"

    for dir in $ora_check_lib; do
      if test -d $dir; then
        dnl ora_ldflags="$ora_ldflags -L$dir"
        ora_ldflags="$ora_ldflags -L$dir -Wl,-rpath,$dir"
      fi
    done
  else
    dnl test if we have includes or libraries
    if test -z "$oracle_user_lib" || test -z "$oracle_user_inc"; then
       AC_MSG_WARN(no)
       have_oracle=no
     else
      ora_ldflags="-L$oracle_user_lib"
      ora_cflags="-I$oracle_user_inc"
    fi
  fi

  if test "x$ORACLE_HOME" != "x"; then
    dnl check real quick that ORACLE_HOME doesn't end with a slash
    dnl for some stupid reason, the 10g instant client bombs.
    ora_home_oops=`echo $ORACLE_HOME | $AWK '/\/@S|@/ {print "oops"}'`
    if test "$ora_home_oops" = "oops"; then
      AC_MSG_WARN([Your ORACLE_HOME environment variable ends with a
slash (i.e. /). Oracle 10g Instant Client is known to have a problem
with this. If you get the message "otl_initialize failed!" at the
console when running TOra, this is probably why.])
    fi
  fi

  if test $have_oracle = yes; then
    CFLAGS="$CFLAGS $ora_cflags"
    LDFLAGS="$LDFLAGS $ora_ldflags"
    CXXFLAGS="$CFLAGS"
    LIBS="$ora_lib"
    if test "x$oracle_user_lib" != "x"; then
        LDFLAGS="$LDFLAGS -Wl,-rpath,$oracle_user_lib"
    fi

    AC_CACHE_CHECK(for Lda_Def, found_oracle_cv_, [
    found_oracle_cv_=no
    AC_TRY_LINK([#include <oci.h>],
      [Lda_Def lda;
       ub4     hda [HDA_SIZE/(sizeof(ub4))];
      ;],
      found_oracle_cv_=yes)
    ])

    if test $found_oracle_cv_ = no; then
      AC_MSG_ERROR([Couldn't compile and run a simpile OCI app.
      Try setting ORACLE_HOME or check config.log.
      Otherwise, make sure ORACLE_HOME/lib is in /etc/ld.so.conf or LD_LIBRARY_PATH])
    fi

    sqlplus=

    if test -x "$oracle_user_instant/sqlplus"; then
      sqlplus="$oracle_user_instant/sqlplus"
    elif test -x "$ORACLE_HOME/bin/sqlplus"; then
      sqlplus="$ORACLE_HOME/bin/sqlplus"
    fi
    if test "x${sqlplus}" = "x"; then
      if test -x "$ORACLE_HOME/bin/sqlplusO"; then
        sqlplus="$ORACLE_HOME/bin/sqlplusO"
      fi
    fi

    if test "x$oracle_user_otl_ver" != "x"; then
      otl_ver=$oracle_user_otl_ver
    elif test "x${sqlplus}" = "x"; then
      AC_MSG_ERROR([Couldn't find sqlplus. Set the Oracle version manually.])
    else
      # get oracle oci version. know a better way?
      sqlplus_ver=`$sqlplus -? | $AWK '/Release/ {print @S|@3}'`
      echo "sqlplus_ver: $sqlplus_ver" >&5

      if expr $sqlplus_ver \> 12 >/dev/null; then
        otl_ver=12G
      elif expr $sqlplus_ver \> 11 >/dev/null; then
        otl_ver=11G
      elif expr $sqlplus_ver \> 10 >/dev/null; then
        dnl our version of otl doesn't have 10g defined yet
        otl_ver=10G
      elif expr $sqlplus_ver \> 9 >/dev/null; then
        otl_ver=9I
      elif expr $sqlplus_ver \< 8.1 >/dev/null; then
        otl_ver=8
      else
        otl_ver=8I
      fi
    fi

    echo "Oracle Instant Client Version is $otl_ver" >&5

    ora_cflags="$ora_cflags"

    # don't change flags for all targets, just export ORA variables.
    CFLAGS=$cflags_ora_save
    CXXFLAGS=$cxxflags_ora_save
    AC_SUBST(ORACLE_CXXFLAGS, $ora_cflags)
    AC_SUBST(ORACLE_CPPFLAGS, $ora_cflags)

    LDFLAGS=$ldflags_ora_save
    AC_SUBST(ORACLE_LDFLAGS, $ora_ldflags)

    LIBS=$libs_ora_save
    AC_SUBST(ORACLE_LIBS, $ora_lib)
    # AM_CONDITIONAL in configure.in uses this variable to enable oracle
    # targets.
    enable_oracle=yes
  fi
])

