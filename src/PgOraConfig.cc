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
        { "SP_PG_GROUP_IAPI",  { "IAPI_PAX_DATA" } },
        { "SP_PG_GROUP_IATCI", { "IATCI_TABS_SEQ", "IATCI_TABS", "IATCI_SETTINGS", "GRP_IATCI_XML", "DEFERRED_CKI_DATA", "CKI_DATA" } },
    };

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
        return getGroupInner(objectName, sGroups);
    }

    bool supportsPg(const std::string& objectName)
    {
        const std::string& group = getGroup(objectName);
        return group == "" ? false
                           : Config(getGroup(objectName)).writePostgres();
    }

    DbCpp::Session& getROSession(const std::string& objectName)
    {
        if (supportsPg(objectName))
        {
            const std::string& group = getGroup(objectName);
            if (group == "SP_PG_GROUP_ARX")
            {
                return *get_arx_pg_ro_sess(STDLOG);
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

    long getSeqNextVal(const std::string& sequenceName)
    {
        long result;
        make_db_curs(makeSeqNextVal(sequenceName),
                     getRWSession(sequenceName))
                .def(result)
                .EXfet();
        return result;
    }

} // namespace PgOra
