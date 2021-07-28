#!/bin/sh
rm -f sirena_run_time.txt
(cd "$LIBROOT/serverlib" && echo "pkg_mkIndex -verbose . arr.tcl" | tclsh)
(cd "$LIBROOT/serverlib/scripts" && echo "pkg_mkIndex -verbose . sirena_mail.tcl" | tclsh)

if [ ! -f "astra.tcl" ]; then
    cp ../RUN_EXAMPLE/astra.tcl astra.tcl
fi

if [ ! -f "nosir.tcl" ]; then
    cp ../RUN_EXAMPLE/nosir.tcl nosir.tcl
fi

if [ ! -f "date_time_zonespec.csv" ]; then
    cp ../RUN_EXAMPLE/date_time_zonespec.csv date_time_zonespec.csv
fi

cat >local_before.tcl <<EOF
lappend auto_path "$LIBROOT/serverlib"

proc set_local { varname value} {
    puts "set_local: $varname set to $value"
    uplevel #0 set $varname [list $value]
}

proc set_hidden { varname value} {
    uplevel #0 set $varname [list $value]
}

set grp2_Inet(OBRZAP_NUM) 5
set grp3_Jxt(OBRZAP_NUM) 15
set grp8_Http(OBRZAP_NUM) 1
set grp9_HttpSSL(OBRZAP_NUM) 0
set SOCKDIR ./Sockets

EOF

cat >local_after.tcl <<EOF
if { [ info exists env(XP_SPEED) ] } {
    if { \$env(XP_SPEED) > 0 } {
        set CUTLOGGING 1
    }
} else {
    set CUTLOGGING 20
}
if { [ info exists env(XP_CUTLOGGING) ] } {
    set CUTLOGGING \$env(XP_CUTLOGGING)
}

set UNDER_GDB 0
set CSA_KEY 127
set GROUPS_TO_RUN [list astra_init3 astra_init2 astra_init8 astra_init_other]
set TIMER_NUM 6
set REOPENLOGS 1
set grp2_Inet(APORT) 8005
set grp3_Jxt(APORT) 8001
set monitor1(TCP_PORT) 7700
set monitor2(TCP_PORT) 7710
set LOG_LEVEL 20
set TRUE_TIME_LOG_STYLE 1

set CONNECT_STRING "$CONNECT_STRING"
set PG_CONNECT_STRING "$PG_CONNECT_STRING"
set PG_CONNECT_STRING_ARX "$PG_CONNECT_STRING_ARX"
set XP_TESTING_FILES "$XP_TESTING_FILES"
set XP_TESTING_FILES_SERVERLIB "$XP_TESTING_FILES_SERVERLIB"

set OWN_POINT_ADDR "TSTDCS"
set ETS_CANON_NAME "MOWET"
set OWN_CANON_NAME "$OURNAME"
set SND_PORT 8996
set SRV_PORT 8995

set ENABLE_FR_DESIGN 1
set ENABLE_UNLOAD_PECTAB 1
set ENABLE_REQUEST_DUP 0
set TEST_SERVER 1
set USE_SEANCES 0

set SERVER_ID astra

set CREATE_SPP_DAYS 2
set ARX_READ_PG 1
set ARX_DAYS 120
set ARX_DURATION 5
set ARX_SLEEP 35
set ARX_MAX_ROWS 1000
set ARX_MAX_DATE_RANGE 999
set ARX_TRIP_DATE_RANGE 1

#set APIS_PARTY_INFO SIRENA-TRAVEL:4959504991:4959504973
#set MESPRO_PSE_PATH "./crypt"

set TLG_ACK_TIMEOUT 1000
set TLG_SND_PROC_INTERVAL 1000
set SCAN_TLG_FETCH_LIMIT 10

set ROT_UPDATE_INTERVAL 60

set USE_RSYSLOG 0
set ONE_LOGGER 0
set START_EMAIL_SENDER 0
set ALWAYS_OCI8 0

# trace all ora sql if set TRACE_ORA_SQL 1
set TRACE_ORA_SQL 0

set SP_PG_GROUP_ET 3
set SP_PG_GROUP_EMD 3
set SP_PG_GROUP_CRS_SVC 3
set SP_PG_GROUP_CRS_DOC 3
set SP_PG_GROUP_PAX_ALARMS 3
set SP_PG_GROUP_CRS_PAX_1 3
set SP_PG_GROUP_REG_PAX_1 0
set SP_PG_GROUP_CRS_DATA 3
set SP_PG_GROUP_CRS_SET 3
set SP_PG_GROUP_DCS_BAG 3
set SP_PG_GROUP_CRS_TRANSFER 3
set SP_PG_GROUP_TLG_TRANSFER 3
set SP_PG_GROUP_TYPEB_DATA_STAT 3
set SP_PG_GROUP_CRS_DISPLACE2 3
set SP_PG_GROUP_TLG_TRIPS 3
set SP_PG_GROUP_CRS_TKN 3
set SP_PG_GROUP_TYPEB_1 3
set SP_PG_GROUP_ANNUL_BAG 3
set SP_PG_GROUP_PAID_RFISC 3
set SP_PG_GROUP_CONFIRM_PRINT 3
set SP_PG_GROUP_ROZYSK 3
set SP_PG_GROUP_TAGS_GENERATED 3
set SP_PG_GROUP_STAT_1 3
set SP_PG_GROUP_PAX_SERVICES 3
set SP_PG_GROUP_SERVICE_LISTS 3
set SP_PG_GROUP_BAG_RECEIPTS 3
set SP_PG_GROUP_HOTEL_ACMD 3
set SP_PG_GROUP_KASSA 3
set SP_PG_GROUP_AODB_1 3
set SP_PG_GROUP_TRIP_VOUCHERS 3

set SP_PG_GROUP_TASKS 3
set SP_PG_GROUP_TAG_PACKS 3

set SP_PG_GROUP_JXTCONT 3
set SP_PG_GROUP_PP_TRIP_TASK 3
set SP_PG_GROUP_FILE 3
set SP_PG_GROUP_FILE2 3
set SP_PG_CONVERT_TABS 3
# only after cache_tables migrated
set SP_PG_GROUP_FILE_CFG 0
set SP_PG_GROUP_IAPI 3
set SP_PG_GROUP_IATCI 3
set SP_PG_GROUP_WC 3
set SP_PG_GROUP_WB 3
set SP_PG_GROUP_SCHED 3
set SP_PG_GROUP_SCHED_SEQ 3
set SP_PG_GROUP_CONTEXT 3
set SP_PG_GROUP_EDIFACT 3
set SP_PG_GROUP_SPPCKIN 3
set SP_PG_GROUP_SPPCKIN2 0
set SP_PG_GROUP_APPS 3
set SP_PG_GROUP_ARX 3
set SP_PG_GROUP_TLG_QUE 3
set SP_PG_GROUP_COMP 3
set SP_PG_GROUP_HTML 3
set SP_PG_GROUP_FR 3
set SP_PG_GROUP_LIBTLG 3
set SP_PG_GROUP_PP_TLG 3
set SP_PG_GROUP_CRYPT 3
set SP_PG_GROUP_BASETABLES 0
set SP_PG_GROUP_STAGES 0
set SP_PG_GROUP_COUNTERS 0
set SP_PG_GROUP_PAX 0
set SP_PG_GROUP_TRIP_TASK 0
set SP_PG_GROUP_FILE_PARAM_SETS 0
set SP_PG_GROUP_TYPEB_TYPES 0
set SP_PG_GROUP_CRS_PNR 0
set SP_PG_GROUP_TRANSFER 0
set SP_PG_GROUP_TRIP_CALC 0
set SP_PG_GROUP_STATIONS 0
set SP_PG_GROUP_COMP_BASELAYERS 0
set SP_PG_GROUP_COMP_CLASSES 0

EOF
