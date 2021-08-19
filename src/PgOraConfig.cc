#include "PgOraConfig.h"
#include "hooked_session.h"
#include "exceptions.h"

#ifdef ENABLE_ORACLE
#include <serverlib/cursctl.h>
#endif
#include <serverlib/algo.h>
#include <serverlib/dbcpp_cursctl.h>
#include <serverlib/dbcpp_session.h>
#include <serverlib/str_utils.h>
#include <serverlib/tcl_utils.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace PgOra
{
    Config::Config(const std::string& tcl)
        : mCfg((tcl == PgOra_config_OnlyPGparam()) ? 3 : readIntFromTcl(tcl, 0))
    {
        if (mCfg < 0 || mCfg > 3)
        {
            throw EXCEPTIONS::Exception("Invalid " + tcl + " value");
        }
    }

    using GroupsType = std::vector<std::pair<std::string, std::vector<std::string>>>;

    static const GroupsType sGroups {
        { "SP_PG_GROUP_JXTCONT", {"CONT"} },
        { "SP_PG_GROUP_FILE", {"FILE_ERROR", "FILE_QUEUE", "FILES", "FILE_PARAMS"} },
        { "SP_PG_GROUP_FILE2", {"FILE_SETS"} },
        { "SP_PG_CONVERT_TABS", {"CONVERT_AIRLINES", "CONVERT_AIRPS", "CONVERT_CRAFTS", "CONVERT_LITERS"} },
        { "SP_PG_GROUP_FILE_CFG", {"FILE_ENCODING", "FILE_TYPES"} },
        { "SP_PG_GROUP_REG_PAX_1", {"PAX_FQT"} },
        { "SP_PG_GROUP_PP_TRIP_TASK", {"POSTPONED_TRIP_TASK"} },
        { "SP_PG_GROUP_IAPI",  { "IAPI_PAX_DATA" } },
        { "SP_PG_GROUP_APPS",  { "APPS_MESSAGES", "APPS_PAX_DATA", "APPS_MANIFEST_DATA", "APPS_MSG_ID__SEQ"}},
        { "SP_PG_GROUP_IATCI", { "IATCI_TABS_SEQ", "IATCI_TABS", "IATCI_SETTINGS", "GRP_IATCI_XML", "DEFERRED_CKI_DATA", "CKI_DATA" } },
        { "SP_PG_GROUP_WC",    { "RL_SEQ", "WC_PNR", "WC_TICKET", "WC_COUPON", "AIRPORT_CONTROLS" } },
        { "SP_PG_GROUP_ET",    { "ETICKETS", "ETICKS_DISPLAY", "ETICKS_DISPLAY_TLGS" } },
        { "SP_PG_GROUP_EMD",   { "EMDOCS", "EMDOCS_DISPLAY" } },
        { "SP_PG_GROUP_CRS_SVC",{ "CRS_PAX_ASVC", "CRS_PAX_REM" } },
        { "SP_PG_GROUP_CRS_DOC",{ "CRS_PAX_DOC", "CRS_PAX_DOCA", "CRS_PAX_DOCO" } },
        { "SP_PG_GROUP_PAX_ALARMS",{ "PAX_ALARMS", "CRS_PAX_ALARMS" } },
        { "SP_PG_GROUP_CRS_PAX_1", { "CRS_PAX_FQT", "CRS_PAX_REFUSE", "CRS_PAX_CONTEXT" } },
        { "SP_PG_GROUP_CRS_DATA", { "CRS_DATA", "CRS_DATA_STAT", "CRS_COUNTERS", "TRIP_DATA" } },
        { "SP_PG_GROUP_CRS_SET", { "CRS_SET", "HIST_CRS_SET" } },
        { "SP_PG_GROUP_DCS_BAG", { "DCS_BAG", "DCS_TAGS" } },
        { "SP_PG_GROUP_CRS_TRANSFER", { "CRS_TRANSFER" } },
        { "SP_PG_GROUP_TLG_TRANSFER", { "TLG_TRANSFER", "TLG_TRFER_EXCEPTS", "TLG_TRFER_ONWARDS", "TRFER_GRP", "TRFER_PAX", "TRFER_TAGS" } },
        { "SP_PG_GROUP_TYPEB_DATA_STAT", { "TYPEB_DATA_STAT" } },
        { "SP_PG_GROUP_CRS_DISPLACE2", { "CRS_DISPLACE2" } },
        { "SP_PG_GROUP_TLG_TRIPS", { "TLG_TRIPS", "TLG_BIND_TYPES" } },
        { "SP_PG_GROUP_CRS_TKN",{ "CRS_PAX_TKN" } },
        { "SP_PG_GROUP_TYPEB_1",{ "CRS_SEATS_BLOCKING", "PNR_MARKET_FLT", "PNR_ADDRS", "PNR_ADDRS_PC" } },
        { "SP_PG_GROUP_ANNUL_BAG",{ "ANNUL_BAG", "ANNUL_TAGS", "BI_STAT" } },
        { "SP_PG_GROUP_PAID_RFISC",{ "PAID_RFISC" } },
        { "SP_PG_GROUP_CONFIRM_PRINT",{ "CONFIRM_PRINT", "CONFIRM_PRINT_VO_UNREG", "STAT_REPRINT" } },
        { "SP_PG_GROUP_ROZYSK",{ "ROZYSK" } },
        { "SP_PG_GROUP_TAGS_GENERATED",{ "SBDO_TAGS_GENERATED", "BAG_TAGS_GENERATED" } },
        { "SP_PG_GROUP_STAT_1",{ "STAT_AD", "STAT_SERVICES", "TRFER_PAX_STAT", "BASEL_STAT" } },
        { "SP_PG_GROUP_PAX_SERVICES",{ "SERVICE_PAYMENT", "PAX_SERVICES", "PAX_SERVICES_AUTO" } },
        { "SP_PG_GROUP_SERVICE_LISTS",{ "PAX_BRANDS", "PAX_NORMS_TEXT", "SVC_PRICES",
                                        "PAX_SERVICE_LISTS", "GRP_SERVICE_LISTS", "SERVICE_LISTS",
                                        "SERVICE_LISTS_GROUP", "RFISC_LIST_ITEMS", "BAG_TYPE_LIST_ITEMS"} },
        { "SP_PG_GROUP_PAX_EVENTS",{ "PAX_EVENTS", "PAX_CUSTOM_ALARMS" } },
        { "SP_PG_GROUP_BAG_RECEIPTS",{ "BAG_PREPAY", "BAG_RECEIPTS", "BAG_RCPT_KITS", "BAG_PAY_TYPES" } },
        { "SP_PG_GROUP_HOTEL_ACMD",{ "HOTEL_ACMD_DATES", "HOTEL_ACMD_FREE_PAX", "HOTEL_ACMD_PAX", "HOTEL_ACMD", "HIST_HOTEL_ACMD", "HOTEL_ROOM_TYPES", "SOFI_FILES" } },
        { "SP_PG_GROUP_KASSA",{ "PAX_NORMS", "GRP_NORMS",
                                "BAG_NORMS", "HIST_BAG_NORMS",
                                "BAG_RATES", "HIST_BAG_RATES",
                                // "VALUE_BAG_TAXES", "HIST_VALUE_BAG_TAXES", // VALUE_BAG
                                "EXCHANGE_RATES", "HIST_EXCHANGE_RATES",
                                "PAID_BAG", "PAID_BAG_EMD_PROPS" } },
        { "SP_PG_GROUP_AODB_1",{ "AODB_BAG_NAMES", "AODB_BAG", "AODB_PAX", "AODB_UNACCOMP",
                                 "AODB_SPP_FILES", "AODB_EVENTS", "SNAPSHOT_PARAMS", "MERIDIAN_AIRPS_OWNER" } },
        { "SP_PG_GROUP_AODB_STATIONS",{ "AODB_STATIONS", "AODB_PAX_CHANGE" } }, // STATIONS
        { "SP_PG_GROUP_AODB_POINTS",{ "SNAPSHOT_POINTS", "AODB_POINTS" } }, // POINTS
        { "SP_PG_GROUP_TRIP_VOUCHERS",{ "TRIP_VOUCHERS", "VOUCHER_TYPES" } },
        { "SP_PG_GROUP_PAX_TRANSLIT",{ "PAX_TRANSLIT", "CRS_PAX_TRANSLIT", "TEST_PAX_TRANSLIT" } }, // new tables
        { "SP_PG_GROUP_STAT_ORDERS",{
                "STAT_ORDERS",
                "STAT_ORDERS_DATA",
        } },
        { "SP_PG_GROUP_BAG_NAMES",{
                "STAT_FV_BAG_NAMES",
                "RPT_BM_BAG_NAMES"
        } },
        { "SP_PG_GROUP_STAT_2",{
                "STAT_TYPES",
                "STAT_LEVELS",
                "STAT_PARAMS",
                "STAT_ZAMAR",
        } },
        { "SP_PG_GROUP_STAT_3",{
                "STAT",                    // "POINTS","TRFER_STAT","PACTS"
                "LIMITED_CAPABILITY_STAT", // "POINTS"
                "STAT_HA",                 // "POINTS"
                "STAT_VO",                 // "POINTS"
                "STAT_REM",                // "POINTS","USERS2",
                "TRFER_STAT",              // "POINTS","STAT","PACTS"
                "AGENT_STAT",              // "POINTS","USERS2", agent_stat_t
                "SELF_CKIN_STAT",          // "POINTS"
                "PFS_STAT",                // "POINTS"
                "RFISC_STAT",              // "POINTS"
                "PACTS",                   // "POINTS","TRFER_STAT","STAT"
        } },
        { "SP_PG_GROUP_KUF_STAT",{
                "KUF_STAT",                // "KUF_STAT_TEXT"
                "KUF_STAT_AIRLINES",
                "KUF_STAT_AIRPS",
                "KUF_STAT_TEXT",           // "KUF_STAT"
        } },


        { "SP_PG_GROUP_WB",    { "WB_MSG", "WB_MSG_TEXT" } },
        { "SP_PG_GROUP_SCHED", { "SCHED_DAYS", "SEASON_SPP", "ROUTES", "SSM_SCHEDULE"} },
        { "SP_PG_GROUP_SCHED_SEQ", {"ROUTES_MOVE_ID", "ROUTES_TRIP_ID", "SSM_ID"} },
        { "SP_PG_GROUP_CONTEXT", {"CONTEXT", "CONTEXT__SEQ"} },
        { "SP_PG_GROUP_EDIFACT", {"EDISESSION"} },
        { "SP_PG_GROUP_ARX", {"ARX_AGENT_STAT"    , "ARX_BAG_RATES"      , "ARX_GRP_NORMS"  , "ARX_PAX_DOC"         , "ARX_RFISC_STAT"    , "ARX_STAT"            , "ARX_TRANSFER_SUBCLS"   ,
                              "ARX_ANNUL_BAG"     , "ARX_BAG_RECEIPTS"   , "ARX_PAX_GRP"    , "ARX_SELF_CKIN_STAT"  , "ARX_STAT_VO"       , "ARX_TRFER_PAX_STAT"  , "ARX_UNACCOMP_BAG_INFO" ,
                              "ARX_ANNUL_TAGS"    , "ARX_BAG_TAGS"       , "ARX_MARK_TRIPS" , "ARX_PAX_NORMS"       , "ARX_STAT_AD"       , "ARX_STAT_ZAMAR"      , "ARX_TRFER_STAT"        ,
                              "ARX_BAG2"          , "ARX_BI_STAT"        , "ARX_MOVE_REF"   , "ARX_PAX_REM"         , "ARX_STAT_HA"       , "ARX_TCKIN_SEGMENTS"  , "ARX_TRIP_CLASSES"      ,
                              "ARX_BAG_NORMS"     , "ARX_CRS_DISPLACE2"  , "ARX_PAID_BAG"   , "ARX_PAX"             , "ARX_STAT_REM"      , "ARX_TLG_OUT"         , "ARX_TRIP_DELAYS"       ,
                              "ARX_BAG_PAY_TYPES" , "ARX_EVENTS"         , "ARX_PAX_DOCA"   , "ARX_PFS_STAT"        , "ARX_STAT_REPRINT"  , "ARX_TLG_STAT"        , "ARX_TRIP_LOAD"         ,
                              "ARX_BAG_PREPAY"    , "ARX_EXCHANGE_RATES" , "ARX_PAX_DOCO"   , "ARX_POINTS"          , "ARX_STAT_SERVICES" , "ARX_TRANSFER"        , "ARX_TRIP_SETS"         ,
                              "MOVE_ARX_EXT"      , "ARX_VALUE_BAG"      , "ARX_TRIP_STAGES", "ARX_VALUE_BAG_TAXES" , "ARX_LIMITED_CAPABILITY_STAT", "ARX_PAY_SERVICES" }},
        { "SP_PG_GROUP_SPPCKIN", {"TRIP_ALARMS", "TRIP_APIS_PARAMS", "TRIP_RPT_PERSON", "UTG_PRL"} },
        { "SP_PG_GROUP_SPPCKIN2", {"COUNTERS_BY_SUBCLS", "COUNTERS2", "TRIP_CREW", "TRIP_LOAD"} },
        { "SP_PG_GROUP_TLG_QUE",   { "TLGS", "TLGS_TEXT", "TLG_QUEUE", "TLG_ERROR", "TLG_STAT" } },
        { "SP_PG_GROUP_COMP", {
            "COMPARE_COMP_LAYERS",
            "COMP_LAYER_TYPES",
            "COMP_LAYER_PRIORITIES",
            "COMPART_DESC_TYPES",
            "COMPART_DESC_SETS",
            "COMP_LAYER_RULES",
            "COMP_TARIFF_COLORS",
            "COMP_ELEM_TYPES",
        }},
        { "SP_PG_GROUP_HTML", { "HTML_PAGES", "HTML_PAGES_TEXT" } },
        { "SP_PG_GROUP_FR", { "FR_FORMS2" } },
        { "SP_PG_GROUP_LIBTLG", { "TEXT_TLG_H2H" } },
        { "SP_PG_GROUP_PP_TLG", { "POSTPONED_TLG", "POSTPONED_TLG_CONTEXT" } },
        { "SP_PG_GROUP_CRYPT", { "CRYPT_FILE_PARAMS", "CRYPT_FILES"   , "CRYPT_REQ_DATA"     , "CRYPT_SERVER"   , "CRYPT_SETS",
                                 "CRYPT_TERM_CERT"  , "CRYPT_TERM_REQ", "HIST_CRYPT_REQ_DATA", "HIST_CRYPT_SETS" } },
        { "SP_PG_GROUP_TYPEB_IN", { "TLGS_IN", "TYPEB_PARSE_PROCESSES", "TYPEB_IN_HISTORY", "TYPEB_IN_ERRORS", "TYPEB_IN_BODY", "TYPEB_IN" } },
        { "SP_PG_GROUP_TYPEB_OUT", { "TLG_OUT", "TYPEB_OUT_ERRORS", "TYPEB_OUT_EXTRA" } },
        { "SP_PG_GROUP_TLG_IN_OUT_SEQ", { "TLG_IN_OUT__SEQ" } },
        { "SP_PG_GROUP_STAGES", { "GRAPH_RULES"     , "GRAPH_STAGES"    , "GRAPH_TIMES"       , "STAGE_NAMES"      , "STAGE_SETS" ,
                                  "STAGE_STATUSES"  , "STAGE_TYPES"     , "CKIN_CLIENT_STAGES", "TRIP_FINAL_STAGES", "TRIP_STAGES",
                                  "TRIP_CKIN_CLIENT", "HIST_GRAPH_TIMES", "HIST_STAGE_NAMES"  , "HIST_STAGE_SETS" } },
        { "SP_PG_GROUP_BASETABLES", { "AIRLINES",         "AIRPS",        "CITIES",
                                      "CRAFTS",           "COUNTRIES"     "PERS_TYPES",
                                      "PAX_CATS",         "TRIP_TYPES",   "DELAYS",
                                      "TRIP_LITERS",      "REFUSAL_TYPES","TAG_TYPES",
                                      "RFIC_TYPES",       "PAY_TYPES",    "RCPT_DOC_TYPES",
                                      "PAX_DOC_SUBTYPES", "PAX_DOC_TYPES","PERS_WEIGHTS",
                                      "PAX_DOC_COUNTRIES","TAG_COLORS",   "GENDER_TYPES",
                                      "SEAT_DESCR",       "LANG_TYPES",   "CLASSES",
                                      "FORM_TYPES",       "FORM_PACKS",   "TYPEB_OPTION_VALUES"
        } },
        { "SP_PG_GROUP_SPP_SETS", { "LAST_TAG_RANGES2", "TRIP_CALC_DATA", "TRIP_AUTO_WEIGHING", "TRIP_BP", "TRIP_SETS", "TRIP_STATIONS"
                                    "TAG_RANGES2"     , "TRIP_PAID_CKIN", "TRIP_PERS_WEIGHTS" , "TRIP_BT", "TRIP_HALL",
        } },
        { "SP_PG_GROUP_ADM", { "ROT" } },
        { "SP_PG_GROUP_LOCALE_MESSAGES", { "LOCALE_MESSAGES", "CLIENT_ERROR_LIST" } },
        { "SP_PG_GROUP_LIBRA", { "LIBRA_PROC_LOG_EVENTS", "AHM_DICT", "AHM_ID__SEQ" } },


        // ASTRA MINIMUM
        { "SP_PG_GROUP_DESK", {
                "DESK_LOGGING",         // 4load: adm_cache_tables.ldr, cache_fields.ldr, cache_tables.ldr
                "DESK_TRACES",          // 4load: adm_cache_tables.ldr, cache_fielddesks.ldr, cache_tables.ldr
                "DESK_NOTICES",         // +
                "LOCALE_NOTICES",       // +
                "DESK_DISABLE_NOTICES", // +
                "DESK_OWNERS",          // 4load: adm_cache_tables.ldr, cache_fields.ldr, cache_tables.ldr, desk_owners.ldr
                                        // 7Proc: ADM
                "DESKS",                // 4load: adm_cache_tables.ldr, cache_fields.ldr, cache_tables.ldr, desks.ldr
                                        // 7Proc: ADM
                "DESK_GRP",
                "TERM_EXPIRE_DATES",    // +
        }},
        { "SP_PG_GROUP_PROFILES", {
                "AIRLINE_PROFILES",     // profile_rights
                                        // 4load: cache_fields.ldr, cache_tables.ldr
                                        // 7Proc: ADM
                "PROFILE_RIGHTS",       // 4load: adm_cache_tables.ldr, cache_fields.ldr, cache_tables.ldr
                                        // 7Proc: ADM
                                        // client_error_list,
                                        // // 7Proc: ADM, SYSTEM
        }},
        { "SP_PG_GROUP_RIGHTS", {
                "SCREEN",               // 4load: screen.ldr
                "SCREEN_RIGHTS",        // user_roles,role_rights,screen
                "USER_ROLES",           // rights_list,role_rights,role_assign_rights,
                                        // adm.cc: LoadAdm(): adm_cache_tables,cache_tables,user_roles,role_rights
                                        // 4load: cache_tables.ldr, user_roles.ldr, history_tables.ldr,
                                        // 7Proc: ADM
                "ROLE_RIGHTS",          // rights_list,role_rights,role_assign_rights,
                                        // adm.cc: LoadAdm(): adm_cache_tables,cache_tables,user_roles,role_rights
                                        // 4load: cache_tables.ldr, role_rights.ldr, history_tables.ldr,
                                        // 7Proc: ADM
                "ROLE_ASSIGN_RIGHTS",   // access.cc: AccessInterface::Clone
                                        // 4load: cache_tables.ldr, role_assign_rights.ldr, history_tables.ldr,
                                        // 7Proc: ADM
                "ARO_AIRLINES",         // access.cc: TARO, TAirlinesARO
                                        // 4load: history_tables.ldr,
                                        // 7Proc: ADM
                "ARO_AIRPS",            // access.cc: TARO, TAirpsARO
                                        // 4load: history_tables.ldr,
                                        // 7Proc: ADM
        }},
        { "SP_PG_GROUP_USERS", {
                "USERS2",               // user_sets, user_set_types, desks, web_clients
                                        // 4load: cache_tables.ldr, users2.ldr, history_tables.ldr,
                                        // 7Proc: ADM
                                        // brd.cc: BrdInterface::GetPaxQuery()
                                        // sofi.cc: createSofiFile()
                                        // stat_agent.cc: RunAgentStat()
                                        // stat_arx.cc: 7Proc: ADM: adm.check_user_access
                                        // stat_rem.cc: RunRemStat()
                                        // stat_rfisc.cc: get_rfisc_stat()
                                        // stat_unacc.cc: RunUNACCStat()
                                        // stat_rfisc.cc: get_rfisc_stat()
                "USER_SETS",            // users2,user_set_types
                "USER_SET_TYPES",       // base_tables.h
                                        // 4load: cache_tables.ldr, user_set_types.ldr,
                "PRN_FORMS_LAYOUT",     // 4load: cache_fields.ldr, cache_tables.ldr
                "WEB_CLIENTS",
        }},
        { "SP_PG_GROUP_DEV", {
                "DEV_EVENT_CMD",
                "DEV_FMT_OPERS",        // "DEV_MODEL_PARAMS","DEV_MODEL_SESS_FMT","DEV_SESS_MODES","DEV_OPER_TYPES","DEV_MODELS"
                "DEV_FMT_TYPES",
                "DEV_MODEL_DEFAULTS",   // "DEV_OPER_TYPES"
                "DEV_MODEL_FMT_PARAMS",
                "DEV_MODEL_PARAMS",     // "DEV_SESS_MODES","DEV_FMT_OPERS"
                "DEV_MODEL_SESS_FMT",   // "DEV_SESS_MODES", "DEV_FMT_OPERS","DEV_OPER_TYPES","DEV_MODELS"
                "DEV_MODELS",           // "DEV_FMT_OPERS","DEV_MODEL_SESS_FMT"
                "DEV_OPER_TYPES",       // "DEV_MODEL_DEFAULTS","DEV_MODELS","DEV_MODEL_SESS_FMT","DEV_SESS_MODES","DEV_FMT_OPERS"
                "DEV_SESS_MODES",       // "DEV_MODEL_PARAMS","DEV_MODEL_SESS_FMT","DEV_FMT_OPERS","DEV_OPER_TYPES","DEV_MODELS","DEV_SESS_MODES"
                "DEV_SESS_TYPES",
        }},
        { "SP_PG_GROUP_CACHE_TABLES", {
                "CACHE_TABLES",         // 7Proc: ADM
                "CACHE_FIELDS",
                "CACHE_CHILD_TABLES",
                "CACHE_CHILD_FIELDS",
                "ADM_CACHE_TABLES",
        }},
        { "SP_PG_GROUP_COUNTERS", {
                "COUNTERS2",
                "COUNTERS_BY_SUBCLS"
        }},
        { "SP_PG_GROUP_CRS_PAX", {
                "CRS_PAX",
                "CRS_PNR",         // "CRS_PAX", "TLG_BINDING", "PAX", "PAX_GRP", salons.get_crs_seat_no, SALONS
                "CRS_INF",         // "PAX_GRP", "PAX", "CRS_INF_DELETED", "CRS_PNR", "CRS_PAX", "CRS_PAX_CHKD", salons.get_crs_seat_no
                "CRS_INF_DELETED", // "CRS_PAX", "CRS_PNR", "CRS_INF"
                "CRS_RBD",         // "CRS_PAX", "CRS_PNR", "SUBCLS", "CLASSES", "TLG_BINDING"
                "CRS_PAX_CHKD",    // "CRS_PAX", "CRS_PNR", "PAX", "CRS_INF", salons.get_crs_seat_no
        }},
        { "SP_PG_GROUP_PAX", {
                "PAX",      // !!!
                "PAX_GRP",  // !!!
                "PAX_EMD",  // "PAX", "PAX_GRP", "PAX_ASVC"
                "PAX_ASVC", // "PAX", "PAX_GRP", "PAX_EMD", "TCKIN_PAX_GRP", "PAX_DOC", "PAX_DOCO", "PAX_DOCA", "PAX_REM", "TRANSFER", "TRFER_TRIPS", "TRIP_CLASSES", "COUNTERS2", "PAX_FQT", "BAG2"
                "PAX_REM",  // "PAX", "PAX_GRP", "PAX_ASVC", "PAX_DOC", "PAX_DOCO", "PAX_DOCA", "COMP_REM_TYPES", "TRANSFER", "TRFER_TRIPS", "TRIP_CLASSES", "COUNTERS2", "PAX_FQT", "BAG2"
                "PAX_REM_ORIGIN", // "PAX", "PAX_GRP", "POINTS"
                "PAX_CALC_DATA",  // "PAX", "PAX_GRP", "CRS_PAX", "CRS_PNR", "TCKIN_PAX_GRP", "PAX_CALC_DATA", "USERS2", "TLG_BINDING"
                "PAX_DOC",  // "PAX", "PAX_GRP", "PAX_DOCO", "PAX_DOCA", "PAX_ASVC", "PAX_REM", "POINTS", "AIRPS", "TRANSFER", "TRFER_TRIPS", "TRIP_CLASSES", "COUNTERS2", "PAX_FQT", "BAG2", salons.get_seat_no
                "PAX_DOCA", // "PAX", "PAX_GRP", "PAX_DOC", "PAX_DOCO", "PAX_ASVC", "PAX_REM", "TRANSFER", "TRFER_TRIPS", "TRIP_CLASSES", "COUNTERS2", "PAX_FQT", "BAG2"
                "PAX_DOCO", // "PAX", "PAX_GRP", "PAX_DOC", "PAX_DOCA", "PAX_ASVC", "PAX_REM", "TRANSFER", "TRFER_TRIPS", "TRIP_CLASSES", "COUNTERS2", "PAX_FQT", "BAG2"
                "TEST_PAX", // "PAX","CRS_PAX","CRS_PNR","SUBCLS","CLS_GRP",
                "TCKIN_PAX_GRP",  // "PAX","PAX_GRP","BAG2","EVENTS_BILINGUAL","STATIONS","WEB_CLIENTS","DESKS","DESK_GRP","POINTS","CRS_PAX","PAX_CALC_DATA","USERS2","PAX_ASVC","CLS_GRP","HALLS2","TRANSFER","TRFER_TRIPS"
                "TCKIN_SEGMENTS", // "PAX","PAX_GRP","MARK_TRIPS","TRANSFER","TRFER_TRIPS"
                "MARK_TRIPS",     // "PAX","PAX_GRP","TRANSFER","TRFER_TRIPS","TCKIN_SEGMENTS","POINTS"
        }},
        { "SP_PG_GROUP_TRIP_TASK", {
                "TRIP_CLASSES",
                "TRIP_TASKS",
                "TRIP_TASK_PROCESSES",
        }},
        { "SP_PG_GROUP_FILE_PARAM_SETS", {
                "FILE_PARAM_SETS",
        }},
        { "SP_PG_GROUP_TYPEB_TYPES", {
                "TYPEB_TYPES",
        }},
        { "SP_PG_GROUP_TYPEB_ADDRS", {
                "TYPEB_ADDR_OPTIONS",
                "TYPEB_ADDR_OWNERS",
                "TYPEB_ADDRS",              // "POINTS"
                "TYPEB_ADDR_TRANS_PARAMS",
                "TYPEB_CREATE_POINTS",
                "TYPEB_OPTIONS"
        }},
        { "SP_PG_GROUP_CRS_PNR", {
                "CRS_PNR",
                "CRS_PAX",
                "TLG_BINDING", // "CRS_PAX", "CRS_PNR", "TLG_SOURCE", "TYPEB_IN_HISTORY", "POINTS", "PAX", "PAX_GRP", "TRIP_SETS", "TRIP_PAID_CKIN", "TLG_COMP_LAYERS", "TLGS_IN", "TLG_SOURCE", salons.get_crs_seat_no, SALONS
        }},
        { "SP_PG_GROUP_TRANSFER", {
                "TRANSFER",
                "TRANSFER_SUBCLS", // "PAX","PAX_GRP","SUBCLS","CLASSES","TRANSFER","TRFER_TRIPS"
        }},
        { "SP_PG_GROUP_TRIP_CALC", {
                "TRIP_CALC_DATA",
                "TRIP_COMP_ELEMS",
        }},
        { "SP_PG_GROUP_STATIONS", {
                "TRIP_STATIONS",
                "STATIONS",
        }},
        // ASTRA MINIMUM END
        { "SP_PG_GROUP_TASKS", {
                "TASKS",
        }},
        { "SP_PG_GROUP_TAG_PACKS", {
                "TAG_PACKS",
        }},
        { "SP_PG_GROUP_TRFER_TRIPS", {
                "TRFER_TRIPS", //"PAX","PAX_GRP","MARK_TRIPS","TRANSFER","TCKIN_SEGMENTS","TRANSFER_SUBCLS","SUBCLS","BAG2","TRIP_STAGES"
        }},
        { "SP_PG_GROUP_COMP_BASELAYERS", {
                "COMP_BASELAYERS", // COMP_ELEMS
                "TRIP_COMP_BASELAYERS", // COMP_ELEMS, COMP_BASELAYERS
        }},
        { "SP_PG_GROUP_COMP_CLASSES", {
                "COMP_CLASSES", // COMPS
        }},
        { "SP_PG_GROUP_COMP_ELEMS", {
                "COMP_ELEMS", // TRIP_COMP_ELEMS, TRIP_COMP_BASELAYERS, COMP_BASELAYERS
        }},
        { "SP_PG_GROUP_COMP_RATES", {
                "COMP_RATES", // TRIP_COMP_RATES
        }},
        { "SP_PG_GROUP_COMP_REM", {
                "COMP_REM", // TRIP_COMP_REM
        }},
        { "SP_PG_GROUP_COMP_REM_TYPES", {
                "COMP_REM_TYPES", // PAX_REM
        }},
        { "SP_PG_GROUP_COMP_RFISC", {
                "COMP_RFISC", // TRIP_COMP_RFISC
        }},
        { "SP_PG_GROUP_COMP_SECTIONS", {
                "COMP_SECTIONS",
        }},
        { "SP_PG_GROUP_COMP_COMPS", {
                "COMPS", // POINTS, LIBRA_COMPS, COMP_CLASSES, TRIP_SETS
        }},
        { "SP_PG_GROUP_COMP_SUBCLS_SETS", {
                "COMP_SUBCLS_SETS",
        }},
        { "SP_PG_GROUP_COMP_RFISC_COMP_PROPS", {
                "RFISC_COMP_PROPS", // BRAND_FARES, RFISC_RATES
        }},
        { "SP_PG_GROUP_COMP_TRIP_COMP_RATES", {
                "TRIP_COMP_RATES", // COMP_RATES
        }},
        { "SP_PG_GROUP_BAG2", {
                "BAG2",
        }},
        { "SP_PG_GROUP_OPERATORS", {"OPERATORS","HIST_OPERATORS"} },
        { "SP_PG_GROUP_SALE_POINTS", {"SALE_DESKS","SALE_POINTS","HIST_SALE_DESKS","HIST_SALE_POINTS"} },
        { "SP_PG_GROUP_POS_TERM", {
                "PAY_CLIENTS", "POS_TERM_ASSIGN", "POS_TERM_SETS", "POS_TERM_VENDORS",
                "HIST_PAY_CLIENTS", "HIST_POS_TERM_SETS"
        }},
        { "SP_PG_GROUP_VALIDATOR_TYPES", {"VALIDATOR_TYPES","HIST_VALIDATOR_TYPES"} },
        { "SP_PG_GROUP_PLACE_CALC", {"PLACE_CALC","HIST_PLACE_CALC"} },
        { "SP_PG_GROUP_KIOSKS", {"KIOSK_EVENT__SEQ", "KIOSK_ADDR", "KIOSK_ALIASES", "KIOSK_CONFIG_LIST",
                                 "KIOSK_EVENT_ERRORS", "KIOSK_EVENTS", "KIOSK_GRP", "KIOSK_ALIASES_LIST",
                                 "KIOSK_APP_LIST", "KIOSK_CONFIG", "KIOSK_EVENT_PARAMS", "KIOSK_GRP_NAMES",
                                 "KIOSK_LANG", "HIST_KIOSK_APP_LIST", "HIST_KIOSK_ADDR",
                                 "HIST_KIOSK_ALIASES", "HIST_KIOSK_CONFIG"}},
        { "SP_PG_GROUP_TYPEB_ORIGINATORS", {
                "TYPEB_ORIGINATORS",
                "HIST_TYPEB_ORIGINATORS"
        }},
        { "SP_PG_GROUP_TRIP_COMP_RFISC", {
                "TRIP_COMP_RFISC", // COMP_RFISC
        }},
    };

    static std::string getGroupByName(std::string objectName, const GroupsType& groups)
    {
        if (!objectName.empty())
        {
            objectName = StrUtils::ToUpper(objectName);
            for (const auto& group : groups)
            {
                if (group.first == objectName) {
                    return group.first;
                }
            }
        }
        return std::string();
    }

    static std::string getGroupInner(std::string objectName, const GroupsType& groups)
    {
        if (!objectName.empty())
        {
            objectName = StrUtils::ToUpper(objectName);
            for (const auto& group : groups)
            {
                if (algo::any_of(group.second,
                                 [&](const std::string& objName) { return objName == objectName; }))
                {
                    return group.first;
                }
            }
        }
        return std::string();
    }

    std::string getGroup(const std::string& objectName)
    {
        std::string result = getGroupInner(objectName, sGroups);
        if (result.empty()) {
            return getGroupByName(objectName, sGroups);
        }
        return result;
    }

    bool supportsPg(const std::string& objectName)
    {
        LogTrace(TRACE6) << __func__ << ": objectName=" << objectName;
        const std::string& group = getGroup(objectName);
        LogTrace(TRACE6) << __func__ << ": group=" << group;
        bool result = group.empty() ? false
                                    : Config(group).writePostgres();
        LogTrace(TRACE6) << __func__ << ": result=" << result;
#ifdef ENABLE_ORACLE
        return result;
#else
        LogWarning(STDLOG) << "Force PG support for obj: " << objectName;
        return true;
#endif
    }

    bool supportsPg(const std::initializer_list<std::string>& objects)
    {
      for (auto &obj : objects)
      {
        if(!supportsPg(obj))
          return false;
      }
      return true;
    }

    DbCpp::Session& getROSession(const std::string& objectName)
    {
        if (supportsPg(objectName))
        {
            const std::string& group = getGroup(objectName);
            if (group == "SP_PG_GROUP_ARX")
            {
                if(Config(group).readPostgres()) {
                    return *get_arx_pg_ro_sess(STDLOG);
                } else {
                    return *get_main_ora_sess(STDLOG);
                }
            }
            return *get_main_pg_ro_sess(STDLOG);
        }
#ifdef ENABLE_ORACLE
        return *get_main_ora_sess(STDLOG);
#else
        LogError(STDLOG) << "Force PG session for obj: " << objectName;
        return *get_main_pg_ro_sess(STDLOG);
#endif
    }

    DbCpp::Session& getRWSession(const std::string& objectName)
    {
        if (supportsPg(objectName))
        {
            const std::string& group = getGroup(objectName);
            if (group == "SP_PG_GROUP_ARX")
            {
                return *get_arx_pg_rw_sess(STDLOG);
            }
            return *get_main_pg_rw_sess(STDLOG);
        }
#ifdef ENABLE_ORACLE
        return *get_main_ora_sess(STDLOG);
#else
        LogError(STDLOG) << "Force PG session for obj: " << objectName;
        return *get_main_pg_rw_sess(STDLOG);
#endif
    }

    DbCpp::Session& getAutoSession(const std::string& objectName)
    {
        if (supportsPg(objectName))
        {
            return *get_main_pg_au_sess(STDLOG);
        }
#ifdef ENABLE_ORACLE
        return *get_main_ora_sess(STDLOG);
#else
        LogError(STDLOG) << "Force PG session for obj: " << objectName;
        return *get_main_pg_au_sess(STDLOG);
#endif
    }

    bool areROSessionsEqual(const std::list<std::string>& objects)
    {
        ASSERT(!objects.empty());
        auto dbType = getROSession(objects.front()).getType();
        for(auto obj: objects) {
            if(getROSession(obj).getType() != dbType) {
                return false;
            }
        }

        return true;
    }

    std::string traceObjects(const std::list<std::string>& objects)
    {
        std::string result;
        for(auto obj: objects) {
            if (!result.empty()) {
                result += ",";
            }
            result += obj;
        }
        return result;
    }

    bool areRWSessionsEqual(const std::list<std::string>& objects)
    {
        ASSERT(!objects.empty());
        auto dbType = getRWSession(objects.front()).getType();
        for(auto obj: objects) {
            if(getRWSession(obj).getType() != dbType) {
                LogError(STDLOG) << "Different session for " << obj << " in list: " <<  traceObjects(objects);
                return false;
            }
        }

        return true;
    }

    bool areAutoSessionsEqual(const std::list<std::string>& objects)
    {
        ASSERT(!objects.empty());
        auto dbType = getAutoSession(objects.front()).getType();
        for(auto obj: objects) {
            if(getAutoSession(obj).getType() != dbType) {
                return false;
            }
        }

        return true;
    }
#define ASSERT_OBJECT_LIST(check, list) \
    if(not check(list)) comtech::assertionFailed0Impl(STDLOG, (#check "{"+StrUtils::join(",",list)+"}").c_str());
    DbCpp::Session& getROSession(const std::list<std::string>& objectList)
    {
        ASSERT_OBJECT_LIST(areROSessionsEqual, objectList);
        return getROSession(objectList.front());
    }

    DbCpp::Session& getRWSession(const std::list<std::string>& objectList)
    {
        ASSERT_OBJECT_LIST(areRWSessionsEqual, objectList);
        return getRWSession(objectList.front());
    }

    DbCpp::Session& getAutoSession(const std::list<std::string>& objectList)
    {
        ASSERT_OBJECT_LIST(areAutoSessionsEqual, objectList);
        return getAutoSession(objectList.front());
    }
#undef ASSERT_OBJECT_LIST
    DbCpp::Session& getROSession(const std::initializer_list<std::string>& objects)
    {
        return getROSession(std::list<std::string>(objects));
    }

    DbCpp::Session& getRWSession(const std::initializer_list<std::string>& objects)
    {
        return getRWSession(std::list<std::string>(objects));
    }

    DbCpp::Session& getAutoSession(const std::initializer_list<std::string>& objects)
    {
        return getAutoSession(std::list<std::string>(objects));
    }

    std::string makeSeqNextVal(const std::string& sequenceName)
    {
        if (supportsPg(sequenceName)) {
            return ("SELECT NEXTVAL('" + sequenceName + "')");
        }
#ifdef ENABLE_ORACLE
        return ("SELECT " + sequenceName + ".NEXTVAL FROM DUAL");
#else
        LogError(STDLOG) << "Force PG syntax for seq: " << sequenceName;
        return ("SELECT NEXTVAL('" + sequenceName + "')");
#endif
    }

    std::string makeSeqCurrVal(const std::string& sequenceName)
    {
        if (supportsPg(sequenceName)) {
            return ("SELECT CURRVAL('" + sequenceName + "')");
        }
#ifdef ENABLE_ORACLE
        return ("SELECT " + sequenceName + ".CURRVAL FROM DUAL");
#else
        LogError(STDLOG) << "Force PG syntax for seq: " << sequenceName;
        return ("SELECT CURRVAL('" + sequenceName + "')");
#endif
    }

    template<class T>
    T getSeqNextValInner(const std::string& sequenceName)
    {
        T result;
        make_db_curs(makeSeqNextVal(sequenceName),
                     getRWSession(sequenceName))
                .def(result)
                .EXfet();
        return result;
    }

    long getSeqNextVal(const std::string& sequenceName)
    {
        return getSeqNextValInner<long>(sequenceName);
    }

    int getSeqNextVal_int(const std::string& sequenceName)
    {
        return getSeqNextValInner<int>(sequenceName);
    }

    unsigned long getSeqNextVal_ul(const std::string& sequenceName)
    {
        return getSeqNextValInner<unsigned long>(sequenceName);
    }

    template<class T>
    T getSeqCurrValInner(const std::string& sequenceName)
    {
        T result;
        make_db_curs(makeSeqCurrVal(sequenceName),
                     getRWSession(sequenceName))
                .def(result)
                .EXfet();
        return result;
    }

    long getSeqCurrVal(const std::string& sequenceName)
    {
        return getSeqCurrValInner<long>(sequenceName);
    }

    int getSeqCurrVal_int(const std::string& sequenceName)
    {
        return getSeqCurrValInner<int>(sequenceName);
    }

    unsigned long getSeqCurrVal_ul(const std::string& sequenceName)
    {
        return getSeqCurrValInner<unsigned long>(sequenceName);
    }

} // namespace PgOra

#include "stdio.h"
static int count_ora_tabs()
{
    int cnt=0;
#ifdef ENABLE_ORACLE
    auto cur = make_curs("select count(*) from user_tables");
    cur.def(cnt).EXfet();
#endif //ENABLE_ORACLE
    return cnt;
}

int print_pg_tables(int argc, char **argv)
{
    int tab_cnt = 0;
    std::vector<std::string> tabs;
    for(const auto &gr: PgOra::sGroups) {
        for(const auto &tab: gr.second) {
            tabs.push_back(tab);
            tab_cnt ++;
        }
    }
    const auto tab_ora_count = count_ora_tabs();
    std::cout << "total tabs in pg: " << tab_cnt
              << ", total in ora: " << tab_ora_count
              << " " << ((1.0 * tab_cnt)/tab_ora_count)*100 << "% moved." << std::endl;
    return 0;
}
