#include <stdlib.h>
#include <boost/crc.hpp>
#include "seat_descript.h"
#include "date_time.h"
#include "misc.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "astra_date_time.h"
#include "oralib.h"
#include "astra_locale.h"
#include "term_version.h"

#define NICKNAME "DJEK"
#include "serverlib/slogger.h"

using namespace std;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace ASTRA::date_time;

namespace SEAT_DESCR
{

const std::string descrSeatTypeContainer::SELECT_SQL =
      "SELECT seatkey, name FROM trip_comp_descr "
      " WHERE point_id=:point_id "
      " ORDER BY seatkey";

const std::string descrSeatTypeContainer::CLEAR_SQL =
      "DELETE trip_comp_descr "
      " WHERE point_id=:point_id";
const std::string descrSeatTypeContainer::INSERT_SQL =
      "INSERT INTO trip_comp_descr(point_id, seatkey, code) "
      " VALUES(:point_id, :seatkey, :code)";

const std::string descrSeatTypePaxContainer::SELECT_SQL =
      "SELECT seatkey, name, pax_id FROM pax_comp_descr "
      " WHERE point_id=:point_id "
      " ORDER BY seatkey";

const std::string descrSeatTypePaxContainer::CLEAR_SQL =
      "DELETE trip_comp_descr "
      " WHERE point_id=:point_id";
const std::string descrSeatTypePaxContainer::INSERT_SQL =
      "INSERT INTO trip_comp_descr(point_id, seatkey, code, pax_id) "
      " VALUES(:point_id, :seatkey, :code,:pax_id)";


const SeatDescrServices& seatDescrServices() { return ASTRA::singletone<SeatDescrServices>(); }

bool descriptSeatCreator::existsRemark( const SeatCoord &seatCoord, const std::string &remark, bool pr_denial ) {
  SALONS2::TPlace seat;
  if ( seatCoord.isValidSeat( seat ) ) {
      vecstr remarks;
      seat.GetRemarks( seatCoord.point_dep,  remarks, pr_denial );
      return ( std::find( remarks.begin(), remarks.end(), remark ) != remarks.end() );
  }
  return false;
}

bool descriptSeatCreator::existsLayer( const SeatCoord &seatCoord, ASTRA::TCompLayerType layer_type ) {
  SALONS2::TPlace seat;
  if ( seatCoord.isValidSeat( seat ) ) {
    std::set<SALONS2::TSeatLayer,SALONS2::SeatLayerCompare> vlayers;
    seat.GetLayers( seatCoord.point_dep,  vlayers, SALONS2::TGetLayersMode::glBase );
    for ( auto ilayer : vlayers ) {
      if ( ilayer.layer_type == layer_type ) {
        return true;
      }
    }
  }
  return false;
}
//=====================================================================

// Левая сторона воздушного судна
bool descriptSeatCreator::leftAirCraftSide( const SeatCoord &seatCoord ) {
  if ( seatCoord.isValidSeat( ) ) {
    for ( int x=seatCoord.line; x>=0; --x ) {
      if ( seatCoord.placeList->GetXsName( x ).empty() ) {
         return false;
      }
    }
    return true;
  }
  return false;
}

// Правая сторона воздушного судна
bool descriptSeatCreator::rightAirCraftSide( const SeatCoord &seatCoord ) {
  if ( seatCoord.isValidSeat( ) ) {
    for ( int x=seatCoord.line; x<seatCoord.placeList->GetXsCount(); ++x ) {
      if ( seatCoord.placeList->GetXsName( x ).empty() ) {
         return false;
      }
    }
    return true;
  }
  return false;
}
 // Места у окна
bool descriptSeatCreator::windowSeat( const SeatCoord &seatCoord ) {
  return ( (seatCoord.line == 0 || seatCoord.line == seatCoord.placeList->GetXsCount() - 1 ) && seatCoord.isValidSeat( ) );
}

// Место у прохода
bool descriptSeatCreator::aisleSeat( const SeatCoord &seatCoord ) {
  if ( seatCoord.isValidSeat( ) ) {
    int aisleLine = seatCoord.line + 1;
    if ( aisleLine < seatCoord.placeList->GetXsCount() &&
         seatCoord.placeList->GetXsName( aisleLine ).empty() &&
         ++aisleLine < seatCoord.placeList->GetXsCount() &&
         !seatCoord.placeList->GetXsName( aisleLine ).empty() ) {
      return true;
    }
    aisleLine = seatCoord.line - 1;
    if ( aisleLine > 0 &&
         seatCoord.placeList->GetXsName( aisleLine ).empty() &&
         --aisleLine >= 0 &&
         !seatCoord.placeList->GetXsName( aisleLine ).empty() ) {
      return true;
    }
  }
  return false;
}

 // Место для курящих
bool descriptSeatCreator::smokingSeat( const SeatCoord &seatCoord ) {
  return existsLayer( seatCoord, ASTRA::cltSmoke  );
}

// Место у перегородки
bool descriptSeatCreator::partitionSeat( const SeatCoord &seatCoord ) {
  SALONS2::TPlace seat;
  if ( seatCoord.isValidSeat( seat ) ) {
     return ( seatCoord.isValidElemType( seatCoord.line, seatCoord.row - 1,  seat, SALONS2::PARTITION_ELEM_TYPE ) );
  }
  return false;
}

// Место лицом к хвосту
bool descriptSeatCreator::facingTailSeat( const SeatCoord &seatCoord ) {
  SALONS2::TPlace seat;
  if ( seatCoord.isValidSeat( seat ) ) {
     return ( seat.agle == 180 );
  }
  return false;
}

// Комфортное место
bool descriptSeatCreator::comforttableSeat( const SeatCoord &seatCoord ) {
  SALONS2::TPlace seat;
  if ( seatCoord.isValidSeat( seat ) ) {
      vecstr remarks;
      seat.GetRemarks( seatCoord.point_dep,  remarks, false );
      return comforttableSeatDefines.isComfort( seatCoord.airline, remarks );
  }
  return false;
}

// Место не для детей
bool descriptSeatCreator::notChildrenSeat( const SeatCoord &seatCoord ) {
  //CHIN Место для детей и больных ЗАПРЕТ
  return existsRemark( seatCoord, REMARK_CHILDREN_MEDA, true ); //pr_denial=true
}

// Не для пассажирами с младенцами
bool descriptSeatCreator::notBabySeat( const SeatCoord &seatCoord ) {
  return existsRemark( seatCoord, REMARK_INFANT, true ); //pr_denial=true
}

// Не для сопровождающих детей
bool descriptSeatCreator::notEscortChildrenSeat( const SeatCoord &seatCoord ) {
  return existsRemark( seatCoord, REMARK_UNACCOMPANIED_CHILDREN, true ); //pr_denial=true
}

// Не для больных
bool descriptSeatCreator::notSickSeat( const SeatCoord &seatCoord ) {
  return existsRemark( seatCoord, REMARK_MEDA, true ); //pr_denial=true
}

// Место с пространством для ног
bool descriptSeatCreator::legSpaceSeat( const SeatCoord &seatCoord ) {
  return existsRemark( seatCoord, REMARK_LEGSPACE, false ); //pr_denial=true
}

// Место у окна без окна
bool descriptSeatCreator::windowWOWindowSeat( const SeatCoord &seatCoord ) {
  if ( windowSeat( seatCoord ) ) {
    return existsRemark( seatCoord, REMARK_WITHOUT_WINDOW, false );
  }
  return false;
}

// Место в ряду аварийного выхода
bool descriptSeatCreator::emergencyExitRowSeat( const SeatCoord &seatCoord ) {
  SALONS2::TPlace seat;
  if ( seatCoord.isValidSeat( seat ) ) {
     return ( seatCoord.isValidElemType( seat, SALONS2::ARMCHAIR_EMERGENCY_EXIT_TYPE ) );
  }
  return false;
}

// Не продается или предлагается последним
bool descriptSeatCreator::notSaleOfferedLastSeat( const SeatCoord &seatCoord ) {
  return existsLayer( seatCoord, ASTRA::cltProtect  ); //базовый слой "Резерв"
}

// Платное место
bool descriptSeatCreator::paySeat( const SeatCoord &seatCoord ) {
  SALONS2::TPlace seat;
  if ( seatCoord.isValidSeat( seat ) ) {
    if ( seatCoord.RFISCMode == SALONS2::rRFISC ) {
       return !seat.getRFISC( seatCoord.point_dep ).empty();
    }
    if ( seatCoord.RFISCMode == SALONS2::rTariff  ) {
      return !seat.GetTariff( seatCoord.point_dep ).empty();
    }
    if ( seatCoord.RFISCMode == SALONS2::rAll  ) {
      return !seat.GetTariff( seatCoord.point_dep ).empty() || !seat.getRFISC( seatCoord.point_dep ).empty();
    }
    }
  return false;
}

bool descriptSeatCreator::locationLeftElemTypeSeat( const SeatCoord &seatCoord, int line, int row, const std::string &elem_type ) { // слева или права - в ряду ???
  SALONS2::TPlace seat;
  for ( int vline = line - 1; vline >= 0; vline-- ) {
    if ( seatCoord.isValidElemType( vline, row, seat, elem_type ) ) {
      return true;
    }
  }
  return false;
}

bool descriptSeatCreator::locationRightElemTypeSeat( const SeatCoord &seatCoord, int line, int row, const std::string &elem_type ) { // слева или права - в ряду ???
  SALONS2::TPlace seat;
  for ( int vline = line + 1; vline < seatCoord.placeList->GetXsCount(); vline++ ) {
    if ( seatCoord.isValidElemType( vline, row, seat, elem_type ) ) {
      return true;
    }
  }
  return false;
}

bool descriptSeatCreator::locationElemTypeSeat( const SeatCoord &seatCoord, int line, int row, const std::string &elem_type  ) { // слева или права - в ряду ???
  return ( locationLeftElemTypeSeat( seatCoord, line, row, elem_type ) ||
           locationRightElemTypeSeat( seatCoord, line, row, elem_type ) );
}

// Место, соседнее с туалетом
bool descriptSeatCreator::locationToiletSeat( const SeatCoord &seatCoord ) {
  if ( seatCoord.isValidSeat( ) ) {
    return locationElemTypeSeat( seatCoord, seatCoord.line, seatCoord.row, SALONS2::TOILET_ELEM_TYPE );
  }
  return false;
}

// Место перед туалетом
bool descriptSeatCreator::frontToiletSeat( const SeatCoord &seatCoord ) {
  if ( seatCoord.isValidSeat( ) ) {
    return locationElemTypeSeat( seatCoord, seatCoord.line, seatCoord.row + 1, SALONS2::TOILET_ELEM_TYPE );
  }
  return false;
}

// Место позади туалета
bool descriptSeatCreator::behindToiletSeat( const SeatCoord &seatCoord ) {
  if ( seatCoord.isValidSeat( ) ) {
    return locationElemTypeSeat( seatCoord, seatCoord.line, seatCoord.row - 1, SALONS2::TOILET_ELEM_TYPE );
  }
  return false;
}

// Место посередине
bool descriptSeatCreator::middleSeat( const SeatCoord &seatCoord ) {
  if ( seatCoord.isValidSeat( ) ) {
    SALONS2::TPlace seat;
    return ( seatCoord.isValidSeat( seatCoord.line - 1, seatCoord.row, seat ) &&
             seatCoord.isValidSeat( seatCoord.line + 1, seatCoord.row, seat )  );
  }
  return false;
}

// Место в центральном секторе
bool descriptSeatCreator::middleSectorSeat( const SeatCoord &seatCoord ) {
  if ( seatCoord.isValidSeat( ) ) {
    for ( int line = seatCoord.line + 1; line < seatCoord.placeList->GetXsCount(); line ++ ) {
      if ( seatCoord.placeList->GetXsName( line ).empty() ) {
        for ( int line = seatCoord.line - 1; line >= 0; line-- ) {
          if ( seatCoord.placeList->GetXsName( line ).empty() ) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

std::pair<SeatDescrService::Enum,bool> descriptSeatCreator::translate( SeatDescrService::Enum valueType, const  SeatCoord &seatCoord ) {
  switch (valueType) {
    case SeatDescrService::Enum::LeftAirCraftSide:
      return make_pair( valueType, leftAirCraftSide( seatCoord ) );
    case SeatDescrService::Enum::RightAirCraftSide:
      return make_pair( valueType, rightAirCraftSide( seatCoord ) );
    case SeatDescrService::Enum::WindowSeat:
      return make_pair( valueType, windowSeat( seatCoord ) );
    case SeatDescrService::Enum::AisleSeat:
      return make_pair( valueType, aisleSeat( seatCoord ) );
    case SeatDescrService::Enum::SmokingSeat:
      return make_pair( valueType, smokingSeat( seatCoord ) );
    case SeatDescrService::Enum::PartitionSeat:
      return make_pair( valueType, partitionSeat( seatCoord ) );
    case SeatDescrService::Enum::FacingTailSeat:
      return make_pair( valueType, facingTailSeat( seatCoord ) );
    case SeatDescrService::Enum::ComforttableSeat:
      return make_pair( valueType, comforttableSeat( seatCoord ) );
    case SeatDescrService::Enum::NotChildrenSeat:
      return make_pair( valueType, notChildrenSeat( seatCoord ) );
    case SeatDescrService::Enum::NotBabySeat:
      return make_pair( valueType, notBabySeat( seatCoord ) );
    case SeatDescrService::Enum::NotEscortChildrenSeat:
      return make_pair( valueType, notEscortChildrenSeat( seatCoord ) );
    case SeatDescrService::Enum::NotSickSeat:
      return make_pair( valueType, notSickSeat( seatCoord ) );
    case SeatDescrService::Enum::LegSpaceSeat:
      return make_pair( valueType, legSpaceSeat( seatCoord ) );
    case SeatDescrService::Enum::WindowWOWindowSeat:
      return make_pair( valueType, windowWOWindowSeat( seatCoord ) );
    case SeatDescrService::Enum::EmergencyExitRowSeat:
      return make_pair( valueType, emergencyExitRowSeat( seatCoord ) );
    case SeatDescrService::Enum::NotSaleOfferedLastSeat:
      return make_pair( valueType, notSaleOfferedLastSeat( seatCoord ) );
    case SeatDescrService::Enum::PaySeat:
      return make_pair( valueType, paySeat( seatCoord ) );
    case SeatDescrService::Enum::LocationToiletSeat:
      return make_pair( valueType, locationToiletSeat( seatCoord ) );
    case SeatDescrService::Enum::FrontToiletSeat:
      return make_pair( valueType, frontToiletSeat( seatCoord ) );
    case SeatDescrService::Enum::BehindToiletSeat:
      return make_pair( valueType, behindToiletSeat( seatCoord ) );
    case SeatDescrService::Enum::MiddleSeat:
      return make_pair( valueType, middleSeat( seatCoord ) );
    case SeatDescrService::Enum::MiddleSectorSeat:
      return make_pair( valueType, middleSectorSeat( seatCoord ) );
    default:
      return make_pair( valueType, false );
  }
}

//================================== descrSeatType ===========================================
xmlNodePtr descrSeatType::toXML( xmlNodePtr node ) {
  xmlNodePtr n = nullptr;
  for ( auto e : *this ) {
    if ( e.second ) {
      n = n==nullptr?NewTextChild( node, "descrSeatType" ):n;
      NewTextChild( n, "code", seatDescrServices().encode( e.first ) );
    }
  }
  return n;
}

std::string descrSeatType::toPropHintFormatStr() {
  std::string hint = "";
  for ( auto e : *this ) {
    if ( e.second ) {
      if ( !hint.empty() ) {
        hint += " ";
      }
      hint += seatDescrServices().encode( e.first );
    }
  }
  return hint;
}

void descrSeatType::toXMLPropHintFormatStr( xmlNodePtr node ) {
  std::string hint = toPropHintFormatStr();
  if ( !hint.empty() ) {
    NewTextChild( node, "hint", hint );
  }
}

void descrSeatType::Trace() {
  LogTrace(TRACE5) << traceStr();
}

//================================== descrSeatTypeContainer ==================================
void descrSeatTypeContainer::toDB() {
  TQuery Qry( &OraSession );
  Qry.SQLText = CLEAR_SQL;
  Qry.CreateVariable( "point_id", otInteger, point_dep );
  Qry.Execute();
  Qry.Clear();
  Qry.SQLText = INSERT_SQL;
  Qry.CreateVariable( "point_id", otInteger, point_dep );
  Qry.DeclareVariable( "seatkey", otInteger );
  Qry.DeclareVariable( "code", otString );
  for ( auto props : *this ) {
    Qry.SetVariable( "seatkey", props.first );
    for ( auto prop : props.second ) {
      if ( prop.second ) {
         Qry.SetVariable( "code", seatDescrServices().encode( prop.first ) );
         Qry.Execute();
      }
    }
  }
}

void descrSeatTypeContainer::fromDB() {
  TQuery Qry( &OraSession );
  Qry.SQLText = SELECT_SQL;
  Qry.CreateVariable( "point_id", otInteger, point_dep );
  Qry.Execute();
  int seatkey = -1;
  descrSeatType descrSeat;
  for ( ;!Qry.Eof; Qry.Next() ) {
    if ( seatkey != Qry.FieldAsInteger( "seatkey" ) ) {
      if ( seatkey >= 0 ) { //был предыдущий
        insert( make_pair( seatkey, descrSeat ) );
        descrSeat = descrSeatType();
      }
      seatkey = Qry.FieldAsInteger( "seatkey" );
    }
    descrSeat.add( Qry.FieldAsString( "code" ) );
  }
  if ( seatkey >= 0 ) {
    insert( make_pair( seatkey, descrSeat ) );
  }
}

void descrSeatTypeContainer::toXML( xmlNodePtr node ) {
   node = NewTextChild( node, "descrSeatTypeContainer" );
   xmlNodePtr n = NewTextChild( node, "descrTypes" );
   for ( const auto e : SeatDescrService::sets() ) {
     NewTextChild( n, "code", seatDescrServices().encode( e ) );
   }
   n = NewTextChild( node, "seats" );
   for ( auto e : *this ) {
     xmlNodePtr p = e.second.toXML( n );
     p!=nullptr?SetProp( p, "seatkey", e.first ):nullptr;
   }
}

bool isConstructiveRow( SALONS2::TPlaceList* isalon, int row ) {
  for ( int line=0; line<isalon->GetXsCount(); line++ ) {
    SALONS2::TPoint p( line, row );
    if ( isalon->ValidPlace( p ) ) {
      SALONS2::TPlace *seat = isalon->place( p );
      if ( seat->visible &&
           !SALONS2::isConstructivePlace( seat->elem_type ) ) {
        return false;
      }
    }
  }
  return true;
}

void paxsWaitListDescrSeat::get( int point_dep ) {
  clear();
  TQuery Qry( &OraSession );
  std::map<int, std::map<TSeat,std::string,CompareSeat> > paxWL;
  Qry.SQLText =
    "SELECT pax_id, xname, yname, seat_descr FROM pax_seats "
    " WHERE point_id=:point_id AND NVL(pr_wl,0)=1 "
    " ORDER BY pax_id";
  Qry.CreateVariable( "point_id", otInteger, point_dep );
  Qry.Execute();
  for (; !Qry.Eof; Qry.Next() ) {
    (*this)[ Qry.FieldAsInteger( "pax_id" ) ].insert( make_pair( TSeat( Qry.FieldAsString( "yname" ), Qry.FieldAsString( "xname" ) ), Qry.FieldAsString( "seat_descr" ) ) );
  }
}

std::string paxsWaitListDescrSeat::getDescr( const std::string &format, int pax_id )
{
  std::map<int, paxDescrSeats >::const_iterator ipaxSeatDescr = find( pax_id );
  std::string res = "";
  if ( ipaxSeatDescr != end() ) {
    for ( auto descrs : ipaxSeatDescr->second ) {
      if ( "one" != format && !res.empty() ) {
        res += ",";
      }
      res += descrs.second;
      if ( "one" == format ) {
        break;
      }
    }
  }
  return res;
}


} // end namespace SEAT_DESCR
