# put the following line (possible edited into your local_before.tcl file)
# see local_before_example.tcl
#lappend auto_path "$env(HOME)/work/tclmon"

# don't edit the following lines here ; put it in local_after.tcl instead,
# changing set to set_local
# see local_after_example.tcl

if { ! [  info exists env(ASTRA_INSTANCE) ] } {
    set env(ASTRA_INSTANCE) [pid]:0
} else {
    regexp {(.*):(.*)} $env(ASTRA_INSTANCE) dummy v1 v2
    incr v2
    set env(ASTRA_INSTANCE) $v1:$v2
}

proc load_variables {} {
    set l1 [ file split   $::argv0 ]
        puts $l1
    set lbefore [ join [ lreplace  $l1 end end local_before.tcl ] / ]
    if { [ file exists $lbefore ] } {
        uplevel #0 [list source $lbefore ]
    }

    set lafter [ join [ lreplace  $l1 end end local_after.tcl ] / ]

    if { [ file exists $lafter ] } {
        uplevel #0 [ list source $lafter ]
    }
}


if { ! [ info exists env(OBRZAP_NOSIR) ] } {
    set env(OBRZAP_NOSIR) NO
}

load_variables

if { ![ info exists env(XP_TESTING) ] } {
    set env(XP_TESTING)  0
}

if {  [ string equal -nocase $env(OBRZAP_NOSIR) YES ]
    && [ info exists env(CONNECT_STRING) ] } {
 set CONNECT_STRING $env(CONNECT_STRING)

}
if { [ catch {
         package require tclmon
         } pkg_err ] } {
puts $pkg_err
puts {
   put the following line (possible edited into your local_before.tcl file)
   see local_before_example.tcl
   lappend auto_path "$env(HOME)/work/tclmon"
}
exit 1
}

namespace import tclmon::*
set COMMON_SOCKDIR /usr/local/Sockets
set REQUEST_DUP_SOCKDIR /usr/local/Sockets

if { $::env(XP_TESTING) } {
    file mkdir $SOCKDIR
}

if {! [info exists grp2_Inet(APORT)]} {
    set grp2_Inet(APORT) 8001
}
set grp2_Inet(BIN) $SOCKDIR/sirena-BIN2
set grp2_Inet(BOUT) $SOCKDIR/sirena-BOUT2
set grp2_Inet(SIGNAL) $SOCKDIR/sirena-BSIG2
set grp2_Inet(HEADTYPE) 2

if {! [info exists grp3_Jxt(APORT)]} {
    set grp3_Jxt(APORT) 8002
}
set grp3_Jxt(BIN) $SOCKDIR/sirena-BIN3
set grp3_Jxt(BOUT) $SOCKDIR/sirena-BOUT3
set grp3_Jxt(SIGNAL) $SOCKDIR/sirena-BSIG3
set grp3_Jxt(HEADTYPE) 3
set grp3_Jxt(REDISPLAY) 1 ;#any value is equivalent to 1 !!! Don't set to 0 or NO

set grp8_Http(BIN) $SOCKDIR/sirena-BIN8
set grp8_Http(SIGNAL) $SOCKDIR/sirena-BSIG8
set grp8_Http(HEADTYPE) 8
set grp8_Http(APORT) 8080

set CMD_EDI_HANDLER $COMMON_SOCKDIR/edi_handler_cmd
set CMD_TYPEB_HANDLER $COMMON_SOCKDIR/typeb_handler_cmd
set CMD_TYPEB_PARSER $COMMON_SOCKDIR/typeb_parser_cmd
set CMD_TLG_HTTP_SND $COMMON_SOCKDIR/tlg_http_snd_cmd
set CMD_TLG_SND $COMMON_SOCKDIR/tlg_snd_cmd
set CMD_PARSE_AODB $SOCKDIR/parse_aodb_cmd

set REQUEST_DUP $REQUEST_DUP_SOCKDIR/request_dup

set env(NLS_DATE_FORMAT) RRMMDD


set log1(SOCKET) $SOCKDIR/logger-socket
set log1(SOCKET_SHM) $SOCKDIR/logger-socket-shm
set log1(FILE) txtastra.log
if { ! [  info exists log1(LEVEL) ] } {
    set log1(LEVEL) 19
}

set loginet(SOCKET) $SOCKDIR/loginet-socket
set loginet(SOCKET_SHM) $SOCKDIR/loginet-socket-shm
set loginet(FILE) internet.log
if { ! [  info exists loginet(LEVEL) ] } {
    set loginet(LEVEL) 19
}

set logjxt(SOCKET) $SOCKDIR/logjxt-socket
set logjxt(SOCKET_SHM) $SOCKDIR/logjxt-socket-shm
set logjxt(FILE) astra.log
if { ! [  info exists logjxt(LEVEL) ] } {
    set logjxt(LEVEL) 19
}

set logairimp(SOCKET) $SOCKDIR/logairimp-socket
set logairimp(SOCKET_SHM) $SOCKDIR/logairimp-socket-shm
set logairimp(FILE) logairimp.log
if { ! [  info exists logairimp(LEVEL) ] } {
    set logairimp(LEVEL) 19
}

set logdaemon(SOCKET) $SOCKDIR/logdaemon-socket
set logdaemon(SOCKET_SHM) $SOCKDIR/logdaemon-socket-shm
set logdaemon(FILE) daemon.log
if { ! [  info exists logdaemon(LEVEL) ] } {
    set logdaemon(LEVEL) 19
}

set log_sys(SOCKET) $SOCKDIR/logsys-socket
set log_sys(SOCKET_SHM) $SOCKDIR/logsys-socket-shm
set log_sys(FILE) system.log
if { ! [  info exists log_sys(LEVEL) ] } {
    set log_sys(LEVEL) 19
}

if { ! [ info exists CUTLOGGING] } {
    set CUTLOGGING  100
}
if { ! [ info exists env(XP_TESTING) ] } {
    set env(XP_TESTING)  0
}

set monitor1(TCP_HOST) 0.0.0.0
set monitor1(TCP_LOG) 1
if { ! [info exists monitor1(TCP_PORT)] } {
    set monitor1(TCP_PORT) 7700
}
if { ! [info exists monitor1(UDP_FILE)] } {
    set monitor1(UDP_FILE) $SOCKDIR/udp-01
}

set monitor2(TCP_HOST) 0.0.0.0
set monitor2(TCP_LOG) 1
if { ! [info exists monitor2(TCP_PORT)] } {
    set monitor2(TCP_PORT) 7703
}
if { ! [info exists monitor2(UDP_FILE)] } {
    set monitor2(UDP_FILE) $SOCKDIR/udp-01
}

set MONITOR_LOG monitor.log

execute

