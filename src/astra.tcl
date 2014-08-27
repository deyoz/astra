proc load_variables {file_name} {
    set l1 [ file split   $::argv0 ]
    puts $l1

    set path [ join [ lreplace  $l1 end end $file_name ] / ]
    if { [ file exists $path ] } {
        uplevel #0 [list source $path ]
    }
}

load_variables "local_before.tcl"

if { ! [ info exists env(OBRZAP_NOSIR) ] } {
    set env(OBRZAP_NOSIR) NO
}
if { ! [ info exists env(XP_TESTING) ] } {
    set env(XP_TESTING)  0
}

set OUR_PRIVATE_KEY "NO KEY DEFINED"
set CMD_AIRSRV $SOCKDIR/airsrv_cmd
set CMD_SENDER_TYPEA $SOCKDIR/sender_typea_cmd
set CMD_SENDER_OTHER $SOCKDIR/sender_other_cmd
set CMD_OBRZAP_TLG $SOCKDIR/obrzap_tlg
set CMD_TLGDISPATCHER $SOCKDIR/tlgdispatcher
set CMD_TLGCLEANER $SOCKDIR/tlgcleaner
set CMD_AC_SWITCHER $SOCKDIR/ac_switcher_cmd
set CMD_EDI_TIMER $SOCKDIR/edi_timer_cmd
set AIRIMP_CMD $CMD_OBRZAP_TLG
set CMD_SENDER_EXPRESS $SOCKDIR/sender_express_cmd
set XPR_DISP_OUT $SOCKDIR/xpr_disp_out_cmd
set XPR_DISP_IN $SOCKDIR/xpr_disp_in_cmd
set XPR_DISP_SIGNAL $SOCKDIR/xpr_disp_signal_

set grp3(BIN) $SOCKDIR/sirena-BIN3
set grp3(BOUT) $SOCKDIR/sirena-BOUT3
set grp3(SIGNAL) $SOCKDIR/sirena-BSIG3
set grp3(HEADTYPE) 3
set grp3(REDISPLAY) 1 ;#any value is equivalent to 1 !!! Don't set to 0 or NO

set logjxt(SOCKET) $SOCKDIR/logger-socket
set logjxt(SOCKET_SHM) $SOCKDIR/logger-socket-shm
set logjxt(FILE) eticket.log

set loginet(SOCKET) $SOCKDIR/loginet-socket
set loginet(SOCKET_SHM) $SOCKDIR/loginet-socket-shm
set loginet(FILE) internet.log

set logdaemon(SOCKET) $SOCKDIR/logdaemon-socket
set logdaemon(SOCKET_SHM) $SOCKDIR/logdaemon-socket-shm
set logdaemon(FILE) daemon.log

set logtlghandler(SOCKET) $SOCKDIR/logtlghandler-socket
set logtlghandler(SOCKET_SHM) $SOCKDIR/logtlghandler-socket-shm
set logtlghandler(FILE) tlghandler.log

set logairimp(SOCKET) $SOCKDIR/logairimp-socket
set logairimp(SOCKET_SHM) $SOCKDIR/logairimp-socket-shm
set logairimp(FILE) logairimp.log

set log_sys(SOCKET) $SOCKDIR/logsys-socket
set log_sys(SOCKET_SHM) $SOCKDIR/logsys-socket-shm
set log_sys(FILE) system.log

set logtlg(SOCKET) $SOCKDIR/logtlg-socket
set logtlg(SOCKET_SHM) $SOCKDIR/logtlg-socket-shm
set logtlg(FILE) tlg.log

set monitor1(TCP_HOST) 0.0.0.0
set monitor1(TCP_LOG) 1

set MONITOR_PORT_FILE "./monitor.port"

load_variables "local_after.tcl"

if { ! [  info exists logjxt(LEVEL) ] } {
    set logjxt(LEVEL) 19
}
if { ! [  info exists loginet(LEVEL) ] } {
    set loginet(LEVEL) 19
}
if { ! [  info exists logdaemon(LEVEL) ] } {
    set logdaemon(LEVEL) 19
}
if { ! [  info exists logtlghandler(LEVEL) ] } {
    set logtlghandler(LEVEL) 19
}
if { ! [  info exists logairimp(LEVEL) ] } {
    set logairimp(LEVEL) 19
}
if { ! [  info exists log_sys(LEVEL) ] } {
    set log_sys(LEVEL) 19
}
if { ! [  info exists logtlg(LEVEL) ] } {
    set logtlg(LEVEL) 19
}
if {! [info exists grp3(APORT)]} {
    set grp3(APORT) 8002
}
if { ! [info exists LOCKER_TIMEOUT ] } {
    set LOCKER_TIMEOUT  100
}
if { ! [ info exists LOGGERMETHOD ] } {
    set LOGGERMETHOD LOGGER_SYSTEM_WRITE
}
if { ! [ info exists CUTLOGGING] } {
    set CUTLOGGING  100
}
if { ! [info exists monitor1(TCP_PORT)] } {
    set monitor1(TCP_PORT) 7700
}
if { ! [info exists monitor1(UDP_FILE)] } {
    set monitor1(UDP_FILE) $SOCKDIR/udp-01
}

if { $::START_EMAIL_SENDER } {
    if { [ catch { package require sirena_mail 1.1002 } pkg_err ] } {
        puts $pkg_err
        puts {
            mail will not work
            you should install tcllib (from sourceforge)
            put the following line (possible edited into your local_before.tcl file)
            lappend auto_path "$env(HOME)/work/serverlib/scripts"
        }
    }
}

execute

