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
        new_body=new_body.substr(0,pos+sss.size())+" id='"+client.client_type+"' screen='AIR.EXE' opr='"+CP866toUTF8(client.opr)+"'"+new_body.substr(pos+sss.size());
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

	int airline_fmt,suffix_fmt,airp_dep_fmt,craft_fmt;

  int point_arv;
	TDateTime scd_in;
	string airp_arv;
	string city_arv;
	int pnr_id;
	string cls;
	string subcls;
	int bag_norm;
	vector<TPnrAddr> pnraddrs;
};

bool getTripData( int point_id, TSearchPnrData &SearchPnrData, bool pr_throw )
{
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
	tst();
	if ( Qry.FieldAsInteger( "pr_del" ) == -1 )
		if ( pr_throw )
		  throw UserException( "MSG.FLIGHT.DELETED" );
		else
			return false;
	tst();
	if ( Qry.FieldAsInteger( "pr_del" ) == 1 )
		if ( pr_throw )
		  throw UserException( "MSG.FLIGHT.CANCELED" );
		else
			return false;
	tst();
	if ( Qry.FieldAsInteger( "pr_reg" ) == 0 )
		if ( pr_throw )
		  throw UserException( "MSG.FLIGHT.CHECKIN_CANCELED" );
		else
			return false;
	tst();
	if ( Qry.FieldIsNULL( "act_out" ) )
		SearchPnrData.act_out = NoExists;
	else
		SearchPnrData.act_out = Qry.FieldAsDateTime( "act_out" );
	tst();
	TTripStages tripStages( point_id );
	SearchPnrData.web_stage = tripStages.getStage( stWEB );
	SearchPnrData.checkin_stage = tripStages.getStage( stCheckIn );
	SearchPnrData.brd_stage = tripStages.getStage( stBoarding );

	ProgTrace( TRACE5, "web_stage=%d, checkin_stage=%d, brd_stage=%d",
	           (int)SearchPnrData.web_stage, (int)SearchPnrData.checkin_stage, (int)SearchPnrData.brd_stage );
	TBaseTable &baseairps = base_tables.get( "airps" );
	TBaseTable &basecities = base_tables.get( "cities" );

	//SearchPnrData.airline = ElemIdToElemCtxt( ecDisp, etAirline, Qry.FieldAsString( "airline" ), Qry.FieldAsInteger( "airline_fmt" ) ); //??? нужно ли переводить в формат терминала
	SearchPnrData.airline = Qry.FieldAsString( "airline" );
	SearchPnrData.airline_fmt = Qry.FieldAsInteger( "airline_fmt" );
	SearchPnrData.flt_no = Qry.FieldAsInteger( "flt_no" );
	//SearchPnrData.suffix = ElemIdToElemCtxt( ecDisp, etSuffix, Qry.FieldAsString( "suffix" ), Qry.FieldAsInteger( "suffix_fmt" ) );
	SearchPnrData.suffix = Qry.FieldAsString( "suffix" );
	SearchPnrData.suffix_fmt = Qry.FieldAsInteger( "suffix_fmt" );
	//SearchPnrData.airp_dep = ElemIdToElemCtxt( ecDisp, etAirp, Qry.FieldAsString( "airp" ), Qry.FieldAsInteger( "airp_fmt" ) );
	SearchPnrData.airp_dep = Qry.FieldAsString( "airp" );
	SearchPnrData.airp_dep_fmt = Qry.FieldAsInteger( "airp_fmt" );
	SearchPnrData.city_dep = ((TAirpsRow&)baseairps.get_row( "code", Qry.FieldAsString( "airp" ), true )).city;
	string region = ((TCitiesRow&)basecities.get_row( "code", SearchPnrData.city_dep, true )).region;
	SearchPnrData.scd_out = Qry.FieldAsDateTime( "scd_out" );
	SearchPnrData.scd_out_local = UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), region );
	//SearchPnrData.craft = ElemIdToElemCtxt( ecDisp, etCraft, Qry.FieldAsString( "craft" ), Qry.FieldAsInteger( "craft_fmt" ) );
	SearchPnrData.craft = Qry.FieldAsString( "craft" );
	SearchPnrData.craft_fmt = Qry.FieldAsInteger( "craft_fmt" );
	SearchPnrData.stages.clear();
	SearchPnrData.stages.insert( make_pair(sOpenWEBCheckIn, UTCToLocal( tripStages.time(sOpenWEBCheckIn), region ) ) );
	SearchPnrData.stages.insert( make_pair(sCloseWEBCheckIn, UTCToLocal( tripStages.time(sCloseWEBCheckIn), region ) ) );
	SearchPnrData.stages.insert( make_pair(sOpenCheckIn, UTCToLocal( tripStages.time(sOpenCheckIn), region ) ) );
	SearchPnrData.stages.insert( make_pair(sCloseCheckIn, UTCToLocal( tripStages.time(sCloseCheckIn), region ) ) );
	SearchPnrData.stages.insert( make_pair(sOpenBoarding, UTCToLocal( tripStages.time(sOpenBoarding), region ) ) );
	SearchPnrData.stages.insert( make_pair(sCloseBoarding, UTCToLocal( tripStages.time(sCloseBoarding), region ) ) );
	SearchPnrData.point_id = point_id;
	SearchPnrData.point_num = Qry.FieldAsInteger("point_num");
	SearchPnrData.first_point = Qry.FieldAsInteger("first_point");
	SearchPnrData.pr_tranzit = Qry.FieldAsInteger("pr_tranzit")!=0;
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

bool findPnr( const string &surname, const string &pnr_addr,
              const string &ticket_no, const string &document,
              TSearchPnrData &SearchPnrData )
{
	ProgTrace( TRACE5, "surname=%s, pnr_addr=%s, ticket_no=%s, document=%s",
	           surname.c_str(), pnr_addr.c_str(), ticket_no.c_str(), document.c_str() );
  SearchPnrData.pnr_id = NoExists;
  if ( surname.empty() )
  	throw UserException( "MSG.PASSENGER.NOT_SET.SURNAME" );
  if ( pnr_addr.empty() && ticket_no.empty() && document.empty() )
  	throw UserException( "MSG.NOTSET.SEARCH_PARAMS" );

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
 	TQuery Qry(&OraSession);
	Qry.SQLText =
	 "SELECT crs_pax.pnr_id, "
   "       crs_pax.pax_id, "
   "       crs_pnr.target, "
   "       crs_pnr.class, "
   "       crs_pnr.subclass "
   " FROM tlg_binding,crs_pnr,crs_pax "
   "WHERE tlg_binding.point_id_tlg=crs_pnr.point_id AND "
   "      crs_pax.pnr_id=crs_pnr.pnr_id AND "
   "      tlg_binding.point_id_spp=:point_id AND "
   "      system.transliter(crs_pax.surname,1)=:surname AND "
   "      crs_pax.pr_del=0";
  Qry.CreateVariable( "point_id", otInteger, SearchPnrData.point_id );
  Qry.CreateVariable( "surname", otString, transliter( surname, 1 ) );
  Qry.Execute();
  int fcount = 0;
  vector<TPnrAddr> pnraddrs;
  int point_arv = NoExists;

  for ( ; !Qry.Eof; Qry.Next() ) { // пробег по всем найденным пассажирам
  	// не проверяем остальных пассажиров в группе, в которой был найден пассажир удовлетворяющий критериям поиска
  	if ( SearchPnrData.pnr_id > NoExists && SearchPnrData.pnr_id == Qry.FieldAsInteger( "pnr_id" ) )
  		continue;

  	bool pr_find = true;
  	if ( !ticket_no.empty() ) { // если задан поиск по билету, то делаем проверку
      QryTicket.SetVariable( "pax_id", Qry.FieldAsInteger( "pax_id" ) );
      QryTicket.Execute();
      pr_find = ( !QryTicket.Eof );
    };
    ProgTrace( TRACE5, "after search ticket, pr_find=%d", pr_find );
    if ( pr_find && !document.empty() ) { // если пред. проверка удовл. и если задан документ, то делаем проверку
    	QryDoc.SetVariable( "pax_id", Qry.FieldAsInteger( "pax_id" ) );
    	QryDoc.Execute();
    	pr_find = ( !QryDoc.Eof );
    }
    ProgTrace( TRACE5, "after search document, pr_find=%d", pr_find );
    if ( pr_find ) { // если пред. проверки удовл.
    	pr_find = false;
    	pnraddrs.clear();
  	  QryPnrAddr.SetVariable( "pnr_id", Qry.FieldAsInteger( "pnr_id" ) );
    	QryPnrAddr.Execute();
    	for ( ; !QryPnrAddr.Eof; QryPnrAddr.Next() ) { // пробег по всем адресам и поиск по условию + заполнение вектора с адресами
    		TPnrAddr addr;
    		addr.airline = QryPnrAddr.FieldAsString( "airline" );
    		addr.addr = QryPnrAddr.FieldAsString( "addr" );
    		pnraddrs.push_back( addr );
    		if ( pnr_addr.empty() || pnr_addr == addr.addr )
    			pr_find = true;
    	}
    }

    ProgTrace( TRACE5, "after search pnr_addr, pr_find=%d", pr_find );

    if ( pr_find ) { // проверка на совпадение аэропорта прилета из pnr и СПП
      pr_find = false;
      TTripRoute routes;
      routes.GetRouteAfter( SearchPnrData.point_id,
                            SearchPnrData.point_num,
                            SearchPnrData.first_point,
                            SearchPnrData.pr_tranzit,
                            trtNotCurrent,
                            trtNotCancelled );
      for ( vector<TTripRouteItem>::iterator i=routes.begin(); i!=routes.end(); i++ ) {
        if ( i->airp == Qry.FieldAsString( "target" ) ) {
 	  point_arv = i->point_id;
      	  pr_find = true;
      	  break;
        }
      }
    }

    ProgTrace( TRACE5, "after search TTripRoute, pr_find=%d", pr_find );

  	if ( pr_find ) { // нашли
  		tst();
  		if ( fcount )
  			throw UserException( "MSG.PASSENGERS.FOUND_MORE" );
  		SearchPnrData.pnraddrs = pnraddrs;
  		SearchPnrData.cls = Qry.FieldAsString( "class" );
  		SearchPnrData.subcls = Qry.FieldAsString( "subclass" );
  		SearchPnrData.pnr_id = Qry.FieldAsInteger( "pnr_id" );
      TBaseTable &baseairps = base_tables.get( "airps" );
	    TBaseTable &basecities = base_tables.get( "cities" );
  		Qry.Clear();
  		Qry.SQLText =
  		  "SELECT airp, scd_in FROM points WHERE point_id=:point_id";
  		Qry.CreateVariable( "point_id", otInteger, point_arv );
  		Qry.Execute();
  		SearchPnrData.point_arv = point_arv;
  		SearchPnrData.airp_arv = Qry.FieldAsString( "airp" );
	    SearchPnrData.city_arv = ((TAirpsRow&)baseairps.get_row( "code", SearchPnrData.airp_arv, true )).city;
      string region = ((TCitiesRow&)basecities.get_row( "code", SearchPnrData.city_arv, true )).region;
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


  		fcount++;
  	}
  }
  return fcount;
}

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
</SearchPnr>
*/

  ProgTrace(TRACE1,"WebRequestsIface::SearchFlt");
  string surname = NodeAsString( "surname", reqNode, "" );
  string pnr_addr = NodeAsString( "pnr_addr", reqNode, "" );
  string ticket_no = NodeAsString( "ticket_no", reqNode, "" );
  string document = NodeAsString( "document", reqNode, "" );

  TSearchPnrData SearchPnrData;
  int fmt;
  SearchPnrData.airline = NodeAsString( "airline", reqNode, "" );
	SearchPnrData.airline = TrimString( SearchPnrData.airline );
  if ( SearchPnrData.airline.empty() )
   	throw UserException( "MSG.AIRLINE.NOT_SET" );
  try {
    SearchPnrData.airline = ElemCtxtToElemId( ecDisp, etAirline, SearchPnrData.airline, fmt, false );
  }
  catch( EXCEPTIONS::EConvertError &e ) {
  	throw UserException( "MSG.AIRLINE.INVALID",
  		                   LParams()<<LParam("airline", SearchPnrData.airline ) );
  }
  string str_flt_no = NodeAsString( "flt_no", reqNode, "" );
	if ( StrToInt( str_flt_no.c_str(), SearchPnrData.flt_no ) == EOF ||
		   SearchPnrData.flt_no > 99999 || SearchPnrData.flt_no <= 0 )
		throw UserException( "MSG.FLT_NO.INVALID",
			                   LParams()<<LParam("flt_no", str_flt_no) );
	SearchPnrData.suffix = NodeAsString( "suffix", reqNode, "" );
	SearchPnrData.suffix = TrimString( SearchPnrData.suffix );
	if ( SearchPnrData.suffix.size() > 1 )
		throw UserException( "MSG.SUFFIX.INVALID",
			                   LParams()<<LParam("suffix", SearchPnrData.suffix) );
  try {
   SearchPnrData.suffix = ElemCtxtToElemId( ecDisp, etSuffix, SearchPnrData.suffix, fmt, false );
  }
  catch( EXCEPTIONS::EConvertError &e ) {
		throw UserException( "MSG.SUFFIX.INVALID",
			                   LParams()<<LParam("suffix", SearchPnrData.suffix) );
  }
  string str_scd_out = NodeAsString( "scd_out", reqNode, "" );
	str_scd_out = TrimString( str_scd_out );
  if ( str_scd_out.empty() )
		throw UserException( "MSG.FLIGHT_DATE.NOT_SET" );
	else
		if ( StrToDateTime( str_scd_out.c_str(), "dd.mm.yyyy hh:nn:ss", SearchPnrData.scd_out_local ) == EOF )
			throw UserException( "MSG.FLIGHT_DATE.INVALID",
				                   LParams()<<LParam("suffix", str_scd_out) );
	TQuery Qry(&OraSession);
	Qry.SQLText =
	  "SELECT point_id,airp,scd_out FROM points"
	  " WHERE airline=:airline AND "
	  "       flt_no=:flt_no AND "
	  "      ( suffix IS NULL AND :suffix IS NULL OR suffix=:suffix ) AND "
	  "      scd_out >= :first_date AND scd_out <= :last_date ";
	Qry.CreateVariable( "airline", otString, SearchPnrData.airline );
	Qry.CreateVariable( "flt_no", otInteger, SearchPnrData.flt_no );
	if ( SearchPnrData.suffix.empty() )
	  Qry.CreateVariable( "suffix", otString, FNull );
	else
		Qry.CreateVariable( "suffix", otString, SearchPnrData.suffix );
  double sd, sd1;
  modf( SearchPnrData.scd_out_local, &sd );
	Qry.CreateVariable( "first_date", otDate, sd-1 );
	Qry.CreateVariable( "last_date", otDate, sd+1 );
	Qry.Execute();
	ProgTrace( TRACE5, "airline=%s, flt_no=%d, suffix=|%s|, sd=%s",
	           SearchPnrData.airline.c_str(), SearchPnrData.flt_no, SearchPnrData.suffix.c_str(),
	           DateTimeToStr( sd ).c_str() );
	if ( Qry.Eof )
		throw UserException( "MSG.FLIGHT.NOT_FOUND" );
	TBaseTable &baseairps = base_tables.get( "airps" );
	TBaseTable &basecities = base_tables.get( "cities" );
	SearchPnrData.point_id = NoExists;
	SearchPnrData.pnr_id = NoExists;
	while ( !Qry.Eof ) {
		string city = ((TAirpsRow&)baseairps.get_row( "code", Qry.FieldAsString( "airp" ), true )).city;
		string region = ((TCitiesRow&)basecities.get_row( "code", city, true )).region;
		modf( UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), region ), &sd1 );
		if ( sd == sd1 ) {
			if ( getTripData( Qry.FieldAsInteger( "point_id" ), SearchPnrData, false ) &&
				   findPnr( surname, pnr_addr, ticket_no, document, SearchPnrData ) ) {
				tst();
      	break;
      }
		}
		Qry.Next();
	}
	if ( SearchPnrData.point_id == NoExists )
		throw UserException( "MSG.FLIGHT.NOT_FOUND" );
  if ( SearchPnrData.pnr_id == NoExists )
  	throw UserException( "MSG.PASSENGERS.NOT_FOUND" );

  xmlNodePtr node = NewTextChild( resNode, "SearchFlt" );
  NewTextChild( node, "point_id", SearchPnrData.point_id );
  NewTextChild( node, "airline", SearchPnrData.airline );
  NewTextChild( node, "flt_no", SearchPnrData.flt_no );
  if ( !SearchPnrData.suffix.empty() )
    NewTextChild( node, "suffix", SearchPnrData.suffix );
  NewTextChild( node, "craft", SearchPnrData.craft );
  NewTextChild( node, "scd_out", DateTimeToStr( SearchPnrData.scd_out_local, ServerFormatDateTimeAsString ) );
  NewTextChild( node, "airp_dep", SearchPnrData.airp_dep );
  NewTextChild( node, "city_dep", SearchPnrData.city_dep );
  NewTextChild( node, "scd_in", DateTimeToStr( SearchPnrData.scd_in, ServerFormatDateTimeAsString ) );
  NewTextChild( node, "airp_arv", SearchPnrData.airp_arv );
  NewTextChild( node, "city_arv", SearchPnrData.city_arv );
  if ( SearchPnrData.act_out > NoExists )
  	NewTextChild( node, "status", "sTakeoff" );
  else
    switch ( SearchPnrData.web_stage ) {
    	case sNoActive:
    		NewTextChild( node, "status", "sNoActive" );
    		break;
    	case sOpenWEBCheckIn:
    		NewTextChild( node, "status", "sOpenWEBCheckIn" );
    		break;
    	case sCloseWEBCheckIn:
    		NewTextChild( node, "status", "sCloseWEBCheckIn" );
    		break;
    	case sTakeoff:
    		NewTextChild( node, "status", "sTakeoff" );
    		break;
 	  	default:;
    };

  xmlNodePtr stagesNode = NewTextChild( node, "stages" );
  SetProp( NewTextChild( stagesNode, "stage", DateTimeToStr( SearchPnrData.stages[ sOpenWEBCheckIn ], ServerFormatDateTimeAsString ) ),
           "type", "sOpenWEBCheckIn" );
  SetProp( NewTextChild( stagesNode, "stage", DateTimeToStr( SearchPnrData.stages[ sCloseWEBCheckIn ], ServerFormatDateTimeAsString ) ),
           "type", "sCloseWEBCheckIn" );
  SetProp( NewTextChild( stagesNode, "stage", DateTimeToStr( SearchPnrData.stages[ sOpenCheckIn ], ServerFormatDateTimeAsString ) ),
           "type", "sOpenCheckIn" );
  SetProp( NewTextChild( stagesNode, "stage", DateTimeToStr( SearchPnrData.stages[ sCloseCheckIn ], ServerFormatDateTimeAsString ) ),
           "type", "sCloseCheckIn" );
  SetProp( NewTextChild( stagesNode, "stage", DateTimeToStr( SearchPnrData.stages[ sOpenBoarding ], ServerFormatDateTimeAsString ) ),
           "type", "sOpenBoarding" );
  SetProp( NewTextChild( stagesNode, "stage", DateTimeToStr( SearchPnrData.stages[ sCloseBoarding ], ServerFormatDateTimeAsString ) ),
           "type", "sCloseBoarding" );
  xmlNodePtr semNode = NewTextChild( node, "semaphors" );
  NewTextChild( semNode, "web_checkin", SearchPnrData.act_out == NoExists && SearchPnrData.web_stage == sOpenWEBCheckIn );
  NewTextChild( semNode, "term_checkin", SearchPnrData.act_out == NoExists && SearchPnrData.checkin_stage == sOpenCheckIn );
  NewTextChild( semNode, "term_brd", SearchPnrData.act_out == NoExists && SearchPnrData.brd_stage == sOpenBoarding );
  NewTextChild( node, "pnr_id", SearchPnrData.pnr_id );
  NewTextChild( node, "subclass", SearchPnrData.subcls );
  if (SearchPnrData.bag_norm!=ASTRA::NoExists)
    NewTextChild( node, "bag_norm", SearchPnrData.bag_norm );
  else
  	NewTextChild( node, "bag_norm" );
  if ( !SearchPnrData.pnraddrs.empty() ) {
  	node = NewTextChild( node, "pnr_addrs" );
  	for ( vector<TPnrAddr>::iterator i=SearchPnrData.pnraddrs.begin(); i!=SearchPnrData.pnraddrs.end(); i++ ) {
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
	string pers_type;
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
	TQuery PaxBirthQry(&OraSession);
	PaxBirthQry.SQLText =
	 "SELECT birth_date "
   "FROM pax_doc "
   "WHERE pax_id=:pax_id AND "
   "      birth_date IS NOT NULL ";
  PaxBirthQry.DeclareVariable( "pax_id", otInteger );
	TQuery CrsBirthQry(&OraSession);
	CrsBirthQry.SQLText =
    "SELECT birth_date "
    "FROM crs_pax_doc "
    "WHERE pax_id=:pax_id AND "
    "      birth_date IS NOT NULL "
    "ORDER BY DECODE(type,'P',0,NULL,2,1),DECODE(rem_code,'DOCS',0,1),no";
  CrsBirthQry.DeclareVariable( "pax_id", otInteger );
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
    "       crs_pnr.tid AS crs_pnr_tid, "
    "       crs_pax.tid AS crs_pax_tid, "
    "       pax_grp.tid AS pax_grp_tid, "
    "       pax.tid AS pax_tid, "
    "       pax.pax_id, "
    "       pax_grp.client_type, "
    "       pax.refuse "
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
  	pax.pers_type = Qry.FieldAsString( "pers_type" );
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
  		    	pax.checkin_status = "web_checked";
  		  		break;
  		  	default: ;
    	  };
  	  };
  		pax.pax_id = Qry.FieldAsInteger( "pax_id" );
   		PaxBirthQry.SetVariable( "pax_id", pax.pax_id );
   		PaxBirthQry.Execute();
   		if ( !PaxBirthQry.Eof )
   			pax.birth_date = PaxBirthQry.FieldAsDateTime( "birth_date" );

   	}
   	else {
  		pax.checkin_status = "not_checked";
   		CrsBirthQry.SetVariable( "pax_id", pax.crs_pax_id );
   		CrsBirthQry.Execute();
   		if ( !CrsBirthQry.Eof )
   			pax.birth_date = CrsBirthQry.FieldAsDateTime( "birth_date" );
   	}
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

void IntLoadPnr( int point_id, int pnr_id, xmlNodePtr resNode )
{
	ProgTrace( TRACE5, "point_id=%d, pnr_id=%d", point_id, pnr_id );
  getTripData( point_id, true );
  VerifyPNR( point_id, pnr_id );
  vector<TWebPax> pnr;
  getPnr( pnr_id, pnr );
  xmlNodePtr node = NewTextChild( resNode, "passengers" );
  for ( vector<TWebPax>::iterator i=pnr.begin(); i!=pnr.end(); i++ ) {
  	xmlNodePtr paxNode = NewTextChild( node, "pax" );
  	NewTextChild( paxNode, "crs_pax_id", i->crs_pax_id );
  	if ( i->crs_pax_id_parent > NoExists )
  		NewTextChild( paxNode, "crs_pax_id_parent", i->crs_pax_id_parent );
  	NewTextChild( paxNode, "surname", i->surname );
  	NewTextChild( paxNode, "name", i->name );
  	if ( i->birth_date > NoExists )
  		NewTextChild( paxNode, "birth_date", DateTimeToStr( i->birth_date, ServerFormatDateTimeAsString ) );
  	NewTextChild( paxNode, "pers_type", i->pers_type );
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
   	xmlNodePtr tidsNode = NewTextChild( paxNode, "tids" );
   	NewTextChild( tidsNode, "crs_pnr_tid", i->crs_pnr_tid );
   	NewTextChild( tidsNode, "crs_pax_tid", i->crs_pax_tid );
   	if ( i->pax_grp_tid > NoExists )
   		NewTextChild( tidsNode, "pax_grp_tid", i->pax_grp_tid );
   	if ( i->pax_tid > NoExists )
   		NewTextChild( tidsNode, "pax_tid", i->pax_tid );
  }
}

void WebRequestsIface::LoadPnr(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE1,"WebRequestsIface::SearchPnr");
  int point_id = NodeAsInteger( "point_id", reqNode );
  int pnr_id = NodeAsInteger( "pnr_id", reqNode );
  IntLoadPnr( point_id, pnr_id, NewTextChild( resNode, "LoadPnr" ) );
}

bool isOwnerFreePlace( int pax_id, const vector<TWebPax> &pnr )
{
  bool res = false;
  for ( vector<TWebPax>::const_iterator i=pnr.begin(); i!=pnr.end(); i++ ) {
  	if ( i->pax_id > NoExists )
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
  	if ( i->pax_id > NoExists && pax_id == i->pax_id ) {
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
    if ( i->pers_type != "ВЗ" )
    	pr_CHIN = true;
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
      if ( wp->pax_id > NoExists )
      	NewTextChild( placeNode, "pax_id", wp->pax_id );
    }
  }
}

void VerifyPax(xmlNodePtr reqNode, xmlDocPtr emulReqDoc, int &pnr_id)
{
	pnr_id=ASTRA::NoExists;
  if (emulReqDoc==NULL)
    throw EXCEPTIONS::Exception("VerifyPax: emulReqDoc=NULL");
  xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulReqDoc);
  emulReqNode=NewTextChild(emulReqNode,"TCkinSavePax");

  int point_id=NodeAsInteger("point_id",reqNode);
  TSearchPnrData PnrData;
	getTripData( point_id, PnrData, true );

	if ( PnrData.act_out != NoExists )
		throw UserException( "MSG.FLIGHT.TAKEOFF" );
	if ( PnrData.web_stage != sOpenWEBCheckIn )
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
    "SELECT rem_code,rem FROM crs_pax_rem WHERE pax_id=:pax_id";
  RemQry.DeclareVariable("pax_id",otInteger);

  xmlNodePtr node,reqPaxNode=NodeAsNode("passengers/pax",reqNode);

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
	int point_id = NodeAsInteger( "point_id", reqNode );
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

  VerifyPax(reqNode, emulReqDoc.docPtr(), pnr_id);
  if (pnr_id==ASTRA::NoExists)
    throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: pnr_id not defined");

  xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulReqDoc.docPtr())->children;
  if (emulReqNode==NULL)
    throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: emulReqNode=NULL");

  bool result=true;

  if (GetNode("segments",emulReqNode)!=NULL) //не только пересадка, но и регистрация
    result=CheckInInterface::SavePax(reqNode, emulReqNode, ediResNode, resNode);

  if (result)
    IntLoadPnr( point_id, pnr_id, NewTextChild( resNode, "SavePax" ) );
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

