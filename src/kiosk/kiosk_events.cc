#include "kiosk_events.h"
#include "exceptions.h"
#define NICKNAME "DJEK"
#include "serverlib/slogger.h"
#include "serverlib/str_utils.h"
#include "oralib.h"
#include "stl_utils.h"
#include "qrys.h"
#include "date_time.h"
#include "xml_unit.h"
#include "date_time.h"
#include "web_main.h"
#include "serverlib/json_packer_heavy.h"
#include "serverlib/json_pack_types.h"

using namespace std;


template<typename T>
struct property_map: public std::map<std::string, T> {
    using std::map<std::string, T>::map;
};


namespace json_spirit
{


template<typename T> struct Traits< property_map<T> >
{
    typedef property_map<T> Type;
    enum { allowEmptyConstructor = 0 };
    static mValue packInt(const Type& i);
    static UnpackResult< Type > unpackInt(const mValue& v);
    static mValue packExt(const Type& i);
    static UnpackResult< Type > unpackExt(const mValue& v);
};

} //namespace json_spirit


#define DEFINE_SOAP_PROPMAP_TO_JSON_PACK_UNPACK_EI(EI) \
template <typename T>\
mValue Traits<property_map<T>>::pack##EI(const property_map<T>& i)\
{\
    mObject ret;\
    for (const auto & val : i) {\
        const auto& key = std::get<0>(val);\
        ret.emplace(key, Traits<T>::pack##EI(std::get<1>(val)));\
    }\
    return ret;\
}\
template <typename T>\
UnpackResult<property_map<T>> Traits<property_map<T>>::unpack##EI(const mValue& v)\
{\
    JSON_ASSERT_TYPE(property_map<T>, v, json_spirit::obj_type);\
    const mObject& obj(v.get_obj());\
    property_map<T>  ts;\
    for (const auto & val : obj) {\
        const auto& key = std::get<0>(val);\
        auto t = Traits<T>::unpack##EI(std::get<1>(val));\
        if (!t) {\
            return UnpackError{"unpack" #EI " failed " + key};\
        }\
        if (!std::get<1>(ts.emplace(key, *t)) ) {\
            return UnpackError{key + " insert failed"};\
        }\
    }\
    return ts;\
}

namespace json_spirit
{
DEFINE_SOAP_PROPMAP_TO_JSON_PACK_UNPACK_EI(Ext)
DEFINE_SOAP_PROPMAP_TO_JSON_PACK_UNPACK_EI(Int)
} //namespace json_spirit




namespace json_spirit
{

  struct KioskServerEvent {
    boost::optional<std::string> application;
    boost::optional<std::string> screen;
    boost::optional<std::string> kioskId;
    boost::optional<std::string> hardwareId;
    boost::optional<std::string> mode;
    boost::optional<std::string> typeRequest;
    boost::optional< property_map<std::vector<boost::optional<std::string>>> > inputParams;
    boost::optional<std::string> timeout;
    std::string time;
    int id;
    KioskServerEvent( const boost::optional<std::string> &vapplication,
                      const boost::optional<std::string> &vscreen,
                      const boost::optional<std::string> &vkioskId,
                      const boost::optional<std::string> &vhardwareId,
                      const boost::optional<std::string> &vmode,
                      const boost::optional<std::string> &vtypeRequest,
                      const boost::optional< property_map<std::vector<boost::optional<std::string>>> > &vparams,
                      const boost::optional<std::string> &vtimeout,
                      std::string vtime,
                      int vid
                       ) {
      application = vapplication;
      screen = vscreen;
      kioskId = vkioskId;

      hardwareId = vhardwareId;
      mode = vmode;
      typeRequest = vtypeRequest;
      inputParams = vparams;
      timeout = vtimeout;
      time = vtime;
      id = vid;
    }
    string session_id();
  };

string KioskServerEvent::session_id()
{
    string result;
    for ( property_map<std::vector<boost::optional<std::string>>>::const_iterator i=inputParams.get().begin(); i!=inputParams.get().end(); i++ ) {
        if(i->first == "sessionId" and not i->second.empty()) {
            result = i->second.begin()->get();
            break;
        }
    }
/*    if(result.empty())
        throw EXCEPTIONS::Exception("KioskServerEvent::session_id(): not defined");*/
    return result;
}

JSON_DESC_TYPE_DECL(KioskServerEvent);
JSON_BEGIN_DESC_TYPE(KioskServerEvent)
    DESC_TYPE_FIELD("application", application)
    DESC_TYPE_FIELD("screen", screen)
    DESC_TYPE_FIELD("kioskId", kioskId)
    DESC_TYPE_FIELD("hardwareId", hardwareId)
    DESC_TYPE_FIELD("mode", mode)
    DESC_TYPE_FIELD("typeRequest", typeRequest)
    DESC_TYPE_FIELD("inputParams", inputParams)
    DESC_TYPE_FIELD("timeout", timeout)
    DESC_TYPE_FIELD("time", time)
    DESC_TYPE_FIELD("id", id)
JSON_END_DESC_TYPE(KioskServerEvent)

}

/*
 *
 *
DROP TABLE kiosk_events;
DROP TABLE kiosk_event_params;
DROP TABLE kiosk_event_errors;

CREATE TABLE kiosk_events (
        id NUMBER(9) NOT NULL,
        type VARCHAR2(50),
        application VARCHAR2(50),
        screen VARCHAR2(50),
        kioskid VARCHAR2(50),
        time DATE NOT NULL,
        ev_order NUMBER(9) NOT NULL
);

CREATE INDEX kiosk_events__AK ON kiosk_events
(
       time                          ASC,
       ev_order                       ASC
);

ALTER TABLE kiosk_events
       ADD CONSTRAINT kiosk_events__PK PRIMARY KEY (id);



CREATE TABLE kiosk_event_params (
        event_id NUMBER(9) NOT NULL,
        num  NUMBER(9) NOT NULL,
        name VARCHAR2(250),
        value VARCHAR2(2000),
        page_no NUMBER(9) NOT NULL
);

CREATE INDEX kiosk_event_params__AK ON kiosk_event_params
(
       event_id                          ASC
);

CREATE INDEX kiosk_event_params__AK1 ON kiosk_event_params
(
       num                         ASC,
       name                        ASC,
       page_no                     ASC
);



CREATE TABLE kiosk_event_errors (
        content VARCHAR2(2000),
        error VARCHAR2(2000),
        reference VARCHAR2(2000),
        time DATE NOT NULL
);

CREATE SEQUENCE kiosk_event__seq
        INCREMENT BY 1
        START WITH 1
        MAXVALUE 999999999
        CYCLE
        ORDER;

*/


struct KioskServerEventContainer {
  std::string JAVA_TIME_FORMAT = "dd.mm.yyyy hh:nn:ss";
  public:
    json_spirit::UnpackResult<json_spirit::KioskServerEvent> event = json_spirit::UnpackError{""};
    std::map<std::string,std::string> headers;
    TDateTime time;
    void parse( xmlNodePtr reqNode ) {
      headers[ "content" ] = "";
      xmlNodePtr node = GetNode( "content", reqNode );
      if ( node != NULL ) {
        headers[ "content" ] = NodeAsString( node );
      }
      node = GetNode( "header", reqNode );
      if ( node != NULL ) {
         node = node->children;
         ProgTrace(TRACE5, "name=%s", (const char*)node->name );
         while ( node != NULL && string((const char*)node->name ) == string( "param" ) ) {
           xmlNodePtr n = node;
           ProgTrace(TRACE5, "name=%s", NodeAsString( "name", n ) );
           ProgTrace(TRACE5, "value=%s", NodeAsString( "value", n ) );
           headers[ NodeAsString( "name", n ) ] = NodeAsString( "value", n );
           node = node->next;
         }
      }
      json_spirit::mValue v;
      bool pr_read = json_spirit::read(headers[ "content" ], v);
      ProgTrace(TRACE5, " json read=%d",  pr_read == true );
      //json_spirit::JsonLogger l;
      event = json_spirit::Traits<json_spirit::KioskServerEvent>::unpackExt(v);
      if ( event.valid() &&
           BASIC::date_time::StrToDateTime( event->time.c_str(),JAVA_TIME_FORMAT.c_str(), time ) == EOF ) { //!!!очень криво, надо, чтобы работал json!!!
         event = json_spirit::UnpackError{"invalid value at field time"};
      }
    }
    void toDB() {
      TQuery Qry( &OraSession );
      if ( event.valid()) {
        try {
          Qry.SQLText =
            "SELECT kiosk_event__seq.nextval AS event_id FROM dual";
          Qry.Execute();
          int event_id = Qry.FieldAsInteger( "event_id" );
          Qry.Clear();
          Qry.SQLText =
            "INSERT INTO kiosk_events(id,type,application,screen,kioskid,time,ev_order,session_id) "
            "VALUES(:event_id,:type,:application,:screen,:kioskid,:time,:ev_order,:session_id) ";
          Qry.CreateVariable( "event_id", otString, event_id );
          Qry.CreateVariable( "type", otString, event->typeRequest == boost::none?string("unknown"):event->typeRequest.get() );
          Qry.CreateVariable( "application", otString, event->application == boost::none?string(""):event->application.get() );
          Qry.CreateVariable( "screen", otString, event->screen == boost::none?string(""):event->screen.get() );
          Qry.CreateVariable( "kioskid", otString, event->kioskId==boost::none?string(""):event->kioskId.get() );
          Qry.CreateVariable( "time", otDate, time );
          ProgTrace( TRACE5, "id=%d", event->id );
          Qry.CreateVariable( "ev_order", otInteger, event->id );
          Qry.CreateVariable( "session_id", otString, event->session_id() );
          Qry.Execute();
          if ( event->inputParams != boost::none ) {
            Qry.Clear();
            Qry.SQLText =
              "INSERT INTO kiosk_event_params(event_id,num,name,value,page_no) "
              " VALUES(:event_id,:num,:name,:value,:page_no)";
            Qry.CreateVariable( "event_id", otInteger, event_id );
            Qry.DeclareVariable( "num", otInteger );
            Qry.DeclareVariable( "name", otString );
            Qry.DeclareVariable( "value", otString );
            Qry.DeclareVariable( "page_no", otInteger );
            int num = 0;
            for ( property_map<std::vector<boost::optional<std::string>>>::const_iterator i=event->inputParams.get().begin(); i!=event->inputParams.get().end(); i++ ) {
              Qry.SetVariable( "name", i->first );
              if ( !i->second.empty() ) {
                for ( vector<boost::optional<std::string>>::const_iterator v=i->second.begin(); v!=i->second.end(); v++ ) {
                  if ( *v == boost::none ) {
                     continue;
                  }
                  Qry.SetVariable( "num", num );
                  string value = v->get();
                  int i=0;
                  while ( !value.empty() ) {
                    Qry.SetVariable( "value", value.substr( 0, 2000 ) );
                    Qry.SetVariable( "page_no", i );
                    Qry.Execute();
                    i++;
                    value.erase( 0, 2000 );
                  }
                  num++;
                }
              }
            }
          }
        }
        catch( EXCEPTIONS::Exception &e ) {
          event = json_spirit::UnpackError{e.what()};
        }
        catch( std::exception &e ) {
          event = json_spirit::UnpackError{e.what()};
        }
        catch(...) {
          event = json_spirit::UnpackError{"unknown error"};
        }
    }
    if ( !event.valid() ) {
      ProgError( STDLOG, "kiosk_event error %s", event.err().text.c_str() );
      Qry.Clear();
      Qry.SQLText =
        "INSERT INTO kiosk_event_errors(content,error,reference,time) "
        " VALUES(:content,:error,:reference,:time)";
      Qry.CreateVariable( "content", otString, headers[ "content" ].substr(0,2000) );
      Qry.CreateVariable( "error", otString, event.err().text.substr(0,2000) );
      Qry.CreateVariable( "reference", otString, string("") );
      Qry.CreateVariable( "time", otDate, BASIC::date_time::Now() );
      tst();
      Qry.Execute();
      tst();
    }
  }
};

namespace KIOSKEVENTS
{

void KioskRequestInterface::Ping(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}

void KioskRequestInterface::EventToServer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  KioskServerEventContainer event;
  event.parse(reqNode);
  event.toDB();
}

void KioskRequestInterface::ViewCraft(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  AstraWeb::WebRequestsIface::IntViewCraft( reqNode, resNode);
}

}
/*
void KioskRequestInterface::ViewKioskEvents(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TDateTime user_time = BASIC::date_time::Now();
  xmlNodePtr node = GetNode( "time", reqNode );
  if ( node != NULL ) {
    user_time = NodeAsDateTime(node);
  }
  else
    user_time -= 1;
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT ID,TYPE,APPLICATION,SCREEN,KIOSKID,TIME FROM kiosk_events WHERE time>=:time "
    " ORDER BY TIME,EV_ORDER  ";
  Qry.CreateVariable("time", otDate, user_time);
  Qry.Execute();
  TQuery PQry( &OraSession );
  PQry.SQLText =
    "SELECT num,name,value,page_no FROM kiosk_event_params "
    " WHERE event_id=:event_id "
    " ORDER BY num,page_no ";
  PQry.DeclareVariable( "event_id", otInteger );
  std::vector<json_spirit::KioskServerEvent> events;
  for ( ; !Qry.Eof; Qry.Next() ) {
    json_spirit::KioskServerEvent event(
                                         Qry.FieldAsString("application"),
                                         Qry.FieldAsString("screen"),
                                         Qry.FieldAsString("kioskId"),
                                         "",
                                         "",
                                         Qry.FieldAsString("type"),
                                         boost::none,
                                         "",
                                         Qry.FieldAsString("time"),
                                         Qry.FieldAsInteger("id")
                                       );


    PQry.SetVariable( "event_id", event.id );
    PQry.Execute();
    property_map<std::vector<boost::optional<std::string>>> params;
    int prior_num = -1;
    string value = "";
    string prior_name, name;
    for ( ; !PQry.Eof; PQry.Next() ) {
       name = PQry.FieldAsString( "name" );
       if ( prior_num != PQry.FieldAsInteger( "num" ) ) {
         if ( !value.empty() ) {
           params[ prior_name ].push_back( value );
           value.clear();
         }
         prior_num = PQry.FieldAsInteger( "num" );
         prior_name = name;
       }
       value += PQry.FieldAsString( "value" );
    }
    if ( !value.empty() ) {
      params[ name ].push_back( value );
    }
    event.inputParams = params;
    events.push_back( event );
  }
  node = NewTextChild( resNode, "events" );
  for ( std::vector<json_spirit::KioskServerEvent>::iterator iev=events.begin(); iev!=events.end(); iev++ ) {
    xmlNodePtr eventNode = NewTextChild( node, "event" );
    NewTextChild( eventNode, "application", iev->application.get() );
    NewTextChild( eventNode, "screen", iev->screen.get() );
    NewTextChild( eventNode, "kioskId", iev->kioskId.get() );
    NewTextChild( eventNode, "typeRequest", iev->typeRequest.get() );
    NewTextChild( eventNode, "time", iev->time );
    NewTextChild( eventNode, "id", iev->id );
    eventNode = NewTextChild( eventNode, "params" );
    for ( property_map<std::vector<boost::optional<std::string>>>::iterator ip=iev->inputParams.get().begin(); ip!=iev->inputParams.get().end(); ip++ ) {
       xmlNodePtr n = NewTextChild( eventNode, "param" );
       NewTextChild( n, "name", ip->first );
       xmlNodePtr n2 = NewTextChild( n, "values" );
       for ( std::vector<boost::optional<std::string>>::iterator iv=ip->second.begin(); iv!=ip->second.end(); iv++ ) {
         NewTextChild( n2, "value", iv->get() );
         ProgTrace( TRACE5, "param name=%s, value=%s", ip->first.c_str(),  iv->get().c_str() );
       }
    }
  }
}
*/


