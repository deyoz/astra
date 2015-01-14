set_local UNDER_GDB 0
set CSA_KEY 665
#set_local GROUPS_TO_RUN [list astra_init3 astra_init2 astra_init8]
set_local GROUPS_TO_RUN [list astra_init3]
#set_local TIMER_NUM 6
set_local REOPENLOGS 1
set_local grp2_Inet(APORT) 58009
set_local grp3_Jxt(APORT) 58005
set_local monitor1(TCP_PORT) 57702
#set_local monitor2(TCP_PORT) 27712
#set_local CONNECT_STRING stand/rty@stand
set_local CONNECT_STRING beta/beta@beta
#set_local CONNECT_STRING test/astratst@stand
#set_local CONNECT_STRING astra/astra@astra
set_local LOG_LEVEL 20

set OWN_POINT_ADDR BETADC
set ETS_CANON_NAME MOWTT
set OWN_CANON_NAME BETDC
set SND_PORT 8994
set SRV_PORT 8993

set ENABLE_FR_DESIGN 1
set ENABLE_UNLOAD_PECTAB 1
set ENABLE_REQUEST_DUP 0
set TEST_SERVER 0
set USE_SEANCES 0

set SERVER_ID beta

set CREATE_SPP_DAYS 2
set ARX_MIN_DAYS 120
set ARX_MAX_DAYS 120
set ARX_DURATION 20
set ARX_SLEEP 10
set ARX_MAX_ROWS 1000
#set ARX_TRIP_DATE_RANGE 1

#set APIS_PARTY_INFO SIRENA-TRAVEL:4959504991:4959504973
#set_local MESPRO_PSE_PATH "./crypt"

set TLG_ACK_TIMEOUT 1000
set TLG_SND_PROC_INTERVAL 1000

set USE_RSYSLOG 0
set ONE_LOGGER 0
set START_EMAIL_SENDER 0

