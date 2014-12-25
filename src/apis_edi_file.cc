//
// C++ Implementation: apis_edi_file
//
// Description:
//
//
// Author: anton <anton@whale>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "apis_edi_file.h"
#include "exceptions.h"
#include "tlg/view_edi_elements.h"
#include "config.h"
#include "tlg/tlg.h"
#include "astra_misc.h"
#include "file_queue.h"

#include <edilib/edi_func_cpp.h>
#include <edilib/edi_astra_msg_types.h>
#include <edilib/edi_sess.h>

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <time.h>
#include <sstream>

#define NICKNAME "ANTON"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>


namespace Paxlst
{
using namespace edilib;
using namespace edifact;


static const char* UnhNumber = "1";
static const char* VerNum = "D";
static const char* CntrlAgn = "UN";
static const char* Chset = "UNOA";
static const int   SyntaxVer = 4;


std::string createIataCode( const std::string& flight,
                            const BASIC::TDateTime& destDateTime,
                            const std::string& destDateTimeFmt )
{
    std::ostringstream iata;
    iata << flight;
    iata << BASIC::DateTimeToStr( destDateTime, destDateTimeFmt );
    return iata.str();
}

std::string createEdiPaxlstFileName( const std::string& carrierCode,
                                     const int& flightNumber,
                                     const std::string& flightSuffix,
                                     const std::string& origin,
                                     const std::string& destination,
                                     const BASIC::TDateTime& departureDate,
                                     const std::string& ext,
                                     unsigned partNum )
{
    ostringstream f;
    f << carrierCode << flightNumber << flightSuffix;

    std::ostringstream fname;
    fname << carrierCode
          << (f.str().size()<6?string(6-f.str().size(),'0'):"") << flightNumber
          << flightSuffix
          << origin << destination;
    fname << BASIC::DateTimeToStr( departureDate, "yyyymmdd" );
    fname << "." << ext;
    if( partNum )
        fname << ".PART" << std::setfill('0') << std::setw(2) << partNum;
    return fname.str();
}

static std::string createEdiInterchangeReference()
{
    std::ostringstream ref;
    ref << time( NULL );
    return ref.str();
}

static UnhElem::SeqFlag getSeqFlag( unsigned partNum, unsigned partsCnt )
{
    UnhElem::SeqFlag seqFlag = UnhElem::Middle;
    if( partNum == 1 )
        seqFlag = UnhElem::First;
    if( partNum == partsCnt )
        seqFlag = UnhElem::Last;
    return seqFlag;
}

static void collectPaxlstMessage( _EDI_REAL_MES_STRUCT_* pMes,
                                  const PaxlstInfo& paxlst,
                                  const BASIC::TDateTime& nowUtc,
                                  unsigned partNum,
                                  unsigned partsCnt,
                                  unsigned totalCnt )
{
    ResetEdiPointW( pMes );

    // UNB
    viewUnbElement( pMes, UnbElem( paxlst.senderCarrierCode(),
                                   paxlst.recipientCarrierCode() ) );
    if (paxlst.settings().viewUNGandUNE())
    {
      // UNG
      viewUngElement( pMes, UngElem( "PAXLST",
                                     paxlst.senderName(),
                                     paxlst.senderCarrierCode(),
                                     paxlst.recipientName(),
                                     paxlst.recipientCarrierCode(),
                                     nowUtc,
                                     UnhNumber,
                                     CntrlAgn,
                                     VerNum,
                                     paxlst.settings().mesRelNum() ) );
    }

    // UNH
    viewUnhElement( pMes, UnhElem( "PAXLST",
                                   VerNum,
                                   paxlst.settings().mesRelNum(),
                                   CntrlAgn,
                                   paxlst.settings().mesAssCode(),
                                   partNum,
                                   getSeqFlag( partNum, partsCnt ) ) ) ;

    // BGM
    viewBgmElement( pMes, BgmElem( paxlst.type()==PaxlstInfo::FlightPassengerManifest?"745":"250",
                                   paxlst.docId() ) );

    if( !paxlst.partyName().empty() )
    {
        SetEdiSegGr( pMes, SegGrElement( 1, 0 ) );

        PushEdiPointW( pMes );
        SetEdiPointToSegGrW( pMes, SegGrElement( 1, 0 ) );

        // NAD
        viewNadElement( pMes, NadElem( "MS", paxlst.partyName() ) );

        if( !paxlst.phone().empty() || !paxlst.fax().empty() || !paxlst.email().empty() )
        {
            // COM
            viewComElement( pMes, ComElem( paxlst.phone(),
                                           paxlst.fax(),
                                           paxlst.email() ) );
        }

        PopEdiPointW( pMes );
    }

    SetEdiSegGr( pMes, SegGrElement( 2, 0 ) );
    PushEdiPointW( pMes );
    SetEdiPointToSegGrW( pMes, SegGrElement( 2, 0 ) );

    // TDT
    viewTdtElement( pMes, TdtElem( "20", paxlst.flight(), paxlst.carrier() ) );

    SetEdiSegGr( pMes, SegGrElement( 3, 0 ) );
    PushEdiPointW( pMes );
    SetEdiPointToSegGrW( pMes, SegGrElement( 3, 0 ) );
    // LOC departure
    viewLocElement( pMes, LocElem( LocElem::Departure, paxlst.depPort() ) );
    // DTM departure
    viewDtmElement( pMes, DtmElem( DtmElem::Departure, paxlst.depDateTime(), "201" ) );
    PopEdiPointW( pMes );


    SetEdiSegGr( pMes, SegGrElement( 3, 1 ) );
    PushEdiPointW( pMes );
    SetEdiPointToSegGrW( pMes, SegGrElement( 3, 1 ) );
    // LOC arrival
    viewLocElement( pMes, LocElem( LocElem::Arrival, paxlst.arrPort() ) );
    // DTM arrival
    viewDtmElement( pMes, DtmElem( DtmElem::Arrival, paxlst.arrDateTime(), "201" ) );
    PopEdiPointW( pMes );

    PopEdiPointW( pMes );


    PassengersList_t passList = paxlst.passengersList();
    int segmGroupNum = 0;
    for( std::list< Paxlst::PassengerInfo >::const_iterator it = passList.begin();
           it != passList.end(); ++it, segmGroupNum++ )
    {
        SetEdiSegGr( pMes, SegGrElement( 4, segmGroupNum ) );

        PushEdiPointW( pMes );
        SetEdiPointToSegGrW( pMes, SegGrElement( 4, segmGroupNum ) );

        // NAD
        viewNadElement( pMes, NadElem( paxlst.type()==PaxlstInfo::FlightPassengerManifest?"FL":"FM",
                                       it->surname(),
                                       it->first_name(),
                                       it->second_name(),
                                       it->street(),
                                       it->city(),
                                       it->countrySubEntityCode(),
                                       it->postalCode(),
                                       it->destCountry() ) );
        // ATT
        viewAttElement( pMes, AttElem( "2", it->sex() ) );
        // DTM
        viewDtmElement( pMes, DtmElem( DtmElem::DateOfBirth, it->birthDate() ) );

        int locNum = 0;
        if( !it->CBPPort().empty() )
        {
            // LOC
            viewLocElement( pMes, LocElem( LocElem::CustomsAndBorderProtection, it->CBPPort() ), locNum++ );
        }
        if( !it->depPort().empty() )
        {
            // LOC
            viewLocElement( pMes, LocElem( LocElem::StartJourney, it->depPort() ), locNum++ );
        }
        if( !it->arrPort().empty() )
        {
            // LOC
            viewLocElement( pMes, LocElem( LocElem::FinishJourney, it->arrPort() ), locNum++ );
        }
        if( !it->residCountry().empty() )
        {
            // LOC
            viewLocElement( pMes, LocElem( LocElem::CountryOfResidence, it->residCountry() ), locNum++ );
        }
        if( !it->birthCountry().empty() )
        {
            // LOC
            viewLocElement( pMes, LocElem( LocElem::CountryOfBirth,
                                           it->birthCountry(),
                                           it->birthCity(),
                                           it->birthRegion()), locNum++ );
        }

        if( !it->nationality().empty() )
        {
            // NAT
            viewNatElement( pMes, NatElem( "2", it->nationality() ) );
        }

        if( !it->reservNum().empty() )
        {
            // RFF
            viewRffElement( pMes, RffElem( "AVF", it->reservNum() ) );
        }

        if( !it->docType().empty() || !it->docNumber().empty() )
        {
            SetEdiSegGr( pMes, SegGrElement( 5, 0 ) );
            PushEdiPointW( pMes );
            SetEdiPointToSegGrW( pMes, SegGrElement( 5, 0 ) );

            // DOC
            viewDocElement( pMes, DocElem( it->docType(), it->docNumber(), paxlst.settings().respAgnCode() ) );

            if( it->docExpirateDate() != ASTRA::NoExists )
            {
                // DTM
                viewDtmElement( pMes, DtmElem( DtmElem::DocExpireDate, it->docExpirateDate() ) );
            }


            if( !it->docCountry().empty() )
            {
                // LOC
                viewLocElement( pMes, LocElem( LocElem::DocCountry, it->docCountry() ) );
            }

            PopEdiPointW( pMes );
        }

        PopEdiPointW( pMes );
    }

    // CNT
    viewCntElement( pMes, CntElem( paxlst.type()==PaxlstInfo::FlightPassengerManifest?
                                     CntElem::PassengersTotal:
                                     CntElem::CrewTotal,
                                   totalCnt ) );
    if (paxlst.settings().viewUNGandUNE())
    {
      // UNE
      viewUneElement( pMes, UneElem( UnhNumber ) );
    }
}

static std::string ediMessageToStr( _EDI_REAL_MES_STRUCT_ *pMes )
{
    try
    {
        return WriteEdiMessage( pMes );
    }
    catch( edilib::Exception& e )
    {
        throw EXCEPTIONS::Exception( e.what() );
    }
}

static std::string createEdiPaxlstString( const PaxlstInfo& paxlst,
                                          const std::string& ediRef,
                                          unsigned partNum,
                                          unsigned partsCnt,
                                          unsigned totalCnt )
{
    BASIC::TDateTime nowUtc = BASIC::NowUTC();

    edi_mes_head edih;
    memset( &edih, 0, sizeof(edih) );
    edih.syntax_ver = SyntaxVer;
    edih.mes_num = 1;
    strcpy( edih.chset, Chset );
    strcpy( edih.to, paxlst.recipientName().c_str() );
    strcpy( edih.date, BASIC::DateTimeToStr( nowUtc, "yymmdd" ).c_str() );
    strcpy( edih.time, BASIC::DateTimeToStr( nowUtc, "hhnn" ).c_str() );
    strcpy( edih.from, paxlst.senderName().c_str() );
    strcpy( edih.acc_ref, paxlst.iataCode().c_str() );
    strcpy( edih.other_ref, "" );
    strcpy( edih.assoc_code, "" );
    strcpy( edih.our_ref, ediRef.c_str() );
    strcpy( edih.FseId, paxlst.settings().appRef().c_str() );
    strcpy( edih.unh_number, UnhNumber );
    strcpy( edih.ver_num, VerNum );
    strcpy( edih.rel_num, paxlst.settings().mesRelNum().c_str() );
    strcpy( edih.cntrl_agn, CntrlAgn );

    _EDI_REAL_MES_STRUCT_* pMes = GetEdiMesStructW();
    if( GetEdiMsgTypeByType( PAXLST, &edih ) )
        throw EXCEPTIONS::Exception( "GetEdiMsgTypeByType failed!" );

    CreateMesByHead( &edih );
    if( !pMes )
        throw EXCEPTIONS::Exception( "pMes is null" );

    collectPaxlstMessage( pMes, paxlst, nowUtc, partNum, partsCnt, totalCnt );
    return "UNA:+.? '\n" + ediMessageToStr( pMes );
}

static void splitPaxlst( std::list< PaxlstInfo >& splitted,
                         const PaxlstInfo& paxlst,
                         unsigned partSize )
{
    PassengersList_t passList = paxlst.passengersList(), iterList;
    for( PassengersList_t::const_iterator it = passList.begin();
           it != passList.end(); ++it )
    {
        iterList.push_back( *it );
        if( ( iterList.size() == partSize ) || ( it == --passList.end() ) )
        {
            PaxlstInfo newPaxlst( paxlst );
            newPaxlst.setPassengersList( iterList );
            splitted.push_back( newPaxlst );
            iterList.clear();
        }
    }
}

//-----------------------------------------------------------------------------

void PaxlstInfo::addPassenger( const PassengerInfo& pass )
{
    if( pass.surname().empty() )
        throw EXCEPTIONS::Exception( "Empty passenger's surname!" );
    m_passList.push_back( pass );
}

std::string PaxlstInfo::toEdiString() const
{
    checkInvariant();
    return createEdiPaxlstString( *this, createEdiInterchangeReference(), 1, 1, passengersList().size() );
}

std::vector< std::string > PaxlstInfo::toEdiStrings( unsigned maxPaxPerString ) const
{
    checkInvariant();
    std::list< PaxlstInfo > splitted;
    splitPaxlst( splitted, *this, maxPaxPerString );
    LogTrace(TRACE3) << "paxlst splitted into " << splitted.size() << " parts";
    if( splitted.empty() )
        return std::vector< std::string >();
    std::vector< std::string > res;
    std::string ediRef = createEdiInterchangeReference();
    unsigned partNum = 0,
            partsCnt = splitted.size();
    BOOST_FOREACH( const PaxlstInfo& paxlst, splitted )
    {
        res.push_back( createEdiPaxlstString( paxlst,
                                              ediRef,
                                              ++partNum,
                                              partsCnt,
                                              passengersList().size() ) );
    }

    return res;
}

void PaxlstInfo::toXMLFormat(xmlNodePtr emulApisNode, const int pax_num, const int crew_num, const int version) const
{
  // Make segment "Message"
  if(GetNode("Message", emulApisNode) == NULL) {
    BASIC::TDateTime nowUtc = BASIC::NowUTC();
    xmlNodePtr messageNode = NewTextChild(emulApisNode, "Message");
    NewTextChild(messageNode, "Destination", "GTB");
    xmlNodePtr systemNode = NewTextChild(messageNode, "System");
    SetProp(systemNode, "Application", "DCS ASTRA");
    SetProp(systemNode, "Organization", "SIRENA-TRAVEL");
    SetProp(systemNode, "ApplicationVersion", 1);
    xmlNodePtr contactNode = NewTextChild(systemNode, "Contact");
    NewTextChild(contactNode, "Name", "SIRENA-TRAVEL");
    NewTextChild(messageNode, "CreateDateTime",
                 BASIC::DateTimeToStr(nowUtc, "yyyy-mm-dd'T'hh:nn:00"));
    NewTextChild(messageNode, "SentDateTime",
                 BASIC::DateTimeToStr(nowUtc, "yyyy-mm-dd"));
    NewTextChild(messageNode, "EnvelopeID",
                 generate_envelope_id(senderCarrierCode()));
    NewTextChild(messageNode, "Owner", "DCS ASTRA");
    NewTextChild(messageNode, "Identifier", get_msg_identifier());
    NewTextChild(messageNode, "Version", version);
    NewTextChild(messageNode, "Context", version?"Update":"Original");
  }
  // Make segment "Flight"
  if(GetNode("Flight", emulApisNode) == NULL) {
    xmlNodePtr flightNode = NewTextChild(emulApisNode, "Flight");
    SetProp(flightNode, "AllCrewFlag", pax_num?"false":"true");
    SetProp(flightNode, "CAR", iataCode());
    SetProp(flightNode, "PassengerCount", pax_num);
    SetProp(flightNode, "CrewCount", crew_num);
    SetProp(flightNode, "TotalCount", pax_num + crew_num);
    xmlNodePtr opfltidNode = NewTextChild(flightNode, "OperatingFlightId");
    xmlNodePtr carrierNode = NewTextChild(opfltidNode, "Carrier");
    SetProp(carrierNode, "CodeType", settings().mesAssCode());
    NewTextChild(carrierNode, "CarrierCode",carrier());
    NewTextChild(opfltidNode, "FlightNumber",flight());
    NewTextChild(flightNode, "ScheduledDepartureDateTime",
                 BASIC::DateTimeToStr(depDateTime(), "yyyy-mm-dd'T'hh:nn:00"));
    NewTextChild(flightNode, "DepartureAirport",depPort());
    NewTextChild(flightNode, "EstimatedArrivalDateTime",
                 BASIC::DateTimeToStr(arrDateTime(), "yyyy-mm-dd'T'hh:nn:00"));
    NewTextChild(flightNode, "ArrivalAirport",arrPort());
    xmlNodePtr FlightLegsNode = NewTextChild(flightNode, "FlightLegs");
    FlightLegstoXML(FlightLegsNode);
  }
  // Make segment "Travellers"
  xmlNodePtr travellersNode = GetNode("Travellers", emulApisNode);
  if(travellersNode == NULL)
    travellersNode = NewTextChild(emulApisNode, "Travellers");

  for( std::list< Paxlst::PassengerInfo >::const_iterator it = m_passList.begin();
         it != m_passList.end(); ++it)
  {
    xmlNodePtr travellerNode = NewTextChild(travellersNode, "Traveller");
    SetProp(travellerNode, "GoShow", it->goShow()?"true":"false");
    SetProp(travellerNode, "NoShow", "false");
    xmlNodePtr flyerNode,checkInNode,boardingNode;
    if (!it->prBrd()) {
      checkInNode = NewTextChild(travellerNode, "CheckIn");
      flyerNode = NewTextChild(checkInNode, "DCS_Traveller");
      SetProp(flyerNode, "Type", it->persType());
      if (!it->seats().empty()) {
        xmlNodePtr seatsNode = NewTextChild(checkInNode, "CheckInSeats");
        SetProp(seatsNode, "NumberOfSeats", it->seats().size());
        for(vector< pair<int, string> >::const_iterator i=it->seats().begin();i!=it->seats().end();i++)
        {
          xmlNodePtr seatNode = NewTextChild(seatsNode, "CheckInSeat");
          SetProp(seatNode, "Number", IntToString(i->first) + i->second);
          SetProp(seatNode, "Row", i->first);
          SetProp(seatNode, "Column", i->second);
        }
      }
      xmlNodePtr TicketsNode = NewTextChild(checkInNode, "Tickets");
      xmlNodePtr TicketNode = NewTextChild(TicketsNode, "Ticket");
      NewTextChild(TicketNode, "TicketNumber", it->ticketNumber());
    }
    else
    {
      boardingNode = NewTextChild(travellerNode, "Boarding");
      flyerNode = NewTextChild(boardingNode, "Flyer");
      SetProp(flyerNode, "Type", type()?"FM":"FL");
      xmlNodePtr itineraryNode = NewTextChild(boardingNode, "FlyerItinerary");
      SetProp(itineraryNode, "JourneyCommence", it->depPort());
      SetProp(itineraryNode, "JourneyConclude", it->arrPort());
      for(vector< pair<int, string> >::const_iterator i=it->seats().begin();i!=it->seats().end();i++)
      {
        xmlNodePtr referenceNode = NewTextChild(boardingNode, "Reference");
        SetProp(referenceNode, "ReferenceCode", "SEA");
        NewTextChild(referenceNode, "ReferenceIdentifier", IntToString(i->first) + i->second);
      }
      if(!it->destCountry().empty() && !it->city().empty() && !it->street().empty()) {
        xmlNodePtr addressNode = NewTextChild(boardingNode, "FlyerAddress");
        SetProp(addressNode, "Type", "Inbound");
        NewTextChild(addressNode, "AddressLine", it->street());
        NewTextChild(addressNode, "City", it->city());
        if (!it->countrySubEntityCode().empty()) {
          xmlNodePtr provinceNode = NewTextChild(addressNode, "ProvinceState");
          SetProp(provinceNode, "Name", it->countrySubEntityCode());
        }
        xmlNodePtr countryNode = NewTextChild(addressNode, "Country");
        SetProp(countryNode, "ISO3166Code", it->destCountry());
        if (!it->postalCode().empty())
          NewTextChild(addressNode, "PostalZipCode", it->postalCode());
      }
    }
    SetProp(flyerNode, "InfantIndicator", (it->persType()=="Infant")?"true":"false");
    xmlNodePtr nameNode = NewTextChild(flyerNode, "Name");
    NewTextChild(nameNode, "Surname", it->surname());
    NewTextChild(nameNode, "FirstName", it->first_name());
    if (!it->second_name().empty()) NewTextChild(nameNode, "MiddleName", it->second_name());

    NewTextChild(flyerNode, "DateOfBirth", BASIC::DateTimeToStr(it->birthDate(), "yyyy-mm-dd"));
    NewTextChild(flyerNode, "Gender", it->sex());
    NewTextChild(flyerNode, "Nationality", it->nationality());
    if (!it->residCountry().empty()) NewTextChild(flyerNode, "CountryOfResidence", it->residCountry());
    if (!it->birthCountry().empty()) NewTextChild(flyerNode, "CountryOfBirth", it->birthCountry());

    xmlNodePtr docNode = NewTextChild(flyerNode, "TravelDocument");
    SetProp(docNode, "TypeCode", it->docType());
    NewTextChild(docNode, "Number", it->docNumber());
    NewTextChild(docNode, "IssueCountry", it->docCountry());
    NewTextChild(docNode, "ExpiryDate", BASIC::DateTimeToStr(it->docExpirateDate(), "yyyy-mm-dd"));
  }
}

void FlightLeg::toXML(xmlNodePtr FlightLegsNode) const
{
  xmlNodePtr legNode = NewTextChild(FlightLegsNode, "FlightLeg");
  SetProp(legNode, "LocationQualifier", loc_qualifier);
  SetProp(legNode, "Airport", airp);
  SetProp(legNode, "Country", country);
  if (sch_in != ASTRA::NoExists)
    SetProp(legNode, "ArrivalDateTime", BASIC::DateTimeToStr(sch_in, "yyyy-mm-dd'T'hh:nn:00"));
  if (sch_out != ASTRA::NoExists)
    SetProp(legNode, "DepartureDateTime", BASIC::DateTimeToStr(sch_out, "yyyy-mm-dd'T'hh:nn:00"));
}

void FlightLegs::FlightLegstoXML(xmlNodePtr FlightLegsNode) const {
  for (std::vector<FlightLeg>::const_iterator iter=begin(); iter != end(); iter++)
     iter->toXML(FlightLegsNode);
}

void FlightLegs::MakeFlightLegs(int first_point) {
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT airp,scd_in,scd_out,country "
    "FROM points,airps,cities "
    "WHERE points.airp=airps.code AND airps.city=cities.code "
    "AND :first_point IN (first_point,point_id) "
    "AND points.pr_del=0 ORDER BY point_num " ;
  Qry.CreateVariable("first_point",otInteger,first_point);
  Qry.Execute();

  for(;!Qry.Eof;Qry.Next())
  {
    TAirpsRow &airp = (TAirpsRow&)base_tables.get("airps").get_row("code",Qry.FieldAsString("airp"));
    if (airp.code_lat.empty()) throw Exception("airp.code_lat empty (code=%s)",airp.code.c_str());
    string tz_region=AirpTZRegion(airp.code);
    BASIC::TDateTime scd_in_local,scd_out_local;
    scd_in_local = scd_out_local = ASTRA::NoExists;
    if (!Qry.FieldIsNULL("scd_out"))
      scd_out_local	= UTCToLocal(Qry.FieldAsDateTime("scd_out"),tz_region);
    if (!Qry.FieldIsNULL("scd_in"))
      scd_in_local = UTCToLocal(Qry.FieldAsDateTime("scd_in"),tz_region);
    TCountriesRow &countryRow = (TCountriesRow&)base_tables.get("countries").get_row("code",Qry.FieldAsString("country"));
    if (countryRow.code_iso.empty()) throw Exception("countryRow.code_iso empty (code=%s)",countryRow.code.c_str());
    push_back(FlightLeg(airp.code_lat, countryRow.code_iso, scd_in_local, scd_out_local));
  }
  /* Fill in LocationQualifier. Code set:
  87 : airport initial arrival in target country.
  125: last departure airport before arrival in target country.
  130: final destination airport in target country.
  92: in-transit airport. */
  std::string target_country;
  bool change_flag = false;
  vector<FlightLeg>::reverse_iterator previos, next;
  for (previos=rbegin(), (next=rbegin())++; next!=rend(); previos++, next++) {
    if(previos==rbegin()) target_country = previos->Country();
    if(change_flag) next->setLocQualifier(92);
    else if(previos->Country() != next->Country() && previos->Country() == target_country) {
      previos->setLocQualifier(87);
      next->setLocQualifier(125);
      change_flag = true;
    }
    else if(previos==rbegin())
      previos->setLocQualifier(130);
    else previos->setLocQualifier(92);
  }
}

void PaxlstInfo::checkInvariant() const
{
    if( passengersList().size() < 1 || passengersList().size() > 99999 )
        throw EXCEPTIONS::Exception( "Bad paxlst size!" );

    if( senderName().empty() )
        throw EXCEPTIONS::Exception( "Empty sender name!" );
}

const std::string generate_envelope_id (const std::string& airl)
{
  boost::uuids::uuid uuid = boost::uuids::random_generator()();
  std::stringstream ss;
  ss << uuid;
  return airl + string("-") + ss.str();
}

const std::string get_msg_identifier ()
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT apis_id__seq.nextval vid FROM dual";
  Qry.Execute();
  std::stringstream ss;
  ss << string("ASTRA") << setw(7) << setfill('0') << Qry.FieldAsString("vid");
  return ss.str();
}

bool get_trip_apis_param (const int point_id, const std::string& format, const std::string& param_name, int& param_value)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT param_value "
    "FROM trip_apis_params "
    "WHERE point_id=:point_id AND format=:format "
    "AND param_name=:param_name ";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.CreateVariable("format", otString, format);
  Qry.CreateVariable("param_name", otString, param_name);
  Qry.Execute();
  if (Qry.Eof) return false;
  param_value = Qry.FieldAsInteger("param_value");
  return true;
}

void set_trip_apis_param(const int point_id, const std::string& format, const std::string& param_name, const int param_value)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "BEGIN "
    "  UPDATE trip_apis_params SET param_value=:param_value "
    "  WHERE point_id=:point_id AND format=:format "
    "  AND param_name=:param_name; "
    "  IF SQL%ROWCOUNT=0 THEN "
    "    INSERT INTO trip_apis_params(point_id, format, param_name, param_value)"
    "    VALUES (:point_id, :format, :param_name, :param_value);"
    "  END IF; "
    "END;";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.CreateVariable("format", otString, format);
  Qry.CreateVariable("param_name", otString, param_name);
  Qry.CreateVariable("param_value", otInteger, param_value);
  try
  {
    Qry.Execute();
  }
  catch(EOracleError E)
  {
    if (E.Code!=1) throw;
  };
}

void put_in_queue(XMLDoc& doc)
{
  std::string airp, airline, flt_no;
  std::map<std::string, std::string> file_params;
  TFileQueue::add_sets_params(airp, airline, flt_no, OWN_POINT_ADDR(),
                              "APIS_TR", 1, file_params);
  std::string content = GetXMLDocText(doc.docPtr());
  std::string search("soapenvEnvelope");
  std::string replace("soapenv:Envelope");
  size_t pos = 0;
  while ((pos = content.find(search, pos)) != std::string::npos) {
       content.replace(pos, search.length(), replace);
       pos += replace.length();
  }
  if(not file_params.empty())
    TFileQueue::putFile(OWN_POINT_ADDR(), OWN_POINT_ADDR(),
                        "APIS_TR", file_params, content);
}

}//namespace Paxlst


//-----------------------------------------------------------------------------

#ifdef XP_TESTING

#include "xp_testing.h"

using namespace xp_testing;

namespace
{
    void init()
    {
        ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()->connect_db();
        edifact::init_edifact();
    }

    void tear_down()
    {
    }

    Paxlst::PaxlstInfo makePaxlst1()
    {
        Paxlst::PaxlstInfo paxlstInfo(Paxlst::PaxlstInfo::FlightPassengerManifest, "");
        paxlstInfo.settings().setViewUNGandUNE(true);

        paxlstInfo.setPartyName( "CDGkoAF" );

        paxlstInfo.setSenderName( "1h" );
        paxlstInfo.setRecipientName( "CzApIs" );

        Paxlst::PassengerInfo pass1;
        pass1.setSurname( "StRaNsKy" );
        pass1.setSex( "M" );

        paxlstInfo.addPassenger( pass1 );

        return paxlstInfo;
    }

    Paxlst::PaxlstInfo makePaxlst3()
    {
        Paxlst::PaxlstInfo paxlstInfo(Paxlst::PaxlstInfo::FlightPassengerManifest, "");
        paxlstInfo.settings().setViewUNGandUNE(true);

        paxlstInfo.setPartyName( "cdgKoaf" );
        paxlstInfo.setPhone( "0148642106" );
        paxlstInfo.setFax( "0148643999" );

        paxlstInfo.setSenderName( "1h" );
        paxlstInfo.setSenderCarrierCode( "zZ" );
        paxlstInfo.setRecipientName( "CzApIs" );
        paxlstInfo.setRecipientCarrierCode( "fR" );
        paxlstInfo.setIataCode( "OK688/071008/1310" );

        paxlstInfo.setFlight( "OK688" );
        paxlstInfo.setDepPort( "PrG" );
        BASIC::TDateTime depDate = ASTRA::NoExists, arrDate = ASTRA::NoExists;
        BASIC::StrToDateTime( "08.10.07 10:45:00", depDate ); //"0710081045"
        paxlstInfo.setDepDateTime( depDate );
        paxlstInfo.setArrPort( "BCN" );
        BASIC::StrToDateTime( "08.10.07 13:10:00", arrDate ); //"0710081310"
        paxlstInfo.setArrDateTime( arrDate );

        Paxlst::PassengerInfo pass1;
        pass1.setSurname( "STRANSKY" );
        pass1.setFirstName( "JAROSLAV VICtOROVICH" );
        pass1.setSex( "M" );
        BASIC::TDateTime bd1 = ASTRA::NoExists;
        BASIC::StrToDateTime( "10.06.67 00:00:00", bd1 ); //"670610"
        pass1.setBirthDate( bd1 );
        pass1.setDepPort( "ZdN" );
        pass1.setArrPort( "bcN" );
        pass1.setNationality( "CZe" );
        pass1.setReservNum( "Z9WkH" );
        pass1.setDocType( "i" );
        pass1.setDocNumber( "102865098" );

        Paxlst::PassengerInfo pass2;
        pass2.setSurname( "kovacs" );
        pass2.setFirstName( "PETR" );
        pass2.setSex( "M" );
        BASIC::TDateTime bd2 = ASTRA::NoExists;
        BASIC::StrToDateTime( "09.12.69 00:00:00", bd2 ); //"691209"
        pass2.setBirthDate( bd2 );
        pass2.setDepPort( "ZDN" );
        pass2.setArrPort( "BCN" );
        pass2.setNationality( "CZE" );
        pass2.setReservNum( "Z9WJK" );
        pass2.setDocType( "p" );
        pass2.setDocNumber( "35485167" );
        BASIC::TDateTime expd1 = ASTRA::NoExists;
        BASIC::StrToDateTime( "11.09.08 00:00:00", expd1 );
        pass2.setDocExpirateDate( expd1 );

        Paxlst::PassengerInfo pass3;
        pass3.setSurname( "LESKA" );
        pass3.setFirstName( "PAVEL" );
        pass3.setSex( "M" );
        BASIC::TDateTime bd3 = ASTRA::NoExists;
        BASIC::StrToDateTime( "02.05.76 00:00:00", bd3 ); //"760502"
        pass3.setBirthDate( bd3 );
        pass3.setDepPort( "VIE" );
        pass3.setArrPort( "BCN" );
        pass3.setNationality( "CZE" );
        pass3.setReservNum( "z57l3" );
        pass3.setDocType( "P" );
        pass3.setDocNumber( "34356146" );
        pass3.setDocCountry( "RUS" );

        paxlstInfo.addPassenger( pass1 );
        paxlstInfo.addPassenger( pass2 );
        paxlstInfo.addPassenger( pass3 );

        return paxlstInfo;
    }

    Paxlst::PaxlstInfo makePaxlst3_long()
    {
        Paxlst::PaxlstInfo paxlstInfo(Paxlst::PaxlstInfo::FlightPassengerManifest, "");
        paxlstInfo.settings().setViewUNGandUNE(true);

        paxlstInfo.setPartyName( "CDGKOAFXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        paxlstInfo.setPhone( "0148642106XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        paxlstInfo.setFax( "0148643999XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );

        paxlstInfo.setSenderName( "1HXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        paxlstInfo.setSenderCarrierCode( "ZZXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        paxlstInfo.setRecipientName( "CzApIsXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        paxlstInfo.setRecipientCarrierCode( "FRXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        paxlstInfo.setIataCode( "OK688/071008/1310XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );

        paxlstInfo.setFlight( "OK688XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );

        paxlstInfo.setDepPort( "PRGXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        BASIC::TDateTime depDate = ASTRA::NoExists;
        BASIC::StrToDateTime( "08.10.07 10:45:00", depDate ); //"0710081045"
        paxlstInfo.setDepDateTime( depDate );

        paxlstInfo.setArrPort( "BCNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        BASIC::TDateTime arrDate = ASTRA::NoExists;
        BASIC::StrToDateTime( "08.10.07 13:10:00", arrDate ); //"0710081310"
        paxlstInfo.setArrDateTime( arrDate );


        Paxlst::PassengerInfo pass1;
        pass1.setSurname( "STRANSKYXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setFirstName( "JAROSLAV VICTOROVICHXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setSex( "M" );
        BASIC::TDateTime bd1 = ASTRA::NoExists;
        BASIC::StrToDateTime( "10.06.67 00:00:00", bd1 ); //"670610"
        pass1.setBirthDate( bd1 );
        pass1.setDepPort( "ZDNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setArrPort( "BCNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setNationality( "CZEXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setDocNumber( "Z9WKHXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setDocType( "IXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setDocNumber( "102865098XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );


        Paxlst::PassengerInfo pass2;
        pass2.setSurname( "KOVACSXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setFirstName( "PETRXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setSex( "MXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        BASIC::TDateTime bd2 = ASTRA::NoExists;
        BASIC::StrToDateTime( "09.12.69 00:00:00", bd2 ); //"691209"
        pass2.setBirthDate( bd2 );
        pass2.setDepPort( "ZDNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setArrPort( "BCNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setNationality( "CZEXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setReservNum( "Z9WJKXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setDocType( "PXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setDocNumber( "35485167XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        BASIC::TDateTime expd2 = ASTRA::NoExists;
        BASIC::StrToDateTime( "11.09.08 00:00:00", expd2 );
        pass2.setDocExpirateDate( expd2 );


        Paxlst::PassengerInfo pass3;
        pass3.setSurname( "LESKAXXXXXXXXXXXXXXXXXXXXXdXXXXXXXXXXXXXXXXX" );
        pass3.setFirstName( "PAVELXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setSex( "MXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        BASIC::TDateTime bd3 = ASTRA::NoExists;
        BASIC::StrToDateTime( "02.05.76 00:00:00", bd3 ); //"760502"
        pass3.setBirthDate( bd3 );
        pass3.setDepPort( "VIEXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setArrPort( "BCNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setNationality( "CZEXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setReservNum( "Z57L3XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setDocType( "PXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setDocNumber( "34356146XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setNationality( "RUSXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setCity( "MOSCOWXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setStreet( "ARBATXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );

        paxlstInfo.addPassenger( pass1 );
        paxlstInfo.addPassenger( pass2 );
        paxlstInfo.addPassenger( pass3 );

        return paxlstInfo;
    }

    Paxlst::PaxlstInfo makePaxlst5()
    {
        Paxlst::PaxlstInfo paxlstInfo( makePaxlst3() );

        Paxlst::PassengerInfo pass4;
        pass4.setSurname( "PUTIN" );
        pass4.setFirstName( "VOVA" );
        pass4.setSex( "M" );
        BASIC::TDateTime bd4 = ASTRA::NoExists;
        BASIC::StrToDateTime( "02.05.52 00:00:00", bd4 );
        pass4.setBirthDate( bd4 );
        pass4.setDepPort( "VIE" );
        pass4.setArrPort( "BCN" );
        pass4.setNationality( "RUS" );
        pass4.setReservNum( "GGFGD1" );
        pass4.setDocType( "P" );
        pass4.setDocNumber( "000001" );
        pass4.setDocCountry( "RUS" );

        Paxlst::PassengerInfo pass5;
        pass5.setSurname( "PUTINA" );
        pass5.setFirstName( "LUDA" );
        pass5.setSex( "F" );
        BASIC::TDateTime bd5 = ASTRA::NoExists;
        BASIC::StrToDateTime( "10.05.55 00:00:00", bd5 );
        pass5.setBirthDate( bd5 );
        pass5.setDepPort( "VIE" );
        pass5.setArrPort( "BCN" );
        pass5.setNationality( "RUS" );
        pass5.setReservNum( "GGFGD2" );
        pass5.setDocType( "P" );
        pass5.setDocNumber( "000002" );
        pass5.setDocCountry( "RUS" );

        paxlstInfo.addPassenger( pass4 );
        paxlstInfo.addPassenger( pass5 );

        return paxlstInfo;
    }
}

///////////////////////////////////////////////////////////////////////////////

START_TEST( test1 )
{

    Paxlst::PaxlstInfo paxlstInfo( makePaxlst3() );

    std::string text = paxlstInfo.toEdiString();
    fail_if( text.empty() );

    // Ожидаемый текст
    TestStrings ts;
    ts <<
      "UNH+1+PAXLST:D:02B:UN:IATA+OK688/071008/1310+01:F'\n"
      "BGM+745'\n"
      "NAD+MS+++CDGKOAF'\n"
      "COM+0148642106:TE+0148643999:FX'\n"
      "TDT+20+OK688'\n"
      "LOC+125+PRG'\n"
      "DTM+189:0710081045:201'\n"
      "LOC+87+BCN'\n"
      "DTM+232:0710081310:201'\n"
      "NAD+FL+++STRANSKY:JAROSLAV VICTOROVICH'\n"
      "ATT+2++M'\n"
      "DTM+329:670610'\n"
      "LOC+178+ZDN'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z9WKH'\n"
      "DOC+I:110:111+102865098'\n"
      "NAD+FL+++KOVACS:PETR'\n"
      "ATT+2++M'\n"
      "DTM+329:691209'\n"
      "LOC+178+ZDN'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z9WJK'\n"
      "DOC+P:110:111+35485167'\n"
      "DTM+36:080911'\n"
      "NAD+FL+++LESKA:PAVEL'\n"
      "ATT+2++M'\n"
      "DTM+329:760502'\n"
      "LOC+178+VIE'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z57L3'\n"
      "DOC+P:110:111+34356146'\n"
      "LOC+91+RUS'\n"
      "CNT+42:3'\n"
      "UNT+37+1'\n"
      "UNE+1+1'\n";

    // Сгенерированный текст
    LogTrace(TRACE5) << "\nText:\n" << text;

    std::string chk( ts.show_mismatch( text ) );
    fail_unless( chk.empty(), "PAXLST mismatched %s", chk.c_str() );
}
END_TEST;

START_TEST( test2 )
{
    Paxlst::PaxlstInfo paxlstInfo( makePaxlst1() );

    std::string text = paxlstInfo.toEdiString();
    fail_if( text.empty() );

    // Ожидаемый текст
    TestStrings ts;
    ts <<
      "UNH+1+PAXLST:D:02B:UN:IATA++01:F'\n"
      "BGM+745'\n"
      "NAD+MS+++CDGKOAF'\n"
      "TDT+20'\n"
      "LOC+125'\n"
      "DTM+189::201'\n"
      "LOC+87'\n"
      "DTM+232::201'\n"
      "NAD+FL+++STRANSKY'\n"
      "ATT+2++M'\n"
      "DTM+329'\n"
      "CNT+42:1'\n"
      "UNT+13+1'\n"
      "UNE+1+1'\n";

    // Сгенерированный текст
    LogTrace(TRACE5) << "\nText:\n" << text;

    std::string chk( ts.check( text ) );
    fail_unless( chk.empty(), "PAXLST mismatched %s", chk.c_str() );
}
END_TEST;

START_TEST( test3 )
{
    Paxlst::PaxlstInfo paxlstInfo( makePaxlst3_long() );

    std::string text = paxlstInfo.toEdiString();
    fail_if( text.empty() );

    // Ожидаемый текст
    TestStrings ts;
    ts <<
      "UNH+1+PAXLST:D:02B:UN:IATA+OK688/071008/1310XXXXXXXXXXXXXXXXXX+01:F'\n"
      "BGM+745'\n"
      "NAD+MS+++CDGKOAFXXXXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "COM+0148642106XXXXXXXXXXXXXXX:TE+0148643999XXXXXXXXXXXXXXX:FX'\n"
      "TDT+20+OK688XXXXXXXXXXXX'\n"
      "LOC+125+PRGXXXXXXXXXXXXXXXXXXXXXX'\n"
      "DTM+189:0710081045:201'\n"
      "LOC+87+BCNXXXXXXXXXXXXXXXXXXXXXX'\n"
      "DTM+232:0710081310:201'\n"
      "NAD+FL+++STRANSKYXXXXXXXXXXXXXXXXXXXXXXXXXXX:JAROSLAV VICTOROVICHXXXXXXXXXXXXXXX'\n"
      "ATT+2++M'\n"
      "DTM+329:670610'\n"
      "LOC+178+ZDNXXXXXXXXXXXXXXXXXXXXXX'\n"
      "LOC+179+BCNXXXXXXXXXXXXXXXXXXXXXX'\n"
      "NAT+2+CZE'\n"
      "DOC+IXX:110:111+102865098XXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "NAD+FL+++KOVACSXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:PETRXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "ATT+2++MXXXXXXXXXXXXXXXX'\n"
      "DTM+329:691209'\n"
      "LOC+178+ZDNXXXXXXXXXXXXXXXXXXXXXX'\n"
      "LOC+179+BCNXXXXXXXXXXXXXXXXXXXXXX'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z9WJKXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "DOC+PXX:110:111+35485167XXXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "DTM+36:080911'\n"
      "NAD+FL+++LESKAXXXXXXXXXXXXXXXXXXXXXDXXXXXXXX:PAVELXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX+ARBATXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX+MOSCOWXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "ATT+2++MXXXXXXXXXXXXXXXX'\n"
      "DTM+329:760502'\n"
      "LOC+178+VIEXXXXXXXXXXXXXXXXXXXXXX'\n"
      "LOC+179+BCNXXXXXXXXXXXXXXXXXXXXXX'\n"
      "NAT+2+RUS'\n"
      "RFF+AVF:Z57L3XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "DOC+PXX:110:111+34356146XXXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "CNT+42:3'\n"
      "UNT+35+1'\n"
      "UNE+1+1'\n";

    // Сгенерированный текст
    LogTrace(TRACE5) << "\nText:\n" << text;
    
    std::string chk( ts.check( text ) );
    fail_unless( chk.empty(), "PAXLST mismatched %s", chk.c_str() );
}
END_TEST;

START_TEST( test4 )
{
    BASIC::TDateTime depDate = ASTRA::NoExists;
    BASIC::StrToDateTime( "2007.09.07", "yyyy.mm.dd", depDate );

    std::string fname = Paxlst::createEdiPaxlstFileName( "OK", 421, "", "CAI", "PRG", depDate, "TXT" );
    fail_if( fname != "OK0421CAIPRG20070907.TXT" );

    fname = Paxlst::createEdiPaxlstFileName( "OK", 421, "", "CAI", "PRG", depDate, "TXT", 2 );
    fail_if( fname != "OK0421CAIPRG20070907.TXT.PART02" );

    fname = Paxlst::createEdiPaxlstFileName( "OK", 421, "", "CAI", "PRG", depDate, "TXT", 22 );
    fail_if( fname != "OK0421CAIPRG20070907.TXT.PART22" );

    fname = Paxlst::createEdiPaxlstFileName( "OK", 421, "", "CAI", "PRG", depDate, "TXT", 222 );
    fail_if( fname != "OK0421CAIPRG20070907.TXT.PART222" );
}
END_TEST;

START_TEST( test5 )
{
    BASIC::TDateTime destDate = ASTRA::NoExists;
    BASIC::StrToDateTime( "2007.09.15 12:10", "yyyy.mm.dd hh:nn", destDate );

    std::string iataCode = Paxlst::createIataCode( "OK0012", destDate );
    fail_if( iataCode != "OK0012/070915/1210" );
}
END_TEST;

START_TEST( test6 )
{
    Paxlst::PaxlstInfo paxlstInfo = makePaxlst3();

    std::vector< std::string > tlgs = paxlstInfo.toEdiStrings( 3 );
    fail_unless( tlgs.size() == 1 );

    // Сгенерированный текст
    LogTrace(TRACE5) << "\nText:\n" << tlgs.front();

    // Ожидаемый текст
    TestStrings ts;
    ts <<
      "UNH+1+PAXLST:D:02B:UN:IATA+OK688/071008/1310+01:F'\n"
      "BGM+745'\n"
      "NAD+MS+++CDGKOAF'\n"
      "COM+0148642106:TE+0148643999:FX'\n"
      "TDT+20+OK688'\n"
      "LOC+125+PRG'\n"
      "DTM+189:0710081045:201'\n"
      "LOC+87+BCN'\n"
      "DTM+232:0710081310:201'\n"
      "NAD+FL+++STRANSKY:JAROSLAV VICTOROVICH'\n"
      "ATT+2++M'\n"
      "DTM+329:670610'\n"
      "LOC+178+ZDN'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z9WKH'\n"
      "DOC+I:110:111+102865098'\n"
      "NAD+FL+++KOVACS:PETR'\n"
      "ATT+2++M'\n"
      "DTM+329:691209'\n"
      "LOC+178+ZDN'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z9WJK'\n"
      "DOC+P:110:111+35485167'\n"
      "DTM+36:080911'\n"
      "NAD+FL+++LESKA:PAVEL'\n"
      "ATT+2++M'\n"
      "DTM+329:760502'\n"
      "LOC+178+VIE'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z57L3'\n"
      "DOC+P:110:111+34356146'\n"
      "LOC+91+RUS'\n"
      "CNT+42:3'\n"
      "UNT+37+1'\n"
      "UNE+1+1'\n";

    std::string chk( ts.check( tlgs.front() ) );
    fail_unless( chk.empty(), "PAXLST mismatched %s", chk.c_str() );
}
END_TEST;

START_TEST( test7 )
{
    Paxlst::PaxlstInfo paxlstInfo = makePaxlst5();

    std::vector< std::string > tlgs = paxlstInfo.toEdiStrings( 2 );
    fail_unless( tlgs.size() == 3 );

    {
        TestStrings ts;
        ts <<
          "UNH+1+PAXLST:D:02B:UN:IATA+OK688/071008/1310+01:C'\n";
          ts << "BGM+745'\n";
          ts << "NAD+MS+++CDGKOAF'\n";
          ts << "COM+0148642106:TE+0148643999:FX'\n";
          ts << "TDT+20+OK688'\n";
          ts << "LOC+125+PRG'\n";
          ts << "DTM+189:0710081045:201'\n";
          ts << "LOC+87+BCN'\n";
          ts << "DTM+232:0710081310:201'\n";
          ts << "NAD+FL+++STRANSKY:JAROSLAV VICTOROVICH'\n";
          ts << "ATT+2++M'\n";
          ts << "DTM+329:670610'\n";
          ts << "LOC+178+ZDN'\n";
          ts << "LOC+179+BCN'\n";
          ts << "NAT+2+CZE'\n";
          ts << "RFF+AVF:Z9WKH'\n";
          ts << "DOC+I:110:111+102865098'\n";
          ts << "NAD+FL+++KOVACS:PETR'\n";
          ts << "ATT+2++M'\n";
          ts << "DTM+329:691209'\n";
          ts << "LOC+178+ZDN'\n";
          ts << "LOC+179+BCN'\n";
          ts << "NAT+2+CZE'\n";
          ts << "RFF+AVF:Z9WJK'\n";
          ts << "DOC+P:110:111+35485167'\n";
          ts << "DTM+36:080911'\n";
          ts << "CNT+42:5'\n";
          ts << "UNT+28+1'\n";
          ts << "UNE+1+1'\n";;

        LogTrace(TRACE5) << "tlgs.part1:\n" << tlgs[ 0 ];
        std::string chk( ts.check( tlgs[ 0 ] ) );
        fail_unless( chk.empty(), "PAXLST part1 mismatched %s", chk.c_str() );
    }

    {
        TestStrings ts;
        ts <<
          "UNH+1+PAXLST:D:02B:UN:IATA+OK688/071008/1310+02'\n"
          "BGM+745'\n"
          "NAD+MS+++CDGKOAF'\n"
          "COM+0148642106:TE+0148643999:FX'\n"
          "TDT+20+OK688'\n"
          "LOC+125+PRG'\n"
          "DTM+189:0710081045:201'\n"
          "LOC+87+BCN'\n"
          "DTM+232:0710081310:201'\n"
          "NAD+FL+++LESKA:PAVEL'\n"
          "ATT+2++M'\n"
          "DTM+329:760502'\n"
          "LOC+178+VIE'\n"
          "LOC+179+BCN'\n"
          "NAT+2+CZE'\n"
          "RFF+AVF:Z57L3'\n"
          "DOC+P:110:111+34356146'\n"
          "LOC+91+RUS'\n"
          "NAD+FL+++PUTIN:VOVA'\n"
          "ATT+2++M'\n"
          "DTM+329:520502'\n"
          "LOC+178+VIE'\n"
          "LOC+179+BCN'\n"
          "NAT+2+RUS'\n"
          "RFF+AVF:GGFGD1'\n"
          "DOC+P:110:111+000001'\n"
          "LOC+91+RUS'\n"
          "CNT+42:5'\n"
          "UNT+29+1'\n"
          "UNE+1+1'\n";

        LogTrace(TRACE5) << "tlg.part2:\n" << tlgs[ 1 ];
        std::string chk( ts.check( tlgs[ 1 ] ) );
        fail_unless( chk.empty(), "PAXLST part2 mismatched %s", chk.c_str() );
    }

    {
        TestStrings ts;
        ts <<
          "UNH+1+PAXLST:D:02B:UN:IATA+OK688/071008/1310+03:F'\n"
          "BGM+745'\n"
          "NAD+MS+++CDGKOAF'\n"
          "COM+0148642106:TE+0148643999:FX'\n"
          "TDT+20+OK688'\n"
          "LOC+125+PRG'\n"
          "DTM+189:0710081045:201'\n"
          "LOC+87+BCN'\n"
          "DTM+232:0710081310:201'\n"
          "NAD+FL+++PUTINA:LUDA'\n"
          "ATT+2++F'\n"
          "DTM+329:550510'\n"
          "LOC+178+VIE'\n"
          "LOC+179+BCN'\n"
          "NAT+2+RUS'\n"
          "RFF+AVF:GGFGD2'\n"
          "DOC+P:110:111+000002'\n"
          "LOC+91+RUS'\n"
          "CNT+42:5'\n"
          "UNT+20+1'\n"
          "UNE+1+1'\n";

        LogTrace(TRACE5) << "tlg.part3:\n" << tlgs[ 2 ];
        std::string chk( ts.check( tlgs[ 2 ] ) );
        fail_unless( chk.empty(), "PAXLST part3 mismatched %s", chk.c_str() );
    }
}
END_TEST;

START_TEST( test8 )
{
    Paxlst::PaxlstInfo paxlstInfo = makePaxlst3();

    std::vector< std::string > tlgs = paxlstInfo.toEdiStrings( 50 );
    fail_unless( tlgs.size() == 1 );

    // Сгенерированный текст
    LogTrace(TRACE5) << "\nText:\n" << tlgs.front();

    // Ожидаемый текст
    TestStrings ts;
    ts <<
      "UNH+1+PAXLST:D:02B:UN:IATA+OK688/071008/1310+01:F'\n"
      "BGM+745'\n"
      "NAD+MS+++CDGKOAF'\n"
      "COM+0148642106:TE+0148643999:FX'\n"
      "TDT+20+OK688'\n"
      "LOC+125+PRG'\n"
      "DTM+189:0710081045:201'\n"
      "LOC+87+BCN'\n"
      "DTM+232:0710081310:201'\n"
      "NAD+FL+++STRANSKY:JAROSLAV VICTOROVICH'\n"
      "ATT+2++M'\n"
      "DTM+329:670610'\n"
      "LOC+178+ZDN'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z9WKH'\n"
      "DOC+I:110:111+102865098'\n"
      "NAD+FL+++KOVACS:PETR'\n"
      "ATT+2++M'\n"
      "DTM+329:691209'\n"
      "LOC+178+ZDN'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z9WJK'\n"
      "DOC+P:110:111+35485167'\n"
      "DTM+36:080911'\n"
      "NAD+FL+++LESKA:PAVEL'\n"
      "ATT+2++M'\n"
      "DTM+329:760502'\n"
      "LOC+178+VIE'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z57L3'\n"
      "DOC+P:110:111+34356146'\n"
      "LOC+91+RUS'\n"
      "CNT+42:3'\n"
      "UNT+37+1'\n"
      "UNE+1+1'\n";

    std::string chk( ts.check( tlgs.front() ) );
    fail_unless( chk.empty(), "PAXLST mismatched %s", chk.c_str() );
}
END_TEST;


#define SUITENAME "apis_file"
TCASEREGISTER( init, tear_down)
{
    ADD_TEST( test1 );
    ADD_TEST( test2 );
    ADD_TEST( test3 );
    ADD_TEST( test4 );
    ADD_TEST( test5 );
    ADD_TEST( test6 );
    ADD_TEST( test7 );
    ADD_TEST( test8 );
}
TCASEFINISH;

#endif /*XP_TESTING*/
