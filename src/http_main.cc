//#include <arpa/inet.h>
//#include <memory.h>
#include <string>
#include <vector>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include "oralib.h"
#include "exceptions.h"
#include "astra_utils.h"
#include "http_main.h"
#include "astra_locale.h"
#include "serverlib/perfom.h"
#include "serverlib/ourtime.h"
#include "serverlib/str_utils.h"
#include "xml_unit.h"
#include "points.h"
#include "basic.h"

using namespace EXCEPTIONS;

//#include "serverlib/query_runner.h"
#include "serverlib/http_parser.h"
#include "serverlib/query_runner.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
//using namespace ASTRA;
using namespace BASIC;
//using namespace ServerFramework;

namespace AstraHTTP
{

const std::string CLIENT_ID = "CLIENT_ID";
const std::string OPERATION = "OPERATION";
const std::string AUTORIZATION = "Authorization";

using namespace ServerFramework::HTTP;

std::string HTTPClient::toString()
{
  string res = "client_id: " + client_id + ", operation=" + operation + ",user_name=" + user_name + ",password=" + password;
  return res;
}

HTTPClient getHTTPClient(const request& req)
{
  HTTPClient client;
  for (request::Headers::const_iterator iheader=req.headers.begin(); iheader!=req.headers.end(); iheader++){
    if ( iheader->name == CLIENT_ID ) {
      client.client_id = iheader->value;
    }
    if ( iheader->name == OPERATION ) {
      client.operation = iheader->value;
    }
    if ( iheader->name == AUTORIZATION && iheader->value.length() > 6 ) {
      string Authorization = iheader->value.substr( 6 );
      Authorization = StrUtils::b64_decode( Authorization );
      if ( Authorization.find( ":" ) != std::string::npos ) {
        client.user_name = Authorization.substr( 0, Authorization.find( ":" ) - 1 );
        client.password = Authorization.substr( Authorization.find( ":" ) + 1 );
      }
    }
  }
  if ( client.client_id == "SPPUFA" ) {
    client.pult = client.client_id;
  }
  ProgTrace( TRACE5, "client=%s", client.toString().c_str() );
  return client;
}

void HTTPClient::toJXT( const ServerFramework::HTTP::request& req, std::string &header, std::string &body )
{
  header.clear();
  body.clear();
  string http_header = "<" + operation + ">\n<header>\n";
  for (request::Headers::const_iterator iheader=req.headers.begin(); iheader!=req.headers.end(); iheader++){
    http_header += string("<param>") +
                   "<name>" + iheader->name + "</name>\n" +
                   "<value>" + iheader->value + "</value>\n" + "</param>\n";
  }
  http_header += "</header>\n";
  header=(string(45,0)+pult+"  "+client_id+string(100,0)).substr(0,100);
  string sss( "?>" );
  ProgTrace( TRACE5, "HTTPClient::toJXT: req.content=%s", req.content.c_str() );
  string::size_type pos = req.content.find( sss );
  if ( pos == string::npos ) { //only text
    body = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    pos = body.find( sss );
  }
  if ( pos != string::npos ) {
     string content = req.content;
     char str_BOM[4];
     str_BOM[0] = 0xEF;
     str_BOM[1] = 0xBB;
     str_BOM[2] = 0xBF;
     str_BOM[3] = 0x00;
     if ( content.size() > 3 && content.substr(0,3) == str_BOM ) {
       tst();
       content.erase( 0, 3 );
     }
     body += content.c_str();
     body.insert( pos + sss.length(), string("<term><query id=") + "'" + HTTP_JXT_IFACE_ID + "' screen='AIR.exe' opr='" + client_id + "'>" + http_header + "<content>\n" );
     body += string(" </content>\n") + "</" + operation + ">\n</query></term>";
  }
  ProgTrace( TRACE5, "http_main: body=%s", body.c_str() );
}

reply& HTTPClient::fromJXT( std::string res, reply& rep )
{
  rep.status = reply::ok;
  string::size_type pos = res.find( "<answer" );
  ProgTrace( TRACE5, "res=%s", res.c_str() );
  if ( pos != string::npos ) {
    pos = res.find( "/>", pos );
    if ( pos != string::npos ) {
      res.erase( 0, pos + 2 );
      ProgTrace( TRACE5, "res=%s", res.c_str() );
      pos = res.find( "</term>" );
      res.erase( pos );
      ProgTrace( TRACE5, "res=%s", res.c_str() );
    }
  }
  rep.content = /*"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +*/ res;
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
  rep.headers[1].name = "Content-Type";
  ProgTrace( TRACE5, "rep.content=%s", rep.content.c_str() );
  return rep;
}

void http_main(reply& rep, const request& req)
{
  string answer;

  try
  {
    HTTPClient client = getHTTPClient( req );

    InitLogTime(client.pult.c_str());

    char *res = 0;
    int len = 0;
    static ServerFramework::ApplicationCallbacks *ac=
             ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks();
    string header, body;
    client.toJXT( req, header, body );
    ProgTrace( TRACE5, "body.size()=%zu, header.size()=%zu, len=%d", body.size(), header.size(), len );
    int newlen=ac->jxt_proc((const char *)body.data(),body.size(),(const char *)header.data(),header.size(), &res, len);
    ProgTrace( TRACE5, "newlen=%d, len=%d, header.size()=%zu, *res+100=%s", newlen, len, header.size(), res+100 );
    body = string( res + header.size(), newlen - header.size() );
    client.fromJXT( body, rep );
  }
  catch(...)
  {
     ProgError( STDLOG,"HTTP Exception");
     rep.status = reply::internal_server_error;
     rep.headers.resize(2);
     rep.headers[0].name = "Content-Length";
     rep.content = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<error/>";
     rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
     rep.headers[1].name = "Content-Type";
  }
  InitLogTime(NULL);
  return;
}

struct FlightProperty {
  std::string name;
  std::string value;
  FlightProperty(const std::string &vname,
                 const std::string &vvalue) {
    name = upperc( vname );
    name = TrimString( name );
    value = vvalue;
    value = TrimString( value );
  }
};

const string FormatFlightDateTime = "yyyy-mm-dd hh:nn:ss";

struct TCode {
  std::string code;
  TElemFmt fmt;
  TCode() {
    clear();
  }
  void clear() {
    code.clear();
    fmt = efmtUnknown;
  }
};

struct  TParseFlight {
  std::string own_region;
  std::string own_airp;
  std::string error;
  TCode airline;
  string fltNo;
  int flt_no;
  TCode suffix;
  std::string trip_type;
  BASIC::TDateTime scd;
  BASIC::TDateTime est;
  TCode craft;
  std::vector<TCode> airps_in;
  std::vector<TCode> airps_out;
  bool pr_landing;
  std::string status;
  void add_airline( const std::string &value ) {
    try {
      airline.code = ElemToElemId( etAirline, value, airline.fmt, false );
      if ( airline.fmt == efmtUnknown )
        throw EConvertError("");
	    if ( airline.fmt == efmtCodeInter || airline.fmt == efmtCodeICAOInter )
		    trip_type = "м";  //!!!vlad а правильно ли так определять тип рейса? не уверен. Проверка при помощи маршрута. Если в маршруте все п.п. принадлежат одной стране то "п" иначе "м"
      else
  	    trip_type = "п";
    }
    catch( EConvertError &e ) {
	  	throw Exception( "Неизвестная авиакомпания, значение=%s", value.c_str() );
    }
    if ( !fltNo.empty() ) {
       add_fltno( fltNo );
    }
  }
  void add_fltno( const std::string &value ) {
    fltNo = value;
    if ( airline.code.empty() ) {
      return;
    }
    fltNo = airline.code + fltNo;
    string tmp_airline;
    parseFlt( fltNo, tmp_airline, flt_no, suffix.code );
    if ( !suffix.code.empty() ) {
      try {
        suffix.code = ElemToElemId( etSuffix, suffix.code, suffix.fmt, false );
        if ( suffix.fmt == efmtUnknown )
         throw EConvertError("");
      }
      catch( EConvertError &e ) {
    	  throw Exception( "Ошибка формата номера рейса, значение=%s", suffix.code.c_str() );
      }
    }
  }
  void add_scd( const std::string &value ) {
    if ( value.empty() )
		  throw Exception( "Ошибка формата планового времени, значение=%s", value.c_str() );
    std::string tmp_value = value;
    if ( tmp_value.size() == 23 ) {
      tmp_value = tmp_value.substr(0, 19); //отсекаем миллисекунды
    }
    if ( StrToDateTime( tmp_value.c_str(), FormatFlightDateTime.c_str(), scd ) == EOF )
		  throw Exception( "Ошибка формата планового времени, значение=%s", value.c_str() );
    ProgTrace( TRACE5, "own_region=%s", own_region.c_str() );
    try {
	    scd = LocalToUTC( scd, own_region );
	  }
	  catch( boost::local_time::ambiguous_result ) {
		  throw Exception( "Плановое время выполнения рейса определено не однозначно" );
    }
    catch( boost::local_time::time_label_invalid ) {
      throw Exception( "Плановое время выполнения рейса не существует" );
    }
    ProgTrace( TRACE5, "scd=%f, error=%s", scd, error.c_str() );
    if ( est != ASTRA::NoExists ) {
      if ( scd == est ) {
        est = ASTRA::NoExists;
      }
    }
  }
  void add_est( const std::string &value ) {
    if ( value.empty() )
		  throw Exception( "Ошибка формата расчетного времени, значение=%s", value.c_str() );
    std::string tmp_value = value;
    if ( tmp_value.size() == 23 ) {
      tmp_value = tmp_value.substr(0, 19); //отсекаем миллисекунды
    }
    if ( StrToDateTime( tmp_value.c_str(), FormatFlightDateTime.c_str(), est ) == EOF )
		  throw Exception( "Ошибка формата расчетного времени, значение=%s", value.c_str() );
    try {
	    est = LocalToUTC( est, own_region );
	  }
	  catch( boost::local_time::ambiguous_result ) {
		  throw Exception( "Расчетное время выполнения рейса определено не однозначно" );
    }
    catch( boost::local_time::time_label_invalid ) {
      throw Exception( "Расчетное время выполнения рейса не существует" );
    }
    if ( scd != ASTRA::NoExists ) {
      if ( scd == est ) {
        est = ASTRA::NoExists;
      }
    }
  }
  void add_craft( const std::string &value ) {
	  craft.code = value;
  	bool pr_craft_error = true;
  	if ( !craft.code.empty() ) {
 	    try {
        craft.code = ElemCtxtToElemId( ecDisp, etCraft, craft.code, craft.fmt, false );
        pr_craft_error = false;
      }
      catch( EConvertError &e ) {
        craft.code.clear();
      }
    }
	}
  void add_dests( const std::string &value ) {
    typedef boost::char_separator<char> token_func_type;
    typedef boost::tokenizer<token_func_type> tokenizer_type;
    token_func_type sep("-");
    tokenizer_type tok(value, sep);
    bool pr_own = false;
    for ( tokenizer_type::iterator itok=tok.begin(); itok!=tok.end(); itok++ ) {
      if ( *itok == own_airp ) {
        pr_own = true;
        continue;
      }
      TCode airp;
  	  try {
         airp.code = ElemCtxtToElemId( ecDisp, etAirp, *itok, airp.fmt, false );
         if ( airp.fmt == efmtUnknown ) {
           throw EConvertError( "" );
         }
      }
      catch( EConvertError &e ) {
     		throw Exception( "Неизвестный код аэропорта, значение=%s", value.c_str() );
      }
      if ( pr_own ) {
        airps_out.push_back( airp );
      }
      else {
        airps_in.push_back( airp );
      }
    }
  }
  void add_status( const std::string &value ) {
    status = value;
  }
  void add_prlanding( const std::string &value ) {
    std::string tmp_value = upperc( value );
    pr_landing = ( tmp_value != "ВЗЛЕТ" );
  }
  TParseFlight& operator << (const FlightProperty &prop) {
    ProgTrace( TRACE5, "own_region=%s", own_region.c_str() );
    if ( !error.empty() ) {
      return *this;
    }
    try {
      if ( prop.name == "AK" ) {
        add_airline( prop.value );
        return *this;
      }
      if ( prop.name == "NREIS" ) {
        add_fltno( prop.value );
        return *this;
      }
      if ( prop.name == "RASPDATETIME" ) {
        add_scd( prop.value );
        return *this;
      }
      if ( prop.name == "FACTDATETIME" ) {
        add_est( prop.value );
        return *this;
      }
      if ( prop.name == "TYPEVS" ) {
        add_craft( prop.value );
        return *this;
      }
      if ( prop.name == "MARSHRUT" ) {
        add_dests( prop.value );
        return *this;
      }
      if ( prop.name == "REISSTATE" ) {
        add_status( prop.value );
        return *this;
      }
      if ( prop.name == "OP" ) {
        add_prlanding( prop.value );
        return *this;
      }
    }
    catch( Exception &e ) {
      error = string( "property " ) + prop.name + ", value " + prop.value + " " + e.what();
      ProgTrace( TRACE5, "error=%s", error.c_str() );
    }
    return *this;
  }
  void clear() {
    error.clear();
    airline.clear();
    fltNo.clear();
    flt_no = ASTRA::NoExists;
    suffix.clear();
    trip_type.clear();
    scd = ASTRA::NoExists;
    est = ASTRA::NoExists;
    craft.clear();
    airps_in.clear();
    airps_out.clear();
    pr_landing = false;
    status.clear();
  }
  TParseFlight( const std::string &airp ) {
    string city =((TAirpsRow&)base_tables.get("airps").get_row( "code", airp, true )).city;
    own_region = ((TCitiesRow&)base_tables.get("cities").get_row( "code", city, true )).region;
    own_airp = airp;
    ProgTrace( TRACE5, "own_region=%s, own_airp=%s", own_region.c_str(), own_airp.c_str() );
  }
  bool is_valid() {
    return error.empty();
  }
  std::string key() const {
    ProgTrace( TRACE5, "scd=%f, error=%s", scd, error.c_str() );
    string res = airline.code + IntToString( flt_no ) + suffix.code + DateTimeToStr( scd, "dd" );
    return res;
  }
};

void saveFlights( std::map<std::string,map<bool, TParseFlight> > &flights )
{
  ProgTrace( TRACE5, "saveFlights" );
  for ( std::map<std::string,map<bool, TParseFlight> >::iterator iflight = flights.begin();
        iflight != flights.end(); iflight ++ ) {
    TPoints points;
    std::string airline;
    int flt_no;
    std::string suffix;
    std::string airp;
    BASIC::TDateTime scd_in;
    BASIC::TDateTime scd_out;
    bool pr_land = false, pr_takeoff= false;
    int doubleMove_id = ASTRA::NoExists;
    int doublePoint_id = ASTRA::NoExists;
    vector<TCode> airps;
    map<bool, TParseFlight>::iterator fl_in = iflight->second.find( true );
    if ( fl_in != iflight->second.end() && fl_in->second.is_valid() ) {
      airps.clear();
      airline = fl_in->second.airline.code;
      flt_no = fl_in->second.flt_no;
      suffix = fl_in->second.suffix.code;
      airp = fl_in->second.own_airp;
      scd_in = fl_in->second.scd;
      airps.insert( airps.end(), fl_in->second.airps_in.begin(), fl_in->second.airps_in.end() );
      TCode c;
      c.code = ElemCtxtToElemId( ecDisp, etAirp, fl_in->second.own_airp, c.fmt, false );
      airps.push_back( c );
      airps.insert( airps.end(), fl_in->second.airps_out.begin(), fl_in->second.airps_out.end() );
/*      for ( vector<TCode>::iterator iairp= airps.begin(); iairp!= airps.end(); iairp++ ) {*/
        pr_land = points.isDouble( ASTRA::NoExists, airline, flt_no, suffix, airp, scd_in, ASTRA::NoExists, doubleMove_id, doublePoint_id );
        ProgTrace( TRACE5, "fl_in found: airline=%s, flt_no=%d, airp=%s, scd_in=%s, pr_land=%d",
                   airline.c_str(), flt_no, airp.c_str(), DateTimeToStr( scd_in ).c_str(), pr_land );
/*        if ( pr_land ) {
          tst();
          break;
        }
      }*/
    }
    map<bool, TParseFlight>::iterator fl_out = iflight->second.find( false );
    if ( fl_out != iflight->second.end() && fl_out->second.is_valid() ) {
      airps.clear();
      airline = fl_out->second.airline.code;
      flt_no = fl_out->second.flt_no;
      suffix = fl_out->second.suffix.code;
      airp = fl_out->second.own_airp;
      scd_out = fl_out->second.scd;
      airps.insert( airps.end(), fl_out->second.airps_in.begin(), fl_out->second.airps_in.end() );
      TCode c;
      c.code = ElemCtxtToElemId( ecDisp, etAirp, fl_out->second.own_airp, c.fmt, false );
      airps.push_back( c );
      airps.insert( airps.end(), fl_out->second.airps_out.begin(), fl_out->second.airps_out.end() );
      /*for ( vector<TCode>::iterator iairp= airps.begin(); iairp!= airps.end(); iairp++ ) {*/
        pr_takeoff = points.isDouble( ASTRA::NoExists, airline, flt_no, suffix, airp, ASTRA::NoExists, scd_out, doubleMove_id, doublePoint_id );
        ProgTrace( TRACE5, "fl_out found: airline=%s, flt_no=%d, airp=%s, scd_out=%s, pr_takeoff=%d",
                   airline.c_str(), flt_no, airp.c_str(), DateTimeToStr( scd_out ).c_str(), pr_takeoff );
/*        if ( pr_takeoff ) {
          tst();
          break;
        }
      }  */
    }
/*    if ( pr_land || pr_takeoff ) {
      if ( fl_in != iflight->second.end() ) {
        ProgTrace( TRACE5, "flight landing not writed %s", fl_in->second.key().c_str() );
      }
      if ( fl_out != iflight->second.end() ) {
        ProgTrace( TRACE5, "flight takeoff not writed %s", fl_out->second.key().c_str() );
      }
      continue;
    }*/
    tst();
    bool pr_own = false;
    TPointDests dests;
    if ( doubleMove_id != ASTRA::NoExists ) {
      tst();
      points.dests.Load( doubleMove_id );
      tst();
    }
    for ( std::vector<TCode>::iterator iairp=airps.begin(); iairp!=airps.end(); iairp++ ) {
      ProgTrace( TRACE5, "airp=%s", iairp->code.c_str() );
      TPointsDest dest;
      dest.airp = iairp->code;
      dest.airp_fmt = iairp->fmt;
      if ( !pr_own &&
           ( (fl_in != iflight->second.end() && fl_in->second.own_airp == dest.airp) ||
             (fl_out != iflight->second.end() && fl_out->second.own_airp == dest.airp) ) ) {
        ProgTrace( TRACE5, "own airp found" );
        pr_own = true;
      }
      if ( fl_in != iflight->second.end() && fl_in->second.is_valid() ) {
        ProgTrace( TRACE5, "flight in  found flt_no=%d", fl_in->second.flt_no );
        if ( !pr_own ) {
          ProgTrace( TRACE5, "dest in found airp=%s, flt_no=%d", dest.airp.c_str(), fl_in->second.flt_no );
          dest.airline = fl_in->second.airline.code;
          dest.airline_fmt = fl_in->second.airline.fmt;
          dest.flt_no = fl_in->second.flt_no;
          dest.suffix = fl_in->second.suffix.code;
          dest.suffix_fmt = fl_in->second.suffix.fmt;
          dest.craft = fl_in->second.craft.code;
          dest.craft_fmt = fl_in->second.craft.fmt;
          dest.trip_type = fl_in->second.trip_type;
        }
        if ( fl_in->second.own_airp == dest.airp ) {
          dest.scd_in = fl_in->second.scd;
          dest.est_in = fl_in->second.est;
          ProgTrace( TRACE5, "dest in own, set scd_in=%f, est_in=%f", dest.scd_in, dest.est_in );
        }
      }
      if ( fl_out != iflight->second.end() && fl_out->second.is_valid() ) {
        ProgTrace( TRACE5, "flight out  found flt_no=%d", fl_out->second.flt_no );
        if ( pr_own ) {
          ProgTrace( TRACE5, "own airp found, flt_no=%d, airp=%s", fl_out->second.flt_no, dest.airp.c_str() );
          dest.airline = fl_out->second.airline.code;
          dest.airline_fmt = fl_out->second.airline.fmt;
          dest.flt_no = fl_out->second.flt_no;
          dest.suffix = fl_out->second.suffix.code;
          dest.suffix_fmt = fl_out->second.suffix.fmt;
          dest.craft = fl_out->second.craft.code;
          dest.craft_fmt = fl_out->second.craft.fmt;
          dest.trip_type = fl_out->second.trip_type;
        }
        if ( fl_out->second.own_airp == dest.airp ) {
          dest.scd_out = fl_out->second.scd;
          dest.est_out = fl_out->second.est;
          ProgTrace( TRACE5, "set scd_out=%f, est_out=%f", dest.scd_out, dest.est_out );
        }
      }
      tst();
      dests.items.push_back( dest );
    }
    //синхронизация маршрута, но не уже существующих пунктов
    points.dests.sychDests( dests, false, true );
    if ( doubleMove_id != ASTRA::NoExists ) {
      for ( std::vector<TPointsDest>::iterator idest=dests.items.begin(); idest!= dests.items.end(); idest++ ) {
        ProgTrace( TRACE5, "idest->point_id=%d", idest->point_id );
        if ( idest->point_id == ASTRA::NoExists ) {
          continue;
        }
        for ( std::vector<TPointsDest>::iterator jdest=points.dests.items.begin(); jdest!= points.dests.items.end(); jdest++ ) {
          ProgTrace( TRACE5, "idest->point_id=%d, jdest->point_id=%d", idest->point_id,  jdest->point_id );
          if ( idest->point_id == jdest->point_id ) {
            ProgTrace( TRACE5, "jdest->est_in=%f, idest->est_in=%f, jdest->est_out=%f, idest->est_out=%f, jdest->craft=%s, idest->craft=%s",
                       jdest->est_in, idest->est_in, jdest->est_out, idest->est_out, jdest->craft.c_str(), idest->craft.c_str() );
            jdest->est_in = idest->est_in;
            jdest->est_out = idest->est_out;
            jdest->craft = idest->craft;
            jdest->craft_fmt = idest->craft_fmt;
            break;
          }
        }
      }
    }
    points.move_id = doubleMove_id;
    tst();
    try {
      tst();
      if ( !points.dests.items.empty() ) {
        tst();
        points.Save( false );
      }
      if ( fl_in != iflight->second.end() ) {
        fl_in->second.error = "ok";
        tst();
      }
      if ( fl_out != iflight->second.end() ) {
        fl_out->second.error = "ok";
        tst();
      }
      tst();
    }
    catch( Exception &e ) {
      tst();
      if ( fl_in != iflight->second.end() ) {
        fl_in->second.error = e.what();
        tst();
      }
      if ( fl_out != iflight->second.end() ) {
        fl_out->second.error = e.what();
        tst();
      }
    }
  }
}

void HTTPRequestsIface::SaveSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace( TRACE5, "SaveSPP" );
  xmlNodePtr contentNode = GetNode( "content", reqNode );
  if ( contentNode == NULL ) {
    return;
  }
  string buffer = NodeAsString( contentNode );
  ProgTrace( TRACE5, "contentNode=%s", buffer.c_str() );
  typedef boost::char_separator<char> token_func_type;
  typedef boost::tokenizer<token_func_type> tokenizer_type;
  //typedef std::vector<std::string> token_vector_type;
  token_func_type sep("\n");
  token_func_type fields_sep("\t");
  tokenizer_type tok(buffer, sep);
  std::vector<std::string> fields, values;

  typedef std::vector<std::string> splitted_vector_type;
  splitted_vector_type split_vec;

  std::map<std::string,map<bool, TParseFlight> > flights;
  std::string content;
  for ( tokenizer_type::iterator itok=tok.begin(); itok!=tok.end(); itok++ ) {
     string line_buffer = boost::algorithm::trim_copy(*itok);
     if ( line_buffer.empty() ) {
       continue;
     }
    boost::algorithm::split(split_vec, line_buffer, boost::is_any_of("\t"));
    string val;
    if ( itok == tok.begin() ) {  //fields name
       std::copy (
            split_vec.begin(),
            split_vec.end(),
            std::back_inserter<splitted_vector_type>(fields) );
    }
    else {
      values.clear();
      std::copy (
           split_vec.begin(),
           split_vec.end(),
           std::back_inserter<splitted_vector_type>(values) );
    }
    if ( fields.size() != values.size() ) {
      ProgTrace( TRACE5, ">>>fields.size(%zu) != values.size(%zu)", fields.size(), values.size() );
      continue;
    }
    TParseFlight flight( "УФА" );
    for ( std::vector<std::string>::iterator ifield=fields.begin(), ivalue=values.begin();
         ifield!=fields.end(), ivalue!=values.end(); ifield++, ivalue++ ) {
       ProgTrace( TRACE5, "ifield=%s, ivalue=%s", ifield->c_str(), ivalue->c_str() );
       flight<<FlightProperty( *ifield, *ivalue );
    }
    if ( flight.is_valid() ) {
      ProgTrace( TRACE5, "flights insert airline=%s, flt_no=%d, scd=%f, pr_landing=%d",
                 flight.airline.code.c_str(),
                 flight.flt_no,
                 flight.scd,
                 flight.pr_landing );

      try {
        flights[ flight.key() ].insert( make_pair( flight.pr_landing, flight ) );
      }
      catch( ... ) { //double flight
        flight.error = "double flight";
        ProgTrace( TRACE5, ">>> double flight %s", flight.key().c_str() );
      }
    }
    else {
      content += flight.airline.code + IntToString( flight.flt_no ) + flight.suffix.code + DateTimeToStr( flight.scd ) + " -parse error " +
                 flight.error;
      if ( flight.pr_landing ) {
        content += " landing data\n";
      }
      else {
        content += " takeoff data\n";
      }
      ProgTrace( TRACE5, "flight: airline=%s, flt_no=%d, scd=%f, error=%s",
                 flight.airline.code.c_str(),
                 flight.flt_no,
                 flight.scd,
                 flight.error.c_str() );
    }
  }
  tst();
  saveFlights( flights );
  tst();
  for ( std::map<std::string,map<bool, TParseFlight> >::iterator iflight = flights.begin();
        iflight != flights.end(); iflight ++ ) {
    map<bool, TParseFlight>::iterator fl_in = iflight->second.find( true );
    if ( fl_in != iflight->second.end() ) {
      content += fl_in->second.airline.code + IntToString( fl_in->second.flt_no ) + " " + fl_in->second.suffix.code + DateTimeToStr( fl_in->second.scd );
      if ( fl_in->second.error.empty() ) {
        content += " not used";
      }
      else {
        content += " -" + fl_in->second.error;
      }
      content += " landing data\n";
    }
    map<bool, TParseFlight>::iterator fl_out = iflight->second.find( false );
    if ( fl_out != iflight->second.end() ) {
      content += fl_out->second.airline.code + IntToString( fl_out->second.flt_no ) + " " + fl_out->second.suffix.code + DateTimeToStr( fl_out->second.scd );
      if ( fl_out->second.error.empty() ) {
        content += " not used";
      }
      else {
        content += " -" + fl_out->second.error;
      }
      content += " takeoff data\n";
    }
  }
  NodeSetContent( resNode, content );
  ProgTrace( TRACE5, "finish, context=%s", content.c_str() );
}


} //end namespace AstraHTTP

/////////////////////////////////////////////////////////////////////////////////////////////////////////

