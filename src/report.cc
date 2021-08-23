#include "report.h"
#include "PgOraConfig.h"

#include <serverlib/dbcpp_cursctl.h>

#define NICKNAME "ANTON"
#include <serverlib/slogger.h>


namespace ASTRA {

std::optional<AirportCode_t> get_last_trfer_airp(const GrpId_t& grpId)
{
    auto cur = make_db_curs(
"select AIRP_ARV "
"from TRANSFER "
"where GRP_ID = :grp_id and PR_FINAL <> 0",
                PgOra::getROSession("TRANSFER"));

    std::string airp_arv;
    cur
            .def(airp_arv)
            .bind(":grp_id", grpId.get())
            .exfet();
    if(cur.err() != DbCpp::ResultCode::NoDataFound) {
        return AirportCode_t(airp_arv);
    }

    return {};
}

}//namespace ASTRA
