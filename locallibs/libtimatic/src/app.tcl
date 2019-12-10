set WHERE_SERVERLIB ../../serverlib

#------------------------------------------------

if { [ file exists $WHERE_SERVERLIB/local_before.tcl ] } {
    source $WHERE_SERVERLIB/local_before.tcl
}

#------------------------------------------------

file mkdir $SOCKDIR
set OUR_PRIVATE_KEY "NO KEY DEFINED"
set CMD_SMTP $SOCKDIR/smtp
set HTTPSRV_CMD $SOCKDIR/httpsrv_cmd
set LOCAL_HOST 0.0.0.0
set KRV_TEST 2

set log1(SOCKET) $SOCKDIR/logger-socket
set log1(SOCKET_SHM) $SOCKDIR/logger-socket-shm
set log1(FILE) sirenatim.log

set MONITOR_LOG monitor.log
set MONITOR_PORT_FILE "./monitor.port"
set monitor1(TCP_HOST) 0.0.0.0
set monitor1(TCP_LOG) 1
set monitor2(TCP_HOST) 0.0.0.0
set monitor2(TCP_LOG) 1

#------------------------------------------------

if { [ file exists $WHERE_SERVERLIB/local_after.tcl ] } {
    source $WHERE_SERVERLIB/local_after.tcl
}

#------------------------------------------------

set TCLMON_LOG tclmon.log
set USE_RSYSLOG 0

set_logging $::USE_RSYSLOG
execute
