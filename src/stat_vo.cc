#include "stat_vo.h"
#include "qrys.h"

void get_stat_vo(int point_id)
{
    TCachedQuery delQry("delete from stat_vo where point_id = :point_id", QParams() << QParam("point_id", otInteger, point_id));
    delQry.get().Execute();

    TCachedQuery Qry(
            "select "
            "   scd_out, "
            "   voucher, "
            "   sum(amount) amount "
            "from "
            "(select "
            "   points.scd_out, "
            "   cp.voucher,  "
            "   count(*) amount, "
            "   1 "
            "from "
            "   points, "
            "   pax_grp, "
            "   pax, "
            "   confirm_print cp "
            "where "
            "   points.point_id = :point_id and "
            "   pax_grp.point_dep = points.point_id and "
            "   pax_grp.grp_id = pax.grp_id and "
            "   pax.pax_id = cp.pax_id and "
            "   cp.voucher is not null and "
            "   cp.pr_print <> 0 "
            "group by "
            "   points.scd_out, "
            "   cp.voucher "
            "union "
            "select "
            "    scd_out, "
            "    voucher, "
            "    count(*) amount, "
            "    2 "
            "from confirm_print_vo_unreg where "
            "    point_id = :point_id and "
            "    pr_print <> 0 "
            "group by "
            "    scd_out, "
            "    voucher) "
            "group by "
            "    scd_out, "
            "    voucher ",
        QParams() << QParam("point_id", otInteger, point_id));
    QParams insQryParams;
    insQryParams
        << QParam("point_id", otInteger, point_id)
        << QParam("voucher", otString)
        << QParam("scd_out", otDate)
        << QParam("amount", otInteger);
    TCachedQuery insQry(
            "insert into stat_vo ( "
            "   point_id, "
            "   voucher, "
            "   scd_out, "
            "   amount "
            ") values ( "
            "   :point_id, "
            "   :voucher, "
            "   :scd_out, "
            "   :amount "
            ") ", insQryParams);
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next()) {
        insQry.get().SetVariable("voucher", Qry.get().FieldAsString("voucher"));
        insQry.get().SetVariable("scd_out", Qry.get().FieldAsDateTime("scd_out"));
        insQry.get().SetVariable("amount", Qry.get().FieldAsInteger("amount"));
        insQry.get().Execute();
    }
}
