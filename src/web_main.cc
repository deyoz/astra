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
#include "convert.h"
#include "basic.h"
#include "astra_misc.h"
#include "print.h"
#include "web_main.h"
#include "checkin.h"
#include "astra_locale.h"
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

const string PARTITION_ELEM_TYPE = "П";
const string ARMCHAIR_ELEM_TYPE = "К";

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
    newlen=answer.size()+hlen; // размер ответа (с заголовком)
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
	TDateTime scd_out, scd_out_local;
	string craft;
	string airp_dep;
	string city_dep;
	map<TStage, TDateTime> stages;
	TStage web_stage;
	TStage kiosk_stage;
	TStage checkin_stage;
	TStage brd_stage;
	TDateTime act_out;
	std::vector<TTripInfo> mark_flights;

	int airline_fmt,suffix_fmt,airp_dep_fmt,craft_fmt;

  int pnr_id;
  int point_arv;
	TDateTime scd_in;
	string airp_arv;
	string city_arv;
	string cls;
	string subcls;
	vector<TPnrAddr> pnr_addrs;
	int bag_norm;
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

#define TRACE_SIGNATURE int Level, const char *nickname, const char *filename, int line
#define TRACE_PARAMS Level, nickname, filename, line

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

void traceTrfer( TRACE_SIGNATURE,
                 const string &descr,
                 const vector<TTransferItem> &trfer )
{
  ProgTrace(TRACE_PARAMS, "============ %s ============", descr.c_str());
  for(vector<TTransferItem>::const_iterator iTrfer=trfer.begin();iTrfer!=trfer.end();iTrfer++)
  {
    ostringstream str;

    if (iTrfer==trfer.begin())
    {
      str << setw(3) << right << "num" << " "
          << setw(3) << left  << "a/l" << " "
          << setw(6) << right << "flt_no" << " "
          << setw(4) << right << "date" << " "
          << setw(3) << left  << "dep" << " "
          << setw(3) << left  << "arv" << " "
          << setw(3) << left  << "scl";
      ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

      str.str("");
    };

    str << setw(2) << right << (iTrfer->num !=NoExists ? IntToString(iTrfer->num) : " ") << ": "
        << setw(3) << left  << iTrfer->airline << " "
        << setw(5) << right << (iTrfer->flt_no !=NoExists ? IntToString(iTrfer->flt_no) : " ")
        << setw(1) << left  << iTrfer->suffix << " "
        << setw(4) << right << (iTrfer->local_date !=NoExists ? IntToString(iTrfer->local_date) : " ") << " "
        << setw(3) << left  << iTrfer->airp_dep << " "
        << setw(3) << left  << iTrfer->airp_arv << " "
        << setw(3) << left  << iTrfer->subcl;
    ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());
  };

  ProgTrace(TRACE_PARAMS, "^^^^^^^^^^^^ %s ^^^^^^^^^^^^", descr.c_str());
};

void traceTrfer( TRACE_SIGNATURE,
                 const string &descr,
                 const vector<CheckIn::TTransferItem> &trfer )
{
  ProgTrace(TRACE_PARAMS, "============ %s ============", descr.c_str());
  int trfer_num=1;
  for(vector<CheckIn::TTransferItem>::const_iterator iTrfer=trfer.begin();iTrfer!=trfer.end();iTrfer++,trfer_num++)
  {
    ostringstream str;

    if (iTrfer==trfer.begin())
    {
      str << setw(3) << right << "num" << " "
          << setw(3) << left  << "a/l" << " "
          << setw(6) << right << "flt_no" << " "
          << setw(4) << right << "date" << " "
          << setw(3) << left  << "dep" << " "
          << setw(3) << left  << "arv" << " "
          << setw(3) << left  << "scl";
      ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

      str.str("");
    };

    str << setw(2) << right << trfer_num << ": "
        << setw(3) << left  << iTrfer->operFlt.airline << " "
        << setw(5) << right << (iTrfer->operFlt.flt_no !=NoExists ? IntToString(iTrfer->operFlt.flt_no) : " ")
        << setw(1) << left  << iTrfer->operFlt.suffix << " "
        << setw(4) << right << (iTrfer->operFlt.scd_out !=NoExists ? DateTimeToStr(iTrfer->operFlt.scd_out,"dd") : " ") << " "
        << setw(3) << left  << iTrfer->operFlt.airp << " "
        << setw(3) << left  << iTrfer->airp_arv << " "
        << setw(3) << left  << iTrfer->subclass;
    ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());
  };

  ProgTrace(TRACE_PARAMS, "^^^^^^^^^^^^ %s ^^^^^^^^^^^^", descr.c_str());
};

void traceTCkinSegs( TRACE_SIGNATURE,
                    const string &descr,
                    const vector<TCkinSegFlts> &segs )
{
  ProgTrace(TRACE_PARAMS, "============ %s ============", descr.c_str());
  int seg_no=1;
  for(vector<TCkinSegFlts>::const_iterator iSeg=segs.begin();iSeg!=segs.end();iSeg++)
  {
    ostringstream str;

    if (iSeg==segs.begin())
    {
      str << setw(3) << right << "seg" << " "
          << setw(3) << left  << "a/l" << " "
          << setw(6) << right << "flt_no" << " "
          << setw(3) << left  << "a/p" << " "
          << setw(9) << left  << "scd_out" << " "
          << setw(9) << right << "point_dep" << " "
          << setw(9) << right << "point_arv" << " "
          << setw(3) << left  << "dep" << " "
          << setw(3) << left  << "arv";
      ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

      str.str("");
    };

    if (!iSeg->flts.empty())
    {
      for(vector<TSegInfo>::const_iterator f=iSeg->flts.begin();f!=iSeg->flts.end();f++)
      {
        if (f==iSeg->flts.begin())
          str << setw(2) << right << seg_no << ": ";
        else
          str << setw(2) << right << " " << "  ";

        str << setw(3) << left  << f->fltInfo.airline << " "
            << setw(5) << right << (f->fltInfo.flt_no != NoExists ? IntToString(f->fltInfo.flt_no) : " ")
            << setw(1) << left  << f->fltInfo.suffix << " "
            << setw(3) << left  << f->fltInfo.airp << " "
            << setw(9) << left  << DateTimeToStr(f->fltInfo.scd_out,"ddmm hhnn") << " "
            << setw(9) << right << (f->point_dep != NoExists ? IntToString(f->point_dep) : "NoExists") << " "
            << setw(9) << right << (f->point_arv != NoExists ? IntToString(f->point_arv) : "NoExists") << " "
            << setw(3) << left  << f->airp_dep << " "
            << setw(3) << left  << f->airp_arv;
        ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

        str.str("");
      };
    }
    else
    {
      str << setw(2) << right << seg_no << ": " << "not found!";

      ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

      str.str("");
    };

    seg_no++;
  };
  ProgTrace(TRACE_PARAMS, "^^^^^^^^^^^^ %s ^^^^^^^^^^^^", descr.c_str());
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
  	  "SELECT point_id,point_num,first_point,pr_tranzit,pr_del,pr_reg,scd_out,act_out,"
  	  "       airline,airline_fmt,flt_no,airp,airp_fmt,suffix,suffix_fmt,"
  	  "       craft,craft_fmt FROM points "
  	  " WHERE point_id=:point_id";
  	Qry.CreateVariable( "point_id", otInteger, point_id );
  	Qry.Execute();
  	if ( Qry.Eof )
  		throw UserException( "MSG.FLIGHT.NOT_FOUND" );

  	if ( Qry.FieldAsInteger( "pr_del" ) == -1 )
  		throw UserException( "MSG.FLIGHT.DELETED" );

    if ( !reqInfo->CheckAirline(Qry.FieldAsString("airline")) ||
         !reqInfo->CheckAirp(Qry.FieldAsString("airp")) )
      throw UserException( "MSG.FLIGHT.ACCESS_DENIED" );

  	if ( Qry.FieldAsInteger( "pr_del" ) == 1 )
  		throw UserException( "MSG.FLIGHT.CANCELED" );

  	if ( Qry.FieldAsInteger( "pr_reg" ) == 0 )
  		throw UserException( "MSG.FLIGHT.CHECKIN_CANCELED" );

  	if ( Qry.FieldIsNULL( "act_out" ) )
  		SearchPnrData.act_out = NoExists;
  	else
  		SearchPnrData.act_out = Qry.FieldAsDateTime( "act_out" );

  	TTripStages tripStages( point_id );
  	SearchPnrData.web_stage = tripStages.getStage( stWEB );
    SearchPnrData.kiosk_stage = tripStages.getStage( stKIOSK );
  	SearchPnrData.checkin_stage = tripStages.getStage( stCheckIn );
  	SearchPnrData.brd_stage = tripStages.getStage( stBoarding );

  	ProgTrace( TRACE5, "web_stage=%d, kiosk_stage=%d, checkin_stage=%d, brd_stage=%d",
  	           (int)SearchPnrData.web_stage,
  	           (int)SearchPnrData.kiosk_stage,
               (int)SearchPnrData.checkin_stage,
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
  	SearchPnrData.craft = Qry.FieldAsString( "craft" );
  	SearchPnrData.craft_fmt = Qry.FieldAsInteger( "craft_fmt" );
  	
  	SearchPnrData.point_id = point_id;
  	SearchPnrData.point_num = Qry.FieldAsInteger("point_num");
  	SearchPnrData.first_point = Qry.FieldAsInteger("first_point");
  	SearchPnrData.pr_tranzit = Qry.FieldAsInteger("pr_tranzit")!=0;

  	TTripInfo operFlt(Qry);
  	GetMktFlights(operFlt, SearchPnrData.mark_flights);
  	
  	SearchPnrData.stages.clear();
  	
  	TStagesRules *sr = TStagesRules::Instance();
  	TCkinClients ckin_clients;
  	TTripStages::ReadCkinClients( point_id, ckin_clients );
  	TStage stage;
    for(int pass=0; pass<6; pass++)
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
        case 2: stage=sOpenCheckIn;
                break;
        case 3: stage=sCloseCheckIn;
                break;
        case 4: stage=sOpenBoarding;
                break;
        case 5: stage=sCloseBoarding;
                break;
      };
      
      if ( reqInfo->client_type == ctKiosk && first_segment )
      {
        //проверяем возможность регистрации для киоска только на первом сегменте
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
            if (SearchPnrData.kiosk_stage==sOpenKIOSKCheckIn ||
                SearchPnrData.kiosk_stage==sCloseKIOSKCheckIn)
              SearchPnrData.kiosk_stage=sNoActive;
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
    //заполняем scd_in и city_arv
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
     "SELECT scd_in FROM points WHERE point_id=:point_id AND airp=:airp AND pr_del=0";
    Qry.CreateVariable( "point_id", otInteger, SearchPnrData.point_arv );
    Qry.CreateVariable( "airp", otString, SearchPnrData.airp_arv );
  	Qry.Execute();
  	if (Qry.Eof)
  	  throw UserException( "MSG.FLIGHT.NOT_FOUND" );

    string region = AirpTZRegion(SearchPnrData.airp_arv);
    if ( Qry.FieldIsNULL( "scd_in" ) )
    	SearchPnrData.scd_in = NoExists;
    else
      SearchPnrData.scd_in = UTCToLocal( Qry.FieldAsDateTime( "scd_in" ), region );

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
	//first_segment=false специально, чтобы действовало более мягкое условие на возможную неактивность этапа тех. графика
	return getTripData( point_id, false, SearchPnrData, pr_throw );
}

void getTCkinData( const TSearchPnrData &firstPnrData,
                   vector<TSearchPnrData> &pnrs)
{
  pnrs.clear();
  pnrs.push_back(firstPnrData);
  
  TReqInfo *reqInfo = TReqInfo::Instance();
  
  //поиск стыковочных сегментов (возвращаем вектор point_id)
  TQuery Qry(&OraSession);
  vector<TTransferItem> crs_trfer;
  vector<CheckIn::TTransferItem> trfer;
  CheckInInterface::GetOnwardCrsTransfer(firstPnrData.pnr_id, Qry, crs_trfer);
  if (!crs_trfer.empty())
  {
    //проверяем разрешение сквозной регистрации для данного типа клиента
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
      //сквозная регистрация запрещена
      ProgTrace(TRACE5, ">>>> Through check-in not permitted (point_id=%d, client_type=%s, desk_grp_id=%d)",
                        firstPnrData.point_id, EncodeClientType(reqInfo->client_type), reqInfo->desk.grp_id);
      return;
    };
  
  
    TTripInfo operFlt;
    operFlt.airline=firstPnrData.airline;
    operFlt.flt_no=firstPnrData.flt_no;
    operFlt.suffix=firstPnrData.suffix;
    operFlt.airp=firstPnrData.airp_dep;
    operFlt.scd_out=firstPnrData.scd_out;

    //трансфер есть. проверим сквозняк
    CheckInInterface::LoadOnwardCrsTransfer(operFlt,
                                            firstPnrData.airp_arv,
                                            "",
                                            crs_trfer, trfer, NULL);

    if (crs_trfer.size()!=trfer.size())
    {
      traceTrfer(TRACE5, "crs_trfer", crs_trfer);
      traceTrfer(TRACE5, "trfer", trfer);
    };

    vector<TCkinSegFlts> segs;
    CheckInInterface::GetTCkinFlights(trfer, segs);

    int seg_no=1;
    try
    {
      string airline_in=operFlt.airline;
      int flt_no_in=operFlt.flt_no;

      //цикл по стыковочным сегментам и по трансферным рейсам
      vector<TCkinSegFlts>::const_iterator s=segs.begin();
      vector<CheckIn::TTransferItem>::const_iterator f=trfer.begin();
      for(;s!=segs.end() && f!=trfer.end();s++,f++)
      {
        seg_no++;
        //возможность сквозной регистрации
        TCkinSetsInfo tckinSets;
        CheckInInterface::CheckTCkinPermit(airline_in,
                                           flt_no_in,
                                           f->operFlt.airp,
                                           f->operFlt.airline,
                                           f->operFlt.flt_no,
                                           tckinSets);
        if (!tckinSets.pr_permit)
          throw "Check-in not permitted";

        if (s->is_edi)
          throw "Flight from the other DCS";

        airline_in=f->operFlt.airline;
        flt_no_in=f->operFlt.flt_no;

        if (s->flts.empty())
          throw "Flight not found";

        if (s->flts.size()>1)
          throw "More than one flight found";

        const TSegInfo &currSeg=*(s->flts.begin());

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
               pnrData.stages[ sCloseWEBCheckIn ] == NoExists )
            throw "Stage of web check-in not found";
        };

        //ищем PNR по номеру
        pnrData.pnr_id=NoExists;
        if (!firstPnrData.pnr_addrs.empty())
        {
          Qry.Clear();
          Qry.SQLText=
            "SELECT crs_pnr.class, "
            "       pnr_addrs.pnr_id, pnr_addrs.airline, pnr_addrs.addr "
            "FROM tlg_binding, crs_pnr, pnr_addrs "
            "WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
            "      tlg_binding.point_id_spp=:point_id AND "
            "      pnr_addrs.pnr_id=crs_pnr.pnr_id AND "
            "      crs_pnr.target=:airp_arv AND "
            "      crs_pnr.subclass=:subclass "
            "ORDER BY pnr_addrs.pnr_id, pnr_addrs.airline";
          Qry.CreateVariable("point_id", otInteger, currSeg.point_dep);
          Qry.CreateVariable("airp_arv", otString, currSeg.airp_arv); //идет проверка совпадения а/п назначения из трансферного маршрута
          Qry.CreateVariable("subclass", otString, f->subclass);      //идет проверка совпадения подкласса из трансферного маршрута
          Qry.Execute();
          int prior_pnr_id=NoExists;
          //по ходу заполняем pnrData.pnr_id, pnrData.cls, pnrData.pnr_addrs
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
              //нашли PNR
              if (pnrData.pnr_id!=NoExists)
              {
                if (pnrData.pnr_id!=Qry.FieldAsInteger("pnr_id")) break; //дубль PNR
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


        //дозаполним поля pnrData
      	pnrData.airp_arv = currSeg.airp_arv;
      	pnrData.subcls = f->subclass;
      	pnrData.point_arv = currSeg.point_arv; //NoExists быть не может - проверено ранее

      	if (!getTripData2(pnrData, false))
      	  throw "Error in 'getTripData2'";

        pnrData.bag_norm = ASTRA::NoExists;

        pnrs.push_back(pnrData);
      };
    }
    catch(const char* error)
    {
      ProgTrace(TRACE5, ">>>> seg_no=%d: %s ", seg_no, error);
      traceTrfer(TRACE5, "trfer", trfer);
      traceTCkinSegs(TRACE5, "segs", segs);
    };
  };
};

void VerifyPNR( int point_id, int pnr_id )
{
	TQuery Qry(&OraSession);
	Qry.SQLText =
    "SELECT point_id_spp "
    " FROM crs_pnr,tlg_binding "
    "WHERE crs_pnr.point_id=tlg_binding.point_id_tlg(+) AND "
    "      crs_pnr.pnr_id=:pnr_id AND "
    "      tlg_binding.point_id_spp(+)=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "pnr_id", otInteger, pnr_id );
	Qry.Execute();
  if ( Qry.Eof )
  	throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );
  if ( Qry.FieldIsNULL( "point_id_spp" ) )
  	throw UserException( "MSG.PASSENGERS.OTHER_FLIGHT" );
}

//перед вызовом должен быть выставлен pnrData.pnr_id!
void GetPNRCodeshare(const TSearchPnrData &pnrData,
                     TTripInfo &operFlt,
                     TTripInfo &pnrMarkFlt,
                     TCodeShareSets &codeshareSets)
{
  operFlt.Clear();
  pnrMarkFlt.Clear();
  codeshareSets.clear();

  //фактический перевозчик
  operFlt.airline=pnrData.airline;
  operFlt.flt_no=pnrData.flt_no;
  operFlt.suffix=pnrData.suffix;
  operFlt.airp=pnrData.airp_dep;
  operFlt.scd_out=pnrData.scd_out;

  //коммерческий рейс PNR
  TMktFlight mktFlt;
  mktFlt.getByPnrId(pnrData.pnr_id);
  if (mktFlt.IsNULL())
    throw EXCEPTIONS::Exception("GetPNRCodeshare: empty TMktFlight (pnr_id=%d)",pnrData.pnr_id);

  pnrMarkFlt.airline=mktFlt.airline;
  pnrMarkFlt.flt_no=mktFlt.flt_no;
  pnrMarkFlt.suffix=mktFlt.suffix;
  pnrMarkFlt.airp=mktFlt.airp_dep;
  pnrMarkFlt.scd_out=mktFlt.scd_date_local;

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
  route.GetRouteAfter( point_id,
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
          //нашли пункт назначения
          i->second=r->point_id;
          break;
        };
      if (r==route.end())
      {
        //не нашли пункт назначения

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

 	for(vector<TPnrInfo>::iterator iPnr=pnr.begin();iPnr!=pnr.end();)
 	{
    for(vector<int>::iterator iPax=iPnr->pax_id.begin();iPax!=iPnr->pax_id.end();)
    {
      //билет
    	if ( !ticket_no.empty() ) { // если задан поиск по билету, то делаем проверку
        QryTicket.SetVariable( "pax_id", *iPax );
        QryTicket.Execute();
        if ( QryTicket.Eof && ticket_no.size() == 14 ) {
        	QryTicket.SetVariable( "ticket_no", ticket_no.substr(0,13) );
        	QryTicket.Execute();
        }
        if (QryTicket.Eof)
        {
          iPax=iPnr->pax_id.erase(iPax); //не подходит pax_id - удаляем
          continue;
        };
      };
      //ProgTrace( TRACE5, "after search ticket, pr_find=%d", pr_find );

      //документ
      if ( !document.empty() ) { // если пред. проверка удовл. и если задан документ, то делаем проверку
      	QryDoc.SetVariable( "pax_id", *iPax );
      	QryDoc.Execute();
      	if (QryDoc.Eof)
      	{
          iPax=iPnr->pax_id.erase(iPax); //не подходит pax_id - удаляем
          continue;
        };
      }
      //ProgTrace( TRACE5, "after search document, pr_find=%d", pr_find );

      iPax++;
    };

    if (iPnr->pax_id.empty()) //ни одного пассажира не прошло проверку в PNR
    {
      iPnr=pnr.erase(iPnr);
      continue;
    };

    //номер PNR (не только проверить но и начитать в структуру TPnrInfo)
    bool pr_find=pnr_addr.empty();
    QryPnrAddr.SetVariable( "pnr_id", iPnr->pnr_id );
  	QryPnrAddr.Execute();
    for ( ; !QryPnrAddr.Eof; QryPnrAddr.Next() )
  	{
  	  // пробег по всем адресам и поиск по условию + заполнение вектора с адресами
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
    pnrInfo.airp_arv=Qry.FieldAsString("target");
    pnrInfo.cl=Qry.FieldAsString("class");
    pnrInfo.subcl=Qry.FieldAsString("subclass");
    pnrInfo.pax_id.push_back(Qry.FieldAsInteger("pax_id"));
    pnrInfo.point_dep.push_back( make_pair(point_dep,NoExists) );
    pnr.push_back(pnrInfo);
  };
};

void findPnr( const TTripInfo &flt,
              const string &surname,
              const string &pnr_addr,
              const string &ticket_no,
              const string &document,
              vector<TPnrInfo> &pnr,
              int pass )
{
  ProgTrace( TRACE5, "surname=%s, pnr_addr=%s, ticket_no=%s, document=%s, pass=%d",
	           surname.c_str(), pnr_addr.c_str(), ticket_no.c_str(), document.c_str(), pass );
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
	  //ищем фактический рейс
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

  	if (point_ids.empty()) return; //не нашли рейса

  	Qry.Clear();
  	Qry.SQLText=
  	  "SELECT crs_pnr.pnr_id, "
	    "       crs_pnr.target, "
	    "       crs_pnr.class, "
	    "       crs_pnr.subclass, "
	    "       crs_pax.pax_id "
      "FROM tlg_binding,crs_pnr,crs_pax "
      "WHERE tlg_binding.point_id_tlg=crs_pnr.point_id AND "
      "      crs_pax.pnr_id=crs_pnr.pnr_id AND "
      "      tlg_binding.point_id_spp=:point_id AND "
      "      system.transliter_equal(crs_pax.surname,:surname)<>0 AND "
      "      crs_pax.pr_del=0";
    Qry.DeclareVariable("point_id", otInteger);
    Qry.CreateVariable("surname", otString, surname);
    for(vector<int>::iterator i=point_ids.begin();i!=point_ids.end();i++)
    {
      Qry.SetVariable("point_id",*i);
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        addPax(Qry,*i,pnr);
      };

      ostringstream descr;
      descr << "point_id_spp=" << *i;
      tracePax(TRACE5, descr.str(), pass, pnr);
      filterPax(pnr_addr, ticket_no, document, pnr);
      tracePax(TRACE5, descr.str(), pass, pnr);
      filterPax(*i, pnr);
      tracePax(TRACE5, descr.str(), pass, pnr);
    };
	};
	if (pass==2)
	{
	  //ищем среди телеграммных рейсов
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

  	if (point_ids_tlg.empty()) return; //не нашли рейса

  	Qry.Clear();
	  Qry.SQLText=
	    "SELECT crs_pnr.pnr_id, "
	    "       crs_pnr.target, "
	    "       crs_pnr.class, "
	    "       crs_pnr.subclass, "
	    "       crs_pax.pax_id "
     	"FROM crs_pnr,crs_pax,pnr_market_flt "
     	"WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
     	"      crs_pnr.pnr_id=pnr_market_flt.pnr_id(+) AND "
     	"      crs_pnr.point_id=:point_id AND "
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
        //цикл по реальным рейсам
        if ( !reqInfo->CheckAirline(PointsQry.FieldAsString("airline")) ||
             !reqInfo->CheckAirp(PointsQry.FieldAsString("airp")) ) continue;
        point_ids_spp.push_back(PointsQry.FieldAsInteger("point_id"));
      };

      if (point_ids_spp.empty()) continue;

      Qry.SetVariable("point_id",*i);
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        for(vector<int>::iterator j=point_ids_spp.begin();j!=point_ids_spp.end();j++)
          addPax(Qry,*j,pnr);
      };

      ostringstream descr;
      descr << "point_id_tlg=" << *i;
      tracePax(TRACE5, descr.str(), pass, pnr);
      filterPax(pnr_addr, ticket_no, document, pnr);
      tracePax(TRACE5, descr.str(), pass, pnr);
      for(vector<int>::iterator j=point_ids_spp.begin();j!=point_ids_spp.end();j++)
        filterPax(*j, pnr);
      tracePax(TRACE5, descr.str(), pass, pnr);
    };

	};
	if (pass==3)
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

	  //ищем среди эл-тов .M из PNL
	  Qry.Clear();
	  Qry.SQLText=
	    "SELECT tlg_trips.point_id, "
	    "       tlg_trips.scd, "
	    "       crs_pnr.pnr_id, "
	    "       crs_pnr.target, "
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
          //цикл по реальным рейсам
          if ( !reqInfo->CheckAirline(PointsQry.FieldAsString("airline")) ||
               !reqInfo->CheckAirp(PointsQry.FieldAsString("airp")) ) continue;
          point_ids_spp.push_back(PointsQry.FieldAsInteger("point_id"));
        };
      };
      if (point_ids_spp.empty()) continue;

      for(vector<int>::iterator j=point_ids_spp.begin();j!=point_ids_spp.end();j++)
        addPax(Qry,*j,pnr);
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
    filterPax(pnr_addr, ticket_no, document, pnr);
    tracePax(TRACE5, "", pass, pnr);
  };
};

/* Пункт посадки в PNL может не совпадать с пунктом посадки в СПП */
void WebRequestsIface::SearchFlt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
/*
<SearchFlt>
  <airline>
  <flt_no>число(1-5 цифр)
  <suffix>
  <scd_out>DD.MM.YYYY HH24:MI:SS
  <surname>  в поиске рейса обязательно нужна инфа по пассажиру (для многопосадочных рейсов)
  <pnr_addr>  номер PNR
  <ticket_no> номер билета
  <document>  номер документа
</SearchFlt>
*/

  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

  ProgTrace(TRACE1,"WebRequestsIface::SearchFlt");
  string surname = NodeAsString( "surname", reqNode, "" );
  string pnr_addr = NodeAsString( "pnr_addr", reqNode, "" );
  string ticket_no = NodeAsString( "ticket_no", reqNode, "" );
  string document = NodeAsString( "document", reqNode, "" );

  TTripInfo flt;
  TElemFmt fmt;
  string value;
  value = NodeAsString( "airline", reqNode, "" );
  value = TrimString( value );
  if ( value.empty() )
   	throw UserException( "MSG.AIRLINE.NOT_SET" );

  flt.airline = ElemToElemId( etAirline, value, fmt );
  if (fmt==efmtUnknown)
  	throw UserException( "MSG.AIRLINE.INVALID",
  		                   LParams()<<LParam("airline", value ) );

  string str_flt_no = NodeAsString( "flt_no", reqNode, "" );
	if ( StrToInt( str_flt_no.c_str(), flt.flt_no ) == EOF ||
		   flt.flt_no > 99999 || flt.flt_no <= 0 )
		throw UserException( "MSG.FLT_NO.INVALID",
			                   LParams()<<LParam("flt_no", str_flt_no) );
	flt.suffix = NodeAsString( "suffix", reqNode, "" );
	flt.suffix = TrimString( flt.suffix );
  if (!flt.suffix.empty())
  {
    flt.suffix = ElemToElemId( etSuffix, flt.suffix, fmt );
    if (fmt==efmtUnknown)
  		throw UserException( "MSG.SUFFIX.INVALID",
  			                   LParams()<<LParam("suffix", flt.suffix) );
  };
  string str_scd_out = NodeAsString( "scd_out", reqNode, "" );
	str_scd_out = TrimString( str_scd_out );
  if ( str_scd_out.empty() )
		throw UserException( "MSG.FLIGHT_DATE.NOT_SET" );
	else
		if ( StrToDateTime( str_scd_out.c_str(), "dd.mm.yyyy hh:nn:ss", flt.scd_out ) == EOF )
			throw UserException( "MSG.FLIGHT_DATE.INVALID",
				                   LParams()<<LParam("scd_out", str_scd_out) );


	vector<TPnrInfo> pnr;
	findPnr(flt, surname, pnr_addr, ticket_no, document, pnr, 1);
  if (pnr.empty())
  {
    findPnr(flt, surname, pnr_addr, ticket_no, document, pnr, 2);
    findPnr(flt, surname, pnr_addr, ticket_no, document, pnr, 3);
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
	
	//дозаполним поля firstPnrData
	firstPnrData.pnr_id = pnr.begin()->pnr_id;
	firstPnrData.airp_arv = pnr.begin()->airp_arv;
	firstPnrData.cls = pnr.begin()->cl;
	firstPnrData.subcls = pnr.begin()->subcl;
	firstPnrData.point_arv = pnr.begin()->point_dep.begin()->second;
	if (firstPnrData.point_arv==NoExists)
	  throw UserException( "MSG.FLIGHT.NOT_FOUND" );
	firstPnrData.pnr_addrs = pnr.begin()->pnr_addrs;
	  
  getTripData2(firstPnrData, true);

  //определение багажной нормы (с учетом возможного кодшера)
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
  pax.final_target=""; //трансфер пока не анализируем
  pax.subcl=firstPnrData.subcls;
  pax.cl=firstPnrData.cls;
  TBagNormInfo norm;

  GetPaxBagNorm(BagNormsQry, use_mixed_norms, pax, norm, false);

  firstPnrData.bag_norm = norm.weight;

  //в этом месте у нас полностью заполненный firstPnrData для первого сегмента
  
  vector<TSearchPnrData> PNRs;
  getTCkinData( firstPnrData, PNRs);

  //в этом месте у нас полностью заполненный PNRs, содержащий как минимум 1 сегмент

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
    NewTextChild( node, "scd_out", DateTimeToStr( pnrData->scd_out_local, ServerFormatDateTimeAsString ) );
    NewTextChild( node, "airp_dep", pnrData->airp_dep );
    NewTextChild( node, "city_dep", pnrData->city_dep );
    if ( pnrData->scd_in != NoExists )
      NewTextChild( node, "scd_in", DateTimeToStr( pnrData->scd_in, ServerFormatDateTimeAsString ) );
    NewTextChild( node, "airp_arv", pnrData->airp_arv );
    NewTextChild( node, "city_arv", pnrData->city_arv );

    TReqInfo *reqInfo = TReqInfo::Instance();
    TStage stage;
    if ( pnrData->act_out != NoExists )
    	NewTextChild( node, "status", "sTakeoff" );
    else
    {
      if ( reqInfo->client_type == ctKiosk )
        stage=pnrData->kiosk_stage;
      else
        stage=pnrData->web_stage;
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
    for(int pass=0; pass<6; pass++)
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
        case 2: stage=sOpenCheckIn;
                stage_name="sOpenCheckIn";
                break;
        case 3: stage=sCloseCheckIn;
                stage_name="sCloseCheckIn";
                break;
        case 4: stage=sOpenBoarding;
                stage_name="sOpenBoarding";
                break;
        case 5: stage=sCloseBoarding;
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
      NewTextChild( semNode, "kiosk_checkin", (int)(pnrData->act_out == NoExists && pnrData->kiosk_stage == sOpenKIOSKCheckIn) );
    else
      NewTextChild( semNode, "web_checkin", (int)(pnrData->act_out == NoExists && pnrData->web_stage == sOpenWEBCheckIn) );
    NewTextChild( semNode, "term_checkin", (int)(pnrData->act_out == NoExists && pnrData->checkin_stage == sOpenCheckIn) );
    NewTextChild( semNode, "term_brd", (int)(pnrData->act_out == NoExists && pnrData->brd_stage == sOpenBoarding) );

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
1. Если кто-то уже начал работать с pnr (агент,разборщик PNL)
2. Если пассажир зарегистрировался, а разборщик PNL ставит признак удаления
*/

struct TWebPax {
  int pax_no; //виртуальный ид. для связи одного и того же пассажира разных сегментов
	int crs_pax_id;
	int crs_pax_id_parent;
	string surname;
	string name;
	TDateTime birth_date;
	string pers_type_extended; //может содержать БГ (CBBG)
	string crs_seat_no;
	string preseat_no;
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
	vector<TFQTItem> fqt_rems;
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

bool is_agent_checkin(const string &pnr_status)
{
  return (//pax.name=="CBBG" ||  надо спросить у Сергиенко
     		   pnr_status=="DG2" ||
     		   pnr_status=="RG2" ||
     		   pnr_status=="ID2" ||
     		   pnr_status=="WL");
};

void getPnr( int pnr_id, vector<TWebPax> &pnr, bool pr_throw )
{
  try
  {
  	pnr.clear();
  	TQuery PaxDocQry(&OraSession);
  	PaxDocQry.SQLText =
  	 "SELECT birth_date "
     "FROM pax_doc "
     "WHERE pax_id=:pax_id AND "
     "      birth_date IS NOT NULL ";
    PaxDocQry.DeclareVariable( "pax_id", otInteger );

  	TQuery CrsDocQry(&OraSession);
  	CrsDocQry.SQLText =
      "SELECT birth_date "
      "FROM crs_pax_doc "
      "WHERE pax_id=:pax_id AND "
      "      birth_date IS NOT NULL "
      "ORDER BY DECODE(type,'P',0,NULL,2,1),DECODE(rem_code,'DOCS',0,1),no";
    CrsDocQry.DeclareVariable( "pax_id", otInteger );

    TQuery CrsTKNQry(&OraSession);
    CrsTKNQry.SQLText =
      "SELECT rem_code AS ticket_rem, "
      "       ticket_no, "
      "       DECODE(rem_code,'TKNE',coupon_no,NULL) AS coupon_no "
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

    TQuery Qry(&OraSession);
  	Qry.SQLText =
  	  "SELECT crs_pax.pax_id AS crs_pax_id, "
      "       crs_inf.pax_id AS crs_pax_id_parent, "
      "       DECODE(pax.pax_id,NULL,crs_pax.surname,pax.surname) AS surname, "
      "       DECODE(pax.pax_id,NULL,crs_pax.name,pax.name) AS name, "
      "       DECODE(pax.pax_id,NULL,crs_pax.pers_type,pax.pers_type) AS pers_type, "
      "       salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS crs_seat_no, "
      "       salons.get_crs_seat_no(crs_pax.pax_id,:protckin_layer,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS preseat_no, "
      "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'one',rownum) AS seat_no, "
      "       DECODE(pax.pax_id,NULL,crs_pax.seats,pax.seats) AS seats, "
      "       DECODE(pax_grp.class,NULL,crs_pnr.class,pax_grp.class) AS class, "
      "       DECODE(pax.subclass,NULL,crs_pnr.subclass,pax.subclass) AS subclass, "
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
    Qry.CreateVariable( "protckin_layer", otString, EncodeCompLayerType(cltProtCkin) );
  	Qry.Execute();
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
    	pax.preseat_no = Qry.FieldAsString( "preseat_no" );
    	pax.crs_seat_no = Qry.FieldAsString( "crs_seat_no" );
    	pax.seats = Qry.FieldAsInteger( "seats" );
    	pax.pass_class = Qry.FieldAsString( "class" );
    	pax.pass_subclass = Qry.FieldAsString( "subclass" );
    	if ( !Qry.FieldIsNULL( "pax_id" ) )
    	{
    	  //пассажир зарегистрирован
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
     		PaxDocQry.SetVariable( "pax_id", pax.pax_id );
     		PaxDocQry.Execute();
     		if ( !PaxDocQry.Eof )
     			pax.birth_date = PaxDocQry.FieldAsDateTime( "birth_date" );
     		pax.pr_eticket = strcmp(Qry.FieldAsString("ticket_rem"),"TKNE")==0;
     		pax.ticket_no	= Qry.FieldAsString("ticket_no");
     		FQTQry.SQLText=PaxFQTQrySQL;
     		FQTQry.SetVariable( "pax_id", pax.pax_id );
     	}
     	else {
    		pax.checkin_status = "not_checked";
     		CrsDocQry.SetVariable( "pax_id", pax.crs_pax_id );
     		CrsDocQry.Execute();
     		if ( !CrsDocQry.Eof )
     			pax.birth_date = CrsDocQry.FieldAsDateTime( "birth_date" );
     		//проверка CBBG (доп место багажа в салоне)
     		/*CrsPaxRemQry.SetVariable( "pax_id", pax.pax_id );
     		CrsPaxRemQry.SetVariable( "rem_code", "CBBG" );
     		CrsPaxRemQry.Execute();
     		if (!CrsPaxRemQry.Eof)*/
     		if (pax.name=="CBBG")
     		  pax.pers_type_extended = "БГ"; //CBBG

        if (is_agent_checkin(Qry.FieldAsString("pnr_status")))
          pax.checkin_status = "agent_checkin";

     		CrsTKNQry.SetVariable( "pax_id", pax.crs_pax_id );
     		CrsTKNQry.Execute();
     		if ( !CrsTKNQry.Eof )
     		{
     		  pax.pr_eticket = strcmp(CrsTKNQry.FieldAsString("ticket_rem"),"TKNE")==0;
     		  pax.ticket_no	= CrsTKNQry.FieldAsString("ticket_no");
     		};
     		FQTQry.SQLText=CrsFQTQrySQL;
     		FQTQry.SetVariable( "pax_id", pax.crs_pax_id );
     	}
     	FQTQry.Execute();
   		for(; !FQTQry.Eof; FQTQry.Next())
   		{
        TFQTItem FQTItem;
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

void IntLoadPnr( const vector< pair<int, int> > &ids, vector< vector<TWebPax> > &pnrs, xmlNodePtr segsNode )
{
  pnrs.clear();
  for(vector< pair<int, int> >::const_iterator i=ids.begin();i!=ids.end();i++)
  {
    int point_id=i->first;
    int pnr_id=i->second;

    try
    {
      vector<TWebPax> pnr;
      getPnr( pnr_id, pnr, pnrs.empty() );
      if (pnrs.begin()!=pnrs.end())
      {
        //фильтруем пассажиров из второго и следующих сегментов
        const vector<TWebPax> &firstPnr=*(pnrs.begin());
        for(vector<TWebPax>::iterator iPax=pnr.begin();iPax!=pnr.end();)
        {
          //удалим дублирование фамилия+имя из pnr
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
            //не нашли соответствующего пассажира из первого сегмента
            iPax=pnr.erase(iPax);
            continue;
          };
          iPax->pax_no=iPaxFirst->pax_no; //проставляем пассажиру соотв. виртуальный ид. из первого сегмента
          iPax++;
        };
      }
      else
      {
        //пассажиров первого сегмента проставим pax_no по порядку
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
      	if ( !iPax->seat_no.empty() )
      		NewTextChild( paxNode, "seat_no", iPax->seat_no );
      	else
      		if ( !iPax->preseat_no.empty() )
      			NewTextChild( paxNode, "seat_no", iPax->preseat_no );
      		else
      			if ( !iPax->crs_seat_no.empty() )
      			  NewTextChild( paxNode, "seat_no", iPax->crs_seat_no );
        NewTextChild( paxNode, "seats", iPax->seats );
       	NewTextChild( paxNode, "checkin_status", iPax->checkin_status );
       	if ( iPax->pr_eticket )
       	  NewTextChild( paxNode, "eticket", "true" );
       	else
       		NewTextChild( paxNode, "eticket", "false" );
       	NewTextChild( paxNode, "ticket_no", iPax->ticket_no );
       	xmlNodePtr fqtsNode = NewTextChild( paxNode, "fqt_rems" );
       	for ( vector<TFQTItem>::const_iterator f=iPax->fqt_rems.begin(); f!=iPax->fqt_rems.end(); f++ )
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
  vector< pair<int, int> > ids;
  for(xmlNodePtr node=segsNode->children; node!=NULL; node=node->next)
  {
    int point_id=NodeAsInteger( "point_id", node );
    int pnr_id=NodeAsInteger( "pnr_id", node );
    getTripData( point_id, true );
    VerifyPNR( point_id, pnr_id );
    ids.push_back( make_pair(point_id, pnr_id) );
  };
  
  vector< vector<TWebPax> > pnrs;
  segsNode = NewTextChild( NewTextChild( resNode, "LoadPnr" ), "segments" );
  IntLoadPnr( ids, pnrs, segsNode );
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
	string seat_no;
	string elem_type;
	int pr_free;
	int pr_CHIN;
	int pax_id;
};

typedef std::vector<TWebPlace> TWebPlaces;

struct TWebPlaceList {
	TWebPlaces places;
	int xcount, ycount;
};

/*
1. Что делать если пассажир имеет спец. подкласс (ремарки MCLS) - Пока выбираем только места с ремарками нужного подкласса.
Что делать , если салон не размечен?
2. Есть группа пассажиров, некоторые уже зарегистрированы, некоторые нет.
*/
void WebRequestsIface::ViewCraft(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

  ProgTrace(TRACE1,"WebRequestsIface::ViewCraft");
  int point_id = NodeAsInteger( "point_id", reqNode );
  int pnr_id = NodeAsInteger( "pnr_id", reqNode );
  string crs_class, crs_subclass;
  vector<TWebPax> pnr;
  getTripData( point_id, true );
  VerifyPNR( point_id, pnr_id );
  getPnr( pnr_id, pnr, true );
  bool pr_CHIN = false;
  for ( vector<TWebPax>::iterator i=pnr.begin(); i!=pnr.end(); i++ ) {
  	if ( !i->pass_class.empty() )
  	  crs_class = i->pass_class;
  	if ( !i->pass_subclass.empty() )
  	  crs_subclass = i->pass_subclass;
  	TPerson p=DecodePerson(i->pers_type_extended.c_str());
  	pr_CHIN=(p==ASTRA::child || p==ASTRA::baby); //среди типов может быть БГ (CBBG) который приравнивается к взрослому
  }
  if ( crs_class.empty() )
  	throw UserException( "MSG.CLASS.NOT_SET" );

  map<string,bool> ispl;
  ImagesInterface::GetisPlaceMap( ispl );
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT airline FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  TSublsRems subcls_rems( Qry.FieldAsString("airline") );
  SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
  Salons.ClName = crs_class;
  Salons.Read();
  // получим признак того, что в салоне есть свободные места с данным подклассом
  map<int, TWebPlaceList> web_salons;
  bool pr_find_free_subcls_place=false;
  string pass_rem;

  subcls_rems.IsSubClsRem( crs_subclass, pass_rem );

  for( vector<TPlaceList*>::iterator placeList = Salons.placelists.begin();
       placeList != Salons.placelists.end(); placeList++ ) {
    TWebPlaceList web_place_list;
    web_place_list.xcount=0;
    web_place_list.ycount=0;
    for ( TPlaces::iterator place = (*placeList)->places.begin();
          place != (*placeList)->places.end(); place++ ) { // пробег по салонам
      if ( !place->visible )
       continue;
      TWebPlace wp;
      wp.x = place->x;
      wp.y = place->y;
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
     	if ( place->isplace && !place->clname.empty() && place->clname == crs_class ) {
     		bool pr_first = true;
     		for( std::vector<TPlaceLayer>::iterator l=place->layers.begin(); l!=place->layers.end(); l++ ) { // сортировка по приоритета
     			if ( pr_first &&
     				   l->layer_type != cltUncomfort &&
     				   l->layer_type != cltSmoke &&
     				   l->layer_type != cltUnknown ) {
     				pr_first = false;
     				wp.pr_free = ( ( l->layer_type == cltPNLCkin ||
     				                 l->layer_type == cltProtCkin ) && isOwnerFreePlace( l->pax_id, pnr ) );
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

     	  wp.pr_free = ( wp.pr_free || pr_first ); // 0 - занято, 1 - свободно, 2 - частично занято

        if ( wp.pr_free ) {
        	if ( !pass_rem.empty() ) {
        	  wp.pr_free = 2; // свободно без учета подкласса
            for ( vector<TRem>::iterator i=place->rems.begin(); i!=place->rems.end(); i++ ) {
            	if ( i->rem == pass_rem ) {
            		if ( !i->pr_denial ) {
            		  wp.pr_free = 3;  // свободно с учетом подкласса
            		  pr_find_free_subcls_place=true;
            		}
            		break;
            	}
            }
          }
          else { // пассажир без подкласса
          	for ( vector<TRem>::iterator i=place->rems.begin(); i!=place->rems.end(); i++ ) {
          		if ( isREM_SUBCLS( i->rem ) ) {
          			wp.pr_free = 0;
          			break;
          		}
            }
          }
        }
        if ( pr_CHIN ) { // встречаются в группе пассажиры с детьми
        	if ( place->elem_type == "А" ) { // место у аварийного выхода
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
      int status;
      switch( wp->pr_free ) {
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
      if ( status == 0 && wp->pr_CHIN ) {
      	status = 2;
      }
      NewTextChild( placeNode, "status", status );
      if ( wp->pax_id != NoExists )
      	NewTextChild( placeNode, "pax_id", wp->pax_id );
    }
  }
}

void CreateEmulXMLDoc(xmlNodePtr reqNode, XMLDoc &emulDoc)
{
  emulDoc.set("UTF-8","term");
  if (emulDoc.docPtr()==NULL)
    throw EXCEPTIONS::Exception("CreateEmulXMLDoc: CreateXMLDoc failed");
  CopyNode(NodeAsNode("/term",emulDoc.docPtr()),
           NodeAsNode("/term/query",reqNode->doc), true); //копируем полностью тег query
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
    CopyNode(destNode, srcNode, true); //копируем полностью XML
};

void CreateEmulRems(xmlNodePtr paxNode, TQuery &RemQry, const vector<string> &fqtv_rems)
{
  xmlNodePtr remsNode=NewTextChild(paxNode,"rems");
  for(;!RemQry.Eof;RemQry.Next())
  {
    if (strcmp(RemQry.FieldAsString("rem_code"),"FQTV")==0) continue;
    xmlNodePtr remNode=NewTextChild(remsNode,"rem");
    NewTextChild(remNode,"rem_code",RemQry.FieldAsString("rem_code"));
    NewTextChild(remNode,"rem_text",RemQry.FieldAsString("rem"));
  };
  //добавим переданные fqtv_rems
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
  int crs_pnr_tid;
	int crs_pax_tid;
	int pax_grp_tid;
	int pax_tid;
  TWebPaxFromReq() {
		crs_pax_id = NoExists;
		fqt_rems_present = false;
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
  string preseat_no;
  string seat_type;
  int seats;
  string eticket;
  string ticket;
  string document;
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
  list<TWebPaxForChng> paxForChng;
  list<TWebPaxForCkin> paxForCkin;
  TWebPnrForSave() {
    pnr_id = NoExists;
  };
};



void VerifyPax(vector< pair<int, TWebPnrForSave > > &segs, XMLDoc &emulDocHeader,
               XMLDoc &emulCkinDoc, map<int,XMLDoc> &emulChngDocs, vector< pair<int, int> > &ids)
{
  ids.clear();
  
  if (segs.empty()) return;
  
  TReqInfo *reqInfo = TReqInfo::Instance();

  //первым делом проверяем, что незарегистрированные пассажиры совпадают по кол-ву для каждого сегмента
  //на последних сегментах кол-во незарегистрированных пассажиров м.б. нулевым
  int prevNotCheckedCount=NoExists;
  int seg_no=1;
  for(vector< pair<int, TWebPnrForSave > >::const_iterator s=segs.begin(); s!=segs.end(); s++, seg_no++)
  {
    int currNotCheckedCount=0;
    for(vector<TWebPaxFromReq>::const_iterator iPax=s->second.paxFromReq.begin(); iPax!=s->second.paxFromReq.end(); iPax++)
    {
      if (iPax->pax_grp_tid==NoExists || iPax->pax_tid==NoExists)
        //пассажир не зарегистрирован
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
      "       salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, "
      "       salons.get_crs_seat_no(crs_pax.pax_id,:protckin_layer,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS preseat_no, "
      "       crs_pax.seat_type, "
      "       crs_pax.seats, "
      "       crs_pnr.pnr_id, "
      "       report.get_PSPT(crs_pax.pax_id) AS document, "
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
  
  const char* PaxRemQrySQL=
      "SELECT rem_code,rem FROM pax_rem "
      "WHERE pax_id=:pax_id AND rem_code NOT IN ('FQTV')";

  const char* CrsPaxRemQrySQL=
      "SELECT rem_code,rem FROM crs_pax_rem "
      "WHERE pax_id=:pax_id AND rem_code NOT IN ('FQTV')";
  TQuery Qry(&OraSession);

  TQuery RemQry(&OraSession);
  RemQry.DeclareVariable("pax_id",otInteger);
  
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
        int pnr_id=NoExists; //попробуем определить из секции passengers
        int adult_count=0, without_seat_count=0;
        for(vector<TWebPaxFromReq>::const_iterator iPax=s->second.paxFromReq.begin(); iPax!=s->second.paxFromReq.end(); iPax++)
        {
          try
          {
            bool not_checked=(iPax->pax_grp_tid==NoExists || iPax->pax_tid==NoExists);

            Qry.Clear();
            try
            {
              if (not_checked)
              {
                //пассажир не зарегистрирован
                Qry.SQLText=CrsPaxQrySQL;
                Qry.CreateVariable("protckin_layer", otString, EncodeCompLayerType(ASTRA::cltProtCkin) );
                Qry.CreateVariable("crs_pax_id", otInteger, iPax->crs_pax_id);
                Qry.Execute();
                if (Qry.Eof)
                  throw UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
                if (Qry.FieldAsInteger("checked")!=0)
                  throw UserException("MSG.PASSENGER.CHECKED.REFRESH_DATA");
                if (iPax->crs_pnr_tid!=Qry.FieldAsInteger("crs_pnr_tid") ||
                    iPax->crs_pax_tid!=Qry.FieldAsInteger("crs_pax_tid"))
                  throw UserException("MSG.PASSENGER.CHANGED.REFRESH_DATA");

                if (is_agent_checkin(Qry.FieldAsString("pnr_status")))
                  throw UserException("MSG.PASSENGER.CHECKIN_DENIAL");
              }
              else
              {
                //пассажир зарегистрирован
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
              //первый пассажир
              pnr_id=Qry.FieldAsInteger("pnr_id");
              //проверим, что данное PNR привязано к рейсу
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
              pax.preseat_no = Qry.FieldAsString("preseat_no");
              pax.seat_type = Qry.FieldAsString("seat_type");
              pax.seats = Qry.FieldAsInteger("seats");
              pax.eticket = Qry.FieldAsString("eticket");
              pax.ticket = Qry.FieldAsString("ticket");
              pax.document = Qry.FieldAsString("document");
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
        //проверим лишь соответствие point_id и pnr_id
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
  //в любом случае дочитываем сквозной маршрут
  try
  {
    //дозаполним поля firstPnrData
  	firstPnrData.pnr_id = segs.begin()->second.pnr_id;

    Qry.Clear();
    Qry.SQLText=
      "SELECT target, subclass, class "
      "FROM crs_pnr "
      "WHERE pnr_id=:pnr_id";
    Qry.CreateVariable("pnr_id", otInteger, firstPnrData.pnr_id);
    Qry.Execute();
    if (Qry.Eof)
      throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );

    firstPnrData.airp_arv = Qry.FieldAsString("target");
  	firstPnrData.cls = Qry.FieldAsString("class");
  	firstPnrData.subcls = Qry.FieldAsString("subclass");

    TTripRoute route; //маршрут рейса
    route.GetRouteAfter( firstPnrData.point_id,
                         firstPnrData.point_num,
                         firstPnrData.first_point,
                         firstPnrData.pr_tranzit,
                         trtNotCurrent,
                         trtNotCancelled );

    vector<TTripRouteItem>::iterator i=route.begin();
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
  //проверка всего сквозного маршрута на совпадение point_id, pnr_id и соответствие фамилий/имен
  vector<TSearchPnrData>::const_iterator iPnrData=PNRs.begin();
  seg_no=1;
  for(vector< pair<int, TWebPnrForSave > >::iterator s=segs.begin(); s!=segs.end(); s++, iPnrData++, seg_no++)
  {
    try
    {
      if (iPnrData!=PNRs.end() &&
          (!s->second.paxForCkin.empty() || !s->second.paxForChng.empty())) //типа есть пассажиры
      {
        //проверяем на сегменте вылет рейса и состояние соответствующего этапа
        if ( iPnrData->act_out != NoExists )
  	      throw UserException( "MSG.FLIGHT.TAKEOFF" );

  	    if ( reqInfo->client_type == ctKiosk )
        {
          if (!(iPnrData->kiosk_stage == sOpenKIOSKCheckIn ||
                iPnrData->kiosk_stage == sNoActive && s!=segs.begin())) //для сквозных сегментов регистрация может быть еще не открыта
          {
            if (iPnrData->kiosk_stage == sNoActive)
              throw UserException( "MSG.CHECKIN.NOT_OPEN" );
            else
              throw UserException( "MSG.CHECKIN.CLOSED_OR_DENIAL" );
          };
        }
        else
        {
          if (!(iPnrData->web_stage == sOpenWEBCheckIn ||
                iPnrData->web_stage == sNoActive && s!=segs.begin())) //для сквозных сегментов регистрация может быть еще не открыта
            if (iPnrData->web_stage == sNoActive)
              throw UserException( "MSG.CHECKIN.NOT_OPEN" );
            else
              throw UserException( "MSG.CHECKIN.CLOSED_OR_DENIAL" );
        };
      };

      if (s==segs.begin()) continue; //пропускаем первый сегмент

      TWebPnrForSave &currPnr=s->second;
      if (iPnrData==PNRs.end()) //лишние сегменты в запросе на регистрацию
        throw UserException("MSG.THROUGH_ROUTE_CHANGED.REFRESH_DATA");
      if (iPnrData->point_id!=s->first) //другой рейс на сквозном сегменте
        throw UserException("MSG.THROUGH_ROUTE_CHANGED.REFRESH_DATA");
      if (iPnrData->pnr_id!=currPnr.pnr_id) //другой pnr_id на сквозном сегменте
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

          currPnr.paxForCkin.splice(currPnr.paxForCkin.end(),currPnr.paxForCkin,iPax2,iPax3); //перемещаем найденного пассажира в конец
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

  //составляем XML-запрос
  iPnrData=PNRs.begin();
  seg_no=1;
  for(vector< pair<int, TWebPnrForSave > >::const_iterator s=segs.begin(); s!=segs.end(); s++, iPnrData++, seg_no++)
  {
    try
    {
      if (iPnrData==PNRs.end()) //лишние сегменты в запросе на регистрацию
        throw EXCEPTIONS::Exception("VerifyPax: iPnrData==PNRs.end() (seg_no=%d)", seg_no);

      const TWebPnrForSave &currPnr=s->second;
      //пассажиры для регистрации
      if (!currPnr.paxForCkin.empty())
      {
        if (emulCkinDoc.docPtr()==NULL)
        {
          CopyEmulXMLDoc(emulDocHeader, emulCkinDoc);
          xmlNodePtr emulCkinNode=NodeAsNode("/term/query",emulCkinDoc.docPtr());
          emulCkinNode=NewTextChild(emulCkinNode,"TCkinSavePax");
        	NewTextChild(emulCkinNode,"transfer"); //пустой тег - трансфера нет
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
        NewTextChild(node,"scd",DateTimeToStr(pnrMarkFlt.scd_out));  //локальная дата
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
            NewTextChild(paxNode,"preseat_no",iPaxForCkin->preseat_no);
            NewTextChild(paxNode,"seat_type",iPaxForCkin->seat_type);
            NewTextChild(paxNode,"seats",iPaxForCkin->seats);
            //обработка билетов
            string ticket_no;
            if (!iPaxForCkin->eticket.empty())
            {
              //билет TKNE
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
            NewTextChild(paxNode,"document",iPaxForCkin->document);
            NewTextChild(paxNode,"subclass",iPaxForCkin->subclass);
            NewTextChild(paxNode,"transfer"); //пустой тег - трансфера нет

            //ремарки
            RemQry.SQLText=CrsPaxRemQrySQL;
            RemQry.SetVariable("pax_id",iPaxForCkin->crs_pax_id);
            RemQry.Execute();
            CreateEmulRems(paxNode, RemQry, iPaxFromReq->fqt_rems);

            NewTextChild(paxNode,"norms"); //пустой тег - норм нет
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

      //пассажиры для изменения
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
          //пассажир зарегистрирован
          if (!iPaxFromReq->seat_no.empty() && iPaxForChng->seats > 0)
          {
          	string prior_xname, prior_yname;
          	string curr_xname, curr_yname;
          	// надо номализовать старое и новое место, сравнить их, если изменены, то вызвать пересадку
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
          if (iPaxFromReq->fqt_rems_present) //тег <fqt_rems> пришел
          {
            vector<string> prior_fqt_rems;
            //читаем уже записанные ремарки FQTV
            RemQry.SQLText="SELECT rem FROM pax_rem WHERE pax_id=:pax_id AND rem_code='FQTV'";
            RemQry.SetVariable("pax_id", iPaxForChng->crs_pax_id);
            RemQry.Execute();
            for(;!RemQry.Eof;RemQry.Next()) prior_fqt_rems.push_back(RemQry.FieldAsString("rem"));
            //сортируем и сравниваем
            sort(prior_fqt_rems.begin(),prior_fqt_rems.end());
            if (prior_fqt_rems.size()==iPaxFromReq->fqt_rems.size())
              FQTRemUpdatesPending=!equal(prior_fqt_rems.begin(),prior_fqt_rems.end(),
                                          iPaxFromReq->fqt_rems.begin());
            else
              FQTRemUpdatesPending=true;
          };
          if (FQTRemUpdatesPending)
          {
            //придется вызвать транзакцию на запись изменений
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
                NewTextChild(emulChngNode,"bag_refuse","А");
              else
                NewTextChild(emulChngNode,"bag_refuse");
            };
            xmlNodePtr paxsNode=NodeAsNode("/term/query/TCkinSavePax/segments/segment/passengers",emulChngDoc.docPtr());

            xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
            NewTextChild(paxNode,"pax_id",iPaxForChng->crs_pax_id);
            NewTextChild(paxNode,"surname",iPaxForChng->surname);
            NewTextChild(paxNode,"name",iPaxForChng->name);
            NewTextChild(paxNode,"tid",pax_tid);

            //ремарки
            RemQry.SQLText=PaxRemQrySQL;
            RemQry.SetVariable("pax_id",iPaxForChng->crs_pax_id);
            RemQry.Execute();
            CreateEmulRems(paxNode, RemQry, iPaxFromReq->fqt_rems);
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
  
  //возвращаем ids
  for(iPnrData=PNRs.begin();iPnrData!=PNRs.end();iPnrData++)
    ids.push_back( make_pair(iPnrData->point_id, iPnrData->pnr_id) );
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
        pax.fqt_rems_present=(fqtNode!=NULL); //если тег <fqt_rems> пришел, то изменяем и перезаписываем ремарки FQTV
        if (fqtNode!=NULL)
        {
          //читаем пришедшие ремарки
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
        
        xmlNodePtr tidsNode=NodeAsNode("tids", paxNode)->children;
        pax.crs_pnr_tid=NodeAsIntegerFast("crs_pnr_tid",tidsNode);
        pax.crs_pax_tid=NodeAsIntegerFast("crs_pax_tid",tidsNode);
        pax.pax_grp_tid=NodeAsIntegerFast("pax_grp_tid",tidsNode,NoExists);
        pax.pax_tid=NodeAsIntegerFast("pax_tid",tidsNode,NoExists);
        
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
  vector< pair<int, int> > ids;
  VerifyPax(segs, emulDocHeader, emulCkinDoc, emulChngDocs, ids);

  bool result=true;
  if (emulCkinDoc.docPtr()!=NULL) //регистрация новой группы
  {
    xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulCkinDoc.docPtr())->children;
    if (emulReqNode==NULL)
      throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: emulReqNode=NULL");
    if (!CheckInInterface::SavePax(reqNode, emulReqNode, ediResNode, resNode)) result=false;
  };
  if (result)
  {
    for(map<int,XMLDoc>::iterator i=emulChngDocs.begin();i!=emulChngDocs.end();i++)
    {
      XMLDoc &emulChngDoc=i->second;
      xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulChngDoc.docPtr())->children;
      if (emulReqNode==NULL)
        throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: emulReqNode=NULL");
      if (!CheckInInterface::SavePax(reqNode, emulReqNode, ediResNode, resNode))
      {
        result=false;
        break;
      };
    };
  };

  if (result)
  {
    vector< vector<TWebPax> > pnrs;
    xmlNodePtr segsNode = NewTextChild( NewTextChild( resNode, "SavePax" ), "segments" );
    IntLoadPnr( ids, pnrs, segsNode );
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

void WebRequestsIface::GetBPTags(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

	ProgTrace(TRACE1,"WebRequestsIface::GetBPTags");
	int pax_id = NodeAsInteger( "pax_id", reqNode );
	xmlNodePtr node = NodeAsNode( "tids", reqNode );
	int crs_pnr_tid = NodeAsInteger( "crs_pnr_tid", node );
	int crs_pax_tid = NodeAsInteger( "crs_pax_tid", node );
	int pax_grp_tid = NodeAsInteger( "pax_grp_tid", node );
	int pax_tid = NodeAsInteger( "pax_tid", node );
	verifyPaxTids( pax_id, crs_pnr_tid, crs_pax_tid, pax_grp_tid, pax_tid );
	TQuery Qry(&OraSession);
	Qry.SQLText =
	 "SELECT pax_grp.grp_id, pax_grp.point_dep "
	 " FROM pax_grp, pax "
	 "WHERE pax_id=:pax_id AND pax.grp_id=pax_grp.grp_id";
	Qry.CreateVariable( "pax_id", otInteger, pax_id );
	Qry.Execute();
	if ( Qry.Eof )
		throw UserException( "MSG.PASSENGER.NOT_FOUND" );
	int point_id = Qry.FieldAsInteger( "point_dep" );
	PrintDataParser parser( Qry.FieldAsInteger( "grp_id" ), pax_id, 0, NULL );
	vector<string> tags;
	BPTags::Instance()->getFields( tags );
	node = NewTextChild( resNode, "GetBPTags" );
    for ( vector<string>::iterator i=tags.begin(); i!=tags.end(); i++ ) {
        for(int j = 0; j < 2; j++) {
            string value = parser.pts.get_tag(*i, ServerFormatDateTimeAsString, (j == 0 ? "R" : "E"));
            NewTextChild( node, (*i + (j == 0 ? "" : "_lat")), value );
            ProgTrace( TRACE5, "field name=%s, value=%s", (*i + (j == 0 ? "" : "_lat")).c_str(), value.c_str() );
        }
    }
	Qry.Clear();
	Qry.SQLText =
    "SELECT stations.name FROM stations,trip_stations "
    " WHERE point_id=:point_id AND "
    "       stations.desk=trip_stations.desk AND "
    "       stations.work_mode=trip_stations.work_mode AND "
    "       stations.work_mode=:work_mode";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "work_mode", otString, "П" );
	Qry.Execute();
	if ( !Qry.Eof ) {
		string gate = Qry.FieldAsString( "name" );
		Qry.Next();
		if ( Qry.Eof )
			NewTextChild( node, "gate", gate );
	}
}

} //end namespace AstraWeb

