#include "astra_calls.h"
#include "astra_dates.h"
#include "astra_main.h"
#include "wb_messages.h"
#include "tlg/tlg.h"
#include "salons.h"
#include "images.h"
#include "astra_consts.h"
#include "crafts/ComponCreator.h"
#include "seat_number.h"
#include "date_time.h"
#include "typeb_utils.h"

#include <serverlib/xml_tools.h>
#include <serverlib/exception.h>
#include <serverlib/rip.h>
#include <serverlib/rip_oci.h>
#include <serverlib/dates_oci.h>
#include <serverlib/new_daemon.h>
#include <serverlib/daemon_event.h>
#include <serverlib/monitor_ctl.h>
#include <serverlib/oci8.h>
#include <serverlib/oci8cursor.h>

#include <list>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace AstraCalls {

using namespace OciCpp;
using namespace BASIC::date_time;

#define __DECLARE_CALL__(func_name_, func) \
    if(func_name == func_name_)\
    {\
        try {\
            return func(reqNode, detailsNode);\
        } catch(std::exception& e) {\
            LogWarning(STDLOG) << "Exception cathed during call '" << func_name << "'. " << e.what();\
            NewTextChild(resNode, "error", "Internal error occured during call '" + func_name + "'");\
        } catch(...) {\
            LogWarning(STDLOG) << "Unknown exception catched during call '" << func_name << "'";\
            NewTextChild(resNode, "error", "Internal error occured during call '" + func_name + "'");\
        }\
        RemoveNode(detailsNode);\
        return false;\
    }

//---------------------------------------------------------------------------------------

static std::string make_full_tlg_text(const std::string& source,
                                      const std::string& content)
{
    ASSERT(!source.empty());
    auto orig = TypeB::getOriginator("", "", "", NowUTC(), true/*with_exception*/);
    const std::string sender = ".00" + source;
    return orig.addr + "\xa" + sender + "\n" + content;
}

//---------------------------------------------------------------------------------------

static bool load_tlg(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr argsNode = findNodeR(reqNode, "args");
    ASSERT(argsNode);
    const std::string content = getStrFromXml(argsNode, "text");
    const std::string  source = getStrFromXml(argsNode, "func_called_by");
    LogTrace(TRACE5) << "content:'" << content << "'";
    const std::string tlgText = make_full_tlg_text(source, content);
    LogTrace(TRACE5) << "tlg_text:'" << tlgText << "'";
    auto tlg_id = loadTlg(tlgText);
    NewTextChild(resNode, "tlg_id", tlg_id);
    return true;
}

static bool load_doc(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr argsNode = findNodeR(reqNode, "args");
    ASSERT(argsNode);
    auto             point_id = getIntFromXml(argsNode, "point_id");
    const std::string    type = getStrFromXml(argsNode, "type");
    const std::string content = getStrFromXml(argsNode, "content");
    const std::string  source = getStrFromXml(argsNode, "func_called_by");
    WBMessages::toDB(point_id, WBMessages::MsgTypes().decode(type), content, source);
    return true;
}

static bool get_user_info(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    NewTextChild(resNode, "login", TReqInfo::Instance()->user.login);
    NewTextChild(resNode,  "descr", TReqInfo::Instance()->user.descr);
    NewTextChild(resNode,  "pult",  TReqInfo::Instance()->desk.code);
    TReqInfo::Instance()->user.access.toXML(NewTextChild(resNode, "access"));
    return true;
}
static bool set_blocked(xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "args/point_id", reqNode );
  xmlNodePtr rowNode = GetNode( "args/row", reqNode );
  TSeatRanges seatRanges;
  while ( rowNode &&
          std::string( (const char*)rowNode->name ) == "row" ) {
    TSeat seat( NodeAsString( "@RowNo", rowNode ),
                NodeAsString( "seat/@Ident", rowNode ) );
    NormalizeSeat( seat );
    seatRanges.emplace_back( seat );
    LogTrace(TRACE5) << seat.toString();
    rowNode = rowNode->next;
  }
  LogTrace(TRACE5) << seatRanges.size();
  SALONS2::resetLayers( point_id, ASTRA::TCompLayerType::cltBlockCent,
                        seatRanges, "EVT.COMPON_LAYOUT_MODIFIED.LIBRA_BLOCK_CENT", true );
  return true;
}

static bool get_seating_details(xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "args/point_id", reqNode );
  TQuery Qry(&OraSession);
  resNode = NewTextChild( resNode, "compon" );
  tst();
  if ( !ComponCreator::ComponSetter( point_id ).isLibraMode() ) {
    throw EXCEPTIONS::Exception("Mode is not Libra");
  }
  tst();
  //может измениться componSetter.getCompId()
  ComponCreator::ComponLibraFinder::SetChangesAHMFromPointId( point_id, Qry );
  tst();
  //заново начитываем
  ComponCreator::ComponSetter componSetter( point_id );
  ComponCreator::ComponLibraFinder::AstraSearchResult res = ComponCreator::ComponLibraFinder::checkChangeAHMFromCompId( componSetter.getCompId(), Qry );
  tst();
  if ( !res.isOk( ) ) {
    throw EXCEPTIONS::Exception("Compon is not valid");
  }
  SetProp( resNode, "plan_id", res.plan_id );
  SetProp( resNode, "conf_id", res.conf_id );
  SetProp( resNode, "comp_id", res.comp_id );
  SetProp( resNode, "time_create", BASIC::date_time::DateTimeToStr( res.comps_time_create,BASIC::date_time::ServerFormatDateTimeAsString ) );
  xmlNodePtr personsNode = NewTextChild( resNode, "persons" );
  xmlNodePtr layersNode = NewTextChild( resNode, "layers" );
  xmlNodePtr seatsNode = NewTextChild( resNode, "seats" );
  std::set<std::string> persons;
  std::set<ASTRA::TCompLayerType> layers;
  SALONS2::TSalonList salonList( true );
  salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id ), "", ASTRA::NoExists );
  SALONS2::TGetPassFlags flags;
  flags.setFlag( SALONS2::gpPassenger );
  flags.setFlag( SALONS2::gpTranzits );
  flags.setFlag( SALONS2::gpInfants );
  std::set<ASTRA::TCompLayerType> search_layers;
  for ( int ilayer=0; ilayer<(int)ASTRA::cltTypeNum; ilayer++ ) {
    ASTRA::TCompLayerType layer_type = (ASTRA::TCompLayerType)ilayer;
    BASIC_SALONS::TCompLayerType layer_elem;
    if ( BASIC_SALONS::TCompLayerTypes::Instance()->getElem( layer_type, layer_elem ) &&
         layer_elem.getOccupy() ) {
      search_layers.insert( layer_type );
    }
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT "
    "       pax_grp.airp_arv,pax_grp.airp_dep, "
    "       pax_grp.class, NVL(pax.cabin_class, pax_grp.class) AS cabin_class, "
    "       ckin.get_excess_wt(pax.grp_id, pax.pax_id, pax_grp.excess_wt, pax_grp.bag_refuse) AS excess_wt, "
    "       ckin.get_excess_pc(pax.grp_id, pax.pax_id) AS excess_pc, "
    "       ckin.get_rkAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) rkamount,"
    "       ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) rkweight,"
    "       ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) bagamount,"
    "       ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) bagweight"
    " FROM pax_grp, pax"
    " WHERE pax_grp.grp_id=pax.grp_id AND "
    "       pax.pax_id=:pax_id AND "
    "       pax.wl_type IS NULL";
  Qry.DeclareVariable( "pax_id", otInteger );
  SALONS2::TSectionInfo sectionInfo;
  salonList.getSectionInfo( sectionInfo, flags );
  SALONS2::TLayersSeats layerSeats;
  sectionInfo.GetLayerSeats( layerSeats );
  std::map<int,SALONS2::TSalonPax> paxs;
  sectionInfo.GetPaxs( paxs );
  struct Counters{
    int amount;
    int weight;
    TBagKilos excess_wt;
    TBagPieces excess_pc;
    Counters():excess_wt(0),excess_pc(0) {
      amount = 0;
      weight = 0;
    }
  };
  std::map<std::string,std::map<std::string,Counters>> weights; //направление, класс
  for ( SALONS2::TLayersSeats::iterator ilayer=layerSeats.begin();
        ilayer!=layerSeats.end(); ilayer++ ) {
    if ( search_layers.find( ilayer->first.layer_type ) == search_layers.end() ) {
      continue;
    }
    if ( layers.find( ilayer->first.layer_type ) == layers.end() ) {
      layers.insert( ilayer->first.layer_type );
    }
    if ( paxs.find( ilayer->first.getPaxId() ) == paxs.end() ) {
      LogError(STDLOG) << "pax_id=%" <<  ilayer->first.getPaxId() << " not found";
    }
    xmlNodePtr n = NewTextChild( seatsNode, "passenger" );
    SetProp( n, "pax_id", ilayer->first.getPaxId() );
    LogTrace(TRACE5) <<  paxs[ ilayer->first.getPaxId() ].pers_type;
    Qry.SetVariable( "pax_id", ilayer->first.getPaxId() );
    Qry.Execute();
    std::string gender;
    switch( paxs[ ilayer->first.getPaxId() ].pers_type ) {
      case ASTRA::adult:
        switch ( paxs[ ilayer->first.getPaxId() ].is_female ) {
          case ASTRA::NoExists:
            gender = "A";
            break;
          case 0:
            gender = "M";
            break;
          default:
            gender = "F";
            break;
        }
        break;
      case ASTRA::child:
        gender = "C";
        break;
      case ASTRA::baby:
        gender = "I";
        break;
      default:
        break;
    }
    NewTextChild( n, "gender", gender );
    persons.insert( gender );
    if ( paxs[ ilayer->first.getPaxId() ].pr_infant != ASTRA::NoExists ) {
      NewTextChild( n, "infant" );
    }
    if ( paxs[ ilayer->first.getPaxId() ].crew_type != ASTRA::TCrewType::Unknown ) {
       NewTextChild( n, "crew", ASTRA::TCrewTypes().encode( paxs[ ilayer->first.getPaxId() ].crew_type ) );
    }
    NewTextChild( n, "airp_dep", Qry.FieldAsString( "airp_dep" ) );
    NewTextChild( n, "airp_arv", Qry.FieldAsString( "airp_arv" ) );
    if ( Qry.FieldAsInteger( "rkamount" ) ) {
      NewTextChild( n, "rkamount", Qry.FieldAsInteger( "rkamount" ) );
    }
    if ( Qry.FieldAsInteger( "rkweight" ) ) {
      NewTextChild( n, "rkweight", Qry.FieldAsInteger( "rkweight" ) );
    }
    NewTextChild( n, "class", Qry.FieldAsString( "class" ) );
    weights[Qry.FieldAsString( "airp_arv" )][Qry.FieldAsString( "class" )].amount += Qry.FieldAsInteger( "bagamount" );
    weights[Qry.FieldAsString( "airp_arv" )][Qry.FieldAsString( "class" )].weight += Qry.FieldAsInteger( "bagweight" );
    weights[Qry.FieldAsString( "airp_arv" )][Qry.FieldAsString( "class" )].excess_wt += TBagKilos( Qry.FieldAsInteger( "excess_wt" ) );
    weights[Qry.FieldAsString( "airp_arv" )][Qry.FieldAsString( "class" )].excess_pc += TBagPieces( Qry.FieldAsInteger( "excess_pc" ) );


    n = NewTextChild( n, "pax_seats" );
    for ( TPassSeats::const_iterator iseat=ilayer->second.begin();
          iseat!=ilayer->second.end(); iseat++ ) {
      xmlNodePtr nseat = NewTextChild( n, "item" );
      NewTextChild( nseat, "RowNo", SeatNumber::tryDenormalizeRow( iseat->row ) );
      NewTextChild( nseat, "Ident", SeatNumber::tryDenormalizeLine( iseat->line, true ) );
      NewTextChild( nseat, "layer", EncodeCompLayerType( ilayer->first.layer_type ) );
    }
  }
  for ( const auto& p : persons ) {
    SetProp( NewTextChild( personsNode, "code", p ), "name", (p=="U")?"Unknown":(p=="A")?"Adult":(p=="M")?"Male":(p=="F")?"Female":(p=="C")?"Children":"Baby");
  }
  for ( const auto& p : layers ) {
    BASIC_SALONS::TCompLayerType layer_elem;
    BASIC_SALONS::TCompLayerTypes::Instance()->getElem( p, layer_elem );
    SetProp( NewTextChild( layersNode, "code", EncodeCompLayerType( p ) ), "name", layer_elem.getName().c_str() );
  }

  xmlNodePtr wNode = NewTextChild( resNode, "baggage" );
  for ( const auto& destW : weights ) {
    xmlNodePtr n1 = NewTextChild( wNode, "dest", destW.first );
    for ( const auto &classW : destW.second ) {
      xmlNodePtr classNode = NewTextChild( n1, "class", classW.first );
      if ( classW.second.amount ) {
        SetProp( classNode, "bagamount", classW.second.amount );
      }
      if ( classW.second.weight ) {
        SetProp( classNode, "bagweight", classW.second.weight );
      }
    }
  }
  tst();
  Qry.Clear();
  Qry.SQLText =
    "  SELECT airp_arv, "
    "         NVL(SUM(ckin.get_bagWeight2(grp_id,NULL,NULL,rownum)),0) bag_weight "
    "  FROM pax_grp "
    "  WHERE point_dep=:point_id AND class IS NULL AND pax_grp.status NOT IN ('E') AND bag_refuse=0 "
    "  GROUP BY airp_arv";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  tst();
  xmlNodePtr unaccompNode = nullptr;
  for( ;!Qry.Eof; Qry.Next() ) {
    if ( !unaccompNode ) {
      unaccompNode = NewTextChild( resNode, "unaccompanied_baggage" );
    }
    SetProp( NewTextChild( unaccompNode, Qry.FieldAsString( "airp_arv" ) ), "weight", Qry.FieldAsInteger( "bag_weight" ) );
  }
  return true;
}

static bool call(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    auto func_name = getStrFromXml(reqNode, "func");
    LogTrace(TRACE5) << "func=" << func_name;

    xmlNodePtr detailsNode = NewTextChild(resNode, "details");

    __DECLARE_CALL__("load_tlg",            load_tlg);
    __DECLARE_CALL__("load_doc",            load_doc);
    __DECLARE_CALL__("get_user_info",       get_user_info);
    __DECLARE_CALL__("get_seating_details", get_seating_details);
    __DECLARE_CALL__("set_blocked", set_blocked);

    LogError(STDLOG) << "Unknown astra call '" << func_name << "'";
    NewTextChild(resNode, "error", "Unknown astra call '" + func_name + "'");
    RemoveNode(detailsNode);
    return false;
}

//---------------------------------------------------------------------------------------

static bool callBy(xmlNodePtr reqNode, xmlNodePtr resNode, const std::string& caller)
{
    xmlNodePtr argsNode = findNodeR(reqNode, "args");
    if(argsNode) {
        NewTextChild(argsNode, "func_called_by", caller);
    }
    return call(reqNode, resNode);
}

//---------------------------------------------------------------------------------------

bool callByLibra(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    return callBy(reqNode, resNode, "LIBRA");
}

//---------------------------------------------------------------------------------------

DECL_RIP(AstraCallId, int);

//---------------------------------------------------------------------------------------

class DeferredAstraCall
{
public:
    enum class Status
    {
        Created   = 1,
        Processed = 2,
        Failed    = 3
    };

private:
    AstraCallId       m_id;
    std::string       m_xmlReq;
    std::string       m_xmlRes;
    Dates::DateTime_t m_timeCreate;
    Dates::DateTime_t m_timeHandle;
    Status            m_status;
    std::string       m_source;

public:
    static DeferredAstraCall create(const std::string& xmlReq, const std::string& source);
    static DeferredAstraCall create(xmlNodePtr reqNode, const std::string& source);

    static std::list<DeferredAstraCall> read4handle(unsigned limit);

    static int removeOld();

    const AstraCallId                id() const { return m_id; }
    const std::string&           xmlReq() const { return m_xmlReq; }
    const std::string&           xmlRes() const { return m_xmlRes; }
    const Dates::DateTime_t& timeCreate() const { return m_timeCreate; }
    const Dates::DateTime_t& timeHandle() const { return m_timeHandle; }
    const Status&                status() const { return m_status; }
    const std::string&           source() const { return m_source; }

    void setXmlRes(const std::string& xmlRes) {
        m_xmlRes = xmlRes;
    }
    void setStatus(const Status& status) {
        m_status = status;
    }
    void setTimeHandle(const Dates::DateTime_t& timeHandle) {
        m_timeHandle = timeHandle;
    }

    void writeDb() const;
    void deleteDb() const;
    void updateDb() const;

protected:
    DeferredAstraCall(const AstraCallId& id,
                      const std::string& xmlReq,
                      const Dates::DateTime_t& timeCreate,
                      Status status,
                      const std::string& source);

    static AstraCallId genNextId();
};

//

DeferredAstraCall::DeferredAstraCall(const AstraCallId& id,
                                     const std::string& xmlReq,
                                     const Dates::DateTime_t& timeCreate,
                                     Status status,
                                     const std::string& source)
    : m_id(id),
      m_xmlReq(xmlReq),
      m_timeCreate(timeCreate),
      m_status(status),
      m_source(source)
{}

DeferredAstraCall DeferredAstraCall::create(const std::string& xmlReq,
                                            const std::string& source)
{
    return DeferredAstraCall(DeferredAstraCall::genNextId(),
                             xmlReq,
                             Dates::second_clock::universal_time(),
                             DeferredAstraCall::Status::Created,
                             source);
}

DeferredAstraCall DeferredAstraCall::create(xmlNodePtr reqNode,
                                            const std::string& source)
{
    return create(XMLTreeToText(reqNode->doc), source);
}

std::list<DeferredAstraCall> DeferredAstraCall::read4handle(unsigned limit)
{
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
"select ID, XML_REQ, TIME_CREATE, SOURCE "
"from DEFERRED_ASTRA_CALLS "
"where TIME_CREATE <= :ct and STATUS = :created", &os);

    AstraCallId::base_type id;
    std::string xmlReq, source;
    Dates::DateTime_t timeCreate;
    auto nowUtc = Dates::second_clock::universal_time();

    cur
        .def(id)
        .defClob(xmlReq)
        .def(timeCreate)
        .def(source)
        .bind(":ct",      nowUtc)
        .bind(":created", (int)Status::Created)
        .exec();

    std::list<DeferredAstraCall> lDac;

    unsigned count = 0;
    while(!cur.fen())
    {
        if(limit && ++count > limit) break;

        lDac.push_back(DeferredAstraCall(AstraCallId(id),
                                         xmlReq,
                                         timeCreate,
                                         Status::Created,
                                         source));
    }

    return lDac;
}

int DeferredAstraCall::removeOld()
{
    auto hoursAgo = Dates::second_clock::universal_time() - Dates::hours(12);

    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
"delete from DEFERRED_ASTRA_CALLS where TIME_HANDLE < :time_handle and STATUS = :status", &os);
    cur
        .bind(":time_handle", hoursAgo)
        .bind(":status",      (int)Status::Processed)
        .exec();

    return cur.rowcount();
}

void DeferredAstraCall::writeDb() const
{
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
"insert into DEFERRED_ASTRA_CALLS(ID, XML_REQ, TIME_CREATE, STATUS, SOURCE) "
"values (:id, :xml_req, :xml_res, :time_create, :time_handle, :status, :source)", &os);
    cur
        .bind(":id",          m_id.get())
        .bindClob(":xml_req", m_xmlReq)
        .bind(":time_create", m_timeCreate)
        .bind(":status",      (int)m_status)
        .bind(":source",      m_source)
        .exec();
    if(cur.rowcount() != 1) {
        LogWarning(STDLOG) << __func__ << ": " << cur.rowcount() << " rows were inserted";
    }
}

void DeferredAstraCall::deleteDb() const
{
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
"delete from DEFERRED_ASTRA_CALLS where ID=:id", &os);
    cur
        .bind(":id", m_id.get())
        .exec();
    if(cur.rowcount() != 1) {
        LogWarning(STDLOG) << __func__ << ": " << cur.rowcount() << " rows were deleted";
    }
}

void DeferredAstraCall::updateDb() const
{
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
"update DEFERRED_ASTRA_CALLS set "
"XML_RES=:xml_res, "
"STATUS=:status, "
"TIME_HANDLE=:time_handle "
"where ID=:id", &os);
    cur
            .bind(":id",          m_id.get())
            .bindClob(":xml_res", m_xmlRes)
            .bind(":time_handle", m_timeHandle)
            .bind(":status",      (int)m_status)
            .exec();
    if(cur.rowcount() != 1) {
        LogWarning(STDLOG) << __func__ << ": " << cur.rowcount() << " rows were updated";
    }
}

AstraCallId DeferredAstraCall::genNextId()
{
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
"select DEFERRED_ASTRA_CALLS__SEQ.nextval from DUAL", &os);
    AstraCallId::base_type id;
    cur.def(id).EXfet();
    return AstraCallId(id);
}

//---------------------------------------------------------------------------------------

void HandleDeferredAstraCall(const DeferredAstraCall& dac)
{
    LogTrace(TRACE5) << __func__ << " " << dac.xmlReq();
    auto reqXmlDoc = ASTRA::createXmlDoc(dac.xmlReq());
    ASSERT(reqXmlDoc.docPtr());
    auto resXmlDoc = ASTRA::createXmlDoc("<root/>");
    ASSERT(resXmlDoc.docPtr());

    xmlNodePtr reqRootNode = findNodeR(reqXmlDoc.docPtr()->children, "root");
    ASSERT(reqRootNode);
    xmlNodePtr resRootNode = findNodeR(resXmlDoc.docPtr()->children, "root");
    ASSERT(resRootNode);
    bool result = callBy(reqRootNode, resRootNode, dac.source());
    xmlSetProp(resRootNode, "result", result ? "ok" : "err");

    DeferredAstraCall dacNew = dac;
    dacNew.setXmlRes(XMLTreeToText(resRootNode->doc));
    dacNew.setStatus(result ? DeferredAstraCall::Status::Processed
                            : DeferredAstraCall::Status::Failed);
    dacNew.setTimeHandle(Dates::second_clock::universal_time());

    dacNew.updateDb();
}

#undef __DECLARE_CALL__

}//namespace AstraCalls

/////////////////////////////////////////////////////////////////////////////////////////

using namespace ServerFramework;


class AstraCallsDaemon: public NewDaemon
{
    virtual void init();
public:
    static const char *Name;
    AstraCallsDaemon();
};

//

const char* AstraCallsDaemon::Name = "Astra calls daemon";

AstraCallsDaemon::AstraCallsDaemon()
    : NewDaemon("ASTRA_CALLS")
{}

void AstraCallsDaemon::init()
{
    if(init_locale() < 0) {
        throw EXCEPTIONS::Exception("init_locale failed");
    }
}

//---------------------------------------------------------------------------------------

class AstraCallsCleaner: public DaemonTask
{
public:
    virtual int run(const boost::posix_time::ptime&);
    virtual void monitorRequest() {}
    AstraCallsCleaner();
};

//

AstraCallsCleaner::AstraCallsCleaner()
    : DaemonTask(DaemonTaskTraits::OracleAndHooks())
{}

//---------------------------------------------------------------------------------------

class AstraCallsHandler: public DaemonTask
{
    bool m_exec_me_again;

public:
    virtual int run(const boost::posix_time::ptime&);
    virtual void monitorRequest() {}
    virtual bool doNeedRepeat() { return m_exec_me_again; }
    AstraCallsHandler();
};

//

AstraCallsHandler::AstraCallsHandler()
    : DaemonTask(DaemonTaskTraits::OracleAndHooks())
{}

/////////////////////////////////////////////////////////////////////////////////////////

namespace {

using namespace AstraCalls;

void run_astra_calls_cleaner()
{
    LogTrace(TRACE1) << "Astra calls cleaner run at "
                     << boost::posix_time::second_clock::universal_time() << " (UTC)";

    auto removed = DeferredAstraCall::removeOld();
    LogTrace(TRACE5) << removed << " DAC records were removed";
    monitor_idle_zapr_type(removed, QUEPOT_NULL);
}

size_t run_astra_calls_handler()
{
    LogTrace(TRACE1) << "Astra calls handler run at "
                     << boost::posix_time::second_clock::universal_time() << " (UTC)";

    auto lDac = DeferredAstraCall::read4handle(5);
    LogTrace(TRACE5) << lDac.size() << " DAC records are ready for handle";

    try {
        for(const auto& dac: lDac) {
            HandleDeferredAstraCall(dac);
        }
    } catch(std::exception& e) {
        LogError(STDLOG) << e.what() << ": rollback!";
        ASTRA::rollback();
        return 0;
    }

    monitor_idle_zapr_type(lDac.size(), QUEPOT_NULL);

    return lDac.size();
}

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

int AstraCallsCleaner::run(const boost::posix_time::ptime&)
{
    run_astra_calls_cleaner();
    return 0;
}

//---------------------------------------------------------------------------------------

int AstraCallsHandler::run(const boost::posix_time::ptime&)
{
    m_exec_me_again = run_astra_calls_handler() > 0 ? true : false;
    return 0;
}

//---------------------------------------------------------------------------------------

int main_astra_calls_daemon_tcl(int supervisorSocket, int argc, char *argv[])
{
    AstraCallsDaemon daemon;

    // обработчик
    DaemonEventPtr handleTimer(new TimerDaemonEvent(1));
    handleTimer->addTask(DaemonTaskPtr(new AstraCallsHandler));
    daemon.addEvent(handleTimer);
    // очиститель таблицы с очередью
    DaemonEventPtr cleanTimer(new TimerDaemonEvent(60));
    cleanTimer->addTask(DaemonTaskPtr(new AstraCallsCleaner));
    daemon.addEvent(cleanTimer);

    daemon.run();
    return 0;
}

//---------------------------------------------------------------------------------------

#ifdef XP_TESTING
namespace xp_testing {

    void runAstraCallsHandler_4testsOnly()
    {
        run_astra_calls_handler();
    }

    void runAstraCallsCleaner_4testsOnly()
    {
        run_astra_calls_cleaner();
    }

}//namespace xp_testing
#endif//XP_TESTING
