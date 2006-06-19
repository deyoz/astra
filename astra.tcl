#!/bin/sh
# backslash to prevent from running next line under tcl \
exec astra "$0" ${1+"$@"} 


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

set MINIMUM 0
set SYSLOG_TO_LOGFILE 1
set LOG_ERR_TO_CONSOLE 0
set GROUPS_TO_RUN [list astra_init1 astra_init2 astra_init3]
set OUR_PRIVATE_KEY "NO KEY DEFINED"
set ONE_LOGGER 0
proc set_local { varname value} {
	puts "set_local: $varname set to $value"
	uplevel #0 set $varname [list $value] 
}

proc set_hidden { varname value} {
	uplevel #0 set $varname [list $value] 
}

proc parse_old_cf { fl fltcl } {
    set fnew [ open $fltcl w 0600] 
    if  [catch { open $fl r } file]  {
        puts stderr "error opening $fl: $file"
    } else { 
        set par 0
        while { [gets $file line] >=0 } {
            if { ![string length $line]  } {
                continue
            }
            if { [ regexp -nocase {^ *\[parameters\] *$} $line ] } {
                set  par 1
                continue
            }
            if { $par == 1 } {
                if { [ regexp -nocase {^ *\[[[:alpha:]]+\] *$} $line ] } {
                    break
                }
            }
            if { [ regexp -nocase {^ *(;|#)} $line ] } {
                continue
            }
            if { $par  } {
                regsub {^[[:space:]]*([^[:space:]=].+)[[:space:]]*=[[:space:]]*(.*)$}  \
                $line {set \1 {\2}}  cc
                uplevel #0 [concat $cc] ;# eval set KEY=VALUE at upper level
		puts $fnew "sirena.cfg:[concat $cc]\n"
            }
        }
        close $file
    }
}

proc load_variables {} {
    set l1 [ file split   $::argv0 ]
        puts $l1
    set lbefore [ join [ lreplace  $l1 end end local_before.tcl ] / ]  
    if { [ file exists $lbefore ] } {
        uplevel #0 [list source $lbefore ]
    } 
    set sirenacfg [ join [ lreplace  $l1 end end sirena.cfg ] / ]
#    parse_old_cf $sirenacfg sirena_cfg.tcl

    set lafter [ join [ lreplace  $l1 end end local_after.tcl ] / ]  
    
    if { [ file exists $lafter ] } {
        uplevel #0 [ list source $lafter ]
    } 
}



if { ! [ info exists env(OBRZAP_NOSIR) ] } {
    set env(OBRZAP_NOSIR) NO
}
load_variables

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
set SOCKDIR Sockets
file mkdir $SOCKDIR

set grp1(BIN) $SOCKDIR/sirena-BIN1 


set grp1(BOUT) $SOCKDIR/sirena-BOUT1 
set grp1(BPORT) $KRV_PORT
set grp1(SIGNAL) $SOCKDIR/sirena-BSIG1
set grp1(HEADTYPE) 1
set grp1(REDISPLAY) 1 ;#any value is equivalent to 1 !!! Don't set to 0 or NO
if {! [info exists OBRZAP_NUM_1]} {
set OBRZAP_NUM_1  3
}
if {! [info exists grp2(APORT)]} {
	set grp2(APORT) 8001
}
set grp2(BIN) $SOCKDIR/sirena-BIN2 
set grp2(BOUT) $SOCKDIR/sirena-BOUT2 
set grp2(SIGNAL) $SOCKDIR/sirena-BSIG2
set grp2(HEADTYPE) 2 
if {! [info exists OBRZAP_NUM_2]} {
set OBRZAP_NUM_2  1
}

if {! [info exists grp3(APORT)]} {
set grp3(APORT) 8002
}
set grp3(BIN) $SOCKDIR/sirena-BIN3 
set grp3(BOUT) $SOCKDIR/sirena-BOUT3
set grp3(SIGNAL) $SOCKDIR/sirena-BSIG3
set grp3(HEADTYPE) 3 
set grp3(REDISPLAY) 1 ;#any value is equivalent to 1 !!! Don't set to 0 or NO
if {! [info exists OBRZAP_NUM_3]} {
set OBRZAP_NUM_3  1
}


set CMD_WLB  $SOCKDIR/wlb_cmd
set PNRNIGHT_CMD  $SOCKDIR/pnr_night_cmd
set CMD_NIGHT  $SOCKDIR/night_cmd
set CMD_RSD_CH  $SOCKDIR/rsd_ch_cmd
set ARCH_CMD $SOCKDIR/arch-cmd-
set SHMSERV $SOCKDIR/sirena-shmserv ;
set CMD_SHMSRV $SOCKDIR/shmserv_cmd ;
set LOCKERCMD $SOCKDIR/locker-socket
set AIRIMP_CMD $SOCKDIR/airimp_cmd_
set CMD_AIRSNDACK $SOCKDIR/airsndack_cmd
set CMD_AIRSND $SOCKDIR/airsnd_cmd
set CMD_AIRSRV $SOCKDIR/airsrv_cmd
set CMD_AIRXML $SOCKDIR/airxml_cmd
set CMD_WAIT_PNR $SOCKDIR/wait_pnr_cmd

set env(NLS_DATE_FORMAT) RRMMDD


set log1(SOCKET) $SOCKDIR/logger-socket
set log1(SOCKET_SHM) $SOCKDIR/logger-socket-shm
if { ! [ info exists LOGGERMETHOD ] } {
    set LOGGERMETHOD LOGGER_SYSTEM_WRITE
}
set log1(FILE)	sirena.log
if { ! [  info exists log1(LEVEL) ] } {
	set log1(LEVEL)	19
}
set loginet(SOCKET) $SOCKDIR/loginet-socket
set loginet(SOCKET_SHM) $SOCKDIR/loginet-socket-shm
set loginet(FILE)	internet.log
if { ! [  info exists loginet(LEVEL) ] } {
	set loginet(LEVEL)	19
}

set logdaemon(SOCKET) $SOCKDIR/logdaemon-socket
set logdaemon(SOCKET_SHM) $SOCKDIR/logdaemon-socket-shm
set logdaemon(FILE) daemon.log
if { ! [  info exists logdaemon(LEVEL) ] } {
	set logdaemon(LEVEL)	19
}

set logdebug(SOCKET) $SOCKDIR/logdebug-socket
set logdebug(SOCKET_SHM) $SOCKDIR/logdebug-socket-shm
set logdebug(FILE) debug.log
if { ! [  info exists logdebug(LEVEL) ] } {
	set logdebug(LEVEL)	19
}

set logairimp(SOCKET) $SOCKDIR/logairimp-socket
set logairimp(SOCKET_SHM) $SOCKDIR/logairimp-socket-shm
set logairimp(FILE) logairimp.log
if { ! [  info exists logairimp(LEVEL) ] } {
	set logairimp(LEVEL)	19
}

set log_sys(SOCKET) $SOCKDIR/logsys-socket
set log_sys(SOCKET_SHM) $SOCKDIR/logsys-socket-shm
set log_sys(FILE) system.log
if { ! [  info exists log_sys(LEVEL) ] } {
	set log_sys(LEVEL)	19
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

set_logging tclmon.log
set MONITOR_LOG monitor.log

#this procedure is called to start the system
proc astra_init {} {
#    get_csa
    if { ! $::ONE_LOGGER } {
        start_obr [ list logger log_sys $::SOCKDIR/logger-signal-sys ] 
    }
    start_obr [ list monitor monitor1 monitor2]
    start_obr [ list logger log1 $::SOCKDIR/logger-signal ] 
    if { ! $::ONE_LOGGER } {
        if { [lsearch -exact $::GROUPS_TO_RUN astra_init2 ] != -1 } {
            start_obr [ list logger loginet $::SOCKDIR/loginet-signal ] 
        }
    }
    if { ! $::ONE_LOGGER } {
        start_obr [ list logger logdaemon $::SOCKDIR/logger-signal-daemon ] 
    }
	if { $::AIRXMLRUN !=0  } {
        if { ! $::ONE_LOGGER } {
            start_obr [ list logger logdebug $::SOCKDIR/logger-signal-debug ] 
        }
    }
    if { ! $::ONE_LOGGER } {
        start_obr [ list logger logairimp $::SOCKDIR/logger-signal-airimp] 
    }
    sirena_sleep 3000
#    start_shmserv	
#    sirena_sleep 3000

#   group 1 - usual sirena terminal queries    
    if { [lsearch -exact $::GROUPS_TO_RUN astra_init1 ] != -1 } {
        astra_init1;
    }
#   group 2 - internet queries
    if { [lsearch -exact $::GROUPS_TO_RUN astra_init2 ] != -1 } {
        astra_init2;
    }

#   group 3 - XML_TERMINAL
    if { [lsearch -exact $::GROUPS_TO_RUN astra_init3 ] != -1 } {
        astra_init3;
    }
#   other processes    
#    start_other_daemons standard archive airimp;


    if {  ! $::MINIMUM  } {
         start_other_daemons standard archive airimp;
    }
}
proc start_shmserv {} {
#        for {set i 0} {$i<$::SHMSERV_NUM} {incr i} { 
#                start_obr [list shmsrv $i ]
#        }
}
proc astra_init1 {} {
    start_obr [list levb grp1]
    sirena_sleep 1000
    for {set i 1} {$i<=$::OBRZAP_NUM_1} {incr i} { 
        if  {  [ info exists ::OBRZAP_START_PAUSE ] } {
            sirena_sleep $::OBRZAP_START_PAUSE 
        }
        start_obr [list obrzap grp1 $i  ] 
    }
}
proc astra_init2 {} {

    start_obr [list levb grp2]
    sirena_sleep 1000
    for {set i 1} {$i<=$::OBRZAP_NUM_2} {incr i} { 
        start_obr [list obrzap grp2 [ expr $i + 20 ]  ] 
    }
    start_obr [list leva grp2]
}

proc astra_init3 {} {

    start_obr [list levb grp3]
    sirena_sleep 1000
    for {set i 1} {$i<=$::OBRZAP_NUM_3} {incr i} { 
        start_obr [list obrzap grp3 [ expr $i + 30 ]  ] 
    }
    start_obr [list leva grp3]
}

proc start_obr1 { a } {
    set name [ lindex $a 0 ]
    puts "start $name"
    if [ catch { start_obr $a } res ] {
	puts "$a failed to run : $res"
    }
}

proc start_other_daemons {args} {
    if { [lsearch -exact $args standard ] != -1 } {
        start_obr1 [ list locker $::LOCKER_TIMEOUT ]
	if { ! [ info exists ::DONT_RUN_WLB ] } {
        	start_obr1 [ list wlb  ]
	}
        start_obr1 [ list pnr_night  ]
        start_obr1 [ list rsd_ch  ]
        start_obr1 [ list night  ]
        start_obr1 [ list wait_pnr  ]
    }
	sirena_sleep 1000
    if { [lsearch -exact $args archive ] != -1 } {
        start_obr1 [ list arch_bgnd 70 ]
        start_obr1 [ list arch_pack  1 22 72 ]
    }
	sirena_sleep 1000
        
}



#check_context_size
if { 0 } {
if { [ string equal -nocase $env(OBRZAP_NOSIR) YES ] } {
   eval  run_nosir_mode  $argv ;
   return;
} else {
    set f [open sirena.pid w 0600]
    puts $f "tclmode [pid] $monitor1(TCP_PORT)"
    close $f
    lock_startup sirena.pid
}
last_run_time sirena_run_time.txt

if { [ string equal -nocase $env(XP_TESTING) 0 ] } { 
astra_init
}
if { [ string equal -nocase $env(XP_TESTING) 1 ] } { 
set_testing_mode 1
start_obr1 [list run_tests ]
}
if { [ string equal -nocase $env(XP_TESTING) 2 ] } { 
astra_init
sirena_sleep 5000
set_testing_mode 1
start_obr1 [list run_tests ]
}
}
set INIT_PROC astra_init
watch_loop mark_run_time
