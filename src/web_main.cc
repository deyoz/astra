#include <arpa/inet.h>
#include <memory.h>
#include <string>

#include "oralib.h"
#include "exceptions.h"
#include "stages.h"
#include "salons.h"
#include "salonform.h"
#include "seats.h"
#include "images.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_context.h"
#include "convert.h"
#include "basic.h"
#include "astra_misc.h"
#include "print.h"
#include "web_main.h"
#include "checkin.h"
#include "astra_locale.h"
#include "comp_layers.h"
#include "passenger.h"
#include "remarks.h"
#include "sopp.h"
#include "points.h"
#include "stages.h"
#include "astra_callbacks.h"
#include "tlg/tlg.h"
#include "serverlib/perfom.h"
#include "serverlib/ourtime.h"
#include "serverlib/query_runner.h"
#include "jxtlib/xmllibcpp.h"
#include "jxtlib/xml_stuff.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

namespace AstraWeb
{

using namespace std;
using namespace ASTRA;
using namespace SEATS2;
using namespace BASIC;
using namespace AstraLocale;

const string PARTITION_ELEM_TYPE = "�";
const string ARMCHAIR_ELEM_TYPE = "�";

int readInetClientId(const char *head)
{
  short grp;
  memcpy(&grp,head+45,2);
  return ntohs(grp);
}

InetClient getInetClient(int client_id)
{
  InetClient client;
  client.client_id = client_id;
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT client_type,web_clients.desk,login "
    "FROM web_clients,users2 "
    "WHERE web_clients.id=:client_id AND "
    "      web_clients.user_id=users2.user_id";
  Qry.CreateVariable( "client_id", otInteger, client_id );
  Qry.Execute();
  if ( !Qry.Eof ) {
    client.pult = Qry.FieldAsString( "desk" );
    client.opr = Qry.FieldAsString( "login" );
    client.client_type = Qry.FieldAsString( "client_type" );
  }
  return client;
}

int internet_main(const char *body, int blen, const char *head,
                  int hlen, char **res, int len)
{
  InitLogTime(0);
  PerfomInit();
  int client_id=readInetClientId(head);
  ProgTrace(TRACE1,"new web request received from client %i",client_id);

  try
  {
    if (ENABLE_REQUEST_DUP() &&
        hlen>0 && *head==char(2))
    {
      std::string b(body,blen);
      if ( b.find("<kick") == string::npos )
      {
        std::string msg;
        if (BuildMsgForWebRequestDup(client_id, b, msg))
        {
          /*std::string msg_hex;
          StringToHex(msg, msg_hex);
          ProgTrace(TRACE5, "internet_main: msg_hex=%s", msg_hex.c_str());*/
          sendCmd("REQUEST_DUP", msg.c_str(), msg.size());
        };
      };
    };
  }
  catch(...) {};

  string answer;
  int newlen=0;

  try
  {
    InetClient client=getInetClient(client_id);
    string new_header=(string(head,45)+client.pult+"  "+client.opr+string(100,0)).substr(0,100)+string(head+100,hlen-100);

    string new_body(body,blen);
    string sss("<query");
    string::size_type pos=new_body.find(sss);
    if(pos!=string::npos)
    {
    	if ( new_body.find("<kick") == string::npos )
        new_body=new_body.substr(0,pos+sss.size())+" id='"+WEB_JXT_IFACE_ID/*client.client_type*/+"' screen='AIR.EXE' opr='"+CP866toUTF8(client.opr)+"'"+new_body.substr(pos+sss.size());
    }
    else
      ProgTrace(TRACE1,"Unable to find <query> tag!");


    static ServerFramework::ApplicationCallbacks *ac=
             ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks();
    newlen=ac->jxt_proc((const char *)new_body.data(),new_body.size(),(const char *)new_header.data(),new_header.size(),res,len);
    ProgTrace(TRACE1,"newlen=%i",newlen);

    memcpy(*res,head,hlen);


  }
  catch(...)
  {
    answer="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<error/>";
    newlen=answer.size()+hlen; // ࠧ��� �⢥� (� ����������)
    ProgTrace(TRACE1,"Outgoing message is %i bytes long",int(answer.size()));
    if(newlen>len)
    {
      *res=(char *)malloc(newlen*sizeof(char));
      if(*res==NULL)
      {
        ProgError(STDLOG,"malloc failed to allocate %i bytes",newlen);
        return 0;
      }
    }
    memcpy(*res,head,hlen);
    memcpy(*res+hlen,answer.data(),answer.size());
  }

  InitLogTime(0);
  return newlen;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
class TPnrAddr {
  public:
  	string airline;
  	string addr;
  	string addr_inter;

  	TPnrAddr(const string &vairline, const string &vaddr)
  	{
      airline=vairline;
      addr=vaddr;
      addr_inter=convert_pnr_addr(vaddr, true);
    };

  	bool operator == (const TPnrAddr &item) const
    {
      return airline==item.airline &&
             addr_inter==item.addr_inter;
    };
};

struct TSearchPnrData {
	int point_id;
	int point_num;
	int first_point;
	bool pr_tranzit;
	string airline;
	int flt_no;
	string suffix;
	TDateTime scd_out, act_out;
	TDateTime scd_out_local, est_out_local, act_out_local;
	int dep_utc_offset;
	string craft;
	string airp_dep;
	string city_dep;
	map<TStage, TDateTime> stages;
	TStage web_checkin_stage;
  TStage web_cancel_stage;
	TStage kiosk_checkin_stage;
	TStage term_checkin_stage;
	TStage brd_stage;
	std::vector<TTripInfo> mark_flights;

	int airline_fmt,suffix_fmt,airp_dep_fmt,craft_fmt;

  bool is_test;
  int pnr_id;
  int point_arv;
	TDateTime scd_in_local, est_in_local, act_in_local;
	int arv_utc_offset;
	string airp_arv;
	string city_arv;
	string cls;
	string subcls;
	vector<TPnrAddr> pnr_addrs;
	int bag_norm;
	
	bool pr_paid_ckin;
};

struct TIdsPnrData {
  int point_id;
  int pnr_id;
  bool pr_paid_ckin;
};

struct TPnrInfo {
  int pnr_id;
  string airp_arv, cl, subcl;
  vector<int> pax_id;
  vector<TPnrAddr> pnr_addrs;
  vector< pair<int,int> > point_dep;
  TPnrInfo():
    pnr_id(NoExists) {};
};

void tracePax( TRACE_SIGNATURE,
               const string &descr,
               const int pass,
               const vector<TPnrInfo> &pnr )
{
  ProgTrace(TRACE_PARAMS, "============ %s, pass=%d ============", descr.c_str(), pass);
  for(vector<TPnrInfo>::const_iterator iPnr=pnr.begin();iPnr!=pnr.end();iPnr++)
  {
    ostringstream str;

    str << "PNR: pnr_id=" << iPnr->pnr_id << ", airp_arv=" << iPnr->airp_arv;

    str << ", pnr_addrs=";
    for(vector<TPnrAddr>::const_iterator i=iPnr->pnr_addrs.begin(); i!=iPnr->pnr_addrs.end(); i++)
      str << i->addr << "/" << i->airline << " | ";
    ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

    str.str("");

    str << "     pax_id=";
    for(vector<int>::const_iterator i=iPnr->pax_id.begin(); i!=iPnr->pax_id.end(); i++)
      str << *i << " | ";
    ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

    str.str("");

    str << "     point_dep:point_arv=";
    for(vector< pair<int,int> >::const_iterator i=iPnr->point_dep.begin(); i!=iPnr->point_dep.end(); i++)
      str << i->first << ":" << (i->second != NoExists ? IntToString(i->second) : "NoExists") << " | ";
    ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());
  };

  ProgTrace(TRACE_PARAMS, "^^^^^^^^^^^^ %s, pass=%d ^^^^^^^^^^^^", descr.c_str(), pass);
};

bool getTripData( int point_id, bool first_segment, TSearchPnrData &SearchPnrData, bool pr_throw )
{
  try
  {
    TReqInfo *reqInfo = TReqInfo::Instance();

  	ProgTrace( TRACE5, "point_id=%d", point_id );
  	SearchPnrData.point_id = NoExists;
  	TQuery Qry(&OraSession);
  	Qry.SQLText =
  	  "SELECT points.point_id,point_num,first_point,pr_tranzit,pr_del,pr_reg,scd_out,est_out,act_out, "
  	  "       airline,airline_fmt,flt_no,airp,airp_fmt,suffix,suffix_fmt, "
  	  "       craft,craft_fmt, "
  	  "       trip_paid_ckin.pr_permit AS pr_paid_ckin "
      "FROM points, trip_paid_ckin "
  	  "WHERE points.point_id=trip_paid_ckin.point_id(+) AND "
      "      points.point_id=:point_id";
  	Qry.CreateVariable( "point_id", otInteger, point_id );
  	Qry.Execute();
  	if ( Qry.Eof )
  		throw UserException( "MSG.FLIGHT.NOT_FOUND" );

  	if ( Qry.FieldAsInteger( "pr_del" ) < 0 )
  		throw UserException( "MSG.FLIGHT.DELETED" );

    if (first_segment)
    {
      //����室��� ᮡ��� ����� ⮫쪮 � ��ࢮ�� ᥣ�����
      if ( !reqInfo->CheckAirline(Qry.FieldAsString("airline")) ||
           !reqInfo->CheckAirp(Qry.FieldAsString("airp")) )
        throw UserException( "MSG.FLIGHT.ACCESS_DENIED" );
    };

  	if ( Qry.FieldAsInteger( "pr_del" ) > 0 )
  		throw UserException( "MSG.FLIGHT.CANCELED" );

  	if ( Qry.FieldAsInteger( "pr_reg" ) == 0 )
  		throw UserException( "MSG.FLIGHT.CHECKIN_CANCELED" );

    if ( Qry.FieldIsNULL( "scd_out" ) )
      throw UserException( "MSG.FLIGHT_DATE.NOT_SET" );

  	TTripStages tripStages( point_id );
  	SearchPnrData.web_checkin_stage = tripStages.getStage( stWEBCheckIn );
    SearchPnrData.web_cancel_stage = tripStages.getStage( stWEBCancel );
    SearchPnrData.kiosk_checkin_stage = tripStages.getStage( stKIOSKCheckIn );
  	SearchPnrData.term_checkin_stage = tripStages.getStage( stCheckIn );
  	SearchPnrData.brd_stage = tripStages.getStage( stBoarding );

  	ProgTrace( TRACE5, "stages: web_checkin=%d, web_cancel=%d, kiosk_checkin=%d, term_checkin=%d, brd=%d",
  	           (int)SearchPnrData.web_checkin_stage,
               (int)SearchPnrData.web_cancel_stage,
  	           (int)SearchPnrData.kiosk_checkin_stage,
               (int)SearchPnrData.term_checkin_stage,
               (int)SearchPnrData.brd_stage );
  	TBaseTable &baseairps = base_tables.get( "airps" );

  	SearchPnrData.airline = Qry.FieldAsString( "airline" );
  	SearchPnrData.airline_fmt = Qry.FieldAsInteger( "airline_fmt" );
  	SearchPnrData.flt_no = Qry.FieldAsInteger( "flt_no" );
  	SearchPnrData.suffix = Qry.FieldAsString( "suffix" );
  	SearchPnrData.suffix_fmt = Qry.FieldAsInteger( "suffix_fmt" );
  	SearchPnrData.airp_dep = Qry.FieldAsString( "airp" );
  	SearchPnrData.airp_dep_fmt = Qry.FieldAsInteger( "airp_fmt" );
  	SearchPnrData.city_dep = ((TAirpsRow&)baseairps.get_row( "code", Qry.FieldAsString( "airp" ), true )).city;
  	
  	string region=AirpTZRegion(SearchPnrData.airp_dep);
  	SearchPnrData.scd_out = Qry.FieldAsDateTime( "scd_out" );
  	SearchPnrData.scd_out_local = UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), region );
  	SearchPnrData.dep_utc_offset = (int)round((SearchPnrData.scd_out_local - SearchPnrData.scd_out) * 1440);
  	if ( Qry.FieldIsNULL( "est_out" ) )
  	{
  		SearchPnrData.est_out_local = NoExists;
  	}
  	else
  	{
  		SearchPnrData.est_out_local = UTCToLocal( Qry.FieldAsDateTime( "est_out" ), region );
    };
  	if ( Qry.FieldIsNULL( "act_out" ) )
  	{
  		SearchPnrData.act_out = NoExists;
  		SearchPnrData.act_out_local = NoExists;
  	}
  	else
  	{
  		SearchPnrData.act_out = Qry.FieldAsDateTime( "act_out" );
  		SearchPnrData.act_out_local = UTCToLocal( Qry.FieldAsDateTime( "act_out" ), region );
    };
  	
  	SearchPnrData.craft = Qry.FieldAsString( "craft" );
  	SearchPnrData.craft_fmt = Qry.FieldAsInteger( "craft_fmt" );
  	
  	SearchPnrData.point_id = point_id;
  	SearchPnrData.point_num = Qry.FieldAsInteger("point_num");
  	SearchPnrData.first_point = Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point");
  	SearchPnrData.pr_tranzit = Qry.FieldAsInteger("pr_tranzit")!=0;
  	SearchPnrData.pr_paid_ckin = false;
    if (!Qry.FieldIsNULL("pr_paid_ckin"))
      SearchPnrData.pr_paid_ckin = Qry.FieldAsInteger("pr_paid_ckin")!=0;

  	TTripInfo operFlt(Qry);
  	GetMktFlights(operFlt, SearchPnrData.mark_flights);
  	
  	SearchPnrData.stages.clear();
  	
  	TStagesRules *sr = TStagesRules::Instance();
  	TCkinClients ckin_clients;
  	TTripStages::ReadCkinClients( point_id, ckin_clients );
  	TStage stage;
    for(int pass=0; pass<7; pass++)
    {
      switch(pass)
      {
        case 0: if ( reqInfo->client_type == ctKiosk )
                  stage=sOpenKIOSKCheckIn;
                else
                  stage=sOpenWEBCheckIn;
                break;
        case 1: if ( reqInfo->client_type == ctKiosk )
                  stage=sCloseKIOSKCheckIn;
                else
                  stage=sCloseWEBCheckIn;
                break;
        case 2: if ( reqInfo->client_type == ctKiosk )
                  continue;
                else
                  stage=sCloseWEBCancel;
                break;
        case 3: stage=sOpenCheckIn;
                break;
        case 4: stage=sCloseCheckIn;
                break;
        case 5: stage=sOpenBoarding;
                break;
        case 6: stage=sCloseBoarding;
                break;
      };
      
      if ( reqInfo->client_type == ctKiosk && first_segment )
      {
        //�஢��塞 ����������� ॣ����樨 ��� ���᪠ ⮫쪮 �� ��ࢮ� ᥣ����
        TCkinClients::iterator iClient=find(ckin_clients.begin(),ckin_clients.end(),EncodeClientType(reqInfo->client_type));
        if (iClient!=ckin_clients.end())
        {
          Qry.Clear();
          Qry.SQLText=
            "SELECT pr_permit FROM trip_ckin_client "
            "WHERE point_id=:point_id AND client_type=:client_type AND desk_grp_id=:desk_grp_id";
          Qry.CreateVariable("point_id", otInteger, point_id);
          Qry.CreateVariable("client_type", otString, EncodeClientType(reqInfo->client_type));
          Qry.CreateVariable("desk_grp_id", otInteger, reqInfo->desk.grp_id);
          Qry.Execute();
          if (Qry.Eof || Qry.FieldAsInteger("pr_permit")==0)
          {
            ckin_clients.erase(iClient);
            if (SearchPnrData.kiosk_checkin_stage==sOpenKIOSKCheckIn ||
                SearchPnrData.kiosk_checkin_stage==sCloseKIOSKCheckIn)
              SearchPnrData.kiosk_checkin_stage=sNoActive;
          };
        };
      };
      
      if ( sr->isClientStage( (int)stage ) && !sr->canClientStage( ckin_clients, (int)stage ) )
      	SearchPnrData.stages.insert( make_pair(stage, NoExists) );
      else
      	SearchPnrData.stages.insert( make_pair(stage, UTCToLocal( tripStages.time(stage), region ) ) );
  	};
	}
  catch(UserException &E)
  {
    ProgTrace(TRACE5, ">>>> %s", getLocaleText(E.getLexemaData()).c_str());
    if ( pr_throw )
      throw;
    else
      return false;
  };
	
	return true;
}

bool getTripData2( TSearchPnrData &SearchPnrData, bool pr_throw )
{
  try
  {
    //������塞 scd_in � city_arv
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
     "SELECT scd_in, est_in, act_in FROM points WHERE point_id=:point_id AND airp=:airp AND pr_del=0";
    Qry.CreateVariable( "point_id", otInteger, SearchPnrData.point_arv );
    Qry.CreateVariable( "airp", otString, SearchPnrData.airp_arv );
  	Qry.Execute();
  	if (Qry.Eof)
  	  throw UserException( "MSG.FLIGHT.NOT_FOUND" );

    string region = AirpTZRegion(SearchPnrData.airp_arv);
    SearchPnrData.arv_utc_offset = NoExists;
    if ( Qry.FieldIsNULL( "scd_in" ) )
    	SearchPnrData.scd_in_local = NoExists;
    else
    {
      SearchPnrData.scd_in_local = UTCToLocal( Qry.FieldAsDateTime( "scd_in" ), region );
      if (SearchPnrData.arv_utc_offset == NoExists)
        SearchPnrData.arv_utc_offset = (int)round((SearchPnrData.scd_in_local-Qry.FieldAsDateTime( "scd_in" )) * 1440);
    };
    if ( Qry.FieldIsNULL( "est_in" ) )
    	SearchPnrData.est_in_local = NoExists;
    else
    {
      SearchPnrData.est_in_local = UTCToLocal( Qry.FieldAsDateTime( "est_in" ), region );
      if (SearchPnrData.arv_utc_offset == NoExists)
        SearchPnrData.arv_utc_offset = (int)round((SearchPnrData.est_in_local-Qry.FieldAsDateTime( "est_in" )) * 1440);
    };
    if ( Qry.FieldIsNULL( "act_in" ) )
    	SearchPnrData.act_in_local = NoExists;
    else
    {
      SearchPnrData.act_in_local = UTCToLocal( Qry.FieldAsDateTime( "act_in" ), region );
      if (SearchPnrData.arv_utc_offset == NoExists)
        SearchPnrData.arv_utc_offset = (int)round((SearchPnrData.act_in_local-Qry.FieldAsDateTime( "act_in" )) * 1440);
    };

    TBaseTable &baseairps = base_tables.get( "airps" );
    try
    {
   	  SearchPnrData.city_arv = ((TAirpsRow&)baseairps.get_row( "code", SearchPnrData.airp_arv, true )).city;
   	}
   	catch(EBaseTableError)
   	{
  	  throw UserException( "MSG.AIRP.INVALID_GIVEN_CODE" );
    };
  }
  catch(UserException &E)
  {
    ProgTrace(TRACE5, ">>>> %s", getLocaleText(E.getLexemaData()).c_str());
    if ( pr_throw )
      throw;
    else
      return false;
  };
  return true;
}

bool getTripData( int point_id, bool pr_throw )
{
	TSearchPnrData SearchPnrData;
	//first_segment=false ᯥ樠�쭮, �⮡� ����⢮���� ����� ��� �᫮��� �� ��������� ����⨢����� �⠯� ��. ��䨪�
	return getTripData( point_id, false, SearchPnrData, pr_throw );
}

void getTCkinData( const TSearchPnrData &firstPnrData,
                   vector<TSearchPnrData> &pnrs)
{
  pnrs.clear();
  pnrs.push_back(firstPnrData);
  
  if (firstPnrData.is_test) return;
  
  TReqInfo *reqInfo = TReqInfo::Instance();
  
  //���� ��몮����� ᥣ���⮢ (�����頥� ����� point_id)
  
  TQuery Qry(&OraSession);
  map<int, CheckIn::TTransferItem> crs_trfer;
  TTripInfo operFlt;
  operFlt.airline=firstPnrData.airline;
  operFlt.flt_no=firstPnrData.flt_no;
  operFlt.suffix=firstPnrData.suffix;
  operFlt.airp=firstPnrData.airp_dep;
  operFlt.scd_out=firstPnrData.scd_out;
  CheckInInterface::GetOnwardCrsTransfer(firstPnrData.pnr_id, Qry, operFlt, firstPnrData.airp_arv, crs_trfer);
  if (!crs_trfer.empty())
  {
    //�஢��塞 ࠧ�襭�� ᪢����� ॣ����樨 ��� ������� ⨯� ������
    Qry.Clear();
    if (reqInfo->client_type==ctKiosk)
    {
      Qry.SQLText=
        "SELECT pr_tckin FROM trip_ckin_client "
        "WHERE point_id=:point_id AND client_type=:client_type AND desk_grp_id=:desk_grp_id";
      Qry.CreateVariable("desk_grp_id", otInteger, reqInfo->desk.grp_id);
    }
    else
    {
      Qry.SQLText=
        "SELECT pr_tckin FROM trip_ckin_client "
        "WHERE point_id=:point_id AND client_type=:client_type AND desk_grp_id IS NULL";
    };
    Qry.CreateVariable("point_id", otInteger, firstPnrData.point_id);
    Qry.CreateVariable("client_type", otString, EncodeClientType(reqInfo->client_type));
    Qry.Execute();
    if (Qry.Eof || Qry.FieldAsInteger("pr_tckin")==0)
    {
      //᪢����� ॣ������ ����饭�
      ProgTrace(TRACE5, ">>>> Through check-in not permitted (point_id=%d, client_type=%s, desk_grp_id=%d)",
                        firstPnrData.point_id, EncodeClientType(reqInfo->client_type), reqInfo->desk.grp_id);
      return;
    };

    map<int, pair<TCkinSegFlts, TTrferSetsInfo> > trfer_segs;
    traceTrfer(TRACE5, "getTCkinData: crs_trfer", crs_trfer);
    CheckInInterface::GetTrferSets(operFlt,
                                   firstPnrData.airp_arv,
                                   "",
                                   crs_trfer,
                                   false,
                                   trfer_segs);
    traceTrfer(TRACE5, "getTCkinData: trfer_segs", trfer_segs);
    if (crs_trfer.size()!=trfer_segs.size())
      throw EXCEPTIONS::Exception("getTCkinData: different array sizes "
                                  "(crs_trfer.size()=%d, trfer_segs.size()=%d)",
                                  crs_trfer.size(),trfer_segs.size());
    
    int seg_no=1;
    try
    {
      //横� �� ��몮���� ᥣ���⠬ � �� �࠭���� ३ᠬ
      map<int, pair<TCkinSegFlts, TTrferSetsInfo> >::const_iterator s=trfer_segs.begin();
      map<int, CheckIn::TTransferItem>::const_iterator f=crs_trfer.begin();
      for(;s!=trfer_segs.end() && f!=crs_trfer.end();++s,++f)
      {
        seg_no++;
        if (s->second.first.is_edi)
          throw "Flight from the other DCS";

        if (s->second.first.flts.empty())
          throw "Flight not found";

        if (s->second.first.flts.size()>1)
          throw "More than one flight found";
          
        if (!s->second.second.tckin_permit)
          throw "Check-in not permitted";

        const TSegInfo &currSeg=*(s->second.first.flts.begin());

        if (currSeg.fltInfo.pr_del!=0)
          throw "Flight canceled";

        if (currSeg.point_arv==ASTRA::NoExists)
          throw "Destination not found";

        Qry.Clear();
        Qry.SQLText="SELECT COUNT(*) AS num FROM trip_classes WHERE point_id=:point_id";
        Qry.CreateVariable("point_id",otInteger,currSeg.point_dep);
        Qry.Execute();
        if (Qry.Eof || Qry.FieldAsInteger("num")==0)
          throw "Configuration of the flight not assigned";

        TSearchPnrData pnrData;
        if (!getTripData(currSeg.point_dep, false, pnrData, false))
          throw "Error in 'getTripData'";

        if (reqInfo->client_type==ctKiosk)
        {
          if ( pnrData.stages[ sOpenKIOSKCheckIn ] == NoExists ||
               pnrData.stages[ sCloseKIOSKCheckIn ] == NoExists )
            throw "Stage of kiosk check-in not found";
        }
        else
        {
          if ( pnrData.stages[ sOpenWEBCheckIn ] == NoExists ||
               pnrData.stages[ sCloseWEBCheckIn ] == NoExists ||
               pnrData.stages[ sCloseWEBCancel ] == NoExists)
            throw "Stage of web check-in not found";
        };

        //�饬 PNR �� ������
        pnrData.pnr_id=NoExists;
        if (!firstPnrData.pnr_addrs.empty())
        {
          Qry.Clear();
          Qry.SQLText=
            "SELECT DISTINCT crs_pnr.class, "
            "       pnr_addrs.pnr_id, pnr_addrs.airline, pnr_addrs.addr "
            "FROM tlg_binding, crs_pnr, pnr_addrs, crs_pax "
            "WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
            "      tlg_binding.point_id_spp=:point_id AND "
            "      pnr_addrs.pnr_id=crs_pnr.pnr_id AND "
            "      crs_pnr.system='CRS' AND "
            "      crs_pnr.airp_arv=:airp_arv AND "
            "      crs_pnr.subclass=:subclass AND "
            "      crs_pax.pnr_id=crs_pnr.pnr_id AND "
            "      crs_pax.pr_del=0 "
            "ORDER BY pnr_addrs.pnr_id, pnr_addrs.airline";
          Qry.CreateVariable("point_id", otInteger, currSeg.point_dep);
          Qry.CreateVariable("airp_arv", otString, currSeg.airp_arv);  //���� �஢�ઠ ᮢ������� �/� �����祭�� �� �࠭��୮�� �������
          Qry.CreateVariable("subclass", otString, f->second.subclass);//���� �஢�ઠ ᮢ������� �������� �� �࠭��୮�� �������
          Qry.Execute();
          int prior_pnr_id=NoExists;
          //�� 室� ������塞 pnrData.pnr_id, pnrData.cls, pnrData.pnr_addrs
          for(;!Qry.Eof;Qry.Next())
          {
            TPnrAddr addr(Qry.FieldAsString("airline"),
                          Qry.FieldAsString("addr"));
            if (pnrData.pnr_id==NoExists || pnrData.pnr_id==Qry.FieldAsInteger("pnr_id"))
            {
              if (prior_pnr_id==NoExists || prior_pnr_id!=Qry.FieldAsInteger("pnr_id"))
              {
                prior_pnr_id=Qry.FieldAsInteger("pnr_id");
                pnrData.pnr_addrs.clear();
              };
              pnrData.pnr_addrs.push_back(addr);
            };

            if (find(firstPnrData.pnr_addrs.begin(),
                     firstPnrData.pnr_addrs.end(), addr)!=firstPnrData.pnr_addrs.end())
            {
              //��諨 PNR
              if (pnrData.pnr_id!=NoExists)
              {
                if (pnrData.pnr_id!=Qry.FieldAsInteger("pnr_id")) break; //�㡫� PNR
              }
              else
              {
                pnrData.pnr_id=Qry.FieldAsInteger("pnr_id");
                pnrData.cls=Qry.FieldAsString("class");
              };
            };
          };
        };

        if (pnrData.pnr_id==NoExists)
          throw "PNR not found";

        if (!Qry.Eof)
          throw "More than one PNR found";


        //���������� ���� pnrData
      	pnrData.airp_arv = currSeg.airp_arv;
      	pnrData.subcls = f->second.subclass;
      	pnrData.point_arv = currSeg.point_arv; //NoExists ���� �� ����� - �஢�७� ࠭��

      	if (!getTripData2(pnrData, false))
      	  throw "Error in 'getTripData2'";

        pnrData.bag_norm = ASTRA::NoExists;

        pnrs.push_back(pnrData);
      };
    }
    catch(const char* error)
    {
      ProgTrace(TRACE5, ">>>> seg_no=%d: %s ", seg_no, error);
      traceTrfer(TRACE5, "crs_trfer", crs_trfer);
      traceTrfer(TRACE5, "trfer_segs", trfer_segs);
    };
  };
};

void VerifyPNR( int point_id, int pnr_id )
{
	TQuery Qry(&OraSession);
  if (!isTestPaxId(pnr_id))
  {
  	Qry.SQLText =
      "SELECT point_id_spp "
      "FROM crs_pnr,crs_pax,tlg_binding "
      "WHERE crs_pax.pnr_id=crs_pnr.pnr_id AND "
      "      crs_pnr.point_id=tlg_binding.point_id_tlg(+) AND "
      "      crs_pnr.pnr_id=:pnr_id AND crs_pax.pr_del=0 AND "
      "      tlg_binding.point_id_spp(+)=:point_id AND rownum<2";
  	Qry.CreateVariable( "point_id", otInteger, point_id );
  	Qry.CreateVariable( "pnr_id", otInteger, pnr_id );
  	Qry.Execute();
    if ( Qry.Eof )
    	throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );
    if ( Qry.FieldIsNULL( "point_id_spp" ) )
    	throw UserException( "MSG.PASSENGERS.OTHER_FLIGHT" );
  }
  else
  {
    Qry.SQLText =
      "SELECT id FROM test_pax WHERE id=:pnr_id";
    Qry.CreateVariable( "pnr_id", otInteger, pnr_id );
  	Qry.Execute();
    if ( Qry.Eof )
    	throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );
  };
}

//��। �맮��� ������ ���� ���⠢��� pnrData.pnr_id!
void GetPNRCodeshare(const TSearchPnrData &pnrData,
                     TTripInfo &operFlt,
                     TTripInfo &pnrMarkFlt,
                     TCodeShareSets &codeshareSets)
{
  operFlt.Clear();
  pnrMarkFlt.Clear();
  codeshareSets.clear();

  //䠪��᪨� ��ॢ��稪
  operFlt.airline=pnrData.airline;
  operFlt.flt_no=pnrData.flt_no;
  operFlt.suffix=pnrData.suffix;
  operFlt.airp=pnrData.airp_dep;
  operFlt.scd_out=pnrData.scd_out;

  if (!pnrData.is_test)
  {
    //�������᪨� ३� PNR
    TMktFlight mktFlt;
    mktFlt.getByPnrId(pnrData.pnr_id);
    if (mktFlt.IsNULL())
      throw EXCEPTIONS::Exception("GetPNRCodeshare: empty TMktFlight (pnr_id=%d)",pnrData.pnr_id);

    pnrMarkFlt.airline=mktFlt.airline;
    pnrMarkFlt.flt_no=mktFlt.flt_no;
    pnrMarkFlt.suffix=mktFlt.suffix;
    pnrMarkFlt.airp=mktFlt.airp_dep;
    pnrMarkFlt.scd_out=mktFlt.scd_date_local;
  }
  else
  {
    pnrMarkFlt.airline=operFlt.airline;
    pnrMarkFlt.flt_no=operFlt.flt_no;
    pnrMarkFlt.suffix=operFlt.suffix;
    pnrMarkFlt.airp=operFlt.airp;
    pnrMarkFlt.scd_out=UTCToLocal(operFlt.scd_out, AirpTZRegion(operFlt.airp));
  };

  codeshareSets.get(operFlt,pnrMarkFlt);
};

class TBagNormInfo
{
  public:
    int bag_type;
    TBagNormType norm_type;
    int norm_id;
    int amount;
    int weight;
    int per_unit;
    bool norm_trfer;
    void clear()
    {
      bag_type=ASTRA::NoExists;
      norm_type=bntUnknown;
      norm_id=ASTRA::NoExists;
      amount=ASTRA::NoExists;
      weight=ASTRA::NoExists;
      per_unit=ASTRA::NoExists;
      norm_trfer=false;
    };
    TBagNormInfo()
    {
      clear();
    };
};

class TPaxInfo
{
  public:
    string pax_cat;
    string target;
    string final_target;
    string subcl;
    string cl;
};

class TBagNormFields
{
  public:
    //string name;
    int amount;
    bool filtered;
    int fieldIdx;
    otFieldType fieldType;
    string value;
    TBagNormFields(/*string pname,*/ int pamount, bool pfiltered):
                   /*name(pname),*/amount(pamount),filtered(pfiltered)
    {};
};

void GetPaxBagNorm(TQuery &Qry, const bool use_mixed_norms, const TPaxInfo &pax,
                   TBagNormInfo &norm, bool onlyCategory)
{
  static map<string,TBagNormFields> BagNormFields;
  if (BagNormFields.empty())
  {
    BagNormFields.insert(make_pair("city_arv",TBagNormFields(  1, false)));
    BagNormFields.insert(make_pair("pax_cat", TBagNormFields(100, false)));
    BagNormFields.insert(make_pair("subclass",TBagNormFields( 50, false)));
    BagNormFields.insert(make_pair("class",   TBagNormFields(  1, false)));
    BagNormFields.insert(make_pair("flt_no",  TBagNormFields(  1, true)));
    BagNormFields.insert(make_pair("craft",   TBagNormFields(  1, true)));
  };

  map<string,TBagNormFields>::iterator i;

  for(i=BagNormFields.begin();i!=BagNormFields.end();i++)
  {
    i->second.fieldIdx=Qry.FieldIndex(i->first);
    i->second.fieldType=Qry.FieldType(i->second.fieldIdx);
    i->second.value.clear();
  };

  if ((i=BagNormFields.find("pax_cat"))!=BagNormFields.end())
    i->second.value=pax.pax_cat;
  if ((i=BagNormFields.find("subclass"))!=BagNormFields.end())
    i->second.value=pax.subcl;
  if ((i=BagNormFields.find("class"))!=BagNormFields.end())
    i->second.value=pax.cl;

  bool pr_basic=(!Qry.Eof && Qry.FieldIsNULL("airline"));
  int max_amount=ASTRA::NoExists;
  int curr_amount=ASTRA::NoExists;
  TBagNormInfo max_norm,curr_norm;

  int pr_trfer=(int)(!pax.final_target.empty());
  for(;pr_trfer>=0;pr_trfer--)
  {
    if ((i=BagNormFields.find("city_arv"))!=BagNormFields.end())
    {
      i->second.value.clear();
      if (pr_trfer!=0)
        i->second.value=pax.final_target;
      else
        i->second.value=pax.target;
    };

    for(;!Qry.Eof;Qry.Next())
    {
      curr_norm.clear();

      if (Qry.FieldIsNULL("airline") && !pr_basic) continue;
      if (!Qry.FieldIsNULL("pr_trfer") && (Qry.FieldAsInteger("pr_trfer")!=0)!=(pr_trfer!=0)) continue;
      if (!Qry.FieldIsNULL("bag_type"))
        curr_norm.bag_type=Qry.FieldAsInteger("bag_type");
      if (curr_norm.bag_type!=norm.bag_type) continue;
      if (onlyCategory && Qry.FieldAsString("pax_cat")!=pax.pax_cat) continue;

      curr_norm.norm_type=DecodeBagNormType(Qry.FieldAsString("norm_type"));
      curr_norm.norm_id=Qry.FieldAsInteger("id");
      if (!Qry.FieldIsNULL("amount"))
        curr_norm.amount=Qry.FieldAsInteger("amount");
      if (!Qry.FieldIsNULL("weight"))
        curr_norm.weight=Qry.FieldAsInteger("weight");
      if (!Qry.FieldIsNULL("per_unit"))
        curr_norm.per_unit=(int)(Qry.FieldAsInteger("per_unit")!=0);
      curr_norm.norm_trfer=pr_trfer!=0;

      curr_amount=0;
      i=BagNormFields.begin();
      for(;i!=BagNormFields.end();i++)
      {
        if (Qry.FieldIsNULL(i->second.fieldIdx)) continue;
        if (i->second.filtered)
          curr_amount+=i->second.amount;
        else
          if (!i->second.value.empty())
          {
            if (i->second.value==Qry.FieldAsString(i->second.fieldIdx))
              curr_amount+=i->second.amount;
            else
              break;
          }
          else break;
      };
      if (i==BagNormFields.end())
      {
        if (max_amount==ASTRA::NoExists || curr_amount>max_amount) max_norm=curr_norm;
      };
    };
    if (pr_trfer!=0 && max_amount!=ASTRA::NoExists || !use_mixed_norms) break;
  };
  norm=max_norm;
};

void filterPax( int point_id,
                vector<TPnrInfo> &pnr )
{
  if (pnr.empty()) return;

  TTripRoute route;
  route.GetRouteAfter( NoExists,
                       point_id,
                       trtNotCurrent,
                       trtNotCancelled );

  for(vector<TPnrInfo>::iterator iPnr=pnr.begin();iPnr!=pnr.end();)
  {
    vector< pair<int,int> >::iterator i=iPnr->point_dep.begin();
    for(;i!=iPnr->point_dep.end();i++)
      if (i->first==point_id) break;

    if (i!=iPnr->point_dep.end())
    {
      TTripRoute::iterator r=route.begin();
      for ( ; r!=route.end(); r++ )
        if ( r->airp == iPnr->airp_arv )
        {
          //��諨 �㭪� �����祭��
          i->second=r->point_id;
          break;
        };
      if (r==route.end())
      {
        //�� ��諨 �㭪� �����祭��

        iPnr->point_dep.erase(i);
        if (iPnr->point_dep.empty())
        {
          iPnr==pnr.erase(iPnr);
          continue;
        };
      };
    };
    iPnr++;
  };
};

void filterPax( const string &pnr_addr,
                const string &ticket_no,
                const string &document,
                const int reg_no,
                vector<TPnrInfo> &pnr)
{
  if (pnr.empty()) return;

  string pnr_addr_inter;
	if ( !pnr_addr.empty() ) {
	  pnr_addr_inter = convert_pnr_addr( pnr_addr, true );
	  ProgTrace( TRACE5, "convert_pnr_addr=%s", pnr_addr_inter.c_str() );
	}

	TQuery QryPnrAddr(&OraSession);
	QryPnrAddr.SQLText =
	  "SELECT pnr_id,airline,addr "
    "FROM pnr_addrs "
    "WHERE pnr_id=:pnr_id";
  QryPnrAddr.DeclareVariable( "pnr_id", otInteger );
  TQuery QryTicket(&OraSession);
  QryTicket.SQLText =
    "SELECT pax_id "
    "FROM crs_pax_tkn "
    "WHERE pax_id=:pax_id AND ticket_no=:ticket_no";
  QryTicket.DeclareVariable( "pax_id", otInteger );
  QryTicket.CreateVariable( "ticket_no", otString, ticket_no );
  TQuery QryDoc(&OraSession);
  QryDoc.SQLText =
    "SELECT pax_id "
    "FROM crs_pax_doc "
    "WHERE pax_id=:pax_id AND no=:document";
  QryDoc.DeclareVariable( "pax_id", otInteger );
  QryDoc.CreateVariable( "document", otString, document );
  TQuery QryPax(&OraSession);
  QryPax.SQLText =
    "SELECT pax_id "
    "FROM pax "
    "WHERE pax_id=:pax_id AND reg_no=:reg_no";
  QryPax.DeclareVariable( "pax_id", otInteger );
  QryPax.DeclareVariable( "reg_no", otInteger );

 	for(vector<TPnrInfo>::iterator iPnr=pnr.begin();iPnr!=pnr.end();)
 	{
    for(vector<int>::iterator iPax=iPnr->pax_id.begin();iPax!=iPnr->pax_id.end();)
    {
      //�����
    	if ( !ticket_no.empty() ) { // �᫨ ����� ���� �� ������, � ������ �஢���
        QryTicket.SetVariable( "pax_id", *iPax );
        QryTicket.Execute();
        if ( QryTicket.Eof && ticket_no.size() == 14 ) {
        	QryTicket.SetVariable( "ticket_no", ticket_no.substr(0,13) );
        	QryTicket.Execute();
        }
        if (QryTicket.Eof)
        {
          iPax=iPnr->pax_id.erase(iPax); //�� ���室�� pax_id - 㤠�塞
          continue;
        };
      };
      //ProgTrace( TRACE5, "after search ticket, pr_find=%d", pr_find );

      //���㬥��
      if ( !document.empty() ) { // �᫨ �।. �஢�ઠ 㤮��. � �᫨ ����� ���㬥��, � ������ �஢���
      	QryDoc.SetVariable( "pax_id", *iPax );
      	QryDoc.Execute();
      	if (QryDoc.Eof)
      	{
          iPax=iPnr->pax_id.erase(iPax); //�� ���室�� pax_id - 㤠�塞
          continue;
        };
      }
      //ProgTrace( TRACE5, "after search document, pr_find=%d", pr_find );
      
      //ॣ.�����
      if ( reg_no!=NoExists ) { // �᫨ �।. �஢�ઠ 㤮��. � �᫨ ����� ॣ.�����, � ������ �஢���
      	QryPax.SetVariable( "pax_id", *iPax );
      	QryPax.SetVariable( "reg_no", reg_no );
      	QryPax.Execute();
      	if (QryPax.Eof)
      	{
          iPax=iPnr->pax_id.erase(iPax); //�� ���室�� pax_id - 㤠�塞
          continue;
        };
      }
      //ProgTrace( TRACE5, "after search reg_no, pr_find=%d", pr_find );

      iPax++;
    };

    if (iPnr->pax_id.empty()) //�� ������ ���ᠦ�� �� ��諮 �஢��� � PNR
    {
      iPnr=pnr.erase(iPnr);
      continue;
    };

    //����� PNR (�� ⮫쪮 �஢���� �� � ������ � �������� TPnrInfo)
    bool pr_find=pnr_addr.empty();
    QryPnrAddr.SetVariable( "pnr_id", iPnr->pnr_id );
  	QryPnrAddr.Execute();
    for ( ; !QryPnrAddr.Eof; QryPnrAddr.Next() )
  	{
  	  // �஡�� �� �ᥬ ���ᠬ � ���� �� �᫮��� + ���������� ����� � ���ᠬ�
  		TPnrAddr addr(QryPnrAddr.FieldAsString( "airline" ),
                    QryPnrAddr.FieldAsString( "addr" ));
  		iPnr->pnr_addrs.push_back( addr );

  		if ( !pnr_addr.empty() && pnr_addr_inter == addr.addr_inter )
  			pr_find = true;
  	};

  	if (!pr_find)
  	{
  	  iPnr=pnr.erase(iPnr);
      continue;
    };
    //ProgTrace( TRACE5, "after search pnr_addr, pr_find=%d", pr_find );

 	  iPnr++;
 	};
};

void addPax( TQuery &Qry,
             int point_dep,
             int test_pax_id,
             vector<TPnrInfo> &pnr )
{
  int pnr_id=Qry.FieldAsInteger("pnr_id");
  int pax_id=Qry.FieldAsInteger("pax_id");
  vector<TPnrInfo>::iterator iPnr=pnr.begin();
  for(;iPnr!=pnr.end();iPnr++)
  {
    if (iPnr->pnr_id==pnr_id) break;
  };
  if (iPnr!=pnr.end())
  {
    if (find(iPnr->pax_id.begin(),
             iPnr->pax_id.end(),pax_id)==iPnr->pax_id.end())
      iPnr->pax_id.push_back(pax_id);

    vector< pair<int,int> >::iterator i=iPnr->point_dep.begin();
    for(;i!=iPnr->point_dep.end();i++)
      if (i->first==point_dep) break;
    if (i==iPnr->point_dep.end())
      iPnr->point_dep.push_back( make_pair(point_dep,NoExists) );
  }
  else
  {
    TPnrInfo pnrInfo;
    pnrInfo.pnr_id=pnr_id;
    pnrInfo.airp_arv=Qry.FieldAsString("airp_arv");
    pnrInfo.cl=Qry.FieldAsString("class");
    pnrInfo.subcl=Qry.FieldAsString("subclass");
    pnrInfo.pax_id.push_back(Qry.FieldAsInteger("pax_id"));
    if (test_pax_id!=NoExists)
    {
      TPnrAddr addr(Qry.FieldAsString("pnr_airline"),
                    Qry.FieldAsString("pnr_addr"));
      pnrInfo.pnr_addrs.push_back( addr );
      pnrInfo.point_dep.push_back( make_pair(point_dep,Qry.FieldAsInteger("point_arv")) );
    }
    else
    {
      pnrInfo.point_dep.push_back( make_pair(point_dep,NoExists) );
    };
    pnr.push_back(pnrInfo);
  };
};

void addTestPax( int point_dep,
                 int test_pax_id,
                 vector<TPnrInfo> &pnr )
{
  TTripRouteItem next;
  TTripRoute().GetNextAirp(NoExists,point_dep,trtNotCancelled,next);
  if (next.point_id==NoExists || next.airp.empty()) return;

  TQuery Qry(&OraSession);
  Qry.Clear();
	Qry.SQLText=
	  "SELECT id AS pnr_id, id AS pax_id, "
	  "       :point_arv AS point_arv, :airp_arv AS airp_arv, "
    "       subcls.class, subclass, "
	  "       pnr_airline, pnr_addr "
	  "FROM test_pax, subcls "
	  "WHERE test_pax.subclass=subcls.code AND test_pax.id=:test_pax_id";
	Qry.CreateVariable("test_pax_id", otInteger, test_pax_id);
	Qry.CreateVariable("point_arv", otInteger, next.point_id);
	Qry.CreateVariable("airp_arv", otString, next.airp);
	Qry.Execute();
	if (Qry.Eof) return;
	addPax(Qry, point_dep, test_pax_id, pnr);
};

int findTestPaxId( const TTripInfo &flt,
                   const string &surname,
                   const string &pnr_addr,
                   const string &ticket_no,
                   const string &document,
                   const int reg_no)
{
  if ( surname.empty() || pnr_addr.empty() && ticket_no.empty() && document.empty() ) return NoExists;
  
  TQuery Qry(&OraSession);
  Qry.Clear();
	Qry.SQLText=
    "SELECT id, airline, surname, doc_no, tkn_no, pnr_addr, reg_no FROM test_pax "
    "WHERE (airline=:airline OR airline IS NULL) "
    "ORDER BY airline NULLS LAST, id";
  Qry.CreateVariable("airline", otString, flt.airline);
  Qry.Execute();
  if (Qry.Eof) return NoExists;
  string airline=Qry.FieldAsString("airline");
  for(;!Qry.Eof;Qry.Next())
  {
    if (airline!=Qry.FieldAsString("airline")) break;
    if (!transliter_equal(surname,Qry.FieldAsString("surname"))) continue;
    if (!document.empty() &&
        document!=Qry.FieldAsString("doc_no")) continue;
    if (!ticket_no.empty() &&
        ticket_no!=Qry.FieldAsString("tkn_no")) continue;
    if (!pnr_addr.empty() &&
        convert_pnr_addr(pnr_addr, true)!=convert_pnr_addr(Qry.FieldAsString("pnr_addr"), true)) continue;
    if (reg_no!=NoExists &&
        reg_no!=Qry.FieldAsInteger("reg_no")) continue;
    if (!isTestPaxId(Qry.FieldAsInteger("id"))) continue;
    return Qry.FieldAsInteger("id");
  };
  return NoExists;
};

void findPnr( const TTripInfo &flt,
              const string &surname,
              const string &pnr_addr,
              const string &ticket_no,
              const string &document,
              const int reg_no,
              const int test_pax_id,
              vector<TPnrInfo> &pnr,
              int pass )
{
  ProgTrace( TRACE5, "surname=%s, pnr_addr=%s, ticket_no=%s, document=%s, reg_no=%s, pass=%d",
	           surname.c_str(),
             pnr_addr.c_str(),
             ticket_no.c_str(),
             document.c_str(),
             reg_no==NoExists?"":IntToString(reg_no).c_str(),
             pass );
	if ( surname.empty() )
  	throw UserException( "MSG.PASSENGER.NOT_SET.SURNAME" );
  if ( pnr_addr.empty() && ticket_no.empty() && document.empty() )
  	throw UserException( "MSG.NOTSET.SEARCH_PARAMS" );

  TReqInfo *reqInfo = TReqInfo::Instance();

  TDateTime scd;
  modf( flt.scd_out, &scd );

	TQuery Qry(&OraSession);
	if (pass==1)
	{
	  //�饬 䠪��᪨� ३�
	  Qry.Clear();
	  Qry.SQLText=
  	  "SELECT point_id,airline,airp,scd_out "
  	  "FROM points "
  	  "WHERE airline=:airline AND "
  	  "      flt_no=:flt_no AND "
  	  "      ( suffix IS NULL AND :suffix IS NULL OR suffix=:suffix ) AND "
  	  "      scd_out >= :first_date AND scd_out <= :last_date AND "
  	  "      pr_del=0 AND pr_reg<>0";

  	Qry.CreateVariable( "airline", otString, flt.airline );
  	Qry.CreateVariable( "flt_no", otInteger, flt.flt_no );
  	Qry.CreateVariable( "suffix", otString, flt.suffix );
  	Qry.CreateVariable( "first_date", otDate, scd-1 );
  	Qry.CreateVariable( "last_date", otDate, scd+2 );
  	Qry.Execute();

  	vector<int> point_ids;
  	for(;!Qry.Eof;Qry.Next())
  	{
  	  if ( !reqInfo->CheckAirline(Qry.FieldAsString("airline")) ||
           !reqInfo->CheckAirp(Qry.FieldAsString("airp")) ) continue;
  	  TDateTime scd_local=UTCToLocal(Qry.FieldAsDateTime("scd_out"),AirpTZRegion(Qry.FieldAsString("airp")));
  	  modf(scd_local,&scd_local);
  	  if (scd_local!=scd) continue;
  	  point_ids.push_back(Qry.FieldAsInteger("point_id"));
  	};

  	if (point_ids.empty()) return; //�� ��諨 ३�

  	Qry.Clear();
  	Qry.SQLText=
  	  "SELECT crs_pnr.pnr_id, "
	    "       crs_pnr.airp_arv, "
	    "       crs_pnr.class, "
	    "       crs_pnr.subclass, "
	    "       crs_pax.pax_id "
      "FROM tlg_binding,crs_pnr,crs_pax "
      "WHERE tlg_binding.point_id_tlg=crs_pnr.point_id AND "
      "      crs_pax.pnr_id=crs_pnr.pnr_id AND "
      "      tlg_binding.point_id_spp=:point_id AND "
      "      crs_pnr.system='CRS' AND "
      "      system.transliter_equal(crs_pax.surname,:surname)<>0 AND "
      "      crs_pax.pr_del=0";
    Qry.DeclareVariable("point_id", otInteger);
    Qry.CreateVariable("surname", otString, surname);
    for(vector<int>::const_iterator i=point_ids.begin();i!=point_ids.end();i++)
    {
      if (test_pax_id==NoExists)
      {
        Qry.SetVariable("point_id",*i);
        Qry.Execute();
        for(;!Qry.Eof;Qry.Next())
        {
          addPax(Qry, *i, test_pax_id, pnr);
        };
      }
      else
      {
        addTestPax(*i, test_pax_id, pnr);
      };

      ostringstream descr;
      descr << "point_id_spp=" << *i;
      tracePax(TRACE5, descr.str(), pass, pnr);
      if (test_pax_id==NoExists)
      {
        filterPax(pnr_addr, ticket_no, document, reg_no, pnr);
        tracePax(TRACE5, descr.str(), pass, pnr);
        filterPax(*i, pnr);
        tracePax(TRACE5, descr.str(), pass, pnr);
      };
    };
	};
	if (pass==2)
	{
	  //�饬 �।� ⥫��ࠬ���� ३ᮢ
	  Qry.Clear();
	  Qry.SQLText=
	    "SELECT point_id "
	    "FROM tlg_trips "
	    "WHERE scd=:scd AND pr_utc=0 AND "
      "      airline=:airline AND flt_no=:flt_no AND "
     	"      (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) ";

    Qry.CreateVariable("scd", otDate, scd);
    Qry.CreateVariable("airline", otString, flt.airline);
    Qry.CreateVariable("flt_no", otInteger, flt.flt_no);
    Qry.CreateVariable("suffix", otString, flt.suffix);
    Qry.Execute();

    vector<int> point_ids_tlg;
  	for(;!Qry.Eof;Qry.Next())
  	  point_ids_tlg.push_back(Qry.FieldAsInteger("point_id"));

  	if (point_ids_tlg.empty()) return; //�� ��諨 ३�

  	Qry.Clear();
	  Qry.SQLText=
	    "SELECT crs_pnr.pnr_id, "
	    "       crs_pnr.airp_arv, "
	    "       crs_pnr.class, "
	    "       crs_pnr.subclass, "
	    "       crs_pax.pax_id "
     	"FROM crs_pnr,crs_pax,pnr_market_flt "
     	"WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
     	"      crs_pnr.pnr_id=pnr_market_flt.pnr_id(+) AND "
     	"      crs_pnr.point_id=:point_id AND "
     	"      crs_pnr.system='CRS' AND "
     	"      system.transliter_equal(crs_pax.surname,:surname)<>0 AND "
      "      crs_pax.pr_del=0 AND "
     	"      pnr_market_flt.pnr_id IS NULL";
    Qry.CreateVariable("surname", otString, surname);
    Qry.DeclareVariable("point_id", otInteger);

  	TQuery PointsQry(&OraSession);
  	PointsQry.Clear();
  	PointsQry.SQLText=
  	  "SELECT points.point_id,airline,airp "
  	  "FROM points,tlg_binding "
  	  "WHERE points.point_id=tlg_binding.point_id_spp AND "
  	  "      point_id_tlg=:point_id AND "
  	  "      pr_del=0 AND pr_reg<>0 ";
  	PointsQry.DeclareVariable("point_id", otInteger);
  	for(vector<int>::iterator i=point_ids_tlg.begin();i!=point_ids_tlg.end();i++)
    {
      vector<int> point_ids_spp;
      PointsQry.SetVariable("point_id",*i);
      PointsQry.Execute();
      for(;!PointsQry.Eof;PointsQry.Next())
      {
        //横� �� ॠ��� ३ᠬ
        if ( !reqInfo->CheckAirline(PointsQry.FieldAsString("airline")) ||
             !reqInfo->CheckAirp(PointsQry.FieldAsString("airp")) ) continue;
        point_ids_spp.push_back(PointsQry.FieldAsInteger("point_id"));
      };

      if (point_ids_spp.empty()) continue;

      if (test_pax_id==NoExists)
      {
        Qry.SetVariable("point_id",*i);
        Qry.Execute();
        for(;!Qry.Eof;Qry.Next())
        {
          for(vector<int>::const_iterator j=point_ids_spp.begin();j!=point_ids_spp.end();j++)
            addPax(Qry, *j, test_pax_id, pnr);
        };
      }
      else
      {
        for(vector<int>::const_iterator j=point_ids_spp.begin();j!=point_ids_spp.end();j++)
          addTestPax(*j, test_pax_id, pnr);
      };

      ostringstream descr;
      descr << "point_id_tlg=" << *i;
      tracePax(TRACE5, descr.str(), pass, pnr);
      if (test_pax_id==NoExists)
      {
        filterPax(pnr_addr, ticket_no, document, reg_no, pnr);
        tracePax(TRACE5, descr.str(), pass, pnr);
        for(vector<int>::iterator j=point_ids_spp.begin();j!=point_ids_spp.end();j++)
          filterPax(*j, pnr);
        tracePax(TRACE5, descr.str(), pass, pnr);
      };
    };

	};
	if (pass==3 && test_pax_id==NoExists)
	{
	  TQuery PointsQry(&OraSession);
  	PointsQry.Clear();
  	PointsQry.SQLText=
  	  "SELECT points.point_id,airline,airp "
  	  "FROM points,tlg_binding "
  	  "WHERE points.point_id=tlg_binding.point_id_spp AND "
  	  "      point_id_tlg=:point_id AND "
  	  "      pr_del=0 AND pr_reg<>0 ";
  	PointsQry.DeclareVariable("point_id", otInteger);

	  //�饬 �।� �-⮢ .M �� PNL
	  Qry.Clear();
	  Qry.SQLText=
	    "SELECT tlg_trips.point_id, "
	    "       tlg_trips.scd, "
	    "       crs_pnr.pnr_id, "
	    "       crs_pnr.airp_arv, "
	    "       crs_pnr.class, "
	    "       crs_pnr.subclass, "
	    "       crs_pax.pax_id "
 	    "FROM tlg_trips,crs_pnr,crs_pax,pnr_market_flt "
 	    "WHERE tlg_trips.point_id=crs_pnr.point_id AND "
 	    "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
 	    "      crs_pnr.pnr_id=pnr_market_flt.pnr_id AND "
 	    "      pnr_market_flt.local_date=:scd_day AND "
 	    "      pnr_market_flt.airline=:airline AND pnr_market_flt.flt_no=:flt_no AND "
 	    "      (pnr_market_flt.suffix IS NULL AND :suffix IS NULL OR pnr_market_flt.suffix=:suffix) AND "
 	    "      crs_pnr.system='CRS' AND "
 	    "      system.transliter_equal(crs_pax.surname,:surname)<>0 AND "
      "      crs_pax.pr_del=0 "
      "ORDER BY tlg_trips.point_id";
    int year,month,day;
    DecodeDate(flt.scd_out,year,month,day);

    Qry.CreateVariable("scd_day", otInteger, day);
    Qry.CreateVariable("airline", otString, flt.airline);
    Qry.CreateVariable("flt_no", otInteger, flt.flt_no);
    Qry.CreateVariable("suffix", otString, flt.suffix);
    Qry.CreateVariable("surname", otString, surname);
    Qry.Execute();
    int point_id_tlg=NoExists;
    TDateTime scd_local;
    vector<int> point_ids_spp;

    if (Qry.Eof) return;
    for(;!Qry.Eof;Qry.Next())
    {
      if (point_id_tlg!=Qry.FieldAsInteger("point_id"))
      {
        if (point_id_tlg!=NoExists)
        {
          ostringstream descr;
          descr << "point_id_tlg=" << point_id_tlg;
          tracePax(TRACE5, descr.str(), pass, pnr);

          for(vector<int>::iterator j=point_ids_spp.begin();j!=point_ids_spp.end();j++)
           filterPax(*j, pnr);

          tracePax(TRACE5, descr.str(), pass, pnr);
        };
        point_id_tlg=Qry.FieldAsInteger("point_id");
        point_ids_spp.clear();

        scd_local = DayToDate(day, Qry.FieldAsDateTime("scd"), true);
        if (scd_local!=scd) continue;

        PointsQry.SetVariable("point_id",point_id_tlg);
        PointsQry.Execute();
        for(;!PointsQry.Eof;PointsQry.Next())
        {
          //横� �� ॠ��� ३ᠬ
          if ( !reqInfo->CheckAirline(PointsQry.FieldAsString("airline")) ||
               !reqInfo->CheckAirp(PointsQry.FieldAsString("airp")) ) continue;
          point_ids_spp.push_back(PointsQry.FieldAsInteger("point_id"));
        };
      };
      if (point_ids_spp.empty()) continue;

      for(vector<int>::iterator j=point_ids_spp.begin();j!=point_ids_spp.end();j++)
        addPax(Qry, *j, test_pax_id, pnr);
    };
    if (point_id_tlg!=NoExists)
    {
      ostringstream descr;
      descr << "point_id_tlg=" << point_id_tlg;
      tracePax(TRACE5, descr.str(), pass, pnr);

      for(vector<int>::iterator j=point_ids_spp.begin();j!=point_ids_spp.end();j++)
        filterPax(*j, pnr);

      tracePax(TRACE5, descr.str(), pass, pnr);
    };
    filterPax(pnr_addr, ticket_no, document, reg_no, pnr);
    tracePax(TRACE5, "", pass, pnr);
  };
};

/* �㭪� ��ᠤ�� � PNL ����� �� ᮢ������ � �㭪⮬ ��ᠤ�� � ��� */
void WebRequestsIface::SearchFlt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
/*
<SearchFlt>
  <airline>
  <flt_no>�᫮(1-5 ���)
  <suffix>
  <scd_out>DD.MM.YYYY HH24:MI:SS
  <surname>  � ���᪥ ३� ��易⥫쭮 �㦭� ��� �� ���ᠦ��� (��� �������ᠤ���� ३ᮢ)
  <pnr_addr>  ����� PNR
  <ticket_no> ����� �����
  <document>  ����� ���㬥��
  <reg_no> �᫨ ⥣ 㪠���, � ���� �஢�ઠ �� ᮢ������� ���������� ���ᠦ�� � ��� ॣ. ����஬
           �।���������� �ᯮ�짮���� ⥣ �� �⬥�� ॣ����樨 � ᠩ� ��� ���᪠
</SearchFlt>
*/

  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

  ProgTrace(TRACE1,"WebRequestsIface::SearchFlt");
  xmlNodePtr node2=reqNode->children;
  string surname = NodeAsStringFast( "surname", node2, "" );
  string pnr_addr = NodeAsStringFast( "pnr_addr", node2, "" );
  string ticket_no = NodeAsStringFast( "ticket_no", node2, "" );
  string document = NodeAsStringFast( "document", node2, "" );

  TTripInfo flt;
  TElemFmt fmt;
  string value;
  value = NodeAsStringFast( "airline", node2, "" );
  value = TrimString( value );
  if ( value.empty() )
   	throw UserException( "MSG.AIRLINE.NOT_SET" );

  flt.airline = ElemToElemId( etAirline, value, fmt );
  if (fmt==efmtUnknown)
  	throw UserException( "MSG.AIRLINE.INVALID",
  		                   LParams()<<LParam("airline", value ) );

  string str_flt_no = NodeAsStringFast( "flt_no", node2, "" );
	if ( StrToInt( str_flt_no.c_str(), flt.flt_no ) == EOF ||
		   flt.flt_no > 99999 || flt.flt_no <= 0 )
		throw UserException( "MSG.FLT_NO.INVALID",
			                   LParams()<<LParam("flt_no", str_flt_no) );
	flt.suffix = NodeAsStringFast( "suffix", node2, "" );
	flt.suffix = TrimString( flt.suffix );
  if (!flt.suffix.empty())
  {
    flt.suffix = ElemToElemId( etSuffix, flt.suffix, fmt );
    if (fmt==efmtUnknown)
  		throw UserException( "MSG.SUFFIX.INVALID",
  			                   LParams()<<LParam("suffix", flt.suffix) );
  };
  string str_scd_out = NodeAsStringFast( "scd_out", node2, "" );
	str_scd_out = TrimString( str_scd_out );
  if ( str_scd_out.empty() )
		throw UserException( "MSG.FLIGHT_DATE.NOT_SET" );
	else
		if ( StrToDateTime( str_scd_out.c_str(), "dd.mm.yyyy hh:nn:ss", flt.scd_out ) == EOF )
			throw UserException( "MSG.FLIGHT_DATE.INVALID",
				                   LParams()<<LParam("scd_out", str_scd_out) );

  string str_reg_no = NodeAsStringFast( "reg_no", node2, "" );
  int reg_no = NoExists;
  if (!str_reg_no.empty() &&
	    ( StrToInt( str_reg_no.c_str(), reg_no ) == EOF ||
		    reg_no > 999 || reg_no <= 0 ))
		throw UserException( "MSG.INVALID_REG_NO" );

	vector<TPnrInfo> pnr;
	int test_pax_id=findTestPaxId(flt, surname, pnr_addr, ticket_no, document, reg_no);
	findPnr(flt, surname, pnr_addr, ticket_no, document, reg_no, test_pax_id, pnr, 1);
  if (pnr.empty())
  {
    findPnr(flt, surname, pnr_addr, ticket_no, document, reg_no, test_pax_id, pnr, 2);
    findPnr(flt, surname, pnr_addr, ticket_no, document, reg_no, test_pax_id, pnr, 3);
  };
  
  if (pnr.empty())
    throw UserException( "MSG.PASSENGERS.NOT_FOUND" );
  if (pnr.size()>1)
    throw UserException( "MSG.PASSENGERS.FOUND_MORE" );
  if (pnr.begin()->pax_id.empty())
    throw UserException( "MSG.PASSENGERS.NOT_FOUND" );
  if (pnr.begin()->point_dep.empty())
    throw UserException( "MSG.FLIGHT.NOT_FOUND" );
  if (pnr.begin()->point_dep.size()>1)
    throw UserException( "MSG.FLIGHT.FOUND_MORE" );
    
  TSearchPnrData firstPnrData;
	getTripData(pnr.begin()->point_dep.begin()->first, true, firstPnrData, true);
	
	//���������� ���� firstPnrData
	firstPnrData.is_test = test_pax_id!=NoExists;
	firstPnrData.pnr_id = pnr.begin()->pnr_id;
	firstPnrData.airp_arv = pnr.begin()->airp_arv;
	firstPnrData.cls = pnr.begin()->cl;
	firstPnrData.subcls = pnr.begin()->subcl;
	firstPnrData.point_arv = pnr.begin()->point_dep.begin()->second;
	if (firstPnrData.point_arv==NoExists)
	  throw UserException( "MSG.FLIGHT.NOT_FOUND" );
	firstPnrData.pnr_addrs = pnr.begin()->pnr_addrs;
	  
  getTripData2(firstPnrData, true);

  //��।������ �������� ���� (� ��⮬ ���������� �����)
  TTripInfo operFlt,pnrMarkFlt;
  TCodeShareSets codeshareSets;
  GetPNRCodeshare(firstPnrData, operFlt, pnrMarkFlt, codeshareSets);

  TQuery BagNormsQry(&OraSession);
  BagNormsQry.Clear();
  BagNormsQry.SQLText=
   "SELECT point_id, :use_mark_flt AS use_mark_flt, "
   "       :airline_mark AS airline_mark, :flt_no_mark AS flt_no_mark, "
   "       id,bag_norms.airline,pr_trfer,city_dep,city_arv,pax_cat, "
   "       subclass,class,bag_norms.flt_no,bag_norms.craft,bag_norms.trip_type, "
   "       first_date,last_date-1/86400 AS last_date,"
   "       bag_type,amount,weight,per_unit,norm_type,extra,bag_norms.tid "
   "FROM bag_norms, "
   "     (SELECT point_id,airps.city, "
   "             DECODE(:use_mark_flt,0,airline,:airline_mark) AS airline, "
   "             DECODE(:use_mark_flt,0,flt_no,:flt_no_mark) AS flt_no, "
   "             craft,NVL(est_out,scd_out) AS scd, "
   "             trip_type, point_num, DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
   "      FROM points,airps WHERE points.airp=airps.code AND point_id=:point_id) p "
   "WHERE (bag_norms.airline IS NULL OR bag_norms.airline=p.airline) AND "
   "      (bag_norms.city_dep IS NULL OR bag_norms.city_dep=p.city) AND "
   "      (bag_norms.city_arv IS NULL OR "
   "       bag_norms.pr_trfer IS NULL OR bag_norms.pr_trfer<>0 OR "
   "       bag_norms.city_arv IN "
   "        (SELECT city FROM points p2,airps "
   "         WHERE p2.first_point=p.first_point AND p2.point_num>p.point_num AND p2.pr_del=0 AND "
   "               p2.airp=airps.code)) AND "
   "      (bag_norms.flt_no IS NULL OR bag_norms.flt_no=p.flt_no) AND "
   "      (bag_norms.craft IS NULL OR bag_norms.craft=p.craft) AND "
   "      (bag_norms.trip_type IS NULL OR bag_norms.trip_type=p.trip_type) AND "
   "      first_date<=scd AND (last_date IS NULL OR last_date>scd) AND "
   "      bag_norms.pr_del=0 AND "
   "      bag_norms.norm_type=:norm_type "
   "ORDER BY airline,DECODE(pr_trfer,0,0,NULL,0,1),id";
  BagNormsQry.CreateVariable("use_mark_flt",otInteger,codeshareSets.pr_mark_norms);
  BagNormsQry.CreateVariable("airline_mark",otString,pnrMarkFlt.airline);
  BagNormsQry.CreateVariable("flt_no_mark",otInteger,pnrMarkFlt.flt_no);
  BagNormsQry.CreateVariable("point_id",otInteger,firstPnrData.point_id);
  BagNormsQry.CreateVariable("norm_type",otString,EncodeBagNormType(bntFreeExcess));
  BagNormsQry.Execute();

  bool use_mixed_norms=GetTripSets(tsMixedNorms,operFlt);
  TPaxInfo pax;
  pax.pax_cat="";
  pax.target=firstPnrData.city_arv;
  pax.final_target=""; //�࠭��� ���� �� ��������㥬
  pax.subcl=firstPnrData.subcls;
  pax.cl=firstPnrData.cls;
  TBagNormInfo norm;

  GetPaxBagNorm(BagNormsQry, use_mixed_norms, pax, norm, false);

  firstPnrData.bag_norm = norm.weight;

  //� �⮬ ���� � ��� ��������� ���������� firstPnrData ��� ��ࢮ�� ᥣ����
  
  vector<TSearchPnrData> PNRs;
  getTCkinData( firstPnrData, PNRs);

  //� �⮬ ���� � ��� ��������� ���������� PNRs, ᮤ�ঠ騩 ��� ������ 1 ᥣ����

  xmlNodePtr segsNode=NewTextChild( NewTextChild( resNode, "SearchFlt" ), "segments");
  for(vector<TSearchPnrData>::iterator pnrData=PNRs.begin();pnrData!=PNRs.end();pnrData++)
  {
    xmlNodePtr node = NewTextChild( segsNode, "segment");
    NewTextChild( node, "point_id", pnrData->point_id );
    NewTextChild( node, "airline", pnrData->airline );
    NewTextChild( node, "flt_no", pnrData->flt_no );
    if ( !pnrData->suffix.empty() )
      NewTextChild( node, "suffix", pnrData->suffix );
    NewTextChild( node, "craft", pnrData->craft );
    if (pnrData->scd_out_local!=NoExists)
      NewTextChild( node, "scd_out", DateTimeToStr( pnrData->scd_out_local, ServerFormatDateTimeAsString ) );
    else
      NewTextChild( node, "scd_out" );
    if (pnrData->est_out_local!=NoExists)
      NewTextChild( node, "est_out", DateTimeToStr( pnrData->est_out_local, ServerFormatDateTimeAsString ) );
    else
      NewTextChild( node, "est_out" );
    if (pnrData->act_out_local!=NoExists)
      NewTextChild( node, "act_out", DateTimeToStr( pnrData->act_out_local, ServerFormatDateTimeAsString ) );
    else
      NewTextChild( node, "act_out" );
    NewTextChild( node, "airp_dep", pnrData->airp_dep );
    NewTextChild( node, "city_dep", pnrData->city_dep );
    if (pnrData->dep_utc_offset!=NoExists)
      NewTextChild( node, "dep_utc_offset", pnrData->dep_utc_offset );
    else
      NewTextChild( node, "dep_utc_offset" );
    if ( pnrData->scd_in_local != NoExists )
      NewTextChild( node, "scd_in", DateTimeToStr( pnrData->scd_in_local, ServerFormatDateTimeAsString ) );
    else
      NewTextChild( node, "scd_in" );
    if ( pnrData->est_in_local != NoExists )
      NewTextChild( node, "est_in", DateTimeToStr( pnrData->est_in_local, ServerFormatDateTimeAsString ) );
    else
      NewTextChild( node, "est_in" );
    if ( pnrData->act_in_local != NoExists )
      NewTextChild( node, "act_in", DateTimeToStr( pnrData->act_in_local, ServerFormatDateTimeAsString ) );
    else
      NewTextChild( node, "act_in" );
    NewTextChild( node, "airp_arv", pnrData->airp_arv );
    NewTextChild( node, "city_arv", pnrData->city_arv );
    if (pnrData->arv_utc_offset!=NoExists)
      NewTextChild( node, "arr_utc_offset", pnrData->arv_utc_offset );
    else
      NewTextChild( node, "arr_utc_offset" );

    TReqInfo *reqInfo = TReqInfo::Instance();
    TStage stage;
    if ( pnrData->act_out != NoExists )
    	NewTextChild( node, "status", "sTakeoff" );
    else
    {
      if ( reqInfo->client_type == ctKiosk )
        stage=pnrData->kiosk_checkin_stage;
      else
        stage=pnrData->web_checkin_stage;
      switch ( stage ) {
      	case sNoActive:
      		NewTextChild( node, "status", "sNoActive" );
      		break;
      	case sOpenWEBCheckIn:
      	  NewTextChild( node, "status", "sOpenWEBCheckIn" );
      		break;
      	case sOpenKIOSKCheckIn:
      		NewTextChild( node, "status", "sOpenKIOSKCheckIn" );
      		break;
      	case sCloseWEBCheckIn:
      	  NewTextChild( node, "status", "sCloseWEBCheckIn" );
      		break;
        case sCloseKIOSKCheckIn:
      		NewTextChild( node, "status", "sCloseKIOSKCheckIn" );
      		break;
      	case sTakeoff:
      		NewTextChild( node, "status", "sTakeoff" );
      		break;
   	  	default:;
      };
    };

    xmlNodePtr stagesNode = NewTextChild( node, "stages" );
    xmlNodePtr stageNode;
    string stage_name;
    for(int pass=0; pass<7; pass++)
    {
      switch(pass)
      {
        case 0: if ( reqInfo->client_type == ctKiosk )
                {
                  stage=sOpenKIOSKCheckIn;
                  stage_name="sOpenKIOSKCheckIn";
                }
                else
                {
                  stage=sOpenWEBCheckIn;
                  stage_name="sOpenWEBCheckIn";
                };
                break;
        case 1: if ( reqInfo->client_type == ctKiosk )
                {
                  stage=sCloseKIOSKCheckIn;
                  stage_name="sCloseKIOSKCheckIn";
                }
                else
                {
                  stage=sCloseWEBCheckIn;
                  stage_name="sCloseWEBCheckIn";
                };
                break;
        case 2: if ( reqInfo->client_type == ctKiosk )
                  continue;
                else
                {
                  stage=sCloseWEBCancel;
                  stage_name="sCloseWEBCancel";
                };
                break;
        case 3: stage=sOpenCheckIn;
                stage_name="sOpenCheckIn";
                break;
        case 4: stage=sCloseCheckIn;
                stage_name="sCloseCheckIn";
                break;
        case 5: stage=sOpenBoarding;
                stage_name="sOpenBoarding";
                break;
        case 6: stage=sCloseBoarding;
                stage_name="sCloseBoarding";
                break;
      };
      if ( pnrData->stages[ stage ] != NoExists )
      	stageNode = NewTextChild( stagesNode, "stage", DateTimeToStr( pnrData->stages[ stage ], ServerFormatDateTimeAsString ) );
      else
      	stageNode = NewTextChild( stagesNode, "stage" );
      SetProp( stageNode, "type", stage_name );
    };
    
    xmlNodePtr semNode = NewTextChild( node, "semaphors" );
    if ( reqInfo->client_type == ctKiosk )
      NewTextChild( semNode, "kiosk_checkin", (int)(pnrData->act_out == NoExists && pnrData->kiosk_checkin_stage == sOpenKIOSKCheckIn) );
    else
    {
      NewTextChild( semNode, "web_checkin", (int)(pnrData->act_out == NoExists && pnrData->web_checkin_stage == sOpenWEBCheckIn) );
      NewTextChild( semNode, "web_cancel", (int)(pnrData->act_out == NoExists && pnrData->web_cancel_stage == sOpenWEBCheckIn) );
    };
    NewTextChild( semNode, "term_checkin", (int)(pnrData->act_out == NoExists && pnrData->term_checkin_stage == sOpenCheckIn) );
    NewTextChild( semNode, "term_brd", (int)(pnrData->act_out == NoExists && pnrData->brd_stage == sOpenBoarding) );
    
    NewTextChild( node, "paid_checkin", (int)pnrData->pr_paid_ckin );

    xmlNodePtr fltsNode = NewTextChild( node, "mark_flights" );
    for(vector<TTripInfo>::iterator m=pnrData->mark_flights.begin();
                                    m!=pnrData->mark_flights.end();m++)
    {
      xmlNodePtr fltNode=NewTextChild( fltsNode, "flight" );
      NewTextChild( fltNode, "airline", m->airline );
      NewTextChild( fltNode, "flt_no", m->flt_no );
      NewTextChild( fltNode, "suffix", m->suffix );
    };

    NewTextChild( node, "pnr_id", pnrData->pnr_id );
    NewTextChild( node, "subclass", pnrData->subcls );
    if (pnrData==PNRs.begin())
    {
      if (pnrData->bag_norm!=ASTRA::NoExists)
        NewTextChild( node, "bag_norm", pnrData->bag_norm );
      else
      	NewTextChild( node, "bag_norm" );
    };
    if ( !pnrData->pnr_addrs.empty() ) {
    	node = NewTextChild( node, "pnr_addrs" );
    	for ( vector<TPnrAddr>::iterator i=pnrData->pnr_addrs.begin(); i!=pnrData->pnr_addrs.end(); i++ ) {
        xmlNodePtr addrNode = NewTextChild( node, "pnr_addr" );
        NewTextChild( addrNode, "airline", i->airline );
        NewTextChild( addrNode, "addr", i->addr );
    	}
    }
  };
}

/*
1. �᫨ ��-� 㦥 ��砫 ࠡ���� � pnr (�����,ࠧ���騪 PNL)
2. �᫨ ���ᠦ�� ��ॣ����஢����, � ࠧ���騪 PNL �⠢�� �ਧ��� 㤠�����
*/

struct TWebPax {
  int pax_no; //����㠫�� ��. ��� �裡 ������ � ⮣� �� ���ᠦ�� ࠧ��� ᥣ���⮢
	int crs_pax_id;
	int crs_pax_id_parent;
	string surname;
	string name;
	TDateTime birth_date;
	string pers_type_extended; //����� ᮤ�ঠ�� �� (CBBG)
	TCompLayerType crs_seat_layer;
	string crs_seat_no;
  string seat_no;
  string pass_class;
  string pass_subclass;
	int seats;
	int pax_id;
	int crs_pnr_tid;
	int crs_pax_tid;
	int pax_grp_tid;
	int pax_tid;
	string checkin_status;
	bool pr_eticket;
	string ticket_no;
	vector<TypeB::TFQTItem> fqt_rems;
	TWebPax() {
	  pax_no = NoExists;
		birth_date = NoExists;
		crs_pax_id = NoExists;
		crs_pax_id_parent = NoExists;
		seats = 0;
		pax_id = NoExists;
		crs_pnr_tid = NoExists;
		crs_pax_tid	= NoExists;
		pax_grp_tid = NoExists;
		pax_tid = NoExists;
		pr_eticket = false;
	};
	
	bool operator == (const TWebPax &pax) const
	{
  	return transliter_equal(surname,pax.surname) &&
           transliter_equal(name,pax.name) &&
           pers_type_extended==pax.pers_type_extended &&
           (seats==0 && pax.seats==0 || seats!=0 && pax.seats!=0);
  };
};

void verifyPaxTids( int pax_id, int crs_pnr_tid, int crs_pax_tid, int pax_grp_tid, int pax_tid )
{
  TQuery Qry(&OraSession);
  if (!isTestPaxId(pax_id))
  {
  	Qry.SQLText =
  	  "SELECT crs_pnr.tid AS crs_pnr_tid, "
      "       crs_pax.tid AS crs_pax_tid, "
      "       pax_grp.tid AS pax_grp_tid, "
      "       pax.tid AS pax_tid, "
      "       pax.pax_id "
      " FROM crs_pnr,crs_pax,pax,pax_grp "
      "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      pax.grp_id=pax_grp.grp_id(+) AND "
      "      crs_pax.pax_id=:pax_id AND "
      "      crs_pax.pr_del=0";
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.Execute();
    if ( Qry.Eof ||
    	   crs_pnr_tid != Qry.FieldAsInteger( "crs_pnr_tid" ) ||
    	   crs_pax_tid != Qry.FieldAsInteger( "crs_pax_tid" ) ||
    	   pax_grp_tid != Qry.FieldAsInteger( "pax_grp_tid" ) ||
    	   pax_tid != Qry.FieldAsInteger( "pax_tid" ) )
    	throw UserException( "MSG.PASSENGERS.GROUP_CHANGED.REFRESH_DATA" );
  }
  else
  {
    Qry.SQLText =
      "SELECT id FROM test_pax WHERE id=:pax_id";
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
  	Qry.Execute();
    if ( Qry.Eof ||
         crs_pnr_tid != Qry.FieldAsInteger( "id" ) ||
    	   crs_pax_tid != Qry.FieldAsInteger( "id" ) ||
         pax_grp_tid != pax_tid)
      throw UserException( "MSG.PASSENGERS.GROUP_CHANGED.REFRESH_DATA" );
  };
}

bool is_valid_pnr_status(const string &pnr_status)
{
  return !(//pax.name=="CBBG" ||  ���� ����� � ��ࣨ����
     		   pnr_status=="DG2" ||
     		   pnr_status=="RG2" ||
     		   pnr_status=="ID2" ||
     		   pnr_status=="WL");
};

bool is_valid_pax_status(int pax_id, TQuery& PaxQry)
{
  const char* sql=
    "SELECT time FROM crs_pax_refuse "
    "WHERE pax_id=:pax_id AND client_type=:client_type AND rownum<2";
  if (strcmp(PaxQry.SQLText.SQLText(),sql)!=0)
  {
    PaxQry.Clear();
    PaxQry.SQLText=sql;
    PaxQry.DeclareVariable("pax_id", otInteger);
    PaxQry.CreateVariable("client_type", otString, EncodeClientType(TReqInfo::Instance()->client_type));
  };
  PaxQry.SetVariable("pax_id", pax_id);
  PaxQry.Execute();
  if (!PaxQry.Eof) return false;
  return true;
};

bool is_valid_doc_info(const TCheckDocInfo &checkDocInfo,
                       const CheckIn::TPaxDocItem &doc,
                       const CheckIn::TPaxDocoItem &doco)
{
  if ((checkDocInfo.first.required_fields&doc.getNotEmptyFieldsMask())!=checkDocInfo.first.required_fields) return false;
  if ((checkDocInfo.second.required_fields&doco.getNotEmptyFieldsMask())!=checkDocInfo.second.required_fields) return false;
  return true;
};

bool is_valid_tkn_info(const TCheckDocTknInfo &checkTknInfo,
                       const CheckIn::TPaxTknItem &tkn)
{
  if ((checkTknInfo.required_fields&tkn.getNotEmptyFieldsMask())!=checkTknInfo.required_fields) return false;
  return true;
};

void getPnr( int point_id, int pnr_id, vector<TWebPax> &pnr, bool pr_throw, bool afterSave )
{
  try
  {
  	pnr.clear();

    if (!isTestPaxId(pnr_id))
    {
    	TQuery PaxDocQry(&OraSession);
    	TQuery CrsPaxDocQry(&OraSession);
    	TQuery GetPSPT2Qry(&OraSession);
    	TQuery CrsPaxDocoQry(&OraSession);
      TQuery PaxStatusQry(&OraSession);

      TQuery CrsTKNQry(&OraSession);
      CrsTKNQry.SQLText =
        "SELECT rem_code AS ticket_rem, "
        "       ticket_no, "
        "       DECODE(rem_code,'TKNE',coupon_no,NULL) AS coupon_no, "
        "       0 AS ticket_confirm "
        "FROM crs_pax_tkn "
        "WHERE pax_id=:pax_id "
        "ORDER BY DECODE(rem_code,'TKNE',0,'TKNA',1,'TKNO',2,3),ticket_no,coupon_no";
      CrsTKNQry.DeclareVariable( "pax_id", otInteger );

      TQuery FQTQry(&OraSession);
      FQTQry.DeclareVariable( "pax_id", otInteger );

      const char* PaxFQTQrySQL=
       "SELECT rem_code, airline, no, extra "
       "FROM pax_fqt WHERE pax_id=:pax_id AND rem_code='FQTV'";

      const char* CrsFQTQrySQL=
       "SELECT rem_code, airline, no, extra "
       "FROM crs_pax_fqt WHERE pax_id=:pax_id AND rem_code='FQTV'";

      TQuery SeatQry(&OraSession);
      SeatQry.SQLText=
        "BEGIN "
        "  :crs_seat_no:=salons.get_crs_seat_no(:pax_id,:xname,:yname,:seats,:point_id,:layer_type,'one',:crs_row); "
        "  :crs_row:=:crs_row+1; "
        "END;";
      SeatQry.DeclareVariable("pax_id", otInteger);
      SeatQry.DeclareVariable("xname", otString);
      SeatQry.DeclareVariable("yname", otString);
      SeatQry.DeclareVariable("seats", otInteger);
      SeatQry.DeclareVariable("point_id", otInteger);
      SeatQry.DeclareVariable("layer_type", otString);
      SeatQry.DeclareVariable("crs_row", otInteger);
      SeatQry.DeclareVariable("crs_seat_no", otString);

      TQuery Qry(&OraSession);
    	Qry.SQLText =
    	  "SELECT crs_pax.pax_id AS crs_pax_id, "
        "       crs_inf.pax_id AS crs_pax_id_parent, "
        "       DECODE(pax.pax_id,NULL,crs_pax.surname,pax.surname) AS surname, "
        "       DECODE(pax.pax_id,NULL,crs_pax.name,pax.name) AS name, "
        "       DECODE(pax.pax_id,NULL,crs_pax.pers_type,pax.pers_type) AS pers_type, "
        "       crs_pax.seat_xname, crs_pax.seat_yname, crs_pax.seats AS crs_seats, crs_pnr.point_id AS point_id_tlg, "
        "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'one',rownum) AS seat_no, "
        "       DECODE(pax.pax_id,NULL,crs_pax.seats,pax.seats) AS seats, "
        "       DECODE(pax_grp.class,NULL,crs_pnr.class,pax_grp.class) AS class, "
        "       DECODE(pax.subclass,NULL,crs_pnr.subclass,pax.subclass) AS subclass, "
        "       DECODE(pax.pax_id,NULL,crs_pnr.airp_arv,pax_grp.airp_arv) AS airp_arv, "
        "       crs_pnr.status AS pnr_status, "
        "       crs_pnr.tid AS crs_pnr_tid, "
        "       crs_pax.tid AS crs_pax_tid, "
        "       pax_grp.tid AS pax_grp_tid, "
        "       pax.tid AS pax_tid, "
        "       pax.pax_id, "
        "       pax_grp.client_type, "
        "       pax.refuse, "
        "       pax.ticket_rem, pax.ticket_no, pax.coupon_no "
        "FROM crs_pnr,crs_pax,pax,pax_grp,crs_inf "
        "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
        "      crs_pax.pax_id=pax.pax_id(+) AND "
        "      pax.grp_id=pax_grp.grp_id(+) AND "
        "      crs_pax.pax_id=crs_inf.inf_id(+) AND "
        "      crs_pnr.pnr_id=:pnr_id AND "
        "      crs_pax.pr_del=0";
      Qry.CreateVariable( "pnr_id", otInteger, pnr_id );
    	Qry.Execute();
    	SeatQry.SetVariable("crs_row", 1);
    	SeatQry.SetVariable("layer_type", FNull);
      SeatQry.SetVariable("crs_seat_no", FNull);
      TCheckDocInfo checkDocInfo;
      TCheckDocTknInfo checkTknInfo;
      bool checkInfoInit=false;
      for(;!Qry.Eof;Qry.Next())
      {
        TWebPax pax;
        pax.crs_pax_id = Qry.FieldAsInteger( "crs_pax_id" );
      	if ( !Qry.FieldIsNULL( "crs_pax_id_parent" ) )
      		pax.crs_pax_id_parent = Qry.FieldAsInteger( "crs_pax_id_parent" );
      	pax.surname = Qry.FieldAsString( "surname" );
      	pax.name = Qry.FieldAsString( "name" );
      	pax.pers_type_extended = Qry.FieldAsString( "pers_type" );
      	pax.seat_no = Qry.FieldAsString( "seat_no" );

      	SeatQry.SetVariable("pax_id", Qry.FieldAsInteger("crs_pax_id"));
        SeatQry.SetVariable("xname", Qry.FieldAsString("seat_xname"));
        SeatQry.SetVariable("yname", Qry.FieldAsString("seat_yname"));
        SeatQry.SetVariable("seats", Qry.FieldAsInteger("crs_seats"));
        SeatQry.SetVariable("point_id", Qry.FieldAsInteger("point_id_tlg"));
        SeatQry.Execute();
        pax.crs_seat_layer=DecodeCompLayerType(SeatQry.GetVariableAsString("layer_type"));
        pax.crs_seat_no=SeatQry.GetVariableAsString("crs_seat_no");
      	pax.seats = Qry.FieldAsInteger( "seats" );
      	pax.pass_class = Qry.FieldAsString( "class" );
      	pax.pass_subclass = Qry.FieldAsString( "subclass" );
      	if ( !Qry.FieldIsNULL( "pax_id" ) )
      	{
      	  //���ᠦ�� ��ॣ����஢��
      	  if ( !Qry.FieldIsNULL( "refuse" ) )
      	    pax.checkin_status = "refused";
      	  else
      	  {
      	    pax.checkin_status = "agent_checked";

        	  switch(DecodeClientType(Qry.FieldAsString( "client_type" )))
        	  {
        	    case ctWeb:
        	    case ctKiosk:
      		    	pax.checkin_status = "web_checked";
      		  		break;
      		  	default: ;
        	  };
      	  };
      		pax.pax_id = Qry.FieldAsInteger( "pax_id" );
      		CheckIn::TPaxDocItem doc;
      		LoadPaxDoc(pax.pax_id, doc, PaxDocQry);
       		pax.birth_date = doc.birth_date;
       		pax.pr_eticket = strcmp(Qry.FieldAsString("ticket_rem"),"TKNE")==0;
       		pax.ticket_no	= Qry.FieldAsString("ticket_no");
       		FQTQry.SQLText=PaxFQTQrySQL;
       		FQTQry.SetVariable( "pax_id", pax.pax_id );
       	}
       	else {
      		pax.checkin_status = "not_checked";
      		CheckIn::TPaxDocItem doc;
      		LoadCrsPaxDoc(pax.crs_pax_id, doc, CrsPaxDocQry, GetPSPT2Qry);
          pax.birth_date = doc.birth_date;
          //ProgTrace(TRACE5, "getPnr: pax.crs_pax_id=%d doc.getNotEmptyFieldsMask=%ld", pax.crs_pax_id, doc.getNotEmptyFieldsMask());
      		
       		//�஢�ઠ CBBG (��� ���� ������ � ᠫ���)
       		/*CrsPaxRemQry.SetVariable( "pax_id", pax.pax_id );
       		CrsPaxRemQry.SetVariable( "rem_code", "CBBG" );
       		CrsPaxRemQry.Execute();
       		if (!CrsPaxRemQry.Eof)*/
       		if (pax.name=="CBBG")
       		  pax.pers_type_extended = "��"; //CBBG

          CheckIn::TPaxTknItem tkn;
       		CrsTKNQry.SetVariable( "pax_id", pax.crs_pax_id );
       		CrsTKNQry.Execute();
       		if (!CrsTKNQry.Eof)
       		{
            tkn.fromDB(CrsTKNQry);
       		  pax.pr_eticket = (tkn.rem=="TKNE");
       		  pax.ticket_no = tkn.no;
       		};
       		//ProgTrace(TRACE5, "getPnr: pax.crs_pax_id=%d tkn.getNotEmptyFieldsMask=%ld", pax.crs_pax_id, tkn.getNotEmptyFieldsMask());
       		
       		if (!is_valid_pnr_status(Qry.FieldAsString("pnr_status")) ||
              !is_valid_pax_status(pax.crs_pax_id, PaxStatusQry))
            pax.checkin_status = "agent_checkin";
          else
          {
            CheckIn::TPaxDocoItem doco;
      		  LoadCrsPaxDoco(pax.crs_pax_id, doco, CrsPaxDocoQry);
      		  //ProgTrace(TRACE5, "getPnr: pax.crs_pax_id=%d doco.getNotEmptyFieldsMask=%ld", pax.crs_pax_id, doco.getNotEmptyFieldsMask());
      		  if (!checkInfoInit)
      		  {
      		    checkDocInfo=GetCheckDocInfo(point_id, Qry.FieldAsString("airp_arv"));
              checkTknInfo=GetCheckTknInfo(point_id);
              checkInfoInit=true;
              //ProgTrace(TRACE5, "getPnr: point_id=%d airp_arv=%s", point_id, Qry.FieldAsString("airp_arv"));
              //ProgTrace(TRACE5, "getPnr: checkDocInfo.first.required_fields=%ld", checkDocInfo.first.required_fields);
              //ProgTrace(TRACE5, "getPnr: checkDocInfo.second.required_fields=%ld", checkDocInfo.second.required_fields);
              //ProgTrace(TRACE5, "getPnr: checkTknInfo.required_fields=%ld", checkTknInfo.required_fields);
            };
            if (!is_valid_doc_info(checkDocInfo, doc, doco))
              pax.checkin_status = "agent_checkin";
            else
              if (!is_valid_tkn_info(checkTknInfo, tkn))
                pax.checkin_status = "agent_checkin";
          };
       		
       		FQTQry.SQLText=CrsFQTQrySQL;
       		FQTQry.SetVariable( "pax_id", pax.crs_pax_id );
       	}
       	FQTQry.Execute();
     		for(; !FQTQry.Eof; FQTQry.Next())
     		{
          TypeB::TFQTItem FQTItem;
          strcpy(FQTItem.rem_code, FQTQry.FieldAsString("rem_code"));
          strcpy(FQTItem.airline, FQTQry.FieldAsString("airline"));
          strcpy(FQTItem.no, FQTQry.FieldAsString("no"));
          FQTItem.extra = FQTQry.FieldAsString("extra");
     		  pax.fqt_rems.push_back(FQTItem);
     		};
      	pax.crs_pnr_tid = Qry.FieldAsInteger( "crs_pnr_tid" );
      	pax.crs_pax_tid = Qry.FieldAsInteger( "crs_pax_tid" );
       	if ( !Qry.FieldIsNULL( "pax_grp_tid" ) )
       		pax.pax_grp_tid = Qry.FieldAsInteger( "pax_grp_tid" );
       	if ( !Qry.FieldIsNULL( "pax_tid" ) )
       		pax.pax_tid = Qry.FieldAsInteger( "pax_tid" );
       	pnr.push_back( pax );
      }
    }
    else
    {
      TQuery Qry(&OraSession);
    	Qry.SQLText =
        "SELECT surname, subcls.class, subclass, tkn_no, "
        "       pnr_airline AS fqt_airline, fqt_no, "
        "       seat_xname, seat_yname "
        "FROM test_pax, subcls "
        "WHERE test_pax.subclass=subcls.code AND test_pax.id=:pnr_id";
      Qry.CreateVariable( "pnr_id", otInteger, pnr_id );
    	Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        //��⮢� ���ᠦ��
        TWebPax pax;
        pax.crs_pax_id = pnr_id;
        pax.surname = Qry.FieldAsString("surname");
        pax.pers_type_extended = EncodePerson(adult);
        pax.seats = 1;
        pax.pass_class = Qry.FieldAsString( "class" );
      	pax.pass_subclass = Qry.FieldAsString( "subclass" );

        if (afterSave)
        {
          pax.checkin_status = "web_checked";
          if (!Qry.FieldIsNULL("seat_yname") && !Qry.FieldIsNULL("seat_xname"))
          {
            TSeat seat(Qry.FieldAsString("seat_yname"),Qry.FieldAsString("seat_xname"));
            pax.seat_no=GetSeatView(seat, "one", false);
          };
        }
        else
          pax.checkin_status = "not_checked";

        pax.pr_eticket = true;
       	pax.ticket_no = Qry.FieldAsString("tkn_no");

        if (!Qry.FieldIsNULL("fqt_airline") && !Qry.FieldIsNULL("fqt_no"))
        {
          TypeB::TFQTItem FQTItem;
          strcpy(FQTItem.rem_code, "FQTV");
          strcpy(FQTItem.airline, Qry.FieldAsString("fqt_airline"));
          strcpy(FQTItem.no, Qry.FieldAsString("fqt_no"));
       		pax.fqt_rems.push_back(FQTItem);
        };

        pax.crs_pnr_tid = pnr_id;
      	pax.crs_pax_tid = pnr_id;
      	if (afterSave)
      	{
          pax.pax_grp_tid = point_id;
       		pax.pax_tid = point_id;
       	};

        pnr.push_back( pax );
      };
    };
    ProgTrace( TRACE5, "pass count=%d", pnr.size() );
    if ( pnr.empty() )
  	  throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );
	}
  catch(UserException &E)
  {
    ProgTrace(TRACE5, ">>>> %s", getLocaleText(E.getLexemaData()).c_str());
    if ( pr_throw )
      throw;
    else
      return;
  };
}

void IntLoadPnr( const vector<TIdsPnrData> &ids, vector< vector<TWebPax> > &pnrs, xmlNodePtr segsNode, bool afterSave )
{
  pnrs.clear();
  for(vector<TIdsPnrData>::const_iterator i=ids.begin();i!=ids.end();i++)
  {
    int point_id=i->point_id;
    int pnr_id=i->pnr_id;

    try
    {
      vector<TWebPax> pnr;
      getPnr( point_id, pnr_id, pnr, pnrs.empty(), afterSave );
      if (pnrs.begin()!=pnrs.end())
      {
        //䨫���㥬 ���ᠦ�஢ �� ��ண� � ᫥����� ᥣ���⮢
        const vector<TWebPax> &firstPnr=*(pnrs.begin());
        for(vector<TWebPax>::iterator iPax=pnr.begin();iPax!=pnr.end();)
        {
          //㤠��� �㡫�஢���� 䠬����+��� �� pnr
          bool pr_double=false;
          for(vector<TWebPax>::iterator iPax2=iPax+1;iPax2!=pnr.end();)
          {
            if (transliter_equal(iPax->surname,iPax2->surname) &&
                transliter_equal(iPax->name,iPax2->name))
            {
              pr_double=true;
              iPax2=pnr.erase(iPax2);
              continue;
            };
            iPax2++;
          };
          if (pr_double)
          {
            iPax=pnr.erase(iPax);
            continue;
          };

          vector<TWebPax>::const_iterator iPaxFirst=find(firstPnr.begin(),firstPnr.end(),*iPax);
          if (iPaxFirst==firstPnr.end())
          {
            //�� ��諨 ᮮ⢥�����饣� ���ᠦ�� �� ��ࢮ�� ᥣ����
            iPax=pnr.erase(iPax);
            continue;
          };
          iPax->pax_no=iPaxFirst->pax_no; //���⠢�塞 ���ᠦ��� ᮮ�. ����㠫�� ��. �� ��ࢮ�� ᥣ����
          iPax++;
        };
      }
      else
      {
        //���ᠦ�஢ ��ࢮ�� ᥣ���� ���⠢�� pax_no �� ���浪�
        int pax_no=1;
        for(vector<TWebPax>::iterator iPax=pnr.begin();iPax!=pnr.end();iPax++,pax_no++) iPax->pax_no=pax_no;
      };

      pnrs.push_back(pnr);

      if (segsNode==NULL) continue;

      xmlNodePtr segNode=NewTextChild(segsNode, "segment");
      NewTextChild( segNode, "point_id", point_id );
      xmlNodePtr node = NewTextChild( segNode, "passengers" );
      for ( vector<TWebPax>::const_iterator iPax=pnr.begin(); iPax!=pnr.end(); iPax++ )
      {
      	xmlNodePtr paxNode = NewTextChild( node, "pax" );
      	if (iPax->pax_no!=NoExists)
      	  NewTextChild( paxNode, "pax_no", iPax->pax_no );
      	NewTextChild( paxNode, "crs_pax_id", iPax->crs_pax_id );
      	if ( iPax->crs_pax_id_parent != NoExists )
      		NewTextChild( paxNode, "crs_pax_id_parent", iPax->crs_pax_id_parent );
      	NewTextChild( paxNode, "surname", iPax->surname );
      	NewTextChild( paxNode, "name", iPax->name );
      	if ( iPax->birth_date != NoExists )
      		NewTextChild( paxNode, "birth_date", DateTimeToStr( iPax->birth_date, ServerFormatDateTimeAsString ) );
      	NewTextChild( paxNode, "pers_type", iPax->pers_type_extended );
      	string seat_no_view;
      	if ( !iPax->seat_no.empty() )
      	  seat_no_view = iPax->seat_no;
      	else
    			if ( !iPax->crs_seat_no.empty() )
    			  seat_no_view = iPax->crs_seat_no;
      	NewTextChild( paxNode, "seat_no", seat_no_view );
        string seat_status;
        if (i->pr_paid_ckin)
        {
          switch(iPax->crs_seat_layer)
          {
            case cltPNLBeforePay:  seat_status="PNLBeforePay";  break;
            case cltPNLAfterPay:   seat_status="PNLAfterPay";   break;
            case cltProtBeforePay: seat_status="ProtBeforePay"; break;
            case cltProtAfterPay:  seat_status="ProtAfterPay";  break;
            default: break;
          };
        };
        NewTextChild( paxNode, "seat_status", seat_status );
        NewTextChild( paxNode, "seats", iPax->seats );
       	NewTextChild( paxNode, "checkin_status", iPax->checkin_status );
       	if ( iPax->pr_eticket )
       	  NewTextChild( paxNode, "eticket", "true" );
       	else
       		NewTextChild( paxNode, "eticket", "false" );
       	NewTextChild( paxNode, "ticket_no", iPax->ticket_no );
       	xmlNodePtr fqtsNode = NewTextChild( paxNode, "fqt_rems" );
       	for ( vector<TypeB::TFQTItem>::const_iterator f=iPax->fqt_rems.begin(); f!=iPax->fqt_rems.end(); f++ )
       	{
          xmlNodePtr fqtNode = NewTextChild( fqtsNode, "fqt_rem" );
          NewTextChild( fqtNode, "airline", f->airline );
          NewTextChild( fqtNode, "no", f->no );
        };

       	xmlNodePtr tidsNode = NewTextChild( paxNode, "tids" );
       	NewTextChild( tidsNode, "crs_pnr_tid", iPax->crs_pnr_tid );
       	NewTextChild( tidsNode, "crs_pax_tid", iPax->crs_pax_tid );
       	if ( iPax->pax_grp_tid != NoExists )
       		NewTextChild( tidsNode, "pax_grp_tid", iPax->pax_grp_tid );
       	if ( iPax->pax_tid != NoExists )
       		NewTextChild( tidsNode, "pax_tid", iPax->pax_tid );
      };
    }
    catch(CheckIn::UserException)
    {
      throw;
    }
    catch(UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), point_id);
    };
  };
}

void WebRequestsIface::LoadPnr(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

  ProgTrace(TRACE1,"WebRequestsIface::LoadPnr");
  xmlNodePtr segsNode = NodeAsNode( "segments", reqNode );
  vector<TIdsPnrData> ids;
  for(xmlNodePtr node=segsNode->children; node!=NULL; node=node->next)
  {
    int point_id=NodeAsInteger( "point_id", node );
    int pnr_id=NodeAsInteger( "pnr_id", node );
    TSearchPnrData SearchPnrData;
    getTripData( point_id, false, SearchPnrData, true );
    VerifyPNR( point_id, pnr_id );
    TIdsPnrData idsPnrData;
    idsPnrData.point_id=point_id;
    idsPnrData.pnr_id=pnr_id;
    idsPnrData.pr_paid_ckin=SearchPnrData.pr_paid_ckin;
    ids.push_back( idsPnrData );
  };
  
  vector< vector<TWebPax> > pnrs;
  segsNode = NewTextChild( NewTextChild( resNode, "LoadPnr" ), "segments" );
  IntLoadPnr( ids, pnrs, segsNode, false );
}

bool isOwnerFreePlace( int pax_id, const vector<TWebPax> &pnr )
{
  bool res = false;
  for ( vector<TWebPax>::const_iterator i=pnr.begin(); i!=pnr.end(); i++ ) {
  	if ( i->pax_id != NoExists )
  		continue;
  	if ( i->crs_pax_id == pax_id ) {
  		res = true;
  		break;
  	}
  }
  return res;
}

bool isOwnerPlace( int pax_id, const vector<TWebPax> &pnr )
{
  bool res = false;
  for ( vector<TWebPax>::const_iterator i=pnr.begin(); i!=pnr.end(); i++ ) {
  	if ( i->pax_id != NoExists && pax_id == i->pax_id ) {
  		res = true;
  		break;
  	}
  }
  return res;
}


struct TWebPlace {
	int x, y;
	string xname;
	string yname;
	string seat_no;
	string elem_type;
	int pr_free;
	int pr_CHIN;
	int pax_id;
	SALONS2::TPlaceWebTariff WebTariff;
	TWebPlace() {
	  pr_free = 0;
	  pr_CHIN = 0;
    pax_id = NoExists;
  }
};

typedef std::vector<TWebPlace> TWebPlaces;

struct TWebPlaceList {
	TWebPlaces places;
	int xcount, ycount;
};

void ReadWebSalons( int point_id, vector<TWebPax> pnr, map<int, TWebPlaceList> &web_salons, bool &pr_find_free_subcls_place )
{
  string crs_class, crs_subclass;
  web_salons.clear();
  bool pr_CHIN = false;
  for ( vector<TWebPax>::iterator i=pnr.begin(); i!=pnr.end(); i++ ) {
  	if ( !i->pass_class.empty() )
  	  crs_class = i->pass_class;
  	if ( !i->pass_subclass.empty() )
  	  crs_subclass = i->pass_subclass;
  	TPerson p=DecodePerson(i->pers_type_extended.c_str());
  	pr_CHIN=(p==ASTRA::child || p==ASTRA::baby); //�।� ⨯�� ����� ���� �� (CBBG) ����� ��ࠢ�������� � ���᫮��
  }
  if ( crs_class.empty() )
  	throw UserException( "MSG.CLASS.NOT_SET" );
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT airline FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  TSublsRems subcls_rems( Qry.FieldAsString("airline") );
  SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
  Salons.ClName = crs_class;
  Salons.Read();
  // ����稬 �ਧ��� ⮣�, �� � ᠫ��� ���� ᢮����� ���� � ����� �������ᮬ
  pr_find_free_subcls_place=false;
  string pass_rem;

  subcls_rems.IsSubClsRem( crs_subclass, pass_rem );

  for( vector<TPlaceList*>::iterator placeList = Salons.placelists.begin();
       placeList != Salons.placelists.end(); placeList++ ) {
    TWebPlaceList web_place_list;
    web_place_list.xcount=0;
    web_place_list.ycount=0;
    for ( TPlaces::iterator place = (*placeList)->places.begin();
          place != (*placeList)->places.end(); place++ ) { // �஡�� �� ᠫ����
      if ( !place->visible )
       continue;
      TWebPlace wp;
      wp.x = place->x;
      wp.y = place->y;
      wp.xname = place->xname;
      wp.yname = place->yname;
      if ( place->x > web_place_list.xcount )
      	web_place_list.xcount = place->x;
      if ( place->y > web_place_list.ycount )
      	web_place_list.ycount = place->y;
      wp.seat_no = denorm_iata_row( place->yname, NULL ) + denorm_iata_line( place->xname, Salons.getLatSeat() );
      if ( !place->elem_type.empty() ) {
      	if ( place->elem_type != PARTITION_ELEM_TYPE )
     	    wp.elem_type = ARMCHAIR_ELEM_TYPE;
     	  else
     	  	wp.elem_type = PARTITION_ELEM_TYPE;
     	}
     	wp.pr_free = 0;
     	wp.pr_CHIN = false;
     	wp.pax_id = NoExists;
     	wp.WebTariff.color = place->WebTariff.color;
     	wp.WebTariff.value = place->WebTariff.value;
     	wp.WebTariff.currency_id = place->WebTariff.currency_id;
     	if ( place->isplace && !place->clname.empty() && place->clname == crs_class ) {
     		bool pr_first = true;
     		for( std::vector<TPlaceLayer>::iterator l=place->layers.begin(); l!=place->layers.end(); l++ ) { // ���஢�� �� �ਮ���
     			if ( pr_first &&
     				   l->layer_type != cltUncomfort &&
     				   l->layer_type != cltSmoke &&
     				   l->layer_type != cltUnknown ) {
     				pr_first = false;
     				wp.pr_free = ( ( l->layer_type == cltPNLCkin ||
                             isUserProtectLayer( l->layer_type ) ) && isOwnerFreePlace( l->pax_id, pnr ) );
            ProgTrace( TRACE5, "l->layer_type=%s, l->pax_id=%d, isOwnerFreePlace( l->pax_id, pnr )=%d, pr_first=%d",
                       EncodeCompLayerType(l->layer_type), l->pax_id, isOwnerFreePlace( l->pax_id, pnr ), pr_first );
     				if ( wp.pr_free )
     					break;
     			}

     			if ( l->layer_type == cltCheckin ||
     				   l->layer_type == cltTCheckin ||
     				   l->layer_type == cltGoShow ||
     				   l->layer_type == cltTranzit ) {
     				pr_first = false;
            if ( isOwnerPlace( l->pax_id, pnr ) )
     				  wp.pax_id = l->pax_id;
     			}
     	  }

     	  wp.pr_free = ( wp.pr_free || pr_first ); // 0 - �����, 1 - ᢮�����, 2 - ���筮 �����

        if ( wp.pr_free ) {
        	if ( !pass_rem.empty() ) {
        	  wp.pr_free = 2; // ᢮����� ��� ��� ��������
            for ( vector<TRem>::iterator i=place->rems.begin(); i!=place->rems.end(); i++ ) {
            	if ( i->rem == pass_rem ) {
            		if ( !i->pr_denial ) {
            		  wp.pr_free = 3;  // ᢮����� � ��⮬ ��������
            		  pr_find_free_subcls_place=true;
            		}
            		break;
            	}
            }
          }
          else { // ���ᠦ�� ��� ��������
          	for ( vector<TRem>::iterator i=place->rems.begin(); i!=place->rems.end(); i++ ) {
          		if ( isREM_SUBCLS( i->rem ) ) {
          			wp.pr_free = 0;
          			break;
          		}
            }
          }
        }
        if ( pr_CHIN ) { // ��������� � ��㯯� ���ᠦ��� � ���쬨
        	if ( place->elem_type == "�" ) { // ���� � ���਩���� ��室�
       			wp.pr_CHIN = true;
          }
          else {
        	  for ( vector<TRem>::iterator i=place->rems.begin(); i!=place->rems.end(); i++ ) {
        	  	if ( i->pr_denial && i->rem == "CHIN" ) {
        	  		wp.pr_CHIN = true;
        	  		break;
        	  	}
            }
          }
        }
      } // end if place->isplace && !place->clname.empty() && place->clname == crs_class
      web_place_list.places.push_back( wp );
    }
    if ( !web_place_list.places.empty() ) {
    	web_salons[ (*placeList)->num ] = web_place_list;
    }
  }
}

int get_seat_status( TWebPlace &wp, bool pr_find_free_subcls_place )
{
  int status;
  switch( wp.pr_free ) {
  	case 0:
   		status = 1;
   		break;
   	case 1:
   		status = 0;
   		break;
   	case 2:
   		status = pr_find_free_subcls_place;
   		break;
   	case 3:
   		status = !pr_find_free_subcls_place;
   		break;
  };
  if ( status == 0 && wp.pr_CHIN ) {
   	status = 2;
  }
  return status;
}

// ��।����� ���������� ���� crs_pax_id, crs_seat_no, class, subclass
// �� ��室� ��������� TWebPlace �� ���ᠦ���
void GetCrsPaxSeats( int point_id, const vector<TWebPax> &pnr,
                     vector< pair<TWebPlace, LexemaData> > &pax_seats )
{
  pax_seats.clear();
  map<int, TWebPlaceList> web_salons;
  bool pr_find_free_subcls_place=false;
  ReadWebSalons( point_id, pnr, web_salons, pr_find_free_subcls_place );
  for ( vector<TWebPax>::const_iterator i=pnr.begin(); i!=pnr.end(); i++ ) { // �஡�� �� ���ᠦ�ࠬ
    LexemaData ld;
    bool pr_find = false;
    for( map<int, TWebPlaceList>::iterator isal=web_salons.begin(); isal!=web_salons.end(); isal++ ) {
      for ( TWebPlaces::iterator wp = isal->second.places.begin();
            wp != isal->second.places.end(); wp++ ) {
        if ( i->crs_seat_no == wp->seat_no ) {
          if ( get_seat_status( *wp, pr_find_free_subcls_place ) == 1 )
            ld.lexema_id = "MSG.SEATS.SEAT_NO.NOT_AVAIL";
          wp->pax_id = i->crs_pax_id;
          pr_find = true;
          pax_seats.push_back( make_pair( *wp, ld ) );
     	    break;
        }
      } // �஡�� �� ᠫ���
      if ( pr_find )
        break;
    } // �஡�� �� ᠫ���
    if ( !pr_find ) {
      TWebPlace wp;
      wp.pax_id = i->crs_pax_id;
      wp.seat_no = i->crs_seat_no;
      ld.lexema_id = "MSG.SEATS.SEAT_NO.NOT_FOUND";
      pax_seats.push_back( make_pair( wp, ld ) );
    }
  } // �஡�� �� ���ᠦ�ࠬ
}

/*
1. �� ������ �᫨ ���ᠦ�� ����� ᯥ�. �������� (६�ન MCLS) - ���� �롨ࠥ� ⮫쪮 ���� � ६�ઠ�� �㦭��� ��������.
�� ������ , �᫨ ᠫ�� �� ࠧ��祭?
2. ���� ��㯯� ���ᠦ�஢, ������� 㦥 ��ॣ����஢���, ������� ���.
*/
void WebRequestsIface::ViewCraft(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

  ProgTrace(TRACE1,"WebRequestsIface::ViewCraft");
  int point_id = NodeAsInteger( "point_id", reqNode );
  int pnr_id = NodeAsInteger( "pnr_id", reqNode );
  vector<TWebPax> pnr;
  TSearchPnrData SearchPnrData;
  getTripData( point_id, false, SearchPnrData, true );
  VerifyPNR( point_id, pnr_id );
  getPnr( point_id, pnr_id, pnr, true, false );

  map<int, TWebPlaceList> web_salons;
  bool pr_find_free_subcls_place=false;
  ReadWebSalons( point_id, pnr, web_salons, pr_find_free_subcls_place );

  xmlNodePtr node = NewTextChild( resNode, "ViewCraft" );
  node = NewTextChild( node, "salons" );
  for( map<int, TWebPlaceList>::iterator isal=web_salons.begin(); isal!=web_salons.end(); isal++ ) {
    xmlNodePtr placeListNode = NewTextChild( node, "placelist" );
    SetProp( placeListNode, "num", isal->first );
    SetProp( placeListNode, "xcount", isal->second.xcount + 1 );
    SetProp( placeListNode, "ycount", isal->second.ycount + 1 );
    for ( TWebPlaces::iterator wp = isal->second.places.begin();
          wp != isal->second.places.end(); wp++ ) {
      xmlNodePtr placeNode = NewTextChild( placeListNode, "place" );
      NewTextChild( placeNode, "x", wp->x );
      NewTextChild( placeNode, "y", wp->y );
      NewTextChild( placeNode, "seat_no", wp->seat_no );
      NewTextChild( placeNode, "elem_type", wp->elem_type );
      NewTextChild( placeNode, "status", get_seat_status( *wp, pr_find_free_subcls_place ) );
      if ( wp->pax_id != NoExists )
      	NewTextChild( placeNode, "pax_id", wp->pax_id );
      if ( wp->WebTariff.value != 0.0 ) { // �᫨ ���⭠� ॣ������ �⪫�祭�, value=0.0 � �� ��砥
      	xmlNodePtr rateNode = NewTextChild( placeNode, "rate" );
      	NewTextChild( rateNode, "color", wp->WebTariff.color );
      	ostringstream buf;
        buf << std::fixed << setprecision(2) << wp->WebTariff.value;
      	NewTextChild( rateNode, "value", buf.str() );
      	NewTextChild( rateNode, "currency", wp->WebTariff.currency_id );
      }
    }
  }
}

void CreateEmulXMLDoc(xmlNodePtr reqNode, XMLDoc &emulDoc)
{
  emulDoc.set("UTF-8","term");
  if (emulDoc.docPtr()==NULL)
    throw EXCEPTIONS::Exception("CreateEmulXMLDoc: CreateXMLDoc failed");
  CopyNode(NodeAsNode("/term",emulDoc.docPtr()),
           NodeAsNode("/term/query",reqNode->doc), true); //�����㥬 ��������� ⥣ query
  xmlNodePtr node=NodeAsNode("/term/query",emulDoc.docPtr())->children;
  if (node!=NULL)
  {
    xmlUnlinkNode(node);
    xmlFreeNode(node);
  };
};

void CopyEmulXMLDoc(XMLDoc &srcDoc, XMLDoc &destDoc)
{
  destDoc.set("UTF-8","term");
  if (destDoc.docPtr()==NULL)
    throw EXCEPTIONS::Exception("CopyEmulXMLDoc: CreateXMLDoc failed");
  xmlNodePtr destNode=NodeAsNode("/term",destDoc.docPtr());
  xmlNodePtr srcNode=NodeAsNode("/term",srcDoc.docPtr())->children;
  for(;srcNode!=NULL;srcNode=srcNode->next)
    CopyNode(destNode, srcNode, true); //�����㥬 ��������� XML
};

void CreateEmulRems(xmlNodePtr paxNode, TQuery &RemQry, const vector<string> &fqtv_rems)
{
  xmlNodePtr remsNode=NewTextChild(paxNode,"rems");
  for(;!RemQry.Eof;RemQry.Next())
  {
    const char* rem_code=RemQry.FieldAsString("rem_code");
    const char* rem_text=RemQry.FieldAsString("rem");
    if (isDisabledRem(rem_code, rem_text)) continue;
    if (strcmp(rem_code,"FQTV")==0) continue;
    xmlNodePtr remNode=NewTextChild(remsNode,"rem");
    NewTextChild(remNode,"rem_code",rem_code);
    NewTextChild(remNode,"rem_text",rem_text);
  };
  //������� ��।���� fqtv_rems
  for(vector<string>::const_iterator r=fqtv_rems.begin();r!=fqtv_rems.end();r++)
  {
    xmlNodePtr remNode=NewTextChild(remsNode,"rem");
    NewTextChild(remNode,"rem_code","FQTV");
    NewTextChild(remNode,"rem_text",*r);
  };
};

struct TWebPaxFromReq
{
  int crs_pax_id;
  string seat_no;
  vector<string> fqt_rems;
  bool fqt_rems_present;
  bool refuse;
  int crs_pnr_tid;
	int crs_pax_tid;
	int pax_grp_tid;
	int pax_tid;
  TWebPaxFromReq() {
		crs_pax_id = NoExists;
		fqt_rems_present = false;
		refuse = false;
		crs_pnr_tid = NoExists;
		crs_pax_tid	= NoExists;
		pax_grp_tid = NoExists;
		pax_tid = NoExists;
	};
};

struct TWebPaxForChng
{
  int crs_pax_id;
  int grp_id;
  int point_dep;
  int point_arv;
  string airp_dep;
  string airp_arv;
  string cl;
  int excess;
  bool bag_refuse;
  
  string surname;
  string name;
  string seat_no;
  int seats;
};

struct TWebPaxForCkin
{
  int crs_pax_id;
  
  string surname;
  string name;
  string pers_type;
  string seat_no;
  string seat_type;
  int seats;
  string eticket;
  string ticket;
  CheckIn::TPaxDocItem document;
  string subclass;
  
  bool operator == (const TWebPaxForCkin &pax) const
	{
  	return transliter_equal(surname,pax.surname) &&
           transliter_equal(name,pax.name) &&
           pers_type==pax.pers_type &&
           (seats==0 && pax.seats==0 || seats!=0 && pax.seats!=0);
  };
};

struct TWebPnrForSave
{
  int pnr_id;
  vector<TWebPaxFromReq> paxFromReq;
  unsigned int refusalCountFromReq;
  list<TWebPaxForChng> paxForChng;
  list<TWebPaxForCkin> paxForCkin;

  TWebPnrForSave() {
    pnr_id = NoExists;
    refusalCountFromReq = 0;
  };
};



void VerifyPax(vector< pair<int, TWebPnrForSave > > &segs, XMLDoc &emulDocHeader,
               XMLDoc &emulCkinDoc, map<int,XMLDoc> &emulChngDocs, vector<TIdsPnrData> &ids)
{
  ids.clear();
  
  if (segs.empty()) return;
  
  TReqInfo *reqInfo = TReqInfo::Instance();

  //���� ����� �஢��塞, �� ����ॣ����஢���� ���ᠦ��� ᮢ������ �� ���-�� ��� ������� ᥣ����
  //�� ��᫥���� ᥣ����� ���-�� ����ॣ����஢����� ���ᠦ�஢ �.�. �㫥��
  int prevNotCheckedCount=NoExists;
  int seg_no=1;
  for(vector< pair<int, TWebPnrForSave > >::const_iterator s=segs.begin(); s!=segs.end(); s++, seg_no++)
  {
    int currNotCheckedCount=0;
    for(vector<TWebPaxFromReq>::const_iterator iPax=s->second.paxFromReq.begin(); iPax!=s->second.paxFromReq.end(); iPax++)
    {
      if (iPax->pax_grp_tid==NoExists || iPax->pax_tid==NoExists)
        //���ᠦ�� �� ��ॣ����஢��
        currNotCheckedCount++;
    };

    if (prevNotCheckedCount!=NoExists && currNotCheckedCount!=0)
    {
      if (prevNotCheckedCount!=currNotCheckedCount)
        throw EXCEPTIONS::Exception("VerifyPax: different number of passengers for through check-in (seg_no=%d)", seg_no);
    };
      
    prevNotCheckedCount=currNotCheckedCount;
  };
  
  TSearchPnrData firstPnrData;
	getTripData( segs.begin()->first, true, firstPnrData, true );
	
  const char* PaxQrySQL=
	    "SELECT point_dep,point_arv,airp_dep,airp_arv,class,excess,bag_refuse, "
	    "       pax_grp.grp_id,pax.surname,pax.name,pax.seats, "
	    "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'one',rownum) AS seat_no, "
	    "       pax_grp.tid AS pax_grp_tid, "
	    "       pax.tid AS pax_tid, "
	    "       crs_pax.pnr_id, crs_pax.pr_del "
	    "FROM pax_grp,pax,crs_pax "
	    "WHERE pax_grp.grp_id=pax.grp_id AND "
	    "      pax.pax_id=crs_pax.pax_id(+) AND "
	    "      pax.pax_id=:pax_id";

  const char* CrsPaxQrySQL=
      "SELECT tlg_trips.airline,tlg_trips.flt_no,tlg_trips.suffix, "
      "       tlg_trips.scd AS scd_out,tlg_trips.airp_dep AS airp, "
      "       crs_pnr.point_id,crs_pnr.subclass, "
      "       crs_pnr.status AS pnr_status, "
      "       crs_pax.surname,crs_pax.name,crs_pax.pers_type, "
      "       salons.get_crs_seat_no(crs_pax.pax_id,crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, "
      "       crs_pax.seat_type, "
      "       crs_pax.seats, "
      "       crs_pnr.pnr_id, "
      "       report.get_TKNO(crs_pax.pax_id,'/',0) AS ticket, "
      "       report.get_TKNO(crs_pax.pax_id,'/',1) AS eticket, "
      "       crs_pnr.tid AS crs_pnr_tid, "
      "       crs_pax.tid AS crs_pax_tid, "
      "       DECODE(pax.pax_id,NULL,0,1) AS checked "
      "FROM tlg_trips,crs_pnr,crs_pax,pax "
      "WHERE tlg_trips.point_id=crs_pnr.point_id AND "
      "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      crs_pax.pax_id=:crs_pax_id AND "
      "      crs_pax.pr_del=0 ";
      
  const char* TestPaxQrySQL=
      "SELECT subclass, NULL AS pnr_status, "
      "       surname, NULL AS name, :adult AS pers_type, "
      "       NULL AS seat_no, NULL AS seat_type, 1 AS seats, "
      "       id AS pnr_id, "
      "       NULL AS ticket, tkn_no AS eticket, doc_no, "
      "       id AS crs_pnr_tid, id AS crs_pax_tid, 0 AS checked "
      "FROM test_pax "
      "WHERE id=:crs_pax_id";

  const char* PaxRemQrySQL=
      "SELECT rem_code,rem FROM pax_rem "
      "WHERE pax_id=:pax_id AND rem_code NOT IN ('FQTV')";

  const char* CrsPaxRemQrySQL=
      "SELECT rem_code,rem FROM crs_pax_rem "
      "WHERE pax_id=:pax_id AND rem_code NOT IN ('FQTV')";
  TQuery Qry(&OraSession);

  TQuery RemQry(&OraSession);
  RemQry.DeclareVariable("pax_id",otInteger);
  
  TQuery PaxDocQry(&OraSession);
  TQuery GetPSPT2Qry(&OraSession);
  TQuery PaxStatusQry(&OraSession);
  
  seg_no=1;
  for(vector< pair<int, TWebPnrForSave > >::iterator s=segs.begin(); s!=segs.end(); s++, seg_no++)
  {
    s->second.paxForChng.clear();
    s->second.paxForCkin.clear();
    int point_id=s->first;
    try
    {
      if (!s->second.paxFromReq.empty())
      {
        int pnr_id=NoExists; //���஡㥬 ��।����� �� ᥪ樨 passengers
        int adult_count=0, without_seat_count=0;
        for(vector<TWebPaxFromReq>::const_iterator iPax=s->second.paxFromReq.begin(); iPax!=s->second.paxFromReq.end(); iPax++)
        {
          try
          {
            bool not_checked=isTestPaxId(iPax->crs_pax_id) ||
                             iPax->pax_grp_tid==NoExists || iPax->pax_tid==NoExists;

            Qry.Clear();
            try
            {
              if (not_checked)
              {
                //���ᠦ�� �� ��ॣ����஢��
                if (isTestPaxId(iPax->crs_pax_id))
                {
                  Qry.SQLText=TestPaxQrySQL;
                  Qry.CreateVariable("adult", otString, EncodePerson(adult));
                }
                else
                  Qry.SQLText=CrsPaxQrySQL;
                Qry.CreateVariable("crs_pax_id", otInteger, iPax->crs_pax_id);
                Qry.Execute();
                if (Qry.Eof)
                  throw UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
                if (Qry.FieldAsInteger("checked")!=0)
                  throw UserException("MSG.PASSENGER.CHECKED.REFRESH_DATA");
                if (iPax->crs_pnr_tid!=Qry.FieldAsInteger("crs_pnr_tid") ||
                    iPax->crs_pax_tid!=Qry.FieldAsInteger("crs_pax_tid"))
                  throw UserException("MSG.PASSENGER.CHANGED.REFRESH_DATA");

                if (!is_valid_pnr_status(Qry.FieldAsString("pnr_status")) ||
                    !is_valid_pax_status(iPax->crs_pax_id, PaxStatusQry))
                  throw UserException("MSG.PASSENGER.CHECKIN_DENIAL");
              }
              else
              {
                //���ᠦ�� ��ॣ����஢��
                Qry.SQLText=PaxQrySQL;
                Qry.CreateVariable("pax_id", otInteger, iPax->crs_pax_id);
                Qry.Execute();
                if (Qry.Eof)
                  throw UserException("MSG.PASSENGER.CHANGED.REFRESH_DATA");
                if (Qry.FieldIsNULL("pnr_id") || Qry.FieldAsInteger("pr_del")!=0)
                  throw UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
                if (iPax->pax_grp_tid!=Qry.FieldAsInteger("pax_grp_tid") ||
                    iPax->pax_tid!=Qry.FieldAsInteger("pax_tid"))
                  throw UserException("MSG.PASSENGER.CHANGED.REFRESH_DATA");
              };
            }
            catch(UserException &E)
            {
              ProgTrace(TRACE5, ">>>> %s (seg_no=%d, crs_pax_id=%d)",
                                getLocaleText(E.getLexemaData()).c_str(), seg_no, iPax->crs_pax_id);
              throw;
            };

            if (pnr_id==ASTRA::NoExists)
            {
              //���� ���ᠦ��
              pnr_id=Qry.FieldAsInteger("pnr_id");
              //�஢�ਬ, �� ������ PNR �ਢ易�� � ३��
              VerifyPNR(point_id,pnr_id);
            }
            else
            {
              if (Qry.FieldAsInteger("pnr_id")!=pnr_id)
                throw EXCEPTIONS::Exception("VerifyPax: passengers from different PNR (seg_no=%d)", seg_no);
            };

            if (!not_checked)
            {
              TWebPaxForChng pax;
              pax.crs_pax_id = iPax->crs_pax_id;
              pax.grp_id = Qry.FieldAsInteger("grp_id");
              pax.point_dep = Qry.FieldAsInteger("point_dep");
              pax.point_arv = Qry.FieldAsInteger("point_arv");
              pax.airp_dep = Qry.FieldAsString("airp_dep");
              pax.airp_arv = Qry.FieldAsString("airp_arv");
              pax.cl = Qry.FieldAsString("class");
              pax.excess = Qry.FieldAsInteger("excess");
              pax.bag_refuse = Qry.FieldAsInteger("bag_refuse")!=0;
              pax.surname = Qry.FieldAsString("surname");
              pax.name = Qry.FieldAsString("name");
              pax.seat_no = Qry.FieldAsString("seat_no");
              pax.seats = Qry.FieldAsInteger("seats");
              s->second.paxForChng.push_back(pax);
            }
            else
            {
              TWebPaxForCkin pax;
              pax.crs_pax_id = iPax->crs_pax_id;
              pax.surname = Qry.FieldAsString("surname");
              pax.name = Qry.FieldAsString("name");
              pax.pers_type = Qry.FieldAsString("pers_type");
              pax.seat_no = Qry.FieldAsString("seat_no");
              pax.seat_type = Qry.FieldAsString("seat_type");
              pax.seats = Qry.FieldAsInteger("seats");
              pax.eticket = Qry.FieldAsString("eticket");
              pax.ticket = Qry.FieldAsString("ticket");
              //��ࠡ�⪠ ���㬥�⮢
              if (isTestPaxId(iPax->crs_pax_id))
              {
                pax.document.clear();
                pax.document.no = Qry.FieldAsString("doc_no");
              }
              else
              {
                LoadCrsPaxDoc(pax.crs_pax_id, pax.document, PaxDocQry, GetPSPT2Qry);
              };
    
              pax.subclass = Qry.FieldAsString("subclass");

              TPerson p=DecodePerson(pax.pers_type.c_str());
              if (p==ASTRA::adult) adult_count++;
              if (p==ASTRA::baby && pax.seats==0) without_seat_count++;

              s->second.paxForCkin.push_back(pax);
            };
          }
          catch(CheckIn::UserException)
          {
            throw;
          }
          catch(UserException &e)
          {
            throw CheckIn::UserException(e.getLexemaData(), point_id, iPax->crs_pax_id);
          };

        };
        if (without_seat_count>adult_count)
          throw UserException("MSG.CHECKIN.BABY_WO_SEATS_MORE_ADULT_FOR_GRP");
        if (pnr_id==NoExists)
          throw EXCEPTIONS::Exception("VerifyPax: unknown pnr_id (seg_no=%d)", seg_no);
        s->second.pnr_id=pnr_id;
      }
      else
      {
        //�஢�ਬ ���� ᮮ⢥��⢨� point_id � pnr_id
        VerifyPNR(point_id,s->second.pnr_id);
      };
    }
    catch(CheckIn::UserException)
    {
      throw;
    }
    catch(UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), point_id);
    };
  };
  
  vector<TSearchPnrData> PNRs;
  //� �� ��砥 ����뢠�� ᪢����� �������
  try
  {
    //���������� ���� firstPnrData
    firstPnrData.is_test = isTestPaxId(segs.begin()->second.pnr_id);
  	firstPnrData.pnr_id = segs.begin()->second.pnr_id;

    if (!firstPnrData.is_test)
    {
      Qry.Clear();
      Qry.SQLText=
        "SELECT airp_arv, subclass, class "
        "FROM crs_pnr, crs_pax "
        "WHERE crs_pax.pnr_id=crs_pnr.pnr_id AND "
        "      crs_pnr.pnr_id=:pnr_id AND "
        "      crs_pax.pr_del=0 AND rownum<2";
      Qry.CreateVariable("pnr_id", otInteger, firstPnrData.pnr_id);
      Qry.Execute();
      if (Qry.Eof)
        throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );

      firstPnrData.airp_arv = Qry.FieldAsString("airp_arv");
    	firstPnrData.cls = Qry.FieldAsString("class");
    	firstPnrData.subcls = Qry.FieldAsString("subclass");

      TTripRoute route; //������� ३�
      route.GetRouteAfter( NoExists,
                           firstPnrData.point_id,
                           firstPnrData.point_num,
                           firstPnrData.first_point,
                           firstPnrData.pr_tranzit,
                           trtNotCurrent,
                           trtNotCancelled );

      TTripRoute::iterator i=route.begin();
      for ( ; i!=route.end(); i++ )
        if (i->airp == firstPnrData.airp_arv) break;
      if (i==route.end())
        throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
      firstPnrData.point_arv = i->point_id;

      Qry.Clear();
      Qry.SQLText=
        "SELECT pnr_id,airline,addr "
        "FROM pnr_addrs "
        "WHERE pnr_id=:pnr_id";
      Qry.CreateVariable("pnr_id", otInteger, firstPnrData.pnr_id);
      Qry.Execute();

      for ( ; !Qry.Eof; Qry.Next() )
    	{
    		TPnrAddr addr(Qry.FieldAsString( "airline" ),
                      Qry.FieldAsString( "addr" ));
    		firstPnrData.pnr_addrs.push_back( addr );
    	};
    }
    else
    {
      TTripRouteItem next;
      TTripRoute().GetNextAirp(NoExists,
                               firstPnrData.point_id,
                               firstPnrData.point_num,
                               firstPnrData.first_point,
                               firstPnrData.pr_tranzit,
                               trtNotCancelled,
                               next);
      if (next.point_id==NoExists || next.airp.empty())
        throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
        
      firstPnrData.point_arv = next.point_id;
      firstPnrData.airp_arv = next.airp;

      TQuery Qry(&OraSession);
      Qry.Clear();
    	Qry.SQLText=
    	  "SELECT subcls.class, subclass, "
    	  "       pnr_airline, pnr_addr "
    	  "FROM test_pax, subcls "
    	  "WHERE test_pax.subclass=subcls.code AND test_pax.id=:pnr_id";
    	Qry.CreateVariable("pnr_id", otInteger, firstPnrData.pnr_id);
    	Qry.Execute();
    	if (Qry.Eof)
        throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );

    	firstPnrData.cls = Qry.FieldAsString("class");
    	firstPnrData.subcls = Qry.FieldAsString("subclass");
    	
    	TPnrAddr addr(Qry.FieldAsString( "pnr_airline" ),
                    Qry.FieldAsString( "pnr_addr" ));
    	firstPnrData.pnr_addrs.push_back( addr );
    };

    getTripData2(firstPnrData, true);
  }
  catch(CheckIn::UserException)
  {
    throw;
  }
  catch(UserException &e)
  {
    throw CheckIn::UserException(e.getLexemaData(), firstPnrData.point_id);
  };

  getTCkinData( firstPnrData, PNRs);

    
  const TWebPnrForSave &firstPnr=segs.begin()->second;
  //�஢�ઠ �ᥣ� ᪢������ ������� �� ᮢ������� point_id, pnr_id � ᮮ⢥��⢨� 䠬����/����
  vector<TSearchPnrData>::const_iterator iPnrData=PNRs.begin();
  seg_no=1;
  for(vector< pair<int, TWebPnrForSave > >::iterator s=segs.begin(); s!=segs.end(); s++, iPnrData++, seg_no++)
  {
    try
    {
      if (iPnrData!=PNRs.end() &&
          (!s->second.paxForCkin.empty() ||
           !s->second.paxForChng.empty() && s->second.paxForChng.size()>s->second.refusalCountFromReq)) //⨯� ���� ���ᠦ���
      {
        //�஢��塞 �� ᥣ���� �뫥� ३� � ���ﭨ� ᮮ⢥�����饣� �⠯�
        if ( iPnrData->act_out != NoExists )
  	      throw UserException( "MSG.FLIGHT.TAKEOFF" );

  	    if ( reqInfo->client_type == ctKiosk )
        {
          if (!(iPnrData->kiosk_checkin_stage == sOpenKIOSKCheckIn ||
                iPnrData->kiosk_checkin_stage == sNoActive && s!=segs.begin())) //��� ᪢����� ᥣ���⮢ ॣ������ ����� ���� �� �� �����
          {
            if (iPnrData->kiosk_checkin_stage == sNoActive)
              throw UserException( "MSG.CHECKIN.NOT_OPEN" );
            else
              throw UserException( "MSG.CHECKIN.CLOSED_OR_DENIAL" );
          };
        }
        else
        {
          if (!(iPnrData->web_checkin_stage == sOpenWEBCheckIn ||
                iPnrData->web_checkin_stage == sNoActive && s!=segs.begin())) //��� ᪢����� ᥣ���⮢ ॣ������ ����� ���� �� �� �����
            if (iPnrData->web_checkin_stage == sNoActive)
              throw UserException( "MSG.CHECKIN.NOT_OPEN" );
            else
              throw UserException( "MSG.CHECKIN.CLOSED_OR_DENIAL" );
        };
      };
      
      if (iPnrData!=PNRs.end() && s->second.refusalCountFromReq>0)
      {
        if ( reqInfo->client_type != ctKiosk )
        {
          if (!(iPnrData->web_cancel_stage == sOpenWEBCheckIn ||
                iPnrData->web_cancel_stage == sNoActive && s!=segs.begin())) //��� ᪢����� ᥣ���⮢ ॣ������ ����� ���� �� �� �����
            throw UserException("MSG.PASSENGER.UNREGISTRATION_DENIAL");
        };
      };

      if (s==segs.begin()) continue; //�ய�᪠�� ���� ᥣ����

      TWebPnrForSave &currPnr=s->second;
      if (iPnrData==PNRs.end()) //��譨� ᥣ����� � ����� �� ॣ������
        throw UserException("MSG.THROUGH_ROUTE_CHANGED.REFRESH_DATA");
      if (iPnrData->point_id!=s->first) //��㣮� ३� �� ᪢����� ᥣ����
        throw UserException("MSG.THROUGH_ROUTE_CHANGED.REFRESH_DATA");
      if (iPnrData->pnr_id!=currPnr.pnr_id) //��㣮� pnr_id �� ᪢����� ᥣ����
        throw UserException("MSG.THROUGH_ROUTE_CHANGED.REFRESH_DATA");

      if (!currPnr.paxForCkin.empty() && firstPnr.paxForCkin.size()!=currPnr.paxForCkin.size())
        throw EXCEPTIONS::Exception("VerifyPax: different number of passengers for through check-in (seg_no=%d)", seg_no);

      if (!currPnr.paxForCkin.empty())
      {
        list<TWebPaxForCkin>::const_iterator iPax=firstPnr.paxForCkin.begin();
        for(;iPax!=firstPnr.paxForCkin.end();iPax++)
        {
          list<TWebPaxForCkin>::iterator iPax2=find(currPnr.paxForCkin.begin(),currPnr.paxForCkin.end(),*iPax);
          if (iPax2==currPnr.paxForCkin.end())
            throw EXCEPTIONS::Exception("VerifyPax: passenger not found (seg_no=%d, surname=%s, name=%s, pers_type=%s, seats=%d)",
                                        seg_no, iPax->surname.c_str(), iPax->name.c_str(), iPax->pers_type.c_str(), iPax->seats);

          list<TWebPaxForCkin>::iterator iPax3=iPax2;
          if (find(++iPax3,currPnr.paxForCkin.end(),*iPax)!=currPnr.paxForCkin.end())
            throw EXCEPTIONS::Exception("VerifyPax: passengers are duplicated (seg_no=%d, surname=%s, name=%s, pers_type=%s, seats=%d)",
                                        seg_no, iPax->surname.c_str(), iPax->name.c_str(), iPax->pers_type.c_str(), iPax->seats);

          currPnr.paxForCkin.splice(currPnr.paxForCkin.end(),currPnr.paxForCkin,iPax2,iPax3); //��६�頥� ���������� ���ᠦ�� � �����
        };
      };
    }
    catch(CheckIn::UserException)
    {
      throw;
    }
    catch(UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), s->first);
    };
  };

  if (!firstPnrData.is_test)
  {
    //��⠢�塞 XML-�����
    iPnrData=PNRs.begin();
    seg_no=1;
    for(vector< pair<int, TWebPnrForSave > >::const_iterator s=segs.begin(); s!=segs.end(); s++, iPnrData++, seg_no++)
    {
      try
      {
        if (iPnrData==PNRs.end()) //��譨� ᥣ����� � ����� �� ॣ������
          throw EXCEPTIONS::Exception("VerifyPax: iPnrData==PNRs.end() (seg_no=%d)", seg_no);

        const TWebPnrForSave &currPnr=s->second;
        //���ᠦ��� ��� ॣ����樨
        if (!currPnr.paxForCkin.empty())
        {
          if (emulCkinDoc.docPtr()==NULL)
          {
            CopyEmulXMLDoc(emulDocHeader, emulCkinDoc);
            xmlNodePtr emulCkinNode=NodeAsNode("/term/query",emulCkinDoc.docPtr());
            emulCkinNode=NewTextChild(emulCkinNode,"TCkinSavePax");
          	NewTextChild(emulCkinNode,"transfer"); //���⮩ ⥣ - �࠭��� ���
            NewTextChild(emulCkinNode,"segments");
            NewTextChild(emulCkinNode,"excess",(int)0);
            NewTextChild(emulCkinNode,"hall");
          };
          xmlNodePtr segsNode=NodeAsNode("/term/query/TCkinSavePax/segments",emulCkinDoc.docPtr());

          xmlNodePtr segNode=NewTextChild(segsNode, "segment");
          NewTextChild(segNode,"point_dep",iPnrData->point_id);
          NewTextChild(segNode,"point_arv",iPnrData->point_arv);
          NewTextChild(segNode,"airp_dep",iPnrData->airp_dep);
          NewTextChild(segNode,"airp_arv",iPnrData->airp_arv);
          NewTextChild(segNode,"class",iPnrData->cls);
          NewTextChild(segNode,"status",EncodePaxStatus(psCheckin));
          NewTextChild(segNode,"wl_type");

          TTripInfo operFlt,pnrMarkFlt;
          TCodeShareSets codeshareSets;
          GetPNRCodeshare(*iPnrData, operFlt, pnrMarkFlt, codeshareSets);

          xmlNodePtr node=NewTextChild(segNode,"mark_flight");
          NewTextChild(node,"airline",pnrMarkFlt.airline);
          NewTextChild(node,"flt_no",pnrMarkFlt.flt_no);
          NewTextChild(node,"suffix",pnrMarkFlt.suffix);
          NewTextChild(node,"scd",DateTimeToStr(pnrMarkFlt.scd_out));  //�����쭠� ���
          NewTextChild(node,"airp_dep",pnrMarkFlt.airp);
          NewTextChild(node,"pr_mark_norms",(int)codeshareSets.pr_mark_norms);

          xmlNodePtr paxsNode=NewTextChild(segNode,"passengers");
          for(list<TWebPaxForCkin>::const_iterator iPaxForCkin=currPnr.paxForCkin.begin();iPaxForCkin!=currPnr.paxForCkin.end();iPaxForCkin++)
          {
            try
            {
              vector<TWebPaxFromReq>::const_iterator iPaxFromReq=currPnr.paxFromReq.begin();
              for(;iPaxFromReq!=currPnr.paxFromReq.end();iPaxFromReq++)
                if (iPaxFromReq->crs_pax_id==iPaxForCkin->crs_pax_id) break;
              if (iPaxFromReq==currPnr.paxFromReq.end())
                throw EXCEPTIONS::Exception("VerifyPax: iPaxFromReq==currPnr.paxFromReq.end() (seg_no=%d, crs_pax_id=%d)", seg_no, iPaxForCkin->crs_pax_id);

              xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
              NewTextChild(paxNode,"pax_id",iPaxForCkin->crs_pax_id);
              NewTextChild(paxNode,"surname",iPaxForCkin->surname);
              NewTextChild(paxNode,"name",iPaxForCkin->name);
              NewTextChild(paxNode,"pers_type",iPaxForCkin->pers_type);
              if (!iPaxFromReq->seat_no.empty())
                NewTextChild(paxNode,"seat_no",iPaxFromReq->seat_no);
              else
                NewTextChild(paxNode,"seat_no",iPaxForCkin->seat_no);
              NewTextChild(paxNode,"seat_type",iPaxForCkin->seat_type);
              NewTextChild(paxNode,"seats",iPaxForCkin->seats);
              //��ࠡ�⪠ ����⮢
              string ticket_no;
              if (!iPaxForCkin->eticket.empty())
              {
                //����� TKNE
                ticket_no=iPaxForCkin->eticket;

                int coupon_no=0;
                string::size_type pos=ticket_no.find_last_of('/');
                if (pos!=string::npos)
                {
                  if (StrToInt(ticket_no.substr(pos+1).c_str(),coupon_no)!=EOF &&
                      coupon_no>=1 && coupon_no<=4)
                    ticket_no.erase(pos);
                  else
                    coupon_no=0;
                };

                if (ticket_no.empty())
                  throw UserException("MSG.ETICK.NUMBER_NOT_SET");
                NewTextChild(paxNode,"ticket_no",ticket_no);
                if (coupon_no<=0)
                  throw UserException("MSG.ETICK.COUPON_NOT_SET", LParams()<<LParam("etick", ticket_no ) );
                NewTextChild(paxNode,"coupon_no",coupon_no);
                NewTextChild(paxNode,"ticket_rem","TKNE");
                NewTextChild(paxNode,"ticket_confirm",(int)false);
              }
              else
              {
                ticket_no=iPaxForCkin->ticket;

                NewTextChild(paxNode,"ticket_no",ticket_no);
                NewTextChild(paxNode,"coupon_no");
                if (!ticket_no.empty())
                  NewTextChild(paxNode,"ticket_rem","TKNA");
                else
                  NewTextChild(paxNode,"ticket_rem");
                NewTextChild(paxNode,"ticket_confirm",(int)false);
              };
              iPaxForCkin->document.toXML(paxNode);

              NewTextChild(paxNode,"subclass",iPaxForCkin->subclass);
              NewTextChild(paxNode,"transfer"); //���⮩ ⥣ - �࠭��� ���
              NewTextChild(paxNode,"bag_pool_num");

              //६�ન
              RemQry.SQLText=CrsPaxRemQrySQL;
              RemQry.SetVariable("pax_id",iPaxForCkin->crs_pax_id);
              RemQry.Execute();
              CreateEmulRems(paxNode, RemQry, iPaxFromReq->fqt_rems);

              NewTextChild(paxNode,"norms"); //���⮩ ⥣ - ��� ���
            }
            catch(CheckIn::UserException)
            {
              throw;
            }
            catch(UserException &e)
            {
              throw CheckIn::UserException(e.getLexemaData(), s->first, iPaxForCkin->crs_pax_id);
            };
          };
        };

        //���ᠦ��� ��� ���������
        for(list<TWebPaxForChng>::const_iterator iPaxForChng=currPnr.paxForChng.begin();iPaxForChng!=currPnr.paxForChng.end();iPaxForChng++)
        {
          try
          {
            vector<TWebPaxFromReq>::const_iterator iPaxFromReq=currPnr.paxFromReq.begin();
            for(;iPaxFromReq!=currPnr.paxFromReq.end();iPaxFromReq++)
              if (iPaxFromReq->crs_pax_id==iPaxForChng->crs_pax_id) break;
            if (iPaxFromReq==currPnr.paxFromReq.end())
              throw EXCEPTIONS::Exception("VerifyPax: iPaxFromReq==currPnr.paxFromReq.end() (seg_no=%d, crs_pax_id=%d)", seg_no, iPaxForChng->crs_pax_id);

            int pax_tid=iPaxFromReq->pax_tid;
            //���ᠦ�� ��ॣ����஢��
            if (!iPaxFromReq->seat_no.empty() && iPaxForChng->seats > 0)
            {
            	string prior_xname, prior_yname;
            	string curr_xname, curr_yname;
            	// ���� ������������ ��஥ � ����� ����, �ࠢ���� ��, �᫨ ��������, � �맢��� ���ᠤ��
            	getXYName( iPnrData->point_id, iPaxForChng->seat_no, prior_xname, prior_yname );
            	getXYName( iPnrData->point_id, iPaxFromReq->seat_no, curr_xname, curr_yname );
            	if ( curr_xname.empty() && curr_yname.empty() )
            		throw UserException( "MSG.SEATS.SEAT_NO.NOT_FOUND" );
            	if ( prior_xname + prior_yname != curr_xname + curr_yname ) {
                IntChangeSeats( iPnrData->point_id,
                                iPaxForChng->crs_pax_id,
                                pax_tid,
                                curr_xname, curr_yname,
      	                        SEATS2::stReseat,
      	                        cltUnknown,
                                false, false,
                                NULL );
            	}
            };
            bool FQTRemUpdatesPending;
            if (iPaxFromReq->fqt_rems_present) //⥣ <fqt_rems> ��襫
            {
              vector<string> prior_fqt_rems;
              //�⠥� 㦥 ����ᠭ�� ६�ન FQTV
              RemQry.SQLText="SELECT rem FROM pax_rem WHERE pax_id=:pax_id AND rem_code='FQTV'";
              RemQry.SetVariable("pax_id", iPaxForChng->crs_pax_id);
              RemQry.Execute();
              for(;!RemQry.Eof;RemQry.Next()) prior_fqt_rems.push_back(RemQry.FieldAsString("rem"));
              //����㥬 � �ࠢ������
              sort(prior_fqt_rems.begin(),prior_fqt_rems.end());
              if (prior_fqt_rems.size()==iPaxFromReq->fqt_rems.size())
                FQTRemUpdatesPending=!equal(prior_fqt_rems.begin(),prior_fqt_rems.end(),
                                            iPaxFromReq->fqt_rems.begin());
              else
                FQTRemUpdatesPending=true;
            };
            if (iPaxFromReq->refuse || FQTRemUpdatesPending)
            {
              //�ਤ���� �맢��� �࠭����� �� ������ ���������
              XMLDoc &emulChngDoc=emulChngDocs[iPaxForChng->grp_id];
              if (emulChngDoc.docPtr()==NULL)
              {
                CopyEmulXMLDoc(emulDocHeader, emulChngDoc);

                xmlNodePtr emulChngNode=NodeAsNode("/term/query",emulChngDoc.docPtr());
                emulChngNode=NewTextChild(emulChngNode,"TCkinSavePax");

                xmlNodePtr segNode=NewTextChild(NewTextChild(emulChngNode,"segments"),"segment");
                NewTextChild(segNode,"point_dep",iPaxForChng->point_dep);
                NewTextChild(segNode,"point_arv",iPaxForChng->point_arv);
                NewTextChild(segNode,"airp_dep",iPaxForChng->airp_dep);
                NewTextChild(segNode,"airp_arv",iPaxForChng->airp_arv);
                NewTextChild(segNode,"class",iPaxForChng->cl);
                NewTextChild(segNode,"grp_id",iPaxForChng->grp_id);
                NewTextChild(segNode,"tid",iPaxFromReq->pax_grp_tid);
                NewTextChild(segNode,"passengers");

                NewTextChild(emulChngNode,"excess",iPaxForChng->excess);
                NewTextChild(emulChngNode,"hall");
                if (iPaxForChng->bag_refuse)
                  NewTextChild(emulChngNode,"bag_refuse",refuseAgentError);
                else
                  NewTextChild(emulChngNode,"bag_refuse");
              };
              xmlNodePtr paxsNode=NodeAsNode("/term/query/TCkinSavePax/segments/segment/passengers",emulChngDoc.docPtr());

              xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
              NewTextChild(paxNode,"pax_id",iPaxForChng->crs_pax_id);
              NewTextChild(paxNode,"surname",iPaxForChng->surname);
              NewTextChild(paxNode,"name",iPaxForChng->name);
              if (iPaxFromReq->refuse)
                NewTextChild(paxNode,"refuse",refuseAgentError);
              NewTextChild(paxNode,"tid",pax_tid);

              if (FQTRemUpdatesPending)
              {
                //६�ન
                RemQry.SQLText=PaxRemQrySQL;
                RemQry.SetVariable("pax_id",iPaxForChng->crs_pax_id);
                RemQry.Execute();
                CreateEmulRems(paxNode, RemQry, iPaxFromReq->fqt_rems);
              };
            };
          }
          catch(CheckIn::UserException)
          {
            throw;
          }
          catch(UserException &e)
          {
            throw CheckIn::UserException(e.getLexemaData(), s->first, iPaxForChng->crs_pax_id);
          };
        };
      }
      catch(CheckIn::UserException)
      {
        throw;
      }
      catch(UserException &e)
      {
        throw CheckIn::UserException(e.getLexemaData(), s->first);
      };
    };
  };
  
  //�����頥� ids
  for(iPnrData=PNRs.begin();iPnrData!=PNRs.end();iPnrData++)
  {
    TIdsPnrData idsPnrData;
    idsPnrData.point_id=iPnrData->point_id;
    idsPnrData.pnr_id=iPnrData->pnr_id;
    idsPnrData.pr_paid_ckin=iPnrData->pr_paid_ckin;
    ids.push_back( idsPnrData );
  };
};

void WebRequestsIface::SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;
	SavePax(reqNode, NULL, resNode);
};

bool WebRequestsIface::SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE1,"WebRequestsIface::SavePax");
	vector< pair<int, TWebPnrForSave > > segs;
	xmlNodePtr segNode=NodeAsNode("segments", reqNode)->children;
	for(;segNode!=NULL;segNode=segNode->next)
	{
	  TWebPnrForSave pnr;
    xmlNodePtr paxNode=GetNode("passengers", segNode);
    if (paxNode!=NULL) paxNode=paxNode->children;
    if (paxNode!=NULL)
    {
      for(;paxNode!=NULL;paxNode=paxNode->next)
      {
        xmlNodePtr node2=paxNode->children;
        TWebPaxFromReq pax;
        
        pax.crs_pax_id=NodeAsIntegerFast("crs_pax_id", node2);
        pax.seat_no=NodeAsStringFast("seat_no", node2, "");
        
        xmlNodePtr fqtNode = GetNode("fqt_rems", paxNode);
        pax.fqt_rems_present=(fqtNode!=NULL); //�᫨ ⥣ <fqt_rems> ��襫, � �����塞 � ��१����뢠�� ६�ન FQTV
        if (fqtNode!=NULL)
        {
          //�⠥� ��襤訥 ६�ન
          for(fqtNode=fqtNode->children; fqtNode!=NULL; fqtNode=fqtNode->next)
          {
            ostringstream rem_text;
            rem_text << "FQTV "
                     << NodeAsString("airline",fqtNode) << " "
                     << NodeAsString("no",fqtNode);
            pax.fqt_rems.push_back(rem_text.str());
          };
        };
        sort(pax.fqt_rems.begin(),pax.fqt_rems.end());
        
        pax.refuse=NodeAsIntegerFast("refuse", node2, 0)!=0;
        if (pax.refuse) pnr.refusalCountFromReq++;
        
        xmlNodePtr tidsNode=NodeAsNode("tids", paxNode);
        pax.crs_pnr_tid=NodeAsInteger("crs_pnr_tid",tidsNode);
        pax.crs_pax_tid=NodeAsInteger("crs_pax_tid",tidsNode);
        xmlNodePtr node;
        node=GetNode("pax_grp_tid",tidsNode);
        if (node!=NULL && !NodeIsNULL(node)) pax.pax_grp_tid=NodeAsInteger(node);
        node=GetNode("pax_tid",tidsNode);
        if (node!=NULL && !NodeIsNULL(node)) pax.pax_tid=NodeAsInteger(node);
        
        pnr.paxFromReq.push_back(pax);
      };
    }
    else
      pnr.pnr_id=NodeAsInteger("pnr_id", segNode);
      
    segs.push_back(make_pair( NodeAsInteger("point_id", segNode), pnr ));
  };

	XMLDoc emulDocHeader;
	CreateEmulXMLDoc(reqNode, emulDocHeader);

	XMLDoc emulCkinDoc;
	map<int,XMLDoc> emulChngDocs;
  vector<TIdsPnrData> ids;
  VerifyPax(segs, emulDocHeader, emulCkinDoc, emulChngDocs, ids);

  int first_grp_id, tckin_id;
  TChangeStatusList ETInfo;
  set<int> tckin_ids;
  bool result=true;
  //�����, �� ᭠砫� ��뢠���� CheckInInterface::SavePax ��� emulCkinDoc
  //⮫쪮 �� ���-ॣ����樨 ����� ��㯯� �������� ROLLBACK CHECKIN � SavePax �� ��ॣ�㧪�
  //� ᮮ⢥��⢥��� �����饭�� result=false
  //� ᮮ⢥��⢥��� �맮� ETStatusInterface::ETRollbackStatus ��� ���� ��
  
  if (emulCkinDoc.docPtr()!=NULL) //ॣ������ ����� ��㯯�
  {
    xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulCkinDoc.docPtr())->children;
    if (emulReqNode==NULL)
      throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: emulReqNode=NULL");
    if (CheckInInterface::SavePax(emulReqNode, ediResNode, first_grp_id, ETInfo, tckin_id))
    {
      if (tckin_id!=NoExists) tckin_ids.insert(tckin_id);
    }
    else
      result=false;
  };
  if (result)
  {
    for(map<int,XMLDoc>::iterator i=emulChngDocs.begin();i!=emulChngDocs.end();i++)
    {
      XMLDoc &emulChngDoc=i->second;
      xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulChngDoc.docPtr())->children;
      if (emulReqNode==NULL)
        throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: emulReqNode=NULL");
      if (CheckInInterface::SavePax(emulReqNode, ediResNode, first_grp_id, ETInfo, tckin_id))
      {
        if (tckin_id!=NoExists) tckin_ids.insert(tckin_id);
      }
      else
      {
        //�� ���� � �� ������� �� ������ �������� (�. �������਩ ���)
        //��� �⮣� ������� �� �����頥� false � ������ ᯥ樠���� ����� � SavePax:
        //�� ����� ��������� ��� � ���᪮� �� �⪠�뢠���� �� ��ॣ�㧪�
        throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: CheckInInterface::SavePax=false");
        //result=false;
        //break;
      };

    };
  };

  if (result)
  {
    if (ediResNode==NULL && !ETInfo.empty())
    {
      //��� �� ���� ����� �㤥� ��ࠡ��뢠����
      OraSession.Rollback();  //�⪠�

      int req_ctxt=AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(reqNode->doc));
      if (!ETStatusInterface::ETChangeStatus(req_ctxt,ETInfo))
        throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: Wrong ETInfo");
      AstraLocale::showProgError("MSG.ETS_CONNECT_ERROR");
      return false;
    };
    
    CheckTCkinIntegrity(tckin_ids, NoExists);
  
    vector< vector<TWebPax> > pnrs;
    xmlNodePtr segsNode = NewTextChild( NewTextChild( resNode, "SavePax" ), "segments" );
    IntLoadPnr( ids, pnrs, segsNode, true );
  };
  return result;
};

class BPTags {
    private:
        std::vector<std::string> tags;
        BPTags();
    public:
        void getFields( std::vector<std::string> &atags );
        static BPTags *Instance() {
            static BPTags *instance_ = 0;
            if ( !instance_ )
                instance_ = new BPTags();
            return instance_;
        }
};

BPTags::BPTags()
{
	TQuery Qry(&OraSession);
	Qry.SQLText = "select code from prn_tag_props where op_type = :op_type order by code";
    Qry.CreateVariable("op_type", otString, EncodeDevOperType(dotPrnBP));
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) {
        if(TAG::GATE == Qry.FieldAsString("code")) continue;
        tags.push_back(lowerc(Qry.FieldAsString("code")));
    }
}

void BPTags::getFields( vector<string> &atags )
{
	atags.clear();
	atags = tags;
}

void GetBPPax(xmlNodePtr paxNode, bool is_test, bool check_tids, PrintInterface::BPPax &pax)
{
  pax.clear();
  pax.pax_id = NodeAsInteger( "pax_id", paxNode );
	xmlNodePtr node = NodeAsNode( "tids", paxNode );
	int crs_pnr_tid = NodeAsInteger( "crs_pnr_tid", node );
	int crs_pax_tid = NodeAsInteger( "crs_pax_tid", node );
	int pax_grp_tid = NodeAsInteger( "pax_grp_tid", node );
	int pax_tid = NodeAsInteger( "pax_tid", node );
	if (check_tids) verifyPaxTids( pax.pax_id, crs_pnr_tid, crs_pax_tid, pax_grp_tid, pax_tid );
	TQuery Qry(&OraSession);
	if (!is_test)
	{
	  Qry.Clear();
  	Qry.SQLText =
  	 "SELECT pax_grp.grp_id, pax_grp.point_dep, "
     "       pax.reg_no, pax.surname||' '||pax.name full_name "
  	 "FROM pax_grp, pax "
  	 "WHERE pax_id=:pax_id AND pax.grp_id=pax_grp.grp_id";
  	Qry.CreateVariable( "pax_id", otInteger, pax.pax_id );
  	Qry.Execute();
  	if ( Qry.Eof )
  		throw UserException( "MSG.PASSENGER.NOT_FOUND" );
  	pax.reg_no = Qry.FieldAsInteger( "reg_no" );
    pax.full_name = Qry.FieldAsString( "full_name" );
  }
  else
  {
    Qry.Clear();
    Qry.SQLText =
      "SELECT reg_no, surname||' '||name full_name "
      "FROM test_pax WHERE id=:pax_id";
    Qry.CreateVariable( "pax_id", otInteger, pax.pax_id );
  	Qry.Execute();
  	if ( Qry.Eof )
  		throw UserException( "MSG.PASSENGER.NOT_FOUND" );
  	pax.reg_no = Qry.FieldAsInteger( "reg_no" );
    pax.full_name = Qry.FieldAsString( "full_name" );

    Qry.Clear();
    Qry.SQLText =
     "SELECT :grp_id AS grp_id, point_id AS point_dep "
     "FROM points "
     "WHERE point_id=:point_id AND pr_del>=0";
    Qry.CreateVariable( "grp_id", otInteger, pax_grp_tid + TEST_ID_BASE );
    Qry.CreateVariable( "point_id", otInteger, pax_grp_tid );
    Qry.Execute();
  	if ( Qry.Eof )
  	  throw UserException( "MSG.FLIGHT.NOT_FOUND" );
  };
	pax.point_dep = Qry.FieldAsInteger( "point_dep" );
	pax.grp_id = Qry.FieldAsInteger( "grp_id" );
};

string GetBPGate(int point_id)
{
  string gate;
  TQuery Qry(&OraSession);
  Qry.Clear();
	Qry.SQLText =
    "SELECT stations.name FROM stations,trip_stations "
    " WHERE point_id=:point_id AND "
    "       stations.desk=trip_stations.desk AND "
    "       stations.work_mode=trip_stations.work_mode AND "
    "       stations.work_mode=:work_mode";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "work_mode", otString, "�" );
	Qry.Execute();
	if ( !Qry.Eof ) {
		gate = Qry.FieldAsString( "name" );
		Qry.Next();
		if ( !Qry.Eof )
      gate.clear();
	};
	return gate;
};

void WebRequestsIface::ConfirmPrintBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

  ProgTrace(TRACE1,"WebRequestsIface::ConfirmPrintBP");
  CheckIn::UserException ue;
  vector<PrintInterface::BPPax> paxs;
  bool is_test=isTestPaxId(NodeAsInteger("passengers/pax/pax_id", reqNode));
  xmlNodePtr paxNode = NodeAsNode("passengers", reqNode)->children;
  for(;paxNode!=NULL;paxNode=paxNode->next)
  {
    PrintInterface::BPPax pax;
    try
    {
      GetBPPax( paxNode, is_test, false, pax );
      pax.time_print=NodeAsDateTime("prn_form_key", paxNode);
      paxs.push_back(pax);
    }
    catch(UserException &e)
    {
      //�� ���� �ப��뢠�� ue � �ନ��� - ���⢥ত��� �� �� �����!
      ue.addError(e.getLexemaData(), pax.point_dep, pax.pax_id);
    };
  };

  if (!is_test)
  {
    PrintInterface::ConfirmPrintBP(paxs, ue);  //�� ���� �ப��뢠�� ue � �ନ��� - ���⢥ত��� �� �� �����!
  };
  
  NewTextChild( resNode, "ConfirmPrintBP" );
};

void WebRequestsIface::GetPrintDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;
  
  ProgTrace(TRACE1,"WebRequestsIface::GetPrintDataBP");
  PrintInterface::BPParams params;
  params.dev_model = NodeAsString("dev_model", reqNode);
  params.fmt_type = NodeAsString("fmt_type", reqNode);
  params.prnParams.get_prn_params(reqNode);
  params.clientDataNode = NULL;
  
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
      "SELECT bp_type, "
      "       DECODE(desk_grp_id,NULL,0,2)+ "
      "       DECODE(desk,NULL,0,4) AS priority "
      "FROM desk_bp_set "
      "WHERE (desk_grp_id IS NULL OR desk_grp_id=:desk_grp_id) AND "
      "      (desk IS NULL OR desk=:desk) "
      "ORDER BY priority DESC ";
  Qry.CreateVariable("desk_grp_id", otInteger, reqInfo->desk.grp_id);
  Qry.CreateVariable("desk", otString, reqInfo->desk.code);
  Qry.Execute();
  if(Qry.Eof) throw AstraLocale::UserException("MSG.BP_TYPE_NOT_ASSIGNED_FOR_DESK");
  params.form_type = Qry.FieldAsString("bp_type");
  ProgTrace(TRACE5, "bp_type: %s", params.form_type.c_str());
  
  CheckIn::UserException ue;
  vector<PrintInterface::BPPax> paxs;
  map<int/*point_dep*/, string/*gate*/> gates;
  bool is_test=isTestPaxId(NodeAsInteger("passengers/pax/pax_id", reqNode));
  xmlNodePtr paxNode = NodeAsNode("passengers", reqNode)->children;
  for(;paxNode!=NULL;paxNode=paxNode->next)
  {
    PrintInterface::BPPax pax;
    try
    {
      GetBPPax( paxNode, is_test, true, pax );
      if (gates.find(pax.point_dep)==gates.end()) gates[pax.point_dep]=GetBPGate(pax.point_dep);
      pax.gate=make_pair(gates[pax.point_dep], true);
      paxs.push_back(pax);
    }
    catch(UserException &e)
    {
      ue.addError(e.getLexemaData(), pax.point_dep, pax.pax_id);
    };
  };
  
  if (!ue.empty()) throw ue;

  string pectab;
  PrintInterface::GetPrintDataBP(params, pectab, paxs);

  xmlNodePtr BPNode = NewTextChild( resNode, "GetPrintDataBP" );
  NewTextChild(BPNode, "pectab", pectab);
  xmlNodePtr passengersNode = NewTextChild(BPNode, "passengers");
  for (vector<PrintInterface::BPPax>::iterator iPax=paxs.begin(); iPax!=paxs.end(); ++iPax )
  {
    xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
    NewTextChild(paxNode, "pax_id", iPax->pax_id);
    if (!iPax->hex && params.prnParams.encoding!="UTF-8")
    {
      iPax->prn_form = ConvertCodepage(iPax->prn_form, "CP866", params.prnParams.encoding);
      StringToHex( string(iPax->prn_form), iPax->prn_form );
      iPax->hex=true;
    };
    SetProp(NewTextChild(paxNode, "prn_form", iPax->prn_form),"hex",(int)iPax->hex);
    NewTextChild(paxNode, "prn_form_key", DateTimeToStr(iPax->time_print));
  };
};

void WebRequestsIface::GetBPTags(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

	ProgTrace(TRACE1,"WebRequestsIface::GetBPTags");
	PrintInterface::BPPax pax;
	bool is_test=isTestPaxId(NodeAsInteger("pax_id", reqNode));
	GetBPPax( reqNode, is_test, true, pax );
	PrintDataParser parser( pax.grp_id, pax.pax_id, 0, NULL );
	vector<string> tags;
	BPTags::Instance()->getFields( tags );
	xmlNodePtr node = NewTextChild( resNode, "GetBPTags" );
    for ( vector<string>::iterator i=tags.begin(); i!=tags.end(); i++ ) {
        for(int j = 0; j < 2; j++) {
            string value = parser.pts.get_tag(*i, ServerFormatDateTimeAsString, (j == 0 ? "R" : "E"));
            NewTextChild( node, (*i + (j == 0 ? "" : "_lat")), value );
            ProgTrace( TRACE5, "field name=%s, value=%s", (*i + (j == 0 ? "" : "_lat")).c_str(), value.c_str() );
        }
    }
  parser.pts.save_bp_print(true);
    
  string gate=GetBPGate(pax.point_dep);
  if (!gate.empty())
    NewTextChild( node, "gate", gate );
}

void ChangeProtPaidLayer(xmlNodePtr reqNode, xmlNodePtr resNode,
                         bool pr_del, int time_limit,
                         int &curr_tid, CheckIn::UserException &ue)
{
  TQuery Qry(&OraSession);
  int point_id=NoExists;
  bool error_exists=false;
  try
  {
    if (!pr_del)
    {
      point_id=NodeAsInteger("point_id", reqNode);
      //�஢�ਬ �ਧ��� ���⭮� ॣ����樨 �
      Qry.Clear();
      Qry.SQLText =
    	  "SELECT pr_permit, prot_timeout FROM trip_paid_ckin WHERE point_id=:point_id";
    	Qry.CreateVariable( "point_id", otInteger, point_id );
    	Qry.Execute();
    	if ( Qry.Eof || Qry.FieldAsInteger("pr_permit")==0 )
    	  throw UserException( "MSG.CHECKIN.NOT_PAID_CHECKIN_MODE" );

    	if (time_limit==NoExists)
    	{
    	  //����稬 prot_timeout
        if (!Qry.FieldIsNULL("prot_timeout"))
          time_limit=Qry.FieldAsInteger("prot_timeout");
    	};
    	if (time_limit==NoExists)
    	  throw UserException( "MSG.PROT_TIMEOUT_NOT_DEFINED" );
    };

    Qry.Clear();
    Qry.DeclareVariable("crs_pax_id", otInteger);
    const char* PaxQrySQL=
      "SELECT crs_pnr.pnr_id, crs_pnr.status AS pnr_status, "
      "       crs_pnr.point_id, crs_pnr.airp_arv, "
      "       crs_pnr.subclass, crs_pnr.class, "
      "       crs_pnr.tid AS crs_pnr_tid, "
      "       crs_pax.tid AS crs_pax_tid, "
      "       DECODE(pax.pax_id,NULL,0,1) AS checked "
      "FROM crs_pnr, crs_pax, pax "
      "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "      crs_pax.pax_id=:crs_pax_id AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      crs_pax.pr_del=0";
    const char* TestPaxQrySQL=
      "SELECT id AS pnr_id, NULL AS pnr_status, "
      "       subclass, subcls.class, "
      "       id AS crs_pnr_tid, "
      "       id AS crs_pax_tid, "
      "       0 AS checked "
  	  "FROM test_pax, subcls "
  	  "WHERE test_pax.subclass=subcls.code AND test_pax.id=:crs_pax_id";

    TQuery PaxStatusQry(&OraSession);
    int pnr_id=NoExists, point_id_tlg=NoExists;
    string airp_arv;
    vector<TWebPax> pnr;
    xmlNodePtr node=NodeAsNode("passengers", reqNode)->children;
    for(;node!=NULL;node=node->next)
    {
      xmlNodePtr node2=node->children;
      TWebPax pax;

      pax.crs_pax_id=NodeAsIntegerFast("crs_pax_id", node2);
      try
      {
        if (!pr_del)
        {
          pax.crs_seat_no=NodeAsStringFast("seat_no", node2);
          if (pax.crs_seat_no.empty())
            throw EXCEPTIONS::Exception("ChangeProtPaidLayer: empty seat_no (crs_pax_id=%d)", pax.crs_pax_id);

          //�஢�ਬ �� �㡫�஢����
          for(vector<TWebPax>::const_iterator iPax=pnr.begin();iPax!=pnr.end();iPax++)
          {
            if (iPax->crs_pax_id==pax.crs_pax_id)
              throw EXCEPTIONS::Exception("ChangeProtPaidLayer: crs_pax_id duplicated (crs_pax_id=%d)",
                                          pax.crs_pax_id);
            if (iPax->crs_seat_no==pax.crs_seat_no)
              throw EXCEPTIONS::Exception("ChangeProtPaidLayer: seat_no duplicated (crs_pax_id=%d, seat_no=%s)",
                                          pax.crs_pax_id, pax.crs_seat_no.c_str());
          };
        };
        
        xmlNodePtr tidsNode=NodeAsNodeFast("tids", node2);
        pax.crs_pnr_tid=NodeAsInteger("crs_pnr_tid",tidsNode);
        pax.crs_pax_tid=NodeAsInteger("crs_pax_tid",tidsNode);

        //�஢�ਬ tids ���ᠦ��
        if (!isTestPaxId(pax.crs_pax_id))
          Qry.SQLText=PaxQrySQL;
        else
          Qry.SQLText=TestPaxQrySQL;
        Qry.SetVariable("crs_pax_id",pax.crs_pax_id);
        Qry.Execute();
        if (Qry.Eof)
          throw UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
        if (Qry.FieldAsInteger("checked")!=0)
          throw UserException("MSG.PASSENGER.CHECKED.REFRESH_DATA");
        if (pax.crs_pnr_tid!=Qry.FieldAsInteger("crs_pnr_tid") ||
            pax.crs_pax_tid!=Qry.FieldAsInteger("crs_pax_tid"))
          throw UserException("MSG.PASSENGER.CHANGED.REFRESH_DATA");

        if (!is_valid_pnr_status(Qry.FieldAsString("pnr_status")) ||
            !is_valid_pax_status(pax.crs_pax_id, PaxStatusQry))
          throw UserException("MSG.PASSENGER.CHECKIN_DENIAL");

        if (pnr_id==NoExists)
        {
          pnr_id=Qry.FieldAsInteger("pnr_id");
          if (!isTestPaxId(pax.crs_pax_id))
          {
            point_id_tlg=Qry.FieldAsInteger("point_id");
            airp_arv=Qry.FieldAsString("airp_arv");
          };
        }
        else
        {
          if (pnr_id!=Qry.FieldAsInteger("pnr_id"))
            throw EXCEPTIONS::Exception("ChangeProtPaidLayer: passengers from different PNR (crs_pax_id=%d)", pax.crs_pax_id);
        };
        pax.pass_class=Qry.FieldAsString("class");
        pax.pass_subclass=Qry.FieldAsString("subclass");
        pnr.push_back(pax);
      }
      catch(UserException &e)
      {
        ue.addError(e.getLexemaData(), point_id, pax.crs_pax_id);
        error_exists=true;
      };
    };
    
    vector< pair<TWebPlace, LexemaData> > pax_seats;
    if (!pnr.empty())
    {
      TPointIdsForCheck point_ids_spp;
      if (!pr_del)
      {
        TQuery LayerQry(&OraSession);
        LayerQry.Clear();
        LayerQry.SQLText=
          "DECLARE "
          "  vrange_id    tlg_comp_layers.range_id%TYPE; "
          "  vpoint_id    tlg_comp_layers.point_id%TYPE; "
          "  vairp_arv    tlg_comp_layers.airp_arv%TYPE; "
          "  vfirst_xname tlg_comp_layers.first_xname%TYPE; "
          "  vfirst_yname tlg_comp_layers.first_yname%TYPE; "
          "  vlast_xname  tlg_comp_layers.last_xname%TYPE; "
          "  vlast_yname  tlg_comp_layers.last_yname%TYPE; "
          "  vrem_code    tlg_comp_layers.rem_code%TYPE; "
          "BEGIN "
          "  :delete_seat_ranges:=1; "
          "  BEGIN "
          "    SELECT range_id, point_id, airp_arv, "
          "           first_xname, first_yname, last_xname, last_yname, rem_code "
          "    INTO vrange_id, vpoint_id, vairp_arv, "
          "         vfirst_xname, vfirst_yname, vlast_xname, vlast_yname, vrem_code "
          "    FROM tlg_comp_layers "
          "    WHERE crs_pax_id=:crs_pax_id AND layer_type=:layer_type FOR UPDATE; "
          "    IF :point_id=vpoint_id AND :airp_arv=vairp_arv AND "
          "       :first_xname=vfirst_xname AND :first_yname=vfirst_yname AND "
          "       :last_xname=vlast_xname AND :last_yname=vlast_yname THEN "
          "      :delete_seat_ranges:=0; "
          "      UPDATE tlg_comp_layers "
          "      SET time_remove=SYSTEM.UTCSYSDATE+:timeout/1440 "
          "      WHERE range_id=vrange_id; "
          "    END IF; "
          "  EXCEPTION "
          "    WHEN NO_DATA_FOUND THEN NULL; "
          "    WHEN TOO_MANY_ROWS THEN NULL; "
          "  END; "
          "END; ";
        LayerQry.DeclareVariable("delete_seat_ranges", otInteger);
        LayerQry.DeclareVariable("point_id", otInteger);
        LayerQry.DeclareVariable("airp_arv", otString);
        LayerQry.DeclareVariable("layer_type", otString);
        LayerQry.DeclareVariable("first_xname", otString);
        LayerQry.DeclareVariable("last_xname", otString);
        LayerQry.DeclareVariable("first_yname", otString);
        LayerQry.DeclareVariable("last_yname", otString);
        LayerQry.DeclareVariable("crs_pax_id", otInteger);
        if (time_limit!=NoExists)
          LayerQry.CreateVariable("timeout", otInteger, time_limit);
        else
          LayerQry.CreateVariable("timeout", otInteger, FNull);

        VerifyPNR(point_id, pnr_id);
        GetCrsPaxSeats(point_id, pnr, pax_seats );
        bool UsePriorContext=false;
        for(vector<TWebPax>::const_iterator iPax=pnr.begin();iPax!=pnr.end();iPax++)
        {
          try
          {
            vector< pair<TWebPlace, LexemaData> >::const_iterator iSeat=pax_seats.begin();
            for(;iSeat!=pax_seats.end();iSeat++)
              if (iSeat->first.pax_id==iPax->crs_pax_id &&
                  iSeat->first.seat_no==iPax->crs_seat_no) break;
            if (iSeat==pax_seats.end())
              throw EXCEPTIONS::Exception("ChangeProtPaidLayer: passenger not found in pax_seats (crs_pax_id=%d, crs_seat_no=%s)",
                                          iPax->crs_pax_id, iPax->crs_seat_no.c_str());
            if (!iSeat->second.lexema_id.empty())
              throw UserException(iSeat->second.lexema_id, iSeat->second.lparams);
              
/*            if ( iSeat->first.WebTariff.value == 0.0 )  //��� ���
              throw UserException("MSG.SEATS.NOT_SET_RATE");*/
            
            if (isTestPaxId(iPax->crs_pax_id)) continue;

            vector<TSeatRange> ranges(1,TSeatRange(TSeat(iSeat->first.yname,
                                                         iSeat->first.xname),
                                                   TSeat(iSeat->first.yname,
                                                         iSeat->first.xname)));

            LayerQry.SetVariable("delete_seat_ranges", 1);
            LayerQry.SetVariable("point_id", point_id_tlg);
            LayerQry.SetVariable("airp_arv", airp_arv);
            LayerQry.SetVariable("layer_type", EncodeCompLayerType(cltProtBeforePay));
            LayerQry.SetVariable("first_xname", iSeat->first.xname);
            LayerQry.SetVariable("last_xname", iSeat->first.xname);
            LayerQry.SetVariable("first_yname", iSeat->first.yname);
            LayerQry.SetVariable("last_yname", iSeat->first.yname);
            LayerQry.SetVariable("crs_pax_id", iPax->crs_pax_id);
            LayerQry.Execute();
            if (LayerQry.GetVariableAsInteger("delete_seat_ranges")!=0)
            {
              DeleteTlgSeatRanges(cltProtBeforePay, iPax->crs_pax_id, curr_tid, point_ids_spp);
              InsertTlgSeatRanges(point_id_tlg,
                                  airp_arv,
                                  cltProtBeforePay,
                                  ranges,
                                  iPax->crs_pax_id,
                                  NoExists,
                                  time_limit,
                                  UsePriorContext,
                                  curr_tid,
                                  point_ids_spp);
              UsePriorContext=true;
            };
          }
          catch(UserException &e)
          {
            ue.addError(e.getLexemaData(), point_id, iPax->crs_pax_id);
            error_exists=true;
          };
        };
      }
      else
      {
        //RemoveProtPaidLayer
        for(vector<TWebPax>::const_iterator iPax=pnr.begin();iPax!=pnr.end();iPax++)
        {
          try
          {
            if (isTestPaxId(iPax->crs_pax_id)) continue;
            DeleteTlgSeatRanges(cltProtBeforePay, iPax->crs_pax_id, curr_tid, point_ids_spp);
          }
          catch(UserException &e)
          {
            ue.addError(e.getLexemaData(), point_id, iPax->crs_pax_id);
            error_exists=true;
          };
        };
      };
      check_layer_change(point_ids_spp);
    }; //!pnr.empty()
    if (error_exists) return; //�᫨ ���� �訡��, ��� �� ��ࠡ�⪨ ᥣ����
    
    //�ନ஢���� �⢥�
    NewTextChild(resNode,"point_id",point_id,NoExists);
    NewTextChild(resNode,"time_limit",time_limit,NoExists);
    
    xmlNodePtr paxsNode=NewTextChild(resNode, "passengers");
    for(vector<TWebPax>::iterator iPax=pnr.begin();iPax!=pnr.end();iPax++)
    {
      try
      {
        if (!isTestPaxId(iPax->crs_pax_id))
          Qry.SQLText=PaxQrySQL;
        else
          Qry.SQLText=TestPaxQrySQL;
        Qry.SetVariable("crs_pax_id",iPax->crs_pax_id);
        Qry.Execute();
        if (Qry.Eof)
          throw UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
        iPax->crs_pnr_tid=Qry.FieldAsInteger("crs_pnr_tid");
        iPax->crs_pax_tid=Qry.FieldAsInteger("crs_pax_tid");

        xmlNodePtr paxNode=NewTextChild(paxsNode, "pax");
        NewTextChild(paxNode, "crs_pax_id", iPax->crs_pax_id);
        xmlNodePtr tidsNode=NewTextChild(paxNode, "tids");
        NewTextChild(tidsNode, "crs_pnr_tid", iPax->crs_pnr_tid);
        NewTextChild(tidsNode, "crs_pax_tid", iPax->crs_pax_tid);
        if (!pr_del)
        {
          vector< pair<TWebPlace, LexemaData> >::const_iterator iSeat=pax_seats.begin();
          for(;iSeat!=pax_seats.end();iSeat++)
            if (iSeat->first.pax_id==iPax->crs_pax_id &&
                iSeat->first.seat_no==iPax->crs_seat_no) break;
          if (iSeat!=pax_seats.end())
          {
            if ( iSeat->first.WebTariff.value != 0.0 ) { // �᫨ ���⭠� ॣ������ �⪫�祭�, value=0.0 � �� ��砥
            	xmlNodePtr rateNode = NewTextChild( paxNode, "rate" );
            	NewTextChild( rateNode, "color", iSeat->first.WebTariff.color );
            	ostringstream buf;
              buf << std::fixed << setprecision(2) << iSeat->first.WebTariff.value;
            	NewTextChild( rateNode, "value", buf.str() );
            	NewTextChild( rateNode, "currency", iSeat->first.WebTariff.currency_id );
            };
          };
        };
      }
      catch(UserException &e)
      {
        ue.addError(e.getLexemaData(), point_id, iPax->crs_pax_id);
      };
    };
  }
  catch(UserException &e)
  {
    ue.addError(e.getLexemaData(), point_id);
  };
};

void WebRequestsIface::AddProtPaidLayer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  resNode=NewTextChild(resNode,"AddProtPaidLayer");
  int time_limit=NoExists;
  int curr_tid=NoExists;
  CheckIn::UserException ue;
  xmlNodePtr node=GetNode("time_limit",reqNode);
  if (node!=NULL && !NodeIsNULL(node))
  {
    time_limit=NodeAsInteger(node);
    if (time_limit<=0 || time_limit>999)
      throw EXCEPTIONS::Exception("AddProtPaidLayer: wrong time_limit %d min", time_limit);
  };

  //����� ��窠 ३ᮢ � ���浪� ���஢�� point_id
  vector<int> point_ids;
  node=NodeAsNode("segments", reqNode)->children;
  for(;node!=NULL;node=node->next)
    point_ids.push_back(NodeAsInteger("point_id", node));
  sort(point_ids.begin(),point_ids.end());
  
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT point_id, pr_del, pr_reg "
    "FROM points "
    "WHERE point_id=:point_id FOR UPDATE";
  Qry.DeclareVariable("point_id", otInteger);
  for(vector<int>::const_iterator i=point_ids.begin(); i!=point_ids.end(); i++)
  {
    try
    {
      Qry.SetVariable("point_id", *i);
      Qry.Execute();
      if ( Qry.Eof )
    		throw UserException( "MSG.FLIGHT.NOT_FOUND" );
    	if ( Qry.FieldAsInteger( "pr_del" ) < 0 )
    		throw UserException( "MSG.FLIGHT.DELETED" );
    	if ( Qry.FieldAsInteger( "pr_del" ) > 0 )
    		throw UserException( "MSG.FLIGHT.CANCELED" );
    	if ( Qry.FieldAsInteger( "pr_reg" ) == 0 )
    		throw UserException( "MSG.FLIGHT.CHECKIN_CANCELED" );
    }
    catch(UserException &e)
    {
      ue.addError(e.getLexemaData(), *i);
    };
  };
  if (!ue.empty()) throw ue;
  
  node=NodeAsNode("segments", reqNode)->children;
  xmlNodePtr segsNode=NewTextChild(resNode, "segments");
  for(;node!=NULL;node=node->next)
  {
    xmlNodePtr segNode=NewTextChild(segsNode, "segment");
    ChangeProtPaidLayer(node, segNode, false, time_limit, curr_tid, ue );
  };
  if (!ue.empty()) throw ue;
};

void WebRequestsIface::RemoveProtPaidLayer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  resNode=NewTextChild(resNode,"RemoveProtPaidLayer");
  int curr_tid=NoExists;
  CheckIn::UserException ue;
  ChangeProtPaidLayer(reqNode, resNode, true, NoExists, curr_tid, ue);
  if (!ue.empty()) throw ue;
};

inline void CreateXMLStage( const TCkinClients &CkinClients, TStage stage_id, const TTripStage &stage,
                            xmlNodePtr node, const string &region )
{
  TStagesRules *sr = TStagesRules::Instance();
  if ( sr->isClientStage( (int)stage_id ) && !sr->canClientStage( CkinClients, (int)stage_id ) )
    return;
  xmlNodePtr node1 = NewTextChild( node, "stage" );
  SetProp( node1, "stage_id", stage_id );
  NewTextChild( node1, "scd", DateTimeToStr( UTCToClient( stage.scd, region ), "dd.mm.yyyy hh:nn" ) );
  if ( stage.est != ASTRA::NoExists )
    NewTextChild( node1, "est", DateTimeToStr( UTCToClient( stage.est, region ), "dd.mm.yyyy hh:nn" ) );
  if ( stage.act != ASTRA::NoExists )
    NewTextChild( node1, "act", DateTimeToStr( UTCToClient( stage.act, region ), "dd.mm.yyyy hh:nn" ) );
}

////////////////////////////////////MERIDIAN SYSTEM/////////////////////////////
void WebRequestsIface::GetFlightInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  string airline;
  int flt_no;
  string str_flt_no;
  string suffix;
  string str_scd_out;
  TDateTime scd_out;
  string airp_dep;
  string region;
  TElemFmt fmt;
  
  xmlNodePtr node = GetNode( "airline", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag 'airline' not found" );
  airline = NodeAsString( node );
  airline = ElemToElemId( etAirline, airline, fmt );
  if ( fmt == efmtUnknown )
    throw UserException( "MSG.AIRLINE.INVALID",
    	                   LParams()<<LParam("airline",NodeAsString(node)) );
  node = GetNode( "flt_no", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag 'flt_no' not found" );
  str_flt_no =  NodeAsString( node );
	if ( StrToInt( str_flt_no.c_str(), flt_no ) == EOF ||
		   flt_no > 99999 || flt_no <= 0 )
		throw UserException( "MSG.FLT_NO.INVALID",
			                   LParams()<<LParam("flt_no", str_flt_no) );
  node = GetNode( "suffix", reqNode );
  if ( node != NULL ) {
    suffix =  NodeAsString( node );
    if ( !suffix.empty() ) {
      suffix = ElemToElemId( etSuffix, suffix, fmt );
      if ( fmt == efmtUnknown )
        throw UserException( "MSG.SUFFIX.INVALID",
    	                       LParams()<<LParam("suffix",NodeAsString(node)) );
    }
  }
  node = GetNode( "scd_out", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag 'scd_out' not found" );
  str_scd_out = NodeAsString( node );
  ProgTrace( TRACE5, "str_scd_out=|%s|", str_scd_out.c_str() );
  if ( str_scd_out.empty() )
		throw UserException( "MSG.FLIGHT_DATE.NOT_SET" );
	else
		if ( BASIC::StrToDateTime( str_scd_out.c_str(), "dd.mm.yyyy hh:nn", scd_out ) == EOF )
			throw UserException( "MSG.FLIGHT_DATE.INVALID",
				                   LParams()<<LParam("scd_out", str_scd_out) );
	node = GetNode( "airp_dep", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag 'airp_dep' not found" );
  airp_dep = NodeAsString( node );
  airp_dep = ElemToElemId( etAirp, airp_dep, fmt );
  if ( fmt == efmtUnknown )
    throw UserException( "MSG.AIRP.INVALID_INPUT_VALUE",
    	                   LParams()<<LParam("airp",NodeAsString(node)) );
  TReqInfo *reqInfo = TReqInfo::Instance();
  region = AirpTZRegion( airp_dep );
  scd_out = LocalToUTC( scd_out, region );
  ProgTrace( TRACE5, "scd_out=%f", scd_out );
  if ( !reqInfo->CheckAirline( airline ) ||
       !reqInfo->CheckAirp( airp_dep ) )
    throw UserException( "MSG.FLIGHT.ACCESS_DENIED" );
    
  int findMove_id, point_id;
  if ( !TPoints::isDouble( ASTRA::NoExists, airline, flt_no, suffix, airp_dep, ASTRA::NoExists, scd_out,
        findMove_id, point_id  ) )
     throw UserException( "MSG.FLIGHT.NOT_FOUND" );

	TFlightStations stations;
	stations.Load( point_id );
	TFlightStages stages;
	stages.Load( point_id );
	TCkinClients CkinClients;
	TTripStages::ReadCkinClients( point_id, CkinClients );
	xmlNodePtr flightNode = NewTextChild( resNode, "trip" );
	airline += str_flt_no + suffix;
	SetProp( flightNode, "flightNumber", airline );
  SetProp( flightNode, "date", DateTimeToStr( UTCToClient( scd_out, region ), "dd.mm.yyyy hh:nn" ) );
  SetProp( flightNode, "departureAirport", airp_dep );
  node = NewTextChild( flightNode, "stages" );
  CreateXMLStage( CkinClients, sPrepCheckIn, stages.GetStage( sPrepCheckIn ), node, region );
  CreateXMLStage( CkinClients, sOpenCheckIn, stages.GetStage( sOpenCheckIn ), node, region );
  CreateXMLStage( CkinClients, sCloseCheckIn, stages.GetStage( sCloseCheckIn ), node, region );
  CreateXMLStage( CkinClients, sOpenBoarding, stages.GetStage( sOpenBoarding ), node, region );
  CreateXMLStage( CkinClients, sCloseBoarding, stages.GetStage( sCloseBoarding ), node, region );
  CreateXMLStage( CkinClients, sOpenWEBCheckIn, stages.GetStage( sOpenWEBCheckIn ), node, region );
  CreateXMLStage( CkinClients, sCloseWEBCheckIn, stages.GetStage( sCloseWEBCheckIn ), node, region );
  CreateXMLStage( CkinClients, sOpenKIOSKCheckIn, stages.GetStage( sOpenKIOSKCheckIn ), node, region );
  CreateXMLStage( CkinClients, sCloseKIOSKCheckIn, stages.GetStage( sCloseKIOSKCheckIn ), node, region );
  tstations sts;
  stations.Get( sts );
  xmlNodePtr node1 = NULL;
  for ( tstations::iterator i=sts.begin(); i!=sts.end(); i++ ) {
    if ( node1 == NULL )
      node1 = NewTextChild( flightNode, "stations" );
    SetProp( NewTextChild( node1, "station", i->name ), "work_mode", i->work_mode );
  }
}

void WebRequestsIface::ParseMessage(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node = GetNode( "@type", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag '@type' not found" );
  string stype = NodeAsString( node );
  string body = NodeAsString( reqNode );
  ProgTrace( TRACE5, "ParseMessage: stype=%s, body=|%s|", stype.c_str(), body.c_str() );
  //ࠧ��ઠ ⥫��ࠬ�� ���� ssm
  TQuery Qry(&OraSession);
  Qry.SQLText = "insert into ssm_in(id, type, data) values(ssm_in__seq.nextval, :type, :data)";
  Qry.CreateVariable("data", otString, body);
  Qry.CreateVariable("type", otString, stype);
  Qry.Execute();
}
////////////////////////////////////END MERIDIAN SYSTEM/////////////////////////////
} //end namespace AstraWeb

