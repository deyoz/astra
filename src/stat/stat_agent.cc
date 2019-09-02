#include "stat_agent.h"
#include "astra_date_time.h"
#include "stat_utils.h"
#include "report_common.h"

using namespace std;
using namespace ASTRA;
using namespace ASTRA::date_time;
using namespace BASIC::date_time;
using namespace AstraLocale;
using namespace EXCEPTIONS;

bool TAgentStatRow::operator == (const TAgentStatRow &item) const
{
    return dpax_amount == item.dpax_amount &&
        dtckin_amount == item.dtckin_amount &&
        dbag_amount == item.dbag_amount &&
        dbag_weight == item.dbag_weight &&
        drk_amount == item.drk_amount &&
        drk_weight == item.drk_weight &&
        processed_pax == item.processed_pax &&
        time == item.time;
};

void TAgentStatRow::operator += (const TAgentStatRow &item)
{
    dpax_amount += item.dpax_amount;
    dtckin_amount += item.dtckin_amount;
    dbag_amount += item.dbag_amount;
    dbag_weight += item.dbag_weight;
    drk_amount += item.drk_amount;
    drk_weight += item.drk_weight;
    processed_pax += item.processed_pax;
    time += item.time;
};

bool TAgentCmp::operator() (const TAgentStatKey &lr, const TAgentStatKey &rr) const
{
    if(lr.airline_view == rr.airline_view)
        if(lr.flt_no == rr.flt_no)
            if(lr.suffix_view == rr.suffix_view)
                if(lr.airp_view == rr.airp_view)
                    if(lr.scd_out_local == rr.scd_out_local)
                        if(lr.point_id == rr.point_id)
                            if(lr.desk == rr.desk)
                                if(lr.user_descr == rr.user_descr)
                                    return lr.user_id < rr.user_id;
                                else
                                    return lr.user_descr < rr.user_descr;
                            else
                                return lr.desk < rr.desk;
                        else
                            return lr.point_id < rr.point_id;
                    else
                        return lr.scd_out_local < rr.scd_out_local;
                else
                    return lr.airp_view < rr.airp_view;
            else
                return lr.suffix_view < rr.suffix_view;
        else
            return lr.flt_no < rr.flt_no;
    else
        return lr.airline_view < rr.airline_view;
}

void RunAgentStat(const TStatParams &params,
                  TAgentStat &AgentStat, TAgentStatRow &AgentStatTotal,
                  TPrintAirline &prn_airline, bool override_max_rows)
{
    TQuery Qry(&OraSession);
    for(int pass = 0; pass <= 1; pass++) {
        string SQLText =
            "SELECT \n"
            "  points.point_id, \n"
            "  points.airline, \n"
            "  points.flt_no, \n"
            "  points.suffix, \n"
            "  points.airp, \n"
            "  points.scd_out, \n"
            "  users2.user_id, \n"
            "  users2.descr AS user_descr, \n"
            "  ags.desk, \n"
            "  pax_time, \n"
            "  pax_amount, \n"
            "  ags.dpax_amount.inc pax_am_inc, \n"
            "  ags.dpax_amount.dec pax_am_dec, \n"
            "  ags.dtckin_amount.inc tckin_am_inc, \n"
            "  ags.dtckin_amount.dec tckin_am_dec, \n"
            "  ags.dbag_amount.inc bag_am_inc, \n"
            "  ags.dbag_amount.dec bag_am_dec, \n"
            "  ags.dbag_weight.inc bag_we_inc, \n"
            "  ags.dbag_weight.dec bag_we_dec, \n"
            "  ags.drk_amount.inc rk_am_inc, \n"
            "  ags.drk_amount.dec rk_am_dec, \n"
            "  ags.drk_weight.inc rk_we_inc, \n"
            "  ags.drk_weight.dec rk_we_dec \n"
            "FROM \n"
            "   users2, \n";
        if(pass != 0) {
            SQLText +=
                "   arx_points points, \n"
                "   arx_agent_stat ags \n";
        } else {
            SQLText +=
                "   points, \n"
                "   agent_stat ags \n";
        }
        SQLText += "WHERE \n";
        if (pass!=0)
            SQLText += "    ags.point_part_key = points.part_key AND \n";
        SQLText +=
            "    ags.point_id = points.point_id AND \n"
            "    points.pr_del >= 0 AND \n"
            "    ags.user_id = users2.user_id AND \n";
        params.AccessClause(SQLText);
        if (pass!=0)
          SQLText +=
            "    ags.part_key >= :FirstDate AND ags.part_key < :LastDate \n";
        else
          SQLText +=
            "    ags.ondate >= :FirstDate AND ags.ondate < :LastDate \n";
        if(params.flt_no != NoExists) {
            SQLText += " AND points.flt_no = :flt_no \n";
            Qry.CreateVariable("flt_no", otInteger, params.flt_no);
        }
        if(!params.desk.empty()) {
            SQLText += " AND ags.desk = :desk \n";
            Qry.CreateVariable("desk", otString, params.desk);
        }
        if(params.user_id != NoExists) {
            SQLText += " AND users2.user_id = :user_id \n";
            Qry.CreateVariable("user_id", otInteger, params.user_id);
        }
        if(!params.user_login.empty()) {
            SQLText += " AND users2.login = :user_login \n";
            Qry.CreateVariable("user_login", otString, params.user_login);
        }
        //ProgTrace(TRACE5, "RunAgentStat: pass=%d SQL=\n%s", pass, SQLText.c_str());
        Qry.SQLText = SQLText;
        Qry.CreateVariable("FirstDate", otDate, params.FirstDate);
        Qry.CreateVariable("LastDate", otDate, params.LastDate);
        Qry.Execute();
        if(not Qry.Eof) {
            int col_point_id = Qry.FieldIndex("point_id");
            int col_airline = Qry.FieldIndex("airline");
            int col_flt_no = Qry.FieldIndex("flt_no");
            int col_suffix = Qry.FieldIndex("suffix");
            int col_airp = Qry.FieldIndex("airp");
            int col_scd_out = Qry.FieldIndex("scd_out");
            int col_user_id = Qry.FieldIndex("user_id");
            int col_user_descr = Qry.FieldIndex("user_descr");
            int col_desk = Qry.FieldIndex("desk");
            int col_pax_time = Qry.FieldIndex("pax_time");
            int col_pax_amount = Qry.FieldIndex("pax_amount");
            int col_pax_am_inc = Qry.FieldIndex("pax_am_inc");
            int col_pax_am_dec = Qry.FieldIndex("pax_am_dec");
            int col_tckin_am_inc = Qry.FieldIndex("tckin_am_inc");
            int col_tckin_am_dec = Qry.FieldIndex("tckin_am_dec");
            int col_bag_am_inc = Qry.FieldIndex("bag_am_inc");
            int col_bag_am_dec = Qry.FieldIndex("bag_am_dec");
            int col_bag_we_inc = Qry.FieldIndex("bag_we_inc");
            int col_bag_we_dec = Qry.FieldIndex("bag_we_dec");
            int col_rk_am_inc = Qry.FieldIndex("rk_am_inc");
            int col_rk_am_dec = Qry.FieldIndex("rk_am_dec");
            int col_rk_we_inc = Qry.FieldIndex("rk_we_inc");
            int col_rk_we_dec = Qry.FieldIndex("rk_we_dec");
            for(; not Qry.Eof; Qry.Next())
            {
              string airline = Qry.FieldAsString(col_airline);
              prn_airline.check(airline);

              TAgentStatRow row;
              row.processed_pax = Qry.FieldAsInteger(col_pax_amount);
              row.time = Qry.FieldAsInteger(col_pax_time);
              row.dpax_amount.inc = Qry.FieldAsInteger(col_pax_am_inc);
              row.dpax_amount.dec = Qry.FieldAsInteger(col_pax_am_dec);
              row.dtckin_amount.inc = Qry.FieldAsInteger(col_tckin_am_inc);
              row.dtckin_amount.dec = Qry.FieldAsInteger(col_tckin_am_dec);
              row.dbag_amount.inc = Qry.FieldAsInteger(col_bag_am_inc);
              row.dbag_amount.dec = Qry.FieldAsInteger(col_bag_am_dec);
              row.dbag_weight.inc = Qry.FieldAsInteger(col_bag_we_inc);
              row.dbag_weight.dec = Qry.FieldAsInteger(col_bag_we_dec);
              row.drk_amount.inc = Qry.FieldAsInteger(col_rk_am_inc);
              row.drk_amount.dec = Qry.FieldAsInteger(col_rk_am_dec);
              row.drk_weight.inc = Qry.FieldAsInteger(col_rk_we_inc);
              row.drk_weight.dec = Qry.FieldAsInteger(col_rk_we_dec);

              if (params.statType == statAgentTotal)
              {
                row.dpax_amount.inc -= row.dpax_amount.dec; //сухой остаток (не суммируем)
                row.dpax_amount.dec = 0;
                row.dtckin_amount.inc -= row.dtckin_amount.dec;
                row.dtckin_amount.dec = 0;
                row.dbag_amount.inc -= row.dbag_amount.dec;
                row.dbag_amount.dec = 0;
                row.dbag_weight.inc -= row.dbag_weight.dec;
                row.dbag_weight.dec = 0;
                row.drk_amount.inc -= row.drk_amount.dec;
                row.drk_amount.dec = 0;
                row.drk_weight.inc -= row.drk_weight.dec;
                row.drk_weight.dec = 0;
              };

              if (!params.skip_rows)
              {
                string airp = Qry.FieldAsString(col_airp);
                TAgentStatKey key;
                if(
                        params.statType == statAgentFull or
                        params.statType == statAgentShort
                        ) {
                    key.point_id = Qry.FieldAsInteger(col_point_id);
                    key.airline_view = ElemIdToCodeNative(etAirline, airline);
                    key.flt_no = Qry.FieldAsInteger(col_flt_no);
                    key.suffix_view = ElemIdToCodeNative(etSuffix, Qry.FieldAsString(col_suffix));
                    key.airp = Qry.FieldAsString(col_airp);
                    key.airp_view = ElemIdToCodeNative(etAirp, key.airp);
                    key.scd_out = Qry.FieldAsDateTime(col_scd_out);
                    try
                    {
                        key.scd_out_local = UTCToClient(key.scd_out, AirpTZRegion(key.airp));
                    }
                    catch(AstraLocale::UserException &E) {};
                }

                if(params.statType == statAgentFull) {
                    key.desk = Qry.FieldAsString(col_desk);
                }
                if(
                        params.statType == statAgentFull or
                        params.statType == statAgentTotal
                  ) {
                    key.user_id = Qry.FieldAsInteger(col_user_id);
                    key.user_descr = Qry.FieldAsString(col_user_descr);
                }

                AddStatRow(key, row, AgentStat, override_max_rows);
              }
              else
              {
                AgentStatTotal+=row;
              };
            }
        }
    }
    return;
}

void createXMLAgentStat(const TStatParams &params,
                        const TAgentStat &AgentStat, const TAgentStatRow &AgentStatTotal,
                        const TPrintAirline &airline, xmlNodePtr resNode)
{
    if(AgentStat.empty() && AgentStatTotal==TAgentStatRow())
      throw AstraLocale::UserException("MSG.NOT_DATA");

    NewTextChild(resNode, "airline", airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    TAgentStatRow total;
    bool showTotal=true;
    if (!params.skip_rows)
    {
      int rows = 0;
      for(TAgentStat::const_iterator im = AgentStat.begin(); im != AgentStat.end(); ++im, rows++)
      {
        if(rows >= MAX_STAT_ROWS()) {
            throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
            /*AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH",
                                          LParams() << LParam("num", MAX_STAT_ROWS()));
            if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
            break;*/
        }
        TDateTime scd_out_local=im->first.scd_out_local;
        if(
                params.statType == statAgentFull or
                params.statType == statAgentShort
          )
        {
          if (scd_out_local==NoExists)
          try
          {
              scd_out_local = UTCToClient(im->first.scd_out, AirpTZRegion(im->first.airp));
          }
          catch(AstraLocale::UserException &E)
          {
              AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
              continue;
          };
        };

        rowNode = NewTextChild(rowsNode, "row");
        ostringstream buf;
        if(
                params.statType == statAgentFull or
                params.statType == statAgentShort
          ) {
            // Код а/к
            buf << im->first.airline_view
                << setw(3) << setfill('0') << im->first.flt_no
                << im->first.suffix_view << " "
                << im->first.airp_view;
            NewTextChild(rowNode, "col", buf.str());
            // Дата
            NewTextChild(rowNode, "col", DateTimeToStr( scd_out_local, "dd.mm.yy"));
        }

        if(params.statType == statAgentFull) {
            // Пульт
            NewTextChild(rowNode, "col", im->first.desk);
        }
        if(
                params.statType == statAgentFull or
                params.statType == statAgentTotal
          ) {
            // Пользователь
            NewTextChild(rowNode, "col", im->first.user_descr);
        }
        if(params.statType == statAgentTotal) {
            // Кол-во пасс.
            NewTextChild(rowNode, "col", im->second.dpax_amount.inc);
            // Кол-во сквоз.
            NewTextChild(rowNode, "col", im->second.dtckin_amount.inc);
            // Багаж (мест/вес)
            NewTextChild(rowNode, "col", IntToString(im->second.dbag_amount.inc) +
                                         "/" +
                                         IntToString(im->second.dbag_weight.inc));
            // Р/кладь (вес)
            NewTextChild(rowNode, "col", im->second.drk_weight.inc);
        } else {
            // Кол-во пасс. (+)
            NewTextChild(rowNode, "col", im->second.dpax_amount.inc);
            // Кол-во пасс. (-)
            NewTextChild(rowNode, "col", -im->second.dpax_amount.dec);
            // Кол-во сквоз. (+)
            NewTextChild(rowNode, "col", im->second.dtckin_amount.inc);
            // Кол-во сквоз. (-)
            NewTextChild(rowNode, "col", -im->second.dtckin_amount.dec);
            // Багаж (мест/вес) (+)
            NewTextChild(rowNode, "col", IntToString(im->second.dbag_amount.inc) +
                                         "/" +
                                         IntToString(im->second.dbag_weight.inc));
            // Багаж (мест/вес) (-)
            NewTextChild(rowNode, "col", IntToString(-im->second.dbag_amount.dec) +
                                         "/" +
                                         IntToString(-im->second.dbag_weight.dec));
            // Р/кладь (вес) (+)
            NewTextChild(rowNode, "col", im->second.drk_weight.inc);
            // Р/кладь (вес) (-)
            NewTextChild(rowNode, "col", -im->second.drk_weight.dec);
            // Среднее время, затраченное на пассажира
            buf.str("");
            if (im->second.processed_pax!=0)
              buf << fixed << setprecision(2) << im->second.time / im->second.processed_pax;
            else
              buf << fixed << setprecision(2) << 0.0;
            NewTextChild(rowNode, "col", buf.str());
        }
        total += im->second;
      };
    }
    else total=AgentStatTotal;

    rowNode = NewTextChild(rowsNode, "row");

    xmlNodePtr colNode;
    if(
            params.statType == statAgentFull or
            params.statType == statAgentShort
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("Итого:"));
        colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortDate);
        NewTextChild(rowNode, "col");
    }
    if(params.statType == statAgentFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Стойка"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    if(
            params.statType == statAgentFull or
            params.statType == statAgentTotal
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Агент"));
        SetProp(colNode, "width", 100);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        if (params.statType == statAgentTotal)
          NewTextChild(rowNode, "col", getLocaleText("Итого:"));
        else
          NewTextChild(rowNode, "col");
    }
    if(params.statType == statAgentTotal) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Пас."));
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dpax_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Сквоз."));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dtckin_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Баг."));
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", TAlignment::Center);
        SetProp(colNode, "sort", sortIntegerSlashInteger);
        NewTextChild(rowNode, "col", IntToString(total.dbag_amount.inc) +
                                     "/" +
                                     IntToString(total.dbag_weight.inc));
        colNode = NewTextChild(headerNode, "col", getLocaleText("Р/к"));
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.drk_weight.inc);
    } else {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Пас.")+" (+)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dpax_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Пас.")+" (-)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", -total.dpax_amount.dec);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Сквоз.")+"(+)");
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dtckin_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Сквоз.")+"(-)");
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", -total.dtckin_amount.dec);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Баг.")+" (+)");
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", TAlignment::Center);
        SetProp(colNode, "sort", sortIntegerSlashInteger);
        NewTextChild(rowNode, "col", IntToString(total.dbag_amount.inc) +
                                     "/" +
                                     IntToString(total.dbag_weight.inc));
        colNode = NewTextChild(headerNode, "col", getLocaleText("Баг.")+" (-)");
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", TAlignment::Center);
        SetProp(colNode, "sort", sortIntegerSlashInteger);
        NewTextChild(rowNode, "col", IntToString(-total.dbag_amount.dec) +
                                     "/" +
                                     IntToString(-total.dbag_weight.dec));
        colNode = NewTextChild(headerNode, "col", getLocaleText("Р/к")+" (+)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.drk_weight.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Р/к")+" (-)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", -total.drk_weight.dec);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Сек./пас."));
        SetProp(colNode, "width", 65);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortFloat);
        ostringstream buf;
        if (total.processed_pax!=0)
          buf << fixed << setprecision(2) << total.time / total.processed_pax;
        else
          buf << fixed << setprecision(2) << 0.0;
        NewTextChild(rowNode, "col", buf.str());
    };

    if (!showTotal)
    {
      xmlUnlinkNode(rowNode);
      xmlFreeNode(rowNode);
    };

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Статистика по агентам"));
    string buf;
    switch(params.statType) {
        case statAgentShort:
            buf = getLocaleText("Общая");
            break;
        case statAgentFull:
            buf = getLocaleText("Подробная");
            break;
        case statAgentTotal:
            buf = getLocaleText("Итого");
            break;
        default:
            throw Exception("createXMLAgentStat: unexpected statType %d", params.statType);
            break;
    }
    NewTextChild(variablesNode, "stat_type_caption", buf);
}

struct TAgentStatCombo : public TOrderStatItem
{
    typedef std::pair<TAgentStatKey, TAgentStatRow> Tdata;
    Tdata data;
    TStatParams params;
    TAgentStatCombo(const Tdata &aData, const TStatParams &aParams)
        : data(aData), params(aParams) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TAgentStatCombo::add_header(ostringstream &buf) const
{
    if (params.statType == statAgentFull or params.statType == statAgentShort)
    {
        buf << "Рейс" << delim;
        buf << "Дата" << delim;
    }
    if (params.statType == statAgentFull)
        buf << "Стойка" << delim;
    if (params.statType == statAgentFull or params.statType == statAgentTotal)
        buf << "Агент" << delim;
    if (params.statType == statAgentTotal)
    {
        buf << "Пас." << delim;
        buf << "Сквоз." << delim;
        buf << "Баг." << delim;
        buf << "Р/к";
    }
    else
    {
        buf << "Пас. (+)" << delim;
        buf << "Пас. (-)" << delim;
        buf << "Сквоз.(+)" << delim;
        buf << "Сквоз.(-)" << delim;
        buf << "Баг. (+)" << delim;
        buf << "Баг. (-)" << delim;
        buf << "Р/к (+)" << delim;
        buf << "Р/к (-)" << delim;
        buf << "Сек./пас.";
    }
    buf << endl;
}

void TAgentStatCombo::add_data(ostringstream &buf) const
{
    TDateTime scd_out_local = data.first.scd_out_local;
    if (scd_out_local == NoExists)
    try { scd_out_local = UTCToClient(data.first.scd_out, AirpTZRegion(data.first.airp)); }
    catch(AstraLocale::UserException &E)
    {
        AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN",
            LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
        return;
    };
    ostringstream oss;
    if (params.statType == statAgentFull or params.statType == statAgentShort)
    {
        oss << data.first.airline_view
            << setw(3) << setfill('0') << data.first.flt_no
            << data.first.suffix_view << " "
            << data.first.airp_view;
        buf << oss.str() << delim; // Код а/к
        buf << DateTimeToStr( scd_out_local, "dd.mm.yy") << delim; // Дата
    }
    if (params.statType == statAgentFull)
        buf << data.first.desk << delim; // Пульт
    if (params.statType == statAgentFull or params.statType == statAgentTotal)
        buf << data.first.user_descr << delim; // Пользователь
    if (params.statType == statAgentTotal)
    {
        buf << data.second.dpax_amount.inc << delim; // Кол-во пасс.
        buf << data.second.dtckin_amount.inc << delim; // Кол-во сквоз.
        buf <<  IntToString(data.second.dbag_amount.inc) + "/" +
                IntToString(data.second.dbag_weight.inc) << delim; // Багаж (мест/вес)
        buf << data.second.drk_weight.inc << endl; // Р/кладь (вес)
    }
    else
    {
        buf << data.second.dpax_amount.inc << delim; // Кол-во пасс. (+)
        buf << -data.second.dpax_amount.dec << delim; // Кол-во пасс. (-)
        buf << data.second.dtckin_amount.inc << delim; // Кол-во сквоз. (+)
        buf << -data.second.dtckin_amount.dec << delim; // Кол-во сквоз. (-)
        buf <<  IntToString(data.second.dbag_amount.inc) + "/" +
                IntToString(data.second.dbag_weight.inc) << delim; // Багаж (мест/вес) (+)
        buf <<  IntToString(-data.second.dbag_amount.dec) + "/" +
                IntToString(-data.second.dbag_weight.dec) << delim; // Багаж (мест/вес) (-)
        buf << data.second.drk_weight.inc << delim; // Р/кладь (вес) (+)
        buf << -data.second.drk_weight.dec << delim; // Р/кладь (вес) (-)
        oss.str("");
        if (data.second.processed_pax!=0)
            oss << fixed << setprecision(2) << data.second.time / data.second.processed_pax;
        else
            oss << fixed << setprecision(2) << 0.0;
        buf << oss.str() << endl; // Среднее время, затраченное на пассажира
    }
}

void RunAgentStatFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline)
{
    TAgentStat AgentStat;
    TAgentStatRow AgentStatTotal;
    RunAgentStat(params, AgentStat, AgentStatTotal, prn_airline, true);
    for(TAgentStat::const_iterator im = AgentStat.begin(); im != AgentStat.end(); ++im)
        writer.insert(TAgentStatCombo(*im, params));
}

int STAT::agent_stat_delta(int argc,char **argv)
{
    try {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "begin "
            "   delete from agent_stat; "
            "end; ";
        Qry.Execute();
        Qry.SQLText =
            "select "
            "  point_id, "
            "  user_id, "
            "  desk, "
            "  ondate, "
            "  pax_time, "
            "  pax_amount, "
            "  asp.dpax_amount.inc pax_am_inc, "
            "  asp.dpax_amount.dec pax_am_dec, "
            "  asp.dtckin_amount.inc tckin_am_inc, "
            "  asp.dtckin_amount.dec tckin_am_dec, "
            "  asp.dbag_amount.inc bag_am_inc, "
            "  asp.dbag_amount.dec bag_am_dec, "
            "  asp.dbag_weight.inc bag_we_inc, "
            "  asp.dbag_weight.dec bag_we_dec, "
            "  asp.drk_amount.inc rk_am_inc, "
            "  asp.drk_amount.dec rk_am_dec, "
            "  asp.drk_weight.inc rk_we_inc, "
            "  asp.drk_weight.dec rk_we_dec "
            "from agent_stat_params asp";
        Qry.Execute();
        for(; not Qry.Eof; Qry.Next()) {
            agent_stat_delta(
                    Qry.FieldAsInteger("point_id"),
                    Qry.FieldAsInteger("user_id"),
                    Qry.FieldAsString("desk"),
                    Qry.FieldAsDateTime("ondate"),
                    Qry.FieldAsInteger("pax_time"),
                    Qry.FieldAsInteger("pax_amount"),
                    agent_stat_t(Qry.FieldAsInteger("pax_am_inc"), Qry.FieldAsInteger("pax_am_dec")),
                    agent_stat_t(Qry.FieldAsInteger("tckin_am_inc"), Qry.FieldAsInteger("tckin_am_dec")),
                    agent_stat_t(Qry.FieldAsInteger("bag_am_inc"), Qry.FieldAsInteger("bag_am_dec")),
                    agent_stat_t(Qry.FieldAsInteger("bag_we_inc"), Qry.FieldAsInteger("bag_we_dec")),
                    agent_stat_t(Qry.FieldAsInteger("rk_am_inc"), Qry.FieldAsInteger("rk_am_dec")),
                    agent_stat_t(Qry.FieldAsInteger("rk_we_inc"), Qry.FieldAsInteger("rk_we_dec"))
                    );
        }
        Qry.Clear();
    } catch(Exception &E) {
        cout << "Error: " << E.what() << endl;
        return 1;
    }
    return 0;
}

void STAT::agent_stat_delta(
        int point_id,
        int user_id,
        const std::string &desk,
        TDateTime ondate,
        int pax_time,
        int pax_amount,
        agent_stat_t dpax_amount,
        agent_stat_t dtckin_amount,
        agent_stat_t dbag_amount,
        agent_stat_t dbag_weight,
        agent_stat_t drk_amount,
        agent_stat_t drk_weight
        )
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "begin "
        "  update agent_stat ags set "
        "    pax_time = pax_time + :pax_time, "
        "    pax_amount = pax_amount + :pax_amount, "
        "    ags.dpax_amount.inc = ags.dpax_amount.inc + :pax_am_inc, "
        "    ags.dpax_amount.dec = ags.dpax_amount.dec + :pax_am_dec, "
        "    ags.dtckin_amount.inc = ags.dtckin_amount.inc + :tckin_am_inc, "
        "    ags.dtckin_amount.dec = ags.dtckin_amount.dec + :tckin_am_dec, "
        "    ags.dbag_amount.inc = ags.dbag_amount.inc + :bag_am_inc, "
        "    ags.dbag_amount.dec = ags.dbag_amount.dec + :bag_am_dec, "
        "    ags.dbag_weight.inc = ags.dbag_weight.inc + :bag_we_inc, "
        "    ags.dbag_weight.dec = ags.dbag_weight.dec + :bag_we_dec, "
        "    ags.drk_amount.inc = ags.drk_amount.inc + :rk_am_inc, "
        "    ags.drk_amount.dec = ags.drk_amount.dec + :rk_am_dec, "
        "    ags.drk_weight.inc = ags.drk_weight.inc + :rk_we_inc, "
        "    ags.drk_weight.dec = ags.drk_weight.dec + :rk_we_dec "
        "  where "
        "    point_id = :point_id and "
        "    user_id = :user_id and "
        "    desk = :desk and "
        "    ondate = TRUNC(:ondate); "
        "  if sql%notfound then "
        "    insert into agent_stat( "
        "        point_id, "
        "        user_id, "
        "        desk, "
        "        ondate, "
        "        pax_time, "
        "        pax_amount, "
        "        dpax_amount, "
        "        dtckin_amount, "
        "        dbag_amount, "
        "        dbag_weight, "
        "        drk_amount, "
        "        drk_weight "
        "    ) values ( "
        "        :point_id, "
        "        :user_id, "
        "        :desk, "
        "        TRUNC(:ondate), "
        "        :pax_time, "
        "        :pax_amount, "
        "        agent_stat_t(:pax_am_inc, :pax_am_dec), "
        "        agent_stat_t(:tckin_am_inc, :tckin_am_dec), "
        "        agent_stat_t(:bag_am_inc, :bag_am_dec), "
        "        agent_stat_t(:bag_we_inc, :bag_we_dec), "
        "        agent_stat_t(:rk_am_inc, :rk_am_dec), "
        "        agent_stat_t(:rk_we_inc, :rk_we_dec) "
        "    ); "
        "  end if; "
        "end; ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("user_id", otInteger, user_id);
    Qry.CreateVariable("desk", otString, desk);
    Qry.CreateVariable("ondate", otDate, ondate);
    Qry.CreateVariable("pax_time", otInteger, pax_time);
    Qry.CreateVariable("pax_amount", otInteger, pax_amount);
    Qry.CreateVariable("pax_am_inc", otInteger, dpax_amount.inc);
    Qry.CreateVariable("pax_am_dec", otInteger, dpax_amount.dec);
    Qry.CreateVariable("tckin_am_inc", otInteger, dtckin_amount.inc);
    Qry.CreateVariable("tckin_am_dec", otInteger, dtckin_amount.dec);
    Qry.CreateVariable("bag_am_inc", otInteger, dbag_amount.inc);
    Qry.CreateVariable("bag_am_dec", otInteger, dbag_amount.dec);
    Qry.CreateVariable("bag_we_inc", otInteger, dbag_weight.inc);
    Qry.CreateVariable("bag_we_dec", otInteger, dbag_weight.dec);
    Qry.CreateVariable("rk_am_inc", otInteger, drk_amount.inc);
    Qry.CreateVariable("rk_am_dec", otInteger, drk_amount.dec);
    Qry.CreateVariable("rk_we_inc", otInteger, drk_weight.inc);
    Qry.CreateVariable("rk_we_dec", otInteger, drk_weight.dec);
    Qry.Execute();
}

