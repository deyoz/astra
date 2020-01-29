source ../local_before.tcl
if { [ file exists ../local_after.tcl ] } {
    source ../local_after.tcl
}
set OUR_PRIVATE_KEY "NO KEY DEFINED"
set LOGGERMETHOD LOGGER_SYSTEM_WRITE

set grp1_Txt(BIN) $SOCKDIR/sirena-BIN1
set grp1_Txt(BPORT) 7777
set grp1_Txt(SIGNAL) $SOCKDIR/sirena-BSIG1
set grp1_Txt(HEADTYPE) 1
set grp1_Txt(REDISPLAY) 1 ;#any value is equivalent to 1 !!! Don't set to 0 or NO

set grp2_Inet(BIN) $SOCKDIR/sirena-BIN2
set grp2_Inet(SIGNAL) $SOCKDIR/sirena-BSIG2
set grp2_Inet(HEADTYPE) 2
set grp2_Inet(APORT) 8001
set grp2_Inet(MAX_CONNECTIONS) 5

set grp4_Fcgi(BIN) $SOCKDIR/sirena-BIN4
set grp4_Fcgi(SIGNAL) $SOCKDIR/sirena-BSIG4
set grp4_Fcgi(HEADTYPE) 4
set grp4_Fcgi(APORT) 8099

set grp8_Http(BIN) $SOCKDIR/sirena-BIN8
set grp8_Http(SIGNAL) $SOCKDIR/sirena-BSIG8
set grp8_Http(HEADTYPE) 8
set grp8_Http(APORT) 8080

set grp9_HttpSSL(BIN) $SOCKDIR/sirena-BIN9
set grp9_HttpSSL(SIGNAL) $SOCKDIR/sirena-BSIG9
set grp9_HttpSSL(HEADTYPE) 8
set grp9_HttpSSL(APORT) 8443

set log1(SOCKET) $SOCKDIR/logger-socket
set log1(SOCKET_SHM) $SOCKDIR/logger-socket-shm
set log1(FILE) sirena.log
set log1(LEVEL) 19

set logdaemon(SOCKET) $SOCKDIR/logdaemon-socket
set logdaemon(SOCKET_SHM) $SOCKDIR/logdaemon-socket-shm
set logdaemon(FILE) daemon.log
set logdaemon(LEVEL) 19

set log_sys(SOCKET) $SOCKDIR/logsys-socket
set log_sys(SOCKET_SHM) $SOCKDIR/logsys-socket-shm
set log_sys(FILE) system.log
set log_sys(LEVEL) 19

set loginet(SOCKET) $SOCKDIR/loginet-socket
set loginet(SOCKET_SHM) $SOCKDIR/loginet-socket-shm
set loginet(FILE) internet.log
set loginet(LEVEL) 19

set logtlg(SOCKET) $SOCKDIR/logtlg-socket
set logtlg(SOCKET_SHM) $SOCKDIR/logtlg-socket-shm
set logtlg(FILE) airimp.log
set logtlg(LEVEL) 19

set monitor1(TCP_HOST) 0.0.0.0
set monitor1(TCP_LOG) 1
set monitor1(TCP_PORT) 7700
set monitor1(UDP_FILE) $SOCKDIR/udp-01

set MONITOR_LOG monitor.log
set MONITOR_PORT_FILE "./monitor.port"
set SSL_CERTIFICATE "certificate.pem"
set SSL_PRIVATE_KEY "private.key.pem"
set DIFFIE_HELLMAN_PARAMS "dh1024.pem"
set HTTPSRV_CMD $SOCKDIR/.httpsrv_cmd

if { ! [ info exists ::monitor1(TCP_PORT) ] } {
    set ::monitor1(TCP_PORT) 7700
}

execute
