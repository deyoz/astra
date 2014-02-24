set_local UNDER_GDB 0
set_local GROUPS_TO_RUN [list astra_init3 astra_init2 astra_init8]
set_local TIMER_NUM 6
set_local REOPENLOGS 1
set_local grp3_Jxt(APORT) 8005
set_local grp2_Inet(APORT) 8001
set_local monitor1(TCP_PORT) 7700
set_local monitor2(TCP_PORT) 7710
set_local CONNECT_STRING beta/beta
set_local LOG_LEVEL 20

set OWN_POINT_ADDR HSTDCS
set ETS_CANON_NAME MOWET
set OWN_CANON_NAME HSTDC
set SND_PORT 8996
set SRV_PORT 8995

set ENABLE_FR_DESIGN 1
set ENABLE_UNLOAD_PECTAB 1
set ENABLE_REQUEST_DUP 0
set TEST_SERVER 0
set USE_SEANCES 0

set SERVER_ID astra

set CREATE_SPP_DAYS 2
set ARX_MIN_DAYS 120
set ARX_MAX_DAYS 120
set ARX_DURATION 5
set ARX_SLEEP 35
set ARX_MAX_ROWS 1000
set ARX_TRIP_DATE_RANGE 1

#set APIS_PARTY_INFO SIRENA-TRAVEL:4959504991:4959504973
#set_local MESPRO_PSE_PATH "./crypt"

set TLG_ACK_TIMEOUT 1000
set TLG_SND_PROC_INTERVAL 1000

set USE_RSYSLOG 0
set ONE_LOGGER 0
set START_EMAIL_SENDER 0

