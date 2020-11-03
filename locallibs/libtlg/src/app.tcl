if { [ file exists local_before.tcl ] } {
    source local_before.tcl
}

set TCLMON_LOG tclmon.log
set USE_RSYSLOG 0
set CONNECT_STRING "sirena/orient"

set_logging $::USE_RSYSLOG
execute

