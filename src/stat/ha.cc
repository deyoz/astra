#include "ha.h"
#include "qrys.h"
#include "hotel_acmd.h"
#include "astra_misc.h"
#include "report_common.h"
#include "stat/utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace BASIC::date_time;

void RunHAStat(
        const TStatParams &params,
        THAAbstractStat &HAStat,
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
        string SQLText = "select stat_ha.* from ";
        if(pass != 0) {
            SQLText +=
                "   arx_stat_ha stat_ha, "
                "   arx_points points ";
            if(pass == 2)
                SQLText += ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        } else {
            SQLText +=
                "   stat_ha, "
                "   points ";
        }
        SQLText +=
            "where "
            "   stat_ha.point_id = points.point_id and "
            "   points.pr_del >= 0 and ";
        params.AccessClause(SQLText);
        if(params.flt_no != NoExists) {
            SQLText += " points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        if(pass != 0)
            SQLText +=
                " points.part_key = stat_ha.part_key and ";
        if(pass == 1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        SQLText += "   stat_ha.scd_out >= :FirstDate AND stat_ha.scd_out < :LastDate ";
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_part_key = Qry.get().GetFieldIndex("part_key");
            int col_point_id = Qry.get().FieldIndex("point_id");
            int col_hotel_id = Qry.get().FieldIndex("hotel_id");
            int col_room_type = Qry.get().FieldIndex("room_type");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_adt = Qry.get().FieldIndex("adt");
            int col_chd = Qry.get().FieldIndex("chd");
            int col_inf = Qry.get().FieldIndex("inf");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                THAStatRow row;
                if(col_part_key >= 0)
                    row.part_key = Qry.get().FieldAsDateTime(col_part_key);
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.hotel_id = Qry.get().FieldAsInteger(col_hotel_id);
                if(not Qry.get().FieldIsNULL(col_room_type))
                    row.room_type = Qry.get().FieldAsInteger(col_room_type);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                row.adt = Qry.get().FieldAsInteger(col_adt);
                row.chd = Qry.get().FieldAsInteger(col_chd);
                row.inf = Qry.get().FieldAsInteger(col_inf);
                HAStat.add(row);
                if ((not full) and (HAStat.RowCount() > (size_t)MAX_STAT_ROWS()))
                    throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
            }
        }
    }
}

THAFullCounters &THAFullCounters::operator +=(const THAStatRow &rhs)
{
    adt += rhs.adt;
    chd += rhs.chd;
    inf += rhs.inf;
    switch(rhs.room_type) {
        case 1:
            room_single += rhs.total();
            break;
        case 2:
            room_double += rhs.total();
            break;
    }
    return *this;
}

void THAShortStat::add(const THAStatRow &row)
{
    TFltInfoCacheItem info = flt_cache.get(row.point_id, row.part_key);
    prn_airline.check(info.airline);
    string hotel = ElemIdToNameLong(etHotel, row.hotel_id);
    int &curr_total = (*this)[info.view_airline][info.view_airp][hotel];
    if(not curr_total) FRowCount++;
    curr_total += row.total();
    total += row.total();
}

void THAFullStat::add(const THAStatRow &row)
{
    TFltInfoCacheItem info = flt_cache.get(row.point_id, row.part_key);
    prn_airline.check(info.airline);
    string hotel = ElemIdToNameLong(etHotel, row.hotel_id);
    THAFullCounters &curr_total = (*this)[info.view_airline][info.view_airp][info.view_flt_no][row.scd_out][hotel];
    if(not curr_total.total()) FRowCount++;
    curr_total += row;
    total += row;
}


void createXMLHAFullStat(
        const TStatParams &params,
        const THAFullStat &HAFullStat,
        xmlNodePtr resNode)
{
    if(HAFullStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", HAFullStat.prn_airline.get(), "");

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
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Гостиница"));
    SetProp(colNode, "width", 120);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", "Single");
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortInteger);
    colNode = NewTextChild(headerNode, "col", "Double");
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortInteger);
    colNode = NewTextChild(headerNode, "col", getLocaleText("ВЗ"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortInteger);
    colNode = NewTextChild(headerNode, "col", getLocaleText("РБ"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortInteger);
    colNode = NewTextChild(headerNode, "col", getLocaleText("РМ"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortInteger);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortInteger);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(THAFullStat::const_iterator airline = HAFullStat.begin();
            airline != HAFullStat.end(); airline++) {
        for(THAFullAirpMap::const_iterator airp = airline->second.begin();
                airp != airline->second.end(); airp++) {
            for(THAFullFltMap::const_iterator flt = airp->second.begin();
                    flt != airp->second.end(); flt++) {
                for(THAFullSCDMap::const_iterator scd = flt->second.begin();
                        scd != flt->second.end(); scd++) {
                    for(THAFullHotelMap::const_iterator hotel = scd->second.begin();
                            hotel != scd->second.end(); hotel++) {
                        rowNode = NewTextChild(rowsNode, "row");
                        // АК
                        NewTextChild(rowNode, "col", airline->first);
                        // АП
                        NewTextChild(rowNode, "col", airp->first);
                        // Номер рейса
                        NewTextChild(rowNode, "col", flt->first);
                        // Дата
                        NewTextChild(rowNode, "col", DateTimeToStr(scd->first, "dd.mm.yy"));
                        // Гостиница
                        NewTextChild(rowNode, "col", hotel->first);
                        // Single
                        NewTextChild(rowNode, "col", hotel->second.room_single);
                        // Double
                        NewTextChild(rowNode, "col", hotel->second.room_double);
                        // ВЗ
                        NewTextChild(rowNode, "col", hotel->second.adt);
                        // РБ
                        NewTextChild(rowNode, "col", hotel->second.chd);
                        // РМ
                        NewTextChild(rowNode, "col", hotel->second.inf);
                        // Кол-во
                        NewTextChild(rowNode, "col", hotel->second.total());
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
    NewTextChild(rowNode, "col", HAFullStat.total.room_single);
    NewTextChild(rowNode, "col", HAFullStat.total.room_double);
    NewTextChild(rowNode, "col", HAFullStat.total.adt);
    NewTextChild(rowNode, "col", HAFullStat.total.chd);
    NewTextChild(rowNode, "col", HAFullStat.total.inf);
    NewTextChild(rowNode, "col", HAFullStat.total.total());

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Расселение"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
}

void createXMLHAShortStat(
        const TStatParams &params,
        const THAShortStat &HAShortStat,
        xmlNodePtr resNode)
{
    if(HAShortStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", HAShortStat.prn_airline.get(), "");

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
    colNode = NewTextChild(headerNode, "col", getLocaleText("Гостиница"));
    SetProp(colNode, "width", 120);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortInteger);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(THAShortStat::const_iterator airline = HAShortStat.begin();
            airline != HAShortStat.end(); airline++) {
        for(THAShortAirpMap::const_iterator airp = airline->second.begin();
                airp != airline->second.end(); airp++) {
            for(THAShortHotelMap::const_iterator hotel = airp->second.begin();
                    hotel != airp->second.end(); hotel++) {
                rowNode = NewTextChild(rowsNode, "row");
                // АК
                NewTextChild(rowNode, "col", airline->first);
                // АП
                NewTextChild(rowNode, "col", airp->first);
                // Гостиница
                NewTextChild(rowNode, "col", hotel->first);
                // Кол-во
                NewTextChild(rowNode, "col", hotel->second);
            }
        }
    }
    rowNode = NewTextChild(rowsNode, "row");
    NewTextChild(rowNode, "col", getLocaleText("Итого:"));
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col", HAShortStat.total);

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Расселение"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Общая"));
}

struct THAFullStatCombo: public TOrderStatItem {
    string airline;
    string airp;
    string flt_no;
    TDateTime scd_out;
    string hotel;
    THAFullCounters total;
    THAFullStatCombo(
            const string &_airline,
            const string &_airp,
            const string &_flt_no,
            TDateTime _scd_out,
            const string &_hotel,
            const THAFullCounters &_total):
        airline(_airline),
        airp(_airp),
        flt_no(_flt_no),
        scd_out(_scd_out),
        hotel(_hotel),
        total(_total)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void THAFullStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("АК") << delim
        << getLocaleText("АП") << delim
        << getLocaleText("Номер рейса") << delim
        << getLocaleText("Дата") << delim
        << getLocaleText("Гостиница") << delim
        << "Single" << delim
        << "Double" << delim
        << getLocaleText("ВЗ") << delim
        << getLocaleText("РБ") << delim
        << getLocaleText("РМ") << delim
        << getLocaleText("Кол-во") << endl;
}

void THAFullStatCombo::add_data(ostringstream &buf) const
{
    buf
        << airline << delim
        << airp << delim
        << flt_no << delim
        << DateTimeToStr(scd_out, "dd.mm.yy") << delim
        << hotel << delim
        << total.room_single << delim
        << total.room_double << delim
        << total.adt << delim
        << total.chd << delim
        << total.inf << delim
        << total.total() << endl;
}

void RunHAFullFile(const TStatParams &params, TOrderStatWriter &writer)
{
    THAFullStat HAFullStat;
    RunHAStat(params, HAFullStat, true);

    for(THAFullStat::const_iterator airline = HAFullStat.begin();
            airline != HAFullStat.end(); airline++) {
        for(THAFullAirpMap::const_iterator airp = airline->second.begin();
                airp != airline->second.end(); airp++) {
            for(THAFullFltMap::const_iterator flt = airp->second.begin();
                    flt != airp->second.end(); flt++) {
                for(THAFullSCDMap::const_iterator scd = flt->second.begin();
                        scd != flt->second.end(); scd++) {
                    for(THAFullHotelMap::const_iterator hotel = scd->second.begin();
                            hotel != scd->second.end(); hotel++) {
                        writer.insert(THAFullStatCombo(
                                    airline->first,
                                    airp->first,
                                    flt->first,
                                    scd->first,
                                    hotel->first,
                                    hotel->second));
                    }
                }
            }
        }
    }
}

struct THAShortStatCombo: public TOrderStatItem {
    string airline;
    string airp;
    string hotel;
    int total;
    THAShortStatCombo(
            const string &_airline,
            const string &_airp,
            const string &_hotel,
            int _total):
        airline(_airline),
        airp(_airp),
        hotel(_hotel),
        total(_total)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void THAShortStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("АК") << delim
        << getLocaleText("АП") << delim
        << getLocaleText("Гостиница") << delim
        << getLocaleText("Кол-во") << endl;
}

void THAShortStatCombo::add_data(ostringstream &buf) const
{
    buf
        << airline << delim
        << airp << delim
        << hotel << delim
        << total << endl;
}

void RunHAShortFile(const TStatParams &params, TOrderStatWriter &writer)
{
    THAShortStat HAShortStat;
    RunHAStat(params, HAShortStat, true);

    for(THAShortStat::const_iterator airline = HAShortStat.begin();
            airline != HAShortStat.end(); airline++) {
        for(THAShortAirpMap::const_iterator airp = airline->second.begin();
                airp != airline->second.end(); airp++) {
            for(THAShortHotelMap::const_iterator hotel = airp->second.begin();
                    hotel != airp->second.end(); hotel++) {
                writer.insert(THAShortStatCombo(
                            airline->first,
                            airp->first,
                            hotel->first,
                            hotel->second));
            }
        }
    }
}

void get_stat_ha(int point_id)
{
    TCachedQuery delQry("delete from stat_ha where point_id = :point_id", QParams() << QParam("point_id", otInteger, point_id));
    delQry.get().Execute();

    TPaxList pax_list;
    pax_list.fromDB(point_id);
    THotelAcmdPax hotel_acmd_pax;
    hotel_acmd_pax.fromDB(point_id);

    typedef map<int, THAStatRow> TRoomStat;
    typedef map<int, TRoomStat> THotelStat;
    THotelStat stat;

    for(map<int, TPaxListItem>::iterator pax = pax_list.items.begin();
            pax != pax_list.items.end(); pax++) {
        map<int, THotelAcmdPaxItem>::iterator acmd_pax = hotel_acmd_pax.items.find(pax->second.pax_id);
        if(acmd_pax != hotel_acmd_pax.items.end()) {
            THAStatRow &stat_row = stat[acmd_pax->second.hotel_id][acmd_pax->second.room_type];
            switch(pax->second.pers_type) {
                case adult:
                    stat_row.adt++;
                    break;
                case child:
                    stat_row.chd++;
                    break;
                case baby:
                    stat_row.inf++;
                    break;
                default:
                    break;
            }
        }
    }

    TTripInfo trip;
    trip.getByPointId(point_id);

    QParams insQryParams;
    insQryParams
        << QParam("point_id", otInteger, point_id)
        << QParam("hotel_id", otInteger)
        << QParam("room_type", otInteger)
        << QParam("scd_out", otDate, trip.scd_out)
        << QParam("adt", otInteger)
        << QParam("chd", otInteger)
        << QParam("inf", otInteger);

    TCachedQuery insQry(
            "insert into stat_ha ( "
            "   point_id, "
            "   hotel_id, "
            "   room_type, "
            "   scd_out, "
            "   adt, "
            "   chd, "
            "   inf "
            ") values ( "
            "   :point_id, "
            "   :hotel_id, "
            "   :room_type, "
            "   :scd_out, "
            "   :adt, "
            "   :chd, "
            "   :inf "
            ") ", insQryParams);
    for(THotelStat::iterator hotel = stat.begin();
            hotel != stat.end(); hotel++) {
        for(TRoomStat::iterator room = hotel->second.begin();
                room != hotel->second.end(); room++) {
            insQry.get().SetVariable("hotel_id", hotel->first);
            if(room->first == NoExists)
                insQry.get().SetVariable("room_type", FNull);
            else
                insQry.get().SetVariable("room_type", room->first);
            insQry.get().SetVariable("adt", room->second.adt);
            insQry.get().SetVariable("chd", room->second.chd);
            insQry.get().SetVariable("inf", room->second.inf);
            insQry.get().Execute();
        }
    }
}
