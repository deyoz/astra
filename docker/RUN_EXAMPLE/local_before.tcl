# this file is processed before sirena.cfg
# add path to tclmon library to autoload path
#lappend auto_path "$env(HOME)/work/tclmon"
lappend auto_path "/opt/astra/run"

proc set_local { varname value} {
    puts "set_local: $varname set to $value"
    uplevel #0 set $varname [list $value]
}

proc set_hidden { varname value} {
    uplevel #0 set $varname [list $value]
}

set grp2_Inet(OBRZAP_NUM) 0
set grp3_Jxt(OBRZAP_NUM) 3
set grp8_Http(OBRZAP_NUM) 0
set grp9_HttpSSL(OBRZAP_NUM) 0
set SOCKDIR ./Sockets
