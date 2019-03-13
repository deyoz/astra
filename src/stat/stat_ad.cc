#include "stat_ad.h"
#include "qrys.h"
#include "pax_events.h"
#include "passenger.h"
#include "report_common.h"
#include "stat/stat_utils.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace BASIC::date_time;

string TADFullStat::baggage() const
{
    ostringstream result;
    if(bag_amount) {
        result << bag_amount << "/" << bag_weight;
    }
    return result.str();
}

void TADFullStat::add(const TADStatRow &row)
{
    TFltInfoCacheItem info = flt_cache.get(row.point_id, row.part_key);
    prn_airline.check(info.airline);

    CheckIn::TSimplePaxItem pax;
    pax.getByPaxId(row.pax_id, row.part_key);
    string name = transliter(pax.surname + " " + pax.name,1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);

    TADFullData &data = (*this)[info.view_airline][info.view_airp][info.view_flt_no][row.scd_out][name][pax.id];
    if(data.pers_type == NoPerson) FRowCount++;
    data.pnr = row.pnr;
    data.pers_type = pax.pers_type;
    data.cls = row.cls;
    data.client_type = row.client_type;

    if(row.bag_amount != NoExists) {
        ostringstream buf;
        buf << row.bag_amount << "/" << row.bag_weight;
        data.baggage = buf.str();
        bag_amount += row.bag_amount;
        bag_weight += row.bag_weight;
    }

    data.seat_no =
        (TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU ?
         row.seat_no_lat : row.seat_no);


    data.gate = (row.station.empty() ? row.desk : row.station);
}

void RunADStat(
        const TStatParams &params,
        TADAbstractStat &ADStat,
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
        string SQLText = "select stat_ad.* from ";
        if(pass != 0) {
            SQLText +=
                "   arx_stat_ad stat_ad, "
                "   arx_points points ";
            if(pass == 2)
                SQLText += ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        } else {
            SQLText +=
                "   stat_ad, "
                "   points ";
        }
        SQLText +=
            "where "
            "   stat_ad.point_id = points.point_id and "
            "   points.pr_del >= 0 and ";
        params.AccessClause(SQLText);
        if(params.flt_no != NoExists) {
            SQLText += " points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        if(pass != 0)
            SQLText +=
                " points.part_key = stat_ad.part_key and ";
        if(pass == 1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        SQLText += "   stat_ad.scd_out >= :FirstDate AND stat_ad.scd_out < :LastDate ";
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_part_key = Qry.get().GetFieldIndex("part_key");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_point_id = Qry.get().FieldIndex("point_id");
            int col_pax_id = Qry.get().FieldIndex("pax_id");
            int col_pnr = Qry.get().FieldIndex("pnr");
            int col_class = Qry.get().FieldIndex("class");
            int col_client_type = Qry.get().FieldIndex("client_type");
            int col_bag_amount = Qry.get().FieldIndex("bag_amount");
            int col_bag_weight = Qry.get().FieldIndex("bag_weight");
            int col_seat_no = Qry.get().FieldIndex("seat_no");
            int col_seat_no_lat = Qry.get().FieldIndex("seat_no_lat");
            int col_desk = Qry.get().FieldIndex("desk");
            int col_station = Qry.get().FieldIndex("station");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                TADStatRow row;
                if(col_part_key >= 0)
                    row.part_key = Qry.get().FieldAsDateTime(col_part_key);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.pax_id = Qry.get().FieldAsInteger(col_pax_id);
                row.pnr = Qry.get().FieldAsString(col_pnr);
                row.cls = Qry.get().FieldAsString(col_class);
                row.client_type = DecodeClientType(Qry.get().FieldAsString(col_client_type));
                if(not Qry.get().FieldIsNULL(col_bag_amount))
                    row.bag_amount = Qry.get().FieldAsInteger(col_bag_amount);
                if(not Qry.get().FieldIsNULL(col_bag_weight))
                    row.bag_weight = Qry.get().FieldAsInteger(col_bag_weight);
                row.seat_no = Qry.get().FieldAsString(col_seat_no);
                row.seat_no_lat = Qry.get().FieldAsString(col_seat_no_lat);
                row.desk = Qry.get().FieldAsString(col_desk);
                row.station = Qry.get().FieldAsString(col_station);
                ADStat.add(row);
                if ((not full) and (ADStat.RowCount() > (size_t)MAX_STAT_ROWS()))
                    throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
            }
        }
    }

}

void createXMLADFullStat(
        const TStatParams &params,
        const TADFullStat &ADFullStat,
        xmlNodePtr resNode)
{
    if(ADFullStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", ADFullStat.prn_airline.get(), "");

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
    colNode = NewTextChild(headerNode, "col", "PNR");
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Ф.И.О."));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Тип"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Класс"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Тип рег."));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Багаж"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Вых. на посадку"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("№ м"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(TADFullStat::const_iterator airline = ADFullStat.begin();
            airline != ADFullStat.end(); airline++) {
        for(TADFullAirpMap::const_iterator airp = airline->second.begin();
                airp != airline->second.end(); airp++) {
            for(TADFullFltMap::const_iterator flt = airp->second.begin();
                    flt != airp->second.end(); flt++) {
                for(TADFullSCDMap::const_iterator scd = flt->second.begin();
                        scd != flt->second.end(); scd++) {
                    for(TADFullNameMap::const_iterator name = scd->second.begin();
                            name != scd->second.end(); name++) {
                        for(TADFullPaxIdMap::const_iterator pax_id = name->second.begin();
                                pax_id != name->second.end(); pax_id++) {
                            rowNode = NewTextChild(rowsNode, "row");
                            // АК
                            NewTextChild(rowNode, "col", airline->first);
                            // АП
                            NewTextChild(rowNode, "col", airp->first);
                            // Номер рейса
                            NewTextChild(rowNode, "col", flt->first);
                            // Дата
                            NewTextChild(rowNode, "col", DateTimeToStr(scd->first, "dd.mm.yy"));
                            // PNR
                            NewTextChild(rowNode, "col", pax_id->second.pnr);
                            // ФИО
                            NewTextChild(rowNode, "col", name->first);
                            // Тип пакса
                            NewTextChild(rowNode, "col", ElemIdToCodeNative(etPersType,EncodePerson(pax_id->second.pers_type)));
                            // Класс
                            NewTextChild(rowNode, "col", ElemIdToCodeNative(etClass, pax_id->second.cls));
                            // Тип рег.
                            NewTextChild(rowNode, "col", EncodeClientType(pax_id->second.client_type));
                            // Багаж
                            NewTextChild(rowNode, "col", pax_id->second.baggage);
                            // Вых. на посадку
                            NewTextChild(rowNode, "col", pax_id->second.gate);
                            // № места
                            NewTextChild(rowNode, "col", pax_id->second.seat_no);
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
    NewTextChild(rowNode, "col", ADFullStat.RowCount());
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col", ADFullStat.baggage());
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Факт. вылет"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));

}

struct TADFullStatCombo: public TOrderStatItem {
    string airline;
    string airp;
    string flt_no;
    TDateTime scd_out;
    string pnr;
    string name;
    ASTRA::TPerson pers_type;
    string cls;
    ASTRA::TClientType client_type;
    string baggage;
    string gate;
    string seat_no;
    TADFullStatCombo(
            const string &_airline,
            const string &_airp,
            const string &_flt_no,
            TDateTime _scd_out,
            const string &_pnr,
            const string &_name,
            TPerson _pers_type,
            const string &_cls,
            TClientType _client_type,
            const string &_baggage,
            const string &_gate,
            const string &_seat_no):
        airline(_airline),
        airp(_airp),
        flt_no(_flt_no),
        scd_out(_scd_out),
        pnr(_pnr),
        name(_name),
        pers_type(_pers_type),
        cls(_cls),
        client_type(_client_type),
        baggage(_baggage),
        gate(_gate),
        seat_no(_seat_no)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TADFullStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("АК") << delim
        << getLocaleText("АП") << delim
        << getLocaleText("Номер рейса") << delim
        << getLocaleText("Дата") << delim
        << "PNR" << delim
        << getLocaleText("Ф.И.О.") << delim
        << getLocaleText("Тип") << delim
        << getLocaleText("Класс") << delim
        << getLocaleText("Тип рег.") << delim
        << getLocaleText("Багаж") << delim
        << getLocaleText("Вых. на посадку") << delim
        << getLocaleText("№ м") << endl;

}

void TADFullStatCombo::add_data(ostringstream &buf) const
{
    buf
        << airline << delim
        << airp << delim
        << flt_no << delim
        << DateTimeToStr(scd_out, "dd.mm.yy") << delim
        << pnr << delim
        << name << delim
        << ElemIdToCodeNative(etPersType,EncodePerson(pers_type)) << delim
        << ElemIdToCodeNative(etClass, cls) << delim
        << EncodeClientType(client_type) << delim
        << baggage << delim
        << gate << delim
        << seat_no << endl;
}

void RunADFullFile(const TStatParams &params, TOrderStatWriter &writer)
{
    TADFullStat ADFullStat;
    RunADStat(params, ADFullStat, true);

    for(TADFullStat::const_iterator airline = ADFullStat.begin();
            airline != ADFullStat.end(); airline++) {
        for(TADFullAirpMap::const_iterator airp = airline->second.begin();
                airp != airline->second.end(); airp++) {
            for(TADFullFltMap::const_iterator flt = airp->second.begin();
                    flt != airp->second.end(); flt++) {
                for(TADFullSCDMap::const_iterator scd = flt->second.begin();
                        scd != flt->second.end(); scd++) {
                    for(TADFullNameMap::const_iterator name = scd->second.begin();
                            name != scd->second.end(); name++) {
                        for(TADFullPaxIdMap::const_iterator pax_id = name->second.begin();
                                pax_id != name->second.end(); pax_id++) {
                            writer.insert(TADFullStatCombo(
                                        airline->first,
                                        airp->first,
                                        flt->first,
                                        scd->first,
                                        pax_id->second.pnr,
                                        name->first,
                                        pax_id->second.pers_type,
                                        pax_id->second.cls,
                                        pax_id->second.client_type,
                                        pax_id->second.baggage,
                                        pax_id->second.gate,
                                        pax_id->second.seat_no));
                        }
                    }
                }
            }
        }
    }
}

void get_stat_ad(int point_id)
{
    TCachedQuery delQry("delete from stat_ad where point_id = :point_id", QParams() << QParam("point_id", otInteger, point_id));
    delQry.get().Execute();

    TCachedQuery Qry(
            "select "
            "   pax_grp.*, "
            "   NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS bag_amount, "
            "   NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS bag_weight, "
            "   salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,null,rownum,0) AS seat_no, "
            "   salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,null,rownum,1) AS seat_no_lat, "
            "   pax.pax_id, "
            "   points.scd_out "
            "from "
            "   points, "
            "   pax_grp, "
            "   pax "
            "where "
            "   points.point_id = :point_id and "
            "   points.point_id = pax_grp.point_dep and "
            "   pax_grp.status NOT IN ('E') AND "
            "   pax_grp.grp_id = pax.grp_id and "
            "   pax.pr_brd = 1 ",
            QParams()
            << QParam("point_id", otInteger, point_id));
    QParams insQryParams;
    insQryParams
        << QParam("scd_out", otDate)
        << QParam("point_id", otInteger, point_id)
        << QParam("pax_id", otInteger)
        << QParam("pnr", otString)
        << QParam("class", otString)
        << QParam("client_type", otString)
        << QParam("bag_amount", otInteger)
        << QParam("bag_weight", otInteger)
        << QParam("seat_no", otString)
        << QParam("seat_no_lat", otString)
        << QParam("desk", otString)
        << QParam("station", otString);
    TCachedQuery insQry(
            "insert into stat_ad ( "
            "   scd_out, "
            "   point_id, "
            "   pax_id, "
            "   pnr, "
            "   class, "
            "   client_type, "
            "   bag_amount, "
            "   bag_weight, "
            "   seat_no, "
            "   seat_no_lat, "
            "   desk, "
            "   station "
            ") values ( "
            "   :scd_out, "
            "   :point_id, "
            "   :pax_id, "
            "   :pnr, "
            "   :class, "
            "   :client_type, "
            "   :bag_amount, "
            "   :bag_weight, "
            "   :seat_no, "
            "   :seat_no_lat, "
            "   :desk, "
            "   :station "
            ") ", insQryParams);
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next()) {
        int pax_id = Qry.get().FieldAsInteger("pax_id");
        TPaxEvent pe;
        if(pe.fromDB(pax_id, TPaxEventTypes::BRD)) {
            string pnr=TPnrAddrs().firstAddrByPaxId(pax_id, TPnrAddrInfo::AddrOnly);

            CheckIn::TPaxGrpItem grp;
            grp.fromDB(Qry.get());

            insQry.get().SetVariable("scd_out", Qry.get().FieldAsDateTime("scd_out"));
            insQry.get().SetVariable("pax_id", pax_id);
            insQry.get().SetVariable("pnr", pnr);
            insQry.get().SetVariable("class", grp.cl);
            insQry.get().SetVariable("client_type", EncodeClientType(grp.client_type));
            if(Qry.get().FieldIsNULL("bag_amount"))
                insQry.get().SetVariable("bag_amount", FNull);
            else
                insQry.get().SetVariable("bag_amount", Qry.get().FieldAsInteger("bag_amount"));

            int bag_weight = Qry.get().FieldAsInteger("bag_weight");
            if(bag_weight)
                insQry.get().SetVariable("bag_weight", bag_weight);
            else
                insQry.get().SetVariable("bag_weight", FNull);

            int bag_amount = Qry.get().FieldAsInteger("bag_amount");
            if(bag_amount)
                insQry.get().SetVariable("bag_amount", bag_amount);
            else
                insQry.get().SetVariable("bag_amount", FNull);

            insQry.get().SetVariable("seat_no", Qry.get().FieldAsString("seat_no"));
            insQry.get().SetVariable("seat_no_lat", Qry.get().FieldAsString("seat_no_lat"));
            insQry.get().SetVariable("desk", pe.desk);
            insQry.get().SetVariable("station", pe.station);
            insQry.get().Execute();
        }
    }
}
