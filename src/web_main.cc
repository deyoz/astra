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
#include "seats.h"
#include "images.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "convert.h"
#include "basic.h"
#include "astra_misc.h"
#include "print.h"
#include "web_main.h"

namespace AstraWeb
{

using namespace std;
using namespace EXCEPTIONS;
using namespace ASTRA;
using namespace SEATS2;
using namespace BASIC;

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
	TDateTime scd_out;
	string craft;
	string airp_dep;
	string city_dep;
	map<TStage, TDateTime> stages;

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
		  throw UserException( "���� �� ������" );
		else
			return false;
	tst();
	if ( Qry.FieldAsInteger( "pr_del" ) == -1 )
		if ( pr_throw )
		  throw UserException( "���� 㤠���" );
		else
			return false;
	tst();
	if ( Qry.FieldAsInteger( "pr_del" ) == 1 )
		if ( pr_throw )
		  throw UserException( "���� �⬥���" );
		else
			return false;
	tst();
	if ( Qry.FieldAsInteger( "pr_reg" ) == 0 )
		if ( pr_throw )
		  throw UserException( "�⬥���� ॣ������ �� ३�" );
		else
			return false;
	tst();
	if ( !Qry.FieldIsNULL( "act_out" ) )
		if ( pr_throw )
		  throw UserException( "���� �뫥⥫" );
		else
			return false;
	tst();
	TTripStages tripStages( point_id );
	TStage stage = tripStages.getStage( stWEB );
	ProgTrace( TRACE5, "stage=%d", (int)stage );
	if ( stage != sOpenWEBCheckIn )
		if ( pr_throw )
		  throw UserException( "��������� �� �����" );
		else
			return false;
  tst();
	TBaseTable &baseairps = base_tables.get( "airps" );
	TBaseTable &basecities = base_tables.get( "cities" );

	SearchPnrData.airline = ElemIdToElemCtxt( ecDisp, etAirline, Qry.FieldAsString( "airline" ), Qry.FieldAsInteger( "airline_fmt" ) );
	SearchPnrData.flt_no = Qry.FieldAsInteger( "flt_no" );
	SearchPnrData.suffix = ElemIdToElemCtxt( ecDisp, etSuffix, Qry.FieldAsString( "suffix" ), Qry.FieldAsInteger( "suffix_fmt" ) );
	SearchPnrData.airp_dep = ElemIdToElemCtxt( ecDisp, etAirp, Qry.FieldAsString( "airp" ), Qry.FieldAsInteger( "airp_fmt" ) );
	SearchPnrData.city_dep = ((TAirpsRow&)baseairps.get_row( "code", Qry.FieldAsString( "airp" ), true )).city;
	string region = ((TCitiesRow&)basecities.get_row( "code", SearchPnrData.city_dep, true )).region;
	SearchPnrData.scd_out = UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), region );
	SearchPnrData.craft = ElemIdToElemCtxt( ecDisp, etCraft, Qry.FieldAsString( "craft" ), Qry.FieldAsInteger( "craft_fmt" ) );
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
	tst();
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
    "SELECT 1 "
    " FROM crs_pnr,tlg_binding "
    "WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
    "      crs_pnr.pnr_id=:pnr_id AND "
    "      tlg_binding.point_id_spp=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "pnr_id", otInteger, pnr_id );
	Qry.Execute();
  if ( Qry.Eof )
  	throw UserException( "���ଠ�� �� ���ᠦ�ࠬ �� �������" );
}

bool findPnr( const string &surname, const string &pnr_addr,
              const string &ticket_no, const string &document,
              TSearchPnrData &SearchPnrData )
{
	ProgTrace( TRACE5, "surname=%s, pnr_addr=%s, ticket_no=%s, document=%s",
	           surname.c_str(), pnr_addr.c_str(), ticket_no.c_str(), document.c_str() );
  SearchPnrData.pnr_id = NoExists;
  if ( surname.empty() )
  	throw UserException( "�� ������ 䠬���� ���ᠦ��" );
  if ( pnr_addr.empty() && ticket_no.empty() && document.empty() )
  	throw UserException( "�� ������ ���ਨ ���᪠" );

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

  for ( ; !Qry.Eof; Qry.Next() ) { // �஡�� �� �ᥬ �������� ���ᠦ�ࠬ
  	// �� �஢��塞 ��⠫��� ���ᠦ�஢ � ��㯯�, � ���ன �� ������ ���ᠦ�� 㤮���⢮���騩 ����� ���᪠
  	if ( SearchPnrData.pnr_id > NoExists && SearchPnrData.pnr_id == Qry.FieldAsInteger( "pnr_id" ) )
  		continue;

  	bool pr_find = true;
  	if ( !ticket_no.empty() ) { // �᫨ ����� ���� �� ������, � ������ �஢���
      QryTicket.SetVariable( "pax_id", Qry.FieldAsInteger( "pax_id" ) );
      QryTicket.Execute();
      pr_find = ( !QryTicket.Eof );
    };
    ProgTrace( TRACE5, "after search ticket, pr_find=%d", pr_find );
    if ( pr_find && !document.empty() ) { // �᫨ �।. �஢�ઠ 㤮��. � �᫨ ����� ���㬥��, � ������ �஢���
    	QryDoc.SetVariable( "pax_id", Qry.FieldAsInteger( "pax_id" ) );
    	QryDoc.Execute();
    	pr_find = ( QryDoc.Eof );
    }
    ProgTrace( TRACE5, "after search document, pr_find=%d", pr_find );
    if ( pr_find ) { // �᫨ �।. �஢�ન 㤮��.
    	pr_find = false;
    	pnraddrs.clear();
  	  QryPnrAddr.SetVariable( "pnr_id", Qry.FieldAsInteger( "pnr_id" ) );
    	QryPnrAddr.Execute();
    	for ( ; !QryPnrAddr.Eof; QryPnrAddr.Next() ) { // �஡�� �� �ᥬ ���ᠬ � ���� �� �᫮��� + ���������� ����� � ���ᠬ�
    		TPnrAddr addr;
    		addr.airline = QryPnrAddr.FieldAsString( "airline" );
    		addr.addr = QryPnrAddr.FieldAsString( "addr" );
    		pnraddrs.push_back( addr );
    		if ( pnr_addr.empty() || pnr_addr == addr.addr )
    			pr_find = true;
    	}
    }

    ProgTrace( TRACE5, "after search pnr_addr, pr_find=%d", pr_find );

    if ( pr_find ) { // �஢�ઠ �� ᮢ������� ��ய��� �ਫ�� �� pnr � ���
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

  	if ( pr_find ) { // ��諨
  		tst();
  		if ( fcount )
  			throw UserException( "������� ����� ������ ���ᠦ��. ������� ���ਨ ���᪠" );
  		SearchPnrData.pnraddrs = pnraddrs;
  		SearchPnrData.cls = Qry.FieldAsString( "class" );
  		SearchPnrData.subcls = Qry.FieldAsString( "subclass" );
  		SearchPnrData.bag_norm = 20;
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
  		fcount++;
  	}
  }
  return fcount;
}

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
   	throw UserException( "�� ������ ������������" );
  try {
    SearchPnrData.airline = ElemCtxtToElemId( ecDisp, etAirline, SearchPnrData.airline, fmt, false );
  }
  catch( EConvertError &e ) {
  	throw UserException( "�������⭠� ������������, ���祭��=%s", SearchPnrData.airline.c_str() );
  }

  string str_flt_no = NodeAsString( "flt_no", reqNode, "" );
	if ( StrToInt( str_flt_no.c_str(), SearchPnrData.flt_no ) == EOF )
		throw Exception( "���ࠢ��쭮 ����� ����� ३�, ���祭��=%s", str_flt_no.c_str() );
	if ( SearchPnrData.flt_no > 99999 || SearchPnrData.flt_no <= 0 )
		throw Exception( "���ࠢ��쭮 ����� ����� ३�, ���祭��=%s", str_flt_no.c_str() );

	SearchPnrData.suffix = NodeAsString( "suffix", reqNode, "" );
	SearchPnrData.suffix = TrimString( SearchPnrData.suffix );
	if ( SearchPnrData.suffix.size() > 1 )
		throw UserException( "���ࠢ��쭮 ����� ���䨪� ३�, ���祭��=%s", SearchPnrData.suffix.c_str() );
  try {
   SearchPnrData.suffix = ElemCtxtToElemId( ecDisp, etSuffix, SearchPnrData.suffix, fmt, false );
  }
  catch( EConvertError &e ) {
  	throw UserException( "���ࠢ��쭮 ����� ���䨪� ३�, ���祭��=%s", SearchPnrData.suffix.c_str() );
  }
  string str_scd_out = NodeAsString( "scd_out", reqNode, "" );
	str_scd_out = TrimString( str_scd_out );
  if ( str_scd_out.empty() )
		throw UserException( "�� ������ �������� �६� �뫥� ३�" );
	else
		if ( StrToDateTime( str_scd_out.c_str(), "dd.mm.yyyy hh:nn:ss", SearchPnrData.scd_out ) == EOF )
			throw UserException( "���ࠢ��쭮 ������ �������� �६� �뫥� ३�, ���祭��=%s", str_scd_out.c_str() );
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
  modf( SearchPnrData.scd_out, &sd );
	Qry.CreateVariable( "first_date", otDate, sd-1 );
	Qry.CreateVariable( "last_date", otDate, sd+1 );
	Qry.Execute();
	ProgTrace( TRACE5, "airline=%s, flt_no=%d, suffix=|%s|, sd=%s",
	           SearchPnrData.airline.c_str(), SearchPnrData.flt_no, SearchPnrData.suffix.c_str(),
	           DateTimeToStr( sd ).c_str() );
	if ( Qry.Eof )
		throw UserException( "���� �� ������" );
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
		throw UserException( "���� �� ������" );
  if ( SearchPnrData.pnr_id == NoExists )
  	throw UserException( "���ᠦ��� �� �������" );

  xmlNodePtr node = NewTextChild( resNode, "SearchFlt" );
  NewTextChild( node, "point_id", SearchPnrData.point_id );
  NewTextChild( node, "airline", SearchPnrData.airline );
  NewTextChild( node, "flt_no", SearchPnrData.flt_no );
  if ( !SearchPnrData.suffix.empty() )
    NewTextChild( node, "suffix", SearchPnrData.suffix );
  NewTextChild( node, "craft", SearchPnrData.craft );
  NewTextChild( node, "scd_out", DateTimeToStr( SearchPnrData.scd_out, ServerFormatDateTimeAsString ) );
  NewTextChild( node, "airp_dep", SearchPnrData.airp_dep );
  NewTextChild( node, "city_dep", SearchPnrData.city_dep );
  NewTextChild( node, "scd_in", DateTimeToStr( SearchPnrData.scd_in, ServerFormatDateTimeAsString ) );
  NewTextChild( node, "airp_arv", SearchPnrData.airp_arv );
  NewTextChild( node, "city_arv", SearchPnrData.city_arv );
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
  NewTextChild( node, "pnr_id", SearchPnrData.pnr_id );
  NewTextChild( node, "subclass", SearchPnrData.subcls );
  NewTextChild( node, "bag_norm", SearchPnrData.bag_norm );
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
1. �᫨ ��-� 㦥 ��砫 ࠡ���� � pnr (�����,ࠧ���騪 PNL)
2. �᫨ ���ᠦ�� ��ॣ����஢����, � ࠧ���騪 PNL �⠢�� �ਧ��� 㤠�����
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
  	   pax_tid != Qry.FieldAsInteger( "pax_grp_tid" ) )
  	throw UserException( "��������� � ��㯯� �ந��������� � �⮩�� ॣ����樨. ������� �����" ); //???
}

void getPnr( int pnr_id, vector<TWebPax> &pnr )
{
	pnr.clear();
	TQuery StatusQry(&OraSession);
	StatusQry.SQLText =
	  "SELECT DECODE(client_type, 'WEBC','web_checked','agent_checked') AS checkin_status "
    "FROM web_clients "
    "WHERE web_clients.user_id=:user_id";
  StatusQry.DeclareVariable( "user_id", otInteger );
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
    "       pax_grp.user_id pax_grp_user_id"
    " FROM crs_pnr,crs_pax,pax,pax_grp,crs_inf "
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
	  throw UserException( "���ଠ�� �� ���ᠦ�ࠬ �� �������" );
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
  	if ( !Qry.FieldIsNULL( "pax_id" ) ) {
  		if ( Qry.FieldIsNULL( "pax_grp_user_id" ) )
  			pax.checkin_status = "agent_checked";
  		else { // ��।������
/*!!!��� �⫠���  			StatusQry.SetVariable( "user_id", Qry.FieldAsInteger( "pax_grp_user_id" ) );
  			StatusQry.Execute();
  			if ( StatusQry.Eof )
  		    pax.checkin_status = "agent_checked";
  		  else
  		  	pax.checkin_status = StatusQry.FieldAsString( "checkin_status" );*/
  		  pax.checkin_status = "web_checked";
  		}
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

void WebRequestsIface::LoadPnr(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE1,"WebRequestsIface::SearchPnr");
  int point_id = NodeAsInteger( "point_id", reqNode );
  int pnr_id = NodeAsInteger( "pnr_id", reqNode );
	ProgTrace( TRACE5, "point_id=%d, pnr_id=%d", point_id, pnr_id );
  getTripData( point_id, true );
  VerifyPNR( point_id, pnr_id );
  vector<TWebPax> pnr;
  getPnr( pnr_id, pnr );
  xmlNodePtr node = NewTextChild( resNode, "LoadPnr" );
  node = NewTextChild( node, "passengers" );
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


/*
1. �� ������ �᫨ ���ᠦ�� ����� ᯥ�. �������� (६�ન MCLS) - ���� �롨ࠥ� ⮫쪮 ���� � ६�ઠ�� �㦭��� ��������.
�� ������ , �᫨ ᠫ�� �� ࠧ��祭?
2. ���� ��㯯� ���ᠦ�஢, ������� 㦥 ��ॣ����஢���, ������� ���.
*/
void WebRequestsIface::ViewCraft(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE1,"WebRequestsIface::SearchPnr");
  int point_id = NodeAsInteger( "point_id", reqNode );
  int pnr_id = NodeAsInteger( "pnr_id", reqNode );
  string crs_class, crs_subclass;
  string pass_rem;
  vector<TWebPax> pnr;
  getTripData( point_id, true );
  VerifyPNR( point_id, pnr_id );
  getPnr( pnr_id, pnr );
  for ( vector<TWebPax>::iterator i=pnr.begin(); i!=pnr.end(); i++ ) {
  	if ( i->pax_id > NoExists )
  		continue;
  	crs_class = i->pass_class;
  	crs_subclass = i->pass_subclass;
  	break;
  }
  if ( crs_class.empty() )
  	throw UserException( "�� ����� ����� ���㦨�����" );
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
  try {
    Salons.Read();
  }
  catch( UserException ue ) {
    showErrorMessage( ue.what() );
  }
  xmlNodePtr node = NewTextChild( resNode, "ViewCraft" );
  node = NewTextChild( node, "salons" );
  for( vector<TPlaceList*>::iterator placeList = Salons.placelists.begin();
       placeList != Salons.placelists.end(); placeList++ ) {
    xmlNodePtr placeListNode = NewTextChild( node, "placelist" );
    SetProp( placeListNode, "num", (*placeList)->num );
    int xcount=0, ycount=0;
    for ( TPlaces::iterator place = (*placeList)->places.begin();
          place != (*placeList)->places.end(); place++ ) {
      if ( !place->visible )
       continue;
      xmlNodePtr placeNode = NewTextChild( placeListNode, "place" );
      NewTextChild( placeNode, "x", place->x );
      NewTextChild( placeNode, "y", place->y );
      if ( place->x > xcount )
      	xcount = place->x;
      if ( place->y > ycount )
      	ycount = place->y;
      NewTextChild( placeNode, "seat_no", denorm_iata_row( place->yname ) + denorm_iata_line( place->xname, Salons.getLatSeat() ) );
      if ( !place->elem_type.empty() ) {
      	if ( place->elem_type != PARTITION_ELEM_TYPE )
     	    NewTextChild( placeNode, "elem_type", ARMCHAIR_ELEM_TYPE );
     	  else
     	  	NewTextChild( placeNode, "elem_type", PARTITION_ELEM_TYPE );
     	}

     	bool pr_free = false;
     	int pax_id = NoExists;
     	if ( place->isplace && !place->clname.empty() && place->clname == crs_class ) {
     		bool pr_first = true;
     		for( std::vector<TPlaceLayer>::iterator l=place->layers.begin(); l!=place->layers.end(); l++ ) { // ���஢�� �� �ਮ���
     			if ( pr_first &&
     				   l->layer_type != cltUncomfort &&
     				   l->layer_type != cltSmoke &&
     				   l->layer_type != cltUnknown ) {
     				pr_first = false;
     				pr_free = ( ( l->layer_type == cltPNLCkin ||
     				              l->layer_type == cltProtCkin ) && isOwnerFreePlace( l->pax_id, pnr ) );
     				if ( pr_free )
     					break;
     			}

     			if ( l->layer_type == cltCheckin ||
     				   l->layer_type == cltTCheckin ||
     				   l->layer_type == cltGoShow ||
     				   l->layer_type == cltTranzit ) {
     				pr_first = false;
            if ( isOwnerPlace( l->pax_id, pnr ) )
     				  pax_id = l->pax_id;
     			}
     	  }

     	  pr_free = ( pr_free || pr_first );

        if ( pr_free && subcls_rems.IsSubClsRem( crs_subclass, pass_rem ) ) {
          for ( vector<TRem>::iterator i=place->rems.begin(); i!=place->rems.end(); i++ ) {
          	if ( i->rem == pass_rem ) {
          		pr_free = !i->pr_denial;
          		break;
          	}
          }
        }
      }
      NewTextChild( placeNode, "status", !pr_free );
      if ( pax_id > NoExists )
      	NewTextChild( placeNode, "pax_id", pax_id );
    }
    SetProp( placeListNode, "xcount", xcount + 1 );
    SetProp( placeListNode, "ycount", ycount + 1 );
  }
}

void WebRequestsIface::SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	ProgTrace(TRACE1,"WebRequestsIface::SavePax");
}


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
	 "SELECT grp_id FROM pax WHERE pax_id=:pax_id";
	Qry.CreateVariable( "pax_id", otInteger, pax_id );
	Qry.Execute();
	if ( Qry.Eof )
		throw UserException( "���ᠦ�� �� ������" );
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
}

} //end namespace AstraWeb
