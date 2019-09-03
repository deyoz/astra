#include "stat_general.h"
#include "report_common.h"
#include "astra_date_time.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace ASTRA::date_time;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;

const string AIRP_PERIODS =
"(SELECT DECODE(SIGN(period_first_date-:FirstDate),1,period_first_date,:FirstDate) AS period_first_date, \n"
"         DECODE(SIGN(period_last_date- :LastDate),-1,period_last_date, :LastDate) AS period_last_date \n"
"  FROM \n"
"   (SELECT distinct change_point AS period_first_date, \n"
"           report.get_airp_period_last_date(:ap,change_point) AS period_last_date \n"
"    FROM pacts, \n"
"     (SELECT first_date AS change_point \n"
"      FROM pacts \n"
"      WHERE airp=:ap \n"
"      UNION \n"
"      SELECT last_date   \n"
"      FROM pacts \n"
"      WHERE airp=:ap AND last_date IS NOT NULL) a \n"
"    WHERE airp=:ap AND  \n"
"          change_point>=first_date AND  \n"
"          (last_date IS NULL OR change_point<last_date) AND \n"
"          airline IS NULL) periods \n"
"  WHERE (period_last_date IS NULL OR period_last_date>:FirstDate) AND period_first_date<:LastDate \n"
" ) periods \n";
const string AIRLINE_PERIODS =
"(SELECT DECODE(SIGN(period_first_date-:FirstDate),1,period_first_date,:FirstDate) AS period_first_date, \n"
"         DECODE(SIGN(period_last_date- :LastDate),-1,period_last_date, :LastDate) AS period_last_date \n"
"  FROM \n"
"   (SELECT distinct change_point AS period_first_date, \n"
"           report.get_airline_period_last_date(:ak,change_point) AS period_last_date \n"
"    FROM pacts,  \n"
"     (SELECT first_date AS change_point \n"
"      FROM pacts \n"
"      WHERE airline=:ak \n"
"      UNION \n"
"      SELECT last_date   \n"
"      FROM pacts \n"
"      WHERE airline=:ak AND last_date IS NOT NULL) a \n"
"    WHERE airline=:ak AND  \n"
"          change_point>=first_date AND  \n"
"          (last_date IS NULL OR change_point<last_date)) periods \n"
"  WHERE (period_last_date IS NULL OR period_last_date>:FirstDate) AND period_first_date<:LastDate \n"
" ) periods \n";

const string AIRLINE_LIST =
" (SELECT airline  \n"
"  FROM pacts  \n"
"  WHERE airp=:ap AND \n"
"        first_date<=periods.period_first_date AND  \n"
"        (last_date IS NULL OR periods.period_first_date<last_date) AND \n"
"        airline IS NOT NULL) \n";

const string AIRP_LIST =
" (SELECT airp  \n"
"  FROM pacts  \n"
"  WHERE airline=:ak AND \n"
"        first_date<=periods.period_first_date AND  \n"
"        (last_date IS NULL OR periods.period_first_date<last_date) ) \n";


string GetStatSQLText(const TStatParams &params, int pass)
{
    static const string sum_pax_by_client_type =
        " SUM(DECODE(client_type, :web, adult + child + baby, 0)) web, \n"
        " SUM(DECODE(client_type, :web, term_bag, 0)) web_bag, \n"
        " SUM(DECODE(client_type, :web, term_bp, 0)) web_bp, \n"

        " SUM(DECODE(client_type, :kiosk, adult + child + baby, 0)) kiosk, \n"
        " SUM(DECODE(client_type, :kiosk, term_bag, 0)) kiosk_bag, \n"
        " SUM(DECODE(client_type, :kiosk, term_bp, 0)) kiosk_bp, \n"

        " SUM(DECODE(client_type, :mobile, adult + child + baby, 0)) mobile, \n"
        " SUM(DECODE(client_type, :mobile, term_bp, 0)) mobile_bp, \n"
        " SUM(DECODE(client_type, :mobile, term_bag, 0)) mobile_bag \n";

    static const string sum_pax_by_class =
        " sum(f) f, \n"
        " sum(c) c, \n"
        " sum(y) y, \n";

  ostringstream sql;
  sql << "SELECT \n";

  if (params.statType==statTrferFull)
  {
    sql << " points.airp, \n"
           " points.airline, \n"
           " points.flt_no, \n"
           " points.scd_out, \n"
           " trfer_stat.point_id, \n"
           " trfer_stat.trfer_route places, \n"
           " adult + child + baby pax_amount, \n"
           " adult, \n"
           " child, \n"
           " baby, \n"
           " unchecked rk_weight, \n"
           " pcs bag_amount, \n"
           " weight bag_weight, \n"
           " excess_wt, \n"
           " NVL(excess_pc,0) AS excess_pc \n";
  };
  if (params.statType==statFull)
  {
    sql << " points.airp, \n"
           " points.airline, \n"
           " points.flt_no, \n"
           " points.scd_out, \n";
    if (pass!=0)
      sql << " stat.part_key, \n";
    sql << " stat.point_id, \n"
           " SUM(adult + child + baby) pax_amount, \n" <<
           sum_pax_by_class <<
           sum_pax_by_client_type <<
           ", SUM(adult) adult, \n"
           " SUM(child) child, \n"
           " SUM(baby) baby, \n"
           " SUM(unchecked) rk_weight, \n"
           " SUM(pcs) bag_amount, \n"
           " SUM(weight) bag_weight, \n"
           " SUM(excess_wt) AS excess_wt, \n"
           " SUM(NVL(excess_pc,0)) AS excess_pc \n";
  };
  if (params.statType==statShort)
  {
    if(params.airp_column_first)
      sql << " points.airp, \n";
    else
      sql << " points.airline, \n";
    sql << " COUNT(distinct stat.point_id) flt_amount, \n"
           " SUM(adult + child + baby) pax_amount, \n" <<
           sum_pax_by_class <<
           sum_pax_by_client_type;
  };
  if (params.statType==statDetail)
  {
    sql << " points.airp, \n"
           " points.airline, \n"
           " COUNT(distinct stat.point_id) flt_amount, \n"
           " SUM(adult + child + baby) pax_amount, \n" <<
           sum_pax_by_class <<
           sum_pax_by_client_type;
  };
  sql << "FROM \n";
  if (pass==0)
    sql << " points, \n";
  else
    sql << " arx_points points, \n";
  if (params.statType==statTrferFull)
  {
    if (pass==0)
      sql << " trfer_stat \n";
    else
      sql << " arx_trfer_stat trfer_stat \n";
  };
  if (params.statType==statFull || params.statType==statShort || params.statType==statDetail)
  {
    if (pass==0)
      sql << " stat \n";
    else
      sql << " arx_stat stat \n";
  };

  if (params.seance==seanceAirport ||
      params.seance==seanceAirline)
  {
    if (pass!=2)
    {
      if(params.seance==seanceAirport)
        sql << "," << AIRP_PERIODS;
      if(params.seance==seanceAirline)
        sql << "," << AIRLINE_PERIODS;
    }
    else
    {
      sql << ", (SELECT arx_points.part_key, arx_points.point_id \n"
             "   FROM move_arx_ext, arx_points \n";
      if(params.seance==seanceAirport)
        sql << "," << AIRP_PERIODS;
      if(params.seance==seanceAirline)
        sql << "," << AIRLINE_PERIODS;
      sql << "   WHERE move_arx_ext.part_key >= periods.period_last_date + :arx_trip_date_range AND \n"
             "         move_arx_ext.part_key <= periods.period_last_date + move_arx_ext.date_range AND \n"
             "         move_arx_ext.part_key = arx_points.part_key AND move_arx_ext.move_id = arx_points.move_id AND \n"
             "         arx_points.scd_out >= periods.period_first_date AND arx_points.scd_out < periods.period_last_date AND \n";
      if(params.seance==seanceAirport)
        sql << "         arx_points.airline NOT IN \n" << AIRLINE_LIST;
      if(params.seance==seanceAirline)
        sql << "         arx_points.airp IN \n" << AIRP_LIST;
      sql << "  ) arx_ext \n";
    };
  }
  else
  {
    if (pass==2)
      sql << ", (SELECT part_key, move_id FROM move_arx_ext \n"
             "   WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
  };

  sql << "WHERE \n";
  if(params.flt_no != NoExists)
    sql << " points.flt_no = :flt_no AND \n";
  if (params.statType==statTrferFull)
  {
    if (pass!=0)
      sql << " points.part_key = trfer_stat.part_key AND \n";
    sql << " points.point_id = trfer_stat.point_id AND \n";
  };
  if (params.statType==statFull || params.statType==statShort || params.statType==statDetail)
  {
    if (pass!=0)
      sql << " points.part_key = stat.part_key AND \n";
    sql << " points.point_id = stat.point_id AND \n";
  };

  if (params.seance==seanceAirport ||
      params.seance==seanceAirline)
  {
    if (pass!=2)
    {
      if (pass!=0)
        sql << " points.part_key >= periods.period_first_date AND points.part_key < periods.period_last_date + :arx_trip_date_range AND \n";
      sql << " points.scd_out >= periods.period_first_date AND points.scd_out < periods.period_last_date AND \n";
    }
    else
      sql << " points.part_key=arx_ext.part_key AND points.point_id=arx_ext.point_id AND \n";
  }
  else
  {
    if (pass==1)
      sql << " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
    if (pass==2)
      sql << " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
    sql << " points.scd_out >= :FirstDate AND points.scd_out < :LastDate AND \n";
  };

  sql << " points.pr_del>=0 \n";
  if (params.seance==seanceAirport)
  {
    sql << " AND points.airp = :ap \n";
  }
  else
  {
    if (!params.airps.elems().empty()) {
      if (params.airps.elems_permit())
        sql << " AND points.airp IN " << GetSQLEnum(params.airps.elems()) << "\n";
      else
        sql << " AND points.airp NOT IN " << GetSQLEnum(params.airps.elems()) << "\n";
    };
  };

  if (params.seance==seanceAirline)
  {
    sql << " AND points.airline = :ak \n";
  }
  else
  {
    if (!params.airlines.elems().empty()) {
      if (params.airlines.elems_permit())
        sql << " AND points.airline IN " << GetSQLEnum(params.airlines.elems()) << "\n";
      else
        sql << " AND points.airline NOT IN " << GetSQLEnum(params.airlines.elems()) << "\n";
    }
  };

  if (pass!=2)
  {
    if(params.seance==seanceAirport)
      sql << " AND points.airline NOT IN \n" << AIRLINE_LIST;
    if(params.seance==seanceAirline)
      sql << " AND points.airp IN \n" << AIRP_LIST;
  };

  if (params.statType==statFull)
  {
    sql << "GROUP BY \n"
           " points.airp, \n"
           " points.airline, \n"
           " points.flt_no, \n"
           " points.scd_out, \n";
    if (pass!=0)
      sql << " stat.part_key, \n";
    sql << " stat.point_id \n";
  };
  if (params.statType==statShort)
  {
    sql << "GROUP BY \n";
    if(params.airp_column_first)
      sql << " points.airp \n";
    else
      sql << " points.airline \n";
  };
  if (params.statType==statDetail)
  {
    sql << "GROUP BY \n"
           " points.airp, \n"
           " points.airline \n";
  };
  return sql.str();
}

bool TDetailStatRow::operator == (const TDetailStatRow &item) const
{
    return flt_amount == item.flt_amount &&
        pax_amount == item.pax_amount &&
        i_stat == item.i_stat &&
        f == item.f &&
        c == item.c &&
        y == item.y &&
        flts.size() == item.flts.size();
};

void TDetailStatRow::operator += (const TDetailStatRow &item)
{
    flt_amount += item.flt_amount;
    pax_amount += item.pax_amount;
    i_stat += item.i_stat;
    f += item.f;
    c += item.c;
    y += item.y;
    flts.insert(item.flts.begin(),item.flts.end());
};


bool TDetailStatKey::operator == (const TDetailStatKey &item) const
{
    return pact_descr == item.pact_descr &&
        col1 == item.col1 &&
        col2 == item.col2;
};

bool TDetailCmp::operator() (const TDetailStatKey &lr, const TDetailStatKey &rr) const
{
    if(lr.col1 == rr.col1)
        if(lr.col2 == rr.col2)
            return lr.pact_descr < rr.pact_descr;
        else
            return lr.col2 < rr.col2;
    else
        return lr.col1 < rr.col1;
};

bool TInetStat::operator == (const TInetStat &item) const
{
    return
        web == item.web &&
        kiosk == item.kiosk &&
        mobile == item.mobile &&
        web_bp == item.web_bp &&
        web_bag == item.web_bag &&
        kiosk_bp == item.kiosk_bp &&
        kiosk_bag == item.kiosk_bag &&
        mobile_bp == item.mobile_bp &&
        mobile_bag == item.mobile_bag;
}

void TInetStat::operator += (const TInetStat &item)
{
    web += item.web;
    kiosk += item.kiosk;
    mobile += item.mobile;
    web_bp += item.web_bp;
    web_bag += item.web_bag;
    kiosk_bp += item.kiosk_bp;
    kiosk_bag += item.kiosk_bag;
    mobile_bp += item.mobile_bp;
    mobile_bag += item.mobile_bag;
}

void TInetStat::toXML(xmlNodePtr headerNode, xmlNodePtr rowNode)
{
    //Web
    xmlNodePtr colNode = NewTextChild(headerNode, "col", getLocaleText("Web"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", web);

    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", web_bag);

    colNode = NewTextChild(headerNode, "col", getLocaleText("ПТ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", web_bp);

    // Киоски
    colNode = NewTextChild(headerNode, "col", getLocaleText("Киоски"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", kiosk);

    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", kiosk_bag);

    colNode = NewTextChild(headerNode, "col", getLocaleText("ПТ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", kiosk_bp);

    // Моб.
    colNode = NewTextChild(headerNode, "col", getLocaleText("Моб."));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", mobile);

    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", mobile_bag);

    colNode = NewTextChild(headerNode, "col", getLocaleText("ПТ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", mobile_bp);
}

bool TFullStatRow::operator == (const TFullStatRow &item) const
{
    return pax_amount == item.pax_amount &&
        i_stat == item.i_stat &&
        adult == item.adult &&
        child == item.child &&
        baby == item.baby &&
        rk_weight == item.rk_weight &&
        bag_amount == item.bag_amount &&
        bag_weight == item.bag_weight &&
        excess_wt == item.excess_wt &&
        excess_pc == item.excess_pc;
};

void TFullStatRow::operator += (const TFullStatRow &item)
{
    pax_amount += item.pax_amount;
    i_stat += item.i_stat;
    adult += item.adult;
    child += item.child;
    baby += item.baby;
    rk_weight += item.rk_weight;
    bag_amount += item.bag_amount;
    bag_weight += item.bag_weight;
    excess_wt += item.excess_wt;
    excess_pc += item.excess_pc;
};


bool TFullStatKey::operator == (const TFullStatKey &item) const
{
    return col1 == item.col1 &&
        col2 == item.col2 &&
        airp == item.airp &&
        flt_no == item.flt_no &&
        scd_out == item.scd_out &&
        point_id == item.point_id &&
        places.get() == item.places.get();
};

bool TFullCmp::operator() (const TFullStatKey &lr, const TFullStatKey &rr) const
{
    if(lr.col1 == rr.col1)
        if(lr.col2 == rr.col2)
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
            return lr.col2 < rr.col2;
    else
        return lr.col1 < rr.col1;
};
void GetDetailStat(const TStatParams &params, TQuery &Qry,
                   TDetailStat &DetailStat, TDetailStatRow &DetailStatTotal,
                   TPrintAirline &airline, string pact_descr = "", bool full = false)
{
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    TDetailStatRow row;
    row.flt_amount = Qry.FieldAsInteger("flt_amount");
    row.pax_amount = Qry.FieldAsInteger("pax_amount");

    row.i_stat.web = Qry.FieldAsInteger("web");
    row.i_stat.web_bag = Qry.FieldAsInteger("web_bag");
    row.i_stat.web_bp = Qry.FieldAsInteger("web_bp");

    row.i_stat.kiosk = Qry.FieldAsInteger("kiosk");
    row.i_stat.kiosk_bag = Qry.FieldAsInteger("kiosk_bag");
    row.i_stat.kiosk_bp = Qry.FieldAsInteger("kiosk_bp");

    row.i_stat.mobile = Qry.FieldAsInteger("mobile");
    row.i_stat.mobile_bag = Qry.FieldAsInteger("mobile_bag");
    row.i_stat.mobile_bp = Qry.FieldAsInteger("mobile_bp");
    row.f = Qry.FieldAsInteger("f");
    row.c = Qry.FieldAsInteger("c");
    row.y = Qry.FieldAsInteger("y");

    if (!params.skip_rows)
    {
      TDetailStatKey key;
      key.pact_descr = pact_descr;
      if(params.airp_column_first) {
          key.col1 = ElemIdToCodeNative(etAirp, Qry.FieldAsString("airp"));
          if (params.statType==statDetail)
          {
            key.col2 = ElemIdToCodeNative(etAirline, Qry.FieldAsString("airline"));
            airline.check(key.col2);
          };
      } else {
          key.col1 = ElemIdToCodeNative(etAirline, Qry.FieldAsString("airline"));
          if (params.statType==statDetail)
          {
            key.col2 = ElemIdToCodeNative(etAirp, Qry.FieldAsString("airp"));
          };
          airline.check(key.col1);
      }

      AddStatRow(key, row, DetailStat, full);
    }
    else
    {
      DetailStatTotal+=row;
    };
  }
};

void GetFullStat(const TStatParams &params, TQuery &Qry,
                 TFullStat &FullStat, TFullStatRow &FullStatTotal,
                 TPrintAirline &airline, bool full = false)
{
  Qry.Execute();
  if(!Qry.Eof) {
      int col_point_id = Qry.FieldIndex("point_id");
      int col_airp = Qry.FieldIndex("airp");
      int col_airline = Qry.FieldIndex("airline");
      int col_pax_amount = Qry.FieldIndex("pax_amount");

      int col_web = -1;
      int col_web_bag = -1;
      int col_web_bp = -1;

      int col_kiosk = -1;
      int col_kiosk_bag = -1;
      int col_kiosk_bp = -1;

      int col_mobile = -1;
      int col_mobile_bag = -1;
      int col_mobile_bp = -1;

      if (params.statType==statFull)
      {
        col_web = Qry.FieldIndex("web");
        col_web_bag = Qry.FieldIndex("web_bag");
        col_web_bp = Qry.FieldIndex("web_bp");

        col_kiosk = Qry.FieldIndex("kiosk");
        col_kiosk_bag = Qry.FieldIndex("kiosk_bag");
        col_kiosk_bp = Qry.FieldIndex("kiosk_bp");

        col_mobile = Qry.FieldIndex("mobile");
        col_mobile_bag = Qry.FieldIndex("mobile_bag");
        col_mobile_bp = Qry.FieldIndex("mobile_bp");
      };
      int col_adult = Qry.FieldIndex("adult");
      int col_child = Qry.FieldIndex("child");
      int col_baby = Qry.FieldIndex("baby");
      int col_rk_weight = Qry.FieldIndex("rk_weight");
      int col_bag_amount = Qry.FieldIndex("bag_amount");
      int col_bag_weight = Qry.FieldIndex("bag_weight");
      int col_excess_wt = Qry.FieldIndex("excess_wt");
      int col_excess_pc = Qry.FieldIndex("excess_pc");
      int col_flt_no = Qry.FieldIndex("flt_no");
      int col_scd_out = Qry.FieldIndex("scd_out");
      int col_places = Qry.GetFieldIndex("places");
      int col_part_key = Qry.GetFieldIndex("part_key");
      for(; !Qry.Eof; Qry.Next())
      {
        TFullStatRow row;
        row.pax_amount = Qry.FieldAsInteger(col_pax_amount);
        if (params.statType==statFull)
        {
          row.i_stat.web = Qry.FieldAsInteger(col_web);
          row.i_stat.web_bag = Qry.FieldAsInteger(col_web_bag);
          row.i_stat.web_bp = Qry.FieldAsInteger(col_web_bp);

          row.i_stat.kiosk = Qry.FieldAsInteger(col_kiosk);
          row.i_stat.kiosk_bag = Qry.FieldAsInteger(col_kiosk_bag);
          row.i_stat.kiosk_bp = Qry.FieldAsInteger(col_kiosk_bp);

          row.i_stat.mobile = Qry.FieldAsInteger(col_mobile);
          row.i_stat.mobile_bag = Qry.FieldAsInteger(col_mobile_bag);
          row.i_stat.mobile_bp = Qry.FieldAsInteger(col_mobile_bp);
        };
        row.adult = Qry.FieldAsInteger(col_adult);
        row.child = Qry.FieldAsInteger(col_child);
        row.baby = Qry.FieldAsInteger(col_baby);
        row.rk_weight = Qry.FieldAsInteger(col_rk_weight);
        row.bag_amount = Qry.FieldAsInteger(col_bag_amount);
        row.bag_weight = Qry.FieldAsInteger(col_bag_weight);
        row.excess_wt = Qry.FieldAsInteger(col_excess_wt);
        row.excess_pc = Qry.FieldAsInteger(col_excess_pc);
        if (!params.skip_rows)
        {
          TFullStatKey key;
          key.airp = Qry.FieldAsString(col_airp);
          if(params.airp_column_first) {
              key.col1 = ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp));
              key.col2 = ElemIdToCodeNative(etAirline, Qry.FieldAsString(col_airline));
              airline.check(key.col2);
          } else {
              key.col1 = ElemIdToCodeNative(etAirline, Qry.FieldAsString(col_airline));
              key.col2 = ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp));
              airline.check(key.col1);
          }
          key.flt_no = Qry.FieldAsInteger(col_flt_no);
          key.scd_out = Qry.FieldAsDateTime(col_scd_out);
          key.point_id = Qry.FieldAsInteger(col_point_id);
          if (params.statType==statTrferFull)
            key.places.set(Qry.FieldAsString(col_places), true);
          else
            key.places.set(GetRouteAfterStr( col_part_key>=0?Qry.FieldAsDateTime(col_part_key):NoExists,
                                             Qry.FieldAsInteger(col_point_id),
                                             trtNotCurrent,
                                             trtNotCancelled),
                           false);
          AddStatRow(key, row, FullStat, full);
        }
        else
        {
          FullStatTotal+=row;
        };
      }
  }
};

struct TPact {
    string descr, airline, airp;
    vector<string> airlines; // Список ак, который исключается из ап договора.
    TDateTime first_date, last_date;
    TPact(string vdescr, string vairline, string vairp, TDateTime vfirst_date, TDateTime vlast_date):
        descr(vdescr),
        airline(vairline),
        airp(vairp),
        first_date(vfirst_date),
        last_date(vlast_date)
    {};
    void dump();
};

void TPact::dump()
{
    ProgTrace(TRACE5, "------TPact-------");
    ProgTrace(TRACE5, "descr: %s", descr.c_str());
    ProgTrace(TRACE5, "airline: %s", airline.c_str());
    ProgTrace(TRACE5, "airp: %s", airp.c_str());
    string buf;
    for(vector<string>::iterator iv = airlines.begin(); iv != airlines.end(); iv++) {
        if(not buf.empty())
            buf += ", ";
        buf += *iv;
    }
    ProgTrace(TRACE5, "airlines: %s", buf.c_str());
    ProgTrace(TRACE5, "first_date: %s", DateTimeToStr(first_date, "ddmmyy").c_str());
    ProgTrace(TRACE5, "last_date: %s", DateTimeToStr(last_date, "ddmmyy").c_str());
}
void correct_airp_pacts(vector<TPact> &airp_pacts, TPact &airline_pact)
{
    if(airp_pacts.empty())
        return;
    vector<TPact> added_pacts;
    for(vector<TPact>::iterator iv = airp_pacts.begin(); iv != airp_pacts.end(); iv++) {
        if(
                airline_pact.first_date >= iv->first_date and
                airline_pact.first_date < iv->last_date and
                airline_pact.last_date >= iv->last_date
          ) {// Левый край ак договора попал, разбиваем ап договор на 2 части и добавляем ак в список исключенных для этого ап договора
            TPact new_pact = *iv;
            new_pact.first_date = airline_pact.first_date;
            iv->last_date = airline_pact.first_date;
            new_pact.airlines.push_back(airline_pact.airline);
            added_pacts.push_back(new_pact);
        } if(
                airline_pact.last_date >= iv->first_date and
                airline_pact.last_date < iv->last_date and
                airline_pact.first_date < iv->first_date
          ) { // правый край ак договора попал
            TPact new_pact = *iv;
            new_pact.last_date = airline_pact.last_date;
            iv->first_date = airline_pact.last_date;
            new_pact.airlines.push_back(airline_pact.airline);
            added_pacts.push_back(new_pact);
        } if(
                airline_pact.first_date >= iv->first_date and
                airline_pact.last_date < iv->last_date
          ) { // ак договор целиком внутри периода ап договора, разбиваем ап договор на 3 части.
            TPact new_pact = *iv;
            TPact new_pact1 = *iv;
            new_pact.first_date = airline_pact.first_date;
            new_pact.last_date = airline_pact.last_date;
            new_pact1.first_date = airline_pact.last_date;
            iv->last_date = airline_pact.first_date;
            new_pact.airlines.push_back(airline_pact.airline);
            added_pacts.push_back(new_pact);
            added_pacts.push_back(new_pact1);
        }
        if(
                airline_pact.first_date < iv->first_date and
                airline_pact.last_date >= iv->last_date
          ) { // ап договор целиком внутри ак договора, просто добавляем в список исключаемых ак этого дог. текущую.
            iv->airlines.push_back(airline_pact.airline);
        }
    }
    airp_pacts.insert(airp_pacts.end(), added_pacts.begin(), added_pacts.end());
    while(true) {
        vector<TPact>::iterator iv = airp_pacts.begin();
        for(; iv != airp_pacts.end(); iv++) {
            if(iv->descr.empty() or iv->first_date == iv->last_date)
                break;
        }
        if(iv == airp_pacts.end())
            break;
        airp_pacts.erase(iv);
    }
}

void setClientTypeCaps(xmlNodePtr variablesNode)
{
    NewTextChild(variablesNode, "kiosks", getLocaleText("CAP.KIOSKS"));
    NewTextChild(variablesNode, "pax", getLocaleText("CAP.PAX"));
    NewTextChild(variablesNode, "mob", getLocaleText("CAP.MOB"));
    NewTextChild(variablesNode, "mobile_devices", getLocaleText("CAP.MOBILE_DEVICES"));
}

void createXMLDetailStat(const TStatParams &params, bool pr_pact,
                         const TDetailStat &DetailStat, const TDetailStatRow &DetailStatTotal,
                         const TPrintAirline &airline, xmlNodePtr resNode)
{
    if(DetailStat.empty() && DetailStatTotal==TDetailStatRow())
      throw AstraLocale::UserException("MSG.NOT_DATA");

    NewTextChild(resNode, "airline", airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    TDetailStatRow total;
    bool showTotal=true;
    if (!params.skip_rows)
    {
      int rows = 0;
      for(TDetailStat::const_iterator si = DetailStat.begin(); si != DetailStat.end(); ++si, rows++)
      {
          if(rows >= MAX_STAT_ROWS()) {
              throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
              /*AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH",
                      LParams() << LParam("num", MAX_STAT_ROWS()));
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
              break;*/
          }

          rowNode = NewTextChild(rowsNode, "row");
          if(params.statType != statPactShort)
              NewTextChild(rowNode, "col", si->first.col1);
          if (params.statType==statDetail)
              NewTextChild(rowNode, "col", si->first.col2);

          if(params.statType != statPactShort)
              NewTextChild(rowNode, "col", (int)(pr_pact?si->second.flts.size():si->second.flt_amount));
          if(params.statType == statPactShort)
              NewTextChild(rowNode, "col", si->first.pact_descr);
          NewTextChild(rowNode, "col", si->second.pax_amount);
          if(params.statType != statPactShort) {
              NewTextChild(rowNode, "col", si->second.f);
              NewTextChild(rowNode, "col", si->second.c);
              NewTextChild(rowNode, "col", si->second.y);

              NewTextChild(rowNode, "col", si->second.i_stat.web);
              NewTextChild(rowNode, "col", si->second.i_stat.web_bag);
              NewTextChild(rowNode, "col", si->second.i_stat.web_bp);

              NewTextChild(rowNode, "col", si->second.i_stat.kiosk);
              NewTextChild(rowNode, "col", si->second.i_stat.kiosk_bag);
              NewTextChild(rowNode, "col", si->second.i_stat.kiosk_bp);

              NewTextChild(rowNode, "col", si->second.i_stat.mobile);
              NewTextChild(rowNode, "col", si->second.i_stat.mobile_bag);
              NewTextChild(rowNode, "col", si->second.i_stat.mobile_bp);
          }
          if(pr_pact and params.statType != statPactShort)
              NewTextChild(rowNode, "col", si->first.pact_descr);

          total += si->second;
      };
    }
    else total=DetailStatTotal;

    rowNode = NewTextChild(rowsNode, "row");

    xmlNodePtr colNode;
    if(params.statType != statPactShort) {
        if(params.airp_column_first) {
            colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", TAlignment::LeftJustify);
            SetProp(colNode, "sort", sortString);
            NewTextChild(rowNode, "col", getLocaleText("Итого:"));
            if (params.statType==statDetail)
            {
                colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/к"));
                SetProp(colNode, "width", 50);
                SetProp(colNode, "align", TAlignment::LeftJustify);
                SetProp(colNode, "sort", sortString);
                NewTextChild(rowNode, "col");
            };
        } else {
            colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/к"));
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", TAlignment::LeftJustify);
            SetProp(colNode, "sort", sortString);
            NewTextChild(rowNode, "col", getLocaleText("Итого:"));
            if (params.statType==statDetail)
            {
                colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
                SetProp(colNode, "width", 50);
                SetProp(colNode, "align", TAlignment::LeftJustify);
                SetProp(colNode, "sort", sortString);
                NewTextChild(rowNode, "col");
            };
        }
    }

    if(params.statType != statPactShort) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во рейсов"));
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", (int)(pr_pact?total.flts.size():total.flt_amount));
    }

    if(params.statType == statPactShort)
    {
        colNode = NewTextChild(headerNode, "col", getLocaleText("№ договора"));
        SetProp(colNode, "width", 230);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("Итого:"));
    }

    colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во пасс."));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount);

    if(params.statType != statPactShort) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("П"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.f);

        colNode = NewTextChild(headerNode, "col", getLocaleText("Б"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.c);

        colNode = NewTextChild(headerNode, "col", getLocaleText("Э"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.y);

        total.i_stat.toXML(headerNode, rowNode);
    }

    if(pr_pact and params.statType != statPactShort)
    {
        colNode = NewTextChild(headerNode, "col", getLocaleText("№ договора"));
        SetProp(colNode, "width", 230);
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
    setClientTypeCaps(variablesNode);
    NewTextChild(variablesNode, "pr_pact", pr_pact);
    if(params.statType == statPactShort) {
        NewTextChild(variablesNode, "stat_type", params.statType);
        NewTextChild(variablesNode, "stat_mode", getLocaleText("Договоры на использование DCS АСТРА"));
        string stat_type_caption;
        switch(params.statType) {
            case statPactShort:
                stat_type_caption = getLocaleText("Общая");
                break;
            default:
                throw Exception("createXMLDetailStat: unexpected statType %d", params.statType);
                break;
        }
        NewTextChild(variablesNode, "stat_type_caption", stat_type_caption);
    }
};

void createXMLFullStat(const TStatParams &params,
                       const TFullStat &FullStat, const TFullStatRow &FullStatTotal,
                       const TPrintAirline &airline, xmlNodePtr resNode)
{
    if(FullStat.empty() && FullStatTotal==TFullStatRow())
      throw AstraLocale::UserException("MSG.NOT_DATA");

    NewTextChild(resNode, "airline", airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    TFullStatRow total;
    bool showTotal=true;
    if (!params.skip_rows)
    {
      int rows = 0;
      for(TFullStat::const_iterator im = FullStat.begin(); im != FullStat.end(); ++im, rows++)
      {
          if(rows >= MAX_STAT_ROWS()) {
              throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
              /*AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH",
                                            LParams() << LParam("num", MAX_STAT_ROWS()));
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
              break;*/
          }
          //region обязательно в начале цикла, иначе будет испорчен xml
          string region;
          try
          {
              region = AirpTZRegion(im->first.airp);
          }
          catch(AstraLocale::UserException &E)
          {
              AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
              continue;
          };
          rowNode = NewTextChild(rowsNode, "row");
          NewTextChild(rowNode, "col", im->first.col1);
          NewTextChild(rowNode, "col", im->first.col2);
          NewTextChild(rowNode, "col", im->first.flt_no);
          NewTextChild(rowNode, "col", DateTimeToStr(
                      UTCToClient(im->first.scd_out, region), "dd.mm.yy")
                  );
          NewTextChild(rowNode, "col", im->first.places.get());
          NewTextChild(rowNode, "col", im->second.pax_amount);
          if (params.statType==statFull)
          {
              NewTextChild(rowNode, "col", im->second.i_stat.web);
              NewTextChild(rowNode, "col", im->second.i_stat.web_bag);
              NewTextChild(rowNode, "col", im->second.i_stat.web_bp);

              NewTextChild(rowNode, "col", im->second.i_stat.kiosk);
              NewTextChild(rowNode, "col", im->second.i_stat.kiosk_bag);
              NewTextChild(rowNode, "col", im->second.i_stat.kiosk_bp);

              NewTextChild(rowNode, "col", im->second.i_stat.mobile);
              NewTextChild(rowNode, "col", im->second.i_stat.mobile_bag);
              NewTextChild(rowNode, "col", im->second.i_stat.mobile_bp);
          };
          NewTextChild(rowNode, "col", im->second.adult);
          NewTextChild(rowNode, "col", im->second.child);
          NewTextChild(rowNode, "col", im->second.baby);
          NewTextChild(rowNode, "col", im->second.rk_weight);

          NewTextChild(rowNode, "col", im->second.bag_amount);
          NewTextChild(rowNode, "col", im->second.bag_weight);

          NewTextChild(rowNode, "col", im->second.excess_pc.getQuantity());
          NewTextChild(rowNode, "col", im->second.excess_wt.getQuantity());
          if (params.statType==statTrferFull)
              NewTextChild(rowNode, "col", im->first.point_id);

          total += im->second;
      };
    }
    else total=FullStatTotal;

    rowNode = NewTextChild(rowsNode, "row");

    xmlNodePtr colNode;
    if(params.airp_column_first) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("Итого:"));

        colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/к"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    } else {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/к"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("Итого:"));

        colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
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

    colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во пасс."));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount);

    if (params.statType==statFull)
        total.i_stat.toXML(headerNode, rowNode);

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

    colNode = NewTextChild(headerNode, "col", getLocaleText("Р/кладь (вес)"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.rk_weight);

    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ мест"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortIntegerSlashInteger);
    NewTextChild(rowNode, "col", total.bag_amount);

    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ вес"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortIntegerSlashInteger);
    NewTextChild(rowNode, "col", total.bag_weight);

    colNode = NewTextChild(headerNode, "col", getLocaleText("Пл.м"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col",  total.excess_pc.getQuantity());

    colNode = NewTextChild(headerNode, "col", getLocaleText("Пл.вес"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col",  total.excess_wt.getQuantity());

    if (!showTotal)
    {
      xmlUnlinkNode(rowNode);
      xmlFreeNode(rowNode);
    };

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    setClientTypeCaps(variablesNode);
    if (params.statType==statFull)
      NewTextChild(variablesNode, "caption", getLocaleText("Подробная сводка"));
    else
      NewTextChild(variablesNode, "caption", getLocaleText("Трансферная сводка"));
};

void RunPactDetailStat(const TStatParams &params,
                       TDetailStat &DetailStat, TDetailStatRow &DetailStatTotal,
                       TPrintAirline &prn_airline, bool full)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select\n"
        "    descr,\n"
        "    airline,\n"
        "    airp,\n"
        "    DECODE(SIGN(first_date-:FirstDate),1,first_date,:FirstDate) AS first_date,\n"
        "    DECODE(SIGN(last_date- :LastDate),-1,last_date, :LastDate) AS last_date\n"
        "from pacts where\n"
        "  first_date >= :FirstDate and first_date < :LastDate or\n"
        "  last_date >= :FirstDate and last_date < :LastDate or\n"
        "  first_date < :FirstDate and (last_date >= :LastDate or last_date is null)\n"
        "order by \n"
        "  airline nulls first \n";

    Qry.CreateVariable("FirstDate", otDate, params.FirstDate);
    Qry.CreateVariable("LastDate", otDate, params.LastDate);
    Qry.Execute();
    map<string, vector<TPact> > airp_pacts;
    vector<TPact> result_pacts;
    for(; !Qry.Eof; Qry.Next()) {
        TPact pact(
                Qry.FieldAsString("descr"),
                Qry.FieldAsString("airline"),
                Qry.FieldAsString("airp"),
                Qry.FieldAsDateTime("first_date"),
                Qry.FieldAsDateTime("last_date")
                );
        if(pact.airline.empty())
            airp_pacts[pact.airp].push_back(pact);
        else {
            /*
            ProgTrace(TRACE5, "before correct_airp_pacts:");
            ProgTrace(TRACE5, "descr: %s, first_date: %s, last_date: %s",
                    pact.descr.c_str(),
                    DateTimeToStr(pact.first_date, "ddmmyy").c_str(),
                    DateTimeToStr(pact.last_date, "ddmmyy").c_str()
                    );*/
            /*
            for(map<string, vector<TPact> >::iterator im = airp_pacts.begin(); im != airp_pacts.end(); im++)
                correct_airp_pacts(im->second, pact);
                */
            correct_airp_pacts(airp_pacts[pact.airp], pact);

            result_pacts.push_back(pact);
        }
    }
    /*
    ProgTrace(TRACE5, "AIRP_PACTS:");
    for(map<string, vector<TPact> >::iterator im = airp_pacts.begin(); im != airp_pacts.end(); im++) {
        ProgTrace(TRACE5, "airp: %s", im->first.c_str());
        ProgTrace(TRACE5, "periods:");
        for(vector<TPact>::iterator iv = im->second.begin(); iv != im->second.end(); iv++) {
            ProgTrace(TRACE5, "first_date: %s, last_date: %s",
                    DateTimeToStr(iv->first_date, "ddmmyy").c_str(),
                    DateTimeToStr(iv->last_date, "ddmmyy").c_str()
                    );
            string denied;
            for(set<string>::iterator is = iv->airlines.begin(); is != iv->airlines.end(); is++) {
                if(not denied.empty())
                    denied += ", ";
                denied += *is;
            }
            ProgTrace(TRACE5, "denied airlines: %s", denied.c_str());
        }
    }
    */
    for(map<string, vector<TPact> >::iterator im = airp_pacts.begin(); im != airp_pacts.end(); im++) {
        if(im->second.empty())
            continue;
        result_pacts.insert(result_pacts.end(), im->second.begin(), im->second.end());
    }

    Qry.Clear();
    for(int pass = 0; pass <= 2; pass++) {
        ostringstream sql;
        sql << "SELECT \n"
            " points.airline, \n"
            " points.airp, \n"
            " points.scd_out, \n"
            " stat.point_id, \n"
            " adult, \n"
            " child, \n"
            " baby, \n"
            " term_bp, \n"
            " term_bag, \n"
            " f, c, y, \n"
            " client_type \n"
            "FROM \n";
        if (pass!=0)
        {
            sql << " arx_points points, \n"
                " arx_stat stat \n";
            if (pass==2)
                sql << ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :last_date+:arx_trip_date_range AND part_key <= :last_date+date_range) arx_ext \n";
        }
        else
            sql << " points, \n"
                " stat \n";
        sql << "WHERE \n";
        if (pass==1)
            sql << " points.part_key >= :first_date AND points.part_key < :last_date + :arx_trip_date_range AND \n";
        if (pass==2)
            sql << " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        if (pass!=0)
            sql << " stat.part_key = points.part_key AND \n";
        sql << " stat.point_id = points.point_id AND \n"
            " points.pr_del>=0 AND \n"
            " points.scd_out >= :first_date AND points.scd_out < :last_date \n";

        if(params.flt_no != NoExists) {
            sql << " AND points.flt_no = :flt_no \n";
            Qry.CreateVariable("flt_no", otInteger, params.flt_no);
        }
        if (!params.airps.elems().empty()) {
            if (params.airps.elems_permit())
                sql << " AND points.airp IN " << GetSQLEnum(params.airps.elems()) << "\n";
            else
                sql << " AND points.airp NOT IN " << GetSQLEnum(params.airps.elems()) << "\n";
        };
        if (!params.airlines.elems().empty()) {
            if (params.airlines.elems_permit())
                sql << " AND points.airline IN " << GetSQLEnum(params.airlines.elems()) << "\n";
            else
                sql << " AND points.airline NOT IN " << GetSQLEnum(params.airlines.elems()) << "\n";
        };

        //ProgTrace(TRACE5, "RunPactDetailStat: pass=%d SQL=\n%s", pass, sql.str().c_str());
        Qry.SQLText = sql.str().c_str();
        if (pass!=0)
            Qry.CreateVariable("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());

        Qry.CreateVariable("first_date", otDate, params.FirstDate);
        Qry.CreateVariable("last_date", otDate, params.LastDate);
        Qry.Execute();
        if(not Qry.Eof) {
            int col_airline = Qry.FieldIndex("airline");
            int col_airp = Qry.FieldIndex("airp");
            int col_scd_out = Qry.FieldIndex("scd_out");
            int col_point_id = Qry.FieldIndex("point_id");
            int col_adult = Qry.FieldIndex("adult");
            int col_child = Qry.FieldIndex("child");
            int col_baby = Qry.FieldIndex("baby");
            int col_term_bp = Qry.FieldIndex("term_bp");
            int col_term_bag = Qry.FieldIndex("term_bag");
            int col_f = Qry.FieldIndex("f");
            int col_c = Qry.FieldIndex("c");
            int col_y = Qry.FieldIndex("y");
            int col_client_type = Qry.FieldIndex("client_type");
            for(; not Qry.Eof; Qry.Next())
            {
                TDetailStatRow row;
                TClientType client_type = DecodeClientType(Qry.FieldAsString(col_client_type));
                int pax_amount=Qry.FieldAsInteger(col_adult) +
                    Qry.FieldAsInteger(col_child) +
                    Qry.FieldAsInteger(col_baby);

                row.flts.insert(Qry.FieldAsInteger(col_point_id));
                row.pax_amount = pax_amount;

                row.i_stat.web = (client_type == ctWeb ? pax_amount : 0);
                row.i_stat.kiosk = (client_type == ctKiosk ? pax_amount : 0);
                row.i_stat.mobile = (client_type == ctMobile ? pax_amount : 0);

                row.f = Qry.FieldAsInteger(col_f);
                row.c = Qry.FieldAsInteger(col_c);
                row.y = Qry.FieldAsInteger(col_y);

                int term_bp = Qry.FieldAsInteger(col_term_bp);
                int term_bag = Qry.FieldAsInteger(col_term_bag);

                row.i_stat.web_bag = (client_type == ctWeb ? term_bag : 0);
                row.i_stat.web_bp = (client_type == ctWeb ? term_bp : 0);

                row.i_stat.kiosk_bag = (client_type == ctKiosk ? term_bag : 0);
                row.i_stat.kiosk_bp = (client_type == ctKiosk ? term_bp : 0);

                row.i_stat.mobile_bag = (client_type == ctMobile ? term_bag : 0);
                row.i_stat.mobile_bp = (client_type == ctMobile ? term_bp : 0);

                if (!params.skip_rows)
                {
                    string airline = Qry.FieldAsString(col_airline);
                    string airp = Qry.FieldAsString(col_airp);
                    TDateTime scd_out = Qry.FieldAsDateTime(col_scd_out);

                    vector<TPact>::iterator iv = result_pacts.begin();
                    for(; iv != result_pacts.end(); iv++) {
                        if(
                                scd_out >= iv->first_date and
                                scd_out < iv->last_date and
                                airp == iv->airp and
                                (iv->airline.empty() or iv->airline == airline) and
                                (iv->airlines.empty() or find(iv->airlines.begin(), iv->airlines.end(), airline) == iv->airlines.end())
                          )
                        {
                            break;
                        }
                    }
                    TDetailStatKey key;
                    if(iv != result_pacts.end())
                        key.pact_descr = iv->descr;
                    if(
                            params.statType == statDetail or
                            params.statType == statShort
                      ) {
                        if(params.airp_column_first) {
                            key.col1 = ElemIdToCodeNative(etAirp, airp);
                            if (params.statType==statDetail)
                            {
                                key.col2 = ElemIdToCodeNative(etAirline, airline);
                                prn_airline.check(key.col2);
                            };
                        } else {
                            key.col1 = ElemIdToCodeNative(etAirline, airline);
                            if (params.statType==statDetail)
                            {
                                key.col2 = ElemIdToCodeNative(etAirp, airp);
                            };
                            prn_airline.check(key.col1);
                        }
                    }

                    AddStatRow(key, row, DetailStat, full);
                }
                else
                {
                    DetailStatTotal+=row;
                };
            }
        }
    }
}

void RunDetailStat(const TStatParams &params,
                   TDetailStat &DetailStat, TDetailStatRow &DetailStatTotal,
                   TPrintAirline &airline, bool full)
{
    TQuery Qry(&OraSession);
    Qry.CreateVariable("FirstDate", otDate, params.FirstDate);
    Qry.CreateVariable("LastDate", otDate, params.LastDate);
    Qry.CreateVariable("web", otString, EncodeClientType(ctWeb));
    Qry.CreateVariable("kiosk", otString, EncodeClientType(ctKiosk));
    Qry.CreateVariable("mobile", otString, EncodeClientType(ctMobile));
    if (params.seance==seanceAirline) Qry.DeclareVariable("ak",otString);
    if (params.seance==seanceAirport) Qry.DeclareVariable("ap",otString);
    if(params.flt_no != NoExists)
        Qry.CreateVariable("flt_no", otString, params.flt_no);

    for(int pass = 0; pass <= 2; pass++) {
          Qry.SQLText = GetStatSQLText(params,pass).c_str();
        if (pass!=0)
          Qry.CreateVariable("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());

        if (params.seance==seanceAirline)
        {
          //цикл по компаниям
          if (params.airlines.elems_permit())
          {
            for(set<string>::const_iterator i=params.airlines.elems().begin();
                                            i!=params.airlines.elems().end(); i++)
            {
              Qry.SetVariable("ak",*i);
              GetDetailStat(params, Qry, DetailStat, DetailStatTotal, airline, "", full);
            };
          };
          continue;
        };

        if (params.seance==seanceAirport)
        {
          //цикл по портам
          if (params.airps.elems_permit())
          {
            for(set<string>::const_iterator i=params.airps.elems().begin();
                                            i!=params.airps.elems().end(); i++)
            {
              Qry.SetVariable("ap",*i);
              GetDetailStat(params, Qry, DetailStat, DetailStatTotal, airline, "", full);
            };
          };
          continue;
        };
        GetDetailStat(params, Qry, DetailStat, DetailStatTotal, airline, "", full);
    }
};

struct TDetailStatCombo : public TOrderStatItem
{
    typedef std::pair<TDetailStatKey, TDetailStatRow> Tdata;
    Tdata data;
    TStatParams params;
    bool pr_pact;
    TDetailStatCombo(const Tdata &aData, const TStatParams &aParams)
        : data(aData), params(aParams), pr_pact(aParams.pr_pacts) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TDetailStatCombo::add_header(ostringstream &buf) const
{
    if(params.statType != statPactShort)
    {
        if (params.airp_column_first)
        {
            buf << "Код а/п" << delim;
            if (params.statType==statDetail)
                buf << "Код а/к" << delim;
        } else {
            buf << "Код а/к" << delim;
            if (params.statType==statDetail)
                buf << "Код а/п" << delim;
        }
    }
    if(params.statType != statPactShort)
        buf << "Кол-во рейсов" << delim;
    if(params.statType == statPactShort)
        buf << "№ договора" << delim;
    buf << "Кол-во пасс." << delim;
    if (params.statType != statPactShort)
    {
        buf << "П" << delim;
        buf << "Б" << delim;
        buf << "Э" << delim;
        /* TInetStat::toXML begin */
        buf << "Web" << delim;
        buf << "БГ" << delim;
        buf << "ПТ" << delim;
        buf << "Киоски" << delim;
        buf << "БГ" << delim;
        buf << "ПТ" << delim;
        buf << "Моб." << delim;
        buf << "БГ" << delim;
        buf << "ПТ" << delim;
        /* end */
    }
    if(pr_pact and params.statType != statPactShort)
        buf << "№ договора" << delim;
    buf << endl; /* остаётся пустой столбец */
}

void TDetailStatCombo::add_data(ostringstream &buf) const
{
    if(params.statType != statPactShort)
        buf << data.first.col1 << delim; // col1
    if (params.statType == statDetail)
        buf << data.first.col2 << delim; // col2
    if(params.statType != statPactShort)
        buf << (int)(pr_pact ? data.second.flts.size() : data.second.flt_amount) << delim; // Кол-во рейсов
    if(params.statType == statPactShort)
        buf << data.first.pact_descr << delim; // № договора
    buf << data.second.pax_amount << delim; // Кол-во пасс.
    if (params.statType != statPactShort)
    {
        buf << data.second.f << delim; // П
        buf << data.second.c << delim; // Б
        buf << data.second.y << delim; // Э
        buf << data.second.i_stat.web << delim; // Web
        buf << data.second.i_stat.web_bag << delim; // БГ
        buf << data.second.i_stat.web_bp << delim; // ПТ
        buf << data.second.i_stat.kiosk << delim; // Киоски
        buf << data.second.i_stat.kiosk_bag << delim; // БГ
        buf << data.second.i_stat.kiosk_bp << delim; // ПТ
        buf << data.second.i_stat.mobile << delim; // Моб.
        buf << data.second.i_stat.mobile_bag << delim; // БГ
        buf << data.second.i_stat.mobile_bp << delim; // ПТ
    }
    if(pr_pact and params.statType != statPactShort)
        buf << data.first.pact_descr << delim; // № договора
    buf << endl; /* остаётся пустой столбец */
}

void RunDetailStatFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline)
{
    TDetailStat DetailStat;
    TDetailStatRow DetailStatTotal;
    if(params.pr_pacts)
        RunPactDetailStat(params, DetailStat, DetailStatTotal, prn_airline, true);
    else
        RunDetailStat(params, DetailStat, DetailStatTotal, prn_airline, true);
    for (TDetailStat::const_iterator i = DetailStat.begin(); i != DetailStat.end(); ++i)
        writer.insert(TDetailStatCombo(*i, params));
}

void RunFullStat(const TStatParams &params,
                 TFullStat &FullStat, TFullStatRow &FullStatTotal,
                 TPrintAirline &airline, bool full)
{
    TQuery Qry(&OraSession);
    Qry.CreateVariable("FirstDate", otDate, params.FirstDate);
    Qry.CreateVariable("LastDate", otDate, params.LastDate);
    if (params.statType==statFull)
    {
      Qry.CreateVariable("web", otString, EncodeClientType(ctWeb));
      Qry.CreateVariable("kiosk", otString, EncodeClientType(ctKiosk));
      Qry.CreateVariable("mobile", otString, EncodeClientType(ctMobile));
    };
    if (params.seance==seanceAirline) Qry.DeclareVariable("ak",otString);
    if (params.seance==seanceAirport) Qry.DeclareVariable("ap",otString);
    if(params.flt_no != NoExists)
        Qry.CreateVariable("flt_no", otString, params.flt_no);

    for(int pass = 0; pass <= 2; pass++) {
          Qry.SQLText = GetStatSQLText(params,pass).c_str();
        if (pass!=0)
          Qry.CreateVariable("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        //ProgTrace(TRACE5, "RunFullStat: pass=%d SQL=\n%s", pass, Qry.SQLText.SQLText());

        if (params.seance==seanceAirline)
        {
          //цикл по компаниям
          if (params.airlines.elems_permit())
          {
            for(set<string>::const_iterator i=params.airlines.elems().begin();
                                            i!=params.airlines.elems().end(); i++)
            {
              Qry.SetVariable("ak",*i);
              GetFullStat(params, Qry, FullStat, FullStatTotal, airline, full);
            };
          };
          continue;
        };

        if (params.seance==seanceAirport)
        {
          //цикл по портам
          if (params.airps.elems_permit())
          {
            for(set<string>::const_iterator i=params.airps.elems().begin();
                                            i!=params.airps.elems().end(); i++)
            {
              Qry.SetVariable("ap",*i);
              GetFullStat(params, Qry, FullStat, FullStatTotal, airline, full);
            };
          };
          continue;
        };
        GetFullStat(params, Qry, FullStat, FullStatTotal, airline, full);
    }
};

struct TFullStatCombo : public TOrderStatItem
{
    typedef std::pair<TFullStatKey, TFullStatRow> Tdata;
    Tdata data;
    TStatParams params;
    TFullStatCombo(const Tdata &aData, const TStatParams &aParams)
        : data(aData), params(aParams) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TFullStatCombo::add_header(ostringstream &buf) const
{
    if (params.airp_column_first)
    {
        buf << "Код а/п" << delim;
        buf << "Код а/к" << delim;
    } else {
        buf << "Код а/к" << delim;
        buf << "Код а/п" << delim;
    }
    buf << "Номер рейса" << delim;
    buf << "Дата" << delim;
    buf << "Направление" << delim;
    buf << "Кол-во пасс." << delim;
    if (params.statType==statFull)
    {
        /* TInetStat::toXML begin */
        buf << "Web" << delim;
        buf << "БГ" << delim;
        buf << "ПТ" << delim;
        buf << "Киоски" << delim;
        buf << "БГ" << delim;
        buf << "ПТ" << delim;
        buf << "Моб." << delim;
        buf << "БГ" << delim;
        buf << "ПТ" << delim;
        /* end */
    }
    buf << "ВЗ" << delim;
    buf << "РБ" << delim;
    buf << "РМ" << delim;
    buf << "Р/кладь (вес)" << delim;
    buf << "БГ мест" << delim;
    buf << "БГ вес" << delim;
    buf << "Пл.м" << delim;
    buf << "Пл.вес" << endl;
}

void TFullStatCombo::add_data(ostringstream &buf) const
{
    string region;
    try { region = AirpTZRegion(data.first.airp); }
    catch(AstraLocale::UserException &E) { return; };
    buf << data.first.col1 << delim; // col1
    buf << data.first.col2 << delim; // col2
    buf << data.first.flt_no << delim; // Номер рейса
    buf << DateTimeToStr(UTCToClient(data.first.scd_out, region), "dd.mm.yy") << delim; // Дата
    buf << data.first.places.get() << delim; // Направление
    buf << data.second.pax_amount << delim; // Кол-во пасс.
    if (params.statType==statFull)
    {
        buf << data.second.i_stat.web << delim; // Web
        buf << data.second.i_stat.web_bag << delim; // БГ
        buf << data.second.i_stat.web_bp << delim; // ПТ
        buf << data.second.i_stat.kiosk << delim; // Киоски
        buf << data.second.i_stat.kiosk_bag << delim; // БГ
        buf << data.second.i_stat.kiosk_bp << delim; // ПТ
        buf << data.second.i_stat.mobile << delim; // Моб.
        buf << data.second.i_stat.mobile_bag << delim; // БГ
        buf << data.second.i_stat.mobile_bp << delim; // ПТ
    }
    buf << data.second.adult << delim; // ВЗ
    buf << data.second.child << delim; // РБ
    buf << data.second.baby << delim; // РМ
    buf << data.second.rk_weight << delim; // Р/кладь (вес)
    buf << data.second.bag_amount << delim; // БГ мест
    buf << data.second.bag_weight << delim; // БГ вес
    buf << data.second.excess_pc.getQuantity() << delim; // Пл.м
    buf << data.second.excess_wt.getQuantity() << endl; // Пл.вес
}

void RunFullStatFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline)
{
    TFullStat FullStat;
    TFullStatRow FullStatTotal;
    RunFullStat(params, FullStat, FullStatTotal, prn_airline, true);
    for (TFullStat::const_iterator i = FullStat.begin(); i != FullStat.end(); ++i)
        writer.insert(TFullStatCombo(*i, params));
}

void createCSVFullStat(const TStatParams &params, const TFullStat &FullStat, const TPrintAirline &prn_airline, ostringstream &data)
{
    if(FullStat.empty()) return;

    const char delim = ';';
    data
        << "Код а/к" << delim
        << "Код а/п" << delim
        << "Номер рейса" << delim
        << "Дата" << delim
        << "Направление" << delim
        << "Кол-во пасс." << delim
        << "Web" << delim
        << "БГ" << delim
        << "ПТ" << delim
        << "Киоски" << delim
        << "БГ" << delim
        << "ПТ" << delim
        << "Моб." << delim
        << "БГ" << delim
        << "ПТ" << delim
        << "ВЗ" << delim
        << "РБ" << delim
        << "РМ" << delim
        << "Р/кладь (вес)" << delim
        << "БГ мест" << delim
        << "БГ вес" << delim
        << "Пл.м" << delim
        << "Пл.вес"
        << endl;
    bool showTotal = true;
    for(TFullStat::const_iterator i = FullStat.begin(); i != FullStat.end(); i++) {
        //region обязательно в начале цикла, иначе будет испорчен xml
        string region;
        try
        {
            region = AirpTZRegion(i->first.airp);
        }
        catch(AstraLocale::UserException &E)
        {
            AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
            if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
            continue;
        };

        // Код а/к
        data << i->first.col1 << delim;
        // Код а/п
        data << i->first.col2 << delim;
        // Номер рейса
        data << i->first.flt_no << delim;
        // Дата
        data << DateTimeToStr(
                UTCToClient(i->first.scd_out, region), "dd.mm.yy")
            << delim;
        // Направление
        data << i->first.places.get() << delim;
        // Кол-во пасс.
        data << i->second.pax_amount << delim;
        // Web
        data << i->second.i_stat.web << delim;
        // БГ
        data << i->second.i_stat.web_bag << delim;
        // ПТ
        data << i->second.i_stat.web_bp << delim;
        // Киоски
        data << i->second.i_stat.kiosk << delim;
        // БГ
        data << i->second.i_stat.kiosk_bag << delim;
        // ПТ
        data << i->second.i_stat.kiosk_bp << delim;
        // Моб.
        data << i->second.i_stat.mobile << delim;
        // БГ
        data << i->second.i_stat.mobile_bag << delim;
        // ПТ
        data << i->second.i_stat.mobile_bp << delim;
        // ВЗ
        data << i->second.adult << delim;
        // РБ
        data << i->second.child << delim;
        // РМ
        data << i->second.baby << delim;
        // Р/кладь (вес)
        data << i->second.rk_weight << delim;
        // БГ мест
        data << i->second.bag_amount << delim;
        // БГ вес
        data << i->second.bag_weight << delim;
        // Пл.м
        data << i->second.excess_pc.getQuantity() << delim;
        // Пл.вес
        data << i->second.excess_wt.getQuantity() << endl;
    }
}

