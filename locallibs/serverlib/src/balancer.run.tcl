#!/bin/sh
# backslash to prevent from running next line under tcl \
exec ./balancer.run.sh ./supervisor "$0" ${1+"$@"}

#LC_ALL=C exec: <supervisor executable file> <config file for supervisor (.tcl)> <arguments>

lappend auto_path "$env(PWD)/../"

if { [ file exists ./local_before.tcl ] } {
    source ./local_before.tcl
}

set OBRZAP_START_PAUSE 1
set SIRENA_ABORT(TCLMON) 1

package require tclmon
namespace import tclmon::*

#Executable file name for call exec(...) in create_proc_grp command
set LEVEL_C_EXEC_FILE ./dispatcher
#Executable file name for call exec(...) in create_proc_levela command
set LEVEL_A_EXEC_FILE ./dispatcher
#Executable file name for call exec(...) in create_process command
set OTHER_EXEC_FILE ./dispatcher
#Executable file name for call exec(...) in create_profile_process command
set PROFILE_EXEC_FILE ./dispatcher
#Config file name for $EXEC_FILE
set EXEC_FILE_CONFIG ./balancer.cfg.tcl

if { [ file exists ./local_after.tcl ] } {
    source ./local_after.tcl
}

file mkdir $SOCKDIR

if { ! [ info exists ::RUN_TIME_FILE ] } {
    set ::RUN_TIME_FILE "balancer.time"
}

if { ! [ info exists ::env(PIDFILE) ] } {
    set ::env(PIDFILE) balancer.pid
}

if { ! [ info exists ::env(XP_TESTING) ] } {
    set ::env(XP_TESTING) 0
}

if { ! [ info exists ::BALANCER_GROUPS ] } {
    set ::BALANCER_GROUPS [ list balancer ]
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
        create_dispatcher 1 [ list logger logbalancer $::SOCKDIR/logger-signal-balancer ] 0
        create_dispatcher 1 [ list logger logtlg $::SOCKDIR/logger-signal-logtlg ] 0
    }

    set workers_count 0
    set total_count 0

    foreach g $::BALANCER_GROUPS {
        upvar #0 $g grp
        if { [ info exists grp(NUMBER_OF_LISTENERS) ] } {
            set wc $grp(NUMBER_OF_LISTENERS)

            for { set n 1 } { $n <= $wc } { incr n } {
                create_dispatcher 1 [ list balancer $g $n ] 1
            }

            set workers_count [ expr { $workers_count + $wc } ]
            set total_count [ expr { $total_count + $wc } ]

            if { [ info exists grp(ENABLE_CLIENTS_STATS) ] && $grp(ENABLE_CLIENTS_STATS) } {
                create_dispatcher 1 [ list balancer_clients_stats $g ] 2
                incr total_count
            }

        } else {
            create_dispatcher 1 [ list balancer $g ] 1
        }
    }

    init_semaphore [ list $workers_count $total_count ]
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
