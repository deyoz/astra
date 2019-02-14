#include "services.h"
#include "docs/services.h"

void get_stat_services(int point_id)
{
    TCachedQuery delQry("delete from stat_services where point_id = :point_id", QParams() << QParam("point_id", otInteger, point_id));
    delQry.get().Execute();

    TTripInfo info;
    info.getByPointId(point_id);
    QParams insQryParams;
    insQryParams
        << QParam("point_id", otInteger, point_id)
        << QParam("scd_out", otDate, info.scd_out)
        << QParam("pax_id", otInteger)
        << QParam("list_id", otInteger)
        << QParam("rfisc", otString)
        << QParam("service_type", otString)
        << QParam("rfisc_airline", otString);
    TCachedQuery insQry(
            "insert into stat_services( "
            "   point_id, "
            "   scd_out, "
            "   pax_id, "
            "   list_id, "
            "   rfisc, "
            "   service_type, "
            "   rfisc_airline "
            ") values ( "
            "   :point_id, "
            "   :scd_out, "
            "   :pax_id, "
            "   :list_id, "
            "   :rfisc, "
            "   :service_type, "
            "   :rfisc_airline "
            ") ",insQryParams);

    TRptParams rpt_params;
    rpt_params.point_id = point_id;
    TServiceList rows(true);
    rows.fromDB(rpt_params);
    for(const auto &row: rows) {
        insQry.get().SetVariable("pax_id", row.pax_id);
        insQry.get().SetVariable("list_id", row.paid_rfisc_item.list_id);
        insQry.get().SetVariable("rfisc", row.paid_rfisc_item.RFISC);
        insQry.get().SetVariable("service_type", ServiceTypes().encode(row.paid_rfisc_item.service_type));
        insQry.get().SetVariable("rfisc_airline", row.paid_rfisc_item.airline);
        insQry.get().Execute();
    }
}
