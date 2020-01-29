lappend auto_path "/home/forever/work/sirena_trunk/sirenalibs/serverlib"

set CONNECT_STRING "sirena/orient"
set USE_RSYSLOG 0
set TCLMON_LOG tclmon.log

if { [ file exists local_after.tcl ] } {
    source local_after.tcl
}

set_logging $::USE_RSYSLOG
execute

