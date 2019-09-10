#include "stat_services.h"
#include "docs/docs_services.h"
#include "report_common.h"
#include "stat_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;

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
        << QParam("airp_dep", otString)
        << QParam("airp_arv", otString)
        << QParam("rfic", otString)
        << QParam("rfisc", otString)
        << QParam("receipt_no", otString);
    TCachedQuery insQry(
            "insert into stat_services( "
            "   point_id, "
            "   scd_out, "
            "   pax_id, "
            "   airp_dep, "
            "   airp_arv, "
            "   rfic, "
            "   rfisc, "
            "   receipt_no "
            ") values ( "
            "   :point_id, "
            "   :scd_out, "
            "   :pax_id, "
            "   :airp_dep, "
            "   :airp_arv, "
            "   :rfic, "
            "   :rfisc, "
            "   :receipt_no "
            ") ",insQryParams);

    TRptParams rpt_params;
    rpt_params.point_id = point_id;
    TServiceList rows(true);
    rows.fromDB(rpt_params);
    for(const auto &row: rows) {
        insQry.get().SetVariable("pax_id", row.pax_id);
        insQry.get().SetVariable("airp_dep", row.airp_dep);
        insQry.get().SetVariable("airp_arv", row.airp_arv);
        insQry.get().SetVariable("rfic", row.RFIC);
        insQry.get().SetVariable("rfisc", row.RFISC);
        insQry.get().SetVariable("receipt_no", row.num);

        insQry.get().Execute();
    }
}

void TServicesFullStat::add(const TServicesStatRow &row)
{
    prn_airline.check(row.airline);

    CheckIn::TSimplePaxItem pax;
    pax.getByPaxId(row.pax_id, row.part_key);
    // из-за full_name пришлось копию делать...
    TServicesStatRow insert_row = row;
    insert_row.full_name = transliter(pax.surname + " " + pax.name,1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);
    insert_row.ticket_no = pax.tkn.no_str();
    insert_row.airp_dep = ElemIdToCodeNative(etAirp, row.airp_dep);
    insert_row.airp_arv = ElemIdToCodeNative(etAirp, row.airp_arv);
    (*this)[row.airp][row.view_airline][row.flt_no][row.scd_out].insert(insert_row);
    FRowCount++;
}

void TServicesDetailStat::add(const TServicesStatRow &row)
{
    prn_airline.check(row.airline);
    int &amount = (*this)[row.airp][row.view_airline][row.flt_no][row.scd_out][row.RFIC][row.RFISC];
    if(not amount) FRowCount++;
    amount++;
}

void TServicesShortStat::add(const TServicesStatRow &row)
{
    prn_airline.check(row.airline);
    int &amount = (*this)[row.airp][row.view_airline][row.RFIC][row.RFISC];
    if(not amount) FRowCount++;
    amount++;
}

void RunServicesStat(
        const TStatParams &params,
        TServicesAbstractStat &ServicesStat
        )
{
    TFltInfoCache flt_cache;
    for(int pass = 0; pass <= 2; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate);
        if (pass!=0)
            QryParams << QParam("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        string SQLText = "select stat_services.* from ";
        if(pass != 0) {
            SQLText +=
                "   arx_stat_services stat_services, "
                "   arx_points points ";
            if(pass == 2)
                SQLText += ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        } else {
            SQLText +=
                "   stat_services, "
                "   points ";
        }
        SQLText +=
            "where "
            "   stat_services.point_id = points.point_id and "
            "   points.pr_del >= 0 and ";

        params.AccessClause(SQLText);

        if(params.flt_no != NoExists) {
            SQLText += " points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        if(pass != 0)
            SQLText +=
                " points.part_key = stat_services.part_key and ";
        if(pass == 1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        SQLText += "   stat_services.scd_out >= :FirstDate AND stat_services.scd_out < :LastDate ";
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_part_key = Qry.get().GetFieldIndex("part_key");
            int col_point_id = Qry.get().FieldIndex("point_id");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_pax_id = Qry.get().FieldIndex("pax_id");
            int col_airp_dep = Qry.get().FieldIndex("airp_dep");
            int col_airp_arv = Qry.get().FieldIndex("airp_arv");
            int col_rfic = Qry.get().FieldIndex("rfic");
            int col_rfisc = Qry.get().FieldIndex("rfisc");
            int col_receipt_no = Qry.get().FieldIndex("receipt_no");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                TServicesStatRow row;
                if(col_part_key >= 0)
                    row.part_key = Qry.get().FieldAsDateTime(col_part_key);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                const TFltInfoCacheItem &info = flt_cache.get(row.point_id, row.part_key);
                row.airp = info.view_airp;
                row.airline = info.airline;
                row.view_airline = info.view_airline;
                row.flt_no = info.view_flt_no;
                row.pax_id = Qry.get().FieldAsInteger(col_pax_id);
                row.airp_dep = Qry.get().FieldAsString(col_airp_dep);
                row.airp_arv = Qry.get().FieldAsString(col_airp_arv);
                row.RFIC = Qry.get().FieldAsString(col_rfic);
                row.RFISC = Qry.get().FieldAsString(col_rfisc);
                row.receipt_no = Qry.get().FieldAsString(col_receipt_no);

                ServicesStat.add(row);

                params.overflow.check(ServicesStat.RowCount());
            }
        }
    }
}

void createXMLServicesFullStat(
        const TStatParams &params,
        const TServicesFullStat &ServicesFullStat,
        xmlNodePtr resNode)
{
    if(ServicesFullStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", ServicesFullStat.prn_airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("АП"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АК"));
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
    colNode = NewTextChild(headerNode, "col", getLocaleText("Ф.И.О."));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Билет"));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("От"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("До"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("RFIC"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("RFISC"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("№ квитанции"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;

    for(const auto &airp: ServicesFullStat) {
        for(const auto &airline: airp.second) {
            for(const auto &flt: airline.second) {
                for(const auto &scd_out: flt.second) {
                    for(const auto &pax: scd_out.second) {
                        rowNode = NewTextChild(rowsNode, "row");
                        //АП
                        NewTextChild(rowNode, "col", airp.first);
                        //АК
                        NewTextChild(rowNode, "col", airline.first);
                        //Рейс
                        NewTextChild(rowNode, "col", flt.first);
                        // Дата вылета
                        NewTextChild(rowNode, "col", DateTimeToStr(scd_out.first, "dd.mm.yy"));
                        //ФИО
                        NewTextChild(rowNode, "col", pax.full_name);
                        //Билет
                        NewTextChild(rowNode, "col", pax.ticket_no);
                        //От
                        NewTextChild(rowNode, "col", pax.airp_dep);
                        //До
                        NewTextChild(rowNode, "col", pax.airp_arv);
                        //RFIC
                        NewTextChild(rowNode, "col", pax.RFIC);
                        //RFISC
                        NewTextChild(rowNode, "col", pax.RFISC);
                        //№ квитанции
                        NewTextChild(rowNode, "col", pax.receipt_no);
                    }
                }
            }
        }
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Услуги"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
}

void createXMLServicesDetailStat(
        const TStatParams &params,
        const TServicesDetailStat &ServicesDetailStat,
        xmlNodePtr resNode)
{
    if(ServicesDetailStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", ServicesDetailStat.prn_airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("АП"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АК"));
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
    colNode = NewTextChild(headerNode, "col", getLocaleText("RFIC"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("RFISC"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortInteger);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;

    for(const auto &airp: ServicesDetailStat) {
        for(const auto &airline: airp.second) {
            for(const auto &flt: airline.second) {
                for(const auto &scd_out: flt.second) {
                    for(const auto &rfic: scd_out.second) {
                        for(const auto &rfisc: rfic.second) {
                            rowNode = NewTextChild(rowsNode, "row");
                            //АП
                            NewTextChild(rowNode, "col", airp.first);
                            //АК
                            NewTextChild(rowNode, "col", airline.first);
                            //Рейс
                            NewTextChild(rowNode, "col", flt.first);
                            // Дата вылета
                            NewTextChild(rowNode, "col", DateTimeToStr(scd_out.first, "dd.mm.yy"));
                            //RFIC
                            NewTextChild(rowNode, "col", rfic.first);
                            //RFISC
                            NewTextChild(rowNode, "col", rfisc.first);
                            //Кол-во
                            NewTextChild(rowNode, "col", rfisc.second);
                        }
                    }
                }
            }
        }
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Услуги"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Детализированная"));
}

void createXMLServicesShortStat(
        const TStatParams &params,
        const TServicesShortStat &ServicesShortStat,
        xmlNodePtr resNode)
{
    if(ServicesShortStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");

    NewTextChild(resNode, "airline", ServicesShortStat.prn_airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("АП"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АК"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("RFIC"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("RFISC"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortInteger);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;

    for(const auto &airp: ServicesShortStat) {
        for(const auto &airline: airp.second) {
            for(const auto &rfic: airline.second) {
                for(const auto &rfisc: rfic.second) {
                    rowNode = NewTextChild(rowsNode, "row");
                    //АП
                    NewTextChild(rowNode, "col", airp.first);
                    //АК
                    NewTextChild(rowNode, "col", airline.first);
                    //RFIC
                    NewTextChild(rowNode, "col", rfic.first);
                    //RFISC
                    NewTextChild(rowNode, "col", rfisc.first);
                    //Кол-во
                    NewTextChild(rowNode, "col", rfisc.second);
                }
            }
        }
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Услуги"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Общая"));
}

struct TServicesFullStatCombo : public TOrderStatItem
{
    string airp;
    string airline;
    string flt;
    TDateTime scd_out;
    string full_name;
    string ticket_no;
    string airp_dep;
    string airp_arv;
    string RFIC;
    string RFISC;
    string receipt_no;

    TServicesFullStatCombo(
            const string &_airp,
            const string &_airline,
            const string &_flt,
            TDateTime _scd_out,
            const string &_full_name,
            const string &_ticket_no,
            const string &_airp_dep,
            const string &_airp_arv,
            const string &_RFIC,
            const string &_RFISC,
            const string &_receipt_no
            ):
        airp(_airp),
        airline(_airline),
        flt(_flt),
        scd_out(_scd_out),
        full_name(_full_name),
        ticket_no(_ticket_no),
        airp_dep(_airp_dep),
        airp_arv(_airp_arv),
        RFIC(_RFIC),
        RFISC(_RFISC),
        receipt_no(_receipt_no)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TServicesFullStatCombo::add_data(ostringstream &buf) const
{
    buf
        << airp << delim
        << airline << delim
        << flt << delim
        << DateTimeToStr(scd_out, "dd.mm.yy") << delim
        << full_name << delim
        << ticket_no << delim
        << airp_dep << delim
        << airp_arv << delim
        << RFIC << delim
        << RFISC << delim
        << receipt_no << endl;
}

void TServicesFullStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("АП") << delim
        << getLocaleText("АК") << delim
        << getLocaleText("Номер рейса") << delim
        << getLocaleText("Дата") << delim
        << getLocaleText("Ф.И.О.") << delim
        << getLocaleText("Билет") << delim
        << getLocaleText("От") << delim
        << getLocaleText("До") << delim
        << getLocaleText("RFIC") << delim
        << getLocaleText("RFISC") << delim
        << getLocaleText("№ квитанции") << endl;
}

void RunServicesFullFile(const TStatParams &params, TOrderStatWriter &writer)
{
    TServicesFullStat ServicesFullStat;
    RunServicesStat(params, ServicesFullStat);
    for(const auto &airp: ServicesFullStat) {
        for(const auto &airline: airp.second) {
            for(const auto &flt: airline.second) {
                for(const auto &scd_out: flt.second) {
                    for(const auto &pax: scd_out.second) {
                        writer.insert(TServicesFullStatCombo(
                                    airp.first,
                                    airline.first,
                                    flt.first,
                                    scd_out.first,
                                    pax.full_name,
                                    pax.ticket_no,
                                    pax.airp_dep,
                                    pax.airp_arv,
                                    pax.RFIC,
                                    pax.RFISC,
                                    pax.receipt_no
                                    ));
                    }
                }
            }
        }
    }
}

struct TServicesDetailStatCombo : public TOrderStatItem
{
    string airp;
    string airline;
    string flt;
    TDateTime scd_out;
    string RFIC;
    string RFISC;
    int amount;

    TServicesDetailStatCombo(
            const string &_airp,
            const string &_airline,
            const string &_flt,
            TDateTime _scd_out,
            const string &_RFIC,
            const string &_RFISC,
            int _amount
            ):
        airp(_airp),
        airline(_airline),
        flt(_flt),
        scd_out(_scd_out),
        RFIC(_RFIC),
        RFISC(_RFISC),
        amount(_amount)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TServicesDetailStatCombo::add_data(ostringstream &buf) const
{
    buf
        << airp << delim
        << airline << delim
        << flt << delim
        << DateTimeToStr(scd_out, "dd.mm.yy") << delim
        << RFIC << delim
        << RFISC << delim
        << amount << endl;
}

void TServicesDetailStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("АП") << delim
        << getLocaleText("АК") << delim
        << getLocaleText("Номер рейса") << delim
        << getLocaleText("Дата") << delim
        << getLocaleText("RFIC") << delim
        << getLocaleText("RFISC") << delim
        << getLocaleText("Кол-во") << endl;
}

void RunServicesDetailFile(const TStatParams &params, TOrderStatWriter &writer)
{
    TServicesDetailStat ServicesDetailStat;
    RunServicesStat(params, ServicesDetailStat);
    for(const auto &airp: ServicesDetailStat) {
        for(const auto &airline: airp.second) {
            for(const auto &flt: airline.second) {
                for(const auto &scd_out: flt.second) {
                    for(const auto &rfic: scd_out.second) {
                        for(const auto &rfisc: rfic.second) {
                            writer.insert(TServicesDetailStatCombo(
                                        airp.first,
                                        airline.first,
                                        flt.first,
                                        scd_out.first,
                                        rfic.first,
                                        rfisc.first,
                                        rfisc.second
                                        ));
                        }
                    }
                }
            }
        }
    }
}

struct TServicesShortStatCombo : public TOrderStatItem
{
    string airp;
    string airline;
    string RFIC;
    string RFISC;
    int amount;

    TServicesShortStatCombo(
            const string &_airp,
            const string &_airline,
            const string &_RFIC,
            const string &_RFISC,
            int _amount
            ):
        airp(_airp),
        airline(_airline),
        RFIC(_RFIC),
        RFISC(_RFISC),
        amount(_amount)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TServicesShortStatCombo::add_data(ostringstream &buf) const
{
    buf
        << airp << delim
        << airline << delim
        << RFIC << delim
        << RFISC << delim
        << amount << endl;
}

void TServicesShortStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("АП") << delim
        << getLocaleText("АК") << delim
        << getLocaleText("RFIC") << delim
        << getLocaleText("RFISC") << delim
        << getLocaleText("Кол-во") << endl;
}

void RunServicesShortFile(const TStatParams &params, TOrderStatWriter &writer)
{
    TServicesShortStat ServicesShortStat;
    RunServicesStat(params, ServicesShortStat);
    for(const auto &airp: ServicesShortStat) {
        for(const auto &airline: airp.second) {
            for(const auto &rfic: airline.second) {
                for(const auto &rfisc: rfic.second) {
                    writer.insert(TServicesShortStatCombo(
                                airp.first,
                                airline.first,
                                rfic.first,
                                rfisc.first,
                                rfisc.second
                                ));
                }
            }
        }
    }
}

