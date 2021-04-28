#include "PgOraConfig.h"
#include "hooked_session.h"
#include "exceptions.h"

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
        { "SP_PG_GROUP_PP_TRIP_TASK", {"POSTPONED_TRIP_TASK"} },
        { "SP_PG_GROUP_IAPI",  { "IAPI_PAX_DATA" } },
        { "SP_PG_GROUP_APPS",  { "APPS_MESSAGES", "APPS_PAX_DATA", "APPS_MANIFEST_DATA"}},
        { "SP_PG_GROUP_IATCI", { "IATCI_TABS_SEQ", "IATCI_TABS", "IATCI_SETTINGS", "GRP_IATCI_XML", "DEFERRED_CKI_DATA", "CKI_DATA" } },
        { "SP_PG_GROUP_WC",    { "RL_SEQ", "WC_PNR", "WC_TICKET", "WC_COUPON", "AIRPORT_CONTROLS" } },
        { "SP_PG_GROUP_ET",    { "ETICKETS", "ETICKS_DISPLAY", "ETICKS_DISPLAY_TLGS" } },
        { "SP_PG_GROUP_EMD",   { "EMDOCS", "EMDOCS_DISPLAY" } },
        { "SP_PG_GROUP_CRS_SVC",{ "CRS_PAX_ASVC", "CRS_PAX_REM" } },
        { "SP_PG_GROUP_CRS_DOC",{ "CRS_PAX_DOC", "CRS_PAX_DOCA", "CRS_PAX_DOCO" } },
        { "SP_PG_GROUP_PAX_ALARMS",{ "PAX_ALARMS", "CRS_PAX_ALARMS" } },
        { "SP_PG_GROUP_PAX_1", { "CRS_PAX_FQT", "CRS_PAX_REFUSE", "CRS_PAX_CONTEXT" } },
        { "SP_PG_GROUP_CRS_DATA", { "CRS_DATA", "CRS_DATA_STAT", "CRS_COUNTERS", "TRIP_DATA" } },
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
        { "SP_PG_GROUP_TLG_QUE",   { "TLGS", "TLGS_TEXT", "TLG_QUEUE", "TLG_ERROR", "TLG_STAT" } },
        { "SP_PG_GROUP_COMP_1", {
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
        { "SP_PG_GROUP_CRYPT", { "CRYPT_FILE_PARAMS", "CRYPT_FILES", "CRYPT_REQ_DATA", "CRYPT_SERVER", "CRYPT_SETS", "CRYPT_TERM_CERT", "CRYPT_TERM_REQ" } },
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
        return result;
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
        return *get_main_ora_sess(STDLOG);
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
        return *get_main_ora_sess(STDLOG);
    }

    DbCpp::Session& getAutoSession(const std::string& objectName)
    {
        if (supportsPg(objectName))
        {
            return *get_main_pg_au_sess(STDLOG);
        }
        return *get_main_ora_sess(STDLOG);
    }

    std::string makeSeqNextVal(const std::string& sequenceName)
    {
        if (supportsPg(sequenceName)) {
            return ("SELECT NEXTVAL('" + sequenceName + "')");
        }
        return ("SELECT " + sequenceName + ".NEXTVAL FROM DUAL");
    }

    std::string makeSeqCurrVal(const std::string& sequenceName)
    {
        if (supportsPg(sequenceName)) {
            return ("SELECT CURRVAL('" + sequenceName + "')");
        }
        return ("SELECT " + sequenceName + ".CURRVAL FROM DUAL");
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
        return getSeqNextValInner<int>(sequenceName);
    }

    unsigned long getSeqCurrVal_ul(const std::string& sequenceName)
    {
        return getSeqCurrValInner<unsigned long>(sequenceName);
    }

} // namespace PgOra

#include "stdio.h"
#include <serverlib/cursctl.h>

static int count_ora_tabs()
{
    int cnt=0;
    auto cur = make_curs("select count(*) from user_tables");
    cur.def(cnt).EXfet();
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
