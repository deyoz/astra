#include "comp_layers.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "oralib.h"
#include "alarms.h"
#include "salons.h"
#include "date_time.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;

struct TPointIds
{
  int point_dep,point_arv,point_id;
  bool pr_lat_seat,pr_paid_ckin; //соответствует point_id
};

string GetPaxName(const char* surname,
                  const char* name)
{
  if (*surname==0 && *name==0)
    return "";
  else
    return (string)surname+(*name!=0?" ":"")+name;
};

bool IsTlgCompLayer(TCompLayerType layer_type)
{
  return (layer_type==cltSOMTrzt||
          layer_type==cltPRLTrzt||
          layer_type==cltPNLCkin||
          layer_type==cltProtBeforePay||
          layer_type==cltProtAfterPay||
          layer_type==cltPNLBeforePay||
          layer_type==cltPNLAfterPay||
          layer_type==cltProtCkin ||
          layer_type==cltProtSelfCkin);
};

static std::string getTlgLayerTypeSQLEnum(const std::initializer_list<TCompLayerType>& layerTypes)
{
  set<string> values;
  for(const auto& l : layerTypes)
  {
    if (!IsTlgCompLayer(l)) continue;
    values.insert(EncodeCompLayerType(l));
  }
  return GetSQLEnum(values);
}

void PaxIdsForDeleteTlgSeatRanges::add(const std::initializer_list<ASTRA::TCompLayerType>& layerTypes,
                                       int crs_pax_id)
{
  if (!emplace(crs_pax_id, layerTypes).second)
    ProgError(STDLOG, "PaxIdsForDeleteTlgSeatRanges::add: Warning! crs_pax_id=%d duplicated", crs_pax_id);
}

void PaxIdsForDeleteTlgSeatRanges::handle(int &curr_tid,
                                          TPointIdsForCheck &point_ids_spp) const
{
  for(const auto& i : *this)
  {
    const PaxIdsForDeleteTlgSeatRangesItem& item=i.second;
    DeleteTlgSeatRanges(item.layerTypes, i.first, curr_tid, point_ids_spp);
//    ProgTrace(TRACE5, "DeleteTlgSeatRanges: layer_types=%s crs_pax_id=%d", getTlgLayerTypeSQLEnum(item.layerTypes).c_str(), i.first);
  }
}

void PaxIdsForInsertTlgSeatRanges::add(const std::string& airp_arv,
                                       const TlgSeatRanges& tlgSeatRanges,
                                       int crs_pax_id)
{
  if (!emplace(crs_pax_id, PaxIdsForInsertTlgSeatRangesItem(airp_arv, tlgSeatRanges)).second)
    ProgError(STDLOG, "PaxIdsForInsertTlgSeatRanges::add: Warning! crs_pax_id=%d duplicated", crs_pax_id);
}

void PaxIdsForInsertTlgSeatRanges::handle(int point_id_tlg,
                                          int tlg_id,
                                          int &curr_tid,
                                          TPointIdsForCheck &point_ids_spp) const
{
  bool usePriorContext=false;
  for(const auto& i : *this)
  {
    const PaxIdsForInsertTlgSeatRangesItem& item=i.second;
    for(const auto& r : item.tlgSeatRanges)
    {
      TSeatRanges seatRanges;
      seatRanges.assign(r.second.begin(), r.second.end());
      InsertTlgSeatRanges(point_id_tlg,
                          item.airp_arv,
                          r.first,
                          seatRanges,
                          i.first,
                          tlg_id,
                          ASTRA::NoExists,
                          usePriorContext,
                          curr_tid,
                          point_ids_spp);
//      ProgTrace(TRACE5, "InsertTlgSeatRanges: layer_type=%s seatRanges=%s crs_pax_id=%d", EncodeCompLayerType(r.first), seatRanges.traceStr().c_str(), i.first);
      usePriorContext=true;
    }
  }
}

void InsertTripSeatRanges(const vector< pair<int, TSeatRange> > &ranges, //вектор пар range_id и TSeatRange
                          int point_id_tlg,
                          int point_id_spp,
                          string airp_arv,
                          TCompLayerType layer_type,
                          int crs_pax_id,            //может быть NoExists
                          const string &crs_pax_name,
                          const string &pers_type,
                          bool UsePriorContext,
                          TPointIdsForCheck &point_ids_spp) //вектор point_id_spp по которым были изменения
{
  using namespace BASIC::date_time;
  if (ranges.empty()) return;
  if (!IsTlgCompLayer(layer_type)) return;
  if (layer_type==cltPRLTrzt) return;

  static int prior_point_id_tlg=NoExists;
  static int prior_point_id_spp=NoExists;
  static string prior_airp_arv;
  static TCompLayerType prior_layer_type=cltUnknown;
  static vector<TPointIds> prior_point_ids;

  TQuery InsQry(&OraSession);
  InsQry.Clear();
  InsQry.SQLText=
    "INSERT INTO trip_comp_layers(range_id,point_id,point_dep,point_arv,layer_type, "
    "  first_xname,last_xname,first_yname,last_yname,crs_pax_id,pax_id,time_create) "
    "VALUES(:range_id,:point_id,:point_dep,:point_arv,:layer_type, "
    "  :first_xname,:last_xname,:first_yname,:last_yname,:crs_pax_id,NULL,:time_create)";
  InsQry.DeclareVariable("range_id",otInteger);
  InsQry.DeclareVariable("point_id",otInteger);
  InsQry.DeclareVariable("point_dep",otInteger);
  InsQry.DeclareVariable("point_arv",otInteger);
  InsQry.CreateVariable("layer_type",otString,EncodeCompLayerType(layer_type));
  InsQry.DeclareVariable("first_xname",otString);
  InsQry.DeclareVariable("last_xname",otString);
  InsQry.DeclareVariable("first_yname",otString);
  InsQry.DeclareVariable("last_yname",otString);
  InsQry.CreateVariable( "time_create", otDate, NowUTC() );
  if (crs_pax_id!=NoExists)
    InsQry.CreateVariable("crs_pax_id",otInteger,crs_pax_id);
  else
    InsQry.CreateVariable("crs_pax_id",otInteger,FNull);


  if (!(prior_point_id_tlg!=NoExists &&
        point_id_tlg==prior_point_id_tlg &&
        point_id_spp==prior_point_id_spp &&
        airp_arv==prior_airp_arv &&
        layer_type==prior_layer_type &&
        UsePriorContext))
  {
    prior_point_id_tlg=point_id_tlg;
    prior_point_id_spp=point_id_spp;
    prior_airp_arv=airp_arv;
    prior_layer_type=layer_type;
    prior_point_ids.clear();

    TQuery Qry(&OraSession);
    Qry.Clear();
    ostringstream sql;
    sql << "SELECT points.point_id,point_num,first_point,pr_tranzit,pr_lat_seat, "
           "       trip_paid_ckin.pr_permit AS pr_paid_ckin "
           "FROM tlg_binding,points,trip_sets,trip_paid_ckin "
           "WHERE tlg_binding.point_id_spp=points.point_id AND "
           "      points.point_id=trip_sets.point_id(+) AND "
           "      points.point_id=trip_paid_ckin.point_id(+) AND "
           "      tlg_binding.point_id_tlg=:point_id_tlg ";
    if (point_id_spp!=NoExists)
    {
      sql << "AND point_id_spp=:point_id_spp ";
      Qry.CreateVariable("point_id_spp",otInteger,point_id_spp);
    };
    Qry.SQLText=sql.str().c_str();
    Qry.CreateVariable("point_id_tlg",otInteger,point_id_tlg);
    Qry.Execute();
    TPointIds ids;
    for(;!Qry.Eof;Qry.Next())
    {
      //цикл по привязанным рейсам
      ids.point_dep=Qry.FieldAsInteger("point_id");
      ids.pr_lat_seat=true;
      if (!Qry.FieldIsNULL("pr_lat_seat"))
        ids.pr_lat_seat=Qry.FieldAsInteger("pr_lat_seat")!=0;
      ids.pr_paid_ckin=false;
      if (!Qry.FieldIsNULL("pr_paid_ckin"))
        ids.pr_paid_ckin=Qry.FieldAsInteger("pr_paid_ckin")!=0;

      TTripRoute route;
      route.GetRouteAfter(NoExists,
                          Qry.FieldAsInteger("point_id"),
                          Qry.FieldAsInteger("point_num"),
                          Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point"),
                          Qry.FieldAsInteger("pr_tranzit")!=0,
                          trtNotCurrent, trtWithCancelled);

      if (!route.empty())
      {
        vector<int> point_id;
        if (!(layer_type==cltSOMTrzt||
              layer_type==cltPRLTrzt)) point_id.push_back(ids.point_dep); //point_id=point_dep

        TTripRoute::const_iterator r=route.begin();
        for(; r!=route.end(); r++) //цикл по маршруту
        {
          if (r->airp==airp_arv) break;
          if (layer_type==cltSOMTrzt ||
              layer_type==cltPRLTrzt)
          {
            point_id.push_back(r->point_id);
          };
        };
        if (r!=route.end())
        {
          //нашли пункт прилета
          ids.point_arv=r->point_id;
          for(vector<int>::iterator i=point_id.begin();i!=point_id.end();i++)
          {
            ids.point_id=*i;
            prior_point_ids.push_back(ids);
          };
        };
      };
    };
  };


  TSeatRanges seat_view_ranges;
  for(vector< pair<int, TSeatRange> >::const_iterator r=ranges.begin(); r!=ranges.end(); r++)
    seat_view_ranges.push_back(r->second);

  for(vector<TPointIds>::iterator i=prior_point_ids.begin();i!=prior_point_ids.end();i++)
  {
    if (!i->pr_paid_ckin &&
        (layer_type==cltProtBeforePay||
         layer_type==cltProtAfterPay||
         layer_type==cltPNLBeforePay||
         layer_type==cltPNLAfterPay)) continue; //не синхронизируем с trip_comp_layers платные слои если нет платной регистрации
    InsQry.SetVariable("point_id",i->point_id);
    InsQry.SetVariable("point_dep",i->point_dep);
    InsQry.SetVariable("point_arv",i->point_arv);

    for(vector< pair<int, TSeatRange> >::const_iterator r=ranges.begin(); r!=ranges.end(); r++)
    {
      InsQry.SetVariable("range_id", r->first);
      InsQry.SetVariable("first_xname", r->second.first.line);
      InsQry.SetVariable("last_xname",  r->second.second.line);
      InsQry.SetVariable("first_yname", r->second.first.row);
      InsQry.SetVariable("last_yname",  r->second.second.row);
      InsQry.Execute();
    };

    point_ids_spp.insert( make_pair(i->point_id, layer_type) );

    if (crs_pax_id!=NoExists &&
        (layer_type==cltProtBeforePay||
         layer_type==cltProtAfterPay||
         layer_type==cltPNLBeforePay||
         layer_type==cltPNLAfterPay||
         layer_type==cltProtCkin||
         layer_type==cltProtSelfCkin))
    {
      TLogLocale tlocale;
      tlocale.ev_type=ASTRA::evtPax;
      tlocale.id1=i->point_id;
      int seats;
      string seat_view=GetSeatRangeView(seat_view_ranges, "list", i->pr_lat_seat, seats);
      tlocale.prms << PrmSmpl<string>("pax_name", crs_pax_name) << PrmElem<string>("type", etPersType, pers_type)
                   << PrmSmpl<string>("seat_view", seat_view);
      switch (layer_type)
      {
        case cltPNLBeforePay:  tlocale.lexema_id = "EVT.SEATS_RESERVATION_BEFORE_PNL_ADL";
                               break;
        case cltPNLAfterPay:   tlocale.lexema_id = "EVT.SEATS_RESERVATION_AFTER_PNL_ADL";
                               break;
        case cltProtBeforePay: tlocale.lexema_id = "EVT.SEATS_RESERVATION_BEFORE_WEB";
                               break;
        case cltProtAfterPay:  tlocale.lexema_id = "EVT.SEATS_RESERVATION_AFTER_WEB";
                               break;
        case cltProtCkin:      tlocale.lexema_id = "EVT.PRELIMINARY_ASSIGNED_SEATS";
                               break;
        case cltProtSelfCkin:  tlocale.lexema_id = "EVT.SEATS_RESERVATION_SELFCKIN";
                               break;
        default: break;
      };
      TReqInfo::Instance()->LocaleToLog(tlocale);
    };
  };
};

void InsertTlgSeatRanges(int point_id_tlg,
                         const string& airp_arv,
                         TCompLayerType layer_type,
                         const TSeatRanges &ranges,
                         int crs_pax_id, //может быть NoExists
                         int tlg_id,     //может быть NoExists
                         int timeout,    //может быть NoExists
                         bool UsePriorContext,
                         int &curr_tid,
                         TPointIdsForCheck &point_ids_spp) //вектор point_id_spp по которым были изменения
{
  if (ranges.empty()) return;
  if (!IsTlgCompLayer(layer_type)) return;

  TQuery Qry(&OraSession);

  string crs_pax_name, pers_type;
  if (crs_pax_id!=NoExists)
  {
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  IF :tid IS NULL THEN "
      "    SELECT cycle_tid__seq.nextval INTO :tid FROM dual; "
      "  END IF; "
      "  UPDATE crs_pax SET tid=:tid WHERE pax_id=:pax_id "
      "  RETURNING surname,name,pers_type INTO :surname,:name,:pers_type; "
      "END;";
    Qry.CreateVariable("pax_id",otInteger,crs_pax_id);
    if (curr_tid!=NoExists)
      Qry.CreateVariable("tid", otInteger, curr_tid);
    else
      Qry.CreateVariable("tid", otInteger, FNull);
    Qry.CreateVariable("surname", otString, FNull);
    Qry.CreateVariable("name", otString, FNull);
    Qry.CreateVariable("pers_type", otString, FNull);
    Qry.Execute();
    crs_pax_name=GetPaxName(Qry.GetVariableAsString("surname"),
                            Qry.GetVariableAsString("name"));
    curr_tid=(Qry.VariableIsNULL("tid")?NoExists:Qry.GetVariableAsInteger("tid"));
    pers_type = Qry.GetVariableAsString("pers_type");
  };
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  SELECT comp_layers__seq.nextval INTO :range_id FROM dual; "
    "  INSERT INTO tlg_comp_layers "
    "    (range_id,point_id,airp_arv,layer_type, "
    "     first_xname,last_xname,first_yname,last_yname,rem_code, "
    "     crs_pax_id,tlg_id,time_remove) "
    "  VALUES "
    "    (:range_id,:point_id,:airp_arv,:layer_type, "
    "     :first_xname,:last_xname,:first_yname,:last_yname,:rem_code, "
    "     :crs_pax_id,:tlg_id,SYSTEM.UTCSYSDATE+:timeout/1440); "
    "END; ";
  Qry.CreateVariable("range_id",otInteger,FNull);
  Qry.CreateVariable("point_id",otInteger,point_id_tlg);
  Qry.CreateVariable("airp_arv",otString,airp_arv);
  Qry.CreateVariable("layer_type",otString,EncodeCompLayerType(layer_type));

  if (crs_pax_id!=NoExists)
    Qry.CreateVariable("crs_pax_id",otInteger,crs_pax_id);
  else
    Qry.CreateVariable("crs_pax_id",otInteger,FNull);

  if (tlg_id!=NoExists)
    Qry.CreateVariable("tlg_id",otInteger,tlg_id);
  else
    Qry.CreateVariable("tlg_id",otInteger,FNull);

  if (timeout!=NoExists)
    Qry.CreateVariable("timeout",otInteger,timeout);
  else
    Qry.CreateVariable("timeout",otInteger,FNull);

  Qry.DeclareVariable("first_xname",otString);
  Qry.DeclareVariable("last_xname",otString);
  Qry.DeclareVariable("first_yname",otString);
  Qry.DeclareVariable("last_yname",otString);
  Qry.DeclareVariable("rem_code",otString);

  vector< pair<int, TSeatRange> > ranges_for_sync;

  for(TSeatRanges::const_iterator r=ranges.begin();r!=ranges.end();r++)
  {
    Qry.SetVariable("first_xname",r->first.line);
    Qry.SetVariable("last_xname",r->second.line);
    Qry.SetVariable("first_yname",r->first.row);
    Qry.SetVariable("last_yname",r->second.row);
    Qry.SetVariable("rem_code",r->rem);
    Qry.Execute();
    ranges_for_sync.push_back( make_pair(Qry.GetVariableAsInteger("range_id"),*r) );
  };
  InsertTripSeatRanges(ranges_for_sync,
                       point_id_tlg,
                       NoExists,
                       airp_arv,
                       layer_type,
                       crs_pax_id,
                       crs_pax_name,
                       pers_type,
                       UsePriorContext,
                       point_ids_spp);
};

void DeleteTripSeatRanges(const SeatRangeIds& range_ids,
                          int point_id_spp, //может быть NoExists
                          int crs_pax_id,   //может быть NoExists
                          const string& crs_pax_name,
                          const string& pers_type,
                          TPointIdsForCheck &point_ids_spp) //вектор point_id_spp по которым были изменения
{
  if (range_ids.empty()) return;

  TQuery Qry(&OraSession);
  if (crs_pax_id!=NoExists)
  {
    Qry.Clear();
    ostringstream sql;
    sql << "SELECT trip_comp_layers.point_id, layer_type, "
           "       first_xname, last_xname, first_yname, last_yname, "
           "       trip_sets.pr_lat_seat "
           "FROM trip_comp_layers, trip_sets "
           "WHERE trip_comp_layers.point_id=trip_sets.point_id(+) AND "
           "      trip_comp_layers.range_id=:range_id ";
    if (point_id_spp!=NoExists)
    {
      sql << "AND trip_comp_layers.point_id=:point_id_spp ";
      Qry.CreateVariable("point_id_spp",otInteger,point_id_spp);
    };
    sql << "FOR UPDATE";
    Qry.SQLText=sql.str().c_str();
    Qry.DeclareVariable("range_id", otInteger);
    map< pair<TCompLayerType, int> , pair<TSeatRanges, bool> > ranges; //ключ - пара layer_type, point_id
    for(SeatRangeIds::const_iterator i=range_ids.begin(); i!=range_ids.end(); i++)
    {
      Qry.SetVariable("range_id", *i);
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        TCompLayerType layer_type=DecodeCompLayerType(Qry.FieldAsString("layer_type"));
        int point_id=Qry.FieldAsInteger("point_id");
        point_ids_spp.insert( make_pair(point_id, layer_type) );
        pair<TSeatRanges, bool> &seat_view_ranges=ranges[ make_pair(layer_type,point_id) ];

        TSeatRange range(TSeat(Qry.FieldAsString("first_yname"),
                               Qry.FieldAsString("first_xname")),
                         TSeat(Qry.FieldAsString("last_yname"),
                               Qry.FieldAsString("last_xname")));

        seat_view_ranges.first.push_back(range);
        seat_view_ranges.second=(Qry.FieldIsNULL("pr_lat_seat")?true:Qry.FieldAsInteger("pr_lat_seat")!=0);
      };
    };

    map< pair<TCompLayerType, int> , pair<TSeatRanges, bool> >::const_iterator r=ranges.begin();
    for(;r!=ranges.end();r++)
    {
      TCompLayerType layer_type=r->first.first;
      if (layer_type==cltProtBeforePay||
          layer_type==cltProtAfterPay||
          layer_type==cltPNLBeforePay||
          layer_type==cltPNLAfterPay||
          layer_type==cltProtCkin ||
          layer_type==cltProtSelfCkin)
      {
        int point_id=r->first.second;
        bool pr_lat_seat=r->second.second;
        const TSeatRanges &seat_view_ranges=r->second.first;

        TLogLocale tlocale;
        tlocale.ev_type=ASTRA::evtPax;
        tlocale.id1=point_id;
        int seats;
        string seat_view=GetSeatRangeView(seat_view_ranges, "list", pr_lat_seat, seats);
        tlocale.prms << PrmSmpl<string>("pax_name", crs_pax_name) << PrmElem<string>("type", etPersType, pers_type)
                     << PrmSmpl<string>("seat_view", seat_view);
        switch (layer_type)
        {
          case cltPNLBeforePay:  tlocale.lexema_id = "EVT.CANCEL_SEATS_RESERVATION_BEFORE_PNL_ADL";
                                 break;
          case cltPNLAfterPay:   tlocale.lexema_id = "EVT.CANCEL_SEATS_RESERVATION_AFTER_PNL_ADL";
                                 break;
          case cltProtBeforePay: tlocale.lexema_id = "EVT.CANCEL_SEATS_RESERVATION_BEFORE_WEB";
                                 break;
          case cltProtAfterPay:  tlocale.lexema_id = "EVT.CANCEL_SEATS_RESERVATION_AFTER_WEB";
                                 break;
          case cltProtCkin:      tlocale.lexema_id = "EVT.CANCEL_PRELIMINARY_ASSIGNED_SEATS";
                                 break;
          case cltProtSelfCkin:  tlocale.lexema_id = "EVT.CANCEL_SEATS_RESERVATION_SELFCKIN";
                                 break;
          default: break;
        };
        TReqInfo::Instance()->LocaleToLog(tlocale);
      };
    };
  }
  else
  {
    Qry.Clear();
    ostringstream sql;
    sql << "SELECT point_id, layer_type "
           "FROM trip_comp_layers "
           "WHERE trip_comp_layers.range_id=:range_id ";
    if (point_id_spp!=NoExists)
    {
      sql << "AND trip_comp_layers.point_id=:point_id_spp ";
      Qry.CreateVariable("point_id_spp",otInteger,point_id_spp);
    };
    sql << "FOR UPDATE";
    Qry.SQLText=sql.str().c_str();
    Qry.DeclareVariable("range_id", otInteger);
    for(SeatRangeIds::const_iterator i=range_ids.begin(); i!=range_ids.end(); i++)
    {
      Qry.SetVariable("range_id", *i);
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        TCompLayerType layer_type=DecodeCompLayerType(Qry.FieldAsString("layer_type"));
        int point_id=Qry.FieldAsInteger("point_id");
        point_ids_spp.insert( make_pair(point_id, layer_type) );
      };
    };
  };

  Qry.Clear();
  ostringstream sql;
  sql <<
    "DELETE FROM trip_comp_layers "
    "WHERE range_id=:range_id ";
  if (point_id_spp!=NoExists)
  {
    sql << "AND point_id=:point_id_spp ";
    Qry.CreateVariable("point_id_spp",otInteger,point_id_spp);
  };
  Qry.SQLText=sql.str().c_str();
  Qry.DeclareVariable("range_id", otInteger);
  for(SeatRangeIds::const_iterator i=range_ids.begin(); i!=range_ids.end(); i++)
  {
    Qry.SetVariable("range_id", *i);
    Qry.Execute();
  };
};

void DeleteTlgSeatRanges(const SeatRangeIds& range_ids,
                         int crs_pax_id,
                         int &curr_tid,
                         TPointIdsForCheck &point_ids_spp) //вектор point_id_spp по которым были изменения
{
  if (range_ids.empty()) return;
  TQuery Qry(&OraSession);
  string crs_pax_name, pers_type;
  if (crs_pax_id!=NoExists)
  {
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  IF :tid IS NULL THEN "
      "    SELECT cycle_tid__seq.nextval INTO :tid FROM dual; "
      "  END IF; "
      "  UPDATE crs_pax SET tid=:tid WHERE pax_id=:pax_id "
      "  RETURNING surname,name,pers_type INTO :surname,:name,:pers_type; "
      "END;";
    Qry.CreateVariable("pax_id",otInteger,crs_pax_id);
    if (curr_tid!=NoExists)
      Qry.CreateVariable("tid", otInteger, curr_tid);
    else
      Qry.CreateVariable("tid", otInteger, FNull);
    Qry.CreateVariable("surname", otString, FNull);
    Qry.CreateVariable("name", otString, FNull);
    Qry.CreateVariable("pers_type", otString, FNull);
    Qry.Execute();
    crs_pax_name=GetPaxName(Qry.GetVariableAsString("surname"),
                            Qry.GetVariableAsString("name"));
    pers_type = Qry.GetVariableAsString("pers_type");
    curr_tid=(Qry.VariableIsNULL("tid")?NoExists:Qry.GetVariableAsInteger("tid"));
  };

  DeleteTripSeatRanges(range_ids,
                       NoExists,
                       crs_pax_id,
                       crs_pax_name,
                       pers_type,
                       point_ids_spp);
  Qry.Clear();
  Qry.SQLText="DELETE FROM tlg_comp_layers WHERE range_id=:range_id";
  Qry.DeclareVariable("range_id", otInteger);
  for(SeatRangeIds::const_iterator i=range_ids.begin(); i!=range_ids.end(); i++)
  {
    Qry.SetVariable("range_id", *i);
    Qry.Execute();
  };
};

void DeleteTlgSeatRanges(const std::initializer_list<TCompLayerType>& layer_types,
                         int crs_pax_id,
                         int &curr_tid,                    //может быть NoExists, тогда устанавливается в процедуре
                         TPointIdsForCheck &point_ids_spp) //вектор point_id_spp по которым были изменения
{
  if (crs_pax_id==NoExists) return;

  SeatRangeIds range_ids;
  GetTlgSeatRangeIds(layer_types, crs_pax_id, range_ids);

  DeleteTlgSeatRanges(range_ids, crs_pax_id, curr_tid, point_ids_spp);
}

void DeleteTlgSeatRanges(const std::initializer_list<ASTRA::TCompLayerType>& layer_types,
                         int crs_pax_id,
                         int &curr_tid)
{
  TPointIdsForCheck point_ids_spp;
  DeleteTlgSeatRanges(layer_types, crs_pax_id, curr_tid, point_ids_spp);
}

void TlgSeatRanges::add(TCompLayerType layerType,
                        const TSeatRange& seatRange)
{
  if (!IsTlgCompLayer(layerType)) return;
  auto i=find(layerType);
  if (i==end())
    i=emplace(layerType, set<TSeatRange>()).first;
  i->second.emplace(seatRange);
}

void TlgSeatRanges::add(TCompLayerType layerType,
                        const TSeatRanges& seatRanges)
{
  if (!IsTlgCompLayer(layerType)) return;
  if (seatRanges.empty()) return;
  auto i=find(layerType);
  if (i==end())
    i=emplace(layerType, set<TSeatRange>()).first;
  for(const TSeatRange& r : seatRanges)
    i->second.emplace(r);
}

void TlgSeatRanges::get(const std::initializer_list<ASTRA::TCompLayerType>& layerTypes,
                        int crs_pax_id)
{
  clear();

  string sql=getTlgLayerTypeSQLEnum(layerTypes);
  if (sql.empty()) return;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT layer_type, first_xname, last_xname, first_yname, last_yname FROM tlg_comp_layers "
    "WHERE crs_pax_id=:crs_pax_id AND layer_type IN " + sql;
  Qry.CreateVariable("crs_pax_id", otInteger, crs_pax_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    add(DecodeCompLayerType(Qry.FieldAsString("layer_type")),
        TSeatRange(TSeat(Qry.FieldAsString("first_yname"),
                         Qry.FieldAsString("first_xname")),
                   TSeat(Qry.FieldAsString("last_yname"),
                         Qry.FieldAsString("last_xname"))));
  }
}

void TlgSeatRanges::dump(const std::string& title) const
{
  LogTrace(TRACE5) << title;
  if (empty()) LogTrace(TRACE5) << "empty";
  for(const auto& i : *this)
  {
    if (i.second.empty())
      LogTrace(TRACE5) << EncodeCompLayerType(i.first) << ": empty";
    for(const TSeatRange& r : i.second)
      LogTrace(TRACE5) << EncodeCompLayerType(i.first) << ": " << r.traceStr();
  }
}

void GetTlgSeatRanges(ASTRA::TCompLayerType layer_type,
                      int crs_pax_id,
                      TSeatRanges &ranges)
{
  ranges.clear();

  TlgSeatRanges tlgSeatRanges;
  tlgSeatRanges.get({layer_type}, crs_pax_id);

  if (!tlgSeatRanges.empty())
    ranges.assign(tlgSeatRanges.begin()->second.begin(),
                  tlgSeatRanges.begin()->second.end());
}


void GetTlgSeatRangeIds(const std::initializer_list<TCompLayerType>& layerTypes,
                        int crs_pax_id,
                        SeatRangeIds& range_ids)
{
  range_ids.clear();

  string sql=getTlgLayerTypeSQLEnum(layerTypes);
  if (sql.empty()) return;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT range_id FROM tlg_comp_layers "
    "WHERE crs_pax_id=:crs_pax_id AND layer_type IN " + sql;
  Qry.CreateVariable("crs_pax_id", otInteger, crs_pax_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    range_ids.insert(Qry.FieldAsInteger("range_id"));
}

void SyncTripCompLayers(int point_id_tlg,
                        int point_id_spp,
                        TCompLayerType layer_type,
                        TPointIdsForCheck &point_ids_spp) //вектор point_id_spp по которым были изменения
{
  DeleteTripCompLayers(point_id_tlg, point_id_spp, layer_type, point_ids_spp);
  InsertTripCompLayers(point_id_tlg, point_id_spp, layer_type, point_ids_spp);
}

void InsertTripCompLayers(int point_id_tlg,
                          int point_id_spp,
                          TCompLayerType layer_type,
                          TPointIdsForCheck &point_ids_spp) //вектор point_id_spp по которым были изменения
{
  if (!IsTlgCompLayer(layer_type)) return;

  TQuery Qry(&OraSession);
  Qry.Clear();
  if (point_id_tlg!=NoExists)
  {
    Qry.SQLText=
      "SELECT range_id,airp_arv,point_id AS point_id_tlg, "
      "       first_xname,last_xname,first_yname,last_yname,rem_code, "
      "       crs_pax_id, "
      "       surname, name, pers_type "
      "FROM tlg_comp_layers, crs_pax "
      "WHERE tlg_comp_layers.crs_pax_id=crs_pax.pax_id(+) AND "
      "      point_id=:point_id_tlg AND layer_type=:layer_type "
      "ORDER BY point_id_tlg, airp_arv, crs_pax_id";
    Qry.CreateVariable("point_id_tlg",otInteger,point_id_tlg);
  }
  else
  {
    if (point_id_spp!=NoExists)
    {
      Qry.SQLText=
        "SELECT range_id,airp_arv,point_id AS point_id_tlg, "
        "       first_xname,last_xname,first_yname,last_yname,rem_code, "
        "       crs_pax_id, "
        "       surname, name, pers_type "
        "FROM tlg_binding, tlg_comp_layers, crs_pax "
        "WHERE tlg_binding.point_id_tlg=tlg_comp_layers.point_id AND "
        "      tlg_comp_layers.crs_pax_id=crs_pax.pax_id(+) AND "
        "      tlg_binding.point_id_spp=:point_id_spp AND layer_type=:layer_type "
        "ORDER BY point_id_tlg, airp_arv, crs_pax_id";
      Qry.CreateVariable("point_id_spp",otInteger,point_id_spp);
    }
    else return;
  };
  Qry.CreateVariable("layer_type",otString,EncodeCompLayerType(layer_type));
  Qry.Execute();
  bool UsePriorContext=false;
  vector< pair<int, TSeatRange> > ranges_for_sync;
  for(;!Qry.Eof;)
  {
    pair<int, TSeatRange> range(Qry.FieldAsInteger("range_id"),
                                TSeatRange(TSeat(Qry.FieldAsString("first_yname"),
                                                 Qry.FieldAsString("first_xname")),
                                           TSeat(Qry.FieldAsString("last_yname"),
                                                 Qry.FieldAsString("last_xname")),
                                           Qry.FieldAsString("rem_code")));
    ranges_for_sync.push_back( range );
    int curr_point_id_tlg=Qry.FieldAsInteger("point_id_tlg");
    string airp_arv=Qry.FieldAsString("airp_arv");
    int crs_pax_id=(Qry.FieldIsNULL("crs_pax_id")?NoExists:Qry.FieldAsInteger("crs_pax_id"));
    string crs_pax_name=GetPaxName(Qry.FieldAsString("surname"),
                                   Qry.FieldAsString("name"));
    string pers_type = Qry.FieldAsString("pers_type");
    Qry.Next();

    int next_point_id_tlg=NoExists;
    string next_airp_arv;
    int next_crs_pax_id=NoExists;
    if (!Qry.Eof)
    {
      next_point_id_tlg=Qry.FieldAsInteger("point_id_tlg");
      next_airp_arv=Qry.FieldAsString("airp_arv");
      next_crs_pax_id=(Qry.FieldIsNULL("crs_pax_id")?NoExists:Qry.FieldAsInteger("crs_pax_id"));
    };

    if (Qry.Eof ||
        curr_point_id_tlg!=next_point_id_tlg ||
        airp_arv!=next_airp_arv ||
        crs_pax_id==NoExists || next_crs_pax_id==NoExists || crs_pax_id!=next_crs_pax_id)

    {
      InsertTripSeatRanges(ranges_for_sync,
                           curr_point_id_tlg,
                           point_id_spp,
                           airp_arv,
                           layer_type,
                           crs_pax_id,
                           crs_pax_name,
                           pers_type,
                           UsePriorContext,
                           point_ids_spp);
      ranges_for_sync.clear();
      UsePriorContext=true;
    };
  };
};

void DeleteTripCompLayers(int point_id_tlg,
                          int point_id_spp,
                          TCompLayerType layer_type,
                          TPointIdsForCheck &point_ids_spp) //вектор point_id_spp по которым были изменения
{
  if (!IsTlgCompLayer(layer_type)) return;

  TQuery Qry(&OraSession);
  Qry.Clear();
  if (point_id_tlg!=NoExists)
  {
    ostringstream sql;
    sql << "SELECT tlg_comp_layers.range_id, "
           "       tlg_comp_layers.crs_pax_id, "
           "       surname, name, pers_type "
           "FROM trip_comp_layers, tlg_comp_layers, crs_pax "
           "WHERE trip_comp_layers.range_id=tlg_comp_layers.range_id AND "
           "      tlg_comp_layers.crs_pax_id=crs_pax.pax_id(+) AND "
           "      tlg_comp_layers.point_id=:point_id_tlg AND "
           "      tlg_comp_layers.layer_type=:layer_type ";
    if (point_id_spp!=NoExists)
    {
      sql << "AND trip_comp_layers.point_id=:point_id_spp ";
      Qry.CreateVariable("point_id_spp",otInteger,point_id_spp);
    };
    sql << "ORDER BY range_id,crs_pax_id";

    Qry.SQLText=sql.str().c_str();
    Qry.CreateVariable("point_id_tlg",otInteger,point_id_tlg);
  }
  else
  {
    if (point_id_spp!=NoExists)
    {
      Qry.SQLText=
        "SELECT trip_comp_layers.range_id, "
        "       trip_comp_layers.crs_pax_id, "
        "       surname, name, pers_type "
        "FROM trip_comp_layers, crs_pax "
        "WHERE trip_comp_layers.crs_pax_id=crs_pax.pax_id(+) AND "
        "      trip_comp_layers.point_id=:point_id_spp AND "
        "      trip_comp_layers.layer_type=:layer_type "
        "ORDER BY range_id,crs_pax_id";
      Qry.CreateVariable("point_id_spp",otInteger,point_id_spp);
    }
    else return;
  };
  Qry.CreateVariable("layer_type",otString,EncodeCompLayerType(layer_type));
  Qry.Execute();
  SeatRangeIds ranges_for_sync;
  int prior_range_id=NoExists;
  for(;!Qry.Eof;)
  {
    int range_id=Qry.FieldAsInteger("range_id");
    if (prior_range_id==NoExists || prior_range_id!=range_id)
    {
      prior_range_id=range_id;
      ranges_for_sync.insert( range_id );
    };
    int crs_pax_id=(Qry.FieldIsNULL("crs_pax_id")?NoExists:Qry.FieldAsInteger("crs_pax_id"));
    string crs_pax_name=GetPaxName(Qry.FieldAsString("surname"),
                                   Qry.FieldAsString("name"));
    string pers_type = Qry.FieldAsString("pers_type");
    Qry.Next();

    int next_crs_pax_id=NoExists;
    if (!Qry.Eof)
    {
      next_crs_pax_id=(Qry.FieldIsNULL("crs_pax_id")?NoExists:Qry.FieldAsInteger("crs_pax_id"));
    };

    if (Qry.Eof ||
        crs_pax_id==NoExists || next_crs_pax_id==NoExists || crs_pax_id!=next_crs_pax_id)

    {
      DeleteTripSeatRanges(ranges_for_sync,
                           point_id_spp,
                           crs_pax_id,
                           crs_pax_name,
                           pers_type,
                           point_ids_spp);
      ranges_for_sync.clear();
      prior_range_id=NoExists; //можно этого было и не делать
    };
  };
};

void InsertTripCompLayers(int point_id_tlg,
                          int point_id_spp,
                          TPointIdsForCheck &point_ids_spp) //вектор point_id_spp по которым были изменения
{
  for(int layer=0;layer<(int)cltTypeNum;layer++)
    if (IsTlgCompLayer((TCompLayerType)layer)) {
      InsertTripCompLayers(point_id_tlg, point_id_spp, (TCompLayerType)layer, point_ids_spp);
    }
}


void DeleteTripCompLayers(int point_id_tlg,
                          int point_id_spp,
                          TPointIdsForCheck &point_ids_spp) //вектор point_id_spp по которым были изменения
{
  for(int layer=0;layer<(int)cltTypeNum;layer++)
    if (IsTlgCompLayer((TCompLayerType)layer)) {
      DeleteTripCompLayers(point_id_tlg, point_id_spp, (TCompLayerType)layer, point_ids_spp);
    }
}

void SyncTripCompLayers(int point_id_tlg,
                        int point_id_spp,
                        TPointIdsForCheck &point_ids_spp) //вектор point_id_spp по которым были изменения
{
  for(int layer=0;layer<(int)cltTypeNum;layer++)
    if (IsTlgCompLayer((TCompLayerType)layer)) {
      SyncTripCompLayers(point_id_tlg, point_id_spp, (TCompLayerType)layer, point_ids_spp);
    }
}

TCompLayerType GetSeatRemLayer(const string &airline_mark, const string &seat_rem)
{
  static bool rem_layers_init=false;
  static map< pair<string,string>, TCompLayerType> rem_layers;

  if (!rem_layers_init)
  {
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText="SELECT rem_code, airline_mark, layer_type FROM seat_rem_layers";
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      TCompLayerType layer_type=DecodeCompLayerType(Qry.FieldAsString("layer_type"));
      if (layer_type==cltUnknown) continue;
      rem_layers[ make_pair(Qry.FieldAsString("rem_code"), Qry.FieldAsString("airline_mark")) ]=layer_type;
    };
    rem_layers_init=true;
  };

  map< pair<string,string>, TCompLayerType>::const_iterator i;
  i=rem_layers.find( make_pair(seat_rem, airline_mark) );
  if (i==rem_layers.end())
    i=rem_layers.find( make_pair(seat_rem, "") );
  if (i==rem_layers.end())
    return cltUnknown;
  else
    return i->second;
};

bool lessSeatRemPriority(const pair<string, int> &item1,const pair<string, int> &item2)
{
  return item1.second<item2.second;
};

void GetSeatRemPriority(const string &airline_mark, TSeatRemPriority &rems)
{
  rems.clear();
  int priority=100; //самый маленький приоритет
  rems.push_back(make_pair("EXST",priority));
  rems.push_back(make_pair("GPST",priority));
  rems.push_back(make_pair("NSST",priority));
  rems.push_back(make_pair("NSSA",priority));
  rems.push_back(make_pair("NSSB",priority));
  rems.push_back(make_pair("NSSW",priority));
  rems.push_back(make_pair("SMST",priority));
  rems.push_back(make_pair("SMSA",priority));
  rems.push_back(make_pair("SMSB",priority));
  rems.push_back(make_pair("SMSW",priority));
  rems.push_back(make_pair("SEAT",priority));
  rems.push_back(make_pair("RQST",priority));
  for(TSeatRemPriority::iterator r=rems.begin();r!=rems.end();r++)
  {
    TCompLayerType rem_layer=GetSeatRemLayer(airline_mark, r->first);
    if (rem_layer==cltUnknown) continue;
    try
    {
      const TCompLayerTypesRow &row=(const TCompLayerTypesRow&)base_tables.get("comp_layer_types")
                                                                          .get_row("code",EncodeCompLayerType(rem_layer));
      r->second=row.priority;
    }
    catch(EBaseTableError) {};
  };
  //сортируем
  stable_sort(rems.begin(),rems.end(),lessSeatRemPriority);
};

void check_layer_change(const TPointIdsForCheck &point_ids_spp,
                        const std::string& whence)
{
  std::set<int> paxs_external_logged;
  check_layer_change( point_ids_spp, paxs_external_logged, whence );
}

void check_layer_change(const TPointIdsForCheck &point_ids_spp,
                        const std::set<int> &paxs_external_logged,
                        const std::string& whence)
{
  std::vector<int> points_tranzit_check_wait_alarm;

  //убираем повторения
  for(TPointIdsForCheck::const_iterator i=point_ids_spp.begin();i!=point_ids_spp.end();i++)
  {
    if ( SALONS2::isBaseLayer( i->second, false ) ||
         SALONS2::isUserProtectLayer( i->second ) ) {
      continue;
    }
    if ( find( points_tranzit_check_wait_alarm.begin(),
               points_tranzit_check_wait_alarm.end(),
               i->first ) == points_tranzit_check_wait_alarm.end() ) {
      points_tranzit_check_wait_alarm.push_back( i->first );
    }
  }
  SALONS2::check_waitlist_alarm_on_tranzit_routes( points_tranzit_check_wait_alarm, paxs_external_logged, whence );
}

