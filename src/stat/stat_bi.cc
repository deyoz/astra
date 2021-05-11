#include "stat_bi.h"
#include "qrys.h"
#include "report_common.h"
#include "stat/stat_utils.h"
#include "PgOraConfig.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace AstraLocale;
using namespace EXCEPTIONS;
using namespace std;
using namespace BASIC::date_time;

void TBIStatCounters::add(BIPrintRules::TPrintType::Enum print_type)
{
    switch(print_type) {
        case BIPrintRules::TPrintType::One:
            one++;
            break;
        case BIPrintRules::TPrintType::OnePlusOne:
            two++;
            break;
        case BIPrintRules::TPrintType::All:
            all++;
            break;
        default:
            throw Exception("TBIStatCounters::add: wront print_type");
    }
}

void TBIDetailStat::add(const TBIStatRow &row)
{
    TFltInfoCacheItem info = flt_cache.get(row.point_id, row.part_key);
    prn_airline.check(info.airline);
    string terminal = ElemIdToNameLong(etAirpTerminal, row.terminal);
    string bi_hall = ElemIdToNameLong(etBIHall, row.hall);

    int &curr_total = (*this)[info.view_airline][info.view_airp][terminal][bi_hall];
    if(not curr_total) FRowCount++;
    ++curr_total;
    ++total;
}

void TBIShortStat::add(const TBIStatRow &row)
{
    TFltInfoCacheItem info = flt_cache.get(row.point_id, row.part_key);
    prn_airline.check(info.airline);
    string terminal = ElemIdToNameLong(etAirpTerminal, row.terminal);
    string bi_hall = ElemIdToNameLong(etBIHall, row.hall);

    int &curr_total = (*this)[info.view_airline][info.view_airp];
    if(not curr_total) FRowCount++;
    ++curr_total;
    ++total;
}

void TBIFullStat::add(const TBIStatRow &row)
{
    TFltInfoCacheItem info = flt_cache.get(row.point_id, row.part_key);
    prn_airline.check(info.airline);
    string terminal = ElemIdToNameLong(etAirpTerminal, row.terminal);
    string bi_hall = ElemIdToNameLong(etBIHall, row.hall);

    TBIStatCounters &counters = (*this)[info.view_airline][info.view_airp][info.view_flt_no][row.scd_out][terminal][bi_hall];
    if(counters.empty()) FRowCount++;
    counters.add(row.print_type);
    totals.add(row.print_type);
}


void ArxRunBIStat(
        const TStatParams &params,
        TBIAbstractStat &BIStat
        )
{
    LogTrace5 << __func__;
    for(int pass = 1; pass <= 2; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate);
        QryParams << QParam("arx_trip_date_range", otDate, params.LastDate+ARX_TRIP_DATE_RANGE());
        string SQLText = "select arx_bi_stat.* from ";
        SQLText +=
                "   arx_bi_stat , "
                "   arx_points  ";
        if(pass == 2) {
            SQLText += getMoveArxQuery();
        }

        SQLText +=
            "where "
            "   arx_bi_stat.point_id = arx_points.point_id and "
            "   arx_bi_stat.pr_print <> 0 and "
            "   arx_points.pr_del >= 0 and ";
        params.AccessClause(SQLText, "arx_points");
        if(params.flt_no != NoExists) {
            SQLText += " points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        if(params.airp_terminal != NoExists) {
            SQLText += " terminal = :terminal and ";
            QryParams << QParam("terminal", otInteger, params.airp_terminal);
        }
        if(params.bi_hall != NoExists) {
            SQLText += " hall = :hall and ";
            QryParams << QParam("hall", otInteger, params.bi_hall);
        }
        SQLText += " arx_points.part_key = arx_bi_stat.part_key and ";
        if(pass == 1)
            SQLText += " arx_points.part_key >= :FirstDate AND arx_points.part_key < :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " arx_points.part_key=arx_ext.part_key AND arx_points.move_id=arx_ext.move_id AND \n";
        SQLText += "   arx_bi_stat.scd_out >= :FirstDate AND arx_bi_stat.scd_out < :LastDate ";
        DB::TCachedQuery Qry(PgOra::getROSession("ARX_BI_STAT"), SQLText, QryParams, STDLOG);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_part_key = Qry.get().GetFieldIndex("part_key");
            int col_point_id = Qry.get().FieldIndex("point_id");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_pax_id = Qry.get().FieldIndex("pax_id");
            int col_print_type = Qry.get().FieldIndex("print_type");
            int col_terminal = Qry.get().FieldIndex("terminal");
            int col_hall = Qry.get().FieldIndex("hall");
            int col_op_type = Qry.get().FieldIndex("op_type");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                TBIStatRow row;
                if(col_part_key >= 0)
                    row.part_key = Qry.get().FieldAsDateTime(col_part_key);
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                row.pax_id = Qry.get().FieldAsInteger(col_pax_id);
                row.print_type = BIPrintRules::TPrintTypes().decode(Qry.get().FieldAsString(col_print_type));
                row.terminal = Qry.get().FieldAsInteger(col_terminal);
                row.hall = Qry.get().FieldAsInteger(col_hall);
                row.op_type = ASTRA::TDevOperTypes().decode(Qry.get().FieldAsString(col_op_type));
                BIStat.add(row);
                params.overflow.check(BIStat.RowCount());
            }
        }
    }
}

void RunBIStat(
        const TStatParams &params,
        TBIAbstractStat &BIStat
        )
{
    QParams QryParams;
    QryParams
        << QParam("FirstDate", otDate, params.FirstDate)
        << QParam("LastDate", otDate, params.LastDate);

    std::string SQLText =
        "SELECT * FROM "
        "   bi_stat "
        "WHERE "
        "   pr_print <> 0 and ";
    if(params.airp_terminal != NoExists) {
        SQLText += " terminal = :terminal and ";
        QryParams << QParam("terminal", otInteger, params.airp_terminal);
    }
    if(params.bi_hall != NoExists) {
        SQLText += " hall = :hall and ";
        QryParams << QParam("hall", otInteger, params.bi_hall);
    }

    SQLText += "   scd_out >= :FirstDate AND scd_out < :LastDate ";
    DB::TCachedQuery Qry(PgOra::getROSession("BI_STAT"), SQLText, QryParams, STDLOG);
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        int col_part_key = Qry.get().GetFieldIndex("part_key");
        int col_point_id = Qry.get().FieldIndex("point_id");
        int col_scd_out = Qry.get().FieldIndex("scd_out");
        int col_pax_id = Qry.get().FieldIndex("pax_id");
        int col_print_type = Qry.get().FieldIndex("print_type");
        int col_terminal = Qry.get().FieldIndex("terminal");
        int col_hall = Qry.get().FieldIndex("hall");
        int col_op_type = Qry.get().FieldIndex("op_type");
        for(; not Qry.get().Eof; Qry.get().Next()) {
            const PointId_t point_id(Qry.get().FieldAsInteger(col_point_id));
            TAdvTripInfo fltInfo;
            if (!fltInfo.getByPointId(point_id.get())) {
                continue;
            }
            if(params.flt_no != ASTRA::NoExists && params.flt_no != fltInfo.flt_no) {
                continue;
            }
            if (!params.airps.elems().empty()) {
                if (params.airps.elems_permit()) {
                    if (params.airps.elems().find(fltInfo.airp) == params.airps.elems().end()) {
                        continue;
                    }
                } else {
                    if (params.airps.elems().find(fltInfo.airp) != params.airps.elems().end()) {
                        continue;
                    }
                }
            }
            if (!params.airlines.elems().empty()) {
                if (params.airlines.elems_permit()) {
                   if (params.airlines.elems().find(fltInfo.airline) == params.airlines.elems().end()) {
                       continue;
                   }
                } else {
                   if (params.airlines.elems().find(fltInfo.airline) != params.airlines.elems().end()) {
                       continue;
                   }
                }
            };

            TBIStatRow row;
            if(col_part_key >= 0)
                row.part_key = Qry.get().FieldAsDateTime(col_part_key);
            row.point_id = point_id.get();
            row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
            row.pax_id = Qry.get().FieldAsInteger(col_pax_id);
            row.print_type = BIPrintRules::TPrintTypes().decode(Qry.get().FieldAsString(col_print_type));
            row.terminal = Qry.get().FieldAsInteger(col_terminal);
            row.hall = Qry.get().FieldAsInteger(col_hall);
            row.op_type = ASTRA::TDevOperTypes().decode(Qry.get().FieldAsString(col_op_type));
            BIStat.add(row);
            params.overflow.check(BIStat.RowCount());
        }
    }

    ArxRunBIStat(params, BIStat);
}

void createXMLBIFullStat(
        const TStatParams &params,
        const TBIFullStat &BIFullStat,
        xmlNodePtr resNode)
{
    if(BIFullStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", BIFullStat.prn_airline.get(), "");

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
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��ନ���"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("������ ���"));
    SetProp(colNode, "width", 120);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�ᥣ�"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("1 ���"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���� 1"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��㯯�"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;

    for(const auto &airline: BIFullStat) {
        for(const auto &airp: airline.second) {
            for(const auto &flt: airp.second) {
                for(const auto &scd: flt.second) {
                    for(const auto &terminal: scd.second) {
                        for(const auto &hall: terminal.second) {
                            rowNode = NewTextChild(rowsNode, "row");
                            // ��
                            NewTextChild(rowNode, "col", airline.first);
                            // ��
                            NewTextChild(rowNode, "col", airp.first);
                            // ����� ३�
                            NewTextChild(rowNode, "col", flt.first);
                            // ���
                            NewTextChild(rowNode, "col", DateTimeToStr(scd.first, "dd.mm.yy"));
                            // ��ନ���
                            NewTextChild(rowNode, "col", terminal.first);
                            // ���
                            NewTextChild(rowNode, "col", hall.first);
                            // �ᥣ�
                            NewTextChild(rowNode, "col", hall.second.total());
                            // 1 ���
                            NewTextChild(rowNode, "col", hall.second.one);
                            // ���� 1
                            NewTextChild(rowNode, "col", hall.second.two);
                            // ��㯯�
                            NewTextChild(rowNode, "col", hall.second.all);
                        }
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
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col", BIFullStat.totals.total());
    NewTextChild(rowNode, "col", BIFullStat.totals.one);
    NewTextChild(rowNode, "col", BIFullStat.totals.two);
    NewTextChild(rowNode, "col", BIFullStat.totals.all);

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("������ �ਣ��襭��"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("���஡���"));
}

struct TBIFullStatCombo : public TOrderStatItem
{
    string airline;
    string airp;
    string flt_no;
    TDateTime scd;
    string terminal;
    string hall;
    int total;
    int all;
    int two;
    int one;

    TBIFullStatCombo(
            const string &_airline,
            const string &_airp,
            const string &_flt_no,
            TDateTime _scd,
            const string &_terminal,
            const string &_hall,
            int _total,
            int _all,
            int _two,
            int _one):
        airline(_airline),
        airp(_airp),
        flt_no(_flt_no),
        scd(_scd),
        terminal(_terminal),
        hall(_hall),
        total(_total),
        all(_all),
        two(_two),
        one(_one)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TBIFullStatCombo::add_data(ostringstream &buf) const
{
    buf
        << airline << delim
        << airp << delim
        << flt_no << delim
        << DateTimeToStr(scd, "dd.mm.yy") << delim
        << terminal << delim
        << hall << delim
        << total << delim
        << all << delim
        << two << delim
        << one << endl;
}

void TBIFullStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("��") << delim
        << getLocaleText("��") << delim
        << getLocaleText("����� ३�") << delim
        << getLocaleText("���") << delim
        << getLocaleText("��ନ���") << delim
        << getLocaleText("������ ���") << delim
        << getLocaleText("�ᥣ�") << delim
        << getLocaleText("1 ���") << delim
        << getLocaleText("���� 1") << delim
        << getLocaleText("��㯯�") << endl;
}

void RunBIFullFile(const TStatParams &params, TOrderStatWriter &writer)
{
    TBIFullStat BIFullStat;
    RunBIStat(params, BIFullStat);
    for(const auto &airline: BIFullStat) {
        for(const auto &airp: airline.second) {
            for(const auto &flt: airp.second) {
                for(const auto &scd: flt.second) {
                    for(const auto &terminal: scd.second) {
                        for(const auto &hall: terminal.second) {
                            writer.insert(TBIFullStatCombo(
                                        airline.first,
                                        airp.first,
                                        flt.first,
                                        scd.first,
                                        terminal.first,
                                        hall.first,
                                        hall.second.total(),
                                        hall.second.one,
                                        hall.second.two,
                                        hall.second.all));
                        }
                    }
                }
            }
        }
    }
}

void createXMLBIShortStat(
        const TStatParams &params,
        const TBIShortStat &BIShortStat,
        xmlNodePtr resNode)
{
    if(BIShortStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", BIShortStat.prn_airline.get(), "");

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
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�ᥣ�"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(const auto &airline: BIShortStat) {
        for(const auto &airp: airline.second) {
            rowNode = NewTextChild(rowsNode, "row");
            // ��
            NewTextChild(rowNode, "col", airline.first);
            // ��
            NewTextChild(rowNode, "col", airp.first);
            // �ᥣ�
            NewTextChild(rowNode, "col", airp.second);
        }
    }
    rowNode = NewTextChild(rowsNode, "row");
    NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col", BIShortStat.total);

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("������ �ਣ��襭��"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("����"));
}

void createXMLBIDetailStat(
        const TStatParams &params,
        const TBIDetailStat &BIDetailStat,
        xmlNodePtr resNode)
{
    if(BIDetailStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", BIDetailStat.prn_airline.get(), "");

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
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��ନ���"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("������ ���"));
    SetProp(colNode, "width", 120);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�ᥣ�"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(const auto &airline: BIDetailStat) {
        for(const auto &airp: airline.second) {
            for(const auto &terminal: airp.second) {
                for(const auto &hall: terminal.second) {
                    rowNode = NewTextChild(rowsNode, "row");
                    // ��
                    NewTextChild(rowNode, "col", airline.first);
                    // ��
                    NewTextChild(rowNode, "col", airp.first);
                    // ��ନ���
                    NewTextChild(rowNode, "col", terminal.first);
                    // ���
                    NewTextChild(rowNode, "col", hall.first);
                    // �ᥣ�
                    NewTextChild(rowNode, "col", hall.second);
                }
            }
        }
    }
    rowNode = NewTextChild(rowsNode, "row");
    NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col", BIDetailStat.total);

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("������ �ਣ��襭��"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("��⠫���஢�����"));
}

struct TBIShortStatCombo : public TOrderStatItem
{
    string airline;
    string airp;
    int total;
    TBIShortStatCombo(
            const string &_airline,
            const string &_airp,
            int _total
            ):
        airline(_airline),
        airp(_airp),
        total(_total)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

struct TBIDetailStatCombo : public TOrderStatItem
{
    string airline;
    string airp;
    string terminal;
    string hall;
    int total;
    TBIDetailStatCombo(
            const string &_airline,
            const string &_airp,
            const string &_terminal,
            const string &_hall,
            int _total
            ):
        airline(_airline),
        airp(_airp),
        terminal(_terminal),
        hall(_hall),
        total(_total)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TBIShortStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("��") << delim
        << getLocaleText("��") << delim
        << getLocaleText("�ᥣ�") << endl;
}

void TBIShortStatCombo::add_data(ostringstream &buf) const
{
    buf
        << airline << delim
        << airp << delim
        << total << endl;
}

void TBIDetailStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("��") << delim
        << getLocaleText("��") << delim
        << getLocaleText("��ନ���") << delim
        << getLocaleText("������ ���") << delim
        << getLocaleText("�ᥣ�") << endl;
}

void TBIDetailStatCombo::add_data(ostringstream &buf) const
{
    buf
        << airline << delim
        << airp << delim
        << terminal << delim
        << hall << delim
        << total << endl;
}

void RunBIShortFile(const TStatParams &params, TOrderStatWriter &writer)
{
    TBIShortStat BIShortStat;
    RunBIStat(params, BIShortStat);

    for(const auto &airline: BIShortStat) {
        for(const auto &airp: airline.second) {
            writer.insert(TBIShortStatCombo(
                        airline.first,
                        airp.first,
                        airp.second));
        }
    }
}

void RunBIDetailFile(const TStatParams &params, TOrderStatWriter &writer)
{
    TBIDetailStat BIDetailStat;
    RunBIStat(params, BIDetailStat);

    for(const auto &airline: BIDetailStat) {
        for(const auto &airp: airline.second) {
            for(const auto &terminal: airp.second) {
                for(const auto &hall: terminal.second) {
                    writer.insert(TBIDetailStatCombo(
                                airline.first,
                                airp.first,
                                terminal.first,
                                hall.first,
                                hall.second));
                }
            }
        }
    }
}

