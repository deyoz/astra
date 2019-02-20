#include "vo.h"
#include "qrys.h"
#include "report_common.h"
#include "stat/utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace BASIC::date_time;

void RunVOStat(
        const TStatParams &params,
        TVOAbstractStat &VOStat,
        bool full
        )
{
    for(int pass = 0; pass <= 2; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate);
        if (pass!=0)
            QryParams << QParam("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        string SQLText = "select stat_vo.* from ";
        if(pass != 0) {
            SQLText +=
                "   arx_stat_vo stat_vo, "
                "   arx_points points ";
            if(pass == 2)
                SQLText += ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        } else {
            SQLText +=
                "   stat_vo, "
                "   points ";
        }
        SQLText +=
            "where "
            "   stat_vo.point_id = points.point_id and "
            "   points.pr_del >= 0 and ";
        params.AccessClause(SQLText);
        if(params.flt_no != NoExists) {
            SQLText += " points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        if(pass != 0)
            SQLText +=
                " points.part_key = stat_vo.part_key and ";
        if(pass == 1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        SQLText += "   stat_vo.scd_out >= :FirstDate AND stat_vo.scd_out < :LastDate ";
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_part_key = Qry.get().GetFieldIndex("part_key");
            int col_point_id = Qry.get().FieldIndex("point_id");
            int col_voucher = Qry.get().FieldIndex("voucher");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_amount = Qry.get().FieldIndex("amount");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                TVOStatRow row;
                if(col_part_key >= 0)
                    row.part_key = Qry.get().FieldAsDateTime(col_part_key);
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.voucher = Qry.get().FieldAsString(col_voucher);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                row.amount = Qry.get().FieldAsDateTime(col_amount);
                VOStat.add(row);
                if ((not full) and (VOStat.RowCount() > (size_t)MAX_STAT_ROWS()))
                    throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
            }
        }
    }
}

void TVOShortStat::add(const TVOStatRow &row)
{
    TFltInfoCacheItem info = flt_cache.get(row.point_id, row.part_key);
    prn_airline.check(info.airline);
    string voucher = ElemIdToNameLong(etVoucherType, row.voucher);

    int &curr_total = (*this)[info.view_airline][info.view_airp][voucher];
    if(not curr_total) FRowCount++;
    curr_total += row.amount;
    total += row.amount;
}

void TVOFullStat::add(const TVOStatRow &row)
{
    TFltInfoCacheItem info = flt_cache.get(row.point_id, row.part_key);
    prn_airline.check(info.airline);
    string voucher = ElemIdToNameLong(etVoucherType, row.voucher);
    int &curr_total = (*this)[info.view_airline][info.view_airp][info.view_flt_no][row.scd_out][voucher];
    if(not curr_total) FRowCount++;
    curr_total += row.amount;
    total += row.amount;
}

void createXMLVOFullStat(
        const TStatParams &params,
        const TVOFullStat &VOFullStat,
        xmlNodePtr resNode)
{
    if(VOFullStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", VOFullStat.prn_airline.get(), "");

    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����� ३�"));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� �����"));
    SetProp(colNode, "width", 120);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���-��"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortInteger);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(TVOFullStat::const_iterator airline = VOFullStat.begin();
            airline != VOFullStat.end(); airline++) {
        for(TVOFullAirpMap::const_iterator airp = airline->second.begin();
                airp != airline->second.end(); airp++) {
            for(TVOFullFltMap::const_iterator flt = airp->second.begin();
                    flt != airp->second.end(); flt++) {
                for(TVOFullSCDMap::const_iterator scd = flt->second.begin();
                        scd != flt->second.end(); scd++) {
                    for(TVOFullVOMap::const_iterator vo = scd->second.begin();
                            vo != scd->second.end(); vo++) {
                        rowNode = NewTextChild(rowsNode, "row");
                        // ��
                        NewTextChild(rowNode, "col", airline->first);
                        // ��
                        NewTextChild(rowNode, "col", airp->first);
                        // ����� ३�
                        NewTextChild(rowNode, "col", flt->first);
                        // ���
                        NewTextChild(rowNode, "col", DateTimeToStr(scd->first, "dd.mm.yy"));
                        // ��� �����
                        NewTextChild(rowNode, "col", vo->first);
                        // ���-��
                        NewTextChild(rowNode, "col", vo->second);
                    }
                }
            }
        }
    }
    rowNode = NewTextChild(rowsNode, "row");
    NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col", VOFullStat.total);

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("������"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("���஡���"));
}

void createXMLVOShortStat(
        const TStatParams &params,
        const TVOShortStat &VOShortStat,
        xmlNodePtr resNode)
{
    if(VOShortStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", VOShortStat.prn_airline.get(), "");

    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� �����"));
    SetProp(colNode, "width", 120);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���-��"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortInteger);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(TVOShortStat::const_iterator airline = VOShortStat.begin();
            airline != VOShortStat.end(); airline++) {
        for(TVOShortAirpMap::const_iterator airp = airline->second.begin();
                airp != airline->second.end(); airp++) {
            for(TVOShortVOMap::const_iterator vo = airp->second.begin();
                    vo != airp->second.end(); vo++) {
                rowNode = NewTextChild(rowsNode, "row");
                // ��
                NewTextChild(rowNode, "col", airline->first);
                // ��
                NewTextChild(rowNode, "col", airp->first);
                // ��� �����
                NewTextChild(rowNode, "col", vo->first);
                // ���-��
                NewTextChild(rowNode, "col", vo->second);
            }
        }
    }
    rowNode = NewTextChild(rowsNode, "row");
    NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col", VOShortStat.total);

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("������"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("����"));
}

struct TVOFullStatCombo: public TOrderStatItem {
    string airline;
    string airp;
    string flt_no;
    TDateTime scd_out;
    string vo;
    int amount;
    TVOFullStatCombo(
            const string &_airline,
            const string &_airp,
            const string &_flt_no,
            TDateTime _scd_out,
            const string &_vo,
            int _amount):
        airline(_airline),
        airp(_airp),
        flt_no(_flt_no),
        scd_out(_scd_out),
        vo(_vo),
        amount(_amount)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TVOFullStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("��") << delim
        << getLocaleText("��") << delim
        << getLocaleText("����� ३�") << delim
        << getLocaleText("���") << delim
        << getLocaleText("��� �����") << delim
        << getLocaleText("���-��") << endl;
}

void TVOFullStatCombo::add_data(ostringstream &buf) const
{
    buf
        << airline << delim
        << airp << delim
        << flt_no << delim
        << DateTimeToStr(scd_out, "dd.mm.yy") << delim
        << vo << delim
        << amount << endl;
}

void RunVOFullFile(const TStatParams &params, TOrderStatWriter &writer)
{
    TVOFullStat VOFullStat;
    RunVOStat(params, VOFullStat, true);

    for(TVOFullStat::const_iterator airline = VOFullStat.begin();
            airline != VOFullStat.end(); airline++) {
        for(TVOFullAirpMap::const_iterator airp = airline->second.begin();
                airp != airline->second.end(); airp++) {
            for(TVOFullFltMap::const_iterator flt = airp->second.begin();
                    flt != airp->second.end(); flt++) {
                for(TVOFullSCDMap::const_iterator scd = flt->second.begin();
                        scd != flt->second.end(); scd++) {
                    for(TVOFullVOMap::const_iterator vo = scd->second.begin();
                            vo != scd->second.end(); vo++) {
                        writer.insert(TVOFullStatCombo(
                                    airline->first,
                                    airp->first,
                                    flt->first,
                                    scd->first,
                                    vo->first,
                                    vo->second));
                    }
                }
            }
        }
    }
}

struct TVOShortStatCombo: public TOrderStatItem {
    string airline;
    string airp;
    string vo;
    int amount;
    TVOShortStatCombo(
            const string &_airline,
            const string &_airp,
            const string &_vo,
            int _amount):
        airline(_airline),
        airp(_airp),
        vo(_vo),
        amount(_amount)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TVOShortStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("��") << delim
        << getLocaleText("��") << delim
        << getLocaleText("��� �����") << delim
        << getLocaleText("���-��") << endl;
}

void TVOShortStatCombo::add_data(ostringstream &buf) const
{
    buf
        << airline << delim
        << airp << delim
        << vo << delim
        << amount << endl;
}

void RunVOShortFile(const TStatParams &params, TOrderStatWriter &writer)
{
    TVOShortStat VOShortStat;
    RunVOStat(params, VOShortStat, true);

    for(TVOShortStat::const_iterator airline = VOShortStat.begin();
            airline != VOShortStat.end(); airline++) {
        for(TVOShortAirpMap::const_iterator airp = airline->second.begin();
                airp != airline->second.end(); airp++) {
            for(TVOShortVOMap::const_iterator vo = airp->second.begin();
                    vo != airp->second.end(); vo++) {
                writer.insert(TVOShortStatCombo(
                            airline->first,
                            airp->first,
                            vo->first,
                            vo->second));
            }
        }
    }
}

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
