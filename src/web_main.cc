#include <arpa/inet.h>
#include <memory.h>
#include <string>
#define NICKNAME "DJEK"
#include "serverlib/test.h"
#include "serverlib/perfom.h"
#include "serverlib/ourtime.h"
#include "serverlib/query_runner.h"
#include "jxtlib/xmllibcpp.h"
#include "jxtlib/xml_stuff.h"
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

namespace AstraWeb
{

using namespace std;
//using namespace EXCEPTIONS;
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
        new_body=new_body.substr(0,pos+sss.size())+" id='"+"WEB"/*client.client_type*/+"' screen='AIR.EXE' opr='"+CP866toUTF8(client.opr)+"'"+new_body.substr(pos+sss.size());
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
struct TPnrAddr {
	string airline;
	string addr;
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

bool getTripData( int point_id, TSearchPnrData &SearchPnrData, bool pr_throw )
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
		if ( pr_throw )
		  throw UserException( "MSG.FLIGHT.NOT_FOUND" );
		else
			return false;

	if ( Qry.FieldAsInteger( "pr_del" ) == -1 )
		if ( pr_throw )
		  throw UserException( "MSG.FLIGHT.DELETED" );
		else
			return false;

  if ( !reqInfo->CheckAirline(Qry.FieldAsString("airline")) ||
       !reqInfo->CheckAirp(Qry.FieldAsString("airp")) )
    if ( pr_throw )
      throw UserException( "MSG.FLIGHT.ACCESS_DENIED" );
		else
			return false;

	if ( Qry.FieldAsInteger( "pr_del" ) == 1 )
		if ( pr_throw )
		  throw UserException( "MSG.FLIGHT.CANCELED" );
		else
			return false;

	if ( Qry.FieldAsInteger( "pr_reg" ) == 0 )
		if ( pr_throw )
		  throw UserException( "MSG.FLIGHT.CHECKIN_CANCELED" );
		else
			return false;

	if ( Qry.FieldIsNULL( "act_out" ) )
		SearchPnrData.act_out = NoExists;
	else
		SearchPnrData.act_out = Qry.FieldAsDateTime( "act_out" );

	TTripStages tripStages( point_id );
	if ( reqInfo->client_type == ctWeb )
	  SearchPnrData.web_stage = tripStages.getStage( stWEB );
	else
		SearchPnrData.web_stage = tripStages.getStage( stKIOSK );
	SearchPnrData.checkin_stage = tripStages.getStage( stCheckIn );
	SearchPnrData.brd_stage = tripStages.getStage( stBoarding );

	ProgTrace( TRACE5, "web_stage=%d, checkin_stage=%d, brd_stage=%d",
	           (int)SearchPnrData.web_stage, (int)SearchPnrData.checkin_stage, (int)SearchPnrData.brd_stage );
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
	SearchPnrData.stages.clear();

	TStagesRules *sr = TStagesRules::Instance();
	TCkinClients ckin_clients;
	TTripStages::ReadCkinClients( point_id, ckin_clients );
	if ( reqInfo->client_type == ctWeb ) {
    if ( sr->isClientStage( (int)sOpenWEBCheckIn ) && !sr->canClientStage( ckin_clients, (int)sOpenWEBCheckIn ) )
    	SearchPnrData.stages.insert( make_pair(sOpenWEBCheckIn, NoExists) );
    else
    	SearchPnrData.stages.insert( make_pair(sOpenWEBCheckIn, UTCToLocal( tripStages.time(sOpenWEBCheckIn), region ) ) );
    if ( sr->isClientStage( (int)sCloseWEBCheckIn ) && !sr->canClientStage( ckin_clients, (int)sCloseWEBCheckIn ) )
    	SearchPnrData.stages.insert( make_pair(sCloseWEBCheckIn, NoExists) );
    else
    	SearchPnrData.stages.insert( make_pair(sCloseWEBCheckIn, UTCToLocal( tripStages.time(sCloseWEBCheckIn), region ) ) );
  };
  if  ( reqInfo->client_type == ctKiosk ) {
    if ( sr->isClientStage( (int)sOpenKIOSKCheckIn ) && !sr->canClientStage( ckin_clients, (int)sOpenKIOSKCheckIn ) )
    	SearchPnrData.stages.insert( make_pair(sOpenWEBCheckIn, NoExists) );
    else
    	SearchPnrData.stages.insert( make_pair(sOpenWEBCheckIn, UTCToLocal( tripStages.time(sOpenKIOSKCheckIn), region ) ) );
    if ( sr->isClientStage( (int)sCloseKIOSKCheckIn ) && !sr->canClientStage( ckin_clients, (int)sCloseKIOSKCheckIn ) )
    	SearchPnrData.stages.insert( make_pair(sCloseWEBCheckIn, NoExists) );
    else
    	SearchPnrData.stages.insert( make_pair(sCloseWEBCheckIn, UTCToLocal( tripStages.time(sCloseKIOSKCheckIn), region ) ) );
  }
  SearchPnrData.stages.insert( make_pair(sOpenCheckIn, UTCToLocal( tripStages.time(sOpenCheckIn), region ) ) );
	SearchPnrData.stages.insert( make_pair(sCloseCheckIn, UTCToLocal( tripStages.time(sCloseCheckIn), region ) ) );
	SearchPnrData.stages.insert( make_pair(sOpenBoarding, UTCToLocal( tripStages.time(sOpenBoarding), region ) ) );
	SearchPnrData.stages.insert( make_pair(sCloseBoarding, UTCToLocal( tripStages.time(sCloseBoarding), region ) ) );
	SearchPnrData.point_id = point_id;
	SearchPnrData.point_num = Qry.FieldAsInteger("point_num");
	SearchPnrData.first_point = Qry.FieldAsInteger("first_point");
	SearchPnrData.pr_tranzit = Qry.FieldAsInteger("pr_tranzit")!=0;

	TTripInfo operFlt(Qry);
	GetMktFlights(operFlt, SearchPnrData.mark_flights);
	return true;
}


bool getTripData( int point_id, bool pr_throw )
{
	TSearchPnrData SearchPnrData;
	return getTripData( point_id, SearchPnrData, pr_throw );
}

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

  string pnr_addr_lat;
	if ( !pnr_addr.empty() ) {
	  pnr_addr_lat = convert_pnr_addr( pnr_addr, true );
	  ProgTrace( TRACE5, "convert_pnr_addr=%s", pnr_addr_lat.c_str() );
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
  		TPnrAddr addr;
  		addr.airline = QryPnrAddr.FieldAsString( "airline" );
  		addr.addr = QryPnrAddr.FieldAsString( "addr" );
  		iPnr->pnr_addrs.push_back( addr );

  		if ( !pnr_addr.empty() && pnr_addr_lat == convert_pnr_addr( addr.addr, true ) )
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

	TSearchPnrData SearchPnrData;
	getTripData(pnr.begin()->point_dep.begin()->first, SearchPnrData, true);

	//дозаполним поля SearchPnrData
	SearchPnrData.pnr_id = pnr.begin()->pnr_id;
	SearchPnrData.airp_arv = pnr.begin()->airp_arv;
	TBaseTable &baseairps = base_tables.get( "airps" );
	SearchPnrData.city_arv = ((TAirpsRow&)baseairps.get_row( "code", SearchPnrData.airp_arv, true )).city;
	SearchPnrData.cls = pnr.begin()->cl;
	SearchPnrData.subcls = pnr.begin()->subcl;
	SearchPnrData.pnr_addrs = pnr.begin()->pnr_addrs;
	SearchPnrData.point_arv = pnr.begin()->point_dep.begin()->second;
	if (SearchPnrData.point_arv==NoExists)
	  throw UserException( "MSG.FLIGHT.NOT_FOUND" );

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

  //определение багажной нормы (с учетом возможного кодшера)
  TTripInfo operFlt,pnrMarkFlt;
  TCodeShareSets codeshareSets;
  GetPNRCodeshare(SearchPnrData, operFlt, pnrMarkFlt, codeshareSets);

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
  BagNormsQry.CreateVariable("point_id",otInteger,SearchPnrData.point_id);
  BagNormsQry.CreateVariable("norm_type",otString,EncodeBagNormType(bntFreeExcess));
  BagNormsQry.Execute();

  bool use_mixed_norms=GetTripSets(tsMixedNorms,operFlt);
  TPaxInfo pax;
  pax.pax_cat="";
  pax.target=SearchPnrData.city_arv;
  pax.final_target=""; //трансфер пока не анализируем
  pax.subcl=SearchPnrData.subcls;
  pax.cl=SearchPnrData.cls;
  TBagNormInfo norm;

  GetPaxBagNorm(BagNormsQry, use_mixed_norms, pax, norm, false);

  SearchPnrData.bag_norm = norm.weight;

  //в этом месте у нас полностью заполненный SearchPnrData


  xmlNodePtr node = NewTextChild( NewTextChild( NewTextChild( resNode, "SearchFlt" ), "segments"), "segment");
  NewTextChild( node, "point_id", SearchPnrData.point_id );
  NewTextChild( node, "airline", SearchPnrData.airline );
  NewTextChild( node, "flt_no", SearchPnrData.flt_no );
  if ( !SearchPnrData.suffix.empty() )
    NewTextChild( node, "suffix", SearchPnrData.suffix );
  NewTextChild( node, "craft", SearchPnrData.craft );
  NewTextChild( node, "scd_out", DateTimeToStr( SearchPnrData.scd_out_local, ServerFormatDateTimeAsString ) );
  NewTextChild( node, "airp_dep", SearchPnrData.airp_dep );
  NewTextChild( node, "city_dep", SearchPnrData.city_dep );
  if ( SearchPnrData.scd_in != NoExists )
    NewTextChild( node, "scd_in", DateTimeToStr( SearchPnrData.scd_in, ServerFormatDateTimeAsString ) );
  NewTextChild( node, "airp_arv", SearchPnrData.airp_arv );
  NewTextChild( node, "city_arv", SearchPnrData.city_arv );
  if ( SearchPnrData.act_out != NoExists )
  	NewTextChild( node, "status", "sTakeoff" );
  else
    switch ( SearchPnrData.web_stage ) {
    	case sNoActive:
    		NewTextChild( node, "status", "sNoActive" );
    		break;
    	case sOpenWEBCheckIn:
    	case sOpenKIOSKCheckIn:
    		NewTextChild( node, "status", "sOpenWEBCheckIn" );
    		break;
    	case sCloseWEBCheckIn:
      case sCloseKIOSKCheckIn:
    		NewTextChild( node, "status", "sCloseWEBCheckIn" );
    		break;
    	case sTakeoff:
    		NewTextChild( node, "status", "sTakeoff" );
    		break;
 	  	default:;
    };

  xmlNodePtr stagesNode = NewTextChild( node, "stages" );
  xmlNodePtr stageNode;
  if ( SearchPnrData.stages[ sOpenWEBCheckIn ] != NoExists )
  	stageNode = NewTextChild( stagesNode, "stage", DateTimeToStr( SearchPnrData.stages[ sOpenWEBCheckIn ], ServerFormatDateTimeAsString ) );
  else
  	stageNode = NewTextChild( stagesNode, "stage" );
  SetProp( stageNode, "type", "sOpenWEBCheckIn" );
  if ( SearchPnrData.stages[ sCloseWEBCheckIn ] != NoExists )
  	stageNode = NewTextChild( stagesNode, "stage", DateTimeToStr( SearchPnrData.stages[ sCloseWEBCheckIn ], ServerFormatDateTimeAsString ) );
  else
  	stageNode = NewTextChild( stagesNode, "stage" );
  SetProp( stageNode, "type", "sCloseWEBCheckIn" );
  SetProp( NewTextChild( stagesNode, "stage", DateTimeToStr( SearchPnrData.stages[ sOpenCheckIn ], ServerFormatDateTimeAsString ) ),
           "type", "sOpenCheckIn" );
  SetProp( NewTextChild( stagesNode, "stage", DateTimeToStr( SearchPnrData.stages[ sCloseCheckIn ], ServerFormatDateTimeAsString ) ),
           "type", "sCloseCheckIn" );
  SetProp( NewTextChild( stagesNode, "stage", DateTimeToStr( SearchPnrData.stages[ sOpenBoarding ], ServerFormatDateTimeAsString ) ),
           "type", "sOpenBoarding" );
  SetProp( NewTextChild( stagesNode, "stage", DateTimeToStr( SearchPnrData.stages[ sCloseBoarding ], ServerFormatDateTimeAsString ) ),
           "type", "sCloseBoarding" );
  xmlNodePtr semNode = NewTextChild( node, "semaphors" );
  NewTextChild( semNode, "web_checkin", SearchPnrData.act_out == NoExists && (SearchPnrData.web_stage == sOpenWEBCheckIn ||
                                                                              SearchPnrData.web_stage == sOpenKIOSKCheckIn ) );
  NewTextChild( semNode, "term_checkin", SearchPnrData.act_out == NoExists && SearchPnrData.checkin_stage == sOpenCheckIn );
  NewTextChild( semNode, "term_brd", SearchPnrData.act_out == NoExists && SearchPnrData.brd_stage == sOpenBoarding );

  xmlNodePtr fltsNode = NewTextChild( node, "mark_flights" );
  for(vector<TTripInfo>::iterator m=SearchPnrData.mark_flights.begin();
                                  m!=SearchPnrData.mark_flights.end();m++)
  {
    xmlNodePtr fltNode=NewTextChild( fltsNode, "flight" );
    NewTextChild( fltNode, "airline", m->airline );
    NewTextChild( fltNode, "flt_no", m->flt_no );
    NewTextChild( fltNode, "suffix", m->suffix );
  };

  NewTextChild( node, "pnr_id", SearchPnrData.pnr_id );
  NewTextChild( node, "subclass", SearchPnrData.subcls );
  if (SearchPnrData.bag_norm!=ASTRA::NoExists)
    NewTextChild( node, "bag_norm", SearchPnrData.bag_norm );
  else
  	NewTextChild( node, "bag_norm" );
  if ( !SearchPnrData.pnr_addrs.empty() ) {
  	node = NewTextChild( node, "pnr_addrs" );
  	for ( vector<TPnrAddr>::iterator i=SearchPnrData.pnr_addrs.begin(); i!=SearchPnrData.pnr_addrs.end(); i++ ) {
      xmlNodePtr addrNode = NewTextChild( node, "pnr_addr" );
      NewTextChild( addrNode, "airline", i->airline );
      NewTextChild( addrNode, "addr", i->addr );
  	}
  }
}

/*
1. Если кто-то уже начал работать с pnr (агент,разборщик PNL)
2. Если пассажир зарегистрировался, а разборщик PNL ставит признак удаления
*/

struct TWebPax {
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

void getPnr( int pnr_id, vector<TWebPax> &pnr )
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
	if ( Qry.Eof )
	  throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );
  while ( !Qry.Eof ) {
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

   		string pnr_status=Qry.FieldAsString("pnr_status");
   		if (//pax.name=="CBBG" ||  надо спросить у Сергиенко
   		    pnr_status=="DG2" ||
   		    pnr_status=="RG2" ||
   		    pnr_status=="ID2" ||
   		    pnr_status=="WL")
   		{
   		  pax.checkin_status = "agent_checkin";
   		};

   		CrsTKNQry.SetVariable( "pax_id", pax.crs_pax_id );
   		CrsTKNQry.Execute();
   		if ( !CrsTKNQry.Eof )
   		{
   		  pax.pr_eticket = strcmp(CrsTKNQry.FieldAsString("ticket_rem"),"TKNE")==0;
   		  pax.ticket_no	= CrsTKNQry.FieldAsString("ticket_no");
   		};
   		FQTQry.SQLText=CrsFQTQrySQL;
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
  	Qry.Next();
  }
  ProgTrace( TRACE5, "pass count=%d", pnr.size() );
}

void IntLoadPnr( int point_id, int pnr_id, xmlNodePtr segNode )
{
	ProgTrace( TRACE5, "point_id=%d, pnr_id=%d", point_id, pnr_id );
  getTripData( point_id, true );
  VerifyPNR( point_id, pnr_id );
  vector<TWebPax> pnr;
  getPnr( pnr_id, pnr );
  NewTextChild( segNode, "point_id", point_id );
  xmlNodePtr node = NewTextChild( segNode, "passengers" );
  for ( vector<TWebPax>::iterator i=pnr.begin(); i!=pnr.end(); i++ ) {
  	xmlNodePtr paxNode = NewTextChild( node, "pax" );
  	NewTextChild( paxNode, "crs_pax_id", i->crs_pax_id );
  	if ( i->crs_pax_id_parent != NoExists )
  		NewTextChild( paxNode, "crs_pax_id_parent", i->crs_pax_id_parent );
  	NewTextChild( paxNode, "surname", i->surname );
  	NewTextChild( paxNode, "name", i->name );
  	if ( i->birth_date != NoExists )
  		NewTextChild( paxNode, "birth_date", DateTimeToStr( i->birth_date, ServerFormatDateTimeAsString ) );
  	NewTextChild( paxNode, "pers_type", i->pers_type_extended );
  	if ( !i->seat_no.empty() )
  		NewTextChild( paxNode, "seat_no", i->seat_no );
  	else
  		if ( !i->preseat_no.empty() )
  			NewTextChild( paxNode, "seat_no", i->preseat_no );
  		else
  			if ( !i->crs_seat_no.empty() )
  			  NewTextChild( paxNode, "seat_no", i->crs_seat_no );
    NewTextChild( paxNode, "seats", i->seats );
   	NewTextChild( paxNode, "checkin_status", i->checkin_status );
   	if ( i->pr_eticket )
   	  NewTextChild( paxNode, "eticket", "true" );
   	else
   		NewTextChild( paxNode, "eticket", "false" );
   	NewTextChild( paxNode, "ticket_no", i->ticket_no );
   	xmlNodePtr fqtsNode = NewTextChild( paxNode, "fqt_rems" );
   	for ( vector<TFQTItem>::iterator f=i->fqt_rems.begin(); f!=i->fqt_rems.end(); f++ )
   	{
      xmlNodePtr fqtNode = NewTextChild( fqtsNode, "fqt_rem" );
      NewTextChild( fqtNode, "airline", f->airline );
      NewTextChild( fqtNode, "no", f->no );
    };

   	xmlNodePtr tidsNode = NewTextChild( paxNode, "tids" );
   	NewTextChild( tidsNode, "crs_pnr_tid", i->crs_pnr_tid );
   	NewTextChild( tidsNode, "crs_pax_tid", i->crs_pax_tid );
   	if ( i->pax_grp_tid != NoExists )
   		NewTextChild( tidsNode, "pax_grp_tid", i->pax_grp_tid );
   	if ( i->pax_tid != NoExists )
   		NewTextChild( tidsNode, "pax_tid", i->pax_tid );
  }
}

void WebRequestsIface::LoadPnr(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE1,"WebRequestsIface::SearchPnr");
  xmlNodePtr segNode = NodeAsNode( "segments/segment", reqNode );
  int point_id = NodeAsInteger( "point_id", segNode );
  int pnr_id = NodeAsInteger( "pnr_id", segNode );
  segNode = NewTextChild( NewTextChild( NewTextChild( resNode, "LoadPnr" ), "segments" ), "segment" );
  IntLoadPnr( point_id, pnr_id, segNode );
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
  ProgTrace(TRACE1,"WebRequestsIface::SearchPnr");
  int point_id = NodeAsInteger( "point_id", reqNode );
  int pnr_id = NodeAsInteger( "pnr_id", reqNode );
  string crs_class, crs_subclass;
  vector<TWebPax> pnr;
  getTripData( point_id, true );
  VerifyPNR( point_id, pnr_id );
  getPnr( pnr_id, pnr );
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
      wp.seat_no = denorm_iata_row( place->yname ) + denorm_iata_line( place->xname, Salons.getLatSeat() );
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

void VerifyPax(xmlNodePtr segNode, xmlDocPtr emulReqDoc, int &pnr_id)
{
	pnr_id=ASTRA::NoExists;
  if (emulReqDoc==NULL)
    throw EXCEPTIONS::Exception("VerifyPax: emulReqDoc=NULL");
  xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulReqDoc);
  emulReqNode=NewTextChild(emulReqNode,"TCkinSavePax");

  int point_id=NodeAsInteger("point_id",segNode);
  TSearchPnrData PnrData;
	getTripData( point_id, PnrData, true );

	if ( PnrData.act_out != NoExists )
		throw UserException( "MSG.FLIGHT.TAKEOFF" );
	if ( PnrData.web_stage != sOpenWEBCheckIn && PnrData.web_stage != sOpenKIOSKCheckIn )
	  throw UserException( "MSG.CHECKIN.NOT_OPEN" );

	NewTextChild(emulReqNode,"transfer"); //пустой тег - трансфера нет

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
      "SELECT tlg_trips.airline,tlg_trips.flt_no,tlg_trips.suffix, "
      "       tlg_trips.scd AS scd_out,tlg_trips.airp_dep AS airp, "
      "       crs_pax.pax_id,crs_pnr.point_id,crs_pnr.target,crs_pnr.subclass, "
      "       crs_pnr.class,crs_pax.surname,crs_pax.name,crs_pax.pers_type, "
      "       salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, "
      "       salons.get_crs_seat_no(crs_pax.pax_id,:protckin_layer,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS preseat_no, "
      "       crs_pax.seat_type, "
      "       crs_pax.seats, "
      "       pax.seats pax_seats, "
      "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'one',rownum) AS pax_seat_no, "
      "       crs_pnr.pnr_id, "
      "       report.get_PSPT(crs_pax.pax_id) AS document, "
      "       report.get_TKNO(crs_pax.pax_id,'/',0) AS ticket, "
      "       report.get_TKNO(crs_pax.pax_id,'/',1) AS eticket, "
      "       crs_pnr.tid AS crs_pnr_tid, "
      "       crs_pax.tid AS crs_pax_tid, "
      "       pax_grp.tid AS pax_grp_tid, "
      "       pax.tid AS pax_tid "
      "FROM tlg_trips,crs_pnr,crs_pax,pax,pax_grp "
      "WHERE tlg_trips.point_id=crs_pnr.point_id AND "
      "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      pax.grp_id=pax_grp.grp_id(+) AND "
      "      crs_pax.pax_id=:crs_pax_id AND "
      "      crs_pax.pr_del=0 ";
  Qry.CreateVariable( "protckin_layer", otString, EncodeCompLayerType(ASTRA::cltProtCkin) );
  Qry.DeclareVariable("crs_pax_id",otInteger);

  TQuery RemQry(&OraSession);
  RemQry.SQLText =
    "SELECT rem_code,rem FROM crs_pax_rem WHERE pax_id=:pax_id AND rem_code NOT IN ('FQTV')";
  RemQry.DeclareVariable("pax_id",otInteger);

  xmlNodePtr node,reqPaxNode=NodeAsNode("passengers/pax",segNode);

  PnrData.pnr_id=ASTRA::NoExists;
  int adult_count=0, without_seat_count=0;
  xmlNodePtr paxsNode;
  for(;reqPaxNode!=NULL;reqPaxNode=reqPaxNode->next)
  {
    Qry.SetVariable("crs_pax_id",NodeAsInteger("crs_pax_id",reqPaxNode));
    Qry.Execute();
    if (Qry.Eof)
      throw UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");

    xmlNodePtr tidsNode=NodeAsNode("tids",reqPaxNode)->children;
    if (NodeAsIntegerFast("crs_pnr_tid",tidsNode)!=Qry.FieldAsInteger("crs_pnr_tid") ||
        NodeAsIntegerFast("crs_pax_tid",tidsNode)!=Qry.FieldAsInteger("crs_pax_tid"))
      throw UserException("MSG.PASSENGER.CHANGED.REFRESH_DATA");

    if (PnrData.pnr_id==ASTRA::NoExists)
    {
      //первый пассажир
      PnrData.pnr_id=Qry.FieldAsInteger("pnr_id");
      //проверим, что данное PNR привязано к рейсу
      VerifyPNR(point_id,PnrData.pnr_id);
    }
    else
    {
      if (Qry.FieldAsInteger("pnr_id")!=PnrData.pnr_id)
        throw EXCEPTIONS::Exception("VerifyPax: passengers from different PNR");
    };

    if (!Qry.FieldIsNULL("pax_tid"))
    {
      //пассажир зарегистрирован
      if (NodeIsNULLFast("pax_grp_tid",tidsNode,true) ||
          NodeIsNULLFast("pax_tid",tidsNode,true))
        throw UserException("MSG.PASSENGER.CHECKED.REFRESH_DATA");

      if (NodeAsIntegerFast("pax_grp_tid",tidsNode)!=Qry.FieldAsInteger("pax_grp_tid") ||
          NodeAsIntegerFast("pax_tid",tidsNode)!=Qry.FieldAsInteger("pax_tid"))
        throw UserException("MSG.PASSENGER.CHANGED.REFRESH_DATA");

      if (GetNode("seat_no",reqPaxNode)!=NULL &&
      	  !NodeIsNULL("seat_no",reqPaxNode) &&
      	  Qry.FieldAsInteger( "pax_seats" ) > 0)
      {
      	string prior_xname, prior_yname;
      	string curr_xname, curr_yname;
      	// надо номализовать старое и новое место, сравнить их, если изменены, то вызвать пересадку
      	getXYName( point_id, Qry.FieldAsString( "pax_seat_no" ), prior_xname, prior_yname );
      	getXYName( point_id, NodeAsString( "seat_no", reqPaxNode), curr_xname, curr_yname );
      	if ( curr_xname.empty() && curr_yname.empty() )
      		throw UserException( "MSG.SEATS.SEAT_NO.NOT_FOUND" );
      	if ( prior_xname + prior_yname != curr_xname + curr_yname ) {
          IntChangeSeats( point_id, Qry.FieldAsInteger( "pax_id" ),
                          Qry.FieldAsInteger("pax_tid"), curr_xname, curr_yname,
	                        SEATS2::stReseat,
	                        cltUnknown,
                          false, false,
                          NULL );
      	}
      };
    }
    else
    {
      //пассажир не зарегистрирован
      if (GetNode("segments",emulReqNode)==NULL)
      {
        //найдем пункт посдки PNR
        TTripRoute route;
        route.GetRouteAfter( PnrData.point_id,
                             PnrData.point_num,
                             PnrData.first_point,
                             PnrData.pr_tranzit,
                             trtNotCurrent,
                             trtNotCancelled );
        vector<TTripRouteItem>::iterator i=route.begin();
        for ( ; i!=route.end(); i++ )
          if (i->airp == Qry.FieldAsString( "target" )) break;
        if (i==route.end())
          throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");

        xmlNodePtr segNode=NewTextChild(NewTextChild(emulReqNode,"segments"),"segment");

        NewTextChild(segNode,"point_dep",PnrData.point_id);
        NewTextChild(segNode,"point_arv",i->point_id);
        NewTextChild(segNode,"airp_dep",PnrData.airp_dep);
        NewTextChild(segNode,"airp_arv",i->airp);
        NewTextChild(segNode,"class",Qry.FieldAsString("class"));
        NewTextChild(segNode,"status","K");
        NewTextChild(segNode,"wl_type");

        TTripInfo operFlt,pnrMarkFlt;
        TCodeShareSets codeshareSets;
        GetPNRCodeshare(PnrData, operFlt, pnrMarkFlt, codeshareSets);

        node=NewTextChild(segNode,"mark_flight");
        NewTextChild(node,"airline",pnrMarkFlt.airline);
        NewTextChild(node,"flt_no",pnrMarkFlt.flt_no);
        NewTextChild(node,"suffix",pnrMarkFlt.suffix);
        NewTextChild(node,"scd",DateTimeToStr(pnrMarkFlt.scd_out));  //локальная дата
        NewTextChild(node,"airp_dep",pnrMarkFlt.airp);
        NewTextChild(node,"pr_mark_norms",(int)codeshareSets.pr_mark_norms);

        paxsNode=NewTextChild(segNode,"passengers");
      };

      xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
      NewTextChild(paxNode,"pax_id",Qry.FieldAsInteger("pax_id"));
      NewTextChild(paxNode,"surname",Qry.FieldAsString("surname"));
      NewTextChild(paxNode,"name",Qry.FieldAsString("name"));
      NewTextChild(paxNode,"pers_type",Qry.FieldAsString("pers_type"));
      if (GetNode("seat_no",reqPaxNode)!=NULL &&
      	  !NodeIsNULL("seat_no",reqPaxNode))
        NewTextChild(paxNode,"seat_no",NodeAsString("seat_no",reqPaxNode));
      else
        NewTextChild(paxNode,"seat_no",Qry.FieldAsString("seat_no"));
      NewTextChild(paxNode,"preseat_no",Qry.FieldAsString("preseat_no"));
      NewTextChild(paxNode,"seat_type",Qry.FieldAsString("seat_type"));
      NewTextChild(paxNode,"seats",Qry.FieldAsInteger("seats"));
      //обработка билетов
      string ticket_no;
      if (!Qry.FieldIsNULL("eticket"))
      {
        //билет TKNE
        ticket_no=Qry.FieldAsString("eticket");

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
        ticket_no=Qry.FieldAsString("ticket");

        NewTextChild(paxNode,"ticket_no",ticket_no);
        NewTextChild(paxNode,"coupon_no");
        if (!ticket_no.empty())
          NewTextChild(paxNode,"ticket_rem","TKNA");
        else
          NewTextChild(paxNode,"ticket_rem");
        NewTextChild(paxNode,"ticket_confirm",(int)false);
      };
      NewTextChild(paxNode,"document",Qry.FieldAsString("document"));
      NewTextChild(paxNode,"subclass",Qry.FieldAsString("subclass"));
      NewTextChild(paxNode,"transfer"); //пустой тег - трансфера нет
      //ремарки
      RemQry.SetVariable("pax_id",Qry.FieldAsInteger("pax_id"));
      RemQry.Execute();
      xmlNodePtr remsNode=NewTextChild(paxNode,"rems");
      for(;!RemQry.Eof;RemQry.Next())
      {
        xmlNodePtr remNode=NewTextChild(remsNode,"rem");
        NewTextChild(remNode,"rem_code",RemQry.FieldAsString("rem_code"));
        NewTextChild(remNode,"rem_text",RemQry.FieldAsString("rem"));
      };
      //добавим переданные fqt_rems
      xmlNodePtr fqtNode = GetNode("fqt_rems",reqPaxNode);
      if (fqtNode!=NULL) fqtNode=fqtNode->children;
      for(; fqtNode!=NULL; fqtNode=fqtNode->next)
      {
        xmlNodePtr remNode=NewTextChild(remsNode,"rem");
        NewTextChild(remNode,"rem_code","FQTV");
        ostringstream rem_text;
        rem_text << "FQTV "
                 << NodeAsString("airline",fqtNode) << " "
                 << NodeAsString("no",fqtNode);
        NewTextChild(remNode,"rem_text",rem_text.str());
      };

      NewTextChild(paxNode,"norms"); //пустой тег - норм нет

      TPerson p=DecodePerson(Qry.FieldAsString("pers_type"));
      int seats=Qry.FieldAsInteger("seats");
      if (p==ASTRA::adult) adult_count++;
      if (p==ASTRA::baby && seats==0) without_seat_count++;
    };
  };

  NewTextChild(emulReqNode,"excess",(int)0);
  NewTextChild(emulReqNode,"hall");

  if (without_seat_count>adult_count)
    throw UserException("MSG.CHECKIN.BABY_WO_SEATS_MORE_ADULT_FOR_GRP");

  pnr_id=PnrData.pnr_id;
};

void WebRequestsIface::SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	SavePax(reqNode, NULL, resNode);
};

bool WebRequestsIface::SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode, xmlNodePtr resNode)

{
	ProgTrace(TRACE1,"WebRequestsIface::SavePax");
	xmlNodePtr segNode = NodeAsNode( "segments/segment", reqNode );
	int point_id = NodeAsInteger( "point_id", segNode );
	int pnr_id;
	XMLDoc emulReqDoc("UTF-8","term");
  if (emulReqDoc.docPtr()==NULL)
    throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: CreateXMLDoc failed");
  CopyNode(NodeAsNode("/term",emulReqDoc.docPtr()),
           NodeAsNode("/term/query",reqNode->doc), true); //копируем полностью тег query
  xmlNodePtr node=NodeAsNode("/term/query",emulReqDoc.docPtr())->children;
  if (node!=NULL)
  {
    xmlUnlinkNode(node);
    xmlFreeNode(node);
  };

  VerifyPax(segNode, emulReqDoc.docPtr(), pnr_id);
  if (pnr_id==ASTRA::NoExists)
    throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: pnr_id not defined");

  xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulReqDoc.docPtr())->children;
  if (emulReqNode==NULL)
    throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: emulReqNode=NULL");

  bool result=true;

  if (GetNode("segments",emulReqNode)!=NULL) //не только пересадка, но и регистрация
    result=CheckInInterface::SavePax(reqNode, emulReqNode, ediResNode, resNode);

  if (result)
  {
    segNode = NewTextChild( NewTextChild( NewTextChild( resNode, "SavePax" ), "segments" ), "segment" );
    IntLoadPnr( point_id, pnr_id, segNode );
  };
  return result;
};

class BPTags {
	private:
		map<string,string> tags;
		string getFieldFormat( const string &field_name, const string &format );
		void addField( const string &field_name, const string &format = "" );
		BPTags();
	public:
		void getFields( map<string,string> &atags );
		static BPTags *Instance() {
      static BPTags *instance_ = 0;
      if ( !instance_ )
        instance_ = new BPTags();
      return instance_;
		}
};

string BPTags::getFieldFormat( const string &field_name, const string &format )
{
	string res;
	if ( format == "date" )
		res = string("[<") + field_name + "(,," + ServerFormatDateTimeAsString + ")>]";
	else
		if ( format == "lat" )
			res = string("[<") + field_name + "(,,,E)>]";
		else
			res = string("[<") + field_name + ">]";
  return res;
}

void BPTags::addField( const string &field_name, const string &format )
{
	tags[ field_name ] = getFieldFormat( field_name, format );
}

BPTags::BPTags()
{
	addField( "act", "date" );
	addField( "airline" );
	addField( "airline_lat", "lat" );
	addField( "airline_short" );
	addField( "airline_short_lat", "lat" );
	addField( "airp_arv" );
	addField( "airp_arv_lat", "lat" );
	addField( "airp_arv_name" );
	addField( "airp_arv_name_lat", "lat" );
	addField( "airp_dep" );
	addField( "airp_dep_lat", "lat" );
	addField( "airp_dep_name" );
	addField( "airp_dep_name_lat", "lat" );
	addField( "bag_amount" );
	addField( "bag_weight" );
	addField( "bort" );
	addField( "bort_lat", "lat" );
	addField( "brd_from", "date" );
	addField( "brd_to", "date" );
	addField( "city_arv" );
	addField( "city_arv_lat", "lat" );
	addField( "city_arv_name" );
	addField( "city_arv_name_lat", "lat" );
	addField( "city_dep" );
	addField( "city_dep_lat", "lat" );
	addField( "city_dep_name" );
	addField( "city_dep_name_lat", "lat" );
	addField( "class" );
	addField( "class_lat", "lat" );
	addField( "class_name" );
	addField( "class_name_lat", "lat" );
	addField( "coupon_no" );
	addField( "craft" );
	addField( "craft_lat", "lat" );
	addField( "document" );
	addField( "est", "date" );
	addField( "eticket_no" );
	addField( "excess" );
	addField( "flt_no" );
	addField( "flt_no_lat", "lat" );
	addField( "fullname" );
	addField( "fullname_lat", "lat" );
	addField( "list_seat_no" );
	addField( "list_seat_no_lat", "lat" );
	addField( "name" );
	addField( "name_lat", "lat" );
	addField( "no_smoke" );
	addField( "one_seat_no" );
	addField( "one_seat_no_lat", "lat" );
	addField( "pax_id" );
	addField( "pers_type" );
	addField( "pers_type_lat", "lat" );
	addField( "pers_type_name" );
	addField( "pnr" );
	addField( "pr_smoke" );
	addField( "reg_no" );
	addField( "rk_weight" );
	addField( "scd", "date" );
	addField( "seat_no" );
	addField( "seat_no_lat", "lat" );
	addField( "seats" );
	addField( "smoke" );
	addField( "str_seat_no" );
	addField( "str_seat_no_lat", "lat" );
	addField( "subclass" );
	addField( "subclass_lat", "lat" );
	addField( "suffix" );
	addField( "suffix_lat", "lat" );
	addField( "surname" );
	addField( "surname_lat", "lat" );
	addField( "tags" );
	addField( "tags_lat", "lat" );
	addField( "ticket_no" );
	addField( "bcbp_m_2", "lat" );
}

void BPTags::getFields( map<string,string> &atags )
{
	atags.clear();
	atags = tags;
}

void WebRequestsIface::GetBPTags(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
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
	map<string,string> tags;
	BPTags::Instance()->getFields( tags );
	string value;
	node = NewTextChild( resNode, "GetBPTags" );
	for ( map<string,string>::iterator i=tags.begin(); i!=tags.end(); i++ ) {
		value = parser.parse( i->second );
		NewTextChild( node, i->first.c_str(), value );
		ProgTrace( TRACE5, "field name=%s, value=%s", i->first.c_str(), value.c_str() );
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

