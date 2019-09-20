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
#include "tlg/read_edi_elements.h"
#include "config.h"
#include "tlg/tlg.h"
#include "astra_misc.h"
#include "apis_tools.h"
#include "misc.h"

#include <edilib/edi_func_cpp.h>
#include <edilib/edi_astra_msg_types.h>
#include <edilib/edi_sess.h>
#include <serverlib/str_utils.h>

#include <boost/lexical_cast.hpp>

#include <time.h>
#include <sstream>

#define NICKNAME "ANTON"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

using namespace BASIC::date_time;

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
                            const TDateTime& destDateTime,
                            const std::string& destDateTimeFmt )
{
    std::ostringstream iata;
    iata << flight;
    iata << DateTimeToStr( destDateTime, destDateTimeFmt );
    return iata.str();
}

std::string createEdiPaxlstFileName( const std::string& carrierCode,
                                     const int& flightNumber,
                                     const std::string& flightSuffix,
                                     const std::string& origin,
                                     const std::string& destination,
                                     const TDateTime& departureDate,
                                     const std::string& ext,
                                     unsigned partNum,
                                     const std::string& lst_type )
{
    ostringstream f;
    f << carrierCode << flightNumber << flightSuffix;

    std::ostringstream fname;
    fname << carrierCode
          << (f.str().size()<6?string(6-f.str().size(),'0'):"") << flightNumber
          << flightSuffix
          << origin << destination
          << DateTimeToStr( departureDate, "yyyymmdd" )
          << lst_type
          << "." << ext;
    if( partNum )
        fname << ".PART" << std::setfill('0') << std::setw(2) << partNum;
    return fname.str();
}

static std::string createEdiInterchangeReference()
{
#if APIS_TEST
    return "TEST_REF";
#endif
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
                                  const TDateTime& nowUtc,
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
    viewBgmElement( pMes, paxlst.getBgmElem() );

    // RFF
    if (paxlst.settings().view_RFF_TN())
    {
      std::string rff_tn =  (!paxlst.settings().RFF_TN().empty())?
                            paxlst.settings().RFF_TN():
                            createEdiInterchangeReference();
      viewRffElement( pMes, RffElem( "TN", rff_tn.substr(0,35) ), 0 );
    }

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

    int segmGroup2Num=0;
    boost::optional<FlightStop> priorFlightStop;
    for(int pass=0; pass<2; pass++)
    {
      const FlightStops& flightStops=(pass==0?paxlst.stopsBeforeBorder():paxlst.stopsAfterBorder());
      for(FlightStops::const_iterator i=flightStops.begin(); i!=flightStops.end(); ++i)
      {
        const FlightStop& currFlightStop=*i;
        if (priorFlightStop)
        {
          SetEdiSegGr( pMes, SegGrElement( 2, segmGroup2Num ) );
          PushEdiPointW( pMes );
          SetEdiPointToSegGrW( pMes, SegGrElement( 2, segmGroup2Num++ ) );

          // TDT
          viewTdtElement( pMes, TdtElem( "20", paxlst.flight(), paxlst.carrier() ) );


          int segmGroup3Num=0;
          SetEdiSegGr( pMes, SegGrElement( 3, segmGroup3Num ) );
          PushEdiPointW( pMes );
          SetEdiPointToSegGrW( pMes, SegGrElement( 3, segmGroup3Num++ ) );
          // LOC departure
          viewLocElement( pMes, LocElem( i==paxlst.stopsAfterBorder().begin()?LocElem::LastDepartureBeforeBorder:
                                                                              LocElem::OtherDeparturesAndArrivals,
                                         priorFlightStop.get().depPort() ) );
          // DTM departure
          if (priorFlightStop.get().depDateTime()!=ASTRA::NoExists)
            viewDtmElement( pMes, DtmElem( DtmElem::Departure, priorFlightStop.get().depDateTime(), "201" ) );
          PopEdiPointW( pMes );

          SetEdiSegGr( pMes, SegGrElement( 3, segmGroup3Num ) );
          PushEdiPointW( pMes );
          SetEdiPointToSegGrW( pMes, SegGrElement( 3, segmGroup3Num++ ) );
          // LOC arrival
          viewLocElement( pMes, LocElem( i==paxlst.stopsAfterBorder().begin()?LocElem::FirstArrivalAfterBorder:
                                                                              LocElem::OtherDeparturesAndArrivals,
                                         currFlightStop.arrPort() ) );
          // DTM arrival
          if (currFlightStop.arrDateTime()!=ASTRA::NoExists)
            viewDtmElement( pMes, DtmElem( DtmElem::Arrival, currFlightStop.arrDateTime(), "201" ) );
          PopEdiPointW( pMes );

          PopEdiPointW( pMes );
        }
        priorFlightStop=currFlightStop;
      }
    }

    const PassengersList_t& passList = paxlst.passengersList();
    int segmGroupNum = 0;
    for( std::list< Paxlst::PassengerInfo >::const_iterator it = passList.begin();
           it != passList.end(); ++it, segmGroupNum++ )
    {
        SetEdiSegGr( pMes, SegGrElement( 4, segmGroupNum ) );

        PushEdiPointW( pMes );
        SetEdiPointToSegGrW( pMes, SegGrElement( 4, segmGroupNum ) );

        // NAD
        viewNadElement( pMes, paxlst.getNadElem(*it) );

        if (paxlst.type()!=PaxlstInfo::IAPIFlightCloseOnBoard)
        {
           // ATT
           viewAttElement( pMes, AttElem( "2", it->sex() ) );
           // DTM
           viewDtmElement( pMes, DtmElem( DtmElem::DateOfBirth, it->birthDate() ) );

          // GEI
          if (!it->procInfo().empty())
            viewGeiElement( pMes, GeiElem("4", it->procInfo()) );

          int meaNum = 0;
          if( it->bagCount() != ASTRA::NoExists )
          {
            // MEA
            viewMeaElement( pMes, MeaElem( MeaElem::BagCount, it->bagCount() ), meaNum++ );
          }
          if( it->bagWeight() != ASTRA::NoExists )
          {
            // MEA
            viewMeaElement( pMes, MeaElem( MeaElem::BagWeight, it->bagWeight() ), meaNum++ );
          }

          int ftxNum = 0;
          for (auto tag = it->bagTags().begin(); tag != it->bagTags().end() && ftxNum < 99; ++tag, ++ftxNum)
          {
            // FTX
            viewFtx2Element( pMes, Ftx2Elem("BAG", *tag, "1"), ftxNum);
          }

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
        }

        int rffNum = 0;
        if( paxlst.type()==PaxlstInfo::IAPIClearPassengerRequest ||  //для IAPI выводим пустой элемент AVF
            paxlst.type()==PaxlstInfo::IAPIChangePassengerData ||
            !it->reservNum().empty() )
        {
            // RFF
            viewRffElement( pMes, RffElem( "AVF", it->reservNum() ), rffNum++ );
        }
        if( !it->paxRef().empty() )
        {
            // RFF
            viewRffElement( pMes, RffElem( "ABO", it->paxRef() ), rffNum++ );
        }

        if (paxlst.type()!=PaxlstInfo::IAPIFlightCloseOnBoard)
        {
          if( !it->ticketNumber().empty() )
          {
            // RFF
            viewRffElement( pMes, RffElem( "YZY", it->ticketNumber() ), rffNum++ );
          }

          for( vector<pair<int,string>>::const_iterator i = it->seats().begin(); i != it->seats().end(); i++ )
          {
            // RFF
            viewRffElement( pMes, RffElem( "SEA", IntToString( i->first ) + i->second ), rffNum++ );
          }

          int seg5iter = 0;
          if( !it->docType().empty() || !it->docNumber().empty() )
          {
            SetEdiSegGr( pMes, SegGrElement( 5, seg5iter ) );
            PushEdiPointW( pMes );
            SetEdiPointToSegGrW( pMes, SegGrElement( 5, seg5iter ) );

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
            ++seg5iter;
          }

          // ВИЗА
          if( !it->docoType().empty() || !it->docoNumber().empty() )
          {
            SetEdiSegGr( pMes, SegGrElement( 5, seg5iter ) );
            PushEdiPointW( pMes );
            SetEdiPointToSegGrW( pMes, SegGrElement( 5, seg5iter ) );

            // DOC
            viewDocElement( pMes, DocElem( it->docoType(), it->docoNumber(), paxlst.settings().respAgnCode() ) );

            if( it->docoExpirateDate() != ASTRA::NoExists )
            {
              // DTM
              viewDtmElement( pMes, DtmElem( DtmElem::DocExpireDate, it->docoExpirateDate() ) );
            }

            if( !it->docoCountry().empty() )
            {
              // LOC
              viewLocElement( pMes, LocElem( LocElem::DocCountry, it->docoCountry() ) );
            }

            PopEdiPointW( pMes );
            ++seg5iter;
          }
        }

        PopEdiPointW( pMes );
    }

    // CNT
    viewCntElement( pMes, paxlst.getCntElem(totalCnt) );

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
    TDateTime nowUtc = NowUTC();

    edi_mes_head edih;
    memset( &edih, 0, sizeof(edih) );
    edih.syntax_ver = SyntaxVer;
    edih.mes_num = 1;
    strcpy( edih.chset, Chset );
    strcpy( edih.to, paxlst.recipientName().c_str() );
#if APIS_TEST
    // перезаписывается в функции CreateMesByHead
    strcpy( edih.date, "000000" );
    strcpy( edih.time, "0000" );
#else
    strcpy( edih.date, DateTimeToStr( nowUtc, "yymmdd" ).c_str() );
    strcpy( edih.time, DateTimeToStr( nowUtc, "hhnn" ).c_str() );
#endif
    strcpy( edih.from, paxlst.senderName().c_str() );
    strcpy( edih.acc_ref, paxlst.iataCode().c_str() );
    strcpy( edih.other_ref, "" );
    strcpy( edih.assoc_code, "" );
    strcpy( edih.our_ref, ediRef.c_str() );
    strcpy( edih.FseId, paxlst.settings().appRef().c_str() );
    strcpy( edih.unh_number, UnhNumber );
//    strcpy( edih.unh_number,
//            !paxlst.settings().unh_number().empty()?
//             paxlst.settings().unh_number().c_str():
//             UnhNumber );
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

    return "UNA:+.? '\n" + StrUtils::replaceSubstrCopy(ediMessageToStr(pMes), "'", "'\n");
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

//---------------------------------------------------------------------------------------

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
    for( const PaxlstInfo& paxlst: splitted )
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
  if ( ( m_type == FlightPassengerManifest && pax_num == 0 )  ||
       ( m_type == FlightCrewManifest && crew_num == 0 ) ||
       ( m_type == IAPIClearPassengerRequest ) ||
       ( m_type == IAPIChangePassengerData ) ||
       ( m_type == IAPIFlightCloseOnBoard ) ||
       ( m_type == IAPICancelFlight ) )
      return;

  // Make segment "Message"
  if(GetNode("Message", emulApisNode) == NULL) {
    TDateTime nowUtc = NowUTC();
    xmlNodePtr messageNode = NewTextChild(emulApisNode, "Message");
    NewTextChild(messageNode, "Destination", "GTB");
    xmlNodePtr systemNode = NewTextChild(messageNode, "System");
    SetProp(systemNode, "Application", "DCS ASTRA");
    SetProp(systemNode, "Organization", "SIRENA-TRAVEL");
    SetProp(systemNode, "ApplicationVersion", 1);
    xmlNodePtr contactNode = NewTextChild(systemNode, "Contact");
    NewTextChild(contactNode, "Name", "SIRENA-TRAVEL");
#if APIS_TEST
    NewTextChild(messageNode, "CreateDateTime",
                 DateTimeToStr(nowUtc, "0000-00-00'T'00:00:00"));
#else
    NewTextChild(messageNode, "CreateDateTime",
                 DateTimeToStr(nowUtc, "yyyy-mm-dd'T'hh:nn:00"));
#endif
    NewTextChild(messageNode, "SentDateTime",
                 DateTimeToStr(nowUtc, "yyyy-mm-dd"));
    if (senderCarrierCode().empty())
      throw Exception("senderCarrierCode is empty");
    std::string envelope_id = generate_envelope_id(senderCarrierCode());
    if (envelope_id.empty())
      throw Exception("EnvelopeID is empty");
    NewTextChild(messageNode, "EnvelopeID", envelope_id);
    NewTextChild(messageNode, "Owner", "DCS ASTRA");
    std::string msg_identifier = get_msg_identifier();
    if (!msg_identifier.empty()) NewTextChild(messageNode, "Identifier", msg_identifier);
    NewTextChild(messageNode, "Version", version);
    NewTextChild(messageNode, "Context", version?"UPDATE":"ORIGINAL");
  }
  // Make segment "Flight"
  if(GetNode("Flight", emulApisNode) == NULL) {
    xmlNodePtr flightNode = NewTextChild(emulApisNode, "Flight");
    SetProp(flightNode, "AllCrewFlag", pax_num?"false":"true");
    if (!iataCode().empty()) SetProp(flightNode, "CAR", iataCode());
    SetProp(flightNode, "PassengerCount", pax_num);
    SetProp(flightNode, "CrewCount", crew_num);
    SetProp(flightNode, "TotalCount", pax_num + crew_num);
    xmlNodePtr opfltidNode = NewTextChild(flightNode, "OperatingFlightId");
    xmlNodePtr carrierNode = NewTextChild(opfltidNode, "Carrier");
    if (settings().mesAssCode().empty())
      throw Exception("CodeType is empty");
    SetProp(carrierNode, "CodeType", settings().mesAssCode());
    if (settings().mesAssCode().empty())
      throw Exception("CarrierCode is empty");
    NewTextChild(carrierNode, "CarrierCode", carrier());
    if (flight().empty())
      throw Exception("FlightNumber is empty");
    NewTextChild(opfltidNode, "FlightNumber", flight());
    if(!markFlts().empty()) {
      for(std::map<std::string, std::string>::const_iterator i=markFlts().begin();i!=markFlts().end();i++)
      {
        xmlNodePtr codeshareNode = NewTextChild(flightNode, "CodeShareFlightId");
        xmlNodePtr carrierNode = NewTextChild(codeshareNode, "Carrier");
        SetProp(carrierNode, "CodeType", settings().mesAssCode());
        NewTextChild(carrierNode, "CarrierCode", i->first);
        NewTextChild(codeshareNode, "FlightNumber", i->second);
      }
    }
//    if (depDateTime() == ASTRA::NoExists)  //!!!vlad
//      throw Exception("ScheduledDepartureDateTime is empty");
//    NewTextChild(flightNode, "ScheduledDepartureDateTime",
//                 DateTimeToStr(depDateTime(), "yyyy-mm-dd'T'hh:nn:00"));
//    if (depPort().empty())
//      throw Exception("DepartureAirport is empty");
//    NewTextChild(flightNode, "DepartureAirport", depPort());
//    if (arrDateTime() == ASTRA::NoExists)
//      throw Exception("EstimatedArrivalDateTime is empty");
//    NewTextChild(flightNode, "EstimatedArrivalDateTime",
//                 DateTimeToStr(arrDateTime(), "yyyy-mm-dd'T'hh:nn:00"));
//    if (arrPort().empty())
//      throw Exception("ArrivalAirport is empty");
//    NewTextChild(flightNode, "ArrivalAirport", arrPort());
    xmlNodePtr FlightLegsNode = NewTextChild(flightNode, "FlightLegs");
    fltLegs().FlightLegstoXML(FlightLegsNode);
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
    if (!version) {
      checkInNode = NewTextChild(travellerNode, "CheckIn");
      SetProp(checkInNode, "CheckInStatus", it->prBrd()?"B":"C");
      flyerNode = NewTextChild(checkInNode, "DCS_Traveller");
      if (!it->persType().empty()) SetProp(flyerNode, "Type", it->persType());
      for(vector< pair<int, string> >::const_iterator i=it->seats().begin();i!=it->seats().end();i++)
      {
        xmlNodePtr seatNode = NewTextChild(checkInNode, "CheckInSeat");
        SetProp(seatNode, "Number", IntToString(i->first) + i->second);
        SetProp(seatNode, "Row", i->first);
        SetProp(seatNode, "Column", i->second);
      }
      if (!it->fqts().empty()) {
        xmlNodePtr fqtsNode = NewTextChild(checkInNode, "FrequentFlyer");
        for(std::set<CheckIn::TPaxFQTItem>::const_iterator i=it->fqts().begin();i!=it->fqts().end();i++)
        {
          xmlNodePtr memberNode = NewTextChild(fqtsNode, "ProgramMember");
          NewTextChild(memberNode, "Number", i->no);
          const TAirlinesRow &airline = (const TAirlinesRow&)base_tables.get("airlines").get_row("code",i->airline);
          if (airline.code_lat.empty()) throw Exception("airline.code_lat empty (code=%s)",airline.code.c_str());
          NewTextChild(memberNode, "Sponsor", airline.code_lat);
        }
      }
      if(!it->ticketNumber().empty()) {
        xmlNodePtr TicketsNode = NewTextChild(checkInNode, "Tickets");
        xmlNodePtr TicketNode = NewTextChild(TicketsNode, "Ticket");
        NewTextChild(TicketNode, "TicketNumber", it->ticketNumber());
      }
    }
    else
    {
      boardingNode = NewTextChild(travellerNode, "Boarding");
      flyerNode = NewTextChild(boardingNode, "Flyer");
      SetProp(flyerNode, "Type", type()?"FM":"FL");
      xmlNodePtr itineraryNode = NewTextChild(boardingNode, "FlyerItinerary");
      if (!it->depPort().empty()) SetProp(itineraryNode, "JourneyCommence", it->depPort());
      if (!it->arrPort().empty()) SetProp(itineraryNode, "JourneyConclude", it->arrPort());
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
    if (!it->persType().empty())
      SetProp(flyerNode, "InfantIndicator", (it->persType()=="Infant")?"true":"false");
    xmlNodePtr nameNode = NewTextChild(flyerNode, "Name");
    if (it->surname().empty())
      throw Exception("Surname is empty");
    NewTextChild(nameNode, "Surname", it->surname());
    if (it->first_name().empty())
      throw Exception("FirstName is empty");
    NewTextChild(nameNode, "FirstName", it->first_name());
    if (!it->second_name().empty())
      NewTextChild(nameNode, "MiddleName", it->second_name());
    if (it->birthDate() == ASTRA::NoExists)
      throw Exception("DateOfBirth is empty");
    NewTextChild(flyerNode, "DateOfBirth", DateTimeToStr(it->birthDate(), "yyyy-mm-dd"));
    if (it->sex().empty())
      throw Exception("Gender is empty");
    NewTextChild(flyerNode, "Gender", it->sex());
    if (it->nationality().empty())
      throw Exception("Nationality is empty");
    NewTextChild(flyerNode, "Nationality", it->nationality());
    if (!it->residCountry().empty())
      NewTextChild(flyerNode, "CountryOfResidence", it->residCountry());
    if (!it->birthCountry().empty())
      NewTextChild(flyerNode, "CountryOfBirth", it->birthCountry());

    xmlNodePtr docNode = NewTextChild(flyerNode, "TravelDocument");
    if (it->docType().empty())
      throw Exception("TypeCode is empty");
    SetProp(docNode, "TypeCode", it->docType());
    if (it->docNumber().empty())
      throw Exception("Number of document is empty");
    NewTextChild(docNode, "Number", it->docNumber());
    if (it->docCountry().empty())
      throw Exception("IssueCountry is empty");
    NewTextChild(docNode, "IssueCountry", it->docCountry());
    if (it->docExpirateDate()!=ASTRA::NoExists) NewTextChild(docNode, "ExpiryDate", DateTimeToStr(it->docExpirateDate(), "yyyy-mm-dd"));
  }
}

void PaxlstInfo::checkInvariant() const
{
    if( passengersList().size() > 99999 )
        throw EXCEPTIONS::Exception( "Bad paxlst size!" );

    if( senderName().empty() )
        throw EXCEPTIONS::Exception( "Empty sender name!" );

    if( stopsBeforeBorder().empty() ||
        stopsAfterBorder().empty())
        throw EXCEPTIONS::Exception( "Empty stopsBeforeBorder or stopsAfterBorder!" );
}

BgmElem PaxlstInfo::getBgmElem() const
{
  switch(m_type)
  {
    case FlightPassengerManifest:
      return BgmElem("745", m_docId);
    case FlightCrewManifest:
      return BgmElem("250", m_docId);
    case IAPIClearPassengerRequest:
      return BgmElem("745","");
    case IAPIChangePassengerData:
      return BgmElem("745","CP");
    case IAPIFlightCloseOnBoard:
      return BgmElem("266","CLOB");
    case IAPICancelFlight:
      return BgmElem("266","XF");
  }
  throw EXCEPTIONS::Exception( "%s: Unsupported message type", __func__ );
}

CntElem PaxlstInfo::getCntElem(const int totalCnt) const
{
  switch(m_type)
  {
    case FlightPassengerManifest:
    case IAPIClearPassengerRequest:
    case IAPIChangePassengerData:
    case IAPIFlightCloseOnBoard:
    case IAPICancelFlight:
      return CntElem(CntElem::PassengersTotal, totalCnt);
    case FlightCrewManifest:
      return CntElem(CntElem::CrewTotal, totalCnt);
  }
  throw EXCEPTIONS::Exception( "%s: Unsupported message type", __func__ );
}

NadElem PaxlstInfo::getNadElem(const Paxlst::PassengerInfo& pax) const
{
  switch(m_type)
  {
    case FlightPassengerManifest:
    case FlightCrewManifest:
    case IAPIClearPassengerRequest:
    case IAPIChangePassengerData:
      return NadElem(m_type==PaxlstInfo::FlightCrewManifest?"FM":"FL",
                     pax.surname(),
                     pax.first_name(),
                     pax.second_name(),
                     pax.street(),
                     pax.city(),
                     pax.countrySubEntityCode(),
                     pax.postalCode(),
                     pax.destCountry());
    case IAPIFlightCloseOnBoard:
      return NadElem("ZZZ", "");
    case IAPICancelFlight: ; //там вообще нет элементов NAD для пассажира
  }
  throw EXCEPTIONS::Exception( "%s: Unsupported message type", __func__ );
}

}//namespace Paxlst

/////////////////////////////////////////////////////////////////////////////////////////

namespace edifact {

using namespace Ticketing::TickReader;
using namespace edilib;

void collectPAXLST(_EDI_REAL_MES_STRUCT_ *pMes, const Paxlst::PaxlstInfo& paxlst)
{
    Paxlst::collectPaxlstMessage(pMes, paxlst, NowUTC(), 1, 1, paxlst.passengersList().size());
}

//

Cusres::Cusres(const BgmElem& bgm)
    : m_bgm(bgm)
{}


Cusres::SegGr3::SegGr3(const RffElem& rff,
                       const DtmElem& dtm1,
                       const DtmElem& dtm2,
                       const LocElem& loc1,
                       const LocElem& loc2)
    : m_rff(rff),
      m_dtm1(dtm1),
      m_dtm2(dtm2),
      m_loc1(loc1),
      m_loc2(loc2)
{}

Cusres::SegGr4::SegGr4(const ErpElem& erp,
                       const ErcElem& erc)
    : m_erp(erp),
      m_erc(erc)
{}

Cusres readCUSRES(_EDI_REAL_MES_STRUCT_ *pMes)
{
    auto bgm = readEdiBgm(pMes);
    ASSERT(bgm);

    Cusres cusres(*bgm);

    // UNG
    cusres.m_ung = readEdiUng(pMes);
    // RFF
    cusres.m_rff = readEdiRff(pMes);

    int numSg3 = GetNumSegGr(pMes, 3);
    for(int currSg3 = 0; currSg3 < numSg3; ++currSg3)
    {
        EdiPointHolder sg3_holder(pMes);
        SetEdiPointToSegGrG(pMes, SegGrElement(3, currSg3), "PROG_ERR");

        // RFF
        auto rff = readEdiRff(pMes);
        ASSERT(rff);
        // DTM
        auto dtm1 = readEdiDtm(pMes, 0);
        ASSERT(dtm1);
        // DTM
        auto dtm2 = readEdiDtm(pMes, 1);
        ASSERT(dtm2);
        // LOC
        auto loc1 = readEdiLoc(pMes, 0);
        ASSERT(loc1);
        // LOC
        auto loc2 = readEdiLoc(pMes, 1);
        ASSERT(loc2);

        cusres.m_vSegGr3.push_back(Cusres::SegGr3(*rff,
                                                  *dtm1,
                                                  *dtm2,
                                                  *loc1,
                                                  *loc2));
    }

    int numSg4 = GetNumSegGr(pMes, 4);
    for(int currSg4 = 0; currSg4 < numSg4; ++currSg4)
    {
        EdiPointHolder sg4_holder(pMes);
        SetEdiPointToSegGrG(pMes, SegGrElement(4, currSg4), "PROG_ERR");

        // ERP
        auto erp = readEdiErp(pMes);
        ASSERT(erp);
        // ERC
        auto erc = readEdiErc(pMes);
        ASSERT(erc);

        Cusres::SegGr4 segGr4(*erp, *erc);
        // RFF
        if(auto rff0 = readEdiRff(pMes, 0)) {
            segGr4.m_vRff.push_back(*rff0);
        }

        if(auto rff1 = readEdiRff(pMes, 1)) {
            segGr4.m_vRff.push_back(*rff1);
        }

        if(auto rff2 = readEdiRff(pMes, 2)) {
            segGr4.m_vRff.push_back(*rff2);
        }

        // FTX
        segGr4.m_ftx = readEdiFtx(pMes);

        cusres.m_vSegGr4.push_back(segGr4);
    }

    // UNE
    cusres.m_une = readEdiUne(pMes);

    return cusres;
}

Cusres readCUSRES(const std::string& ediText)
{
    int ret = ReadEdiMessage(ediText.c_str());
    if(ret == EDI_MES_STRUCT_ERR){
        throw EXCEPTIONS::Exception("Error in message structure: %s", EdiErrGetString());
    } else if( ret == EDI_MES_NOT_FND){
        throw EXCEPTIONS::Exception("No message found in template: %s", EdiErrGetString());
    } else if( ret == EDI_MES_ERR) {
        throw EXCEPTIONS::Exception("Edifact error ");
    }

    return readCUSRES(GetEdiMesStruct());
}

//---------------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, const Cusres& cusres)
{
    os << "CUSRES:" << "\n";
    os << cusres.m_bgm << "\n";
    if(cusres.m_rff) {
        os << *cusres.m_rff << "\n";
    }
    os << "Sg3:\n";
    for(const Cusres::SegGr3& sg3: cusres.m_vSegGr3) {
        os << sg3.m_rff << "\n"
           << sg3.m_dtm1 << "\n"
           << sg3.m_dtm2 << "\n"
           << sg3.m_loc1 << "\n"
           << sg3.m_loc2 << "\n";
        os << "\n";
    }
    os << "\nSg4:\n";
    for(const Cusres::SegGr4& sg4: cusres.m_vSegGr4) {
        os << sg4.m_erp << "\n"
           << sg4.m_erc << "\n";
        for(const RffElem& rff: sg4.m_vRff) {
            os << rff << "\n";
        }

        if(sg4.m_ftx) {
            os << *sg4.m_ftx << "\n";
        }
        os << "\n";
    }
    return os;
}

}//namespace edifact

/////////////////////////////////////////////////////////////////////////////////////////

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

        TDateTime depDate = ASTRA::NoExists;
        StrToDateTime( "08.10.07 10:45:00", depDate ); //"0710081045"
        TDateTime arrDate = ASTRA::NoExists;
        StrToDateTime( "08.10.07 13:10:00", arrDate ); //"0710081310"

        paxlstInfo.setCrossBorderFlightStops( "PRG", depDate,
                                              "BCN", arrDate );

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
        TDateTime depDate = ASTRA::NoExists, arrDate = ASTRA::NoExists;
        StrToDateTime( "08.10.07 10:45:00", depDate ); //"0710081045"
        StrToDateTime( "08.10.07 13:10:00", arrDate ); //"0710081310"        
        paxlstInfo.setCrossBorderFlightStops( "PrG", depDate, "BCN", arrDate);

        Paxlst::PassengerInfo pass1;
        pass1.setSurname( "STRANSKY" );
        pass1.setFirstName( "JAROSLAV VICtOROVICH" );
        pass1.setSex( "M" );
        TDateTime bd1 = ASTRA::NoExists;
        StrToDateTime( "10.06.67 00:00:00", bd1 ); //"670610"
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
        TDateTime bd2 = ASTRA::NoExists;
        StrToDateTime( "09.12.69 00:00:00", bd2 ); //"691209"
        pass2.setBirthDate( bd2 );
        pass2.setDepPort( "ZDN" );
        pass2.setArrPort( "BCN" );
        pass2.setNationality( "CZE" );
        pass2.setReservNum( "Z9WJK" );
        pass2.setDocType( "p" );
        pass2.setDocNumber( "35485167" );
        TDateTime expd1 = ASTRA::NoExists;
        StrToDateTime( "11.09.08 00:00:00", expd1 );
        pass2.setDocExpirateDate( expd1 );

        Paxlst::PassengerInfo pass3;
        pass3.setSurname( "LESKA" );
        pass3.setFirstName( "PAVEL" );
        pass3.setSex( "M" );
        TDateTime bd3 = ASTRA::NoExists;
        StrToDateTime( "02.05.76 00:00:00", bd3 ); //"760502"
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

        TDateTime depDate = ASTRA::NoExists;
        StrToDateTime( "08.10.07 10:45:00", depDate ); //"0710081045"
        TDateTime arrDate = ASTRA::NoExists;
        StrToDateTime( "08.10.07 13:10:00", arrDate ); //"0710081310"

        paxlstInfo.setCrossBorderFlightStops( "PRGXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
                                              depDate,
                                              "BCNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
                                              arrDate );


        Paxlst::PassengerInfo pass1;
        pass1.setSurname( "STRANSKYXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setFirstName( "JAROSLAV VICTOROVICHXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setSex( "M" );
        TDateTime bd1 = ASTRA::NoExists;
        StrToDateTime( "10.06.67 00:00:00", bd1 ); //"670610"
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
        TDateTime bd2 = ASTRA::NoExists;
        StrToDateTime( "09.12.69 00:00:00", bd2 ); //"691209"
        pass2.setBirthDate( bd2 );
        pass2.setDepPort( "ZDNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setArrPort( "BCNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setNationality( "CZEXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setReservNum( "Z9WJKXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setDocType( "PXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setDocNumber( "35485167XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        TDateTime expd2 = ASTRA::NoExists;
        StrToDateTime( "11.09.08 00:00:00", expd2 );
        pass2.setDocExpirateDate( expd2 );


        Paxlst::PassengerInfo pass3;
        pass3.setSurname( "LESKAXXXXXXXXXXXXXXXXXXXXXdXXXXXXXXXXXXXXXXX" );
        pass3.setFirstName( "PAVELXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setSex( "MXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        TDateTime bd3 = ASTRA::NoExists;
        StrToDateTime( "02.05.76 00:00:00", bd3 ); //"760502"
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
        TDateTime bd4 = ASTRA::NoExists;
        StrToDateTime( "02.05.52 00:00:00", bd4 );
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
        TDateTime bd5 = ASTRA::NoExists;
        StrToDateTime( "10.05.55 00:00:00", bd5 );
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
      "LOC+125+PRG'\n"
      "DTM+189:0710081045:201'\n"
      "LOC+87+BCN'\n"
      "DTM+232:0710081310:201'\n"
      "NAD+FL+++STRANSKY'\n"
      "ATT+2++M'\n"
      "DTM+329'\n"
      "CNT+42:1'\n"
      "UNT+13+1'\n"
      "UNE+1+1'\n";

    // Сгенерированный текст
    LogTrace(TRACE5) << "\nText:\n" << text;

    std::string chk( ts.show_mismatch( text ) );
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

    std::string chk( ts.show_mismatch( text ) );
    fail_unless( chk.empty(), "PAXLST mismatched %s", chk.c_str() );
}
END_TEST;

START_TEST( test4 )
{
    TDateTime depDate = ASTRA::NoExists;
    StrToDateTime( "2007.09.07", "yyyy.mm.dd", depDate );

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
    TDateTime destDate = ASTRA::NoExists;
    StrToDateTime( "2007.09.15 12:10", "yyyy.mm.dd hh:nn", destDate );

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

    std::string chk( ts.show_mismatch( tlgs.front() ) );
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
        std::string chk( ts.show_mismatch( tlgs[ 0 ] ) );
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
        std::string chk( ts.show_mismatch( tlgs[ 1 ] ) );
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
        std::string chk( ts.show_mismatch( tlgs[ 2 ] ) );
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

    std::string chk( ts.show_mismatch( tlgs.front() ) );
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
#undef SUITENAME

#endif /*XP_TESTING*/
