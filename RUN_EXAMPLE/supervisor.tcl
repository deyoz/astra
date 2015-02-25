#!/bin/sh
# backslash to prevent from running next line under tcl \
LC_ALL=C \
exec ./supervisor "$0" ${1+"$@"}

#LC_ALL=C exec: <supervisor executable file> <config file for supervisor (.tcl)> <arguments>

source ./local_before.tcl

package require tclmon
namespace import tclmon::*

if { ! [ info exists ::env(PIDFILE) ] } {
    set ::env(PIDFILE) astra.pid
}

set f [open $::env(PIDFILE) w 0600]
puts -nonewline $f "tclmode [pid] "
close $f

#Executable file name for call exec(...) in create_proc_grp command
set LEVEL_C_EXEC_FILE ./astra
#Executable file name for call exec(...) in create_dispatcher command
set LEVEL_A_EXEC_FILE ./dispatcher
#Executable file name for call exec(...) in create_process command
set OTHER_EXEC_FILE ./astra
#Config file name for $EXEC_FILE
set EXEC_FILE_CONFIG ./astra.tcl

file mkdir $SOCKDIR

set OBRZAP_START_PAUSE 1

set SIRENA_ABORT(TCLMON) 1
if { [ file exists ./local_after.tcl ] } {
    source ./local_after.tcl
}

if { ! [ info exists ::RUN_TIME_FILE ] } {
    set ::RUN_TIME_FILE sirena_run_time.txt
}
if { ! [ info exists ::env(XP_TESTING) ] } {
    set ::env(XP_TESTING) 0
}

if { ! [info exists USE_RSYSLOG] } {
    set USE_RSYSLOG 0
}

if { ! [ info exists ::env(PIDFILE) ] } {
    set ::env(PIDFILE) astra.pid
}

if { ! [ info exists ::env(XP_TESTING) ] } {
    set ::env(XP_TESTING) 0
}

if  { ! [ info exists env(TCL_NO_LOCK_STARTUP__) ]  } {
    lock_startup $::env(PIDFILE)
}

set TCLMON_LOG tclmon.log
set CUT_MONITOR_LOG 0

proc astra_init2 {} {
    create_dispatcher 1 [list leva grp2_Inet] 2
    create_proc_grp $::grp2_Inet(OBRZAP_NUM) [list obrzap grp2_Inet ] 3
}

proc astra_init3 {} {
    create_dispatcher 1 [list leva grp3_Jxt] 2
    create_proc_grp $::grp3_Jxt(OBRZAP_NUM) [list obrzap grp3_Jxt ] 3
}

proc astra_init8 {} {
    create_dispatcher 1 [ list levh grp8_Http] 1
    create_proc_grp $::grp8_Http(OBRZAP_NUM) [ list obrzap grp8_Http ] 2
}

proc epilogue {} {
    rename exec {}
    rename auto_execok {}
    rename cd {}
    rename fcopy {}
    rename fileevent {}
    rename interp {}
    rename load {}
    rename socket {}
}

#this procedure is called to start the system
proc astra_init {} {
    #create_proc_grp <grp_name> <handlers_count> <start_line> <priority>
    #Between group with different priority makes sleep($OBRZAP_START_PAUSE)
    last_run_time $::RUN_TIME_FILE

    create_dispatcher 1 [ list monitor monitor1 ] 0

    if { ! $::USE_RSYSLOG } {
        create_dispatcher 1 [ list logger log_sys $::SOCKDIR/logger-signal-sys ] 0

        create_dispatcher 1 [ list logger log1 $::SOCKDIR/logger-signal ] 0

        create_dispatcher 1 [ list logger logjxt $::SOCKDIR/logjxt-signal ] 0

        create_dispatcher 1 [ list logger loginet $::SOCKDIR/loginet-signal ] 0

        create_dispatcher 1 [ list logger logairimp $::SOCKDIR/logger-signal-airimp] 0

        create_dispatcher 1 [ list logger logdaemon $::SOCKDIR/logger-signal-daemon ] 0

        create_dispatcher 1 [ list logger logtlg $::SOCKDIR/logtlg-signal ] 0
    }

    #   group 2 - internet queries
    if { [lsearch -exact $::GROUPS_TO_RUN astra_init2 ] != -1 } {
        astra_init2;
    }

    #   group 3 - XML_TERMINAL
    if { [lsearch -exact $::GROUPS_TO_RUN astra_init3 ] != -1 } {
        astra_init3;
    }

    #   group 8 - HTTP
    if { [lsearch -exact $::GROUPS_TO_RUN astra_init8 ] != -1 } {
        astra_init8;
    }


    #for {set i 1} {$i<=$::TIMER_NUM} {incr i} {
    #  create_process 1 [ list timer $i ] 3
    #}
    create_process 1 [ list timer den ] 3
    #create_process 1 [ list tlg_http_snd ] 3

#create_process 1 [ list tlg_snd ] 3
#create_process 1 [ list tlg_srv ] 3
#create_process 1 [ list typeb_handler ] 3
#create_process 1 [ list typeb_parser ] 3
#create_process 1 [ list edi_handler ] 3

    #create_process 1 [ list aodb_handler ] 3

    epilogue
}

set f [open $::env(PIDFILE) w 0600]
puts -nonewline $f "tclmode [pid] "
close $f
if  { ! [ info exists env(TCL_NO_LOCK_STARTUP__) ]  } {
    lock_startup $::env(PIDFILE)
}

#set loging to $TCLMON_LOG file or to rsyslog daemon
set_logging $USE_RSYSLOG

astra_init
