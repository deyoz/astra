#include <stdlib.h>
#include <string>
#include "tripinfo.h"
#include "stages.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "stl_utils.h"
#include "astra_misc.h"
#include "astra_date_time.h"
#include "base_tables.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "oralib.h"
#include "xml_unit.h"
#include "brd.h"
#include "checkin.h"
#include "prepreg.h"
#include "telegram.h"
#include "docs/docs_common.h"
#include "stat/stat_main.h"
#include "print.h"
#include "convert.h"
#include "term_version.h"
#include "salons.h"
#include "salonform.h"
#include "remarks.h"
#include "alarms.h"
#include "pers_weights.h"
#include "web_search.h"
#include "sopp.h"
#include "apps_interaction.h"
#include "brands.h"
#include "passenger.h"
#include "comp_props.h"
#include "ckin_search.h"
#include "payment_base.h"

#include <serverlib/testmode.h>

#define NICKNAME "VLAD"
#include <serverlib/slogger.h>

using namespace std;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;
using namespace ASTRA::date_time;

void TTripListInfo::ToXML(xmlNodePtr node)
{
  if (node==NULL) return;
};

void TTripListInfo::FromXML(xmlNodePtr node)
{
  if (node==NULL) return;
  Clear();
  xmlNodePtr node2=node->children;
  if (node2==NULL) return;
  date=NodeAsDateTimeFast("date",node2,NoExists);
  point_id=NodeAsIntegerFast("point_id",node2,NoExists);
  xmlNodePtr paramsNode;
  paramsNode=GetNodeFast("filter",node2);
  filter_from_xml=paramsNode!=NULL;
  filter.FromXML(paramsNode);
  paramsNode=GetNodeFast("view",node2);
  view_from_xml=paramsNode!=NULL;
  view.FromXML(paramsNode);
};

void TTripListFilter::ToXML(xmlNodePtr node)
{
  if (node==NULL) return;
};

void TTripListFilter::FromXML(xmlNodePtr node)
{
  if (node==NULL) return;
  Clear();
  xmlNodePtr node2=node->children;
  if (node2==NULL) return;
  airline=NodeAsStringFast("airline",node2,"");
  if (!NodeIsNULLFast("flt_no",node2,true))
    flt_no=NodeAsIntegerFast("flt_no",node2,NoExists);
  suffix=NodeAsStringFast("suffix",node2,"");
  airp_dep=NodeAsStringFast("airp_dep",node2,"");
  if (!NodeIsNULLFast("pr_cancel",node2,true))
    pr_cancel=NodeAsIntegerFast("pr_cancel",node2,(int)false)!=0;
  if (!NodeIsNULLFast("pr_takeoff",node2,true))
    pr_takeoff=NodeAsIntegerFast("pr_takeoff",node2,(int)false)!=0;
};

void TTripListView::ToXML(xmlNodePtr node)
{
  if (node==NULL) return;
};

void TTripListView::FromXML(xmlNodePtr node)
{
  if (node==NULL) return;
  Clear();
  xmlNodePtr node2=node->children;
  if (node2==NULL) return;
  if (!NodeIsNULLFast("use_colors",node2,true))
    use_colors=NodeAsIntegerFast("use_colors",node2,(int)false)!=0;
  if (!NodeIsNULLFast("codes_fmt",node2,true))
    codes_fmt=(TUserSettingType)NodeAsIntegerFast("codes_fmt",node2,(int)ustCodeNative);
};

void setSQLTripList( TQuery &Qry, const TTripListSQLFilter &filter )
{
  Qry.Clear();
  ostringstream sql;

  try
  {
    const TTripListSQLParams &params=dynamic_cast<const TTripListSQLParams&>(filter);

    sql <<
      "SELECT " + TTripInfo::selectedFields("points") + ", "
      "       points.move_id, "
      "       points.point_num "
      "FROM points ";
    if (!params.station.first.empty() && !params.station.second.empty())
      sql << ",trip_stations ";
    if (params.check_point_id!=NoExists)
    {
      sql << "WHERE points.point_id=:point_id ";
      Qry.CreateVariable( "point_id", otInteger, params.check_point_id);
    }
    else
    {
      if (params.first_date!=params.last_date)
      {
        sql << "WHERE points.time_out BETWEEN :first_date AND :last_date ";
        Qry.CreateVariable("last_date", otDate, params.last_date);
      }
      else
        sql << "WHERE points.time_out=:first_date ";
      Qry.CreateVariable("first_date", otDate, params.first_date);
    };

    sql << "AND points.pr_reg<>0 ";

    if (params.flt_no!=NoExists)
    {
      sql << "AND points.flt_no=:flt_no ";
      Qry.CreateVariable("flt_no", otInteger, params.flt_no);
    };
    if (!params.suffix.empty())
    {
      sql << "AND points.suffix=:suffix ";
      Qry.CreateVariable("suffix", otString, params.suffix);
    };
  }
  catch(bad_cast)
  {
    const TTripInfoSQLParams &params=dynamic_cast<const TTripInfoSQLParams&>(filter);

    sql <<
      "SELECT " + TTripInfo::selectedFields("points") + ", "
      "       points.bort, "
      "       points.park_out, "
      "       points.trip_type, "
      "       points.litera, "
      "       points.remark, "
      "       ckin.tranzitable(points.point_id) AS tranzitable, "
      "       ckin.get_pr_tranzit(points.point_id) AS pr_tranzit, "
      "       points.first_point "
      "FROM points ";
    if (!params.station.first.empty() && !params.station.second.empty())
      sql << ",trip_stations ";
    sql << "WHERE points.point_id=:point_id ";
    Qry.CreateVariable( "point_id", otInteger, params.point_id);
  };

  sql << "AND points.pr_reg<>0 ";

  if (!filter.pr_cancel)
    sql << "AND points.pr_del=0 ";
  else
    sql << "AND points.pr_del>=0 ";
  if (!filter.pr_takeoff)
    sql << "AND points.act_out IS NULL ";

  if (!filter.station.first.empty() && !filter.station.second.empty())
  {
    sql << "AND points.point_id=trip_stations.point_id "
           "AND trip_stations.desk= :desk AND trip_stations.work_mode=:work_mode ";
    Qry.CreateVariable( "desk", otString, filter.station.first );
    Qry.CreateVariable( "work_mode", otString, filter.station.second);
  };

  if (!filter.access.airlines().elems().empty())
  {
    if (filter.access.airlines().elems_permit())
      sql << "AND points.airline IN " << GetSQLEnum(filter.access.airlines().elems()) << " ";
    else
      sql << "AND points.airline NOT IN " << GetSQLEnum(filter.access.airlines().elems()) << " ";
  };

  if (!filter.access.airps().elems().empty())
  {
    if ( !filter.use_arrival_permit )
    {
      if (filter.access.airps().elems_permit())
        sql << "AND points.airp IN " << GetSQLEnum(filter.access.airps().elems()) << " ";
      else
        sql << "AND points.airp NOT IN " << GetSQLEnum(filter.access.airps().elems()) << " ";
    }
    else
    {
      if (filter.access.airps().elems_permit())
        sql << "AND (points.airp IN " << GetSQLEnum(filter.access.airps().elems()) << " OR "
            << "     ckin.next_airp(DECODE(points.pr_tranzit,0,points.point_id,points.first_point),points.point_num) IN "
            << GetSQLEnum(filter.access.airps().elems()) << ") ";
      else
        sql << "AND (points.airp NOT IN " << GetSQLEnum(filter.access.airps().elems()) << " OR "
            << "     ckin.next_airp(DECODE(points.pr_tranzit,0,points.point_id,points.first_point),points.point_num) NOT IN "
            << GetSQLEnum(filter.access.airps().elems()) << ") ";
    };
  };

  Qry.SQLText=sql.str().c_str();
};

TStage getFinalStage( TQuery &Qry, const int point_id, const TStage_Type stage_type )
{
  const char* sql="SELECT stage_id FROM trip_final_stages WHERE point_id=:point_id AND stage_type=:stage_type";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("point_id", otInteger);
    Qry.DeclareVariable("stage_type", otInteger);
  };
  Qry.SetVariable("point_id",point_id);
  Qry.SetVariable("stage_type",(int)stage_type);
  Qry.Execute();
  if (!Qry.Eof)
    return (TStage)Qry.FieldAsInteger("stage_id");
  else
    return sNoActive;
};

bool checkFinalStages( TQuery &Qry, const int point_id, const TTripListSQLFilter &filter)
{
  if (filter.final_stages.empty()) return true;

  const char* sql="SELECT stage_id FROM trip_final_stages WHERE point_id=:point_id AND stage_type=:stage_type";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("point_id", otInteger);
    Qry.DeclareVariable("stage_type", otInteger);
  };

  Qry.SetVariable("point_id",point_id);
  //䨫����� �� final_stages
  map< TStage_Type, vector<TStage> >::const_iterator iStage=filter.final_stages.begin();
  for(; iStage!=filter.final_stages.end(); iStage++)
  {
    Qry.SetVariable("stage_type",(int)iStage->first);
    Qry.Execute();
    if (!Qry.Eof &&
        find(iStage->second.begin(),iStage->second.end(),(TStage)Qry.FieldAsInteger("stage_id"))!=iStage->second.end()) break;
  };
  return iStage!=filter.final_stages.end();
};

struct TTripListColumn
{
  string value;
  int sort_order;
  string b_color, f_color;
};

class TTripListItem
{
  public:
    int point_id;
    TTripListColumn name;
    TTripListColumn date;
    TTripListColumn airp;
    //�������⥫쭮 ��� ���஢��
    TDateTime real_out_client;
    TDateTime real_out_client_trunk;
    int flt_no;
    string airline;
    string suffix;
    int move_id;
    int point_num;
    //梥�
    string b_color, f_color;
};

class TTripListOrder
{
  private:
    string order_type;
  public:
    TTripListOrder(const string &order)
    {
      order_type=order;
    };
    bool operator () (const TTripListItem &item1, const TTripListItem &item2) const
    {
      if (order_type=="name_sort_order" ||
          order_type=="date_sort_order" )
      {
        if (item1.real_out_client_trunk!=item2.real_out_client_trunk)
        return item1.real_out_client_trunk>item2.real_out_client_trunk;
      };

      if (order_type=="date_sort_order")
      {
        if (item1.real_out_client!=item2.real_out_client)
        return item1.real_out_client<item2.real_out_client;
      };

      if (order_type=="airp_sort_order")
      {
        if (item1.airp.value!=item2.airp.value)
        return item1.airp.value<item2.airp.value;
      };

      if (item1.flt_no!=item2.flt_no)
        return item1.flt_no<item2.flt_no;
      if (item1.airline!=item2.airline)
        return item1.airline<item2.airline;
      if (item1.suffix!=item2.suffix)
        return item1.suffix<item2.suffix;
      if (item1.move_id!=item2.move_id)
        return item1.move_id<item2.move_id;
      return item1.point_num<item2.point_num;
    };
};

void TTripListSQLFilter::set(void)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  access=reqInfo->user.access;

  if (reqInfo->screen.name=="BRDBUS.EXE" ||
      reqInfo->screen.name=="EXAM.EXE")
  {
    final_stages[stBoarding].push_back(sOpenBoarding);
  };
  if (reqInfo->screen.name=="AIR.EXE" ||
      reqInfo->screen.name=="DOCS.EXE" ||
      reqInfo->screen.name=="KASSA.EXE")
  {
    final_stages[stCheckIn].push_back(sOpenCheckIn);
    final_stages[stCheckIn].push_back(sCloseCheckIn);
    final_stages[stCheckIn].push_back(sCloseBoarding);
  };
  if (reqInfo->screen.name=="PREPREG.EXE")
  {
    final_stages[stCheckIn].push_back(sPrepCheckIn);
    final_stages[stCheckIn].push_back(sOpenCheckIn);
    final_stages[stCheckIn].push_back(sCloseCheckIn);
    final_stages[stCheckIn].push_back(sCloseBoarding);
  };

  pr_cancel=reqInfo->screen.name!="BRDBUS.EXE" &&
            reqInfo->screen.name!="EXAM.EXE" &&
            reqInfo->screen.name!="AIR.EXE" &&
            reqInfo->screen.name!="DOCS.EXE" &&
            reqInfo->screen.name!="KASSA.EXE" &&
            reqInfo->screen.name!="PREPREG.EXE" &&
            reqInfo->screen.name!="CENT.EXE";

  pr_takeoff=reqInfo->screen.name!="BRDBUS.EXE" &&
             reqInfo->screen.name!="EXAM.EXE" &&
             reqInfo->screen.name!="KASSA.EXE" &&
             reqInfo->screen.name!="CENT.EXE" &&
             !(reqInfo->screen.name=="AIR.EXE" &&
               !access.rights().permitted(320) &&
               !access.rights().permitted(330) &&
               !access.rights().permitted(335));

  if ((reqInfo->screen.name == "BRDBUS.EXE" || reqInfo->screen.name == "AIR.EXE") &&
      reqInfo->user.user_type==utAirport &&
      !access.rights().permitted(335))
  {
    station.first=reqInfo->desk.code;
    if (reqInfo->screen.name == "BRDBUS.EXE")
      station.second="�";
    else
      station.second="�";
  };

  use_arrival_permit=reqInfo->screen.name=="TLG.EXE";
};

void TTripInfoSQLParams::set(void)
{
  TTripListSQLFilter::set();
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->screen.name=="KASSA.EXE")
  {
    final_stages.clear();
    pr_cancel=true;
    pr_takeoff=true;
  };
};

/*******************************************************************************/
void TripsInterface::GetTripList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  bool advanced_trip_list=strcmp((const char*)reqNode->name, "GetAdvTripList")==0;

  TReqInfo *reqInfo = TReqInfo::Instance();
  //reqInfo->user.check_access( amRead );

  TTripListInfo listInfo;
  listInfo.FromXML(reqNode);

  TTripListSQLParams SQLfilter;
  SQLfilter.set();
  if (!listInfo.filter.airline.empty())
    SQLfilter.access.merge_airlines(TAccessElems<string>(listInfo.filter.airline, true));

  SQLfilter.flt_no=listInfo.filter.flt_no;
  SQLfilter.suffix=listInfo.filter.suffix;

  if (!listInfo.filter.airp_dep.empty())
    SQLfilter.access.merge_airps(TAccessElems<string>(listInfo.filter.airp_dep, true));

  if (listInfo.view.codes_fmt==ustCodeNative ||
      listInfo.view.codes_fmt==ustCodeInter ||
      listInfo.view.codes_fmt==ustCodeICAONative ||
      listInfo.view.codes_fmt==ustCodeICAOInter ||
      listInfo.view.codes_fmt==ustCodeMixed )
  {
    reqInfo->user.sets.ckin_airline=listInfo.view.codes_fmt;
    reqInfo->user.sets.ckin_airp=listInfo.view.codes_fmt;
    reqInfo->user.sets.ckin_craft=listInfo.view.codes_fmt;
    switch (listInfo.view.codes_fmt)
    {
      case ustCodeNative:
      case ustCodeICAONative:
        reqInfo->user.sets.ckin_suffix=ustEncNative;
        break;
      case ustCodeInter:
      case ustCodeICAOInter:
        reqInfo->user.sets.ckin_suffix=ustEncLatin;
        break;
      case ustCodeMixed:
        reqInfo->user.sets.ckin_suffix=ustEncMixed;
        break;
      default:;
    };
  };

  vector<TTripListItem> list;
  if (!SQLfilter.access.totally_not_permitted())
  {
    TDateTime utc_date=NowUTC();
    //���᫨� client_date
    TDateTime client_date=UTCToClient(utc_date,reqInfo->desk.tz_region);
    modf(utc_date, &utc_date); //���㣫塞 �� ���
    modf(client_date, &client_date); //���㣫塞 �� ���

    int shift_down_additional=-1;
    int shift_up_additional=1;
    if (reqInfo->user.sets.time==ustTimeUTC)
    {
      shift_down_additional=0;
      shift_up_additional=0;
    };

    vector< pair<int,int> > shifts;
    vector< pair<TDateTime, TDateTime> > ranges;

    TQuery Qry( &OraSession );
    TQuery StagesQry( &OraSession );
    if (advanced_trip_list)
    {
      if (listInfo.date==NoExists)
        listInfo.date=client_date;

      //�஢�ਬ �� ३� �������� � ��࠭�� ��⪨
      if (listInfo.point_id!=NoExists && listInfo.point_id!=-1)
      {
        SQLfilter.check_point_id=listInfo.point_id;
        setSQLTripList( Qry, SQLfilter );
        Qry.Execute();
        if (!Qry.Eof)
        {
          int point_id=Qry.FieldAsInteger("point_id");
          TTripInfo info(Qry);
          if (!(!checkFinalStages(StagesQry, point_id, SQLfilter) ||
                (!listInfo.filter.airp_dep.empty() && listInfo.filter.airp_dep!=info.airp)))
          {
            //३� ���室��
            TDateTime scd_out_client;
            info.get_client_dates(scd_out_client,listInfo.date,false);
          }
        };
      };
      modf(listInfo.date, &listInfo.date);
      shifts.push_back( make_pair((int)(listInfo.date-client_date)+shift_down_additional,
                                  (int)(listInfo.date-client_date)+shift_up_additional+1) );
      ranges.push_back( make_pair(listInfo.date, listInfo.date+1) );
    }
    else
    {
      int shift_down_default=-1;
      int shift_up_default=1;
      if (shifts.empty())
        shifts.push_back( make_pair(shift_down_default+shift_down_additional,
                                    shift_up_default+shift_up_additional+1) );
      if (ranges.empty())
        ranges.push_back( make_pair(client_date+shift_down_default,client_date+shift_up_default+1) );
    };

    for(vector< pair<int,int> >::const_iterator iShift=shifts.begin(); iShift!=shifts.end(); iShift++)
    {
      ProgTrace(TRACE5, "iShift=[%d, %d)", iShift->first, iShift->second);
      for(int i=iShift->first; i<iShift->second; i++)
      {
        bool use_single_day= ((!SQLfilter.access.airlines().elems().empty() && SQLfilter.access.airlines().elems_permit()) ||
                              (!SQLfilter.access.airps().elems().empty() && SQLfilter.access.airps().elems_permit() && !SQLfilter.use_arrival_permit)) &&
                             iShift->second-1-iShift->first<=5;

        if (use_single_day)
        {
          SQLfilter.first_date=utc_date+i;
          SQLfilter.last_date=utc_date+i;
        }
        else
        {
          SQLfilter.first_date=utc_date+iShift->first;
          SQLfilter.last_date=utc_date+iShift->second-1;
        };

        //ProgTrace(TRACE5, "first_date=%s last_date=%s",
        //                  DateTimeToStr(SQLfilter.first_date,"dd.mm.yy hh:nn:ss").c_str(),
        //                  DateTimeToStr(SQLfilter.last_date,"dd.mm.yy hh:nn:ss").c_str() );

        SQLfilter.check_point_id=NoExists;
        setSQLTripList( Qry, SQLfilter );

        //ProgTrace(TRACE5, "TripList SQL=%s", Qry.SQLText.SQLText());
        Qry.Execute();
        for(;!Qry.Eof;Qry.Next())
        {
          int point_id=Qry.FieldAsInteger("point_id");

          if (!checkFinalStages(StagesQry, point_id, SQLfilter)) {
              if(!inTestMode()) continue; //�ய�᪠��, ३� �� ���室�� �� final_stages
          }

          TTripInfo info(Qry);

          try
          {
            TTripListItem listItem;

            if (!listInfo.filter.airp_dep.empty() && listInfo.filter.airp_dep!=info.airp) continue;  //�ய�᪠��, ३� �� ���室�� �� airp_dep
            TDateTime scd_out_client;
            info.get_client_dates(scd_out_client,listItem.real_out_client,false);
            modf(listItem.real_out_client,&listItem.real_out_client_trunk);
            //�஢�ਬ, �� �������� � �������� ���, ��襤�� � ������
            vector< pair<TDateTime, TDateTime> >::const_iterator iRange=ranges.begin();
            for(;iRange!=ranges.end();iRange++)
              if (listItem.real_out_client>=iRange->first &&
                  listItem.real_out_client<iRange->second) break; //३� ���室�� �� real_out_client
            if (iRange==ranges.end()) continue; //�ய�᪠��, ३� �� ���室�� �� real_out_client

            listItem.point_id=point_id;
            listItem.flt_no=info.flt_no;
            listItem.airline=ElemIdToElemCtxt(ecCkin, etAirline, info.airline, info.airline_fmt);
            listItem.suffix=ElemIdToElemCtxt(ecCkin, etSuffix, info.suffix, info.suffix_fmt);
            listItem.move_id=Qry.FieldAsInteger("move_id");
            listItem.point_num=Qry.FieldAsInteger("point_num");
            if (advanced_trip_list)
            {
              ostringstream trip;
              trip << listItem.airline
                   << setw(3) << setfill('0') << listItem.flt_no
                   << listItem.suffix;
              listItem.name.value=trip.str();
              listItem.date.value=GetTripDate(info,"",true);
              listItem.airp.value=ElemIdToElemCtxt(ecCkin, etAirp, info.airp, info.airp_fmt);
            }
            else
              listItem.name.value=GetTripName(info,ecCkin,reqInfo->screen.name=="TLG.EXE",true);
            //���᪠
            if (listInfo.view.use_colors)
            {
              if (Qry.FieldAsInteger("pr_del")!=0)
              {
                listItem.b_color="clMaroon";
                listItem.f_color="clWhite";
              }
              else
                if (!Qry.FieldIsNULL("act_out"))
                {
                  listItem.b_color="$00800000";
                  listItem.f_color="clWhite";
                }
                else
                {
                  //������ ��⨢����
                  TStage stage=getFinalStage(StagesQry, point_id, stCheckIn);
                  if (stage==sNoActive)
                    listItem.b_color="clSilver";
                  else
                    listItem.b_color="$0000D200";

                  if (!Qry.FieldIsNULL("scd_out") && !Qry.FieldIsNULL("est_out") &&
                      Qry.FieldAsDateTime("scd_out")!=Qry.FieldAsDateTime("est_out"))
                  {
                    //����প�
                    listItem.date.b_color="$0054FAF5";
                  };
                };
            };
            list.push_back(listItem);
          }
          catch(AstraLocale::UserException &E)
          {
            AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
          };
        };
        if (!use_single_day) break;
      };
    };

    if (advanced_trip_list)
    {
      int sort_order;
      //����㥬 �� name
      sort(list.begin(),list.end(),TTripListOrder("name_sort_order"));
      sort_order=0;
      for(vector<TTripListItem>::iterator i=list.begin();i!=list.end();i++,sort_order++)
      {
        i->name.sort_order=sort_order;
      };

      //����㥬 �� date
      sort(list.begin(),list.end(),TTripListOrder("date_sort_order"));
      sort_order=0;
      for(vector<TTripListItem>::iterator i=list.begin();i!=list.end();i++,sort_order++)
      {
        i->date.sort_order=sort_order;
      };

      //����㥬 �� airp
      sort(list.begin(),list.end(),TTripListOrder("airp_sort_order"));
      sort_order=0;
      for(vector<TTripListItem>::iterator i=list.begin();i!=list.end();i++,sort_order++)
      {
        i->airp.sort_order=sort_order;
      };
    }
    else
    {
      sort(list.begin(),list.end(),TTripListOrder("name_sort_order"));
    };
  };


  //�ନ�㥬 �⢥�
  xmlNodePtr dataNode;
  if (advanced_trip_list)
  {
    dataNode = resNode;
    //��襬 listInfo
    NewTextChild(resNode, "date", DateTimeToStr(listInfo.date)); //���ࠧ㬥���� �� �� ����� ���� NoExists

    if (!listInfo.filter_from_xml)
    {
      //�����뢠��
      xmlNodePtr paramsNode=NewTextChild(resNode,"filter");
      SetProp(NewTextChild(paramsNode,"airline",listInfo.filter.airline),"editable",(int)true);
      if (listInfo.filter.flt_no!=NoExists)
        SetProp(NewTextChild(paramsNode,"flt_no",listInfo.filter.flt_no),"editable",(int)true);
      else
        SetProp(NewTextChild(paramsNode,"flt_no"),"editable",(int)true);
      SetProp(NewTextChild(paramsNode,"airp_dep",listInfo.filter.airp_dep),"editable",(int)true);
      SetProp(NewTextChild(paramsNode,"pr_cancel",(int)SQLfilter.pr_cancel),"editable",(int)false);
      SetProp(NewTextChild(paramsNode,"pr_takeoff",(int)SQLfilter.pr_takeoff),"editable",(int)false);
    };
    if (!listInfo.view_from_xml)
    {
      //�����뢠��
      xmlNodePtr paramsNode=NewTextChild(resNode,"view");
      SetProp(NewTextChild(paramsNode,"use_colors",(int)listInfo.view.use_colors),"editable",(int)true);
      SetProp(NewTextChild(paramsNode,"codes_fmt",(int)listInfo.view.codes_fmt),"editable",(int)true);
    };
  }
  else
    dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr tripsNode = NewTextChild( dataNode, "trips" );

  if (reqInfo->screen.name=="TLG.EXE")
  {
    xmlNodePtr tripNode = NewTextChild( tripsNode, "trip" );
    if (advanced_trip_list)
    {
      NewTextChild( tripNode, "point_id", -1 );
      NewTextChild( tripNode, "name", AstraLocale::getLocaleText("���ਢ易���") );
      NewTextChild( tripNode, "name_sort_order", -1 );
    }
    else
    {
      NewTextChild( tripNode, "trip_id", -1 );
      NewTextChild( tripNode, "str", AstraLocale::getLocaleText("���ਢ易���") );
    };
  };

  for(vector<TTripListItem>::iterator i=list.begin();i!=list.end();i++)
  {
    xmlNodePtr tripNode = NewTextChild( tripsNode, "trip" );
    if (advanced_trip_list)
    {
      NewTextChild( tripNode, "point_id", i->point_id );
      NewTextChild( tripNode, "name", i->name.value );
      NewTextChild( tripNode, "date", i->date.value, "" );
      NewTextChild( tripNode, "airp", i->airp.value, "" );
      NewTextChild( tripNode, "name_sort_order", i->name.sort_order );
      NewTextChild( tripNode, "date_sort_order", i->date.sort_order, i->name.sort_order );
      NewTextChild( tripNode, "airp_sort_order", i->airp.sort_order, i->name.sort_order );

      NewTextChild( tripNode, "b_color", i->b_color, "");
      NewTextChild( tripNode, "f_color", i->f_color, "");
      NewTextChild( tripNode, "name_b_color", i->name.b_color, "");
      NewTextChild( tripNode, "name_f_color", i->name.f_color, "");
      NewTextChild( tripNode, "date_b_color", i->date.b_color, "");
      NewTextChild( tripNode, "date_f_color", i->date.f_color, "");
      NewTextChild( tripNode, "airp_b_color", i->airp.b_color, "");
      NewTextChild( tripNode, "airp_f_color", i->airp.f_color, "");
    }
    else
    {
      NewTextChild( tripNode, "trip_id", i->point_id );
      NewTextChild( tripNode, "str", i->name.value );
    };
  };
};

static bool skipSeg(xmlNodePtr segNode)
{
    xmlNodePtr pointIdNode = findNode(segNode, "point_id");
    if(pointIdNode != NULL && NodeAsInteger(pointIdNode) < 0) {
        return true;
    }
    return false;
}

void TripsInterface::GetTripInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr dataNode=NewTextChild( resNode, "data" );
  if (GetNode("refresh_type",reqNode)!=NULL)
    NewTextChild( dataNode, "refresh_type", NodeAsInteger("refresh_type",reqNode) );
  int first_point_id = NodeAsInteger( "point_id", reqNode );
  GetSegInfo(reqNode, resNode, dataNode, first_point_id);
  //��ࠡ�⪠ �����ᥣ���⭮�� �����
  xmlNodePtr node=GetNode("segments",reqNode);
  if (node!=NULL)
  {
    xmlNodePtr segsNode=NewTextChild(dataNode,"segments");
    for(node=node->children;node!=NULL;node=node->next)
      if(!skipSeg(node)) {
        GetSegInfo(node, NULL, NewTextChild(segsNode,"segment"), first_point_id);
      }
  };
  if(not TReqInfo::Instance()->desk.compatible(PROFILE_REFACT_VERSION))
      TProfiledRights(first_point_id).toXML(dataNode);
};

void TripsInterface::PectabsResponse(int point_id, xmlNodePtr reqNode, xmlNodePtr dataNode)
{
  xmlNodePtr node;
  node=GetNode( "tripBPpectabs", reqNode );
  if (node!=NULL)
  {
      string dev_model = NodeAsString("dev_model", node);
      string fmt_type = NodeAsString("fmt_type", node);
      GetTripBPPectabs( point_id, TDevOper::PrnBP, dev_model, fmt_type, dataNode );
  };
  node=GetNode( "tripBTpectabs", reqNode );
  if (node!=NULL)
  {
      string dev_model = NodeAsString("dev_model", node);
      string fmt_type = NodeAsString("fmt_type", node);
      GetTripBTPectabs( point_id, dev_model, fmt_type, dataNode );
  };
  node=GetNode( "tripBIpectabs", reqNode );
  if (node!=NULL)
  {
      string dev_model = NodeAsString("dev_model", node);
      string fmt_type = NodeAsString("fmt_type", node);
      GetTripBPPectabs( point_id, TDevOper::PrnBI, dev_model, fmt_type, dataNode );
  };
};

void TripsInterface::GetSegInfo(xmlNodePtr reqNode, xmlNodePtr resNode, xmlNodePtr dataNode,
                                bool first_point_id)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  int point_id = NodeAsInteger( "point_id", reqNode );
  NewTextChild( dataNode, "point_id", point_id );

  if ( GetNode( "tripheader", reqNode ) )
    if ( !readTripHeader( point_id, dataNode ) )
      AstraLocale::showErrorMessage( "MSG.FLT.NOT_AVAILABLE" );


  if (reqInfo->screen.name == "BRDBUS.EXE" ||
      reqInfo->screen.name == "EXAM.EXE" )
  {
      if ( GetNode( "counters", reqNode ) )
      {
          TRptParams rpt_params(reqInfo->desk.lang);
          BrdInterface::readTripCounters( point_id, rpt_params, dataNode, rtUnknown, "" );
      };
      if ( GetNode( "tripdata", reqNode ) )
          BrdInterface::readTripData( point_id, dataNode );
      if ( GetNode( "paxdata", reqNode ) && resNode!=NULL ) {
          BrdInterface::GetPax(reqNode,resNode);
      }
  };
  if (reqInfo->screen.name == "AIR.EXE")
  {
    if ( GetNode( "tripcounters", reqNode ) )
      CheckInInterface::readTripCounters( point_id, dataNode );
    if ( GetNode( "tripdata", reqNode ) )
      CheckInInterface::readTripData( point_id, first_point_id, dataNode );
    if ( GetNode( "tripsets", reqNode ) )
      CheckInInterface::readTripSets( point_id, dataNode );

    PectabsResponse(point_id, reqNode, dataNode);
  };
  if (reqInfo->screen.name == "CENT.EXE")
  {
    if ( GetNode( "tripcounters", reqNode ) ) {
      readPaxLoad( point_id, reqNode, dataNode ); //djek
    }
  };
  if (reqInfo->screen.name == "PREPREG.EXE")
  {
    if ( GetNode( "tripcounters", reqNode ) )
      PrepRegInterface::readTripCounters( point_id, dataNode );
    if ( GetNode( "crsdata", reqNode ) )
      PrepRegInterface::readTripData( point_id, dataNode );
  };
  if (reqInfo->screen.name == "TLG.EXE" ||
      reqInfo->screen.name == "DOCS.EXE")
  {
    if ( GetNode( "tripdata", reqNode ) && point_id != -1 )
      TelegramInterface::readTripData( point_id, dataNode );
    if ( GetNode( "ckin_zones", reqNode ) )
        GetZoneList(point_id, dataNode);
  };
};

void TripsInterface::readOperFltHeader( const TTripInfo &info, xmlNodePtr node )
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  if ( reqInfo->screen.name == "AIR.EXE" )
  {
    string flight=GetTripName(info,ecCkin,true,false);
    NewTextChild( node, "flight", flight );
    NewTextChild( node, "flight_short", info.flight_view(), flight );
  };

  NewTextChild( node, "airline", info.airline );

  const TAirlinesRow &row = (const TAirlinesRow&)base_tables.get("airlines").get_row("code",info.airline);
  NewTextChild( node, "aircode", row.aircode );

  NewTextChild( node, "flt_no", info.flt_no );
  NewTextChild( node, "suffix", info.suffix );
  NewTextChild( node, "airp", info.airp );
  if ( reqInfo->screen.name == "AIR.EXE" ||
       reqInfo->screen.name == "KASSA.EXE" )
  {
    //��������! �����쭠� ��� ����
    NewTextChild( node, "scd_out_local", DateTimeToStr(UTCToLocal(info.scd_out,AirpTZRegion(info.airp))) );
  };
};

bool TripsInterface::readTripHeader( int point_id, xmlNodePtr dataNode )
{
  ProgTrace(TRACE5, "TripsInterface::readTripHeader" );
  TReqInfo *reqInfo = TReqInfo::Instance();
  //reqInfo->user.check_access( amRead );

  if (reqInfo->screen.name=="TLG.EXE" && point_id==-1)
  {
    xmlNodePtr node = NewTextChild( dataNode, "tripheader" );
    NewTextChild( node, "point_id", -1 );
    return true;
  };

  if (reqInfo->user.access.totally_not_permitted())
    return false;

  TTripInfoSQLParams filter;
  filter.set();
  filter.point_id=point_id;

  TQuery Qry( &OraSession );
  TQuery StagesQry( &OraSession );
  setSQLTripList( Qry, filter );
  //ProgTrace(TRACE5, "TripInfo SQL=%s", Qry.SQLText.SQLText());
  Qry.Execute();
  if (Qry.Eof) return false;
  if (!checkFinalStages(StagesQry, point_id, filter)) return false;

  TTripInfo info(Qry);
  xmlNodePtr node = NewTextChild( dataNode, "tripheader" );
  NewTextChild( node, "point_id", Qry.FieldAsInteger( "point_id" ) );

  readOperFltHeader(info,node);

  const string &tz_region=AirpTZRegion(info.airp);
  TDateTime scd_out_client,
            act_out_client,
            real_out_client;

  scd_out_client= UTCToClient(info.scd_out,tz_region);
  real_out_client=UTCToClient(info.act_est_scd_out(),tz_region);

  if ( reqInfo->screen.name == "TLG.EXE" )
  {
    ostringstream trip;
    trip << ElemIdToCodeNative(etAirline,info.airline)
         << setw(3) << setfill('0') << info.flt_no
         << ElemIdToCodeNative(etSuffix,info.suffix);
    NewTextChild( node, "flight", trip.str() );
  };

  NewTextChild( node, "scd_out", DateTimeToStr(scd_out_client) );
  NewTextChild( node, "real_out", DateTimeToStr(real_out_client,"hh:nn") );
  if (!Qry.FieldIsNULL("act_out"))
  {
    act_out_client= UTCToClient(Qry.FieldAsDateTime("act_out"),tz_region);
    NewTextChild( node, "act_out", DateTimeToStr(act_out_client) );
  }
  else
  {
    act_out_client= ASTRA::NoExists;
    NewTextChild( node, "act_out" );
  };

  NewTextChild( node, "craft", ElemIdToElemCtxt(ecCkin,etCraft, Qry.FieldAsString( "craft" ), (TElemFmt)Qry.FieldAsInteger( "craft_fmt" )) );
  NewTextChild( node, "bort", Qry.FieldAsString( "bort" ) );
  NewTextChild( node, "park", Qry.FieldAsString( "park_out" ) );
  NewTextChild( node, "classes", TCFG(point_id).str() );
  string route=GetRouteAfterStr(NoExists, point_id, trtWithCurrent, trtNotCancelled);
  NewTextChild( node, "route", route );
  NewTextChild( node, "places", route );
  NewTextChild( node, "trip_type", ElemIdToCodeNative(etTripType,Qry.FieldAsString( "trip_type" )) );
  NewTextChild( node, "litera", Qry.FieldAsString( "litera" ) );
  NewTextChild( node, "remark", Qry.FieldAsString( "remark" ) );
  NewTextChild( node, "pr_tranzit", (int)Qry.FieldAsInteger( "pr_tranzit" )!=0 );

  //trip �㦥� ��� ChangeTrip ������:
  NewTextChild( node, "trip", GetTripName(info,ecCkin,reqInfo->screen.name=="TLG.EXE",true)); //ecCkin? !!vlad

  TTripStages tripStages( point_id );
  TStagesRules *stagesRules = TStagesRules::Instance();

  //������ ३ᮢ
  string status;
  if ( reqInfo->screen.name == "BRDBUS.EXE" ||
       reqInfo->screen.name == "EXAM.EXE")
  {
    status = stagesRules->status_view( stBoarding, tripStages.getStage( stBoarding ) );
  };
  if ( reqInfo->screen.name == "AIR.EXE" ||
       reqInfo->screen.name == "PREPREG.EXE" )
  {
    status = stagesRules->status_view( stCheckIn, tripStages.getStage( stCheckIn ) );
  };
  if ( reqInfo->screen.name == "KASSA.EXE" ||
       reqInfo->screen.name == "CENT.EXE" )
  {
    TStage ckin_stage =  tripStages.getStage( stCheckIn );
    TStage craft_stage = tripStages.getStage( stCraft );
    if ( craft_stage == sRemovalGangWay || craft_stage == sTakeoff )
      status = stagesRules->status_view( stCraft, craft_stage );
    else
      status = stagesRules->status_view( stCheckIn, ckin_stage );
  };
  if ( reqInfo->screen.name == "DOCS.EXE" )
  {
    if (act_out_client==ASTRA::NoExists)
      status = stagesRules->status_view( stCheckIn, tripStages.getStage( stCheckIn ) );
    else
      status = stagesRules->status_view( stCheckIn, sTakeoff );
  };
  if ( reqInfo->screen.name == "TLG.EXE" )
  {
    if (act_out_client==ASTRA::NoExists)
      status = stagesRules->status_view( stCraft, tripStages.getStage( stCraft ) );
    else
      status = stagesRules->status_view( stCraft, sTakeoff );
  };
  NewTextChild( node, "status", status );

  //��直� �������⥫�� �६���
  TDateTime stage_time=0;
  if ( reqInfo->screen.name == "BRDBUS.EXE" ||
       reqInfo->screen.name == "EXAM.EXE" ||
       reqInfo->screen.name == "CENT.EXE" ||
       reqInfo->screen.name == "KASSA.EXE" )
    stage_time = UTCToClient( tripStages.time( sCloseBoarding ), tz_region );
  if ( reqInfo->screen.name == "AIR.EXE" ||
       reqInfo->screen.name == "PREPREG.EXE" )
    stage_time = UTCToClient( tripStages.time( sCloseCheckIn ), tz_region );
  if ( reqInfo->screen.name == "DOCS.EXE" )
    stage_time = UTCToClient( tripStages.time( sRemovalGangWay ), tz_region );
  if (stage_time!=0)
    NewTextChild( node, "stage_time", DateTimeToStr(stage_time,"hh:nn") );

  if (reqInfo->screen.name == "CENT.EXE" ||
      reqInfo->screen.name == "KASSA.EXE" )
  {
    NewTextChild( node, "craft_stage", tripStages.getStage( stCraft ) );
  };

/* ����ᨫ� ���� ࠡ��� � ⠡�� 24.10.1018 Feature #33436
 *  if (reqInfo->screen.name == "AIR.EXE" && reqInfo->desk.airp == "���")
  {
    TQuery Qryh( &OraSession );
    Qryh.Clear();
    Qryh.SQLText=
      "SELECT start_time FROM trip_stations "
      "WHERE point_id=:point_id AND desk=:desk AND work_mode=:work_mode";
    Qryh.CreateVariable( "point_id", otInteger, point_id );
    Qryh.CreateVariable( "desk", otString, reqInfo->desk.code );
    Qryh.CreateVariable( "work_mode", otString, "�" );
    Qryh.Execute();
    if (!Qryh.Eof)
      NewTextChild( node, "start_check_info", (int)!Qryh.FieldIsNULL( "start_time" ) );
  };*/

  if (reqInfo->screen.name == "AIR.EXE" ||
      reqInfo->screen.name == "BRDBUS.EXE" ||
      reqInfo->screen.name == "EXAM.EXE" ||
      reqInfo->screen.name == "DOCS.EXE" ||
      reqInfo->screen.name == "PREPREG.EXE")
  {
    if (reqInfo->screen.name == "AIR.EXE" ||
        reqInfo->screen.name == "PREPREG.EXE")
    {
      NewTextChild( node, "ckin_stage", tripStages.getStage( stCheckIn ) );
      NewTextChild( node, "tranzitable", (int)(Qry.FieldAsInteger("tranzitable")!=0) );
    };

    TQuery Qryh( &OraSession );
    Qryh.Clear();
    Qryh.SQLText=
      "SELECT NVL(pr_tranz_reg,0) AS pr_tranz_reg, "
      "       NVL(pr_block_trzt,0) AS pr_block_trzt, "
      "       pr_etstatus "
      "FROM trip_sets WHERE point_id=:point_id ";
    Qryh.CreateVariable( "point_id", otInteger, point_id );
    Qryh.Execute();
    if (Qryh.Eof) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);
/*    if (Qryh.FieldAsInteger("pr_etstatus")<0)
    {
      //�뢮� "��� �裡 � ���" � ���ଠ樨 �� ३��
      string remark=Qry.FieldAsString( "remark" );
      if (!remark.empty()) remark.append(" ");
      remark.append("��� �裡 � ���.");
      ReplaceTextChild(node, "remark",  remark);
    };*/

    if (reqInfo->screen.name == "AIR.EXE" ||
        reqInfo->screen.name == "DOCS.EXE" ||
        reqInfo->screen.name == "PREPREG.EXE")
    {
      NewTextChild( node, "pr_tranz_reg", (int)(Qryh.FieldAsInteger("pr_tranz_reg")!=0) );
    };

    if (reqInfo->screen.name == "PREPREG.EXE")
    {
      NewTextChild( node, "pr_block_trzt", (int)(Qryh.FieldAsInteger("pr_block_trzt")!=0) );

      TTripSetList setList;
      setList.fromDB(point_id);
      if (setList.empty()) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);
      setList.toXML(node);

      NewTextChild( node, "apis_exists", (int)(TRouteAPICheckInfo(point_id).apis_generation()) );
    };

    if (reqInfo->screen.name == "AIR.EXE" ||
        reqInfo->screen.name == "BRDBUS.EXE" ||
        reqInfo->screen.name == "EXAM.EXE")
    {
      NewTextChild( node, "pr_etstatus", Qryh.FieldAsInteger("pr_etstatus") );
      NewTextChild( node, "pr_etl_only", (int)GetTripSets(tsETSNoExchange,info) );
    };

    if (reqInfo->screen.name == "AIR.EXE")
    {
        NewTextChild( node, "pr_no_ticket_check", (int)GetTripSets(tsNoTicketCheck,info) );
        NewTextChild( node, "pr_auto_pt_print", (int)GetTripSets(tsAutoPTPrint,info) );
        NewTextChild( node, "pr_auto_pt_print_reseat", (int)GetTripSets(tsAutoPTPrintReseat,info) );

        TTripSetList setList;
        setList.fromDB(point_id);
        if (setList.empty()) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);
        NewTextChild( node, "use_jmp", (int)setList.value<bool>(tsUseJmp) );
    };
  };

  {
    string stralarms;
    BitSet<Alarm::Enum> Alarms;
    TripAlarms( point_id, Alarms );
    for(Alarm::TPairs::const_iterator a=Alarm::pairs().begin(); a!=Alarm::pairs().end(); ++a)
    {
      Alarm::Enum alarm = a->first;
      string rem;
      if ( !Alarms.isFlag( alarm ) )
        continue;
      switch( alarm ) {
        case Alarm::Waitlist:
          if (reqInfo->screen.name == "CENT.EXE" ||
              reqInfo->screen.name == "PREPREG.EXE" ||
              reqInfo->screen.name == "AIR.EXE" ||
              reqInfo->screen.name == "BRDBUS.EXE" ||
              reqInfo->screen.name == "EXAM.EXE" ||
              reqInfo->screen.name == "DOCS.EXE") {
            rem = TripAlarmString( alarm );
          }
          break;
        case Alarm::Overload:
          if (reqInfo->screen.name == "CENT.EXE" ||
              reqInfo->screen.name == "PREPREG.EXE")
            rem = TripAlarmString( alarm );
          break;
        case Alarm::Brd:
          if (reqInfo->screen.name == "BRDBUS.EXE" ||
              reqInfo->screen.name == "DOCS.EXE")
            rem = TripAlarmString( alarm );
          break;
        case Alarm::Salon:
          if (reqInfo->screen.name == "CENT.EXE" ||
              reqInfo->screen.name == "PREPREG.EXE" ||
              reqInfo->screen.name == "AIR.EXE")
            rem = TripAlarmString( alarm );
          break;
        case Alarm::ETStatus:
            if (reqInfo->screen.name == "AIR.EXE" ||
                  reqInfo->screen.name == "BRDBUS.EXE")
            rem = TripAlarmString( alarm );
          break;
        case Alarm::DiffComps:
          if (reqInfo->screen.name == "CENT.EXE" ||
              reqInfo->screen.name == "PREPREG.EXE" ||
              reqInfo->screen.name == "AIR.EXE" ||
              reqInfo->screen.name == "BRDBUS.EXE" ||
              reqInfo->screen.name == "EXAM.EXE" ||
              reqInfo->screen.name == "DOCS.EXE") {
            rem = TripAlarmString( alarm ) + SALONS2::getDiffCompsAlarmRoutes( point_id );
          }
          break;
        case Alarm::TlgIn:
          if (reqInfo->screen.name == "TLG.EXE")
            rem = TripAlarmString( alarm );
          break;
        case Alarm::TlgOut:
          if (reqInfo->screen.name == "TLG.EXE")
            rem = TripAlarmString( alarm );
          break;
        case Alarm::APISIncomplete:
          if (reqInfo->screen.name == "AIR.EXE")
            rem = TripAlarmString( alarm );
          break;
        case Alarm::APISManualInput:
          if (reqInfo->screen.name == "AIR.EXE")
            rem = TripAlarmString( alarm );
          break;
        default:
          break;
      }
      if ( !rem.empty() ) {
        if ( !stralarms.empty() )
            stralarms += " ";
        stralarms += "!" + rem;
      }
    }
    if ( !stralarms.empty() ) {
      NewTextChild( node, "alarms", stralarms );
    }
  }

  if (reqInfo->screen.name == "AIR.EXE")
  {
    NewTextChild( node, "pr_mixed_norms", (int)GetTripSets(tsMixedNorms,info) );
    //������ �뢮����� �� ����� �஢�� � ⥣�� <tripheader>, � ���� �� �����, �� �ନ���� ���� �� �����ন����
    set<string> trip_vouchers;
    getTripVouchers(point_id, trip_vouchers);
    NewTextChild( dataNode, "pr_vouchers", !trip_vouchers.empty() );
  }

  if (reqInfo->screen.name == "KASSA.EXE")
  {
    NewTextChild( node, "pr_mixed_rates", (int)GetTripSets(tsMixedNorms,info) );
    NewTextChild( node, "pr_mixed_taxes", (int)GetTripSets(tsMixedNorms,info) );
  };

  return true;
}

void TripsInterface::readGates(int point_id, vector<string> &gates)
{
    TQuery Qry( &OraSession );
    Qry.SQLText =
        "SELECT name AS gate_name "
        "FROM stations,trip_stations "
        "WHERE stations.desk=trip_stations.desk AND "
        "      stations.work_mode=trip_stations.work_mode AND "
        "      trip_stations.point_id=:point_id AND "
        "      trip_stations.work_mode=:work_mode ";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("work_mode",otString,"�");
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
        gates.push_back(Qry.FieldAsString("gate_name"));
}

void TripsInterface::readHalls( std::string airp_dep, std::string work_mode, xmlNodePtr dataNode)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT halls2.id "
    "FROM station_halls,halls2,stations "
    "WHERE station_halls.hall=halls2.id AND halls2.airp=:airp_dep AND "
    "     station_halls.airp=stations.airp AND "
    "     station_halls.station=stations.name AND "
    "     stations.desk=:desk AND stations.work_mode=:work_mode";
  Qry.CreateVariable("airp_dep",otString,airp_dep);
  Qry.CreateVariable("desk",otString, TReqInfo::Instance()->desk.code);
  Qry.CreateVariable("work_mode",otString,work_mode);

  Qry.Execute();
  if (Qry.Eof)
  {
    Qry.Clear();
    Qry.SQLText =
      "SELECT id FROM halls2 WHERE airp=:airp_dep";
    Qry.CreateVariable("airp_dep",otString,airp_dep);
    Qry.Execute();
  };
  xmlNodePtr node = NewTextChild( dataNode, "halls" );
  for(;!Qry.Eof;Qry.Next())
  {
    xmlNodePtr itemNode = NewTextChild( node, "hall" );
    NewTextChild( itemNode, "id", Qry.FieldAsInteger( "id" ) );
    NewTextChild( itemNode, "name", ElemIdToNameLong( etHall, Qry.FieldAsInteger( "id" ) ) );
  }
}

TripsInterface* TripsInterface::instance()
{
    static TripsInterface* inst = 0;
    if(!inst) {
        inst = new TripsInterface();
    }
    return inst;
}


class TPaxLoadItem
{
  public:
    //���ਨ ��㯯�஢��
    string class_id;
    string class_view;
    int class_priority;

    int cls_grp_id;
    string cls_grp_view;
    int cls_grp_priority;

    int hall_id;
    string hall_airp_view;
    string hall_name_view;

    int point_arv;
    string airp_arv_view;
    int airp_arv_priority;

    string trfer_airline_id;
    int trfer_flt_no;
    string trfer_suffix_id;
    string trfer_airp_arv_id;
    string trfer_view;

    int user_id;
    string user_view;

    string client_type_id;
    string client_type_view;
    int client_type_priority;

    string grp_status_id;
    string grp_status_view;
    int grp_status_priority;

    string ticket_rem;

    string rem_code;

    string section;

    //�����
    int cfg; //����������
    int crs_ok,crs_tranzit; //����� �஭�஢����
    int seats,adult_m,adult_f,child,baby; //���ᠦ���
    int rk_weight,bag_amount,bag_weight; //�����
    TBagKilos excess_wt;  //����� ���
    TBagPieces excess_pc; //����� ����

    TPaxLoadItem():
      class_priority(NoExists),
      cls_grp_id(NoExists),
      cls_grp_priority(NoExists),
      hall_id(NoExists),
      point_arv(NoExists),
      airp_arv_priority(NoExists),
      trfer_flt_no(NoExists),
      user_id(NoExists),
      client_type_priority(NoExists),
      grp_status_priority(NoExists),
      cfg(0),
      crs_ok(0), crs_tranzit(0),
      seats(0), adult_m(0), adult_f(0), child(0), baby(0),
      rk_weight(0), bag_amount(0), bag_weight(0),
      excess_wt(0), excess_pc(0) {};

    bool operator == (const TPaxLoadItem &item) const
    {
      return class_id == item.class_id &&
             cls_grp_id == item.cls_grp_id &&
             hall_id == item.hall_id &&
             point_arv == item.point_arv &&
             trfer_airline_id == item.trfer_airline_id &&
             trfer_flt_no == item.trfer_flt_no &&
             trfer_suffix_id == item.trfer_suffix_id &&
             trfer_airp_arv_id == item.trfer_airp_arv_id &&
             user_id == item.user_id &&
             client_type_id == item.client_type_id &&
             grp_status_id == item.grp_status_id &&
             ticket_rem == item.ticket_rem &&
             rem_code == item.rem_code &&
             section == item.section;
    };
    TPaxLoadItem& operator += (const TPaxLoadItem &item)
    {
      cfg+=item.cfg;
      crs_ok+=item.crs_ok;
      crs_tranzit+=item.crs_tranzit;
      seats+=item.seats;
      adult_m+=item.adult_m;
      adult_f+=item.adult_f;
      child+=item.child;
      baby+=item.baby;
      rk_weight+=item.rk_weight;
      bag_amount+=item.bag_amount;
      bag_weight+=item.bag_weight;
      excess_wt+=item.excess_wt;
      excess_pc+=item.excess_pc;
      return *this;
    };

};

class TPaxLoadOrder
{
  public:
    vector<string> fields;
    bool operator () (const TPaxLoadItem &item1, const TPaxLoadItem &item2) const
    {
      for(vector<string>::const_iterator f=fields.begin();f!=fields.end();f++)
      {
        if (*f=="class")
        {
          if (item1.class_priority!=NoExists &&
              item2.class_priority!=NoExists)
          {
            if (item1.class_priority!=item2.class_priority)
              return item1.class_priority<item2.class_priority;
          }
          else
          {
            if (item1.class_priority!=item2.class_priority)
              return item1.class_priority!=NoExists;
          };
          continue;
        };
        if (*f=="cl_grp")
        {
          if (item1.cls_grp_priority!=NoExists &&
              item2.cls_grp_priority!=NoExists)
          {
            if (item1.cls_grp_priority!=item2.cls_grp_priority)
              return item1.cls_grp_priority<item2.cls_grp_priority;
          }
          else
          {
            if (item1.cls_grp_priority!=item2.cls_grp_priority)
              return item1.cls_grp_priority!=NoExists;
          };
          continue;
        };
        if (*f=="hall")
        {
          if (item1.hall_id>=1000000000 || item2.hall_id>=1000000000)
          {
            if (item1.hall_id==NoExists) return false;
            if (item2.hall_id==NoExists) return true;
            if (item1.hall_id!=item2.hall_id)
              return item1.hall_id<item2.hall_id;
          }
          else
          {
            if (!item1.hall_name_view.empty() &&
                !item2.hall_name_view.empty())
            {
              if (item1.hall_airp_view!=item2.hall_airp_view)
                return item1.hall_airp_view<item2.hall_airp_view;
              if (item1.hall_name_view!=item2.hall_name_view)
                return item1.hall_name_view<item2.hall_name_view;
            }
            else
            {
              if (item1.hall_name_view!=item2.hall_name_view)
                return !item1.hall_name_view.empty();
            };
          };
          continue;
        };
        if (*f=="airp_arv")
        {
          if (item1.airp_arv_priority!=item2.airp_arv_priority)
            return item1.airp_arv_priority<item2.airp_arv_priority;
          continue;
        };
        if (*f=="trfer")
        {
          if (item1.trfer_view!=item2.trfer_view)
            return item1.trfer_view<item2.trfer_view;
          continue;
        };
        if (*f=="user")
        {
          if (item1.user_id>=1000000000 || item2.user_id>=1000000000)
          {
            if (item1.user_id==NoExists) return false;
            if (item2.user_id==NoExists) return true;
            if (item1.user_id!=item2.user_id)
              return item1.user_id<item2.user_id;
          }
          else
          {
            if (!item1.user_view.empty() &&
                !item2.user_view.empty())
            {
              if (item1.user_view!=item2.user_view)
                return item1.user_view<item2.user_view;
            }
            else
            {
              if (item1.user_view!=item2.user_view)
                return !item1.user_view.empty();
            };
          };
          continue;
        };
        if (*f=="client_type")
        {
          if (item1.client_type_priority!=item2.client_type_priority)
            return item1.client_type_priority<item2.client_type_priority;
          continue;
        };
        if (*f=="status")
        {
          if (item1.grp_status_priority!=item2.grp_status_priority)
            return item1.grp_status_priority<item2.grp_status_priority;
          continue;
        };
        if (*f=="ticket_rem")
        {
          if (item1.ticket_rem!=item2.ticket_rem)
            return item1.ticket_rem<item2.ticket_rem;
          continue;
        };
        if (*f=="rems")
        {
          if (item1.rem_code!=item2.rem_code)
            return item1.rem_code<item2.rem_code;
          continue;
        };
        if (*f=="section")
        {
          if (item1.section!=item2.section)
            return item1.section<item2.section;
          continue;
        };
      };
      return false;
    };
};

class TZonePaxItem
{
  public:
    int pax_id, grp_id, seats, parent_pax_id, temp_parent_id, reg_no;
    string surname, pers_type, zone;
    int rk_weight,bag_amount,bag_weight;
    bool is_female;
};

std::string createCrewFilter()
{
  return "pax_grp.status NOT IN ('E') AND ";
}

void readPaxZoneLoad( int point_id, const string &crew_filter, list<TPaxLoadItem> &paxLoad, std::map<std::string,int> &paxRemCounters ) //pr_section=true
{
  multiset<CheckIn::TPaxRemItem> rems;
  for ( auto& ic : paxRemCounters ) {
    ic.second = 0;
  }
  paxLoad.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  sql << "SELECT pax.pax_id, pax.grp_id, pax.surname, pax.pers_type, pax.seats, pax.reg_no, " << endl
      << "       NVL(pax.is_female, 0) AS is_female, " << endl
      << "       crs_inf.pax_id AS parent_pax_id, " << endl
      << "       ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_amount, " << endl
      << "       ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_weight, " << endl
      << "       ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS rk_weight " << endl
      << "FROM pax_grp, pax, crs_inf " << endl
      << "WHERE pax_grp.grp_id=pax.grp_id AND " << endl
      << "      pax_grp.point_dep=:point_id AND " << endl
      << "      " << crew_filter << endl
      << "      pax.pr_brd IS NOT NULL AND " << endl
      << "      pax.pax_id=crs_inf.inf_id(+)" << endl;
  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  vector<TZonePaxItem> zonePaxs;
  //�⠥� ���ᠦ�஢ �� ३�
  for(;!Qry.Eof;Qry.Next())
  {
    TZonePaxItem pax;
    pax.pax_id=Qry.FieldAsInteger("pax_id");
    pax.grp_id=Qry.FieldAsInteger("grp_id");
    pax.seats=Qry.FieldAsInteger("seats");
    pax.reg_no=Qry.FieldAsInteger("reg_no");
    pax.surname=Qry.FieldAsString("surname");
    pax.pers_type=Qry.FieldAsString("pers_type");
    pax.is_female=Qry.FieldAsInteger("is_female")!=0;
    pax.parent_pax_id=Qry.FieldIsNULL("parent_pax_id")?NoExists:Qry.FieldAsInteger("parent_pax_id");
    pax.rk_weight=Qry.FieldAsInteger("rk_weight");
    pax.bag_amount=Qry.FieldAsInteger("bag_amount");
    pax.bag_weight=Qry.FieldAsInteger("bag_weight");
    if ( !paxRemCounters.empty() ) {
      CheckIn::LoadPaxRem( pax.pax_id, rems );
      for ( auto& ir : rems ) {
        if ( paxRemCounters.find( ir.rem_code() ) != paxRemCounters.end() ) {
          paxRemCounters[ ir.rem_code() ]++;
        }
      }
    }
    zonePaxs.push_back(pax);
  };

  vector<SALONS2::TCompSection> compSections;
  //����砥� ���ଠ�� �� �����
  ZonePax(point_id, zonePaxs, compSections);

  for(vector<SALONS2::TCompSection>::const_iterator i=compSections.begin();i!=compSections.end();i++)
  {
    TPaxLoadItem item;
    item.section = i->getName();
    for(vector<TZonePaxItem>::const_iterator p=zonePaxs.begin();p!=zonePaxs.end();p++)
    {
      if (item.section!=p->zone) continue;
      item.seats+=p->seats;
      switch(DecodePerson(p->pers_type.c_str()))
      {
        case adult: p->is_female?item.adult_f++:item.adult_m++; break;
        case child: item.child++; break;
        case baby:  item.baby++;  break;
        default: ;
      };
      item.rk_weight+=p->rk_weight;
      item.bag_amount+=p->bag_amount;
      item.bag_weight+=p->bag_weight;
    };
    paxLoad.push_back(item);
  };
}

void readPaxZoneLoad( int point_id, const string &crew_filter, list<TPaxLoadItem> &paxLoad ) //pr_section=true
{
  std::map<std::string,int> paxRemCounters;
  readPaxZoneLoad( point_id, crew_filter, paxLoad, paxRemCounters );
}

struct PaxLoadTotalRowStruct {
  int seats;
  int adult_m;
  int adult_f;
  int child;
  int baby;
  int bag_amount;
  int bag_weight;
  int rk_weight;
  std::map<std::string, pair<int,int>> bags;
  int crs_ok;
  int crs_tranzit;
  TBagKilos excess_wt;
  TBagPieces excess_pc;
  int cfg;
  PaxLoadTotalRowStruct() :
    excess_wt(0), excess_pc(0)
  {
   seats = 0;
   adult_m = 0;
   adult_f = 0;
   child = 0;
   baby = 0;
   bag_amount = 0;
   bag_weight = 0;
   rk_weight = 0;
   crs_ok = 0;
   crs_tranzit = 0;
   cfg = 0;
  }
};

void createReadPaxLoadTotalRow( TQuery &Qry, int point_id, PaxLoadTotalRowStruct &total )
{
  Qry.Clear();
  Qry.SQLText =
    "SELECT a.seats,a.adult_m,a.adult_f,a.child,a.baby, "
    "       c.crs_ok,c.crs_tranzit, "
    "       e.excess_wt,e.excess_pc,f.cfg "
    "FROM "
    " (SELECT NVL(SUM(seats),0) AS seats, "
    "         NVL(SUM(DECODE(pers_type,:adult,DECODE(pax.is_female,0,1,NULL,1,0),0)),0) AS adult_m, "
    "         NVL(SUM(DECODE(pers_type,:adult,DECODE(pax.is_female,0,0,NULL,0,1),0)),0) AS adult_f, "
    "         NVL(SUM(DECODE(pers_type,:child,1,0)),0) AS child, "
    "         NVL(SUM(DECODE(pers_type,:baby,1,0)),0) AS baby "
    "  FROM pax_grp,pax "
    "  WHERE pax_grp.grp_id=pax.grp_id AND "
    "        point_dep=:point_id AND status NOT IN ('E') AND pr_brd IS NOT NULL) a, "
    " (SELECT NVL(SUM(crs_ok),0) AS crs_ok, "
    "         NVL(SUM(crs_tranzit),0) AS crs_tranzit "
    "  FROM counters2 "
    "  WHERE point_dep=:point_id) c, "
    " (SELECT NVL(SUM(excess_wt),0) AS excess_wt, "
    "         NVL(SUM(excess_pc),0) AS excess_pc "
    "  FROM pax_grp "
    "  WHERE point_dep=:point_id AND status NOT IN ('E') AND bag_refuse=0) e, "
    " (SELECT NVL(SUM(cfg),0) AS cfg "
    "  FROM trip_classes "
    "  WHERE point_id=:point_id) f";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.CreateVariable("adult",otString,EncodePerson(ASTRA::adult));
  Qry.CreateVariable("child",otString,EncodePerson(ASTRA::child));
  Qry.CreateVariable("baby",otString,EncodePerson(ASTRA::baby));
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");

  total.seats = Qry.FieldAsInteger("seats");
  total.adult_m = Qry.FieldAsInteger("adult_m");
  total.adult_f = Qry.FieldAsInteger("adult_f");
  total.child = Qry.FieldAsInteger("child");
  total.baby = Qry.FieldAsInteger("baby");
  total.crs_ok = Qry.FieldAsInteger("crs_ok");
  total.crs_tranzit = Qry.FieldAsInteger("crs_tranzit");
  total.excess_wt = Qry.FieldAsInteger("excess_wt");
  total.excess_pc = Qry.FieldAsInteger("excess_pc");
  total.cfg = Qry.FieldAsInteger("cfg");
  Qry.Clear();
  Qry.SQLText =
    "SELECT NVL(SUM(DECODE(pr_cabin,0,amount,0)),0) AS bag_amount, "
    "       NVL(SUM(DECODE(pr_cabin,0,weight,0)),0) AS bag_weight, "
    "       NVL(SUM(DECODE(pr_cabin,0,0,weight)),0) AS rk_weight, "
    "       pax_grp.class "
    "  FROM pax_grp,bag2 "
    "  WHERE pax_grp.grp_id=bag2.grp_id AND "
    "        point_dep=:point_id AND status NOT IN ('E') AND "
    "        ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 "
    " GROUP BY class";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  total.bags.clear();
  for ( ; !Qry.Eof; Qry.Next() ) {
     total.bag_amount +=  Qry.FieldAsInteger("bag_amount");
     total.bag_weight += Qry.FieldAsInteger("bag_weight");
     total.rk_weight += Qry.FieldAsInteger("rk_weight");
     total.bags[ Qry.FieldAsString( "class" ) ] = make_pair( Qry.FieldAsInteger("bag_amount"),  Qry.FieldAsInteger("bag_weight") );
  }
}

void readPaxLoad( int point_id, xmlNodePtr reqNode, xmlNodePtr resNode )
{
  reqNode=GetNode("tripcounters",reqNode);
  if (reqNode==NULL) return;

  resNode=NewTextChild(resNode,"tripcounters");

  //ᥪ�� �⮫�殢
  xmlNodePtr fieldsNode=NewTextChild(resNode,"fields");

  NewTextChild(fieldsNode,"field","title");
  xmlNodePtr reqFieldsNode=NodeAsNode("fields",reqNode);
  TPaxLoadOrder paxLoadOrder;
  for(xmlNodePtr node=reqFieldsNode->children;node!=NULL;node=node->next)
  {
    paxLoadOrder.fields.push_back((const char*)node->name);
    NewTextChild(fieldsNode,"field",(const char*)node->name);
  };
  NewTextChild(fieldsNode,"field","cfg");
  NewTextChild(fieldsNode,"field","crs_ok");
  NewTextChild(fieldsNode,"field","crs_tranzit");
  NewTextChild(fieldsNode,"field","seats");
  NewTextChild(fieldsNode,"field","adult_m");
  NewTextChild(fieldsNode,"field","adult_f");
  NewTextChild(fieldsNode,"field","child");
  NewTextChild(fieldsNode,"field","baby");
  NewTextChild(fieldsNode,"field","rk_weight");
  NewTextChild(fieldsNode,"field","bag_amount");
  NewTextChild(fieldsNode,"field","bag_weight");
  NewTextChild(fieldsNode,"field","excess");
  NewTextChild(fieldsNode,"field","load");

  //��ப� '�⮣�'
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,flt_no,suffix,airp,scd_out FROM points "
    "WHERE point_id=:point_id AND pr_del>=0 AND pr_reg<>0";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  TTripInfo fltInfo(Qry);
  PaxLoadTotalRowStruct total;
  TComplexBagExcessNodeList  excessNodeList(OutputLang(), {TComplexBagExcessNodeList::ContainsOnlyNonZeroExcess});
  createReadPaxLoadTotalRow( Qry, point_id, total );
  //ᥪ�� ��ப
  xmlNodePtr rowsNode=NewTextChild(resNode,"rows");

  xmlNodePtr rowNode=NewTextChild(rowsNode,"row");
  NewTextChild(rowNode,"title",AstraLocale::getLocaleText("�ᥣ�"));
  NewTextChild(rowNode,"seats",total.seats,0);
  NewTextChild(rowNode,"adult_m",total.adult_m,0);
  NewTextChild(rowNode,"adult_f",total.adult_f,0);
  NewTextChild(rowNode,"child",total.child,0);
  NewTextChild(rowNode,"baby",total.baby,0);
  NewTextChild(rowNode,"bag_amount",total.bag_amount,0);
  NewTextChild(rowNode,"bag_weight",total.bag_weight,0);
  NewTextChild(rowNode,"rk_weight",total.rk_weight,0);
  NewTextChild(rowNode,"crs_ok",total.crs_ok,0);
  NewTextChild(rowNode,"crs_tranzit",total.crs_tranzit,0);

  excessNodeList.add(rowNode, "excess", total.excess_pc, total.excess_wt);

  NewTextChild(rowNode,"cfg",total.cfg,0);
  NewTextChild(rowNode,"load",getCommerceWeight( point_id, onlyCheckin, CWTotal ),0);

  if (paxLoadOrder.fields.empty()) return;

  xmlNodePtr node2=reqFieldsNode->children;
  bool pr_class=      GetNodeFast("class",node2)!=NULL;
  bool pr_cl_grp=     GetNodeFast("cl_grp",node2)!=NULL;
  bool pr_hall=       GetNodeFast("hall",node2)!=NULL;
  bool pr_airp_arv=   GetNodeFast("airp_arv",node2)!=NULL;
  bool pr_trfer=      GetNodeFast("trfer",node2)!=NULL;
  bool pr_user=       GetNodeFast("user",node2)!=NULL;
  bool pr_client_type=GetNodeFast("client_type",node2)!=NULL;
  bool pr_status=     GetNodeFast("status",node2)!=NULL;
  bool pr_ticket_rem= GetNodeFast("ticket_rem",node2)!=NULL;
  bool pr_rems=       GetNodeFast("rems",node2)!=NULL;
  bool pr_section=    GetNodeFast("section",node2)!=NULL;
  map< TRemCategory, vector<string> > rems;
  if (pr_rems)
  {
    xmlNodePtr node=NodeAsNodeFast("rems",node2)->children;
    for(;node!=NULL;node=node->next)
    {
      TRemCategory cat=getRemCategory(NodeAsString(node), "");
      if (isDisabledRemCategory(cat))
        rems[cat].push_back(NodeAsString(node));
      else
        rems[remUnknown].push_back(NodeAsString(node));
    };
    if (rems.empty()) pr_rems=false;
  };

  if (paxLoadOrder.fields.size()==1 &&
      *paxLoadOrder.fields.begin()=="rems" && !pr_rems) return;

  list<TPaxLoadItem> paxLoad;

  string crew_filter;
  if (!(pr_class || pr_cl_grp || pr_status)) crew_filter=createCrewFilter();

  if (!pr_section)
  {
    ostringstream last_trfer_sql;
    last_trfer_sql << "     (SELECT trfer_trips.airline,trfer_trips.flt_no,trfer_trips.suffix,transfer.airp_arv, " << endl
                   << "             transfer.grp_id " << endl
                   << "      FROM pax_grp,transfer,trfer_trips " << endl
                   << "      WHERE pax_grp.grp_id=transfer.grp_id AND " << endl
                   << "            transfer.point_id_trfer=trfer_trips.point_id AND " << endl
                   << "            transfer.pr_final<>0 AND " << endl
                   << "            " << crew_filter << endl
                   << "            pax_grp.point_dep=:point_id) last_trfer" << endl;

    for(int pass=1;pass<=5;pass++)
    {
      //1. ���᫥��� cfg
      //2. ���᫥��� crs_ok, crs_tranzit
      //3. ���᫥��� seats, adult_m, adult_f, child, baby
      //4. ���᫥��� rk_weight, bag_amount, bag_weight
      //5. ���᫥��� excess_wt, excess_pc
      if ((pass==1 && (pr_cl_grp || pr_hall || pr_airp_arv || pr_trfer || pr_user || pr_client_type || pr_status || pr_ticket_rem || pr_rems)) ||
          (pass==2 && (pr_cl_grp || pr_hall || pr_trfer || pr_user || pr_client_type || pr_status || pr_ticket_rem || pr_rems)) ||
          (pass==4 && (pr_ticket_rem || pr_rems)) ||
          (pass==5 && (pr_ticket_rem || pr_rems))) continue;

      ostringstream sql, group_by, select;

      if (pass==1)
      {
        //����� �� ����������
        for(int i=0; i<2; i++)
        {
          ostringstream &s=(i==0)?select:group_by;
          if (pr_class)       s << ", trip_classes.class";
        };

        sql << "SELECT SUM(trip_classes.cfg) AS cfg, " << endl
            << "       " << select.str().erase(0,1) << endl
            << "FROM trip_classes " << endl
            << "WHERE point_id=:point_id " << endl;
      };
      if (pass==2)
      {
        //����� �� �஭�
        for(int i=0; i<2; i++)
        {
          ostringstream &s=(i==0)?select:group_by;
          if (pr_class)       s << ", counters2.class";
          if (pr_airp_arv)    s << ", counters2.point_arv";
        };

        sql << "SELECT SUM(counters2.crs_ok) AS crs_ok, " << endl
            << "       SUM(counters2.crs_tranzit) AS crs_tranzit, " << endl
            << "       " << select.str().erase(0,1) << endl
            << "FROM counters2 " << endl
            << "WHERE point_dep=:point_id " << endl;
      };
      if (pass==3)
      {
        //����� �� ���ᠦ�ࠬ
        for(int i=0; i<2; i++)
        {
          ostringstream &s=(i==0)?select:group_by;
          if (pr_class)       s << ", DECODE(pax_grp.status,'E',' ',pax_grp.class)"
                                << (i==0?" AS class":"");
          if (pr_cl_grp)      s << ", DECODE(pax_grp.status,'E',1000000000,pax_grp.class_grp)"
                                << (i==0?" AS class_grp":"");
          if (pr_hall)        s << ", pax_grp.hall";
          if (pr_airp_arv)    s << ", pax_grp.point_arv";
          if (pr_trfer)       s << ", last_trfer.airline"
                                << ", last_trfer.flt_no"
                                << ", last_trfer.suffix"
                                << ", last_trfer.airp_arv";
          if (pr_user)        s << ", pax_grp.user_id";
          if (pr_client_type) s << ", pax_grp.client_type";
          if (pr_status)      s << ", pax_grp.status";
          if (pr_ticket_rem)  s << ", pax.ticket_rem";
          if (pr_rems)        s << ", pax_rem.rem_code";
        };

        sql << "SELECT SUM(pax.seats) AS seats, " << endl
            << "       SUM(DECODE(pax.pers_type,:adult,DECODE(pax.is_female,0,1,NULL,1,0),0)) AS adult_m, " << endl
            << "       SUM(DECODE(pax.pers_type,:adult,DECODE(pax.is_female,0,0,NULL,0,1),0)) AS adult_f, " << endl
            << "       SUM(DECODE(pax.pers_type,:child,1,0)) AS child, " << endl
            << "       SUM(DECODE(pax.pers_type,:baby,1,0)) AS baby, " << endl
            << "       " << select.str().erase(0,1) << endl
            << "FROM pax_grp,pax " << endl;

        if (pr_trfer) sql << "    ," << endl
                          << last_trfer_sql.str() << endl;
        if (pr_rems)
        {
          sql << ",( " << endl;
          for(map< TRemCategory, vector<string> >::const_iterator iRem=rems.begin(); iRem!=rems.end(); iRem++)
          {
            if (iRem!=rems.begin()) sql << "  UNION " << endl;
            switch(iRem->first)
            {
              case remTKN:
                sql << "  SELECT pax.pax_id,pax.ticket_rem AS rem_code" << endl
                    << "  FROM pax_grp,pax " << endl
                    << "  WHERE pax_grp.grp_id=pax.grp_id AND " << endl
                    << "        pax_grp.point_dep=:point_id AND " << endl
                    << "        " << crew_filter << endl
                    << "        pax.pr_brd IS NOT NULL AND " << endl
                    << "        pax.ticket_rem IN " << GetSQLEnum(iRem->second) << endl;
                break;
              case remDOC:
                sql << "  SELECT pax.pax_id,'DOCS' AS rem_code " << endl
                    << "  FROM pax_grp,pax,pax_doc " << endl
                    << "  WHERE pax_grp.grp_id=pax.grp_id AND " << endl
                    << "        pax.pax_id=pax_doc.pax_id AND " << endl
                    << "        pax_grp.point_dep=:point_id AND " << endl
                    << "        " << crew_filter << endl
                    << "        pax.pr_brd IS NOT NULL AND " << endl
                    << "        'DOCS' IN " << GetSQLEnum(iRem->second) << endl;
                break;
              case remDOCO:
                sql << "  SELECT pax.pax_id,'DOCO' AS rem_code " << endl
                    << "  FROM pax_grp,pax,pax_doco " << endl
                    << "  WHERE pax_grp.grp_id=pax.grp_id AND " << endl
                    << "        pax.pax_id=pax_doco.pax_id AND " << endl
                    << "        pax_grp.point_dep=:point_id AND " << endl
                    << "        " << crew_filter << endl
                    << "        pax.pr_brd IS NOT NULL AND " << endl
                    << "        'DOCO' IN " << GetSQLEnum(iRem->second) << endl;
                break;
              case remDOCA:
                sql << "  SELECT DISTINCT pax.pax_id,'DOCA' AS rem_code " << endl
                    << "  FROM pax_grp,pax,pax_doca " << endl
                    << "  WHERE pax_grp.grp_id=pax.grp_id AND " << endl
                    << "        pax.pax_id=pax_doca.pax_id AND " << endl
                    << "        pax_grp.point_dep=:point_id AND " << endl
                    << "        " << crew_filter << endl
                    << "        pax.pr_brd IS NOT NULL AND " << endl
                    << "        'DOCA' IN " << GetSQLEnum(iRem->second) << endl;
                break;
              case remASVC:
                sql << "  SELECT DISTINCT pax.pax_id,'ASVC' AS rem_code " << endl
                    << "  FROM pax_grp,pax,pax_asvc " << endl
                    << "  WHERE pax_grp.grp_id=pax.grp_id AND " << endl
                    << "        pax.pax_id=pax_asvc.pax_id AND " << endl
                    << "        pax_grp.point_dep=:point_id AND " << endl
                    << "        " << crew_filter << endl
                    << "        pax.pr_brd IS NOT NULL AND " << endl
                    << "        'ASVC' IN " << GetSQLEnum(iRem->second) << endl;
                break;
              case remFQT:
                sql << "  SELECT DISTINCT pax.pax_id, pax_fqt.rem_code " << endl
                    << "  FROM pax_grp,pax,pax_fqt " << endl
                    << "  WHERE pax_grp.grp_id=pax.grp_id AND " << endl
                    << "        pax.pax_id=pax_fqt.pax_id AND " << endl
                    << "        pax_grp.point_dep=:point_id AND " << endl
                    << "        " << crew_filter << endl
                    << "        pax.pr_brd IS NOT NULL AND " << endl
                    << "        pax_fqt.rem_code IN " << GetSQLEnum(iRem->second) << endl;
                break;
              case remCREW:
              case remUnknown:
                sql << "  SELECT DISTINCT pax.pax_id,pax_rem.rem_code " << endl
                    << "  FROM pax_grp,pax,pax_rem " << endl
                    << "  WHERE pax_grp.grp_id=pax.grp_id AND " << endl
                    << "        pax.pax_id=pax_rem.pax_id AND " << endl
                    << "        pax_grp.point_dep=:point_id AND " << endl
                    << "        " << crew_filter << endl
                    << "        pax.pr_brd IS NOT NULL AND " << endl
                    << "        pax_rem.rem_code IN " << GetSQLEnum(iRem->second) << endl;
                break;
            };
          };
          sql << " ) pax_rem " << endl;
        };

        sql << "WHERE pax_grp.grp_id=pax.grp_id AND " << endl
            << "      pax_grp.point_dep=:point_id AND " << endl
            << "      " << crew_filter << endl
            << "      pax.pr_brd IS NOT NULL " << endl;
        if (pr_trfer) sql << "      AND pax_grp.grp_id=last_trfer.grp_id(+) " << endl;
        if (pr_rems)  sql << "      AND pax.pax_id=pax_rem.pax_id " << endl;
      };
      if (pass==4)
      {
        //����� �� ������
        for(int i=0; i<2; i++)
        {
          ostringstream &s=(i==0)?select:group_by;
          if (pr_class)       s << ", DECODE(pax_grp.status,'E',' ',pax_grp.class)"
                                << (i==0?" AS class":"");
          if (pr_cl_grp)      s << ", DECODE(pax_grp.status,'E',1000000000,pax_grp.class_grp)"
                                << (i==0?" AS class_grp":"");
          if (pr_hall)        s << ", NVL(bag2.hall, DECODE(bag2.is_trfer, 0, bag2.hall, DECODE(bag2.handmade, 0, 1000000000, bag2.hall)))"
                                << (i==0?" AS hall":"");
          if (pr_airp_arv)    s << ", pax_grp.point_arv";
          if (pr_trfer)       s << ", last_trfer.airline"
                                << ", last_trfer.flt_no"
                                << ", last_trfer.suffix"
                                << ", last_trfer.airp_arv";
          if (pr_user)        s << ", NVL(bag2.user_id, DECODE(bag2.is_trfer, 0, bag2.user_id, DECODE(bag2.handmade, 0, 1000000000, bag2.user_id)))"
                                << (i==0?" AS user_id":"");
          if (pr_client_type) s << ", pax_grp.client_type";
          if (pr_status)      s << ", pax_grp.status";
        };

        sql << "SELECT SUM(DECODE(bag2.pr_cabin,0,bag2.amount,0)) AS bag_amount, " << endl
            << "       SUM(DECODE(bag2.pr_cabin,0,bag2.weight,0)) AS bag_weight, " << endl
            << "       SUM(DECODE(bag2.pr_cabin,0,0,bag2.weight)) AS rk_weight, " << endl
            << "       " << select.str().erase(0,1) << endl
            << "FROM pax_grp,bag2 " << endl;

        if (pr_trfer) sql << "    ," << endl
                          << last_trfer_sql.str() << endl;

        sql << "WHERE pax_grp.grp_id=bag2.grp_id AND " << endl
            << "      pax_grp.point_dep=:point_id AND " << endl
            << "      " << crew_filter << endl
            << "      ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 " << endl;
        if (pr_trfer) sql << "      AND pax_grp.grp_id=last_trfer.grp_id(+) " << endl;
      };
      if (pass==5)
      {
        //����� �� ���⭮�� ������
        for(int i=0; i<2; i++)
        {
          ostringstream &s=(i==0)?select:group_by;
          if (pr_class)       s << ", DECODE(pax_grp.status,'E',' ',pax_grp.class)"
                                << (i==0?" AS class":"");
          if (pr_cl_grp)      s << ", DECODE(pax_grp.status,'E',1000000000,pax_grp.class_grp)"
                                << (i==0?" AS class_grp":"");
          if (pr_hall)        s << ", DECODE(bag2.grp_id, NULL, pax_grp.hall, NVL(bag2.hall, DECODE(bag2.is_trfer, 0, pax_grp.hall, DECODE(bag2.handmade, 0, 1000000000, pax_grp.hall))))"
                                << (i==0?" AS hall":"");
          if (pr_airp_arv)    s << ", pax_grp.point_arv";
          if (pr_trfer)       s << ", last_trfer.airline"
                                << ", last_trfer.flt_no"
                                << ", last_trfer.suffix"
                                << ", last_trfer.airp_arv";
          if (pr_user)        s << ", DECODE(bag2.grp_id, NULL, pax_grp.user_id, NVL(bag2.user_id, DECODE(bag2.is_trfer, 0, pax_grp.user_id, DECODE(bag2.handmade, 0, 1000000000, pax_grp.user_id))))"
                                << (i==0?" AS user_id":"");
          if (pr_client_type) s << ", pax_grp.client_type";
          if (pr_status)      s << ", pax_grp.status";
        };

        sql << "SELECT SUM(excess_wt) AS excess_wt, " << endl
            << "       SUM(excess_pc) AS excess_pc, " << endl
            << "       " << select.str().erase(0,1) << endl
            << "FROM pax_grp " << endl;

        if (pr_trfer) sql << "    ," << endl
                          << last_trfer_sql.str() << endl;

        if (pr_hall || pr_user)
          sql << "    ,(SELECT bag2.grp_id,bag2.hall,bag2.user_id,bag2.is_trfer,bag2.handmade " << endl
              << "     FROM bag2, " << endl
              << "          (SELECT bag2.grp_id,MAX(bag2.num) AS num " << endl
              << "           FROM pax_grp,bag2 " << endl
              << "           WHERE pax_grp.grp_id=bag2.grp_id AND pax_grp.point_dep=:point_id " << endl
              << "           GROUP BY bag2.grp_id) last_bag " << endl
              << "     WHERE bag2.grp_id=last_bag.grp_id AND bag2.num=last_bag.num) bag2 " << endl;

        sql << "WHERE pax_grp.point_dep=:point_id AND " << endl
            << "      " << crew_filter << endl
            << "      pax_grp.bag_refuse=0 " << endl;
        if (pr_trfer)
          sql << "      AND pax_grp.grp_id=last_trfer.grp_id(+) " << endl;
        if (pr_hall || pr_user)
          sql << "      AND pax_grp.grp_id=bag2.grp_id(+) " << endl;
      };

      sql << "GROUP BY " << group_by.str().erase(0,1) << endl;

      Qry.Clear();
      Qry.SQLText = sql.str().c_str();
      Qry.CreateVariable("point_id",otInteger,point_id);
      if (pass==3)
      {
        Qry.CreateVariable("adult",otString,EncodePerson(ASTRA::adult));
        Qry.CreateVariable("child",otString,EncodePerson(ASTRA::child));
        Qry.CreateVariable("baby",otString,EncodePerson(ASTRA::baby));
      };

      //ProgTrace(TRACE5, "readPaxLoad: SQL=%s", sql.str().c_str());

      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        TPaxLoadItem item;
        if (pr_class) item.class_id=Qry.FieldAsString("class");
        if (pr_cl_grp && !Qry.FieldIsNULL("class_grp")) item.cls_grp_id=Qry.FieldAsInteger("class_grp");
        if (pr_hall && !Qry.FieldIsNULL("hall")) item.hall_id=Qry.FieldAsInteger("hall");
        if (pr_airp_arv) item.point_arv=Qry.FieldAsInteger("point_arv");
        if (pr_trfer)
        {
          item.trfer_airline_id=Qry.FieldAsString("airline");
          if (!Qry.FieldIsNULL("flt_no"))
            item.trfer_flt_no=Qry.FieldAsInteger("flt_no");
          item.trfer_suffix_id=Qry.FieldAsString("suffix");
          item.trfer_airp_arv_id=Qry.FieldAsString("airp_arv");
        };
        if (pr_user && !Qry.FieldIsNULL("user_id")) item.user_id=Qry.FieldAsInteger("user_id");
        if (pr_client_type) item.client_type_id=Qry.FieldAsString("client_type");
        if (pr_status) item.grp_status_id=Qry.FieldAsString("status");
        if (pr_ticket_rem) item.ticket_rem=Qry.FieldAsString("ticket_rem");
        if (pr_rems) item.rem_code=Qry.FieldAsString("rem_code");

        if (pass==1)
        {
          item.cfg=Qry.FieldAsInteger("cfg");
        };
        if (pass==2)
        {
          item.crs_ok=Qry.FieldAsInteger("crs_ok");
          item.crs_tranzit=Qry.FieldAsInteger("crs_tranzit");
        };
        if (pass==3)
        {
          item.seats=Qry.FieldAsInteger("seats");
          item.adult_m=Qry.FieldAsInteger("adult_m");
          item.adult_f=Qry.FieldAsInteger("adult_f");
          item.child=Qry.FieldAsInteger("child");
          item.baby=Qry.FieldAsInteger("baby");
        };
        if (pass==4)
        {
          item.rk_weight=Qry.FieldAsInteger("rk_weight");
          item.bag_amount=Qry.FieldAsInteger("bag_amount");
          item.bag_weight=Qry.FieldAsInteger("bag_weight");
        };
        if (pass==5)
        {
          item.excess_wt=Qry.FieldAsInteger("excess_wt");
          item.excess_pc=Qry.FieldAsInteger("excess_pc");
        };

        list<TPaxLoadItem>::iterator i=find(paxLoad.begin(), paxLoad.end(), item);
        if (i!=paxLoad.end())
          *i+=item;
        else
          paxLoad.push_back(item);
      };
    };

    //������塞 �� �������騥 ���� TPaxLoadItem

    TQuery HallsQry(&OraSession);
    HallsQry.Clear();
    HallsQry.SQLText="SELECT airp FROM halls2 WHERE id=:id";
    HallsQry.DeclareVariable("id",otInteger);
    map<int, pair<string,string> > halls; //����㥬 ���ଠ�� �� �����

    TQuery PointsQry(&OraSession);
    PointsQry.Clear();
    PointsQry.SQLText="SELECT airp,point_num FROM points WHERE point_id=:point_id AND pr_del>=0";
    PointsQry.DeclareVariable("point_id",otInteger);
    map<int, pair<string,int> > points; //����㥬 ���ଠ�� �� �㭪⠬

    TQuery UsersQry(&OraSession);
    UsersQry.Clear();
    UsersQry.SQLText="SELECT descr FROM users2 WHERE user_id=:user_id";
    UsersQry.DeclareVariable("user_id",otInteger);
    map<int, string> users; //����㥬 ���ଠ�� �� ����⠬

    for(list<TPaxLoadItem>::iterator i=paxLoad.begin();i!=paxLoad.end();i++)
    {
      if (!i->class_id.empty())
      {
        if (i->class_id!=CREW_CLASS_ID)
        {
          i->class_view=ElemIdToCodeNative(etClass,i->class_id);
          try
          {
            i->class_priority=getBaseTable(etClass).get_row("code",i->class_id,true).AsInteger("priority");
          }
          catch(EBaseTableError) { throw; };
        }
        else
        {
          i->class_view=CREW_CLASS_VIEW;
          i->class_priority=200;
        };
      }
      else i->class_priority=100;

      if (i->cls_grp_id!=NoExists)
      {
        if (i->cls_grp_id!=CREW_CLS_GRP_ID)
        {
          i->cls_grp_view=ElemIdToCodeNative(etClsGrp,i->cls_grp_id);
          try
          {
            i->cls_grp_priority=getBaseTable(etClsGrp).get_row("id",i->cls_grp_id,true).AsInteger("priority");
          }
          catch(EBaseTableError) { throw; };
        }
        else
        {
          i->cls_grp_view=CREW_CLS_GRP_VIEW;
          i->cls_grp_priority=200;
        };
      }
      else i->cls_grp_priority=100;

      if (i->hall_id!=NoExists)
      {
        map<int, pair<string,string> >::const_iterator h=halls.find(i->hall_id);
        if (h!=halls.end())
        {
          i->hall_airp_view=h->second.first;
          i->hall_name_view=h->second.second;
        }
        else
        {
          if (i->hall_id!=1000000000)
          {
            HallsQry.SetVariable("id",i->hall_id);
            HallsQry.Execute();
            if (!HallsQry.Eof)
            {
              if (HallsQry.FieldAsString("airp")!=fltInfo.airp)
                i->hall_airp_view=ElemIdToCodeNative(etAirp, HallsQry.FieldAsString("airp"));
              else
                i->hall_airp_view="";

              i->hall_name_view=ElemIdToNameLong(etHall, i->hall_id);
            };
          }
          else
          {
            i->hall_airp_view="";
            i->hall_name_view=getLocaleText("�࠭���");
          };
          halls[i->hall_id]=make_pair(i->hall_airp_view,i->hall_name_view);
        };
      };
      if (i->point_arv!=NoExists)
      {
        map<int, pair<string,int> >::const_iterator p=points.find(i->point_arv);
        if (p!=points.end())
        {
          i->airp_arv_view=p->second.first;
          i->airp_arv_priority=p->second.second;
        }
        else
        {
          PointsQry.SetVariable("point_id",i->point_arv);
          PointsQry.Execute();
          if (!PointsQry.Eof)
          {
            i->airp_arv_view=ElemIdToCodeNative(etAirp, PointsQry.FieldAsString("airp"));
            i->airp_arv_priority=PointsQry.FieldAsInteger("point_num");
          };
          points[i->point_arv]=make_pair(i->airp_arv_view,i->airp_arv_priority);
        };
      };
      TLastTrferInfo trferInfo;
      trferInfo.airline=i->trfer_airline_id;
      trferInfo.flt_no=i->trfer_flt_no;
      trferInfo.suffix=i->trfer_suffix_id;
      trferInfo.airp_arv=i->trfer_airp_arv_id;
      i->trfer_view=trferInfo.str();
      if (i->user_id!=NoExists)
      {
        map<int, string>::const_iterator u=users.find(i->user_id);
        if (u!=users.end())
        {
          i->user_view=u->second;
        }
        else
        {
          if (i->user_id!=1000000000)
          {
            UsersQry.SetVariable("user_id",i->user_id);
            UsersQry.Execute();
            if (!UsersQry.Eof)
            {
              i->user_view=UsersQry.FieldAsString("descr");
            };
          }
          else i->user_view=getLocaleText("�࠭���");

          users[i->user_id]=i->user_view;
        };
      };
      if (!i->client_type_id.empty())
      {
        i->client_type_view=ElemIdToNameShort(etClientType,i->client_type_id);
        try
        {
          i->client_type_priority=getBaseTable(etClientType).get_row("code",i->client_type_id,true).AsInteger("priority");
        }
        catch(EBaseTableError) { throw; };
      };
      if (!i->grp_status_id.empty())
      {
        i->grp_status_view=ElemIdToNameLong(etGrpStatusType,i->grp_status_id);
        try
        {
          i->grp_status_priority=getBaseTable(etGrpStatusType).get_row("code",i->grp_status_id,true).AsInteger("priority");
        }
        catch(EBaseTableError) { throw; };
      };
    };
  }
  else
  {
    readPaxZoneLoad( point_id, crew_filter, paxLoad );
  };

  //����㥬 ���ᨢ
  paxLoad.sort(paxLoadOrder);

  xmlNodePtr node;
  for(list<TPaxLoadItem>::iterator i=paxLoad.begin();i!=paxLoad.end();i++)
  {
    rowNode=NewTextChild(rowsNode,"row");
    NewTextChild(rowNode,"seats",i->seats,0);
    NewTextChild(rowNode,"adult_m",i->adult_m,0);
    NewTextChild(rowNode,"adult_f",i->adult_f,0);
    NewTextChild(rowNode,"child",i->child,0);
    NewTextChild(rowNode,"baby",i->baby,0);

    if (!pr_ticket_rem && !pr_rems)
    {
      NewTextChild(rowNode,"bag_amount",i->bag_amount,0);
      NewTextChild(rowNode,"bag_weight",i->bag_weight,0);
      NewTextChild(rowNode,"rk_weight",i->rk_weight,0);
      if (!pr_section)
      {
        excessNodeList.add(rowNode, "excess", i->excess_pc, i->excess_wt);

        if (!pr_cl_grp && !pr_trfer && !pr_client_type && !pr_status && !pr_hall && !pr_user)
        {
          NewTextChild(rowNode,"crs_ok",i->crs_ok,0);
          NewTextChild(rowNode,"crs_tranzit",i->crs_tranzit,0);
          if (!pr_airp_arv)
            NewTextChild(rowNode,"cfg",i->cfg,0);
        };
      };
    };

    if (pr_class)
    {
      NewTextChild(rowNode,"class",i->class_view);
      if (i->class_id.empty())
        ReplaceTextChild(rowNode,"title",AstraLocale::getLocaleText("��ᮯ�"));
      if (i->class_id==CREW_CLASS_ID)
        ReplaceTextChild(rowNode,"title",AstraLocale::getLocaleText("������"));
    };
    if (pr_cl_grp)
    {
      if (i->cls_grp_id!=NoExists)
      {
        node=NewTextChild(rowNode,"cl_grp",i->cls_grp_view);
        SetProp(node,"id",i->cls_grp_id);
        if (i->cls_grp_id==CREW_CLS_GRP_ID)
          ReplaceTextChild(rowNode,"title",AstraLocale::getLocaleText("������"));
      }
      else
      {
        node=NewTextChild(rowNode,"cl_grp");
        SetProp(node,"id",0);
        ReplaceTextChild(rowNode,"title",AstraLocale::getLocaleText("��ᮯ�"));
      };
    };
    if (pr_hall)
    {
      if (i->hall_id!=NoExists)
      {
        ostringstream hall_name;
        hall_name << i->hall_name_view;
        if (!i->hall_airp_view.empty())
          hall_name << "(" << i->hall_airp_view << ")";
        node=NewTextChild(rowNode,"hall",hall_name.str());
        SetProp(node,"id",i->hall_id);
      }
      else
      {
        node=NewTextChild(rowNode,"hall");
        SetProp(node,"id",-1);
      };
    };
    if (pr_airp_arv)
    {
      node=NewTextChild(rowNode,"airp_arv",i->airp_arv_view);
      SetProp(node,"id",i->point_arv);
    };
    if (pr_trfer)
    {
      NewTextChild(rowNode,"trfer",i->trfer_view);
    };
    if (pr_user)
    {
      if (i->user_id!=NoExists)
      {
        node=NewTextChild(rowNode,"user",i->user_view);
        SetProp(node,"id",i->user_id);
      }
      else
      {
        node=NewTextChild(rowNode,"user");
        SetProp(node,"id",-1);
      };
    };
    if (pr_client_type)
    {
      node=NewTextChild(rowNode,"client_type",i->client_type_view);
      SetProp(node,"id",(int)DecodeClientType(i->client_type_id.c_str()));
    };
    if (pr_status)
    {
      node=NewTextChild(rowNode,"status",i->grp_status_view);
      TPaxStatus status_id=DecodePaxStatus(i->grp_status_id.c_str());
      SetProp(node,"id",(int)status_id);
      if (status_id==psCrew)
        ReplaceTextChild(rowNode,"title",AstraLocale::getLocaleText("������"));
    };
    if (pr_ticket_rem)
    {
      NewTextChild(rowNode,"ticket_rem",i->ticket_rem);
    };
    if (pr_rems)
    {
      NewTextChild(rowNode,"rems",i->rem_code);
    };
    if (pr_section)
    {
      NewTextChild(rowNode,"section",i->section);
    };
  };
}

void PaxLoadtoXML(
        xmlNodePtr datasetsNode,
        const map<string, pair<int, int> > &bag_info,   // bagsData ���ଠ�� �� ������
        const map<string,int> &paxRemCounters,          // remsData ���ଠ�� �� ���ᠬ
        const map<string, pair<int, int> > &bags        // ��騥 �����
        )
{
    size_t rowcount = bag_info.size();
    if(rowcount < paxRemCounters.size())
        rowcount = paxRemCounters.size();

    map<string, pair<int, int> >::const_iterator bag_info_i = bag_info.begin();
    map<string,int>::const_iterator paxRemCounters_i = paxRemCounters.begin();
    map<string, pair<int, int> >::const_iterator bags_i = bags.begin();

    vector<vector<string> > result;
    for(size_t i = 0; i < rowcount; i++) {
        vector<string> row;
        if(paxRemCounters_i == paxRemCounters.end()) {
            row.push_back("");
            row.push_back("");
        } else {
            row.push_back(paxRemCounters_i->first);
            row.push_back(IntToString(paxRemCounters_i->second));
            paxRemCounters_i++;
        }

        if(bag_info_i == bag_info.end()) {
                row.push_back("");
                row.push_back("");
                row.push_back("");
        } else {
            row.push_back(bag_info_i->first);
            row.push_back(IntToString(bag_info_i->second.first));
            row.push_back(IntToString(bag_info_i->second.second));
            bag_info_i++;
        }
        result.push_back(row);
    }

    vector<string> row;
    row.push_back("");
    row.push_back("");
    row.push_back("");
    row.push_back("");
    row.push_back("");
    result.push_back(row);
    row.clear();
    row.push_back("");
    row.push_back("");
    row.push_back("��騥 �����");
    row.push_back("");
    row.push_back("");
    result.push_back(row);

    int amount = 0;
    int weight = 0;

    for ( const auto& i : bags ) {
        if ( i.first.empty() ) {
            continue;
        }
        vector<string> row;
        row.push_back("");
        row.push_back("");
        row.push_back(i.first);
        row.push_back(IntToString(i.second.first));
        row.push_back(IntToString(i.second.second));
        result.push_back(row);
        amount += i.second.first;
        weight += i.second.second;
    }

    row.clear();
    row.push_back("");
    row.push_back("");
    row.push_back("�ᥣ�");
    row.push_back(IntToString(amount));
    row.push_back(IntToString(weight));
    result.push_back(row);

    xmlNodePtr node = NewTextChild( datasetsNode, "multiData" );
    for(const auto &i_table : result) {
        xmlNodePtr rowNode=NewTextChild(node,"row");
        int col_idx = 0;
        for(const auto &i_row : i_table) {
            ostringstream col_name;
            col_name << "col" << col_idx++;
            NewTextChild(rowNode, col_name.str().c_str(), i_row);
        }
    }
}

void viewPaxLoadSectionReport(int point_id, xmlNodePtr resNode )
{
  //header

  list<TPaxLoadItem> paxLoad;
  //������ ���� �� ���ᠦ�ࠬ + ���-�� ६�ப XCR, DHC, MOS, UMNR, EXST, WCHC, WCHR, WCHS
  std::map<std::string,int> paxRemCounters;
  paxRemCounters[ "XCR" ] = 0;
  paxRemCounters[ "DHC" ] = 0;
  paxRemCounters[ "MOS" ] = 0;
  paxRemCounters[ "UMNR" ] = 0;
  paxRemCounters[ "EXST" ] = 0;
  paxRemCounters[ "WCHC" ] = 0;
  paxRemCounters[ "WCHR" ] = 0;
  paxRemCounters[ "WCHS" ] = 0;
  readPaxZoneLoad( point_id, createCrewFilter(), paxLoad, paxRemCounters );
  map<string, pair<int, int> > bag_info;
  //������ ६�ப ������
  get_bag_info(bag_info, point_id);
  TPaxLoadOrder paxLoadOrder;
  paxLoadOrder.fields.push_back( "section" );
  paxLoad.sort(paxLoadOrder);
  //////////////////////HEADER////////////////////////////////////////////////

  TQuery Qry( &OraSession );
  //TQuery StagesQry( &OraSession );
  Qry.SQLText =
    "SELECT " + TTripInfo::selectedFields() + "litera, bort, trip_type "
    "FROM points WHERE point_id=:point_id AND pr_del>=0";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if ( Qry.Eof ) {
    throw AstraLocale::UserException( "MSG.FLIGHT.NOT_FOUND" );
  }

  TTripInfo info(Qry);
  string litera=Qry.FieldAsString("litera");
  string bort=Qry.FieldAsString("bort");
  string trip_type=Qry.FieldAsString("trip_type");

  xmlNodePtr node = NewTextChild( resNode, "tripheader" );
  NewTextChild( node, "point_id", info.point_id );

  const string &tz_region=AirpTZRegion(info.airp);

  NewTextChild( node, "scd_out", DateTimeToStr(UTCToClient(info.scd_out,tz_region), "hh:nn") );
  NewTextChild( node, "est_out", info.est_out_exists()?DateTimeToStr(UTCToClient(*info.est_out,tz_region), "hh:nn") : "" );
  NewTextChild( node, "act_out", info.act_out_exists()?DateTimeToStr(UTCToClient(*info.act_out,tz_region), "hh:nn") : "" );
  NewTextChild( node, "craft", ElemIdToElemCtxt(ecCkin,etCraft, info.craft, info.craft_fmt));
  NewTextChild( node, "bort", bort);
  NewTextChild( node, "classes", TCFG(point_id).str() );
  string route=GetRouteAfterStr(NoExists, point_id, trtWithCurrent, trtNotCancelled);
  NewTextChild( node, "route", route );
  NewTextChild( node, "places", route );
  NewTextChild( node, "trip_type", ElemIdToCodeNative(etTripType,trip_type) );
  NewTextChild( node, "litera", litera );
  //trip �㦥� ��� ChangeTrip ������:
  NewTextChild( node, "trip", GetTripName(info,ecCkin,false,true));

  TTripStages tripStages( point_id );
  TStagesRules *stagesRules = TStagesRules::Instance();
  NewTextChild( node, "status", stagesRules->status_view( stCheckIn, tripStages.getStage( stCheckIn ) ));
  string stralarms;
  BitSet<Alarm::Enum> Alarms;
  tst();
  TripAlarms( point_id, Alarms );
  for(Alarm::TPairs::const_iterator a=Alarm::pairs().begin(); a!=Alarm::pairs().end(); ++a)
  {
    tst();
    Alarm::Enum alarm = a->first;
    string rem;
    if ( !Alarms.isFlag( alarm ) )
      continue;
    switch( alarm ) {
      case Alarm::Waitlist:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::Overload:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::Salon:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::DiffComps:
        rem = TripAlarmString( alarm ) + SALONS2::getDiffCompsAlarmRoutes( point_id );
        break;
      case Alarm::Brd:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::ETStatus:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::TlgIn:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::TlgOut:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::APISIncomplete:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::APISManualInput:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::SpecService:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::UnattachedTrfer:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::ConflictTrfer:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::CrewCheckin:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::CrewNumber:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::CrewDiff:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::APISDiffersFromBooking:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::UnboundEMD:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::APPSProblem:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::APPSOutage:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::APPSConflict:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::APPSNegativeDirective:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::APPSError:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::WBDifferLayout:
        rem = TripAlarmString( alarm );
        break;
      case Alarm::WBDifferSeats:
        rem = TripAlarmString( alarm );
        break;
      default:
        break;
    }
    if ( !rem.empty() ) {
      if ( !stralarms.empty() )
          stralarms += " ";
      stralarms += "!" + rem;
    }
  }
  NewTextChild( node, "alarms", stralarms );

  ostringstream tripheader;
  tripheader
      << getLocaleText("����") << " "
      << GetTripName(info,ecCkin,false,true) << " "
      << route << " "
      << getLocaleText("��") << "/" << ElemIdToElemCtxt(ecCkin,etCraft, info.craft, info.craft_fmt) << " "
      << getLocaleText("����") << "/" << bort << " "
      << getLocaleText("����������") << "/" << TCFG(point_id).str() << " "
      << getLocaleText("���.") << "/" << DateTimeToStr(UTCToClient(info.scd_out,tz_region), "hh:nn") << " "
      << getLocaleText("����") << "/"
      << (info.est_out_exists()?DateTimeToStr(UTCToClient(*info.est_out,tz_region), "hh:nn") : "")
      << " "
      << getLocaleText("����") << "/"
      << (info.act_out_exists()?DateTimeToStr(UTCToClient(*info.act_out,tz_region), "hh:nn") : "")
      << endl
      << getLocaleText("�����") << ": " << stagesRules->status_view( stCheckIn, tripStages.getStage( stCheckIn ) ) << endl
      << getLocaleText("�ॢ���") << ": " << stralarms;

  NewTextChild( node, "tripheader", tripheader.str() );


  //////////////////////////////////////////////////////////////////////////////
  xmlNodePtr datasetsNode = NewTextChild(resNode, "datasets");
  node = NewTextChild( datasetsNode, "sectionsData" );
  PaxLoadTotalRowStruct total;
  Qry.Clear();
  createReadPaxLoadTotalRow( Qry, point_id, total );
  xmlNodePtr rowNode=NewTextChild(node,"row");
  NewTextChild(rowNode,"seats",total.seats);
  NewTextChild(rowNode,"adult_m",total.adult_m);
  NewTextChild(rowNode,"adult_f",total.adult_f);
  NewTextChild(rowNode,"child",total.child);
  NewTextChild(rowNode,"baby",total.baby);
  NewTextChild(rowNode,"bag_amount",total.bag_amount);
  NewTextChild(rowNode,"bag_weight",total.bag_weight);
  NewTextChild(rowNode,"rk_weight",total.rk_weight);
  NewTextChild(rowNode,"crs_ok",total.crs_ok);
  NewTextChild(rowNode,"crs_tranzit",total.crs_tranzit);
  NewTextChild(rowNode,"excess",TComplexBagExcess(total.excess_pc, total.excess_wt).view(OutputLang(), false));
  NewTextChild(rowNode,"section","");
  NewTextChild(rowNode,"cfg",total.cfg,0);
  for ( const auto& i : paxLoad ) {
    xmlNodePtr rowNode=NewTextChild(node,"row");
    NewTextChild(rowNode,"seats",i.seats);
    NewTextChild(rowNode,"adult_m",i.adult_m);
    NewTextChild(rowNode,"adult_f",i.adult_f);
    NewTextChild(rowNode,"child",i.child);
    NewTextChild(rowNode,"baby",i.baby);
    NewTextChild(rowNode,"bag_amount",i.bag_amount);
    NewTextChild(rowNode,"bag_weight",i.bag_weight);
    NewTextChild(rowNode,"rk_weight",i.rk_weight);
    NewTextChild(rowNode,"crs_ok","");
    NewTextChild(rowNode,"crs_tranzit","");
    NewTextChild(rowNode,"excess","");
    NewTextChild(rowNode,"section",i.section);
  };

  node = NewTextChild( datasetsNode, "bagsData" );
  for ( const auto& i : bag_info ) {
    xmlNodePtr rowNode=NewTextChild(node,"row");
    NewTextChild(rowNode,"rem",i.first);
    NewTextChild(rowNode,"amount",i.second.first);
    NewTextChild(rowNode,"weight",i.second.second);
  }
  node = NewTextChild( datasetsNode, "remsData" );
  for ( const auto& i : paxRemCounters ) {
    xmlNodePtr rowNode=NewTextChild(node,"row");
    NewTextChild(rowNode,"rem",i.first);
    NewTextChild(rowNode,"count",i.second);
  }
  node = NewTextChild( datasetsNode, "bagsClassData" );
  for ( const auto& i : total.bags ) {
    if ( i.first.empty() ) {
      continue;
    }
    xmlNodePtr rowNode=NewTextChild(node,"row");
    NewTextChild(rowNode,"class",i.first);
    NewTextChild(rowNode,"amount",i.second.first);
    NewTextChild(rowNode,"weight",i.second.second);
  }

  PaxLoadtoXML(datasetsNode, bag_info, paxRemCounters, total.bags);
}

void viewCRSList( int point_id, xmlNodePtr dataNode )
{
  bool pr_free_seating = SALONS2::isFreeSeating( point_id );
  TGrpStatusTypes &grp_status_types = (TGrpStatusTypes &)base_tables.get("GRP_STATUS_TYPES");
  TQuery Qry( &OraSession );
  TPaxSeats priorSeats( point_id );
  Qry.Clear();
  ostringstream sql;

  sql <<
     "SELECT "
     "      crs_pnr.status AS pnr_status, "
     "      crs_pnr.priority AS pnr_priority, "
     "      RTRIM(crs_pax.surname||' '||crs_pax.name) full_name, "
     "      crs_pax.pers_type, "
     "      crs_pnr.class, "
     "      crs_pnr.subclass, "
     "      NVL(crs_pax.etick_class, NVL(crs_pax.orig_class, crs_pnr.class)) AS orig_class, "
     "      NVL(crs_pax.etick_subclass, NVL(crs_pax.orig_subclass, crs_pnr.subclass)) AS orig_subclass, "
     "      crs_pax.seat_xname, "
     "      crs_pax.seat_yname, "
     "      crs_pax.seats seats, "
     "      crs_pnr.airp_arv, "
     "      crs_pnr.airp_arv_final, "
     "      report.get_PSPT(crs_pax.pax_id, 1, :lang) AS document, "
     "      report.get_TKNO(crs_pax.pax_id) AS ticket, "
     "      crs_pax.pax_id, "
     "      crs_pax.tid tid, "
     "      crs_pnr.pnr_id, "
     "      crs_pnr.point_id AS point_id_tlg, "
     "      ids.status, "
     "      pax.reg_no, "
     "      pax.seats pax_seats, "
     "      pax_grp.status grp_status, "
     "      pax_grp.client_type, "
     "      pax.refuse, "
     "      pax.grp_id, "
     "      pax.wl_type, "
     "      pax.is_jmp "
     "FROM crs_pnr,crs_pax,pax,pax_grp,"
     "       ( ";


  sql << getSearchPaxSubquery(psCheckin, true, false, false, false, "")
      << "UNION \n"
      << getSearchPaxSubquery(psGoshow,  true, false, false, false, "")
      << "UNION \n"
      << getSearchPaxSubquery(psTransit, true, false, false, false, "");

  sql <<
     "       ) ids "
     "WHERE crs_pnr.pnr_id=ids.pnr_id AND "
     "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
     "      crs_pax.pax_id=pax.pax_id(+) AND "
     "      pax.grp_id=pax_grp.grp_id(+) AND "
     "      crs_pax.pr_del=0 "
     "ORDER BY crs_pnr.point_id";

  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "ps_ok", otString, EncodePaxStatus(ASTRA::psCheckin) );
  Qry.CreateVariable( "ps_goshow", otString, EncodePaxStatus(ASTRA::psGoshow) );
  Qry.CreateVariable( "ps_transit", otString, EncodePaxStatus(ASTRA::psTransit) );
  Qry.CreateVariable( "lang", otString, TReqInfo::Instance()->desk.lang );
  Qry.Execute();
  // ���� ���ᠦ��
  TQuery SQry( &OraSession );
  SQry.SQLText =
    "BEGIN "
    " IF :mode=0 THEN "
    "  :seat_no:=salons.get_seat_no(:pax_id,:seats,:is_jmp,:layer_type,:point_id,'_seats',:pax_row); "
    " ELSE "
    "  :seat_no:=salons.get_crs_seat_no(:pax_id,:xname,:yname,:seats,:point_id,:layer_type,'_seats',:crs_row); "
    " END IF; "
    "END;";
  SQry.DeclareVariable( "mode", otInteger );
  SQry.DeclareVariable( "pax_id", otInteger );
  SQry.DeclareVariable( "xname", otString );
  SQry.DeclareVariable( "yname", otString );
  SQry.DeclareVariable( "layer_type", otString );
  SQry.DeclareVariable( "seats", otInteger );
  SQry.DeclareVariable( "point_id", otInteger );
  SQry.DeclareVariable( "pax_row", otInteger );
  SQry.DeclareVariable( "crs_row", otInteger );
  SQry.DeclareVariable( "seat_no", otString );
  SQry.DeclareVariable( "is_jmp", otInteger );

  //३� ���ᠦ�஢
  TQuery TlgTripsQry( &OraSession );
  TlgTripsQry.SQLText=
    "SELECT airline,flt_no,suffix,scd,airp_dep "
    "FROM tlg_trips WHERE point_id=:point_id ";
  TlgTripsQry.DeclareVariable("point_id",otInteger);

  TQuery PointsQry( &OraSession );
  PointsQry.SQLText=
    "SELECT point_dep AS point_id FROM pax_grp WHERE grp_id=:grp_id";
  PointsQry.DeclareVariable("grp_id",otInteger);

  xmlNodePtr tripsNode = NewTextChild( dataNode, "tlg_trips" );
  Qry.Execute();
  if (Qry.Eof) return;

  string def_pers_type=EncodePerson(ASTRA::adult); //ᯥ樠�쭮 �� ��४����㥬, ⠪ ��� ���� ������ �� ⨯��
  string def_class=ElemIdToCodeNative(etClass, EncodeClass(ASTRA::Y));
  string def_status=EncodePaxStatus(ASTRA::psCheckin);

  xmlNodePtr defNode = NewTextChild( dataNode, "defaults" );
  NewTextChild(defNode, "pnr_ref", "");
  NewTextChild(defNode, "pnr_status", "");
  NewTextChild(defNode, "pnr_priority", "");
  NewTextChild(defNode, "pers_type", def_pers_type);
  NewTextChild(defNode, "class", def_class);
  NewTextChild(defNode, "seats", 1);
  NewTextChild(defNode, "last_target", "");
  NewTextChild(defNode, "client_type");
  NewTextChild(defNode, "ticket", "");
  NewTextChild(defNode, "document", "");
  NewTextChild(defNode, "status", def_status);
  NewTextChild(defNode, "rems", "");
  NewTextChild(defNode, "rem", "");
  NewTextChild(defNode, "nseat_no", "");
  NewTextChild(defNode, "wl_type", "");
  NewTextChild(defNode, "layer_type", "");
  NewTextChild(defNode, "isseat", 1);
  NewTextChild(defNode, "reg_no", "");
  NewTextChild(defNode, "refuse", "");
  NewTextChild(defNode, "brand", "");
  NewTextChild(defNode, "fqt", "");
  bool apis_generation = TRouteAPICheckInfo(point_id).apis_generation();
  if ( apis_generation ) {
    NewTextChild(defNode, "apis", "");
  }

  TRemGrp rem_grp;
  rem_grp.Load(retPNL_SEL, point_id);

  TTripInfo flt;
  flt.getByPointId(point_id);

  int point_id_tlg=-1;
  xmlNodePtr tripNode=NULL,paxNode=NULL,node=NULL;
  int col_pnr_status=Qry.FieldIndex("pnr_status");
  int col_pnr_priority=Qry.FieldIndex("pnr_priority");
  int col_full_name=Qry.FieldIndex("full_name");
  int col_pers_type=Qry.FieldIndex("pers_type");
  int col_class=Qry.FieldIndex("class");
  int col_subclass=Qry.FieldIndex("subclass");
  int col_orig_class=Qry.FieldIndex("orig_class");
//  int col_orig_subclass=Qry.FieldIndex("orig_subclass");
  int col_seat_xname=Qry.FieldIndex("seat_xname");
  int col_seat_yname=Qry.FieldIndex("seat_yname");
  int col_seats=Qry.FieldIndex("seats");
  int col_airp_arv=Qry.FieldIndex("airp_arv");
  int col_airp_arv_final=Qry.FieldIndex("airp_arv_final");
  int col_document=Qry.FieldIndex("document");
  int col_ticket=Qry.FieldIndex("ticket");
  int col_pax_id=Qry.FieldIndex("pax_id");
  int col_tid=Qry.FieldIndex("tid");
  int col_pnr_id=Qry.FieldIndex("pnr_id");
  int col_point_id_tlg=Qry.FieldIndex("point_id_tlg");
  int col_status=Qry.FieldIndex("status");
  int col_reg_no=Qry.FieldIndex("reg_no");
  int col_refuse=Qry.FieldIndex("refuse");
  int col_grp_id=Qry.FieldIndex("grp_id");
  int col_grp_status=Qry.FieldIndex("grp_status");
  int col_client_type=Qry.FieldIndex("client_type");
  int col_pax_seats=Qry.FieldIndex("pax_seats");
  int col_wl_type=Qry.FieldIndex("wl_type");
  int col_is_jmp=Qry.FieldIndex("is_jmp");
  int mode; // ०�� ��� ���᪠ ���� 0 - ॣ������ ���� ᯨ᮪ pnl
  int crs_row=1, pax_row=1;
  TBrands brands; //���塞 �����, �⮡� ������⢮���� ���஢���� �७���
  for(;!Qry.Eof;Qry.Next())
  {
    mode = -1; // �� ���� �᪠�� ����
    string seat_no;
    if (!Qry.FieldIsNULL(col_grp_id))
    {
      PointsQry.SetVariable("grp_id",Qry.FieldAsInteger(col_grp_id));
      PointsQry.Execute();
      if (!PointsQry.Eof&&point_id!=PointsQry.FieldAsInteger("point_id")) continue;
    };

    if (point_id_tlg!=Qry.FieldAsInteger(col_point_id_tlg))
    {
      point_id_tlg=Qry.FieldAsInteger(col_point_id_tlg);
      tripNode = NewTextChild( tripsNode, "tlg_trip" );
      TlgTripsQry.SetVariable("point_id",point_id_tlg);
      TlgTripsQry.Execute();
      if (TlgTripsQry.Eof) throw AstraLocale::UserException("MSG.FLT.NOT_FOUND.REPEAT_QRY");
      ostringstream trip;
      trip << ElemIdToCodeNative(etAirline,TlgTripsQry.FieldAsString("airline") )
           << setw(3) << setfill('0') << TlgTripsQry.FieldAsInteger("flt_no")
           << ElemIdToCodeNative(etSuffix,TlgTripsQry.FieldAsString("suffix"))
           << "/" << DateTimeToStr(TlgTripsQry.FieldAsDateTime("scd"),"ddmmm",TReqInfo::Instance()->desk.lang!=AstraLocale::LANG_RU)
           << " " << ElemIdToCodeNative(etAirp,TlgTripsQry.FieldAsString("airp_dep"));
      NewTextChild(tripNode,"name",trip.str());
      paxNode = NewTextChild(tripNode,"passengers");
    };
    node = NewTextChild(paxNode,"pax");

    string pnr_addr=TPnrAddrs().firstAddrByPnrId(Qry.FieldAsInteger( col_pnr_id ), TPnrAddrInfo::AddrAndAirline);
    NewTextChild( node, "pnr_ref", pnr_addr, "" );
    NewTextChild( node, "pnr_status", Qry.FieldAsString( col_pnr_status ), "" );
    NewTextChild( node, "pnr_priority", Qry.FieldAsString( col_pnr_priority ), "" );
    NewTextChild( node, "full_name", Qry.FieldAsString( col_full_name ) );
    NewTextChild( node, "pers_type", Qry.FieldAsString( col_pers_type ), def_pers_type ); //ᯥ樠�쭮 �� ��४����㥬, ⠪ ��� ���� ������ �� ⨯��
    string cl=Qry.FieldAsString( col_class );
    string orig_cl=Qry.FieldAsString( col_orig_class );
    NewTextChild( node, "class", (cl!=orig_cl?ElemIdToCodeNative(etClass, orig_cl)+"->":"") +
                                 ElemIdToCodeNative(etClass, cl), def_class );
    string subcl=Qry.FieldAsString( col_subclass );
//    string orig_subcl=Qry.FieldAsString( col_orig_subclass );
    NewTextChild( node, "subclass", //(subcl!=orig_subcl?ElemIdToCodeNative(etSubcls, orig_subcl)+"->":"") +
                                    ElemIdToCodeNative(etSubcls, subcl) );
    NewTextChild( node, "seats", Qry.FieldAsInteger( col_seats ), 1 );
    NewTextChild( node, "target", ElemIdToCodeNative(etAirp,Qry.FieldAsString( col_airp_arv ) ));
    if (!Qry.FieldIsNULL(col_airp_arv_final))
    {
      try
      {
        const TAirpsRow &row=(const TAirpsRow&)(base_tables.get("airps").get_row("code/code_lat",Qry.FieldAsString( col_airp_arv_final )));
        NewTextChild( node, "last_target", ElemIdToCodeNative(etAirp,row.code));
      }
      catch(EBaseTableError)
      {
        NewTextChild( node, "last_target", Qry.FieldAsString( col_airp_arv_final ));
      };
    };

    NewTextChild( node, "client_type", ElemIdToNameShort(etClientType, Qry.FieldAsString(col_client_type)), "" );
    NewTextChild( node, "ticket", Qry.FieldAsString( col_ticket ), "" );
    NewTextChild( node, "document", Qry.FieldAsString( col_document ), "" );
    NewTextChild( node, "status", Qry.FieldAsString( col_status ), def_status );

    int pax_id=Qry.FieldAsInteger( col_pax_id );
    multiset<CheckIn::TPaxRemItem> rems;
    LoadCrsPaxRem(pax_id, rems);
    CheckIn::TPaxRemItem apps_satus_rem = getAPPSRem( pax_id, TReqInfo::Instance()->desk.lang );
    if ( !apps_satus_rem.empty() )
     rems.insert( apps_satus_rem );
    ostringstream rem_detail;
    xmlNodePtr stcrNode = NULL;
    for(multiset<CheckIn::TPaxRemItem>::const_iterator r=rems.begin();r!=rems.end();++r)
    {
      rem_detail << ".R/" << r->text << "   ";
      if ( r->code == "STCR" && !stcrNode )
      {
        stcrNode = NewTextChild( node, "step", "down" );
      };
    };
    NewTextChild( node, "rems", GetRemarkStr(rem_grp, rems), "" );
    NewTextChild( node, "rem", rem_detail.str(), "" );
    NewTextChild( node, "pax_id", pax_id );
    NewTextChild( node, "pnr_id", Qry.FieldAsInteger( col_pnr_id ) );
    NewTextChild( node, "tid", Qry.FieldAsInteger( col_tid ) );
    if ( !Qry.FieldIsNULL( col_wl_type ) )
        NewTextChild( node, "wl_type", Qry.FieldAsString( col_wl_type ) );

    if (!Qry.FieldIsNULL(col_grp_id))
    {
      NewTextChild( node, "reg_no", Qry.FieldAsInteger( col_reg_no ) );
      NewTextChild( node, "refuse", Qry.FieldAsString( col_refuse ), "" );
      if ( !Qry.FieldIsNULL( col_refuse ) )
        continue; // �� ���� �᪠�� ����
      mode = 0;
      SQry.SetVariable( "layer_type", Qry.FieldAsString( col_grp_status ) );
      SQry.SetVariable( "seats", Qry.FieldAsInteger(col_pax_seats)  );
      SQry.SetVariable( "point_id", point_id );
      SQry.SetVariable( "pax_row", pax_row );
      ProgTrace( TRACE5, "mode=%d, pax_id=%d, seats=%d, point_id=%d, pax_row=%d, layer_type=%s",
                         mode, pax_id, Qry.FieldAsInteger(col_pax_seats), point_id,
                         pax_row, Qry.FieldAsString( col_grp_status ) );
    }
    else {
        mode = 1;
        SQry.SetVariable( "xname", Qry.FieldAsString( col_seat_xname ) );
        SQry.SetVariable( "yname", Qry.FieldAsString( col_seat_yname ) );
        SQry.SetVariable( "layer_type", FNull );
        SQry.SetVariable( "seats", Qry.FieldAsInteger(col_seats)  );
        SQry.SetVariable( "point_id", Qry.FieldAsInteger(col_point_id_tlg) );
        SQry.SetVariable( "crs_row", crs_row );
      ProgTrace( TRACE5, "mode=%d, pax_id=%d, seats=%d, point_id=%d, crs_row=%d, layer_type=%s",
                         mode, pax_id, Qry.FieldAsInteger(col_seats), point_id,
                         crs_row, "" );
    }
    SQry.SetVariable( "mode", mode );
    SQry.SetVariable( "pax_id", pax_id );
    SQry.SetVariable( "is_jmp", (int)(Qry.FieldAsInteger(col_is_jmp)!=0) );
    SQry.SetVariable( "seat_no", FNull );
    SQry.Execute();
    if ( mode == 0 )
        pax_row++;
    else
        crs_row++;
    NewTextChild( node, "isseat", ( pr_free_seating || !SQry.VariableIsNULL( "seat_no" ) || !Qry.FieldAsInteger( col_seats ) ) );
    if ( !SQry.VariableIsNULL( "seat_no" ) ) {
        string seat_no = SQry.GetVariableAsString( "seat_no" );
        string layer_type;
        if ( mode == 0 ) {
            layer_type = ((const TGrpStatusTypesRow&)grp_status_types.get_row("code",Qry.FieldAsString( col_grp_status ))).layer_type;
        }
        else {
            layer_type = SQry.GetVariableAsString( "layer_type" );
        }
        switch ( DecodeCompLayerType( (const char*)layer_type.c_str() ) ) { // 12.12.08 ��� ᮢ���⨬��� � ��ன ���ᨥ�
            case cltProtCkin:
                NewTextChild( node, "preseat_no", seat_no );
                NewTextChild( node, "crs_seat_no", string(Qry.FieldAsString( col_seat_xname ))+Qry.FieldAsString( col_seat_yname ) );
                break;
            case cltPNLCkin:
                NewTextChild( node, "crs_seat_no", seat_no );
                break;
            default:
                NewTextChild( node, "seat_no", seat_no );
                break;
        }
        NewTextChild( node, "nseat_no", seat_no );
        NewTextChild( node, "layer_type", layer_type );
    } // �� ������ ����
    else {
        if ( mode == 0 &&
           Qry.FieldAsInteger( col_seats ) &&
           !pr_free_seating ) {
            string old_seat_no;
            if ( Qry.FieldIsNULL( col_wl_type ) ) {
              old_seat_no = priorSeats.getSeats( pax_id, "seats" );
              if ( !old_seat_no.empty() )
                old_seat_no = "(" + old_seat_no + ")";
            }
            else
                old_seat_no = AstraLocale::getLocaleText("��");
            if ( !old_seat_no.empty() )
              NewTextChild( node, "nseat_no", old_seat_no );
        }
    }
    CheckIn::TPaxTknItem tkn;
    CheckIn::LoadCrsPaxTkn(pax_id, tkn);
    if ( tkn.validET() ) {
      brands.get(flt.airline, TETickItem().fromDB(tkn.no, tkn.coupon, TETickItem::Display, false));
      NewTextChild( node, "brand", brands.getSingleBrand().name(AstraLocale::OutputLang()) );
    }
    std::set<CheckIn::TPaxFQTItem> fqts;
    CheckIn::LoadCrsPaxFQT(pax_id, fqts);
    for(set<CheckIn::TPaxFQTItem>::const_iterator f=fqts.begin(); f!=fqts.end(); ++f) {
      if (f->rem=="FQTV" && !f->tier_level.empty())  {
        NewTextChild( node, "fqt", f->tier_level );
        break;
      }
    }
    //DocsFlags
    if ( !apis_generation ) {
      continue;
    }
    std::string docflags = getDocsFlags( pax_id, !Qry.FieldIsNULL(col_grp_id) );
    ProgTrace( TRACE5, "docflags=%s", docflags.c_str() );
    NewTextChild( node, "apis", docflags, "" );
    //end for
  };
  brands.traceCaching();
};


template <class T1>
std::string getDocsFlag_OLD( const T1 &crs_pax_doc, const T1 &pax_doc, bool pr_checkin, std::string flagStr, bool firstFlag )
{
  std::string res;
  if ( (crs_pax_doc.empty() && !pax_doc.empty()) ||
       (!crs_pax_doc.empty() && !pax_doc.empty() && !crs_pax_doc.equalAttrs( pax_doc )) ) {
     if ( !firstFlag ) res += "/";
     res += string("+") + flagStr;
  }
  if ( !crs_pax_doc.empty() && ( !pr_checkin || crs_pax_doc.equalAttrs( pax_doc ) ) ) {
    if ( !firstFlag ) res += "/";
    res += flagStr;
  }
  if ( !crs_pax_doc.empty() && pax_doc.empty() && pr_checkin ) {
    if ( !firstFlag ) res += "/";
    res += string("-") + flagStr;
  }
  return res;
}

template <class TDOC>
std::string getDocsFlag( const TDOC &crs_pax_doc, const TDOC &pax_doc, bool pr_checkin, std::string flagStr, bool firstFlag )
{
  ostringstream res;
  if (!firstFlag) res << "/";
  if (crs_pax_doc.empty())
  {
    if (pax_doc.empty())
    {
      return "";
    }
    else
    {
      if ((pax_doc.scanned_attrs & pax_doc.getNotEmptyFieldsMask()) != pax_doc.getNotEmptyFieldsMask())
        res << "+"; // �������� �� ॣ����樨 � �����
      else
        res << "*"; // �������� �� ॣ����樨 � ������� ᪠��஢����
    }
  }
  else // !crs_pax_doc.empty()
  {
    if (pax_doc.empty())
    {
      if (pr_checkin)
        res << "-"; // �������
    }
    else if (!crs_pax_doc.equalAttrs( pax_doc ))
    {
      if ((pax_doc.scanned_attrs & pax_doc.getNotEmptyFieldsMask()) != pax_doc.getNotEmptyFieldsMask())
        res << "#"; // �������� � �����
      else
        res << "="; // ��������� ������ �१ ᪠��஢����
    }
  }
  res << flagStr;
  return res.str();
}

std::string getDocsFlags( int pax_id, bool pr_checkin )
{
  //ProgTrace( TRACE5, "getDocsFlags: pax_id=%d, pr_checkin=%d", pax_id, pr_checkin);
  std::string res;
  CheckIn::TPaxDocItem pax_doc, crs_pax_doc;
  CheckIn::TPaxDocoItem pax_doco, crs_pax_doco;
  CheckIn::LoadCrsPaxDoc(pax_id,crs_pax_doc);
  if ( pr_checkin ) {
    CheckIn::LoadPaxDoc(pax_id,pax_doc);
  }
  res += getDocsFlag( crs_pax_doc, pax_doc, pr_checkin, "S", res.empty() );
  CheckIn::LoadCrsPaxVisa(pax_id,crs_pax_doco);
  if ( pr_checkin ) {
    CheckIn::LoadPaxDoco(pax_id,pax_doco);
  }
  res += getDocsFlag( crs_pax_doco, pax_doco, pr_checkin, "O", res.empty() );
  CheckIn::TDocaMap doca_map, crs_doca_map;
  CheckIn::LoadCrsPaxDoca(pax_id,crs_doca_map);
  if ( pr_checkin ) {
    CheckIn::LoadPaxDoca(pax_id,doca_map);
  }
  CheckIn::TPaxDocaItem pax_doca, crs_pax_doca;
  if ( doca_map.find( apiDocaB ) != doca_map.end() ||
       !doca_map[ apiDocaB ].empty() ) {
    pax_doca = doca_map[ apiDocaB ];
  }
  if ( crs_doca_map.find( apiDocaB ) != crs_doca_map.end() ||
       !crs_doca_map[ apiDocaB ].empty() ) {
    crs_pax_doca = crs_doca_map[ apiDocaB ];
  }
  res += getDocsFlag( crs_pax_doca, pax_doca, pr_checkin, "AB", res.empty() );
  pax_doca.clear();
  crs_pax_doca.clear();
  if ( doca_map.find( apiDocaR ) != doca_map.end() ||
       !doca_map[ apiDocaR ].empty() ) {
    pax_doca = doca_map[ apiDocaR ];
  }
  if ( crs_doca_map.find( apiDocaR ) != crs_doca_map.end() ||
       !crs_doca_map[ apiDocaR ].empty() ) {
    crs_pax_doca = crs_doca_map[ apiDocaR ];
  }
  res += getDocsFlag( crs_pax_doca, pax_doca, pr_checkin, "AR", res.empty() );
  pax_doca.clear();
  crs_pax_doca.clear();
  if ( doca_map.find( apiDocaD ) != doca_map.end() ||
       !doca_map[ apiDocaD ].empty() ) {
    pax_doca = doca_map[ apiDocaD ];
  }
  if ( crs_doca_map.find( apiDocaD ) != crs_doca_map.end() ||
       !crs_doca_map[ apiDocaD ].empty() ) {
    crs_pax_doca = crs_doca_map[ apiDocaD ];
  }
  res += getDocsFlag( crs_pax_doca, pax_doca, pr_checkin, "AD", res.empty() );
  return res;
}

bool SearchPaxByScanData(xmlNodePtr reqNode,
                         int &point_id,
                         int &reg_no,
                         int &pax_id)
{
    bool isBoardingPass;
    return SearchPaxByScanData(reqNode, point_id, reg_no, pax_id, isBoardingPass);
}

bool SearchPaxByScanData(xmlNodePtr reqNode,
                         int &point_id,
                         int &reg_no,
                         int &pax_id, bool &isBoardingPass)
{
  bool result=false;

  point_id=NoExists;
  reg_no=NoExists;
  pax_id=NoExists;

  string bcbp;
  if (NodeAsInteger("scan_data/@hex", reqNode)!=0)
  {
    if (!HexToString(NodeAsString("scan_data", reqNode), bcbp))
      throw AstraLocale::UserException("MSG.WRONG_DATA_RECEIVED");
  }
  else
    bcbp=NodeAsString("scan_data", reqNode);
  WebSearch::TPNRFilters filters;

  BCBPSections scanSections;
  filters.getBCBPSections(bcbp, scanSections);
  isBoardingPass = scanSections.isBoardingPass();
  filters.fromBCBPSections(scanSections);
  if (filters.segs.empty())
    throw EXCEPTIONS::EConvertError("%s: filters.segs.empty()", __FUNCTION__);
  const WebSearch::TPNRFilter &filter=*(filters.segs.begin());
  try
  {
    if (filter.airlines.size()!=1)
      throw EXCEPTIONS::EConvertError("airlines.size()!=1");
    if (filter.scd_out_local_ranges.size()!=1)
      throw EXCEPTIONS::EConvertError("scd_out_local_ranges.size()!=1");
    TSearchFltInfo searchInfo;
    searchInfo.airline=*(filter.airlines.begin());
    searchInfo.flt_no=filter.flt_no;
    searchInfo.suffix=filter.suffix;
    searchInfo.airp_dep=filter.airp_dep;
    searchInfo.scd_out=filter.scd_out_local_ranges.begin()->first;
    searchInfo.scd_out_in_utc=false;
    searchInfo.only_with_reg=false;
    if (filter.reg_no==NoExists)
      throw EXCEPTIONS::EConvertError("filter.reg_no==NoExists");

    list<TAdvTripInfo> flts;
    SearchFlt(searchInfo, flts);

    if (flts.empty()) return result;

    TQuery Qry( &OraSession );
    Qry.Clear();

    ostringstream sql;
    sql << "SELECT pax.grp_id, pax.pax_id, pax.name, pax.seats, pax.reg_no "
           "FROM pax_grp, pax "
           "WHERE pax_grp.grp_id=pax.grp_id AND "
           "      pax_grp.point_dep=:point_id AND "
           "      pax_grp.airp_arv=:airp_arv AND "
           "      pax.pr_brd IS NOT NULL AND "
        << filter.getSurnameSQLFilter("pax.surname", Qry);
    Qry.DeclareVariable("point_id", otInteger);
    Qry.CreateVariable("airp_arv", otString, filter.airp_arv);
    Qry.SQLText=sql.str().c_str();

    int anySuitablePointId=NoExists;
    int anySuitablePaxId=NoExists;

    for(list<TAdvTripInfo>::const_iterator f=flts.begin(); f!=flts.end(); ++f)
    {
      anySuitablePointId=f->point_id;

      int pax_id_with_seats=NoExists;
      int pax_id_without_seats=NoExists;
      int grp_id=NoExists;
      Qry.SetVariable("point_id", f->point_id);
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        if (!filter.isEqualName(Qry.FieldAsString("name"))) continue;
        anySuitablePaxId=Qry.FieldAsInteger("pax_id");
        if (!filter.isEqualRegNo(Qry.FieldAsInteger("reg_no"))) continue;

        int &pax_id_ref=(Qry.FieldAsInteger("seats")>0?pax_id_with_seats:pax_id_without_seats);
        if (pax_id_ref!=NoExists)
          throw EXCEPTIONS::Exception("Duplicate reg_no (point_id=%d, reg_no=%d)", f->point_id, filter.reg_no);
        if (grp_id!=NoExists && grp_id!=Qry.FieldAsInteger("grp_id"))
          throw EXCEPTIONS::Exception("Duplicate reg_no (point_id=%d, reg_no=%d)", f->point_id, filter.reg_no);
        grp_id=Qry.FieldAsInteger("grp_id");
        pax_id_ref=Qry.FieldAsInteger("pax_id");
      };

      int new_pax_id=pax_id_with_seats!=NoExists?pax_id_with_seats:pax_id_without_seats;
      if (new_pax_id!=NoExists)
      {
        if (pax_id!=NoExists)
          throw EXCEPTIONS::Exception("More than one passenger found (pax_id1=%d, pax_id2=%d)", pax_id, new_pax_id);
        point_id=f->point_id;
        reg_no=filter.reg_no;
        pax_id=new_pax_id;
        result=true;
      };
    };

    if (point_id==NoExists) point_id=anySuitablePointId;
    if (pax_id==NoExists) pax_id=anySuitablePaxId;
  }
  catch(EXCEPTIONS::EConvertError &e)
  {
    filter.trace(TRACE5);
    ProgTrace(TRACE5, ">>>> EConvertError: %s", e.what());
    throw AstraLocale::UserException("MSG.WRONG_DATA_RECEIVED");
  }
  catch(EXCEPTIONS::Exception &e)
  {
    filter.trace(TRACE5);
    ProgTrace(TRACE5, ">>>> Exception: %s", e.what());
    throw;
  };
  return result;
};

