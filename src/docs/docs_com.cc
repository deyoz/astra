#include "docs_com.h"
#include "typeb_utils.h"
#include "telegram.h"
#include "salonform.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;

namespace DOCS {

void COM(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form("WB_msg", reqNode, resNode);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "msg");

    TypeB::TCreateInfo info("COM", TypeB::TCreatePoint());
    info.point_id = rpt_params.point_id;
    TTypeBTypesRow tlgTypeInfo;
    int tlg_id = TelegramInterface::create_tlg(info, ASTRA::NoExists, tlgTypeInfo, true);
    TCachedQuery TlgQry("SELECT * FROM tlg_out WHERE id=:id ORDER BY num",
            QParams() << QParam("id", otInteger, tlg_id));
    TlgQry.get().Execute();
    for(;!TlgQry.get().Eof;TlgQry.get().Next())
    {
        TTlgOutPartInfo tlg;
        tlg.fromDB(TlgQry.get());
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "text", tlg.heading + tlg.body + tlg.ending);
    }

    SALONS2::TBuildMap seats;
    SALONS2::TSalonList salonList(true);
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( rpt_params.point_id, NoExists ), {}, NoExists );
    salonList.Build(seats);

    static const int row_size = 60;
    ostringstream seats_output;
    for(const auto &occupy: seats) {
        seats_output << (occupy.first ? "Занятые места" : "Свободные места") << endl;
        string row;
        for(const auto &seat: occupy.second) {
            if(not row.empty())
                row += ", ";
            if((row + seat).size() > row_size) {
                seats_output << row << endl;
                row = seat;
            } else
                row += seat;
        }
        if(not row.empty())
            seats_output << row << endl << endl;
    }

    NewTextChild(NewTextChild(dataSetNode, "row"), "text", seats_output.str());

    /*
    std::vector<SALONS2::TCompSectionLayers> CompSectionsLayers;
    vector<TZoneOccupiedSeats> zoneSeats, notZoneSeats;
    std::vector<SALONS2::TCompSection> compSections;
    ZoneLoads(rpt_params.point_id, false, false, false, zoneSeats, notZoneSeats, CompSectionsLayers, compSections);
    LogTrace(TRACE5) << "Occupy";
    for(const auto &zone: zoneSeats) {
        LogTrace(TRACE5) << "name: " << zone.name;
        for(const auto &seat: zone.seats)
            LogTrace(TRACE5) << seat.yname << seat.xname;
    }
    LogTrace(TRACE5) << "Not occupy";
    for(const auto &zone: notZoneSeats) {
        LogTrace(TRACE5) << "name: " << zone.name;
        for(const auto &seat: zone.seats)
            LogTrace(TRACE5) << seat.yname << seat.xname;
    }
    */

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "caption", EncodeRptType(rpt_params.rpt_type));
    ASTRA::rollback();
}

}
