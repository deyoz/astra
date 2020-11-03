#!/bin/sh
# backslash to prevent from running next line under tcl \
LC_ALL=C \
exec ./supervisor "$0" ${1+"$@"}

#LC_ALL=C exec: <supervisor executable file> <config file for supervisor (.tcl)> <arguments>

lappend auto_path "$env(PWD)/../"
source ../local_before.tcl

set OBRZAP_START_PAUSE 1
set SIRENA_ABORT(TCLMON) 1

if { [ file exists ../local_after.tcl ] } {
    source ../local_after.tcl
}

package require tclmon
namespace import tclmon::*

#Executable file name for call exec(...) in create_proc_grp command
set LEVEL_C_EXEC_FILE ./testapp
#Executable file name for call exec(...) in create_proc_levela command
set LEVEL_A_EXEC_FILE ./testapp
#Executable file name for call exec(...) in create_process command
set OTHER_EXEC_FILE ./testapp
#Executable file name for call exec(...) in create_profile_process command
set PROFILE_EXEC_FILE ./testapp
#Config file name for $EXEC_FILE
set EXEC_FILE_CONFIG ./handler.tcl

file mkdir $SOCKDIR

if { ! [ info exists ::RUN_TIME_FILE ] } {
    set ::RUN_TIME_FILE sirena_run_time.txt
}

if { ! [ info exists ::env(PIDFILE) ] } {
    set ::env(PIDFILE) sirena.pid
}

if { ! [ info exists ::env(XP_TESTING) ] } {
    set ::env(XP_TESTING) 0
}

set TCLMON_LOG tclmon.log
set CUT_MONITOR_LOG 0
#this procedure is called to start the system
proc app_init {} {
    #create_proc_grp <grp_name> <handlers_count> <start_line> <priority>
    #Between group with different priority makes sleep($OBRZAP_START_PAUSE)
    last_run_time $::RUN_TIME_FILE

    create_dispatcher 1 [ list monitor monitor1 ] 0

    if { ! $::USE_RSYSLOG } {
        create_dispatcher 1 [ list logger log_sys $::SOCKDIR/logger-signal-sys ] 0

        create_dispatcher 1 [ list logger log1 $::SOCKDIR/logger-signal ] 0

        create_dispatcher 1 [ list logger logdaemon $::SOCKDIR/logger-signal-daemon ] 0

        create_dispatcher 1 [ list logger loginet $::SOCKDIR/loginet-signal ] 0

        create_dispatcher 1 [ list logger logtlg $::SOCKDIR/logtlg-signal ] 0
    }

    if { ! $::MINIMUM } {
        create_process 1 [list daemon] 1
    }
    if { $::grp1_Txt(OBRZAP_NUM) > 0 } {
        create_dispatcher 1 [ list leva_udp grp1_Txt ] 1
        create_proc_grp $::grp1_Txt(OBRZAP_NUM) [ list obrzap grp1_Txt ] 2
    }
    if { $::grp2_Inet(OBRZAP_NUM) > 0 } {
        create_dispatcher 1 [ list leva grp2_Inet ] 1
        create_proc_grp  $::grp2_Inet(OBRZAP_NUM) [ list obrzap grp2_Inet ] 2
    }
    if { $::grp8_Http(OBRZAP_NUM) > 0 } {
        create_dispatcher 1 [ list levh grp8_Http] 1
        create_proc_grp $::grp8_Http(OBRZAP_NUM) [ list obrzap grp8_Http ] 2
    }
    if { $::grp9_HttpSSL(OBRZAP_NUM) > 0 } {
        create_dispatcher 1 [ list levhssl grp9_HttpSSL] 1
        create_proc_grp $::grp9_HttpSSL(OBRZAP_NUM) [ list obrzap grp9_HttpSSL ] 2
    }
    if { ! $::MINIMUM } {
        create_process 1 [ list httpsrv ] 1
    }
}

set f [open $::env(PIDFILE) w 0600]
puts -nonewline $f "tclmode [pid]"
close $f
if  { ! [ info exists env(TCL_NO_LOCK_STARTUP__) ]  } {
    lock_startup $::env(PIDFILE)
}

#set loging to $TCLMON_LOG file or to rsyslog daemon
set_logging $USE_RSYSLOG
app_init
