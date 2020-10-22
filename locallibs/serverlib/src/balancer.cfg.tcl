if { [ file exists ./local_before.tcl ] } {
    source ./local_before.tcl
}

set logbalancer(SOCKET) $SOCKDIR/logbalancer-socket
set logbalancer(SOCKET_SHM) $SOCKDIR/logbalancer-socket-shm
set logbalancer(FILE) balancer.log
set logbalancer(LEVEL) 19

set logtlg(SOCKET) $SOCKDIR/logtlg-socket
set logtlg(SOCKET_SHM) $SOCKDIR/logtlg-socket-shm
set logtlg(FILE) msgdump.log
set logtlg(LEVEL) 19

set monitor1(TCP_HOST) 0.0.0.0
set monitor1(TCP_LOG) 1
set monitor1(TCP_PORT) 7701
set monitor1(UDP_FILE) $SOCKDIR/udp-01

set MONITOR_LOG monitor.log
set MONITOR_PORT_FILE "./monitor.port"

set balancer(HEADTYPE) 2
set balancer(PORT) 8003
set balancer(HEADTYPE_STATS) 8
set balancer(PORT_STATS) 8004

set NODES_CFG_FILE "./balancer.nodes.tcl"

if { [ file exists ./local_after.tcl ] } {
    source ./local_after.tcl
}

execute
