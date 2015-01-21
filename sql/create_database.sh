#!/bin/sh -e

if [ -z "$CONNECT_STRING" ];
then
    export CONNECT_STRING=$1
fi

if [ -z "$CONNECT_STRING" -a -n "$TOP_SRCDIR" ];
then
    . $TOP_SRCDIR/connection.mk
    export CONNECT_STRING
fi

if [ -z "$CONNECT_STRING" ];
then
    echo variable CONNECT_STRING not found
    exit 1
fi

SYSPAROL=${SYSPAROL:-system/manager}
if sqlplus /nolog </dev/null 2>&1 | grep     'Release 9' ; then
    SYSCMD="connect $SYSPAROL as sysdba"
    SYSPAROL=/nolog
    SPECIAL_GRANT1="grant select on sys.v_\$timer to public;"
    SPECIAL_GRANT2="grant execute on sys.dbms_pipe to public;"
fi

ORAPASS=`echo $CONNECT_STRING | sed -s 's/@.*//'`
user=`echo ${ORAPASS} | sed -e 's/\/.*//'`
password=`echo ${ORAPASS} | sed -e 's/.*\///'`

sqlplus ${SYSPAROL} <<EOF
${SYSCMD:-""}
drop user $user cascade;
create user $user identified by $password;
alter user $user default tablespace USERS;
alter system set open_cursors=2000 scope=both;
grant create any view to $user;
grant resource to $user;
grant create session, create table, create view, create procedure, create synonym to $user;
grant create materialized view, create sequence, create trigger to $user;
${SPECIAL_GRANT1:-""}
${SPECIAL_GRANT2:-""}
EOF

baseDir=`pwd`
for dir in `ls | LC_ALL="C" grep '^[0-9][A-Za-z].*' | sort`; do
    echo "entering $dir"
    cd $dir
    if [ -f ./install ]; then
        ./install $CONNECT_STRING
    else
        $baseDir/definstall $CONNECT_STRING
    fi
    touch ok
    cd ..
done

