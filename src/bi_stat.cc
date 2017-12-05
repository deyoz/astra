#include "bi_stat.h"
#include "qrys.h"
#include "report_common.h"
#include "stat_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace AstraLocale;
using namespace EXCEPTIONS;
using namespace std;
using namespace BASIC::date_time;

const TFltInfoCacheItem &TFltInfoCache::get(int point_id)
{
    TFltInfoCache::iterator i = this->find(point_id);
    if(i == this->end()) {
        TTripInfo info;
        info.getByPointId(point_id);
        TFltInfoCacheItem item;
        item.airp = info.airp;
        item.airline = info.airline;
        item.view_airp = ElemIdToCodeNative(etAirp, info.airp);
        item.view_airline = ElemIdToCodeNative(etAirline, info.airline);
        ostringstream flt_no_str;
        flt_no_str << setw(3) << setfill('0') << info.flt_no << ElemIdToCodeNative(etSuffix, info.suffix);
        item.view_flt_no = flt_no_str.str();
        pair<TFltInfoCache::iterator, bool> ret;
        ret = this->insert(make_pair(point_id, item));
        i = ret.first;
    }
    return i->second;
}

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

void TBIFullStat::add(const TBIStatRow &row)
{
    TFltInfoCacheItem info = flt_cache.get(row.point_id);
    prn_airline.check(info.airline);
    string terminal = ElemIdToNameLong(etAirpTerminal, row.terminal);
    string bi_hall = ElemIdToNameLong(etBIHall, row.hall);

    TBIStatCounters &counters = (*this)[info.view_airline][info.view_airp][info.view_flt_no][row.scd_out][terminal][bi_hall];
    if(counters.empty()) FRowCount++;
    counters.add(row.print_type);
    totals.add(row.print_type);
}

void RunBIStat(
        const TStatParams &params,
        TBIAbstractStat &BIStat,
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
        string SQLText = "select bi_stat.* from ";
        if(pass != 0) {
            SQLText +=
                "   arx_bi_stat bi_stat, "
                "   arx_points points ";
            if(pass == 2)
                SQLText += ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        } else {
            SQLText +=
                "   bi_stat, "
                "   points ";
        }
        SQLText +=
            "where "
            "   bi_stat.point_id = points.point_id and "
            "   points.pr_del >= 0 and ";
        params.AccessClause(SQLText);
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
        if(pass != 0)
            SQLText +=
                " points.part_key = bi_stat.part_key and ";
        if(pass == 1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        SQLText += "   bi_stat.scd_out >= :FirstDate AND bi_stat.scd_out < :LastDate ";
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_point_id = Qry.get().FieldIndex("point_id");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_pax_id = Qry.get().FieldIndex("pax_id");
            int col_print_type = Qry.get().FieldIndex("print_type");
            int col_terminal = Qry.get().FieldIndex("terminal");
            int col_hall = Qry.get().FieldIndex("hall");
            int col_op_type = Qry.get().FieldIndex("op_type");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                TBIStatRow row;
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                row.pax_id = Qry.get().FieldAsInteger(col_pax_id);
                row.print_type = BIPrintRules::TPrintTypes().decode(Qry.get().FieldAsString(col_print_type));
                row.terminal = Qry.get().FieldAsInteger(col_terminal);
                row.hall = Qry.get().FieldAsInteger(col_hall);
                row.op_type = ASTRA::TDevOperTypes().decode(Qry.get().FieldAsString(col_op_type));
                BIStat.add(row);

                if ((not full) and (BIStat.RowCount() > (size_t)MAX_STAT_ROWS()))
                    throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
            }
        }
    }
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
    colNode = NewTextChild(headerNode, "col", getLocaleText("АК"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АП"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Номер рейса"));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Терминал"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Бизнес зал"));
    SetProp(colNode, "width", 120);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Всего"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("1 пас"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Плюс 1"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Группа"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(TBIFullStat::const_iterator airline = BIFullStat.begin();
            airline != BIFullStat.end(); airline++) {
        for(TBIAirpMap::const_iterator airp = airline->second.begin();
                airp != airline->second.end(); airp++) {
            for(TBIFltMap::const_iterator flt = airp->second.begin();
                    flt != airp->second.end(); flt++) {
                for(TBIScdOutMap::const_iterator scd = flt->second.begin();
                        scd != flt->second.end(); scd++) {
                    for(TBITerminalMap::const_iterator terminal = scd->second.begin();
                            terminal != scd->second.end(); terminal++) {
                        for(TBIHallMap::const_iterator hall = terminal->second.begin();
                                hall != terminal->second.end(); hall++) {
                            rowNode = NewTextChild(rowsNode, "row");
                            // АК
                            NewTextChild(rowNode, "col", airline->first);
                            // АП
                            NewTextChild(rowNode, "col", airp->first);
                            // Номер рейса
                            NewTextChild(rowNode, "col", flt->first);
                            // Дата
                            NewTextChild(rowNode, "col", DateTimeToStr(scd->first, "dd.mm.yy"));
                            // Терминал
                            NewTextChild(rowNode, "col", terminal->first);
                            // Зал
                            NewTextChild(rowNode, "col", hall->first);
                            // Всего
                            NewTextChild(rowNode, "col", hall->second.total());
                            // 1 пас
                            NewTextChild(rowNode, "col", hall->second.one);
                            // Плюс 1
                            NewTextChild(rowNode, "col", hall->second.two);
                            // Группа
                            NewTextChild(rowNode, "col", hall->second.all);
                        }
                    }
                }
            }
        }
    }
    rowNode = NewTextChild(rowsNode, "row");
    NewTextChild(rowNode, "col", getLocaleText("Итого:"));
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
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Бизнес приглашения"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));

    LogTrace(TRACE5) << GetXMLDocText(resNode->doc);
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
        << getLocaleText("АК") << delim
        << getLocaleText("АП") << delim
        << getLocaleText("Номер рейса") << delim
        << getLocaleText("Дата") << delim
        << getLocaleText("Терминал") << delim
        << getLocaleText("Бизнес зал") << delim
        << getLocaleText("Всего") << delim
        << getLocaleText("1 пас") << delim
        << getLocaleText("Плюс 1") << delim
        << getLocaleText("Группа") << endl;
}

void RunBIFullFile(const TStatParams &params, TOrderStatWriter &writer)
{
    TBIFullStat BIFullStat;
    RunBIStat(params, BIFullStat, true);
    for(TBIFullStat::const_iterator airline = BIFullStat.begin();
            airline != BIFullStat.end(); airline++) {
        for(TBIAirpMap::const_iterator airp = airline->second.begin();
                airp != airline->second.end(); airp++) {
            for(TBIFltMap::const_iterator flt = airp->second.begin();
                    flt != airp->second.end(); flt++) {
                for(TBIScdOutMap::const_iterator scd = flt->second.begin();
                        scd != flt->second.end(); scd++) {
                    for(TBITerminalMap::const_iterator terminal = scd->second.begin();
                            terminal != scd->second.end(); terminal++) {
                        for(TBIHallMap::const_iterator hall = terminal->second.begin();
                                hall != terminal->second.end(); hall++) {
                            writer.insert(TBIFullStatCombo(
                                        airline->first,
                                        airp->first,
                                        flt->first,
                                        scd->first,
                                        terminal->first,
                                        hall->first,
                                        hall->second.total(),
                                        hall->second.one,
                                        hall->second.two,
                                        hall->second.all));
                        }
                    }
                }
            }
        }
    }
}
