#include "stat_self_ckin.h"
#include "astra_misc.h"
#include "astra_date_time.h"
#include "report_common.h"

using namespace std;
using namespace ASTRA;
using namespace ASTRA::date_time;
using namespace BASIC::date_time;
using namespace AstraLocale;
using namespace EXCEPTIONS;

bool TSelfCkinStatRow::operator == (const TSelfCkinStatRow &item) const
{
    return pax_amount == item.pax_amount &&
        term_bp == item.term_bp &&
        term_bag == item.term_bag &&
        term_ckin_service == item.term_ckin_service &&
        adult == item.adult &&
        child == item.child &&
        baby == item.baby &&
        tckin == item.tckin &&
        flts.size() == item.flts.size();
};

void TSelfCkinStatRow::operator += (const TSelfCkinStatRow &item)
{
    pax_amount += item.pax_amount;
    term_bp += item.term_bp;
    term_bag += item.term_bag;
    term_ckin_service += item.term_ckin_service;
    adult += item.adult;
    child += item.child;
    baby += item.baby;
    tckin += item.tckin;
    flts.insert(item.flts.begin(),item.flts.end());
};

bool TKioskCmp::operator() (const TSelfCkinStatKey &lr, const TSelfCkinStatKey &rr) const
{
    if(lr.client_type == rr.client_type)
        if(lr.desk == rr.desk)
            if(lr.desk_airp == rr.desk_airp)
                if(lr.ak == rr.ak)
                    if(lr.ap == rr.ap)
                        if(lr.flt_no == rr.flt_no)
                            if(lr.scd_out == rr.scd_out)
                                if(lr.point_id == rr.point_id)
                                    return lr.places.get() < rr.places.get();
                                else
                                    return lr.point_id < rr.point_id;
                            else
                                return lr.scd_out < rr.scd_out;
                        else
                            return lr.flt_no < rr.flt_no;
                    else
                        return lr.ap < rr.ap;
                else
                    return lr.ak < rr.ak;
            else
                return lr.desk_airp < rr.desk_airp;
        else
            return lr.desk < rr.desk;
    else
        return lr.client_type < rr.client_type;
}


void RunSelfCkinStat(const TStatParams &params,
                  TSelfCkinStat &SelfCkinStat, TSelfCkinStatRow &SelfCkinStatTotal,
                  TPrintAirline &prn_airline)
{
    TQuery Qry(&OraSession);
    for(int pass = 0; pass <= 2; pass++) {
        string SQLText =
            "select "
            "    points.airline, "
            "    points.airp, "
            "    points.scd_out, "
            "    points.flt_no, "
            "    self_ckin_stat.point_id, "
            "    self_ckin_stat.client_type, "
            "    desk, "
            "    desk_airp, "
            "    descr, "
            "    adult, "
            "    child, "
            "    baby, "
            "    term_bp, "
            "    term_bag, "
            "    term_ckin_service, "
            "    tckin "
            "from ";
        if(pass != 0) {
            SQLText +=
                " arx_points points, "
                " arx_self_ckin_stat self_ckin_stat ";
            if(pass == 2)
                SQLText +=
                    ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        } else {
            SQLText +=
                " points, "
                " self_ckin_stat ";
        }
        SQLText += "where ";
        if (pass==1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if (pass==2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        if (pass!=0)
            SQLText += " self_ckin_stat.part_key = points.part_key AND \n";

        params.AccessClause(SQLText);

        SQLText +=
            "    self_ckin_stat.point_id = points.point_id and "
            "    points.pr_del >= 0 and "
            "    points.scd_out >= :FirstDate and "
            "    points.scd_out < :LastDate ";

        if(not params.reg_type.empty()) {
            SQLText += " and self_ckin_stat.client_type = :reg_type ";
            Qry.CreateVariable("reg_type", otString, params.reg_type);
        }
        if(params.flt_no != NoExists) {
            SQLText += " and points.flt_no = :flt_no ";
            Qry.CreateVariable("flt_no", otInteger, params.flt_no);
        }
        if(!params.desk.empty()) {
            SQLText += "and self_ckin_stat.desk = :desk ";
            Qry.CreateVariable("desk", otString, params.desk);
        }
        Qry.SQLText = SQLText;
        Qry.CreateVariable("FirstDate", otDate, params.FirstDate);
        Qry.CreateVariable("LastDate", otDate, params.LastDate);
        if (pass!=0)
            Qry.CreateVariable("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        Qry.Execute();
        if(not Qry.Eof) {
            int col_airline = Qry.FieldIndex("airline");
            int col_airp = Qry.FieldIndex("airp");
            int col_scd_out = Qry.FieldIndex("scd_out");
            int col_flt_no = Qry.FieldIndex("flt_no");
            int col_point_id = Qry.FieldIndex("point_id");
            int col_client_type = Qry.FieldIndex("client_type");
            int col_desk = Qry.FieldIndex("desk");
            int col_desk_airp = Qry.FieldIndex("desk_airp");
            int col_descr = Qry.FieldIndex("descr");
            int col_adult = Qry.FieldIndex("adult");
            int col_child = Qry.FieldIndex("child");
            int col_term_bp = Qry.FieldIndex("term_bp");
            int col_term_bag = Qry.FieldIndex("term_bag");
            int col_term_ckin_service = Qry.FieldIndex("term_ckin_service");
            int col_baby = Qry.FieldIndex("baby");
            int col_tckin = Qry.FieldIndex("tckin");
            for(; not Qry.Eof; Qry.Next())
            {
              string airline = Qry.FieldAsString(col_airline);
              prn_airline.check(airline);

              TSelfCkinStatRow row;
              row.adult = Qry.FieldAsInteger(col_adult);
              row.child = Qry.FieldAsInteger(col_child);
              row.baby = Qry.FieldAsInteger(col_baby);
              row.tckin = Qry.FieldAsInteger(col_tckin);
              row.pax_amount = row.adult + row.child + row.baby;
              row.term_bp = Qry.FieldAsInteger(col_term_bp);
              row.term_bag = Qry.FieldAsInteger(col_term_bag);
              if (Qry.FieldIsNULL(col_term_ckin_service))
                row.term_ckin_service = row.pax_amount;
              else
                row.term_ckin_service = Qry.FieldAsInteger(col_term_ckin_service);
              int point_id=Qry.FieldAsInteger(col_point_id);
              row.flts.insert(point_id);
              if (!params.skip_rows)
              {
                string airp = Qry.FieldAsString(col_airp);
                TDateTime scd_out = Qry.FieldAsDateTime(col_scd_out);
                int flt_no = Qry.FieldAsInteger(col_flt_no);
                TSelfCkinStatKey key;
                key.client_type = Qry.FieldAsString(col_client_type);
                key.desk = Qry.FieldAsString(col_desk);
                key.desk_airp = ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_desk_airp));
                key.descr = Qry.FieldAsString(col_descr);

                key.ak = ElemIdToCodeNative(etAirline, airline);
                if(
                        params.statType == statSelfCkinDetail or
                        params.statType == statSelfCkinFull
                  )
                    key.ap = airp;
                if(params.statType == statSelfCkinFull) {
                    key.flt_no = flt_no;
                    key.scd_out = scd_out;
                    key.point_id = point_id;
                    key.places.set(GetRouteAfterStr( NoExists, point_id, trtNotCurrent, trtNotCancelled), false);
                }

                AddStatRow(params.overflow, key, row, SelfCkinStat);
              }
              else
              {
                SelfCkinStatTotal+=row;
              };
            }
        }
    }
}

void createXMLSelfCkinStat(const TStatParams &params,
                        const TSelfCkinStat &SelfCkinStat, const TSelfCkinStatRow &SelfCkinStatTotal,
                        const TPrintAirline &airline, xmlNodePtr resNode)
{
    if(SelfCkinStat.empty() && SelfCkinStatTotal==TSelfCkinStatRow())
        throw AstraLocale::UserException("MSG.NOT_DATA");

    NewTextChild(resNode, "airline", airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    TSelfCkinStatRow total;
    bool showTotal=true;
    int flts_total = 0;
    int rows = 0;
    for(TSelfCkinStat::const_iterator im = SelfCkinStat.begin(); im != SelfCkinStat.end(); ++im, rows++)
    {
        //region обязательно в начале цикла, иначе будет испорчен xml
        string region;
        if(params.statType == statSelfCkinFull)
        {
            try
            {
                region = AirpTZRegion(im->first.ap);
            }
            catch(AstraLocale::UserException &E)
            {
                AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
                if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
                continue;
            };
        };

        rowNode = NewTextChild(rowsNode, "row");
        // Тип рег.
        NewTextChild(rowNode, "col", im->first.client_type);
        // Пульт
        NewTextChild(rowNode, "col", im->first.desk);
        // А/П пульта
        NewTextChild(rowNode, "col", im->first.desk_airp);
        // примечание
        if(params.statType == statSelfCkinFull)
            NewTextChild(rowNode, "col", im->first.descr);
        // код а/к
        NewTextChild(rowNode, "col", im->first.ak);
        if(
                params.statType == statSelfCkinDetail or
                params.statType == statSelfCkinFull
          )
            // код а/п
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, im->first.ap));
        if(
                params.statType == statSelfCkinShort or
                params.statType == statSelfCkinDetail
          ) {
            // Кол-во рейсов
            NewTextChild(rowNode, "col", (int)im->second.flts.size());
            flts_total += im->second.flts.size();
        }
        if(params.statType == statSelfCkinFull) {
            // номер рейса
            ostringstream buf;
            buf << setw(3) << setfill('0') << im->first.flt_no;
            NewTextChild(rowNode, "col", buf.str().c_str());
            // Дата
            NewTextChild(rowNode, "col", DateTimeToStr(
                        UTCToClient(im->first.scd_out, region), "dd.mm.yy")
                    );
            // Направление
            NewTextChild(rowNode, "col", im->first.places.get());
        }
        // Кол-во пасс.
        NewTextChild(rowNode, "col", im->second.pax_amount);
        NewTextChild(rowNode, "col", im->second.term_bag);
        NewTextChild(rowNode, "col", im->second.term_bp);
        NewTextChild(rowNode, "col", im->second.pax_amount - im->second.term_ckin_service);

        if(params.statType == statSelfCkinFull) {
            // ВЗ
            NewTextChild(rowNode, "col", im->second.adult);
            // РБ
            NewTextChild(rowNode, "col", im->second.child);
            // РМ
            NewTextChild(rowNode, "col", im->second.baby);
        }
        if(
                params.statType == statSelfCkinDetail or
                params.statType == statSelfCkinFull
          )
            // Сквоз. рег.
            NewTextChild(rowNode, "col", im->second.tckin);
        if(
                params.statType == statSelfCkinShort or
                params.statType == statSelfCkinDetail
          )
            // Примечание
            NewTextChild(rowNode, "col", im->first.descr);

        total += im->second;
    };

    rowNode = NewTextChild(rowsNode, "row");

    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("Тип рег."));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col", getLocaleText("Итого:"));
    colNode = NewTextChild(headerNode, "col", getLocaleText("Пульт"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    colNode = NewTextChild(headerNode, "col", getLocaleText("АП пульта"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if(params.statType == statSelfCkinFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Примечание"));
        SetProp(colNode, "width", 280);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/к"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if(
            params.statType == statSelfCkinDetail or
            params.statType == statSelfCkinFull
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    if(
            params.statType == statSelfCkinShort or
            params.statType == statSelfCkinDetail
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во рейсов"));
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", flts_total);
    }
    if(params.statType == statSelfCkinFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Номер рейса"));
        SetProp(colNode, "width", 75);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col");
        colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortDate);
        NewTextChild(rowNode, "col");
        colNode = NewTextChild(headerNode, "col", getLocaleText("Направление"));
        SetProp(colNode, "width", 90);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }


    // Киоски
    colNode = NewTextChild(headerNode, "col", getLocaleText("Пас."));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount);

    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.term_bag);

    colNode = NewTextChild(headerNode, "col", getLocaleText("ПТ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.term_bp);

    colNode = NewTextChild(headerNode, "col", getLocaleText("Всё сами"));
    SetProp(colNode, "width", 45);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount-total.term_ckin_service);

    if(params.statType == statSelfCkinFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("ВЗ"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.adult);
        colNode = NewTextChild(headerNode, "col", getLocaleText("РБ"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.child);
        colNode = NewTextChild(headerNode, "col", getLocaleText("РМ"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.baby);
    }
    if(
            params.statType == statSelfCkinDetail or
            params.statType == statSelfCkinFull
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Сквоз."));
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.tckin);
    }
    if(
            params.statType == statSelfCkinShort or
            params.statType == statSelfCkinDetail
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Примечание"));
        SetProp(colNode, "width", 280);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }

    if (!showTotal)
    {
        xmlUnlinkNode(rowNode);
        xmlFreeNode(rowNode);
    };

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Саморегистрация"));
    string buf;
    switch(params.statType) {
        case statSelfCkinShort:
            buf = getLocaleText("Общая");
            break;
        case statSelfCkinDetail:
            buf = getLocaleText("Детализированная");
            break;
        case statSelfCkinFull:
            buf = getLocaleText("Подробная");
            break;
        default:
            throw Exception("createXMLSelfCkinStat: unexpected statType %d", params.statType);
            break;
    }
    NewTextChild(variablesNode, "stat_type_caption", buf);
}

struct TSelfCkinStatCombo : public TOrderStatItem
{
    std::pair<TSelfCkinStatKey, TSelfCkinStatRow> data;
    TStatParams params;
    TSelfCkinStatCombo(const std::pair<TSelfCkinStatKey, TSelfCkinStatRow> &aData,
        const TStatParams &aParams): data(aData), params(aParams) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TSelfCkinStatCombo::add_header(ostringstream &buf) const
{
    buf << getLocaleText("Тип рег.") << delim;
    buf << getLocaleText("Пульт") << delim;
    buf << getLocaleText("АП пульта") << delim;
    if (params.statType == statSelfCkinFull)
        buf << getLocaleText("Примечание") << delim;
    buf << getLocaleText("Код а/к") << delim;
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << getLocaleText("Код а/п") << delim;
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << getLocaleText("Кол-во рейсов") << delim;
    if (params.statType == statSelfCkinFull)
    {
        buf << getLocaleText("Номер рейса") << delim;
        buf << getLocaleText("Дата") << delim;
        buf << getLocaleText("Направление") << delim;
    }
    buf << getLocaleText("Пас.") << delim;
    buf << getLocaleText("БГ") << delim;
    buf << getLocaleText("ПТ") << delim;
    buf << getLocaleText("Всё сами") << delim;
    if (params.statType == statSelfCkinFull)
    {
        buf << getLocaleText("ВЗ") << delim;
        buf << getLocaleText("РБ") << delim;
        buf << getLocaleText("РМ") << delim;
    }
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << getLocaleText("Сквоз.") << delim;
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << getLocaleText("Примечание") << delim;
    buf << endl;
}

void TSelfCkinStatCombo::add_data(ostringstream &buf) const
{
    string region;
    if (params.statType == statSelfCkinFull)
    {
        try { region = AirpTZRegion(data.first.ap); }
        catch(AstraLocale::UserException &E) { return; };
    }
    buf << data.first.client_type << delim; // Тип рег.
    buf << data.first.desk << delim; // Пульт
    buf << data.first.desk_airp << delim; // А/П пульта
    if (params.statType == statSelfCkinFull)
        buf << data.first.descr << delim; // примечание
    buf << data.first.ak << delim; // код а/к
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << ElemIdToCodeNative(etAirp, data.first.ap) << delim; // код а/п
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << (int)data.second.flts.size() << delim; // Кол-во рейсов
    if (params.statType == statSelfCkinFull)
    {
        ostringstream oss1;
        oss1 << setw(3) << setfill('0') << data.first.flt_no;
        buf << oss1.str() << delim; // номер рейса
        buf << DateTimeToStr(UTCToClient(data.first.scd_out, region), "dd.mm.yy")
         << delim; // Дата
        buf << data.first.places.get() << delim; // Направление
    }
    buf << data.second.pax_amount << delim; // Кол-во пасс.
    buf << data.second.term_bag << delim; // БГ
    buf << data.second.term_bp << delim; // ПТ
    buf << (data.second.pax_amount - data.second.term_ckin_service)
     << delim; // Всё сами
    if (params.statType == statSelfCkinFull)
    {
        buf << data.second.adult << delim; // ВЗ
        buf << data.second.child << delim; // РБ
        buf << data.second.baby << delim; // РМ
    }
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << data.second.tckin << delim; // Сквоз. рег.
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << data.first.descr << delim; // примечание
    buf << endl;
}

void RunSelfCkinStatFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline)
{
    TSelfCkinStat SelfCkinStat;
    TSelfCkinStatRow SelfCkinStatTotal;
    RunSelfCkinStat(params, SelfCkinStat, SelfCkinStatTotal, prn_airline);
    for (TSelfCkinStat::const_iterator i = SelfCkinStat.begin(); i != SelfCkinStat.end(); ++i)
        writer.insert(TSelfCkinStatCombo(*i, params));
}

int nosir_self_ckin(int argc,char **argv)
{
    cout << "start time: " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString) << endl;
    map<string, map<string, int> > result;
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "    points.point_id, "
        "    points.airline, "
        "    scs.client_type "
        "from "
        "    self_ckin_stat scs, "
        "    points "
        "where "
        "    points.scd_out >= to_date('01.11.2015 00:00:00', 'DD.MM.YYYY HH24:MI:SS') and "
        "    points.scd_out < to_date('01.02.2016 00:00:00', 'DD.MM.YYYY HH24:MI:SS') and "
        "    points.point_id = scs.point_id ";
    Qry.Execute();
    if(not Qry.Eof) {
        int col_point_id = Qry.GetFieldIndex("point_id");
        int col_airline = Qry.GetFieldIndex("airline");
        int col_client_type = Qry.GetFieldIndex("client_type");
        int count = 0;
        map<int, string> points;
        for(; not Qry.Eof; Qry.Next(), count++) {
            if(count % 100 == 0) cout << count << endl;
            int point_id = Qry.FieldAsInteger(col_point_id);

            map<int, string>::iterator idx = points.find(point_id);
            if(idx == points.end()) {
                TTripRoute route;
                route.GetRouteAfter(NoExists, point_id, trtNotCurrent, trtNotCancelled);
                string airp_arv;
                if(not route.empty())
                    airp_arv = route.back().airp;
                pair<map<int, string>::iterator, bool> res = points.insert(make_pair(point_id, airp_arv));
                idx = res.first;
            }

            if(idx->second != "СОЧ") continue;
            result[Qry.FieldAsString(col_airline)][Qry.FieldAsString(col_client_type)]++;
        }
    }
    ofstream out("self_ckin.csv");
    for(map<string, map<string, int> >::iterator i = result.begin(); i != result.end(); i++) {
        for(map<string, int>::iterator j = i->second.begin(); j != i->second.end(); j++) {
            out
                << i->first << ","
                << j->first << ","
                << "СОЧ,"
                << j->second << endl;
        }
    }
    cout << "end time: " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString) << endl;
    return 1;
}

