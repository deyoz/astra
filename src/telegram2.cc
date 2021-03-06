#include <map>
#include <set>
#include "telegram.h"
#include "xml_unit.h"
#include "tlg/tlg.h"
#include "tlg/mvt_parser.h"
#include "tlg/tlg_parser.h"
#include "tlg/ucm_parser.h"
#include "astra_utils.h"
#include "stl_utils.h"
#include "salonform.h"
#include "astra_consts.h"
#include "passenger.h"
#include "remarks.h"
#include "pers_weights.h"
#include "misc.h"
#include "date_time.h"
#include "qrys.h"
#include "typeb_utils.h"
#include "emdoc.h"
#include "SalonPaxList.h"
#include "serverlib/xml_stuff.h" // ??? xml_decode_nodelist
#include "serverlib/str_utils.h"
#include <boost/regex.hpp>
#include "docs/docs_common.h"
#include "docs/docs_pax_list.h"
#include "seat_number.h"
#include "flt_settings.h"

#define NICKNAME "DEN"
#include "serverlib/slogger.h"

#include "alarms.h"
#include "TypeBHelpMng.h"
#include "html_pages.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace boost::local_time;
using namespace ASTRA;
using namespace SALONS2;

const size_t LINE_SIZE = 64;
int TST_TLG_ID; // for test purposes
const int MAX_DELAY_TIME = 6000; //mins, more than 99 hours 59 mins
const string KG = "KG";

void ExceptionFilter(string &body, TypeB::TDetailCreateInfo &info)
{
    try {
        throw;
    } catch(UserException &E) {
        body = info.err_lst.add_err(TypeB::DEFAULT_ERR, E.getLexemaData());
    } catch(exception &E) {
        body = info.err_lst.add_err(TypeB::DEFAULT_ERR, E.what());
    } catch(...) {
        body = info.err_lst.add_err(TypeB::DEFAULT_ERR, "unknown error");
    }
}

void ExceptionFilter(vector<string> &body, TypeB::TDetailCreateInfo &info)
{
    string buf;
    ExceptionFilter(buf, info);
    body.clear();
    body.push_back(buf);
}

bool CompareCompLayers( TTlgCompLayer t1, TTlgCompLayer t2 )
{
    if ( t1.yname < t2.yname )
        return true;
    else
        if ( t1.yname > t2.yname )
            return false;
        else
            if ( t1.xname < t2.xname )
                return true;
            else
              return false;
};

struct TCompareCompLayers {
  bool operator() ( const TTlgCompLayer &t1, const TTlgCompLayer &t2 ) const {
    return CompareCompLayers( t1, t2 );
  }
};

struct TCheckinPaxSeats {
    std::string gender;
    int pr_infant;
    ASTRA::TCrewType::Enum crew_type;
    std::set<TTlgCompLayer,TCompareCompLayers> seats;
    TCheckinPaxSeats():
        pr_infant(NoExists),
        crew_type(TCrewType::Unknown)
    {}
};

string getDefaultSex()
{
    return "M";
}

/*void getPaxsSeatsTranzitSalons( int point_dep, std::map<int,TCheckinPaxSeats> &checkinPaxsSeats )
{
  ProgTrace(TRACE5, "getPaxsSeatsTranzitSalons: old salons");
  checkinPaxsSeats.clear();
  SALONS2::TSalonList salonList;
  salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_dep, ASTRA::NoExists ), "" );
  std::set<ASTRA::TCompLayerType> search_layers;
  for ( int ilayer=0; ilayer<(int)cltTypeNum; ilayer++ ) {
    ASTRA::TCompLayerType layer_type = (ASTRA::TCompLayerType)ilayer;
    BASIC_SALONS::TCompLayerType layer_elem;
    if ( BASIC_SALONS::TCompLayerTypes::Instance()->getElem( layer_type, layer_elem ) &&
         layer_elem.getOccupy() ) {
      search_layers.insert( layer_type );
    }
  }
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT NVL(pax_doc.gender,:sex) as gender "
    " FROM pax_doc "
    " WHERE pax_id=:pax_id";
  Qry.DeclareVariable( "pax_id", otInteger );
  Qry.CreateVariable( "sex", otString, getDefaultSex() );
  TSalonPassengers passengers;
  SALONS2::TGetPassFlags flags;
  flags.setFlag( SALONS2::gpPassenger );
  flags.setFlag( SALONS2::gpWaitList ); //!!!
  salonList.getPassengers( passengers, flags );
  SALONS2::TSalonPassengers::iterator pass_dep = passengers.find( point_dep );
  if ( pass_dep == passengers.end() ) {
    return;
  }
  //point_arv
  for ( SALONS2::TIntArvSalonPassengers::iterator iroute=pass_dep->second.begin(); iroute!=pass_dep->second.end(); iroute++ ) {
    //class
    for ( TIntClassSalonPassengers::iterator iclass=iroute->second.begin();
          iclass!=iroute->second.end(); iclass++ ) {
      //grp_status
      for ( TIntStatusSalonPassengers::iterator igrp_layer=iclass->second.begin();
            igrp_layer!=iclass->second.end(); igrp_layer++ ) {
        //pass.grp+reg_no
        for ( std::set<TSalonPax,ComparePassenger>::iterator ipass=igrp_layer->second.begin(); ipass!=igrp_layer->second.end(); ipass++ ) {
          if ( ipass->layers.empty() ) {
            continue;
          }
          for ( TLayersPax::const_iterator ilayer=ipass->layers.begin(); ilayer!=ipass->layers.end(); ilayer++ ) {
            if ( search_layers.find( ilayer->first.layer_type ) == search_layers.end() ||
                 ilayer->second.waitListReason.layerStatus != layerValid ) {
              continue;
            }
            TCheckinPaxSeats checkinPaxSeats;
            switch( ipass->pers_type ) {
              case ASTRA::adult:
                Qry.SetVariable( "pax_id", ipass->pax_id );
                Qry.Execute();
                if ( Qry.Eof ) {
                  checkinPaxSeats.gender = getDefaultSex();
                }
                else {
                  checkinPaxSeats.gender = (string(Qry.FieldAsString( "gender" )).substr(0,1) == "F" ? "F" : "M");
                }
                break;
              case ASTRA::child:
                checkinPaxSeats.gender = "C";
                break;
              case ASTRA::baby:
                checkinPaxSeats.gender = "I";
                break;
              default:
                break;
            }
            TTlgCompLayer compLayer;
            compLayer.pax_id = ilayer->first.getPaxId();
              compLayer.point_dep = ilayer->first.point_dep;
              compLayer.point_arv = ilayer->first.point_arv;
              compLayer.layer_type = ilayer->first.layer_type;
              for ( std::set<TPlace*,CompareSeats>::iterator iseat=ilayer->second.seats.begin();
                  iseat!=ilayer->second.seats.end(); iseat++ ) {
              compLayer.xname = (*iseat)->xname;
                compLayer.yname = (*iseat)->yname;
                checkinPaxSeats.seats.insert( compLayer );
            }
            checkinPaxsSeats.insert( make_pair( ipass->pax_id, checkinPaxSeats ) );
              break;
          }
        }
      }
    }
  }
} */

void getSalonPaxsSeats( int point_dep, std::map<int,TCheckinPaxSeats> &checkinPaxsSeats, bool pr_tranzit )
{
  checkinPaxsSeats.clear();
  std::set<ASTRA::TCompLayerType> search_layers;
  for ( int ilayer=0; ilayer<(int)cltTypeNum; ilayer++ ) {
    ASTRA::TCompLayerType layer_type = (ASTRA::TCompLayerType)ilayer;
    BASIC_SALONS::TCompLayerElem layer_elem;
    if ( BASIC_SALONS::TCompLayerTypes::Instance()->getElem( layer_type, layer_elem ) &&
         layer_elem.getOccupy() ) {
      search_layers.insert( layer_type );
    }
  }

//  if ( SALONS2::isTranzitSalons( point_dep ) ) {     //!!!㤠???? ??? ??⠭???? ??? ????? ᠫ????
    SALONS2::TSalonList salonList;
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_dep, ASTRA::NoExists ),
                          "", NoExists );
    TSalonPassengers passengers;
    SALONS2::TGetPassFlags flags;
    flags.setFlag( SALONS2::gpPassenger ); //⮫쪮 ???ᠦ?஢ ? ???⠬?
    if(pr_tranzit)
        flags.setFlag( SALONS2::gpTranzits ); // ?࠭??⭨???
    TSectionInfo sectionInfo;
    salonList.getSectionInfo( sectionInfo, flags );
    TLayersSeats layerSeats;
    sectionInfo.GetLayerSeats( layerSeats );
    std::map<int,TSalonPax> paxs;
    sectionInfo.GetPaxs( paxs );
    for ( TLayersSeats::iterator ilayer=layerSeats.begin();
          ilayer!=layerSeats.end(); ilayer++ ) {
      if ( search_layers.find( ilayer->first.layerType() ) == search_layers.end() ) {
        tst();
        continue;
      }
      if ( paxs.find( ilayer->first.getPaxId() ) == paxs.end() ) {
        ProgError( STDLOG, "telegram2::getSalonPaxsSeats: pax_id=%d not found", ilayer->first.getPaxId() );
      }
      tst();
      string gender;
      switch( paxs[ ilayer->first.getPaxId() ].pers_type ) {
        case ASTRA::adult:
          switch ( paxs[ ilayer->first.getPaxId() ].is_female ) {
            case ASTRA::NoExists:
              gender = getDefaultSex();
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
      for ( TPassSeats::const_iterator iseat=ilayer->second.begin();
            iseat!=ilayer->second.end(); iseat++ ) {
        TTlgCompLayer compLayer;
        compLayer.pax_id = ilayer->first.getPaxId();
        compLayer.point_dep = ilayer->first.point_dep();
        compLayer.point_arv = ilayer->first.point_arv();
        compLayer.layer_type = ilayer->first.layerType();
        compLayer.xname = iseat->line;
        compLayer.yname = iseat->row;
        checkinPaxsSeats[ ilayer->first.getPaxId() ].gender = gender;
        checkinPaxsSeats[ ilayer->first.getPaxId() ].pr_infant = paxs[ ilayer->first.getPaxId() ].pr_infant;
        checkinPaxsSeats[ ilayer->first.getPaxId() ].crew_type = paxs[ ilayer->first.getPaxId() ].crew_type;
        checkinPaxsSeats[ ilayer->first.getPaxId() ].seats.insert( compLayer );
      }
    }

/*    TSeatsPaxs seatsPaxs;
    sectionInfo.GetSalonPaxs( seatsPaxs );
    for ( TSeatsPaxs::const_iterator ipax=seatsPaxs.begin();
          ipax!=seatsPaxs.end(); ipax++ ) {
      TCheckinPaxSeats checkinPaxSeats;

      if ( ipax->)

      if ( ipax.getnder checkinPaxSeats )
      TTlgCompLayer compLayer;
      compLayer.pax_id = iseat->layers.begin()->pax_id;
        compLayer.point_dep = iseat->layers.begin()->point_dep;
        compLayer.point_arv = iseat->layers.begin()->point_arv;
        compLayer.layer_type = iseat->layers.begin()->layer_type;
        compLayer.xname = iseat->xname;
        compLayer.yname = iseat->yname;
      checkinPaxsSeats[ iseat->layers.begin()->pax_id ].seats.insert( compLayer );
    }
    return;
  //}
  ProgTrace(TRACE5, "getSalonPaxsSeats: old salons");
  set<ASTRA::TCompLayerType> occupies;
  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT code FROM comp_layer_types WHERE PR_OCCUPY<>0";
  Qry.Execute();
  for ( ;!Qry.Eof; Qry.Next() ) {
    occupies.insert( DecodeCompLayerType( Qry.FieldAsString( "code" ) ) );
  }
  SALONS2::TSalons Salons( point_dep, SALONS2::rTripSalons );
  Salons.Read();
  Qry.Clear();
  Qry.SQLText =
      "SELECT pax.pax_id,pax.reg_no,pax_grp.grp_id,"
       "      pax_grp.class,pax.refuse,"
       "       pax.pers_type, "
       "       NVL(pax_doc.gender,:sex) as gender, "
       "       pax.seats seats "
       " FROM pax_grp, pax, pax_doc "
       " WHERE pax_grp.grp_id=pax.grp_id AND "
       "       pax_grp.point_dep=:point_id AND "
     "       pax_grp.status NOT IN ('E') AND "
       "       pax.wl_type IS NULL AND "
       "       pax.seats > 0 AND "
       "       salons.is_waitlist(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,rownum)=0 AND "
       "       pax.pax_id=pax_doc.pax_id(+) ";
  Qry.CreateVariable( "point_id", otInteger, point_dep );
  Qry.CreateVariable( "sex", otString, getDefaultSex() );
  Qry.Execute();
  for ( ; not Qry.Eof; Qry.Next() ) {
    TCheckinPaxSeats checkinPaxSeats;
    TPerson person_type = DecodePerson( Qry.FieldAsString( "pers_type" ) );
    switch( person_type ) {
      case ASTRA::adult:
        checkinPaxSeats.gender = (string(Qry.FieldAsString( "gender" )).substr(0,1) == "F" ? "F" : "M");
        break;
      case ASTRA::child:
        checkinPaxSeats.gender = "C";
        break;
      case ASTRA::baby:
        checkinPaxSeats.gender = "I";
        break;
      default:
        break;
    }
    checkinPaxsSeats.insert( make_pair( Qry.FieldAsInteger( "pax_id" ), checkinPaxSeats ) );
  }
  for ( vector<TPlaceList*>::iterator isalonList=Salons.placelists.begin(); isalonList!=Salons.placelists.end(); isalonList++) { // ?஡?? ?? ᠫ????
    for ( IPlace iseat=(*isalonList)->places.begin(); iseat!=(*isalonList)->places.end(); iseat++ ) { // ?஡?? ?? ???⠬ ? ᠫ???
      if ( iseat->layers.empty() ||
           occupies.find( iseat->layers.begin()->layer_type ) == occupies.end() ||
           checkinPaxsSeats.find( iseat->layers.begin()->pax_id ) == checkinPaxsSeats.end() ) {
        continue;
      }
      TTlgCompLayer compLayer;
      compLayer.pax_id = iseat->layers.begin()->pax_id;
        compLayer.point_dep = iseat->layers.begin()->point_dep;
        compLayer.point_arv = iseat->layers.begin()->point_arv;
        compLayer.layer_type = iseat->layers.begin()->layer_type;
        compLayer.xname = iseat->xname;
        compLayer.yname = iseat->yname;
      checkinPaxsSeats[ iseat->layers.begin()->pax_id ].seats.insert( compLayer );
    }
  }*/
}

void getSalonLayers( int point_id,
                     int point_num,
                     int first_point,
                     bool pr_tranzit,
                     TTlgCompLayerList &complayers,
                     bool pr_blocked )
{
  complayers.clear();
  SALONS2::TSalonList salonList;
  salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, ASTRA::NoExists ),
                        "", NoExists );
  SALONS2::TGetPassFlags flags;
  TSectionInfo sectionInfo;
  salonList.getSectionInfo( sectionInfo, flags );
  complayers.pr_craft_lat = salonList.isCraftLat();
  std::vector<std::pair<TLayerPrioritySeat,TPassSeats> > layersSeats;
  TTlgCompLayer comp_layer;
  int next_point_arv = ASTRA::NoExists;

  for ( int ilayer=0; ilayer<(int)cltTypeNum; ilayer++ ) {
    ASTRA::TCompLayerType layer_type = (ASTRA::TCompLayerType)ilayer;
    BASIC_SALONS::TCompLayerElem layer_elem;
    if ( (pr_blocked && (layer_type == cltBlockCent || layer_type == cltDisable)) ||
         (!pr_blocked &&
            BASIC_SALONS::TCompLayerTypes::Instance()->getElem( layer_type, layer_elem ) &&
            layer_elem.getOccupy()) ) {
      sectionInfo.GetCurrentLayerSeat( layer_type, layersSeats );
      for ( std::vector<std::pair<TLayerPrioritySeat,TPassSeats> >::iterator ilayer=layersSeats.begin();
            ilayer!=layersSeats.end(); ilayer++ ) {
        if ( ilayer->first.point_id() != point_id ) {
          continue;
        }
        comp_layer.pax_id = ilayer->first.getPaxId();
        comp_layer.point_dep = ilayer->first.point_dep();
        if ( comp_layer.point_dep == ASTRA::NoExists ) {
          comp_layer.point_dep = ilayer->first.point_id();
        }
        comp_layer.point_arv = ilayer->first.point_arv();
        if ( comp_layer.point_arv == ASTRA::NoExists ) { //?? ᫥?. ?㭪??
          if ( next_point_arv == ASTRA::NoExists ) {
            TTripRoute route;
            TTripRouteItem next_airp;
            route.GetNextAirp(NoExists,
                              point_id,
                              point_num,
                              first_point,
                              pr_tranzit,
                              trtNotCancelled,
                              next_airp);
            if ( next_airp.point_id == NoExists )
              throw Exception( "getSalonLayers: inext_airp.point_id not found, point_dep="+IntToString( point_id ) );
            next_point_arv = next_airp.point_id;
          }
          comp_layer.point_arv = next_point_arv;
        }
        comp_layer.layer_type = ilayer->first.layerType();
        for ( TPassSeats::iterator iseat=ilayer->second.begin();
              iseat!=ilayer->second.end(); iseat++ ) {
          comp_layer.xname = iseat->line;
          comp_layer.yname = iseat->row;
          complayers.push_back( comp_layer );
        }
      }
    }
  }
  // ?????஢?? ?? yname, xname
  sort( complayers.begin(), complayers.end(), CompareCompLayers );
}

void getSalonLayers(const TypeB::TDetailCreateInfo &info,
                    TTlgCompLayerList &complayers,
                    bool pr_blocked)
{
  getSalonLayers(info.point_id,
                 info.point_num,
                 info.first_point,
                 info.pr_tranzit,
                 complayers,
                 pr_blocked);
};


//!!!㤠???? ??? ??⠭???? ??? ????? ᠫ????
/*void ReadTranzitSalons( int point_id,
                        int point_num,
                        int first_point,
                        bool pr_tranzit,
                        vector<TTlgCompLayer> &complayers,
                        bool pr_blocked )
{
  complayers.clear();
  SALONS2::TSalonList salonList;
  salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, ASTRA::NoExists ), SALONS2::rfTranzitVersion, "" );
  std::set<ASTRA::TCompLayerType> search_layers;
  for ( int ilayer=0; ilayer<(int)cltTypeNum; ilayer++ ) {
    ASTRA::TCompLayerType layer_type = (ASTRA::TCompLayerType)ilayer;
    BASIC_SALONS::TCompLayerType layer_elem;
    if ( (pr_blocked && layer_type == cltBlockCent) ||
         (!pr_blocked &&
            BASIC_SALONS::TCompLayerTypes::Instance()->getElem( layer_type, layer_elem ) &&
            layer_elem.getOccupy()) ) {
      search_layers.insert( layer_type );
    }
  }
  TTlgCompLayer comp_layer;
  int next_point_arv = ASTRA::NoExists;
  for ( TSalonList::iterator isalonList=salonList.begin();
        isalonList!=salonList.end(); isalonList++ ) {
    std::map<int, std::set<SALONS2::TSeatLayer,SALONS2::SeatLayerCompare>,classcomp > layers;
    for ( TPlaces::iterator iseat=(*isalonList)->places.begin();
          iseat!=(*isalonList)->places.end(); iseat++ ) {
      iseat->GetLayers( layers, glAll );
       std::map<int, std::set<SALONS2::TSeatLayer,SALONS2::SeatLayerCompare>,classcomp >::iterator ilayers = layers.find( point_id );
      if ( ilayers == layers.end() ) {
        continue;
      }
      for ( std::set<SALONS2::TSeatLayer,SALONS2::SeatLayerCompare>::iterator ilayer=ilayers->second.begin();
            ilayer!=ilayers->second.end(); ilayer++ ) {
        if ( search_layers.find( ilayer->layer_type ) != search_layers.end() ) { //???? ????????
          comp_layer.pax_id = ilayer->getPaxId();
          comp_layer.point_dep = ilayer->point_dep;
          if ( comp_layer.point_dep == ASTRA::NoExists ) {
            comp_layer.point_dep = ilayer->point_id;
          }
          comp_layer.point_arv = ilayer->point_arv;
          if ( comp_layer.point_arv == ASTRA::NoExists ) { //?? ᫥?. ?㭪??
            if ( next_point_arv == ASTRA::NoExists ) {
              TTripRoute route;
              TTripRouteItem next_airp;
              route.GetNextAirp(NoExists,
                                point_id,
                                point_num,
                                first_point,
                                pr_tranzit,
                                trtNotCancelled,
                                next_airp);
              if ( next_airp.point_id == NoExists )
                throw Exception( "ReadSalons: inext_airp.point_id not found, point_dep="+IntToString( point_id ) );
              next_point_arv = next_airp.point_id;
            }
            comp_layer.point_arv = next_point_arv;
          }
          comp_layer.layer_type = ilayer->layer_type;
          comp_layer.xname = iseat->xname;
          comp_layer.yname = iseat->yname;
          complayers.push_back( comp_layer );
        }
      }
    }
  }
  // ?????஢?? ?? yname, xname
  sort( complayers.begin(), complayers.end(), CompareCompLayers );
}

void ReadSalons(const TypeB::TDetailCreateInfo &info,
                vector<TTlgCompLayer> &complayers,
                bool pr_blocked)
{
  ReadSalons(info.point_id,
             info.point_num,
             info.first_point,
             info.pr_tranzit,
             complayers,
             pr_blocked);
};



void ReadSalons(int point_id,
                int point_num,
                int first_point,
                bool pr_tranzit,
                vector<TTlgCompLayer> &complayers,
                bool pr_blocked)
{
    complayers.clear();
    if ( SALONS2::isTranzitSalons( point_id ) ) { //!!!㤠???? ??? ??⠭???? ??? ????? ᠫ????
      ReadTranzitSalons( point_id, //!!!㤠???? ??? ??⠭???? ??? ????? ᠫ???? - ReadTranzitSalons
                         point_num,
                         first_point,
                         pr_tranzit,
                         complayers,
                         pr_blocked );
      return;
    }
    vector<ASTRA::TCompLayerType> layers;
    if(pr_blocked)
        layers.push_back(cltBlockCent);
    else {
        TQuery Qry(&OraSession);
        Qry.SQLText = "SELECT code FROM comp_layer_types WHERE PR_OCCUPY<>0";
        Qry.Execute();
        while ( !Qry.Eof ) {
            layers.push_back( DecodeCompLayerType( Qry.FieldAsString( "code" ) ) );
            Qry.Next();
        }
    }
    TTlgCompLayer comp_layer;
    int next_point_arv = -1;

    SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
    Salons.Read();
    for ( vector<TPlaceList*>::iterator ipl=Salons.placelists.begin(); ipl!=Salons.placelists.end(); ipl++ ) { // ?஡?? ?? ᠫ????
        for ( IPlace ip=(*ipl)->places.begin(); ip!=(*ipl)->places.end(); ip++ ) { // ?஡?? ?? ???⠬ ? ᠫ???
            bool pr_break = false;
            for ( vector<ASTRA::TCompLayerType>::iterator ilayer=layers.begin(); ilayer!=layers.end(); ilayer++ ) { // ?஡?? ?? ᫮?? where pr_occupy<>0
                for ( vector<TPlaceLayer>::iterator il=ip->layers.begin(); il!=ip->layers.end(); il++ ) { // ?஡?? ?? ᫮?? ?????
                    if ( il->layer_type == *ilayer ) { // ??諨 ?㦭?? ᫮?
                        if ( il->point_dep == NoExists )
                            comp_layer.point_dep = point_id;
                        else
                            comp_layer.point_dep = il->point_dep;
                        if ( il->point_arv == NoExists )  {
                            if ( next_point_arv == -1 ) {
                                TTripRoute route;
                                TTripRouteItem next_airp;
                                route.GetNextAirp(NoExists,
                                                  point_id,
                                                  point_num,
                                                  first_point,
                                                  pr_tranzit,
                                                  trtNotCancelled,
                                                  next_airp);

                                if ( next_airp.point_id == NoExists )
                                    throw Exception( "ReadSalons: inext_airp.point_id not found, point_dep="+IntToString( point_id ) );
                                else
                                    next_point_arv = next_airp.point_id;
                            }
                            comp_layer.point_arv = next_point_arv;
                        }
                        else
                            comp_layer.point_arv = il->point_arv;
                        comp_layer.layer_type = il->layer_type;
                        comp_layer.xname = ip->xname;
                        comp_layer.yname = ip->yname;
                        comp_layer.pax_id = il->pax_id;
                        complayers.push_back( comp_layer );
                        pr_break = true;
                        break;
                    }
                }
                if ( pr_break ) // ?????稫? ?????? ?? ᫮?? ?????
                    break;
            }
        }
    }
    // ?????஢?? ?? yname, xname
    sort( complayers.begin(), complayers.end(), CompareCompLayers );
}
*/
struct TTlgDraft {
    private:
        TypeB::TDetailCreateInfo &tlg_info;
        int find_duplicate(TTlgOutPartInfo &tlg_row);
    public:
        vector<TypeB::TDraftPart> parts;
        void Save(TTlgOutPartInfo &info);
        void Commit(TTlgOutPartInfo &info);
        TTlgDraft(TypeB::TDetailCreateInfo &tlg_info_val): tlg_info(tlg_info_val) {}
        void check(string &val);
};

void TTlgDraft::check(string &value)
{
    bool opened = false;
    string result, err;
    for(string::const_iterator i=value.begin();i!=value.end();i++)
    {
        char c=*i;
        if (!IsAscii7(c)) {
            // rus
            if(not opened) {
                opened = true;
                err = c;
            } else
                err += c;
        } else {
            // lat
            if(opened) {
                opened = false;
                result += tlg_info.err_lst.add_err(err, "non lat chars encountered");
            }
            result += *i;
        }
    }
    if(opened) result += tlg_info.err_lst.add_err(err, "non lat chars encountered");
    value = result;
}

int TTlgDraft::find_duplicate(TTlgOutPartInfo &tlg_row)
{
    QParams QryParams;
    QryParams
        << QParam("point_id", otInteger, tlg_row.point_id)
        << QParam("type", otString, tlg_row.tlg_type)
        << QParam("addr", otString, tlg_row.addr)
        << QParam("pr_lat", otInteger, tlg_row.pr_lat);

    TCachedQuery Qry(
            "SELECT * from "
            "    tlg_out "
            "where "
            "   point_id = :point_id and "
            "   type = :type and "
            "   addr = :addr and "
            "   pr_lat = :pr_lat ",
            QryParams
            );

    Qry.get().Execute();

    typedef map<int, TTlgOutPartInfo> t_db_tlg;

    typedef map<int, t_db_tlg> t_db_tlgs;

    t_db_tlgs db_tlgs;

    // ????砥? ? ??????
    for(; not Qry.get().Eof; Qry.get().Next()) {
        TTlgOutPartInfo part;
        part.fromDB(Qry.get());
        db_tlgs[part.id][part.num] = part;
    }

    int result = NoExists;
    TDateTime time_create = NoExists;
    TDateTime latest_time_create = NoExists;
    // ?஡?? ?? ⥫??ࠬ???
    for(t_db_tlgs::iterator i = db_tlgs.begin(); i != db_tlgs.end(); i++) {
        // ?஡?? ?? ?????? ⥫??ࠬ?? ? ?ࠢ?????
        vector<TypeB::TDraftPart>::const_iterator iv = parts.begin();
        t_db_tlg::iterator j = i->second.begin();
        bool differ = true;
        TDateTime curr_time_create = NoExists;
        for(; j != i->second.end() and iv != parts.end(); j++, iv++) {
            TTlgOutPartInfo &part = j->second;
            curr_time_create = part.time_create;
            if(latest_time_create == NoExists or latest_time_create < curr_time_create) {
                latest_time_create = curr_time_create;
            }
            if( not (
                        part.heading == iv->heading and
                        part.body == iv->body and
                        part.ending == iv->ending
                    )
              ) {
                break;
            }
            // ?᫨ ??? ?訡?? ??? ?? ?ॡ????? ???. ????. ? ??? ?⮬ ⥫??ࠬ?? ?? ???ࠢ????
            // ?? ????砥?, ??? ࠧ??????????
            // (?.?. ?㡫???? ?? ??????, ?ॡ????? ???ࠢ??)
            differ = not (part.has_errors or not part.completed) and part.time_send_act == NoExists;
        }
        if(not differ) // ?????? ?㡫????
        {
            if(time_create == NoExists or time_create < curr_time_create) {
                time_create = curr_time_create;
                result = i->first;
            }
        }

    }

    if(result != NoExists and time_create < latest_time_create)
        result = NoExists;

    return result;
}

void TTlgDraft::Commit(TTlgOutPartInfo &tlg_row)
{
    // ? ???????? ᮧ????? ⥫??ࠬ?? ?????, ᮤ?ঠ騥 ?訡??, ????? ??
    // ??????? ? ?⮣???? ⥪??. ???⮬? ???? ᨭ?஭???஢??? ᯨ᮪ ?訡??
    // ? ⥪?⮬ ⥫??ࠬ??. ?.?. 㤠???? ?? ᯨ᪠ ??????????騥 ? ⥪??? ?訡??.
    tlg_info.err_lst.fix(parts);
    bool no_errors = tlg_info.err_lst.empty();

    // ?஢?ઠ ?? ???/?? ???.
    for(vector<TypeB::TDraftPart>::iterator iv = parts.begin(); iv != parts.end(); iv++){
        if(tlg_info.is_lat() and no_errors) {
            check(iv->addr);
            check(iv->origin);
            check(iv->heading);
            check(iv->body);
            check(iv->ending);
        }
        bool heading_visible = iv == parts.begin();
        bool ending_visible = iv + 1 == parts.end();
        tlg_info.err_lst.pack(*iv, heading_visible, ending_visible);
    }

    int tlg_out_id = NoExists;
    // ?᫨ ⥫??ࠬ?? ??⮬?????᪠? ? ??⮭?????
    // (?.?. ?? ????? ?⢥⮬ ?? ⫣-??????) - ?஢?ઠ ?? ?㡫?஢????
    if(
            not tlg_info.manual_creation and
            tlg_info.typeb_in_id == NoExists
      )
        tlg_out_id = find_duplicate(tlg_row);
    if(tlg_out_id != NoExists) {
        TReqInfo::Instance()->LocaleToLog("EVT.TLG.OUT.DUPLICATED", LEvntPrms()
                << PrmElem<std::string>("name", etTypeBType, tlg_row.tlg_type, efmtNameShort)
                << PrmSmpl<int>("id", tlg_out_id),
                evtTlg, tlg_row.point_id, tlg_out_id);
    } else {
        // ?????।?⢥??? ?????? ? ????.
        tlg_row.num = 1;
        for(vector<TypeB::TDraftPart>::iterator iv = parts.begin(); iv != parts.end(); iv++){
            tlg_row.addr = iv->addr;
            tlg_row.origin = iv->origin;
            tlg_row.heading = iv->heading;
            tlg_row.body = iv->body;
            tlg_row.ending = iv->ending;
            TelegramInterface::SaveTlgOutPart(tlg_row, false, false);
        }
    }
}

void TTlgDraft::Save(TTlgOutPartInfo &info)
{
    TypeB::TDraftPart part;
    part.addr = info.addr;
    part.origin = info.origin;
    part.heading = info.heading;
    part.body = info.body;
    part.ending = info.ending;
    parts.push_back(part);
    info.num++;
}

void simple_split(ostringstream &heading, size_t part_len, TTlgDraft &tlg_draft, TTlgOutPartInfo &tlg_row, vector<string> &body)
{
    if(body.empty())
        tlg_row.body = "NIL" + TypeB::endl;
    else
        for(vector<string>::iterator iv = body.begin(); iv != body.end(); iv++) {
            part_len += iv->size() + TypeB::endl.size();
            if(part_len > PART_SIZE) {
                tlg_draft.Save(tlg_row);
                tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
                tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
                tlg_row.body = *iv + TypeB::endl;
                part_len = tlg_row.textSize();
            } else
                tlg_row.body += *iv + TypeB::endl;
        }
}

struct TWItem {
    int bagAmount;
    int bagWeight;
    int rkAmount;
    int rkWeight;
    TBagKilos excess_wt; // used in CKIN_REPORT
    TBagPieces excess_pc; // ???? ?? ?ᯮ????????, ?? ?????뢠????
    void get(int grp_id, int bag_pool_num);
    void get(int pax_id);
    void ToTlg(vector<string> &body);
    TWItem():
        bagAmount(0),
        bagWeight(0),
        rkAmount(0),
        rkWeight(0),
        excess_wt(0),
        excess_pc(0)
    {};
    TWItem &operator += (const TWItem &val)
    {
        bagAmount += val.bagAmount;
        bagWeight += val.bagWeight;
        rkAmount += val.rkAmount;
        rkWeight += val.rkWeight;
        excess_wt += val.excess_wt;
        excess_pc += val.excess_pc;
        return *this;
    }
    void operator = (const TWItem &val)
    {
        bagAmount = val.bagAmount;
        bagWeight = val.bagWeight;
        rkAmount = val.rkAmount;
        rkWeight = val.rkWeight;
        excess_wt = val.excess_wt;
        excess_pc = val.excess_pc;
    }
};

struct TMItem {
    TMktFlight m_flight;
    void get(TypeB::TDetailCreateInfo &info, int pax_id);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

void TMItem::get(TypeB::TDetailCreateInfo &info, int pax_id)
{
        m_flight.getByPaxId(pax_id);
}

void TMItem::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    if(m_flight.empty())
        return;
    const TypeB::TMarkInfoOptions &markOptions=*(info.optionsAs<TypeB::TMarkInfoOptions>());

    if(markOptions.mark_info.empty()) { // ??? ???? ? ??થ⨭? ३??
        if(info == m_flight) // ?᫨ 䠪?. (info) ? ????. (m_flight) ᮢ??????, ???? ?? ?뢮???
            return;
    } else if(markOptions.pr_mark_header) {
        if(
                info.airp_dep == m_flight.airp_dep and
                info.scd_local_day == m_flight.scd_day_local
          )
            return;
    }
    ostringstream result;
    result
        << ".M/"
        << info.TlgElemIdToElem(etAirline, m_flight.airline)
        << setw(3) << setfill('0') << m_flight.flt_no
        << (m_flight.suffix.empty() ? "" : info.TlgElemIdToElem(etSuffix, m_flight.suffix))
        << info.TlgElemIdToElem(etSubcls, m_flight.subcls) //??? ???३?? ?㤥? ????? NVL(pax.subclass,pax_grp.class) mark_subcls !
        << setw(2) << setfill('0') << m_flight.scd_day_local
        << info.TlgElemIdToElem(etAirp, m_flight.airp_dep)
        << info.TlgElemIdToElem(etAirp, m_flight.airp_arv);
    body.push_back(result.str());
}

struct TFTLPax;
struct TETLPax;
struct TTPLPax;
struct TASLPax;
struct TPFSPax;

struct TName {
    string surname;
    string name;
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body, string postfix = "");
    string ToPILTlg(TypeB::TDetailCreateInfo &info) const;
};

TTrickyGender::Enum getGender(TQuery &Qry)
{
    if(Qry.Eof)
        throw Exception("getGender: Qry.Eof");
    TPerson pers_type = DecodePerson(Qry.FieldAsString("pers_type"));
    return CheckIn::TSimplePaxItem::getTrickyGender(pers_type, CheckIn::TSimplePaxItem::genderFromDB(Qry));
}

TTrickyGender::Enum getGender(int pax_id)
{
    TCachedQuery Qry(
            "select is_female, pers_type from pax where pax_id = :pax_id",
            QParams() << QParam("pax_id", otInteger, pax_id));
    Qry.get().Execute();
    return getGender(Qry.get());
}


namespace PRL_SPACE {
    struct TPNRItem {
        string airline, addr;
        void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
    };

    void TPNRItem::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        body.push_back(".L/" + convert_pnr_addr(addr, info.is_lat()) + '/' + info.TlgElemIdToElem(etAirline, airline));
    }

    struct TPNRList {
        vector<TPNRItem> items;
        void get(int pnr_id);
        virtual void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
        virtual ~TPNRList() {};
    };

    struct TPNRListAddressee: public TPNRList {
        void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
    };

    void TPNRListAddressee::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        if(items.size() == 1)
            items.back().ToTlg(info, body);
        else {
            string airline = info.airline_mark();
            if(airline.empty()) airline = info.airline;
            for(vector<TPNRItem>::iterator iv = items.begin(); iv != items.end(); iv++)
                if(airline == iv->airline) {
                    iv->ToTlg(info, body);
                    break;
                }
        }
    }

    void TPNRList::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        for(vector<TPNRItem>::iterator iv = items.begin(); iv != items.end(); iv++)
            iv->ToTlg(info, body);
    }

    void TPNRList::get(int pnr_id)
    {
        if(pnr_id == NoExists) return;
        QParams QryParams;
        QryParams << QParam("pnr_id", otInteger, pnr_id);
        TCachedQuery Qry("SELECT airline,addr FROM pnr_addrs WHERE pnr_id=:pnr_id ORDER BY addr,airline", QryParams);
        Qry.get().Execute();
        for(; !Qry.get().Eof; Qry.get().Next()) {
            TPNRItem pnr;
            pnr.airline = Qry.get().FieldAsString("airline");
            pnr.addr = Qry.get().FieldAsString("addr");
            items.push_back(pnr);
        }
    }

    struct TInfantsItem {
        int pax_id;
        int grp_id;
        int bag_pool_num;
        int reg_no;
        string surname;
        string name;
        int parent_pax_id;
        int temp_parent_id;
        string ticket_no;
        int coupon_no;
        string ticket_rem;
        TWItem W;
        void dump();
        TInfantsItem() {
            pax_id = NoExists;
            grp_id = NoExists;
            bag_pool_num = NoExists;
            reg_no = NoExists;
            parent_pax_id = NoExists;
            temp_parent_id = NoExists;
            coupon_no = NoExists;
        }
    };

    void TInfantsItem::dump()
    {
        ProgTrace(TRACE5, "TInfantsItem");
        ProgTrace(TRACE5, "pax_id: %d", pax_id);
        ProgTrace(TRACE5, "grp_id: %d", grp_id);
        ProgTrace(TRACE5, "bag_pool_num: %d", bag_pool_num);
        ProgTrace(TRACE5, "reg_no: %d", reg_no);
        ProgTrace(TRACE5, "surname: %s", surname.c_str());
        ProgTrace(TRACE5, "name: %s", name.c_str());
        ProgTrace(TRACE5, "pax_id: %d", parent_pax_id);
        ProgTrace(TRACE5, "crs_pax_id: %d", parent_pax_id);
        ProgTrace(TRACE5, "ticket_no: %s", ticket_no.c_str());
        ProgTrace(TRACE5, "coupon_no: %d", coupon_no);
        ProgTrace(TRACE5, "ticket_rem: %s", ticket_rem.c_str());
        ProgTrace(TRACE5, "--------------------");
    }

    struct TAdultsItem {
        int grp_id;
        int pax_id;
        int reg_no;
        string surname;
        void dump();
        TAdultsItem() {
            grp_id = NoExists;
            pax_id = NoExists;
            reg_no = NoExists;
        }
    };

    void TAdultsItem::dump()
    {
        ProgTrace(TRACE5, "TAdultsItem");
        ProgTrace(TRACE5, "grp_id: %d", grp_id);
        ProgTrace(TRACE5, "pax_id: %d", pax_id);
        ProgTrace(TRACE5, "reg_no: %d", reg_no);
        ProgTrace(TRACE5, "surname: %s", surname.c_str());
        ProgTrace(TRACE5, "--------------------");
    }

    struct TInfants {
        vector<TInfantsItem> items;
        void get(TypeB::TDetailCreateInfo &info);
    };

    void TInfants::get(TypeB::TDetailCreateInfo &info)
    {
        items.clear();
        if(info.point_id == NoExists) return;

        const TypeB::TPRLOptions *PRLOptions=NULL;
        if(info.optionsIs<TypeB::TPRLOptions>())
            PRLOptions=info.optionsAs<TypeB::TPRLOptions>();

        TQuery Qry(&OraSession);
        string SQLText =
            "SELECT pax.grp_id, "
            "       pax.bag_pool_num, "
            "       pax.pax_id, "
            "       pax.reg_no, "
            "       pax.surname, "
            "       pax.name, "
            "       pax.ticket_no, "
            "       pax.coupon_no, "
            "       pax.ticket_rem, "
            "       crs_inf.pax_id AS crs_pax_id "
            "FROM pax_grp,pax,crs_inf "
            "WHERE "
            "     pax_grp.grp_id=pax.grp_id AND "
            "     pax_grp.point_dep=:point_id AND "
            "     pax_grp.status NOT IN ('E') AND "
            "     pax.seats=0 AND ";
        if((PRLOptions and PRLOptions->pax_state == "CKIN") or info.get_tlg_type() == "LCI")
            SQLText += " pax.pr_brd is not null and ";
        else
            SQLText += "    pax.pr_brd = 1 and ";
        SQLText +=
            "     pax.pax_id=crs_inf.inf_id(+) ";
        Qry.SQLText = SQLText;
        Qry.CreateVariable("point_id", otInteger, info.point_id);
        Qry.Execute();
        if(!Qry.Eof) {
            int col_pax_id = Qry.FieldIndex("pax_id");
            int col_grp_id = Qry.FieldIndex("grp_id");
            int col_bag_pool_num = Qry.FieldIndex("bag_pool_num");
            int col_reg_no = Qry.FieldIndex("reg_no");
            int col_surname = Qry.FieldIndex("surname");
            int col_name = Qry.FieldIndex("name");
            int col_crs_pax_id = Qry.FieldIndex("crs_pax_id");
            int col_ticket_no = Qry.FieldIndex("ticket_no");
            int col_coupon_no = Qry.FieldIndex("coupon_no");
            int col_ticket_rem = Qry.FieldIndex("ticket_rem");
            for(; !Qry.Eof; Qry.Next()) {
                TInfantsItem item;
                item.pax_id = Qry.FieldAsInteger(col_pax_id);
                item.grp_id = Qry.FieldAsInteger(col_grp_id);
                if(not Qry.FieldIsNULL(col_bag_pool_num))
                    item.bag_pool_num = Qry.FieldAsInteger(col_bag_pool_num);
                item.W.get(item.grp_id, item.bag_pool_num);
                item.reg_no = Qry.FieldAsInteger(col_reg_no);
                item.surname = Qry.FieldAsString(col_surname);
                item.name = Qry.FieldAsString(col_name);
                item.ticket_no = Qry.FieldAsString(col_ticket_no);
                item.coupon_no = Qry.FieldAsInteger(col_coupon_no);
                item.ticket_rem = Qry.FieldAsString(col_ticket_rem);
                if(!Qry.FieldIsNULL(col_crs_pax_id)) {
                    item.parent_pax_id = Qry.FieldAsInteger(col_crs_pax_id);
                }
                items.push_back(item);
            }
        }
        if(!items.empty()) {
            Qry.Clear();
            string SQLText =
                "SELECT pax.grp_id, "
                "       pax.pax_id, "
                "       pax.reg_no, "
                "       pax.surname "
                "FROM pax_grp,pax "
                "WHERE "
                "     pax_grp.grp_id=pax.grp_id AND "
                "     pax_grp.point_dep=:point_id AND "
                "     pax_grp.status NOT IN ('E') AND "
                "     pax.pers_type='??' AND ";
            if((PRLOptions and PRLOptions->pax_state == "CKIN") or info.get_tlg_type() == "LCI")
                SQLText += " pax.pr_brd is not null ";
            else
                SQLText += "    pax.pr_brd = 1 ";
            Qry.SQLText = SQLText;
            Qry.CreateVariable("point_id", otInteger, info.point_id);
            Qry.Execute();
            vector<TAdultsItem> adults;
            if(!Qry.Eof) {
                int col_grp_id = Qry.FieldIndex("grp_id");
                int col_pax_id = Qry.FieldIndex("pax_id");
                int col_reg_no = Qry.FieldIndex("reg_no");
                int col_surname = Qry.FieldIndex("surname");
                for(; !Qry.Eof; Qry.Next()) {
                    TAdultsItem item;
                    item.grp_id = Qry.FieldAsInteger(col_grp_id);
                    item.pax_id = Qry.FieldAsInteger(col_pax_id);
                    item.reg_no = Qry.FieldAsInteger(col_reg_no);
                    item.surname = Qry.FieldAsString(col_surname);
                    adults.push_back(item);
                }
            }
            SetInfantsToAdults( items, adults );
            /*
            for(int k = 1; k <= 3; k++) {
                for(vector<TInfantsItem>::iterator infRow = items.begin(); infRow != items.end(); infRow++) {
                    if(k == 1 and infRow->pax_id != NoExists or
                            k > 1 and infRow->pax_id == NoExists) {
                        infRow->pax_id = NoExists;
                        for(vector<TAdultsItem>::iterator adultRow = adults.begin(); adultRow != adults.end(); adultRow++) {
                            if(
                                    (infRow->grp_id == adultRow->grp_id) and
                                    (k == 1 and infRow->crs_pax_id == adultRow->pax_id or
                                     k == 2 and infRow->surname == adultRow->surname or
                                     k == 3)
                              ) {
                                infRow->pax_id = adultRow->pax_id;
                                adults.erase(adultRow);
                                break;
                            }
                        }
                    }
                }
            }
            */
        }
    }

    struct TPRLPax;
    struct TRemList {
        private:
            void internal_get(TypeB::TDetailCreateInfo &info, int pax_id, string subcls);
            string chkd(int reg_no, const string &name, const string &surname, bool pr_inf, bool pr_lat);
        public:
            TInfants *infants;
            vector<string> items;
            void get(TypeB::TDetailCreateInfo &info, TFTLPax &pax);
            void get(TypeB::TDetailCreateInfo &info, TETLPax &pax);
            void get(TypeB::TDetailCreateInfo &info, TTPLPax &pax);
            void get(TypeB::TDetailCreateInfo &info, TASLPax &pax);
            void get(TypeB::TDetailCreateInfo &info, TPRLPax &pax, vector<TTlgCompLayer> &complayers);
            void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
            TRemList(TInfants *ainfants): infants(ainfants) {};
            void operator = (const TRemList &rems);
    };

    void TRemList::operator = (const TRemList &rems)
    {
        infants = rems.infants;
        items = rems.items;
    }

    struct TTagItem {
        string tag_type;
        int no_len;
        double no;
        string color;
        string airp_arv;
        TTagItem() {
            no_len = NoExists;
            no = NoExists;
        }
    };

    struct TTagList {
        private:
            virtual void format_tag_no(ostringstream &line, const TTagItem &prev_item, const int num, TypeB::TDetailCreateInfo &info)=0;
        public:
            vector<TTagItem> items;
            void get(int grp_id, int bag_pool_num);
            void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
            virtual ~TTagList(){};
    };

    struct TPRLTagList:TTagList {
        private:
            void format_tag_no(ostringstream &line, const TTagItem &prev_item, const int num, TypeB::TDetailCreateInfo &info)
            {
                line
                    << ".N/" << fixed << setprecision(0) << setw(10) << setfill('0') << (prev_item.no - num + 1)
                    << setw(3) << setfill('0') << num
                    << '/' << info.TlgElemIdToElem(etAirp, prev_item.airp_arv);
            }
    };

    struct TBTMTagList:TTagList {
        private:
            void format_tag_no(ostringstream &line, const TTagItem &prev_item, const int num, TypeB::TDetailCreateInfo &info)
            {
                line
                    << ".N/" << fixed << setprecision(0) << setw(10) << setfill('0') << (prev_item.no - num + 1)
                    << setw(3) << setfill('0') << num;
            }
    };

    void TTagList::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        if(items.empty())
            return;
        int num = 0;
        vector<TTagItem>::iterator prev_item;
        vector<TTagItem>::iterator iv = items.begin();
        while(true) {
            if(
                    iv == items.end() or
                    (iv != items.begin() and
                    not(prev_item->tag_type == iv->tag_type and
                        prev_item->color == iv->color and
                        prev_item->no + 1 == iv->no and
                        num < 999))
              ) {
                ostringstream line;
                format_tag_no(line, *prev_item, num, info);
                body.push_back(line.str());
                if(iv == items.end())
                    break;
                num = 1;
            } else
              num++;
            prev_item = iv++;
        }
    }

    void TTagList::get(int grp_id, int bag_pool_num)
    {
        string SQLText =
            "select "
            "    bag_tags.tag_type,  "
            "    tag_types.no_len,  "
            "    bag_tags.no,  "
            "    bag_tags.color,  "
            "    nvl(transfer.airp_arv, pax_grp.airp_arv) airp_arv  "
            "from "
            "    bag_tags, "
            "    bag2, "
            "    tag_types, "
            "    transfer, "
            "    pax_grp "
            "where "
            "    bag_tags.grp_id = :grp_id and "
            "    bag_tags.grp_id = bag2.grp_id(+) and "
            "    bag_tags.bag_num = bag2.num(+) and ";
        if(bag_pool_num == 1) // ???ਢ易???? ??ન ?ਮ?頥? ? bag_pool_num = 1
            SQLText +=
                "    (bag2.bag_pool_num = :bag_pool_num or bag_tags.bag_num is null) and ";
        else
            SQLText +=
                "    bag2.bag_pool_num = :bag_pool_num and ";
        SQLText +=
            "    bag_tags.tag_type = tag_types.code and "
            "    bag_tags.grp_id = transfer.grp_id(+) and transfer.pr_final(+)<>0 and "
            "    bag_tags.grp_id = pax_grp.grp_id ";
        QParams QryParams;
        QryParams
            << QParam("grp_id", otInteger, grp_id)
            << QParam("bag_pool_num", otInteger, bag_pool_num);
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(!Qry.get().Eof) {
            int col_tag_type = Qry.get().FieldIndex("tag_type");
            int col_no_len = Qry.get().FieldIndex("no_len");
            int col_no = Qry.get().FieldIndex("no");
            int col_color = Qry.get().FieldIndex("color");
            int col_airp_arv = Qry.get().FieldIndex("airp_arv");
            for(; !Qry.get().Eof; Qry.get().Next()) {
                TTagItem item;
                item.tag_type = Qry.get().FieldAsString(col_tag_type);
                item.no_len = Qry.get().FieldAsInteger(col_no_len);
                item.no = Qry.get().FieldAsFloat(col_no);
                item.color = Qry.get().FieldAsString(col_color);
                item.airp_arv = Qry.get().FieldAsString(col_airp_arv);
                items.push_back(item);
            }
        }
    }

    struct TOnwardItem {
        string airline;
        int flt_no;
        string suffix;
        TDateTime scd;
        string airp_arv;
        string trfer_subcls;
        string trfer_cls;
        TOnwardItem() {
            flt_no = NoExists;
            scd = NoExists;
        }
    };

    struct TOnwardList {
        private:
            virtual string format(const TOnwardItem &item, const int i, TypeB::TDetailCreateInfo &info)=0;
        public:
            vector<TOnwardItem> items;
            void get(int grp_id, int pax_id);
            void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
            virtual ~TOnwardList(){};
    };

    struct TBTMOnwardList:TOnwardList {
        private:
            string format(const TOnwardItem &item, const int i, TypeB::TDetailCreateInfo &info)
            {
                if(i == 1)
                    return "";
                ostringstream line;
                line
                    << ".O/"
                    << info.TlgElemIdToElem(etAirline, item.airline)
                    << setw(3) << setfill('0') << item.flt_no
                    << (item.suffix.empty() ? "" : info.TlgElemIdToElem(etSuffix, item.suffix))
                    << '/'
                    << DateTimeToStr(item.scd, "ddmmm", info.is_lat())
                    << '/'
                    << info.TlgElemIdToElem(etAirp, item.airp_arv);
                if(not item.trfer_cls.empty())
                    line
                        << '/'
                        << info.TlgElemIdToElem(etClass, item.trfer_cls);
                return line.str();
            }
    };

    struct TPSMOnwardList:TOnwardList {
        private:
            string format(const TOnwardItem &item, const int i, TypeB::TDetailCreateInfo &info)
            {
                ostringstream line;
                line
                    << " "
                    << info.TlgElemIdToElem(etAirline, item.airline)
                    << setw(3) << setfill('0') << item.flt_no
                    << (item.suffix.empty() ? "" : info.TlgElemIdToElem(etSuffix, item.suffix))
                    << info.TlgElemIdToElem(etSubcls, item.trfer_subcls)
                    << DateTimeToStr(item.scd, "dd", info.is_lat())
                    << info.TlgElemIdToElem(etAirp, item.airp_arv);
                return line.str();
            }
        public:
            void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
            {
                if(not items.empty())
                    body.push_back(format(items[0], 0, info));
            }
    };

    struct TPRLOnwardList:TOnwardList {
        private:
            string format(const TOnwardItem &item, const int i, TypeB::TDetailCreateInfo &info)
            {
                ostringstream line;
                line << ".O";
                if(i > 1)
                    line << i;
                line
                    << '/'
                    << info.TlgElemIdToElem(etAirline, item.airline)
                    << setw(3) << setfill('0') << item.flt_no
                    << (item.suffix.empty() ? "" : info.TlgElemIdToElem(etSuffix, item.suffix))
                    << info.TlgElemIdToElem(etSubcls, item.trfer_subcls)
                    << DateTimeToStr(item.scd, "dd", info.is_lat())
                    << info.TlgElemIdToElem(etAirp, item.airp_arv);
                return line.str();
            }
    };

    void TOnwardList::get(int grp_id, int pax_id)
    {
        string SQLText =
            "SELECT \n"
            "    trfer_trips.airline, \n"
            "    trfer_trips.flt_no, \n"
            "    trfer_trips.suffix, \n"
            "    trfer_trips.scd, \n"
            "    transfer.airp_arv, \n";
        if(pax_id == NoExists)
            SQLText +=
                "    null trfer_subclass, \n"
                "    null trfer_class \n";
        else
            SQLText +=
                "    transfer_subcls.subclass trfer_subclass, \n"
                "    subcls.class trfer_class \n";
        SQLText +=
            "FROM \n"
            "    transfer, \n"
            "    trfer_trips \n";
        if(pax_id != NoExists)
            SQLText +=
                "    , pax, \n"
                "    transfer_subcls, \n"
                "    subcls \n";
        SQLText +=
            "WHERE  \n";
        if(pax_id == NoExists)
            SQLText +=
                "       transfer.grp_id = :grp_id and ";
        else
            SQLText +=
                "    pax.pax_id = :pax_id and \n"
                "    pax.grp_id = transfer.grp_id and \n"
                "    transfer_subcls.pax_id = pax.pax_id and \n"
                "    transfer_subcls.transfer_num = transfer.transfer_num and \n"
                "    transfer_subcls.subclass = subcls.code and \n";
        SQLText +=
            "    transfer.transfer_num>=1 and \n"
            "    transfer.point_id_trfer=trfer_trips.point_id \n"
            "ORDER BY \n"
            "    transfer.transfer_num \n";
        QParams QryParams;
        if(pax_id == NoExists)
            QryParams << QParam("grp_id", otInteger, grp_id);
        else
            QryParams << QParam("pax_id", otInteger, pax_id);
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(!Qry.get().Eof) {
            int col_airline = Qry.get().FieldIndex("airline");
            int col_flt_no = Qry.get().FieldIndex("flt_no");
            int col_suffix = Qry.get().FieldIndex("suffix");
            int col_scd = Qry.get().FieldIndex("scd");
            int col_airp_arv = Qry.get().FieldIndex("airp_arv");
            int col_trfer_subcls = Qry.get().FieldIndex("trfer_subclass");
            int col_trfer_cls = Qry.get().FieldIndex("trfer_class");
            for(; !Qry.get().Eof; Qry.get().Next()) {
                TOnwardItem item;
                item.airline = Qry.get().FieldAsString(col_airline);
                item.flt_no = Qry.get().FieldAsInteger(col_flt_no);
                item.suffix = Qry.get().FieldAsString(col_suffix);
                item.scd = Qry.get().FieldAsDateTime(col_scd);
                item.airp_arv = Qry.get().FieldAsString(col_airp_arv);
                item.trfer_subcls = Qry.get().FieldAsString(col_trfer_subcls);
                item.trfer_cls = Qry.get().FieldAsString(col_trfer_cls);
                items.push_back(item);
            }
        }
    }

    void TOnwardList::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        int i = 1;
        for(vector<TOnwardItem>::iterator iv = items.begin(); iv != items.end(); iv++, i++) {
            string line = format(*iv, i, info);
            if(not line.empty())
                body.push_back(line);
        }
    }

    struct TGRPItem {
        int pax_count;
        bool written;
        int bg;
        TWItem W;
        TPRLTagList tags;
        TGRPItem() {
            pax_count = NoExists;
            written = false;
            bg = NoExists;
        }
    };

    struct TGRPMap {
        TInfants &infants;
        map<int, map<int, TGRPItem> > items; // [grp_id][bag_pool_num]
        int items_count;
        bool find(int grp_id, int bag_pool_num);
        void get(int grp_id, int bag_pool_num);
        void ToTlg(TypeB::TDetailCreateInfo &info, int grp_id, int bag_pool_num, vector<string> &body);
        TGRPMap(TInfants &ainfants): infants(ainfants), items_count(0) {};
    };

    struct TFirmSpaceAvail {
        string status, priority;
        void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
    };

    void TFirmSpaceAvail::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        if(
                status == "DG1" or
                status == "RG1" or
                status == "ID1" or
                status == "DG2" or
                status == "RG2" or
                status == "ID2"
          )
            body.push_back("." + status + "/" + priority);
    }

    struct TPRLPax {
        string target;
        int cls_grp_id;
        TName name;
        int pnr_id;
        string crs;
        int pax_id;
        int grp_id;
        int bag_pool_num;
        TPaxStatus grp_status;
        string subcls;
        TPNRList pnrs;
        TMItem M;
        TFirmSpaceAvail firm_space_avail;
        TRemList rems;
        TPRLOnwardList OList;
        TTrickyGender::Enum gender;
        TPerson pers_type;
        int reg_no;
        ASTRA::TCrewType::Enum crew_type;
        TPRLPax(TInfants *ainfants): rems(ainfants) {
            cls_grp_id = NoExists;
            pnr_id = NoExists;
            pax_id = NoExists;
            grp_id = NoExists;
            bag_pool_num = NoExists;
            gender = TTrickyGender::Male;
            reg_no = NoExists;
            crew_type = TCrewType::Unknown;
        }
    };

    void TGRPMap::ToTlg(TypeB::TDetailCreateInfo &info, int grp_id, int bag_pool_num, vector<string> &body)
    {
        TGRPItem &grp_map = items[grp_id][bag_pool_num];
        if(not(grp_map.W.bagAmount == 0 and grp_map.W.bagWeight == 0 and grp_map.W.rkWeight == 0)) {
            ostringstream line;
            if(grp_map.pax_count > 1) {
                line.str("");
                line << ".BG/" << setw(3) << setfill('0') << grp_map.bg;
                body.push_back(line.str());
            }
            if(!grp_map.written) {
                grp_map.written = true;
                grp_map.W.ToTlg(body);
                grp_map.tags.ToTlg(info, body);
            }
        }
    }

    bool TGRPMap::find(int grp_id, int bag_pool_num)
    {
        map<int, map<int, TGRPItem> >::iterator ix = items.find(grp_id);
        bool result = ix != items.end();
        if(result) {
            map<int, TGRPItem>::iterator iy = ix->second.find(bag_pool_num);
            result = iy != ix->second.end();
        }

        return result;
    }

    void TGRPMap::get(int grp_id, int bag_pool_num)
    {
        if(find(grp_id, bag_pool_num)) return; // olready got
        TGRPItem item;
        item.W.get(grp_id, bag_pool_num);
        QParams QryParams;
        QryParams
            << QParam("grp_id", otInteger, grp_id)
            << QParam("bag_pool_num", otInteger, bag_pool_num);
        TCachedQuery Qry(
                "select count(*) from pax where grp_id = :grp_id and bag_pool_num = :bag_pool_num and refuse is null",
                QryParams);
        Qry.get().Execute();
        item.pax_count = Qry.get().FieldAsInteger(0);
        item.tags.get(grp_id, bag_pool_num);
        item.bg = ++items_count;
        items[grp_id][bag_pool_num] = item;
    }

    void TRemList::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        for(vector<string>::iterator iv = items.begin(); iv != items.end(); iv++) {
            string rem = *iv;
            rem = ".R/" + rem;
            while(rem.size() > LINE_SIZE) {
                body.push_back(rem.substr(0, LINE_SIZE));
                rem = ".RN/" + rem.substr(LINE_SIZE);
            }
            if(!rem.empty())
                body.push_back(rem);
        }
    }

    string TRemList::chkd(int reg_no, const string &name, const string &surname, bool pr_inf, bool pr_lat)
    {
        ostringstream result;
        result
            << "CHKD HK1 "
            << setw(4) << setfill('0') << reg_no
            << (pr_inf ? " I" : "") << "-1"
            << transliter(surname, 1, pr_lat)
            << (name.empty() ? "" : "/" + transliter(name, 1, pr_lat));
        return result.str();
    }
    void TRemList::get(TypeB::TDetailCreateInfo &info, TPRLPax &pax, vector<TTlgCompLayer> &complayers )
    {
        const TypeB::TPRLOptions *PRLOptions=NULL;
        if(info.optionsIs<TypeB::TPRLOptions>())
            PRLOptions=info.optionsAs<TypeB::TPRLOptions>();

        items.clear();
        if(pax.pax_id == NoExists) return;
        // rems must be push_backed exactly in this order. Don't swap!
        for(vector<TInfantsItem>::iterator infRow = infants->items.begin(); infRow != infants->items.end(); infRow++) {
            if(infRow->grp_id == pax.grp_id and infRow->parent_pax_id == pax.pax_id) {
                string rem;
                if(PRLOptions and PRLOptions->version == "33") {
                    rem = "INFT HK1 ";
                    CheckIn::TPaxDocItem doc;
                    if(LoadPaxDoc(infRow->pax_id, doc))
                        rem += DateTimeToStr(doc.birth_date, "ddmmmyy", info.is_lat()) + " ";
                } else
                    rem = "1INF ";
                rem += transliter(infRow->surname, 1, info.is_lat());
                if(!infRow->name.empty()) {
                    rem += "/" + transliter(infRow->name, 1, info.is_lat());
                }
                items.push_back(rem);
                items.push_back(chkd(infRow->reg_no, infRow->name, infRow->surname, true, info.is_lat()));

                {
                    // ????? ??
                    CheckIn::TPaxRemItem rem;
                    CheckIn::TPaxTknItem tkn;
                    LoadPaxTkn(infRow->pax_id, tkn);
                    if (getPaxRem(info, tkn, true, rem)) items.push_back(rem.text);
                }
            }
        }
        if(pax.pers_type == child or pax.pers_type == baby)
            items.push_back("1CHD");
        TTlgSeatList seats;
        seats.add_seats(pax.pax_id, complayers);
        string seat_list = seats.get_seat_list(info.is_lat() or info.pr_lat_seat);
        if(!seat_list.empty())
            items.push_back("SEAT HK" + IntToString(seats.get_seat_vector(false).size()) + " " + seat_list);
        items.push_back(chkd(pax.reg_no, pax.name.name, pax.name.surname, pax.pers_type == baby, info.is_lat()));
        internal_get(info, pax.pax_id, pax.subcls);

        QParams QryParams;
        QryParams << QParam("pax_id", otInteger, pax.pax_id);
        TCachedQuery Qry(
                "select "
                "    rem_code, rem "
                "from "
                "    pax_rem "
                "where "
                "    pax_rem.pax_id = :pax_id and "
                "    pax_rem.rem_code not in ('OTHS', 'CHD', 'CHLD', 'INF', 'INFT') ",
                QryParams);
        Qry.get().Execute();
        for(; !Qry.get().Eof; Qry.get().Next())
        {
          CheckIn::TPaxRemItem rem;
          rem.fromDB(Qry.get());
          TRemCategory cat=getRemCategory(rem);
          if (isDisabledRemCategory(cat)) continue;
          items.push_back(transliter(rem.text, 1, info.is_lat()));
        };

        bool inf_indicator=false; //? ???????? ⮫쪮 ? ?? infant ? ६?ન ?뢮??? ⮫쪮 ??? ???? ??
                                  //??? infant ???? ?? ?뢮???, ? ????? ???? ?????
        CheckIn::TPaxRemItem rem;
        //?????
        CheckIn::TPaxTknItem tkn;
        LoadPaxTkn(pax.pax_id, tkn);
        if (getPaxRem(info, tkn, inf_indicator, rem)) items.push_back(rem.text);
        //???㬥??
        CheckIn::TPaxDocItem doc;
        LoadPaxDoc(pax.pax_id, doc);
        if (getPaxRem(info, doc, inf_indicator, rem)) items.push_back(rem.text);
        //????
        CheckIn::TPaxDocoItem doco;
        LoadPaxDoco(pax.pax_id, doco);
        if (getPaxRem(info, doco, inf_indicator, rem)) items.push_back(rem.text);
        //??????
        CheckIn::TDocaMap doca_map;
        LoadPaxDoca(pax.pax_id, doca_map);
        for(CheckIn::TDocaMap::const_iterator d = doca_map.begin(); d != doca_map.end(); ++d)
        {
          if (d->second.type!="D" && d->second.type!="R") continue;
          if (getPaxRem(info, d->second, inf_indicator, rem)) items.push_back(rem.text);
        };
    }

    struct TPRLDest {
        string airp;
        string cls; // if rbd is on, this field contains fare class, otherwise - compartment class
        vector<TPRLPax> PaxList;
        TGRPMap *grp_map;
        TInfants *infants;
        TPRLDest(TGRPMap *agrp_map, TInfants *ainfants) {
            grp_map = agrp_map;
            infants = ainfants;
        }
        void GetPaxList(TypeB::TDetailCreateInfo &info, vector<TTlgCompLayer> &complayers);
        void PaxListToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
    };

    void TPRLDest::PaxListToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        for(vector<TPRLPax>::iterator iv = PaxList.begin(); iv != PaxList.end(); iv++) {
            iv->name.ToTlg(info, body);
            iv->pnrs.ToTlg(info, body);
            iv->M.ToTlg(info, body);
            iv->firm_space_avail.ToTlg(info, body);
            grp_map->ToTlg(info, iv->grp_id, iv->bag_pool_num, body);
            iv->OList.ToTlg(info, body);
            iv->rems.ToTlg(info, body);
        }
    }

    void TPRLDest::GetPaxList(TypeB::TDetailCreateInfo &info, vector<TTlgCompLayer> &complayers)
    {
        const TypeB::TMarkInfoOptions *markOptions=NULL;
        if(info.optionsIs<TypeB::TMarkInfoOptions>())
            markOptions=info.optionsAs<TypeB::TMarkInfoOptions>();
        const TypeB::TPRLOptions *PRLOptions=NULL;
        if(info.optionsIs<TypeB::TPRLOptions>())
            PRLOptions=info.optionsAs<TypeB::TPRLOptions>();


        string SQLText =
            "select "
            "    pax_grp.airp_arv target, ";
        if(PRLOptions and PRLOptions->rbd)
            SQLText += "    NULL cls, ";
        else
            SQLText += "    cls_grp.id cls, ";
        SQLText +=
            "    system.transliter(pax.surname, 1, :pr_lat) surname, "
            "    system.transliter(pax.name, 1, :pr_lat) name, "
            "    crs_pnr.pnr_id, "
            "    crs_pnr.sender crs, "
            "    crs_pnr.status, "
            "    crs_pnr.priority, "
            "    pax.pax_id, "
            "    pax.grp_id, "
            "    pax.bag_pool_num, "
            "    nvl(nvl(pax.cabin_subclass, pax.subclass), nvl(pax.cabin_class, pax_grp.class)) subclass, "
            "    pax_grp.status grp_status, "
            "    pax.is_female, "
            "    pax.pers_type, "
            "    pax.reg_no, "
            "    pax.crew_type "
            "from "
            "    pax, "
            "    pax_grp, ";
        if(not(PRLOptions and PRLOptions->rbd))
            SQLText += "    cls_grp, ";
        SQLText +=
            "    crs_pax, "
            "    crs_pnr "
            "WHERE "
            "    pax_grp.point_dep = :point_id and "
            "    pax_grp.status NOT IN ('E') AND "
            "    pax_grp.airp_arv = :airp and "
            "    pax_grp.grp_id=pax.grp_id AND ";
        if(PRLOptions and PRLOptions->rbd) {
            SQLText +=
                "   nvl(nvl(pax.cabin_subclass, pax.subclass), nvl(pax.cabin_class, pax_grp.class)) = :class and ";
        } else {
            SQLText +=
                "    nvl(pax.cabin_class_grp, pax_grp.class_grp) = cls_grp.id(+) AND "
                "    cls_grp.code = :class and ";
        }
        if((PRLOptions and PRLOptions->pax_state == "CKIN") or info.get_tlg_type() == "LCI")
            SQLText += " pax.pr_brd is not null and ";
        else
            SQLText += "    pax.pr_brd = 1 and ";
        SQLText +=
            "    pax.seats>0 and "
            "    pax.pax_id = crs_pax.pax_id(+) and "
            "    crs_pax.pr_del(+)=0 and "
            "    crs_pax.pnr_id = crs_pnr.pnr_id(+) and "
            "    crs_pnr.system(+) = 'CRS' "
            "order by "
            "    target, "
            "    cls, "
            "    surname, "
            "    name nulls first, "
            "    pax.pax_id ";
        QParams QryParams;
        QryParams
            << QParam("point_id", otInteger, info.point_id)
            << QParam("airp", otString, airp)
            << QParam("class", otString, cls)
            << QParam("pr_lat", otInteger, info.is_lat());
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(!Qry.get().Eof) {
            int col_target = Qry.get().FieldIndex("target");
            int col_cls = Qry.get().FieldIndex("cls");
            int col_surname = Qry.get().FieldIndex("surname");
            int col_name = Qry.get().FieldIndex("name");
            int col_pnr_id = Qry.get().FieldIndex("pnr_id");
            int col_crs = Qry.get().FieldIndex("crs");
            int col_status = Qry.get().FieldIndex("status");
            int col_priority = Qry.get().FieldIndex("priority");
            int col_pax_id = Qry.get().FieldIndex("pax_id");
            int col_grp_id = Qry.get().FieldIndex("grp_id");
            int col_bag_pool_num = Qry.get().FieldIndex("bag_pool_num");
            int col_subcls = Qry.get().FieldIndex("subclass");
            int col_grp_status = Qry.get().FieldIndex("grp_status");
            int col_pers_type = Qry.get().FieldIndex("pers_type");
            int col_reg_no = Qry.get().FieldIndex("reg_no");
            int col_crew_type = Qry.get().FieldIndex("crew_type");
            for(; !Qry.get().Eof; Qry.get().Next()) {
                TPRLPax pax(infants);
                pax.target = Qry.get().FieldAsString(col_target);
                if(!Qry.get().FieldIsNULL(col_cls))
                    pax.cls_grp_id = Qry.get().FieldAsInteger(col_cls);
                pax.name.surname = Qry.get().FieldAsString(col_surname);
                pax.name.name = Qry.get().FieldAsString(col_name);
                if(!Qry.get().FieldIsNULL(col_pnr_id))
                    pax.pnr_id = Qry.get().FieldAsInteger(col_pnr_id);
                pax.crs = Qry.get().FieldAsString(col_crs);
                pax.firm_space_avail.status = Qry.get().FieldAsString(col_status);
                pax.firm_space_avail.priority = Qry.get().FieldAsString(col_priority);
                if(markOptions and not markOptions->crs.empty() and markOptions->crs != pax.crs)
                    continue;
                pax.pax_id = Qry.get().FieldAsInteger(col_pax_id);
                pax.grp_id = Qry.get().FieldAsInteger(col_grp_id);
                pax.grp_status = DecodePaxStatus(Qry.get().FieldAsString(col_grp_status));
                if(not Qry.get().FieldIsNULL(col_bag_pool_num))
                    pax.bag_pool_num = Qry.get().FieldAsInteger(col_bag_pool_num);
                pax.M.get(info, pax.pax_id);
                if(markOptions and not markOptions->mark_info.empty() and not(pax.M.m_flight == markOptions->mark_info))
                    continue;
                pax.pnrs.get(pax.pnr_id);
                if(!Qry.get().FieldIsNULL(col_subcls))
                    pax.subcls = Qry.get().FieldAsString(col_subcls);
                grp_map->get(pax.grp_id, pax.bag_pool_num);
                pax.OList.get(pax.grp_id, pax.pax_id);
                pax.pers_type = DecodePerson(Qry.get().FieldAsString(col_pers_type));
                pax.reg_no = Qry.get().FieldAsInteger(col_reg_no);
                pax.crew_type = TCrewTypes().decode(Qry.get().FieldAsString(col_crew_type));
                pax.rems.get(info, pax, complayers); // ??易⥫쭮 ??᫥ ???樠????樨 pers_type ????
                pax.gender = getGender(Qry.get());

                PaxList.push_back(pax);
            }
        }
    }

    struct TCOMStatsItem {
        string target;
        int f;
        int c;
        int y;
        int adult;
        int male;
        int female;
        int child;
        int baby;
        int f_pad;
        int c_pad;
        int y_pad;
        int f_child;
        int f_baby;
        int c_child;
        int c_baby;
        int y_child;
        int y_baby;
        int bag_amount;
        int bag_weight;
        int rk_weight;
        int f_bag_weight;
        int f_rk_weight;
        int c_bag_weight;
        int c_rk_weight;
        int y_bag_weight;
        int y_rk_weight;
        int f_add_pax;
        int c_add_pax;
        int y_add_pax;
        int male_extra_crew;
        int female_extra_crew;
        int male_dead_head_crew;
        int female_dead_head_crew;
        TCOMStatsItem()
        {
            f = 0;
            c = 0;
            y = 0;
            adult = 0;
            male = 0;
            female = 0;
            child = 0;
            baby = 0;
            f_pad = 0;
            c_pad = 0;
            y_pad = 0;
            f_child = 0;
            f_baby = 0;
            c_child = 0;
            c_baby = 0;
            y_child = 0;
            y_baby = 0;
            bag_amount = 0;
            bag_weight = 0;
            rk_weight = 0;
            f_bag_weight = 0;
            f_rk_weight = 0;
            c_bag_weight = 0;
            c_rk_weight = 0;
            y_bag_weight = 0;
            y_rk_weight = 0;
            f_add_pax = 0;
            c_add_pax = 0;
            y_add_pax = 0;
            male_extra_crew = 0;
            female_extra_crew = 0;
            male_dead_head_crew = 0;
            female_dead_head_crew = 0;
        }
    };

    struct TTotalPaxWeight {
        int weight;
        void get(TypeB::TDetailCreateInfo &info);
        TTotalPaxWeight() {
            weight = 0;
        }
    };

    void TTotalPaxWeight::get(TypeB::TDetailCreateInfo &info)
    {
        TFlightWeights w;
        w.read( info.point_id, onlyCheckin, false );
        weight = w.weight_male +
                 w.weight_female +
                 w.weight_child +
                 w.weight_infant;
    }

    struct TCOMZones {
        map<string, int> items;
        void get(TypeB::TDetailCreateInfo &info);
        void ToTlg(ostringstream &body);
    };

    void TCOMZones::get(TypeB::TDetailCreateInfo &info)
    {
        ZoneLoads(info.point_id, items, true);
    }

    void TCOMZones::ToTlg(ostringstream &body)
    {
        if(not items.empty()) {
            body << "ZONES -";
            for(map<string, int>::iterator i = items.begin(); i != items.end(); i++)
                body << " " << i->first << "/" << i->second;
            body << TypeB::endl;
        }
    }

    struct TCOMStats {
        vector<TCOMStatsItem> items;
        TTotalPaxWeight total_pax_weight;
        void get(const REPORTS::TPaxList &pax_list, TypeB::TDetailCreateInfo &info);
        void ToTlg(TypeB::TDetailCreateInfo &info, ostringstream &body);
    };

    void TCOMStats::ToTlg(TypeB::TDetailCreateInfo &info, ostringstream &body)
    {
        const TypeB::TCOMOptions &options = *info.optionsAs<TypeB::TCOMOptions>();

        TCOMStatsItem sum;
        sum.target = "TTL";
        for(vector<TCOMStatsItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
            if(options.version == "DME")
                body
                    << iv->target       << ' '
                    << iv->adult        << '/'
                    << iv->child        << '/'
                    << iv->baby         << ' '
                    << iv->bag_amount   << '/'
                    << iv->bag_weight   << '/'
                    << iv->rk_weight    << ' '
                    << iv->f            << '/'
                    << iv->c            << '/'
                    << iv->y            << ' '
                    << "0/0/0 0 0 "
                    << iv->f_add_pax    << '/'
                    << iv->c_add_pax    << '/'
                    << iv->y_add_pax    << ' '
                    << iv->f_child      << '/'
                    << iv->c_child      << '/'
                    << iv->y_child      << ' '
                    << iv->f_baby       << '/'
                    << iv->c_baby       << '/'
                    << iv->y_baby       << ' '
                    << iv->f_rk_weight  << '/'
                    << iv->c_rk_weight  << '/'
                    << iv->y_rk_weight  << ' '
                    << iv->f_bag_weight << '/'
                    << iv->c_bag_weight << '/'
                    << iv->y_bag_weight << TypeB::endl;
            else if(options.version == "PAD")
                body
                    << iv->target                   << ' '
                    << iv->male                     << '/'
                    << iv->female                   << '/'
                    << iv->child                    << '/'
                    << iv->baby                     << ' '
                    << iv->bag_amount               << '/'
                    << iv->bag_weight               << '/'
                    << iv->rk_weight                << ' '
                    << iv->f                        << '/'
                    << iv->c                        << '/'
                    << iv->y                        << ' '
                    << iv->f_pad                    << '/'
                    << iv->c_pad                    << '/'
                    << iv->y_pad                    << ' '
                    << iv->male_extra_crew          << '/'
                    << iv->female_extra_crew        << ' '
                    << iv->male_dead_head_crew      << '/'
                    << iv->female_dead_head_crew    << ' '
                    << iv->f_add_pax                << '/'
                    << iv->c_add_pax                << '/'
                    << iv->y_add_pax
                    << TypeB::endl;
            else
                body
                    << iv->target       << ' '
                    << iv->adult        << '/'
                    << iv->child        << '/'
                    << iv->baby         << ' '
                    << iv->bag_amount   << '/'
                    << iv->bag_weight   << '/'
                    << iv->rk_weight    << ' '
                    << iv->f            << '/'
                    << iv->c            << '/'
                    << iv->y            << ' '
                    << "0/0/0 0/0 0/0 0/0/0" << TypeB::endl;

            sum.adult += iv->adult;
            sum.child += iv->child;
            sum.baby += iv->baby;
            sum.bag_amount += iv->bag_amount;
            sum.bag_weight += iv->bag_weight;
            sum.rk_weight += iv->rk_weight;
            sum.f += iv->f;
            sum.c += iv->c;
            sum.y += iv->y;
            sum.f_add_pax += iv->f_add_pax;
            sum.c_add_pax += iv->c_add_pax;
            sum.y_add_pax += iv->y_add_pax;
            sum.f_child += iv->f_child;
            sum.c_child += iv->c_child;
            sum.y_child += iv->y_child;
            sum.f_baby += iv->f_baby;
            sum.c_baby += iv->c_baby;
            sum.y_baby += iv->y_baby;
            sum.f_rk_weight += iv->f_rk_weight;
            sum.c_rk_weight += iv->c_rk_weight;
            sum.y_rk_weight += iv->y_rk_weight;
            sum.f_bag_weight += iv->f_bag_weight;
            sum.c_bag_weight += iv->c_bag_weight;
            sum.y_bag_weight += iv->y_bag_weight;
            sum.male += iv->male;
            sum.female += iv->female;
            sum.f_pad += iv->f_pad;
            sum.c_pad += iv->c_pad;
            sum.y_pad += iv->y_pad;
            sum.male_extra_crew += iv->male_extra_crew;
            sum.female_extra_crew += iv->female_extra_crew;
            sum.male_dead_head_crew += iv->male_dead_head_crew;
            sum.female_dead_head_crew += iv->female_dead_head_crew;
        }
        if(info.get_tlg_type() == "COM") {
            if(options.version == "PAD")
                body
                    << sum.target                   << ' '
                    << sum.male                     << '/'
                    << sum.female                   << '/'
                    << sum.child                    << '/'
                    << sum.baby                     << ' '
                    << sum.bag_amount               << '/'
                    << sum.bag_weight               << '/'
                    << sum.rk_weight                << ' '
                    << sum.f                        << '/'
                    << sum.c                        << '/'
                    << sum.y                        << ' '
                    << sum.f_pad                    << '/'
                    << sum.c_pad                    << '/'
                    << sum.y_pad                    << ' '
                    << sum.male_extra_crew          << '/'
                    << sum.female_extra_crew        << ' '
                    << sum.male_dead_head_crew      << '/'
                    << sum.female_dead_head_crew    << ' '
                    << sum.f_add_pax                << '/'
                    << sum.c_add_pax                << '/'
                    << sum.y_add_pax                << ' '
                    << '0'                          << ' '
                    << total_pax_weight.weight
                    << TypeB::endl;
            else
                body
                    << sum.target       << ' '
                    << sum.adult        << '/'
                    << sum.child        << '/'
                    << sum.baby         << ' '
                    << sum.bag_amount   << '/'
                    << sum.bag_weight   << '/'
                    << sum.rk_weight    << ' '
                    << sum.f            << '/'
                    << sum.c            << '/'
                    << sum.y            << ' '
                    << "0/0/0 0 0 "
                    << sum.f_add_pax    << '/'
                    << sum.c_add_pax    << '/'
                    << sum.y_add_pax    << ' '
                    << "0 " << total_pax_weight.weight << ' '
                    << sum.f_child      << '/'
                    << sum.c_child      << '/'
                    << sum.y_child      << ' '
                    << sum.f_baby       << '/'
                    << sum.c_baby       << '/'
                    << sum.y_baby       << ' '
                    << sum.f_rk_weight  << '/'
                    << sum.c_rk_weight  << '/'
                    << sum.y_rk_weight  << ' '
                    << sum.f_bag_weight << '/'
                    << sum.c_bag_weight << '/'
                    << sum.y_bag_weight << TypeB::endl;
        } else
            body
                << sum.target       << ' '
                << sum.adult        << '/'
                << sum.child        << '/'
                << sum.baby         << ' '
                << sum.bag_amount   << '/'
                << sum.bag_weight   << '/'
                << sum.rk_weight    << ' '
                << sum.f            << '/'
                << sum.c            << '/'
                << sum.y            << ' '
                << "0/0/0 0/0 0/0 0/0/0 0 "
                << total_pax_weight.weight << TypeB::endl;
    }

    void TCOMStats::get(const REPORTS::TPaxList &pax_list, TypeB::TDetailCreateInfo &info)
    {
        const TypeB::TCOMOptions &options = *info.optionsAs<TypeB::TCOMOptions>();
        TTripRoute route;
        route.GetRouteAfter(NoExists, info.point_id, trtNotCurrent, trtNotCancelled);
        map<int, TCOMStatsItem> data;
        if(not pax_list.empty()) {
            for(const auto &pax: pax_list) {
                auto &item = data[pax->grp().point_arv];
                item.target = info.TlgElemIdToElem(etAirp, pax->grp().airp_arv);

                bool male = pax->simple.pers_type == TPerson::adult and pax->simple.gender != TGender::Female;
                bool female = pax->simple.pers_type == TPerson::adult and pax->simple.gender == TGender::Female;
                bool is_f = pax->cl() == "?";
                bool is_c = pax->cl() == "?";
                bool is_y = pax->cl() == "?";

                if(
                        not pax->simple.is_jmp and
                        (options.version != "PAD" or
                         pax->simple.crew_type != TCrewType::ExtraCrew)
                  ) {
                    item.adult += pax->simple.pers_type == TPerson::adult;
                    item.male += male;
                    item.female += female;
                    item.child += pax->simple.pers_type == TPerson::child;
                    item.baby += pax->simple.pers_type == TPerson::baby;

                    item.f_child += is_f and pax->simple.pers_type == TPerson::child;
                    item.f_baby += is_f and pax->simple.pers_type == TPerson::baby;
                    item.c_child += is_c and pax->simple.pers_type == TPerson::child;
                    item.c_baby += is_c and pax->simple.pers_type == TPerson::baby;
                    item.y_child += is_y and pax->simple.pers_type == TPerson::child;
                    item.y_baby += is_y and pax->simple.pers_type == TPerson::baby;

                    item.f += is_f;
                    item.c += is_c;
                    item.y += is_y;
                }

                item.bag_amount += pax->bag_amount();
                item.bag_weight += pax->bag_weight();
                item.rk_weight += pax->rk_weight();

                item.f_bag_weight += (is_f ? pax->bag_weight() : 0);
                item.f_rk_weight += (is_f ? pax->rk_weight() : 0);
                item.c_bag_weight += (is_c ? pax->bag_weight() : 0);
                item.c_rk_weight += (is_c ? pax->rk_weight() : 0);
                item.y_bag_weight += (is_y ? pax->bag_weight() : 0);
                item.y_rk_weight += (is_y ? pax->rk_weight() : 0);

                item.f_pad += is_f and pax->grp().status == psGoshow;
                item.c_pad += is_c and pax->grp().status == psGoshow;
                item.y_pad += is_y and pax->grp().status == psGoshow;

                item.male_extra_crew += male and pax->simple.crew_type == TCrewType::ExtraCrew;
                item.female_extra_crew += female and pax->simple.crew_type == TCrewType::ExtraCrew;

                item.male_dead_head_crew += male and pax->simple.crew_type == TCrewType::DeadHeadCrew;
                item.female_dead_head_crew += female and pax->simple.crew_type == TCrewType::DeadHeadCrew;

                item.f_add_pax += is_f and (pax->seats() > 1);
                item.c_add_pax += is_c and (pax->seats() > 1);
                item.y_add_pax += is_y and (pax->seats() > 1);
            }
            total_pax_weight.get(info);
        }
        for(const auto &point_arv: route) {
            auto i = data.find(point_arv.point_id);
            if(i != data.end())
                items.push_back(data[point_arv.point_id]);
            else {
                TCOMStatsItem item;
                item.target = info.TlgElemIdToElem(etAirp, point_arv.airp);
                items.push_back(item);
            }
        }
    }

    struct TCOMClassesItem {
        string cls;
        int cfg;
        int av;
        int pad;
        TCOMClassesItem() {
            cfg = NoExists;
            av = NoExists;
            pad = NoExists;
        }
    };

    struct TCOMClasses {
        vector<TCOMClassesItem> items;
        void getByStats(const REPORTS::TPaxList &pax_list, TypeB::TDetailCreateInfo &info);
        void get(TypeB::TDetailCreateInfo &info);
        void ToTlg(TypeB::TDetailCreateInfo &info, ostringstream &body);
    };

    void TCOMClasses::getByStats(const REPORTS::TPaxList &pax_list, TypeB::TDetailCreateInfo &info)
    {
        TCFG cfg(info.point_id);
        for(const auto &cfg_item: cfg)
        {
            TCOMClassesItem item;
            item.cls = info.TlgElemIdToElem(etSubcls, cfg_item.cls);
            item.cfg = cfg_item.cfg;
            item.av = item.cfg;
            item.pad = 0;
            for(const auto &pax: pax_list)
                if(cfg_item.cls == pax->cl()) {
                    item.av -= pax->seats();
                    item.pad += pax->grp().status == psGoshow;
                }
            items.push_back(item);
        }
    }

    void TCOMClasses::ToTlg(TypeB::TDetailCreateInfo &info, ostringstream &body)
    {
        ostringstream classes, av, padc;
        for(const auto &iv: items) {
            classes << iv.cls << iv.cfg;
            av << iv.cls << iv.av;
            padc << iv.cls << iv.pad;
        }
        body
            << "ARN/" << info.bort
            << " CNF/" << classes.str()
            << " CAP/" << classes.str()
            << " AV/" << av.str()
            << " PADC/" << padc.str()
            << TypeB::endl;
    }

    void TCOMClasses::get(TypeB::TDetailCreateInfo &info)
    {
        QParams QryParams;
        QryParams
            << QParam("point_id", otInteger, info.point_id)
            << QParam("class", otString)
            << QParam("gosho", otString, EncodePaxStatus(psGoshow));
        TCachedQuery Qry(
                "SELECT "
                "   SUM(pax.seats) seats, "
                "   sum(decode(pax_grp.status, :gosho, 1, 0)) pad "
                "FROM "
                "   pax_grp, "
                "   pax "
                "WHERE "
                "   :point_id = pax_grp.point_dep and "
                "   :class = nvl(pax.cabin_class, pax_grp.class) and "
                "   pax_grp.status NOT IN ('E') AND "
                "   pax_grp.grp_id = pax.grp_id ",
                QryParams
                );
        TCFG cfg(info.point_id);
        for(TCFG::iterator iv = cfg.begin(); iv != cfg.end(); iv++) {
            Qry.get().SetVariable("class", iv->cls);
            Qry.get().Execute();
            TCOMClassesItem item;
            item.cls = info.TlgElemIdToElem(etSubcls, iv->cls);
            item.cfg = iv->cfg;
            item.av = (Qry.get().Eof ? iv->cfg : iv->cfg - Qry.get().FieldAsInteger("seats"));
            item.pad = (Qry.get().Eof ? 0 : Qry.get().FieldAsInteger("pad"));
            items.push_back(item);
        }
    }
}

using namespace PRL_SPACE;

struct TExtraCrew {
    typedef map<TCrewType::Enum, int> TCrewItem; // <rem_code, crew amount>
    typedef map<string, TCrewItem> TAirpItems; // <airp_arv, TCrewItem>
    TAirpItems items;
    void get(int point_id);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

bool isExtraCrew(TCrewType::Enum crew_type)
{
    bool result = false;
    switch(crew_type) {
        case TCrewType::ExtraCrew:
        case TCrewType::DeadHeadCrew:
        case TCrewType::MiscOperStaff:
            result = true;
            break;
        default:
            break;
    };
    return result;
}

void TExtraCrew::get(int point_id)
{
    return; // ???
    items.clear();
    TCachedQuery Qry(
            "select pax.*, pax_grp.airp_arv from "
            "   pax_grp, "
            "   pax "
            "where "
            "   pax_grp.point_dep = :point_id and "
            "   pax_grp.grp_id = pax.grp_id ",
            QParams() << QParam("point_id", otInteger, point_id)
            );
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next()) {
        string airp_arv = Qry.get().FieldAsString("airp_arv");
        CheckIn::TSimplePaxItem pax;
        pax.fromDB(Qry.get());
        if(isExtraCrew(pax.crew_type))
            items[airp_arv][pax.crew_type]++;
    }
}

void TExtraCrew::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    for(TAirpItems::iterator airp = items.begin(); airp != items.end(); airp++) {
        for(TCrewItem::iterator crew = airp->second.begin(); crew != airp->second.end(); crew++) {
            ostringstream res;
            res
                << "-" << info.TlgElemIdToElem(etAirp, airp->first)
                << "." << TCrewTypes().encode(crew->first)
                << "/" << crew->second;
            body.push_back(res.str());
        }
    }
}

int COM(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading;
    heading << "COM" << TypeB::endl;
    tlg_row.heading = heading.str();
    tlg_row.ending = "ENDCOM" + TypeB::endl;
    try {
        const TypeB::TCOMOptions &options = *info.optionsAs<TypeB::TCOMOptions>();
        TDateTime scd = (options.version == "DME" ? info.scd_local : info.scd_utc);
        ostringstream body;
        body
            << info.flight_view() << "/"
            << DateTimeToStr(scd, "ddmmm", 1) << " " << info.airp_dep_view()
            << "/0 OP/NAM" << TypeB::endl;
        TCOMClasses classes;
        TCOMZones zones;
        TCOMStats stats;

        REPORTS::TPaxList pax_list(info.point_id);
        pax_list.options.flags.setFlag(REPORTS::oeRkWeight);
        pax_list.options.flags.setFlag(REPORTS::oeBagAmount);
        pax_list.options.flags.setFlag(REPORTS::oeBagWeight);
        pax_list.options.wait_list = boost::in_place(false);
        pax_list.options.not_refused = true;
        pax_list.fromDB();

        classes.getByStats(pax_list, info);
        stats.get(pax_list, info);
        classes.ToTlg(info, body);
        if(info.get_tlg_type() == "COM") {
            zones.get(info);
            zones.ToTlg(body);
        }
        stats.ToTlg(info, body);
        tlg_row.body = body.str();
    } catch(...) {
        ExceptionFilter(tlg_row.body, info);
    }
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

class LineOverflow: public Exception {
    public:
        LineOverflow( ):Exception( "???????????? ?????? ??????????" ) { };
};

void TWItem::ToTlg(vector<string> &body)
{
    ostringstream buf;
    buf << ".W/K/" << bagAmount << '/' << bagWeight;
    if(rkWeight != 0)
        buf << '/' << rkWeight;
    body.push_back(buf.str());
}

void TWItem::get(int pax_id)
{
    TCachedQuery Qry("select grp_id, bag_pool_num from pax where pax_id = :pax_id",
            QParams() << QParam("pax_id", otInteger, pax_id));
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        int bag_pool_num = NoExists;
        if(not Qry.get().FieldIsNULL("bag_pool_num"))
            bag_pool_num = Qry.get().FieldAsInteger("bag_pool_num");
        int grp_id = Qry.get().FieldAsInteger("grp_id");
        get(grp_id, bag_pool_num);
    }
}

void TWItem::get(int grp_id, int bag_pool_num)
{
    if(bag_pool_num == NoExists) return;
    QParams QryParams;
    QryParams
        << QParam("grp_id", otInteger, grp_id)
        << QParam("bag_pool_num", otInteger, bag_pool_num)
        << QParam("bagAmount", otInteger)
        << QParam("bagWeight", otInteger)
        << QParam("rkAmount", otInteger)
        << QParam("rkWeight", otInteger)
        << QParam("excess_wt", otInteger)
        << QParam("excess_pc", otInteger);
    TCachedQuery Qry(
            "declare "
            "   bag_pool_pax_id pax.pax_id%type; "
            "begin "
            "   bag_pool_pax_id := ckin.get_bag_pool_pax_id(:grp_id, :bag_pool_num); "
            "   :bagAmount := ckin.get_bagAmount2(:grp_id, bag_pool_pax_id, :bag_pool_num); "
            "   :bagWeight := ckin.get_bagWeight2(:grp_id, bag_pool_pax_id, :bag_pool_num); "
            "   :rkAmount := ckin.get_rkAmount2(:grp_id, bag_pool_pax_id, :bag_pool_num); "
            "   :rkWeight := ckin.get_rkWeight2(:grp_id, bag_pool_pax_id, :bag_pool_num); "
            "   :excess_wt := ckin.get_excess_wt(:grp_id, bag_pool_pax_id); "
            "   :excess_pc := ckin.get_excess_pc(:grp_id, bag_pool_pax_id); "
            "end;",
            QryParams);
    Qry.get().Execute();
    bagAmount = Qry.get().GetVariableAsInteger("bagAmount");
    bagWeight = Qry.get().GetVariableAsInteger("bagWeight");
    rkAmount = Qry.get().GetVariableAsInteger("rkAmount");
    rkWeight = Qry.get().GetVariableAsInteger("rkWeight");
    excess_wt = Qry.get().GetVariableAsInteger("excess_wt");
    excess_pc = Qry.get().GetVariableAsInteger("excess_pc");
}

struct TBTMGrpList;
// .F - ???? ????ଠ樨 ??? ??????? ?࠭?????. ?ᯮ???????? ? BTM ? PTM
// ????ࠪ???? ?????
struct TFItem {
    int point_id_trfer;
    string airline;
    int flt_no;
    string suffix;
    TDateTime scd;
    string airp_arv;
    string trfer_cls;
    virtual void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body) = 0;
    virtual TBTMGrpList *get_grp_list() = 0;
    TFItem() {
        point_id_trfer = NoExists;
        flt_no = NoExists;
        scd = NoExists;
    }
    virtual ~TFItem() { };
};

struct TExtraSeatName {
    string value;
    int ord(string rem) {
        static const char *rems[] = {"STCR", "CBBG", "EXST"};
        static const int rems_size = sizeof(rems)/sizeof(rems[0]);
        int result = 0;
        for(; result < rems_size; result++)
            if(rem == rems[result])
                break;
        if(result == rems_size)
            throw Exception("TExtraSeatName:ord: rem %s not found in rems", rem.c_str());
        return result;
    }
    void get(int pax_id, bool pr_crs = false)
    {
        TQuery Qry(&OraSession);
        string SQLText = (string)
            "select distinct "
            "   rem_code "
            "from "
            "   " + (pr_crs ? "crs_pax_rem" : "pax_rem") + " "
            "where "
            "   pax_id = :pax_id and "
            "   rem_code in('STCR', 'CBBG', 'EXST')";
        Qry.SQLText = SQLText;
        Qry.CreateVariable("pax_id", otInteger, pax_id);
        Qry.Execute();
        if(Qry.Eof)
            value = "STCR";
        else
            for(; !Qry.Eof; Qry.Next()) {
                string tmp_value = Qry.FieldAsString("rem_code");
                if(value.empty() or ord(value) > ord(tmp_value))
                    value = tmp_value;
            }
    }
};

struct TPPax {
    public:
        int seats, grp_id;
        int bag_pool_num;
        int pax_id;
        TPerson pers_type;
        bool unaccomp;
        string surname, name;
        TExtraSeatName exst;
        string trfer_cls;
        TBTMOnwardList OList;
        void dump() {
            ProgTrace(TRACE5, "TPPax");
            ProgTrace(TRACE5, "----------");
            ProgTrace(TRACE5, "name: %s", name.c_str());
            ProgTrace(TRACE5, "surname: %s", surname.c_str());
            ProgTrace(TRACE5, "grp_id: %d", grp_id);
            ProgTrace(TRACE5, "bag_pool_num: %d", bag_pool_num);
            ProgTrace(TRACE5, "pax_id: %d", pax_id);
            ProgTrace(TRACE5, "trfer_cls: %s", trfer_cls.c_str());
            ProgTrace(TRACE5, "unaccomp: %s", (unaccomp ? "true" : "false"));
            ProgTrace(TRACE5, "----------");
        }
        size_t name_length() const
        {
            size_t result = surname.size() + name.size();
            if(seats > 1)
                result += exst.value.size() * (seats - (name.empty() ? 0 :1));
            return result;
        }
        TPPax():
            seats(0),
            grp_id(NoExists),
            bag_pool_num(NoExists),
            pax_id(NoExists),
            pers_type(NoPerson),
            unaccomp(false)
        {};
};

struct TBTMGrpListItem;
struct TPList {
    private:
        typedef vector<TPPax> TPSurname;
    public:
        TBTMGrpListItem *grp;
        map<string, TPSurname> surnames; // ???ᠦ??? ???㯯?஢??? ?? 䠬????
        // ???? ???????? ?㦥? ??? sort ??????? TPSurname
        bool operator () (const TPPax &i, const TPPax &j)
        {
            return i.name_length() < j.name_length();
        };
        void get(TypeB::TDetailCreateInfo &info, string trfer_cls = "");
        void ToBTMTlg(TypeB::TDetailCreateInfo &info, vector<string> &body, TFItem &FItem); // used in BTM
        void ToPTMTlg(TypeB::TDetailCreateInfo &info, vector<string> &body, TFItem &FItem); // used in PTM
        void dump_surnames();
        TPList(TBTMGrpListItem *val): grp(val) {};
};

void TPList::dump_surnames()
{
    ProgTrace(TRACE5, "dump_surnames");
    for(map<string, TPSurname>::iterator i_surnames = surnames.begin(); i_surnames != surnames.end(); i_surnames++) {
        ProgTrace(TRACE5, "KEY SURNAME: %s", i_surnames->first.c_str());
        for(TPSurname::iterator i_surname = i_surnames->second.begin(); i_surname != i_surnames->second.end(); i_surname++)
            i_surname->dump();
    }
}


struct TBTMGrpListItem {
    int grp_id;
    int bag_pool_num;
    int main_pax_id;
    TBTMTagList NList;
    TWItem W;
    TPList PList;
    TBTMGrpListItem(): grp_id(NoExists), bag_pool_num(NoExists), main_pax_id(NoExists), PList(this) {};
    TBTMGrpListItem(const TBTMGrpListItem &val): grp_id(NoExists), main_pax_id(NoExists), PList(this)
    {
        // ??????????? ????஢???? ?㦥?, ?⮡? PList.grp ᮤ?ঠ? ?ࠢ?????? 㪠??⥫?
        grp_id = val.grp_id;
        bag_pool_num = val.bag_pool_num;
        main_pax_id = val.main_pax_id;
        NList = val.NList;
        W = val.W;
        PList = val.PList;
        PList.grp = this;
    }
};

struct TBTMGrpList {
    vector<TBTMGrpListItem> items;
    TBTMGrpListItem &get_grp_item(int grp_id, int bag_pool_num)
    {
        vector<TBTMGrpListItem>::iterator iv = items.begin();
        for(; iv != items.end(); iv++) {
            if(iv->grp_id == grp_id and iv->bag_pool_num == bag_pool_num)
                break;
        }
        if(iv == items.end())
            throw Exception("TBTMGrpList::get_grp_item: item not found, grp_id %d, bag_pool_num %d", grp_id, bag_pool_num);
        return *iv;
    }
    void get(TypeB::TDetailCreateInfo &info, TFItem &AFItem);
    virtual void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body, TFItem &FItem);
    virtual ~TBTMGrpList() {};
    void dump() {
        ProgTrace(TRACE5, "TBTMGrpList::dump");
        for(vector<TBTMGrpListItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
            ProgTrace(TRACE5, "SURNAMES FOR GRP_ID %d, BAG_POOL_NUM %d", iv->grp_id, iv->bag_pool_num);
            iv->PList.dump_surnames();
            ProgTrace(TRACE5, "END OF SURNAMES FOR GRP_ID %d, BAG_POOL_NUM %d", iv->grp_id, iv->bag_pool_num);
        }
        ProgTrace(TRACE5, "END OF TBTMGrpList::dump");
    }
};

// ?।?⠢????? ᯨ᪠ ????? .P/ ??? ?? ?㤥? ? ⥫??ࠬ??.
// ???祬 ᯨ᮪ ???? ?㤥? ?।?⠢???? ?⤥????? ??㯯? ???ᠦ?஢
// ??ꥤ??????? ?? grp_id ? bag_pool_num
struct TPLine {
    bool print_bag;
    bool skip;
    int seats;
    size_t inf;
    size_t chd;
    int grp_id;
    int bag_pool_num;
    string surname;
    vector<string> names;

    TPLine():
        print_bag(false),
        skip(false),
        seats(0),
        inf(0),
        chd(0),
        grp_id(NoExists),
        bag_pool_num(NoExists)
    {};
    size_t get_line_size() {
        return get_line().size() + TypeB::endl.size();
    }
    size_t get_line_size(TypeB::TDetailCreateInfo &info, TFItem &FItem) {
        return get_line(info, FItem).size() + TypeB::endl.size();
    }
    string get_line() {
        ostringstream buf;
        buf << ".P/";
        buf << surname;
        for(vector<string>::iterator iv = names.begin(); iv != names.end(); iv++) {
            if(!iv->empty())
                buf << "/" << *iv;
        }
        return buf.str();
    }
    string get_line(TypeB::TDetailCreateInfo &info, TFItem &FItem) {
        ostringstream result;
        result
            << info.TlgElemIdToElem(etAirline, FItem.airline)
            << setw(3) << setfill('0') << FItem.flt_no
            << (FItem.suffix.empty() ? "" : info.TlgElemIdToElem(etSuffix, FItem.suffix))
            << "/"
            << DateTimeToStr(FItem.scd, "dd", info.is_lat())
            << " "
            << info.TlgElemIdToElem(etAirp, FItem.airp_arv)
            << " "
            << seats
            << info.TlgElemIdToElem(etSubcls, FItem.trfer_cls)
            << " ";
        if(print_bag)
            result
                << FItem.get_grp_list()->get_grp_item(grp_id, bag_pool_num).W.bagAmount;
        else
            result
                << 0;
        result
            << "B"
            << " "
            << surname;
        for(vector<string>::iterator iv = names.begin(); iv != names.end(); iv++) {
            if(!iv->empty())
                result << "/" << *iv;
        }
        return result.str();
    }
    TPLine & operator += (const TPPax & pax)
    {
        if(grp_id == NoExists) {
            grp_id = pax.grp_id;
            bag_pool_num = pax.bag_pool_num;
        } else {
            if(grp_id != pax.grp_id and bag_pool_num != pax.bag_pool_num)
                throw Exception("TPLine operator +=: cannot add pax with different grp_id & bag_pool_num");
        }
        seats += pax.seats;
        switch(pax.pers_type) {
            case adult:
            case NoPerson:
                break;
            case child:
                chd++;
                break;
            case baby:
                inf++;
                break;
        }
        int name_count = 0;
        if(not pax.name.empty()) {
            names.push_back(pax.name);
            name_count = 1;
        }
        for(int i = 0; i < pax.seats - name_count; i++)
            names.push_back(pax.exst.value);
        return *this;
    }
    TPLine operator + (const TPPax & pax)
    {
        TPLine result(*this);
        result += pax;
        return result;
    }
    TPLine & operator += (const int &aseats)
    {
        seats += aseats;
        return *this;
    }
    TPLine operator + (const int &aseats)
    {
        TPLine result(*this);
        result += aseats;
        return result;
    }
    bool operator == (const string &s)
    {
        return not skip and surname == s;
    }
};

void TPList::ToPTMTlg(TypeB::TDetailCreateInfo &info, vector<string> &body, TFItem &FItem)
{
    for(map<string, TPSurname>::iterator im = surnames.begin(); im != surnames.end(); im++) {
        vector<TPPax> one; // ???? ?????
        vector<TPPax> many_noname; // ??? ?????, ?????? ?????? ?????
        vector<TPPax> many_name; // ? ??????, ?????? ?????? ?????
        {
            TPSurname &pax_list = im->second;
            // ???????? ᯨ᮪ ???ᠦ?஢ ?? 3 ???ᯨ᪠
            for(vector<TPPax>::iterator iv = pax_list.begin(); iv != pax_list.end(); iv++) {
                if(iv->unaccomp) continue;
                if(iv->seats == 1) {
                    one.push_back(*iv);
                } else if(iv->name.empty()) {
                    many_noname.push_back(*iv);
                } else {
                    many_name.push_back(*iv);
                }
            }
        }
        bool print_bag = im == surnames.begin();
        if(not one.empty()) {
            // ??ࠡ?⪠ one
            // ?????뢠?? ? ??ப? ?⮫쪮 ????, ᪮?쪮 ??????, ??⠫???? ???㡠??.
            sort(one.begin(), one.end(), *this);
            TPLine pax_line;
            pax_line.surname = im->first;
            for(vector<TPPax>::iterator iv = one.begin(); iv != one.end(); iv++) {
                pax_line += *iv;
            }
            if(pax_line.grp_id != NoExists) {
                pax_line.print_bag = print_bag;
                print_bag = false;
                string line = pax_line.get_line(info, FItem);
                if(line.size() + TypeB::endl.size() > LINE_SIZE) {
                    size_t start_pos = line.rfind(" "); // starting position of name element;
                    size_t oblique_pos = line.rfind("/", LINE_SIZE - 1);
                    if(oblique_pos < start_pos)
                        line = line.substr(0, LINE_SIZE - 1);
                    else
                        line = line.substr(0, oblique_pos);
                }
                body.push_back(line);
            }
        }
        if(not many_name.empty()) {
            // ??ࠡ?⪠ many_name. ?????뢠?? ? ??ப? ᪮?쪮 ??????.
            // ??⮬ ?? ᫥? ??ப?. ?᫨ ? ???? ?? ???????, ??१??? ??? ?? ?????? ᨬ????, ??⥬ 䠬????.
            sort(many_name.begin(), many_name.end(), *this);
            TPLine pax_line;
            pax_line.print_bag = print_bag;
            print_bag = false;
            pax_line.surname = im->first;
            for(vector<TPPax>::iterator iv = many_name.begin(); iv != many_name.end(); iv++) {
                bool finished = false;
                while(not finished) {
                    finished = true;
                    TPLine tmp_pax_line = pax_line + *iv;
                    string line = tmp_pax_line.get_line(info, FItem);
                    if(line.size() + TypeB::endl.size() > LINE_SIZE) {
                        if(pax_line.grp_id == NoExists) {
                            // ???? ???ᠦ?? ?? ??? ??ப? ?? ?????⨫??
                            size_t diff = line.size() + TypeB::endl.size() - LINE_SIZE;
                            TPPax fix_pax = *iv;
                            if(fix_pax.name.size() > diff) {
                                fix_pax.name = fix_pax.name.substr(0, fix_pax.name.size() - diff);
                                body.push_back((pax_line + fix_pax).get_line(info, FItem));
                            } else {
                                diff -= fix_pax.name.size() - 1;
                                fix_pax.name = fix_pax.name.substr(0, 1);
                                if(fix_pax.surname.size() > diff) {
                                    pax_line.surname = fix_pax.surname.substr(0, fix_pax.surname.size() - diff);
                                    body.push_back((pax_line + fix_pax).get_line(info, FItem));
                                    pax_line.surname = im->first;
                                } else
                                    throw Exception("many_name item insertion failed");
                            }
                            pax_line.grp_id = NoExists;
                            pax_line.names.clear();
                            pax_line.seats = 0;
                            pax_line.print_bag = false;
                        } else {
                            // ???ᠦ?? ?? ???? ? ??ப? ? ??㣨? ???ᠦ?ࠬ
                            body.push_back((pax_line).get_line(info, FItem));
                            pax_line.grp_id = NoExists;
                            pax_line.names.clear();
                            pax_line.seats = 0;
                            pax_line.print_bag = false;
                            finished = false;
                        }
                    } else
                        pax_line += *iv;
                }
            }
            if(pax_line.grp_id != NoExists)
                body.push_back((pax_line).get_line(info, FItem));
        }
        if(not many_noname.empty()) {
            // ?????뢠?? ??????? ???ᠦ??? ?? ?⤥?쭮???
            // ?᫨ ?? ???????, ??१??? 䠬????
            for(vector<TPPax>::iterator iv = many_noname.begin(); iv != many_noname.end(); iv++) {
                TPLine pax_line;
                pax_line.print_bag = print_bag;
                print_bag = false;
                pax_line.surname = im->first;
                string line = (pax_line + *iv).get_line(info, FItem);
                if(line.size() + TypeB::endl.size() > LINE_SIZE) {
                    size_t diff = line.size() + TypeB::endl.size() - LINE_SIZE;
                    if(pax_line.surname.size() > diff) {
                        pax_line.surname = pax_line.surname.substr(0, pax_line.surname.size() - diff);
                    } else
                        throw Exception("many_noname item insertion failed");
                    body.push_back((pax_line + *iv).get_line(info, FItem));
                } else
                    body.push_back(line);
            }
        }
    }
}

void TPList::ToBTMTlg(TypeB::TDetailCreateInfo &info, vector<string> &body, TFItem &FItem)
{
    vector<TPLine> lines;
    // ??? ??? ᫮???? ???????? ??ꥤ?????? ???? ??? ???? 䠬???? ? ??? ⠪??
    // ⥯??? ?? ??த???? ? ᯨ᮪ ?? ?ᥣ? ?????? ???ᠦ??? ? main_pax_id
    vector<TPPax>::iterator main_pax;
    for(map<string, TPSurname>::iterator im = surnames.begin(); im != surnames.end(); im++) {
        TPSurname &pax_list = im->second;
        sort(pax_list.begin(), pax_list.end(), *this);
        vector<TPPax>::iterator iv = pax_list.begin();
        while(
                iv != pax_list.end() and
                (iv->trfer_cls != FItem.trfer_cls or iv->pax_id != grp->main_pax_id)
             )
            iv++;
        if(iv == pax_list.end())
            continue;
        TPLine line;
        line.surname = im->first;
        lines.push_back(line);
        while(iv != pax_list.end()) {
            if(iv->trfer_cls != FItem.trfer_cls or iv->pax_id != grp->main_pax_id) {
                iv++;
                continue;
            }
            TPLine &curLine = lines.back();
            if((curLine + *iv).get_line_size() > LINE_SIZE) {// ???, ??ப? ??९??????
                if(curLine.names.empty()) {
                    curLine += iv->seats;
                    main_pax = iv;
                    iv++;
                } else {
                    lines.push_back(line);
                }
            } else {
                curLine += *iv;
                main_pax = iv;
                iv++;
            }
        }
    }

    if(lines.size() > 1)
        throw Exception("TPList::ToBTMTlg: unexpected lines size for main_pax_id: %zu", lines.size());
    if(lines.empty())
        return;
    main_pax->OList.ToTlg(info, body);

    // ? ????祭??? ??????? ??ப, ??१??? ᫨誮? ???????
    // 䠬????, ??ꥤ??塞 ????? ᮡ??, ?᫨ ????????
    // ᮢ??????? ??१????? 䠬????.
    for(vector<TPLine>::iterator iv = lines.begin(); iv != lines.end(); iv++) {
        TPLine &curLine = *iv;
        if(curLine.names.empty()) {
            size_t line_size = curLine.get_line_size();
            if(line_size > LINE_SIZE) {
                string surname = curLine.surname.substr(0, curLine.surname.size() - (line_size - LINE_SIZE));
                vector<TPLine>::iterator found_l = find(lines.begin(), lines.end(), surname);
                if(found_l != lines.end()) {
                    curLine.skip = true;
                    *found_l += curLine.seats;
                } else
                    curLine.surname = surname;
            }
        }
    }
    for(vector<TPLine>::iterator iv = lines.begin(); iv != lines.end(); iv++) {
        if(iv->skip) continue;
        body.push_back(iv->get_line());
    }
}

void TPList::get(TypeB::TDetailCreateInfo &info, string trfer_cls)
{
    QParams QryParams;
    QryParams
        << QParam("grp_id", otInteger, grp->grp_id)
        << QParam("pr_lat", otInteger, info.is_lat());
    if(grp->bag_pool_num == NoExists)
        QryParams << QParam("bag_pool_num", otInteger, FNull);
    else
        QryParams << QParam("bag_pool_num", otInteger, grp->bag_pool_num);
    TCachedQuery Qry(
        "select \n"
        "   pax.pax_id, \n"
        "   pax.pr_brd, \n"
        "   pax.seats, \n"
        "   system.transliter(pax.surname, 1, :pr_lat) surname, \n"
        "   pax.pers_type, \n"
        "   system.transliter(pax.name, 1, :pr_lat) name, \n"
        "   subcls.class \n"
        "from \n"
        "   pax, \n"
        "   transfer_subcls, \n"
        "   subcls \n"
        "where \n"
        "  pax.grp_id = :grp_id and \n"
        "  nvl(pax.bag_pool_num, 0) = nvl(:bag_pool_num, 0) and \n"
        "  pax.pax_id = transfer_subcls.pax_id(+) and \n"
        "  transfer_subcls.transfer_num(+) = 1 and \n"
        "  transfer_subcls.subclass = subcls.code(+) \n"
        "order by \n"
        "   surname, \n"
        "   name \n",
        QryParams);
    Qry.get().Execute();
    if(Qry.get().Eof) {
        TPPax item;
        item.grp_id = grp->grp_id;
        item.bag_pool_num = grp->bag_pool_num;
        item.seats = 1;
        item.surname = "UNACCOMPANIED";
        item.unaccomp = true;
        item.OList.get(item.grp_id, item.pax_id);
        surnames[item.surname].push_back(item);
    } else {
        int col_pax_id = Qry.get().FieldIndex("pax_id");
        int col_pr_brd = Qry.get().FieldIndex("pr_brd");
        int col_seats = Qry.get().FieldIndex("seats");
        int col_surname = Qry.get().FieldIndex("surname");
        int col_pers_type = Qry.get().FieldIndex("pers_type");
        int col_name = Qry.get().FieldIndex("name");
        int col_cls = Qry.get().FieldIndex("class");
        for(; !Qry.get().Eof; Qry.get().Next()) {
            if(Qry.get().FieldAsInteger(col_pr_brd) == 0)
                continue;
            TPPax item;
            item.pax_id = Qry.get().FieldAsInteger(col_pax_id);
            item.grp_id = grp->grp_id;
            item.bag_pool_num = grp->bag_pool_num;
            item.seats = Qry.get().FieldAsInteger(col_seats);
            if(item.seats == 0) continue;
            if(item.seats > 1)
                item.exst.get(Qry.get().FieldAsInteger(col_pax_id));
            item.surname = Qry.get().FieldAsString(col_surname);
            item.pers_type = DecodePerson(Qry.get().FieldAsString(col_pers_type));
            item.name = Qry.get().FieldAsString(col_name);
            item.trfer_cls = Qry.get().FieldAsString(col_cls);
            if(not trfer_cls.empty() and item.trfer_cls != trfer_cls)
                continue;
            item.OList.get(item.grp_id, item.pax_id);
            surnames[item.surname].push_back(item);
        }
    }
}

    struct TPTMGrpList:TBTMGrpList {
        void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body, TFItem &FItem)
        {
            if(info.get_tlg_type() == "PTM")
                for(vector<TBTMGrpListItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
                    iv->PList.ToPTMTlg(info, body, FItem);
                }
        }
    };

void TBTMGrpList::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body, TFItem &AFItem)
{
    for(vector<TBTMGrpListItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
        vector<string> plist;
        iv->PList.ToBTMTlg(info, plist, AFItem);
        if(plist.empty())
            continue;
        if(iv->NList.items.empty())
            continue;
        iv->NList.ToTlg(info, body);
        iv->W.ToTlg(body);
        body.insert(body.end(), plist.begin(), plist.end());
    }
}

void TBTMGrpList::get(TypeB::TDetailCreateInfo &info, TFItem &FItem)
{
    const TypeB::TAirpTrferOptions &trferOptions=*(info.optionsAs<TypeB::TAirpTrferOptions>());
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select distinct  \n"
        "   transfer.grp_id,  \n"
        "   nvl2(pax.grp_id, pax.bag_pool_num, 1) bag_pool_num, \n"
        "   ckin.get_bag_pool_pax_id(transfer.grp_id, pax.bag_pool_num) bag_pool_pax_id  \n"
        "from   \n"
        "   transfer,  \n"
        "   pax_grp,  \n"
        "   pax, \n"
        "   (select  \n"
        "       pax_grp.grp_id,  \n"
        "       subcls.class  \n"
        "    from  \n"
        "       pax_grp,  \n"
        "       pax,  \n"
        "       transfer_subcls,  \n"
        "       subcls  \n"
        "    where  \n"
        "      pax_grp.point_dep = :point_id and \n"
        "      pax_grp.airp_arv = :airp_arv and  \n"
        "      pax_grp.grp_id = pax.grp_id and  \n"
        "      pax.pax_id = transfer_subcls.pax_id and  \n"
        "      transfer_subcls.transfer_num = 1 and  \n"
        "      transfer_subcls.subclass = subcls.code  \n"
        "   ) a \n"
        "where   \n"
        "   pax_grp.grp_id = pax.grp_id(+) and  \n"
        "   transfer.point_id_trfer = :point_id_trfer and  \n"
        "   transfer.grp_id = pax_grp.grp_id and  \n"
        "   transfer.transfer_num = 1 and  \n"
        "   transfer.airp_arv = :trfer_airp and  \n"
        "   pax_grp.status NOT IN ('T', 'E') and \n"
        "   pax_grp.point_dep = :point_id and  \n"
        "   pax_grp.airp_arv = :airp_arv and \n"
        "   pax_grp.grp_id = a.grp_id(+) and \n"
        "   nvl(a.class, ' ') = nvl(:trfer_cls, ' ') \n"
        "order by  \n"
        "   grp_id,  \n"
        "   bag_pool_num \n";
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.CreateVariable("airp_arv", otString, trferOptions.airp_trfer);
    Qry.CreateVariable("trfer_airp", otString, FItem.airp_arv);
    Qry.CreateVariable("point_id_trfer", otInteger, FItem.point_id_trfer);
    Qry.CreateVariable("trfer_cls", otString, FItem.trfer_cls);

    for(int i = 0; i < Qry.VariablesCount(); i++)
        LogTrace(TRACE5) << Qry.VariableName(i) << " = " << Qry.GetVariableAsString(i);

    Qry.Execute();
    if(!Qry.Eof) {
        int col_grp_id = Qry.FieldIndex("grp_id");
        int col_bag_pool_num = Qry.FieldIndex("bag_pool_num");
        int col_main_pax_id = Qry.FieldIndex("bag_pool_pax_id");
        for(; !Qry.Eof; Qry.Next()) {
            TBTMGrpListItem item;
            item.grp_id = Qry.FieldAsInteger(col_grp_id);
            if(not Qry.FieldIsNULL(col_bag_pool_num))
                item.bag_pool_num = Qry.FieldAsInteger(col_bag_pool_num);
            if(not Qry.FieldIsNULL(col_main_pax_id))
                item.main_pax_id = Qry.FieldAsInteger(col_main_pax_id);
            item.NList.get(item.grp_id, item.bag_pool_num);
            item.PList.get(info, FItem.trfer_cls);
            if(item.PList.surnames.empty())
                continue;
            item.W.get(item.grp_id, item.bag_pool_num);
            items.push_back(item);
        }
    }
}

struct TBTMFItem:TFItem {
    TBTMGrpList grp_list;
    TBTMGrpList *get_grp_list() { return &grp_list; };
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        ostringstream line;
        line
            << ".F/"
            << info.TlgElemIdToElem(etAirline, airline)
            << setw(3) << setfill('0') << flt_no
            << (suffix.empty() ? "" : info.TlgElemIdToElem(etSuffix, suffix))
            << "/"
            << DateTimeToStr(scd, "ddmmm", info.is_lat())
            << "/"
            << info.TlgElemIdToElem(etAirp, airp_arv);
        if(not trfer_cls.empty())
            line
                << "/"
                << info.TlgElemIdToElem(etClass, trfer_cls);
        body.push_back(line.str());
    }
};

struct TPTMFItem:TFItem {
    TPTMGrpList grp_list;
    TBTMGrpList *get_grp_list() { return &grp_list; };
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        if(info.get_tlg_type() == "PTMN" and not trfer_cls.empty()) {
            ostringstream result;
            result
                << info.TlgElemIdToElem(etAirline, airline)
                << setw(3) << setfill('0') << flt_no
                << (suffix.empty() ? "" : info.TlgElemIdToElem(etSuffix, suffix))
                << "/"
                << DateTimeToStr(scd, "dd", info.is_lat())
                << " "
                << info.TlgElemIdToElem(etAirp, airp_arv)
                << " ";
            int seats = 0;
            int baggage = 0;
            for(vector<TBTMGrpListItem>::iterator iv = grp_list.items.begin(); iv != grp_list.items.end(); iv++) {
                baggage += iv->W.bagAmount;
                map<string, vector<TPPax> > &surnames = iv->PList.surnames;
                for(map<string, vector<TPPax> >::iterator im = surnames.begin(); im != surnames.end(); im++) {
                    vector<TPPax> &paxes = im->second;
                    for(vector<TPPax>::iterator i_paxes = paxes.begin(); i_paxes != paxes.end(); i_paxes++) {
                        if(i_paxes->unaccomp)
                            continue;
                        seats += i_paxes->seats;
                    }
                }
            }
            result
                << seats
                << info.TlgElemIdToElem(etClass, trfer_cls)
                << " "
                << baggage
                << "B";
            body.push_back(result.str());
        }
    }
};

// ???᮪ ???ࠢ????? ?࠭?????
template <class T>
struct TFList {
    vector<T> items;
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

    template <class T>
void TFList<T>::get(TypeB::TDetailCreateInfo &info)
{
    const TypeB::TAirpTrferOptions &trferOptions=*(info.optionsAs<TypeB::TAirpTrferOptions>());
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select distinct \n"
        "    transfer.point_id_trfer, \n"
        "    trfer_trips.airline, \n"
        "    trfer_trips.flt_no, \n"
        "    trfer_trips.scd, \n"
        "    transfer.airp_arv, \n"
        "    a.class \n"
        "from \n"
        "    transfer, \n"
        "    trfer_trips, \n"
        "    (select \n"
        "        pax_grp.grp_id, \n"
        "        subcls.class \n"
        "     from \n"
        "        pax_grp, \n"
        "        pax, \n"
        "        transfer_subcls, \n"
        "        subcls \n"
        "     where \n"
        "       pax_grp.point_dep = :point_id and \n"
        "       pax_grp.status NOT IN ('E') and \n"
        "       pax_grp.airp_arv = :airp and \n"
        "       pax_grp.grp_id = pax.grp_id and \n"
        "       pax.pax_id = transfer_subcls.pax_id and \n"
        "       transfer_subcls.transfer_num = 1 and \n"
        "       transfer_subcls.subclass = subcls.code \n"
        "     union \n"
        "     select \n"
        "       pax_grp.grp_id, \n"
        "       null \n"
        "     from \n"
        "       pax_grp \n"
        "     where \n"
        "       pax_grp.point_dep = :point_id and \n"
        "       pax_grp.status NOT IN ('E') and \n"
        "       pax_grp.airp_arv = :airp and \n"
        "       pax_grp.class is null \n"
        "    ) a \n"
        "where \n"
        "    transfer.grp_id = a.grp_id and \n"
        "    transfer.transfer_num = 1 and \n"
        "    transfer.point_id_trfer = trfer_trips.point_id \n"
        "order by \n"
        "    trfer_trips.airline, \n"
        "    trfer_trips.flt_no, \n"
        "    trfer_trips.scd, \n"
        "    transfer.airp_arv \n";
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.CreateVariable("airp", otString, trferOptions.airp_trfer);
    LogTrace(TRACE5) << "TFList Qry: " << Qry.SQLText.SQLText();
    Qry.Execute();
    LogTrace(TRACE5) << "TFList Qry executed";
    if(!Qry.Eof) {
        int col_point_id_trfer = Qry.FieldIndex("point_id_trfer");
        int col_airline = Qry.FieldIndex("airline");
        int col_flt_no = Qry.FieldIndex("flt_no");
        int col_scd = Qry.FieldIndex("scd");
        int col_airp_arv = Qry.FieldIndex("airp_arv");
        int col_class = Qry.FieldIndex("class");
        for(; !Qry.Eof; Qry.Next()) {
            T item;
            item.point_id_trfer = Qry.FieldAsInteger(col_point_id_trfer);
            item.airline = Qry.FieldAsString(col_airline);
            item.flt_no = Qry.FieldAsInteger(col_flt_no);
            item.scd = Qry.FieldAsDateTime(col_scd);
            item.airp_arv = Qry.FieldAsString(col_airp_arv);
            item.trfer_cls = Qry.FieldAsString(col_class);
            item.grp_list.get(info, item);
            if(item.grp_list.items.empty())
                continue;
            items.push_back(item);
        }
    }
}

    template <class T>
void TFList<T>::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    for(size_t i = 0; i < items.size(); i++) {
        vector<string> grp_list_body;
        items[i].grp_list.ToTlg(info, grp_list_body, items[i]);
        // ??? ⨯? ⥫??ࠬ? ?஬? PTMN ?஢????? grp_list_body.
        // ??? PTMN ?? ?ᥣ?? ???⮩.
        if(info.get_tlg_type() != "PTMN" and grp_list_body.empty())
            continue;
        items[i].ToTlg(info, body);
        body.insert(body.end(), grp_list_body.begin(), grp_list_body.end());
    }
}

int calculate_btm_grp_len(const vector<string>::iterator &iv, const vector<string> &body)
{
    int result = 0;
    bool P_found = false;
    for(vector<string>::iterator j = iv; j != body.end(); j++) {
        if(not P_found and j->find(".P/") == 0)
            P_found = true;
        if(
                P_found and
                (j->find(".N") == 0 or j->find(".F") == 0)
          ) // ??諨 ????? ??㯯?
            break;
        result += j->size() + TypeB::endl.size();
    }
    return result;
}

struct TSSRItem {
    string code;
    string free_text;
    bool operator < (const TSSRItem &item) const
    {
      if (code!=item.code)
        return code<item.code;
      return free_text<item.free_text;
    };
};

struct TSSR {
    vector<TSSRItem> items;
    void get(const TRemGrp &ssr_rem_grp, int pax_id);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
    string ToPILTlg(TypeB::TDetailCreateInfo &info) const;
};

string TSSR::ToPILTlg(TypeB::TDetailCreateInfo &info) const
{
    string result;
    for(vector<TSSRItem>::const_iterator iv = items.begin(); iv != items.end(); iv++) {
        if(not result.empty())
            result += " ";
        result += iv->code;
        if(not iv->free_text.empty())
            result += " " + transliter(iv->free_text, 1, info.is_lat());
    }
    return result;
}

void TSSR::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    for(vector<TSSRItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
        string buf = " " + iv->code;
        if(not iv->free_text.empty()) {
            buf += " " + transliter(iv->free_text, 1, info.is_lat());
            string offset;
            while(buf.size() > LINE_SIZE) {
                size_t idx = buf.rfind(" ", LINE_SIZE - 1);
                if(idx <= 5)
                    idx = LINE_SIZE;
                body.push_back(offset + buf.substr(0, idx));
                buf = buf.substr(idx);
                if(offset.empty())
                    offset.assign(6, ' ');
            }
            if(not buf.empty())
                body.push_back(offset + buf);
        } else
            body.push_back(buf);
    }
}

void TSSR::get(const TRemGrp &ssr_rem_grp, int pax_id)
{
  multiset<CheckIn::TPaxRemItem> rems;
  CheckIn::LoadPaxRem(pax_id, rems);

  set<CheckIn::TPaxFQTItem> fqts;
  CheckIn::LoadPaxFQT(pax_id, fqts);
  for(set<CheckIn::TPaxFQTItem>::const_iterator r=fqts.begin(); r!=fqts.end(); ++r)
    rems.insert(CheckIn::TPaxRemItem(*r, false));

  for(multiset<CheckIn::TPaxRemItem>::const_iterator r=rems.begin(); r!=rems.end(); ++r)
  {
    if(not ssr_rem_grp.exists(r->code)) continue;
    TRemCategory cat=getRemCategory(*r);
    if (cat!=remFQT && isDisabledRemCategory(cat)) continue;
    TSSRItem item;
    item.code=r->code;
    item.free_text=r->text;
    if(item.code == item.free_text)
      item.free_text.erase();
    else
      item.free_text = item.free_text.substr(item.code.size() + 1);
    TrimString(item.free_text);
    items.push_back(item);
  };

  sort(items.begin(), items.end());
}

struct TClsCmp {
    bool operator() (const string &l, const string &r) const
    {
        TClasses &classes = (TClasses &)base_tables.get("classes");
        int l_prior = ((const TClassesRow &)classes.get_row("code", l)).priority;
        int r_prior = ((const TClassesRow &)classes.get_row("code", r)).priority;
        return l_prior < r_prior;
    }
};

struct TPSMPax {
    int pax_id;
    int point_id;
    TName name;
    TPSMOnwardList OItem;
    TTlgSeatList seat_no;
    string airp_arv;
    string cls;
    TSSR ssr;
    TPSMPax(): pax_id(NoExists) {}
};

typedef vector<TPSMPax> TPSMPaxLst;
typedef map<string, TPSMPaxLst> TPSMCls;
typedef map<string, TPSMCls> TPSMTarget;

struct TPSM {
    TCFG cfg;
    TPSMTarget items;
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

struct TCounter {
    int val;
    TCounter(): val(0) {};
};

typedef map<string, TCounter, TClsCmp> TPSMSSRItem;
typedef map<string, TPSMSSRItem> TSSRCodesList;

struct TSSRCodes {
    TCFG &cfg;
    TSSRCodesList items;
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
    void add(string cls, TSSR &ssr);
    TSSRCodes(TCFG &acfg): cfg(acfg) {};
};

void TSSRCodes::add(string cls, TSSR &ssr)
{
    for(vector<TSSRItem>::iterator iv = ssr.items.begin(); iv != ssr.items.end(); iv++)
        (items[iv->code][cls]).val++;
}

void TSSRCodes::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    for(TSSRCodesList::iterator i_items = items.begin(); i_items != items.end(); i_items++) {
        ostringstream buf;
        buf << setw(4) << left << i_items->first;
        TPSMSSRItem &SSRItem = i_items->second;
        for(vector<TCFGItem>::iterator i_cfg = cfg.begin(); i_cfg != cfg.end(); i_cfg++) {
            TCounter &counter = SSRItem[i_cfg->cls];
            buf << " " << setw(3) << setfill('0') << right << counter.val << info.TlgElemIdToElem(etClass, i_cfg->cls);
        }
        body.push_back(buf.str());
    }
}

void TPSM::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    TTripRoute route;
    route.GetRouteAfter(NoExists, info.point_id, trtNotCurrent, trtNotCancelled);
    for(TTripRoute::iterator iv = route.begin(); iv != route.end(); iv++) {
        TSSRCodes ssr_codes(cfg);
        TPSMCls &PSMCls = items[iv->airp];
        vector<string> pax_list_body;
        int target_pax = 0;
        int target_ssr = 0;
        for(vector<TCFGItem>::iterator i_cfg = cfg.begin(); i_cfg != cfg.end(); i_cfg++) {
            TPSMPaxLst &pax_list = PSMCls[i_cfg->cls];
            int ssr = 0;
            vector<string> cls_body;
            for(TPSMPaxLst::iterator i_pax = pax_list.begin(); i_pax != pax_list.end(); i_pax++) {
                ssr_codes.add(i_cfg->cls, i_pax->ssr);
                if(i_pax->seat_no.empty())
                    i_pax->name.ToTlg(info, cls_body);
                else
                    i_pax->name.ToTlg(info, cls_body, "  " + i_pax->seat_no.get_seat_one(info.is_lat()));
                i_pax->OItem.ToTlg(info, cls_body);
                i_pax->ssr.ToTlg(info, cls_body);
                ssr += i_pax->ssr.items.size();
            }
            target_pax += pax_list.size();
            target_ssr += ssr;
            ostringstream buf;
            buf
                << info.TlgElemIdToElem(etClass, i_cfg->cls)
                << " CLASS ";
            if(pax_list.empty())
                buf << "NIL";
            else
                buf
                    << pax_list.size()
                    << "PAX / "
                    << ssr
                    << "SSR";
            pax_list_body.push_back(buf.str());
            pax_list_body.insert(pax_list_body.end(), cls_body.begin(), cls_body.end());
        }
        ostringstream buf;
        buf
            << "-" << info.TlgElemIdToElem(etAirp, iv->airp)
            << " ";
        if(target_pax == 0) {
            buf << "NIL";
            body.push_back(buf.str());
        } else {
            buf
                << target_pax
                << "PAX / "
                << target_ssr
                << "SSR";
            body.push_back(buf.str());
            ssr_codes.ToTlg(info, body);
            body.insert(body.end(), pax_list_body.begin(), pax_list_body.end());
        }
    }
}

void TPSM::get(TypeB::TDetailCreateInfo &info)
{
    cfg.get(info.point_id);
    if(cfg.empty()) cfg.get(NoExists);
    TTlgCompLayerList complayers;
    if(not isFreeSeating(info.point_id) and not isEmptySalons(info.point_id))
        getSalonLayers( info, complayers, false );
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   pax_id, "
        "   pax.surname, "
        "   pax.name, "
        "   pax_grp.airp_arv, "
        "   nvl(pax.cabin_class, pax_grp.class) class "
        "from "
        "   pax, "
        "   pax_grp "
        "where "
        "   pax_grp.point_dep = :point_dep and "
        "   pax_grp.status NOT IN ('E') and "
        "   pax.pr_brd = 1 and "
        "   pax_grp.grp_id = pax.grp_id "
        "order by "
        "   pax.surname ";
    Qry.CreateVariable("point_dep", otInteger, info.point_id);
    Qry.Execute();
    TRemGrp ssr_rem_grp;
    ssr_rem_grp.Load(retTYPEB_PSM, info.point_id);
    if(!Qry.Eof) {
        int col_pax_id = Qry.FieldIndex("pax_id");
        int col_surname = Qry.FieldIndex("surname");
        int col_name = Qry.FieldIndex("name");
        int col_airp_arv = Qry.FieldIndex("airp_arv");
        int col_class = Qry.FieldIndex("class");
        for(; !Qry.Eof; Qry.Next()) {
            TPSMPax item;
            item.pax_id = Qry.FieldAsInteger(col_pax_id);
            item.name.surname = Qry.FieldAsString(col_surname);
            item.name.name = Qry.FieldAsString(col_name);
            item.airp_arv = Qry.FieldAsString(col_airp_arv);
            item.cls = Qry.FieldAsString(col_class);
            item.ssr.get(ssr_rem_grp, item.pax_id);
            if(item.ssr.items.empty())
                continue;
            item.seat_no.add_seats(item.pax_id, complayers);
            item.OItem.get(NoExists, item.pax_id);
            items[item.airp_arv][item.cls].push_back(item);
        }
    }
}

struct TPILPax {
    int pax_id;
    TName name;
    TTlgSeatList seat_no;
    string airp_arv;
    string cls;
    TSSR ssr;
    TPILPax(): pax_id(NoExists) {}
};

typedef vector<TPILPax> TPILPaxLst;
typedef map<string, TPILPaxLst> TPILCls;

struct TPIL {
    TCFG cfg;
    TPILCls items;
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

void TPIL::get(TypeB::TDetailCreateInfo &info)
{
    cfg.get(info.point_id);
    if(cfg.empty()) cfg.get(NoExists);
    TTlgCompLayerList complayers;
    if(isFreeSeating(info.point_id))
        throw UserException("MSG.SALONS.FREE_SEATING");
    if(isEmptySalons(info.point_id))
        throw UserException("MSG.FLIGHT_WO_CRAFT_CONFIGURE");
    getSalonLayers( info, complayers, false );
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   pax_id, "
        "   pax.surname, "
        "   pax.name, "
        "   pax_grp.airp_arv, "
        "   nvl(pax.cabin_class, pax_grp.class) class "
        "from "
        "   pax, "
        "   pax_grp "
        "where "
        "   pax_grp.point_dep = :point_dep and "
        "   pax_grp.status NOT IN ('E') and "
        "   pax.pr_brd = 1 and "
        "   pax_grp.grp_id = pax.grp_id "
        "order by "
        "   pax.surname, "
        "   pax.name ";
    Qry.CreateVariable("point_dep", otInteger, info.point_id);
    TRemGrp ssr_rem_grp;
    ssr_rem_grp.Load(retTYPEB_PIL, info.point_id);
    Qry.Execute(); if(!Qry.Eof) {
        int col_pax_id = Qry.FieldIndex("pax_id");
        int col_surname = Qry.FieldIndex("surname");
        int col_name = Qry.FieldIndex("name");
        int col_airp_arv = Qry.FieldIndex("airp_arv");
        int col_class = Qry.FieldIndex("class");
        for(; !Qry.Eof; Qry.Next()) {
            TPILPax item;
            item.pax_id = Qry.FieldAsInteger(col_pax_id);
            item.name.surname = Qry.FieldAsString(col_surname);
            item.name.name = Qry.FieldAsString(col_name);
            item.airp_arv = Qry.FieldAsString(col_airp_arv);
            item.cls = Qry.FieldAsString(col_class);
            item.ssr.get(ssr_rem_grp, item.pax_id);
            item.seat_no.add_seats(item.pax_id, complayers);
            items[item.cls].push_back(item);
        }
    }
}

void TPIL::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    for(vector<TCFGItem>::iterator iv = cfg.begin(); iv != cfg.end(); iv++) {
        body.push_back(info.TlgElemIdToElem(etClass, iv->cls) + "CLASS");
        const TPILPaxLst &pax_lst = items[iv->cls];
        if(pax_lst.empty())
            body.push_back("NIL");
        else
            for(TPILPaxLst::const_iterator i_lst = pax_lst.begin(); i_lst != pax_lst.end(); i_lst++) {
                vector<string> seat_list = i_lst->seat_no.get_seat_vector(info.is_lat());
                ostringstream pax_str;
                pax_str
                    << info.TlgElemIdToElem(etAirp, i_lst->airp_arv)
                    << " "
                    << i_lst->name.ToPILTlg(info);
                string ssr_str = i_lst->ssr.ToPILTlg(info);
                if(not ssr_str.empty())
                    pax_str << " " << ssr_str;
                for(vector<string>::iterator i_seats = seat_list.begin(); i_seats != seat_list.end(); i_seats++) {
                    ostringstream buf;
                    buf << setw(3) << setfill('0') << *i_seats;
                    if(i_seats == seat_list.begin())
                        buf << " " << pax_str.str();
                    body.push_back(buf.str());
                }
            }
    }
}

int PIL(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading;
    heading << "PIL" << TypeB::endl
            << info.flight_view() << "/"
            << info.scd_local_view() << " " << info.airp_dep_view() << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
    size_t part_len = tlg_row.textSize();
    vector<string> body;
    try {
        TPIL pil;
        pil.get(info);
        pil.ToTlg(info, body);
    } catch(...) {
        ExceptionFilter(tlg_row.body, info);
    }
    simple_split(heading, part_len, tlg_draft, tlg_row, body);
    tlg_row.ending = "ENDPIL" + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

struct TTPMItem {
    int pax_id, grp_id;
    TName name;
    TTPMItem():
        pax_id(NoExists),
        grp_id(NoExists)
    {}
};

typedef vector<TTPMItem> TTPMItemList;

struct TTPM {
    TInfants infants;
    TTPMItemList items;
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

void TTPM::get(TypeB::TDetailCreateInfo &info)
{
    infants.get(info);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   pax.pax_id, "
        "   pax.grp_id, "
        "   pax.name, "
        "   pax.surname "
        "from "
        "   pax, "
        "   pax_grp "
        "where "
        "   pax_grp.point_dep = :point_id and "
        "   pax_grp.status NOT IN ('E') and "
        "   pax_grp.grp_id = pax.grp_id and "
        "   pax.refuse is null and "
        "   pax.pr_brd = 1 and "
        "   pax.seats > 0 "
        "order by "
        "   pax.surname, "
        "   pax.name ";
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next()) {
        TTPMItem item;
        item.pax_id = Qry.FieldAsInteger("pax_id");
        item.grp_id = Qry.FieldAsInteger("grp_id");
        item.name.name = Qry.FieldAsString("name");
        item.name.surname = Qry.FieldAsString("surname");
        items.push_back(item);
    }
}

void TTPM::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    for(TTPMItemList::iterator iv = items.begin(); iv != items.end(); iv++) {
        int inf_count = 0;
        ostringstream buf, buf2;
        for(vector<TInfantsItem>::iterator infRow = infants.items.begin(); infRow != infants.items.end(); infRow++) {
            if(infRow->grp_id == iv->grp_id and infRow->parent_pax_id == iv->pax_id) {
                inf_count++;
                if(!infRow->name.empty())
                    buf << "/" << transliter(infRow->name, 1, info.is_lat());
            }
        }
        if(inf_count > 0)
            buf2 << " " << inf_count << "INF" << buf.str();

        iv->name.ToTlg(info, body, buf2.str());
    }
}

int TPM(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading;
    heading << "TPM" << TypeB::endl
            << info.flight_view() << "/"
            << info.scd_local_view() << " " << info.airp_dep_view() << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
    size_t part_len = tlg_row.textSize();
    vector<string> body;
    try {
        TTPM tpm;
        tpm.get(info);
        tpm.ToTlg(info, body);
    } catch(...) {
        ExceptionFilter(body, info);
    }
    simple_split(heading, part_len, tlg_draft, tlg_row, body);
    tlg_row.ending = "ENDTPM" + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

int PSM(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading;
    heading << "PSM" << TypeB::endl
            << info.flight_view() << "/"
            << info.scd_local_view() << " " << info.airp_dep_view() << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
    size_t part_len = tlg_row.textSize();
    vector<string> body;
    try {
        TPSM psm;
        psm.get(info);
        psm.ToTlg(info, body);
    } catch(...) {
        ExceptionFilter(body, info);
    }
    simple_split(heading, part_len, tlg_draft, tlg_row, body);
    tlg_row.ending = "ENDPSM" + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

int BTM(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading1, heading2;
    heading1 << "BTM" << TypeB::endl
             << ".V/1T" << info.airp_trfer_view();
    heading2 << ".I/"
             << info.flight_view() << "/"
             << info.scd_local_view() << "/" << info.airp_dep_view() << TypeB::endl;
    tlg_row.heading = heading1.str() + "/PART" + IntToString(tlg_row.num) + TypeB::endl + heading2.str();
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
    size_t part_len = tlg_row.textSize();

    vector<string> body;
    try {
        TFList<TBTMFItem> FList;
        FList.get(info);
        FList.ToTlg(info, body);
    } catch(...) {
        ExceptionFilter(body, info);
    }
    string part_begin;
    bool P_found = false;
    for(vector<string>::iterator iv = body.begin(); iv != body.end(); iv++) {
        if(iv->find(".F/") == 0)
            part_begin = *iv;
        if(not P_found and iv->find(".P/") == 0)
            P_found = true;
        int grp_len = 0;
        if(
                P_found and
                (iv->find(".N") == 0 or iv->find(".F") == 0)
          ) { // ??諨 ????? ??㯯?
            P_found = false;
            grp_len = calculate_btm_grp_len(iv, body);
        } else
              grp_len = iv->size() + TypeB::endl.size();
        if(part_len + grp_len <= PART_SIZE)
            grp_len = iv->size() + TypeB::endl.size();
        part_len += grp_len;
        if(part_len > PART_SIZE) {
            tlg_draft.Save(tlg_row);
            tlg_row.heading = heading1.str() + "/PART" + IntToString(tlg_row.num) + TypeB::endl + heading2.str();
            tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
            if(iv->find(".F") == 0)
                tlg_row.body = *iv + TypeB::endl;
            else
                tlg_row.body = part_begin + TypeB::endl + *iv + TypeB::endl;
            part_len = tlg_row.textSize();
        } else
            tlg_row.body += *iv + TypeB::endl;
    }

    if(tlg_row.num == 1)
        tlg_row.heading = heading1.str() + TypeB::endl + heading2.str();
    tlg_row.ending = "ENDBTM" + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

int PTM(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading;
    heading << "PTM" << TypeB::endl
            << info.flight_view() << "/"
            << info.scd_local_view() << " " << info.airp_dep_view() << info.airp_trfer_view() << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
    size_t part_len = tlg_row.textSize();
    vector<string> body;
    try {
        TFList<TPTMFItem> FList;
        FList.get(info);
        FList.ToTlg(info, body);
    } catch(...) {
        ExceptionFilter(body, info);
    }
    simple_split(heading, part_len, tlg_draft, tlg_row, body);
    tlg_row.ending = "ENDPTM" + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

void TTlgPlace::dump()
{
    ostringstream buf;
    buf
        << "num: " << num << "; "
        << "y: " << y << "; "
        << "x: " << x << "; ";
    if(point_arv != NoExists)
        buf << "point_arv: " << point_arv << "; ";
    if(!xname.empty())
        buf << yname << xname;
    ProgTrace(TRACE5, buf.str().c_str());
}

void TSeatRectList::vert_pack()
{
    // ???????? ᯨ᮪ ???? ?? ᯨ᮪ ??㯯 ???? ? ????????묨 ??ਧ??⠫??묨 ????ࢠ????
    vector<TSeatRectList> split;
    for(TSeatRectList::iterator iv = begin(); iv != end(); iv++) {
        vector<TSeatRectList>::iterator i_split = split.begin();
        for(; i_split != split.end(); i_split++) {
            TSeatRect &curr_split_seat = (*i_split)[0];
            if(
                    curr_split_seat.row1 == iv->row1 and
                    curr_split_seat.row2 == iv->row2
              ) {
                i_split->push_back(*iv);
                break;
            }
        }
        if(i_split == split.end()) {
            TSeatRectList sub_list;
            sub_list.push_back(*iv);
            split.push_back(sub_list);
        }
    }

    for(vector<TSeatRectList>::iterator i_split = split.begin(); i_split != split.end(); i_split++) {
        if(i_split->size() > 1) {
            TSeatRectList::iterator first = i_split->begin();
            TSeatRectList::iterator cur = i_split->begin() + 1;
            while(true) {
                TSeatRectList::iterator prev = cur - 1;
                if(cur != i_split->end() and SeatNumber::prevIataLineOrEmptiness(cur->line1) == SeatNumber::tryNormalizeLine(prev->line1)) {
                    if (
                            prev->row1 == cur->row1 and
                            prev->row2 == cur->row2
                       ) {
                        prev->del = true;
                    } else {
                        prev->line1 = first->line1;
                        first = cur;
                    }
                } else {
                    prev->line1 = first->line1;
                    first = cur;
                    if(cur == i_split->end())
                        break;
                }
                cur++;
            }
        }
    }

    clear();
    for(vector<TSeatRectList>::iterator i_split = split.begin(); i_split != split.end(); i_split++)
        for(TSeatRectList::iterator j = i_split->begin(); j != i_split->end(); j++) {
            if(j->del)
                continue;
            push_back(*j);
        }
}

void TSeatRectList::pack()
{
    // ???????? ᯨ᮪ ???? ?? ᯨ᮪ ??㯯 ???? ? ????????묨 ??ਧ??⠫??묨 ????ࢠ????
    vector<TSeatRectList> split;
    for(TSeatRectList::iterator iv = begin(); iv != end(); iv++) {
        vector<TSeatRectList>::iterator i_split = split.begin();
        for(; i_split != split.end(); i_split++) {
            TSeatRect &curr_split_seat = (*i_split)[0];
            if(
                    curr_split_seat.line1 == iv->line1 and
                    curr_split_seat.line2 == iv->line2
              ) {
                i_split->push_back(*iv);
                break;
            }
        }
        if(i_split == split.end()) {
            TSeatRectList sub_list;
            sub_list.push_back(*iv);
            split.push_back(sub_list);
        }
    }

    for(vector<TSeatRectList>::iterator i_split = split.begin(); i_split != split.end(); i_split++) {
        if(i_split->size() > 1) {
            TSeatRectList::iterator first = i_split->begin();
            TSeatRectList::iterator cur = i_split->begin() + 1;
            while(true) {
                TSeatRectList::iterator prev = cur - 1;
                if(cur != i_split->end() and SeatNumber::normalizePrevIataRowOrException(cur->row1) == SeatNumber::tryNormalizeRow(prev->row1)) {
                    if (
                            prev->line1 == cur->line1 and
                            prev->line2 == cur->line2
                       ) {
                        prev->del = true;
                    } else {
                        prev->row1 = first->row1;
                        first = cur;
                    }
                } else {
                    prev->row1 = first->row1;
                    first = cur;
                    if(cur == i_split->end())
                        break;
                }
                cur++;
            }
        }
    }
    clear();
    // ?????뢠?? ????祭??? ??????? ???? ? १??????.
    for(vector<TSeatRectList>::iterator i_split = split.begin(); i_split != split.end(); i_split++)
        for(TSeatRectList::iterator j = i_split->begin(); j != i_split->end(); j++) {
            if(j->del)
                continue;
            push_back(*j);
        }
}

string TSeatRectList::ToTlg()
{
    std::string result;
    // ?????뢠?? ? result,
    // ????⭮ ?饬 ????????? ?????, ?⮡?
    // ???஡????? ???????? ?? ? ???⪮? ??ଥ ????. 3AB?
    // ?????㫨?㥬 ⠪?? ????? ? ??????? alone
    vector<TSeatRectList> alone;
    for(TSeatRectList::iterator iv = begin(); iv != end(); iv++) {
        if(iv->del)
            continue;
        if(
                iv->row1 == iv->row2 and
                iv->line1 == iv->line2
          ) {
            vector<TSeatRectList>::iterator i_alone = alone.begin();
            for(; i_alone != alone.end(); i_alone++) {
                TSeatRect &curr_alone_seat = (*i_alone)[0];
                if(curr_alone_seat.row1 == iv->row1) {
                    i_alone->push_back(*iv);
                    break;
                }
            }
            if(i_alone == alone.end()) {
                TSeatRectList sub_list;
                sub_list.push_back(*iv);
                alone.push_back(sub_list);
            }
            iv->del = true;
        }
        else
        {
            if(!result.empty())
                result += " ";
            result += iv->str();
        }
    }
    for(vector<TSeatRectList>::iterator i_alone = alone.begin(); i_alone != alone.end(); i_alone++) {
        if(!result.empty())
            result += " ";
        if(i_alone->size() == 1) {
            result += (*i_alone)[0].str();
        } else {
            for(TSeatRectList::iterator sr = i_alone->begin(); sr != i_alone->end(); sr++) {
                if(sr == i_alone->begin())
                    result += sr->row1;
                result += sr->line1;
            }
        }
    }
    return result;
}

string TSeatRect::str()
{
    string result;
    if(row1 == row2) {
        if(line1 == line2)
            result = row1 + line1;
        else {
            if(line1 == SeatNumber::prevIataLineOrEmptiness(line2))
                result = row1 + line1 + line2;
            else
                result = row1 + line1 + "-" + line2;
        }
    } else {
        if(line1 == line2)
            result = row1 + "-" + row2 + line1;
        else {
            result = row1 + "-" + row2 + line1 + "-" + line2;
        }
    }
    return result;
}

struct TSeatListContext {
    string first_xname, last_xname;
    void seat_to_str(TSeatRectList &SeatRectList, string yname, string first_place,  string last_place, bool pr_lat);
    void vert_seat_to_str(TSeatRectList &SeatRectList, string yname, string first_place,  string last_place, bool pr_lat);
};

void TTlgSeatList::add_seats(int pax_id, std::vector<TTlgCompLayer> &complayers)
{
    for (vector<TTlgCompLayer>::iterator ic=complayers.begin(); ic!=complayers.end(); ic++ ) {
        if ( ic->pax_id != pax_id ) continue;
        add_seat(ic->xname, ic->yname);
    }
}

void TTlgSeatList::add_seat(int point_id, string xname, string yname)
{
    if(SeatNumber::isIataRow(yname) && SeatNumber::isIataLine(xname)) {
        TTlgPlace place;
        place.xname = SeatNumber::tryNormalizeLine(xname);
        place.yname = SeatNumber::tryNormalizeRow(yname);
        place.point_arv = point_id;
        comp[place.yname][place.xname] = place;
    }
}

void TTlgSeatList::dump_list(std::map<int, TSeatRectList> &list)
{
    for(std::map<int, TSeatRectList>::iterator im = list.begin(); im != list.end(); im++) {
        string result;
        TSeatRectList &SeatRectList = im->second;
        for(TSeatRectList::iterator iv = SeatRectList.begin(); iv != SeatRectList.end(); iv++) {
            if(iv->del) continue;
            result += iv->str() + " ";
        }
        ProgTrace(TRACE5, "point_arv: %d; SeatRectList: %s", im->first, result.c_str());
    }
}

void TTlgSeatList::dump_list(map<int, string> &list)
{
    for(map<int, string>::iterator im = list.begin(); im != list.end(); im++) {
        ProgTrace(TRACE5, "point_arv: %d; seats: %s", im->first, (/*convert_seat_no(*/im->second/*, 1)*/).c_str());
    }
}

vector<string>  TTlgSeatList::get_seat_vector(bool pr_lat) const
{
    vector<string> result;
    for(t_tlg_comp::const_iterator ay = comp.begin(); ay != comp.end(); ay++) {
        const t_tlg_row &row = ay->second;
        for(t_tlg_row::const_iterator ax = row.begin(); ax != row.end(); ax++)
            result.push_back(SeatNumber::tryDenormalizeRow(ay->first) + SeatNumber::tryDenormalizeLine(ax->first, pr_lat));
    }
    return result;
}

string  TTlgSeatList::get_seat_one(bool pr_lat) const
{
    string result;
    if(!comp.empty()) {
        t_tlg_comp::const_iterator ay = comp.begin();
        t_tlg_row::const_iterator ax = ay->second.begin();
        result = SeatNumber::tryDenormalizeRow(ay->first) + SeatNumber::tryDenormalizeLine(ax->first, pr_lat);
    }
    return result;
}

string  TTlgSeatList::get_seat_list(bool pr_lat)
{
    map<int, string> list;
    get_seat_list(list, pr_lat);
    if(list.size() > 1)
        throw Exception("TTlgSeatList::get_seat_list(): wrong map size %zu", list.size());
    return list[0];
}

int TTlgSeatList::get_list_size(std::map<int, std::string> &list)
{
    int result = 0;
    for(map<int, std::string>::iterator im = list.begin(); im != list.end(); im++)
        result += im->second.size();
    return result;
}

void TTlgSeatList::get_seat_list(map<int, string> &list, bool pr_lat)
{
    list.clear();
    map<int, TSeatRectList> hrz_list, vert_list;
    // ?஡?? ????? ???? ?? ??ਧ??⠫?
    // ??।?????? ???????쭮? ? ???ᨬ??쭮? ???न???? ????? ? ??????? ????
    // ??????? ????? (?ᯮ???????? ??? ??᫥???饣? ???⨪??쭮?? ?஡???)
    string min_col, max_col;
    for(t_tlg_comp::iterator ay = comp.begin(); ay != comp.end(); ay++) {
        map<int, TSeatListContext> ctxt;
        string *first_xname = NULL;
        string *last_xname = NULL;
        TSeatRectList *SeatRectList = NULL;
        TSeatListContext *cur_ctxt = NULL;
        t_tlg_row &row = ay->second;
        if(min_col.empty() or SeatNumber::lessIataLine(row.begin()->first, min_col))
            min_col = row.begin()->first;
        if(max_col.empty() or not SeatNumber::lessIataLine(row.rbegin()->first, max_col))
            max_col = row.rbegin()->first;
        for(t_tlg_row::iterator ax = row.begin(); ax != row.end(); ax++) {
            cur_ctxt = &ctxt[ax->second.point_arv];
            first_xname = &cur_ctxt->first_xname;
            last_xname = &cur_ctxt->last_xname;
            SeatRectList = &hrz_list[ax->second.point_arv];
            if(first_xname->empty()) {
                *first_xname = ax->first;
                *last_xname = *first_xname;
            } else {
                if(SeatNumber::prevIataLineOrEmptiness(ax->first) == *last_xname)
                    *last_xname = ax->first;
                else {
                    cur_ctxt->seat_to_str(*SeatRectList, ax->second.yname, *first_xname, *last_xname, pr_lat);
                    *first_xname = ax->first;
                    *last_xname = *first_xname;
                }
            }
        }
        // ?????뢠?? ??᫥???? ??⠢訥?? ????? ? ???? ??? ??????? ???ࠢ?????
        // Put last row seats for each dest hrz_list
        for(map<int, TSeatRectList>::iterator im = hrz_list.begin(); im != hrz_list.end(); im++) {
            cur_ctxt = &ctxt[im->first];
            first_xname = &cur_ctxt->first_xname;
            last_xname = &cur_ctxt->last_xname;
            SeatRectList = &im->second;
            if(first_xname != NULL and !first_xname->empty())
                cur_ctxt->seat_to_str(*SeatRectList, ay->first, *first_xname, *last_xname, pr_lat);
        }
    }

    // ?஡?? ????? ???? ?? ???⨪???
    string i_col = min_col;
    while(true) {
        map<int, TSeatListContext> ctxt;
        string *first_xname = NULL;
        string *last_xname = NULL;
        TSeatRectList *SeatRectList = NULL;
        TSeatListContext *cur_ctxt = NULL;
        for(t_tlg_comp::iterator i_comp = comp.begin(); i_comp != comp.end(); i_comp++) {
            t_tlg_row &row = i_comp->second;
            t_tlg_row::iterator col_pos = row.find(i_col);
            if(col_pos != row.end()) {
                cur_ctxt = &ctxt[col_pos->second.point_arv];
                first_xname = &cur_ctxt->first_xname;
                last_xname = &cur_ctxt->last_xname;
                SeatRectList = &vert_list[col_pos->second.point_arv];
                if(first_xname->empty()) {
                    *first_xname = col_pos->second.yname;
                    *last_xname = *first_xname;
                } else {
                    if(SeatNumber::normalizePrevIataRowOrException(col_pos->second.yname) == *last_xname)
                        *last_xname = col_pos->second.yname;
                    else {
                        cur_ctxt->vert_seat_to_str(*SeatRectList, col_pos->first, *first_xname, *last_xname, pr_lat);
                        *first_xname = col_pos->second.yname;
                        *last_xname = *first_xname;
                    }
                }
            }
        }
        // ?????뢠?? ??᫥???? ??⠢訥?? ????? ? ???? ??? ??????? ???ࠢ?????
        // Put last row seats for each dest vert_list
        for(map<int, TSeatRectList>::iterator im = vert_list.begin(); im != vert_list.end(); im++) {
            cur_ctxt = &ctxt[im->first];
            first_xname = &cur_ctxt->first_xname;
            last_xname = &cur_ctxt->last_xname;
            SeatRectList = &im->second;
            if(first_xname != NULL and !first_xname->empty())
                cur_ctxt->vert_seat_to_str(*SeatRectList, i_col, *first_xname, *last_xname, pr_lat);
        }
        if(i_col == max_col)
            break;
        i_col = SeatNumber::nextIataLineOrEmptiness(i_col);
    }
    map<int, TSeatRectList>::iterator i_hrz = hrz_list.begin();
    map<int, TSeatRectList>::iterator i_vert = vert_list.begin();
    while(true) {
        if(i_hrz == hrz_list.end())
            break;
        i_hrz->second.pack();
        i_vert->second.vert_pack();
        string hrz_result = i_hrz->second.ToTlg();
        string vert_result = i_vert->second.ToTlg();
        if(hrz_result.size() > vert_result.size())
            list[i_vert->first] = vert_result;
        else
            list[i_hrz->first] = hrz_result;
        i_hrz++;
        i_vert++;
    }
}

void TSeatListContext::vert_seat_to_str(TSeatRectList &SeatRectList, string yname, string first_xname, string last_xname, bool pr_lat)
{
    //????? x ? y ??९?⠭. ?????? ?? ??२???????? yname->xname, first_xname->first_yname, last_xname->last_yname
    yname = SeatNumber::tryDenormalizeLine(yname, pr_lat);
    first_xname = SeatNumber::tryDenormalizeRow(first_xname);
    last_xname = SeatNumber::tryDenormalizeRow(last_xname);
    TSeatRect rect;
    rect.row1 = first_xname;
    rect.line1 = yname;
    rect.line2 = yname;
    if(first_xname == last_xname)
        rect.row2 = first_xname;
    else
        rect.row2 = last_xname;
    SeatRectList.push_back(rect);
}

void TSeatListContext::seat_to_str(TSeatRectList &SeatRectList, string yname, string first_xname, string last_xname, bool pr_lat)
{
    yname = SeatNumber::tryDenormalizeRow(yname);
    first_xname = SeatNumber::tryDenormalizeLine(first_xname, pr_lat);
    last_xname = SeatNumber::tryDenormalizeLine(last_xname, pr_lat);
    TSeatRect rect;
    rect.row1 = yname;
    rect.row2 = yname;
    rect.line1 = first_xname;
    if(first_xname == last_xname)
        rect.line2 = first_xname;
    else
        rect.line2 = last_xname;
    SeatRectList.push_back(rect);
}

void TTlgSeatList::dump_comp() const
{
    for(t_tlg_comp::const_iterator ay = comp.begin(); ay != comp.end(); ay++)
        for(t_tlg_row::const_iterator ax = ay->second.begin(); ax != ay->second.end(); ax++) {
            ostringstream buf;
            buf
                << "yname: " << ay->first << "; "
                << "xname: " << ax->first << "; ";
            if(!ax->second.yname.empty())
                buf << ax->second.yname << ax->second.xname << " " << ax->second.point_arv;
            ProgTrace(TRACE5, "%s", buf.str().c_str());
        }
}

void TTlgSeatList::apply_comp(TypeB::TDetailCreateInfo &info, bool pr_blocked = false)
{

  TTlgCompLayerList complayers;
  getSalonLayers( info, complayers, pr_blocked );
  for ( vector<TTlgCompLayer>::iterator il=complayers.begin(); il!=complayers.end(); il++ ) {
    add_seat( il->point_arv, il->xname, il->yname );
  }
}

void TTlgSeatList::get(TypeB::TDetailCreateInfo &info)
{
    apply_comp(info);
    map<int, string> list;
    get_seat_list(list, (info.is_lat() or info.pr_lat_seat));
    // finally we got map with key - point_arv, data - string represents seat list for given point_arv
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "  SELECT point_id, airp FROM points "
        "  WHERE first_point = :vfirst_point AND point_num > :vpoint_num AND pr_del=0 "
        "ORDER by "
        "  point_num ";
    Qry.CreateVariable("vfirst_point", otInteger, info.pr_tranzit ? info.first_point : info.point_id);
    Qry.CreateVariable("vpoint_num", otInteger, info.point_num);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next()) {
        string item;
        int point_id = Qry.FieldAsInteger("point_id");
        string airp = Qry.FieldAsString("airp");
        item = "-" + info.TlgElemIdToElem(etAirp, airp) + ".";
        if(list[point_id].empty())
            item += "NIL";
        else {
            item += /*convert_seat_no(*/list[point_id]/*, info.is_lat())*/;
            while(item.size() + 1 > LINE_SIZE) {
                size_t pos = item.rfind(' ', LINE_SIZE - 2);
                items.push_back(item.substr(0, pos));
                item = item.substr(pos + 1);
            }
        }
        items.push_back(item);
    }

    TTlgSeatList blocked_seats;
    blocked_seats.apply_comp(info, true);
    blocked_seats.get_seat_list(list, (info.is_lat() or info.pr_lat_seat));
    if(not list.empty()) {
        items.push_back("SI");
        string item = "BLOCKED SEATS: ";
        item += list.begin()->second;
        while(item.size() + 1 > LINE_SIZE) {
            size_t pos = item.rfind(' ', LINE_SIZE - 2);
            items.push_back(item.substr(0, pos));
            item = item.substr(pos + 1);
        }
        items.push_back(item);
    }
}

int SOM(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading;
    heading << "SOM" << TypeB::endl
            << info.flight_view() << "/"
            << info.scd_local_view() << " " << info.airp_dep_view() << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
    size_t part_len = tlg_row.textSize();
    TTlgSeatList SOMList;
    try {
        if(isFreeSeating(info.point_id))
            throw UserException("MSG.SALONS.FREE_SEATING");
        if(isEmptySalons(info.point_id))
            throw UserException("MSG.FLIGHT_WO_CRAFT_CONFIGURE");
        SOMList.get(info);
    } catch(...) {
        ExceptionFilter(SOMList.items, info);
    }
    for(vector<string>::iterator iv = SOMList.items.begin(); iv != SOMList.items.end(); iv++) {
        part_len += iv->size() + TypeB::endl.size();
        if(part_len > PART_SIZE) {
            tlg_draft.Save(tlg_row);
            tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
            tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
            tlg_row.body = *iv + TypeB::endl;
            part_len = tlg_row.textSize();
        } else
            tlg_row.body += *iv + TypeB::endl;
    }
    tlg_row.ending = "ENDSOM" + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

string TName::ToPILTlg(TypeB::TDetailCreateInfo &info) const
{
    string result = transliter(surname, 1, info.is_lat());
    if(not name.empty())
        result += "/" + transliter(name, 1, info.is_lat());
    return result;

}

void TName::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body, string postfix)
{
    name = transliter(name, 1, info.is_lat());
    surname = transliter(surname, 1, info.is_lat());
    /*name = "???";
    surname = "??? ???";*/
    if(postfix.size() > (LINE_SIZE - sizeof("1X/X ")))
        throw Exception("TName::ToTlg: postfix too long %s", postfix.c_str());
    size_t name_size = LINE_SIZE - postfix.size();
    string result;
    string one_surname = "1" + surname;
    string pax_name = one_surname;
    if(!name.empty())
        pax_name += "/" + name;
    if(pax_name.size() > name_size){
        size_t diff = pax_name.size() - name_size;
        if(name.empty()) {
            result = one_surname.substr(0, one_surname.size() - diff);
        } else {
            if(name.size() > diff) {
                name = name.substr(0, name.size() - diff);
            } else {
                diff -= name.size() - 1;
                name = name[0];
                one_surname = one_surname.substr(0, one_surname.size() - diff);
            }
            result = one_surname + "/" + name;
        }
    } else
        result = pax_name;
    result += postfix;
    body.push_back(result);
}

typedef map<int, CheckIn::TServicePaymentListWithAuto> TGrpEmds;

struct TASLPax {
    string target;
    int cls_grp_id;
    TName name;
    int pnr_id;
    string crs;
    int pax_id;
    int reg_no; // used in EMD report
    string ticket_no;
    int coupon_no;
    int grp_id;
    int bag_pool_num;
    TPNRListAddressee pnrs;
    TMItem M;
    TRemList rems;
    TGrpEmds *grpEmds;

    // ??????騥 2 ???? ??????????? ? rems.get()
    // ???祬 ?᫨ ? १??????? used_asvc ???⮩,
    // ?? tkn ⮦? ?㤥? ???⮩.
    // ???? ??? EMDReport
    vector<CheckIn::TPaxASVCItem> used_asvc;
    CheckIn::TPaxTknItem tkn;

    TASLPax(TInfants *ainfants, TGrpEmds *agrpEmds): rems(ainfants), grpEmds(agrpEmds) {
        cls_grp_id = NoExists;
        pnr_id = NoExists;
        pax_id = NoExists;
        reg_no = NoExists;
        grp_id = NoExists;
    }
};

struct TETLPax {
    string target;
    int cls_grp_id;
    TName name;
    int pnr_id;
    string crs;
    int pax_id;
    string ticket_no;
    int coupon_no;
    int grp_id;
    int bag_pool_num;
    TPNRListAddressee pnrs;
    TMItem M;
    TRemList rems;
    TETLPax(TInfants *ainfants): rems(ainfants) {
        cls_grp_id = NoExists;
        pnr_id = NoExists;
        pax_id = NoExists;
        grp_id = NoExists;
        bag_pool_num = NoExists;
    }
};

struct TTPLPax {
    string target;
    int cls_grp_id;
    TName name;
    int pnr_id;
    string crs;
    int pax_id;
    int grp_id;
    int bag_pool_num;
    TPNRListAddressee pnrs;
    TRemList rems;
    TTPLPax(TInfants *ainfants): rems(ainfants) {
        cls_grp_id = NoExists;
        pnr_id = NoExists;
        pax_id = NoExists;
        grp_id = NoExists;
        bag_pool_num = NoExists;
    }
};

void TRemList::get(TypeB::TDetailCreateInfo &info, TASLPax &pax)
{
    CheckIn::TPaxRemItem rem;

    TPaxEMDList emds;
    emds.getPaxEMD(pax.pax_id, PaxASVCList::allByPaxId);

    CheckIn::TServicePaymentListWithAuto &payment = (*pax.grpEmds)[pax.grp_id];

    for(CheckIn::TServicePaymentListWithAuto::iterator p = payment.begin(); p != payment.end(); ++p)
    {
      if (!p->isEMD()) continue; //??ࠡ??뢠?? ⮫쪮 EMD
      for(multiset<TPaxEMDItem>::iterator e = emds.begin(); e != emds.end(); ++e)
        if (p->doc_no==e->emd_no &&
            p->doc_coupon==e->emd_coupon) pax.used_asvc.push_back(*e);
    }

    if(not pax.used_asvc.empty()) {
        //?????
        CheckIn::TPaxTknItem tkn;
        LoadPaxTkn(pax.pax_id, tkn);
        if (tkn.rem == "TKNE" and getPaxRem(info, tkn, false, rem)) {
            pax.tkn = tkn;
            items.push_back(rem.text);
        }
        for(vector<TInfantsItem>::iterator infRow = infants->items.begin(); infRow != infants->items.end(); infRow++) {
            if(infRow->grp_id == pax.grp_id and infRow->parent_pax_id == pax.pax_id) {
                LoadPaxTkn(infRow->pax_id, tkn);
                if (tkn.rem == "TKNE" and getPaxRem(info, tkn, true, rem)) items.push_back(rem.text);
            }
        }
        for(vector<CheckIn::TPaxASVCItem>::const_iterator asvc = pax.used_asvc.begin(); asvc != pax.used_asvc.end(); ++asvc)
          if (getPaxRem(info, *asvc, false, rem)) items.push_back(rem.text);
    }
}

void TRemList::get(TypeB::TDetailCreateInfo &info, TTPLPax &pax)
{
    multiset<CheckIn::TPaxRemItem> rems;
    LoadPaxRem(pax.pax_id, rems);
    for(multiset<CheckIn::TPaxRemItem>::iterator i = rems.begin(); i != rems.end(); i++) {
        if(i->code == "ETLP")
            items.push_back(transliter(convert_char_view(i->text, info.is_lat()), 1, info.is_lat()));
    }
}

void TRemList::get(TypeB::TDetailCreateInfo &info, TETLPax &pax)
{
    CheckIn::TPaxRemItem rem;
    //?????
    CheckIn::TPaxTknItem tkn;
    LoadPaxTkn(pax.pax_id, tkn);
    if (tkn.rem == "TKNE" and getPaxRem(info, tkn, false, rem)) items.push_back(rem.text);
    for(vector<TInfantsItem>::iterator infRow = infants->items.begin(); infRow != infants->items.end(); infRow++) {
        if(infRow->grp_id == pax.grp_id and infRow->parent_pax_id == pax.pax_id) {
            LoadPaxTkn(infRow->pax_id, tkn);
            if (tkn.rem == "TKNE" and getPaxRem(info, tkn, true, rem)) items.push_back(rem.text);
        }
    }
}

struct TFTLDest;
struct TFTLPax {
    TName name;
    int pnr_id;
    string crs;
    int pax_id;
    TMItem M;
    TPNRListAddressee pnrs;
    TRemList rems;
    TFTLDest *destInfo;
    TFTLPax(TFTLDest *aDestInfo): rems(NULL) {
        pnr_id = NoExists;
        pax_id = NoExists;
        destInfo = aDestInfo;
    }
};

struct TFTLDest {
    string target;
    string subcls;
    vector<TFTLPax> PaxList;
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

bool getPaxRem(TypeB::TDetailCreateInfo &info, const CheckIn::TPaxRemBasic &basic, bool inf_indicator, CheckIn::TPaxRemItem &rem)
{
  if (basic.empty()) return false;
  rem=CheckIn::TPaxRemItem(basic, inf_indicator, info.is_lat()?AstraLocale::LANG_EN:AstraLocale::LANG_RU, applyLangForAll);
  return true;
}

void TRemList::internal_get(TypeB::TDetailCreateInfo &info, int pax_id, string subcls)
{
    QParams QryParams;
    QryParams << QParam("pax_id", otInteger, pax_id);
    TCachedQuery Qry(
        "select "
        "   pax_fqt.rem_code, "
        "   pax_fqt.airline, "
        "   pax_fqt.no, "
        "   pax_fqt.extra, "
        "   crs_pnr.subclass "
        "from "
        "   pax_fqt, "
        "   crs_pax, "
        "   crs_pnr "
        "where "
        "   pax_fqt.pax_id = :pax_id and "
        "   pax_fqt.pax_id = crs_pax.pax_id(+) and "
        "   crs_pax.pr_del(+)=0 and "
        "   crs_pax.pnr_id = crs_pnr.pnr_id(+) and "
        "   crs_pnr.system(+) = 'CRS' and "
        "   pax_fqt.rem_code in('FQTV', 'FQTU', 'FQTR') ",
            QryParams);
    Qry.get().Execute();
    if(!Qry.get().Eof) {
        int col_rem_code = Qry.get().FieldIndex("rem_code");
        int col_airline = Qry.get().FieldIndex("airline");
        int col_no = Qry.get().FieldIndex("no");
        int col_extra = Qry.get().FieldIndex("extra");
        int col_subclass = Qry.get().FieldIndex("subclass");
        for(; !Qry.get().Eof; Qry.get().Next()) {
            string item;
            string rem_code = Qry.get().FieldAsString(col_rem_code);
            string airline = Qry.get().FieldAsString(col_airline);
            string no = Qry.get().FieldAsString(col_no);
            string extra = Qry.get().FieldAsString(col_extra);
            string subclass = Qry.get().FieldAsString(col_subclass);
            item +=
                rem_code + " " +
                info.TlgElemIdToElem(etAirline, airline) + " " +
                transliter(no, 1, info.is_lat());
            if(rem_code == "FQTV") {
                if(not subclass.empty() and subclass != subcls)
                    item += "-" + info.TlgElemIdToElem(etSubcls, subclass);
            } else {
                if(not extra.empty())
                    item += "-" + transliter(extra, 1, info.is_lat());
            }
            items.push_back(item);
        }
    }
}

void TRemList::get(TypeB::TDetailCreateInfo &info, TFTLPax &pax)
{
    internal_get(info, pax.pax_id, pax.destInfo->subcls);
}


void TFTLDest::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    const TypeB::TMarkInfoOptions &markOptions=*(info.optionsAs<TypeB::TMarkInfoOptions>());
    for(vector<TFTLPax>::iterator iv = PaxList.begin(); iv != PaxList.end(); iv++) {
        iv->name.ToTlg(info, body);
        iv->pnrs.ToTlg(info, body);
        if(markOptions.mark_info.empty())
            iv->M.ToTlg(info, body);
        iv->rems.ToTlg(info, body);
    }
}

struct TPIMPax {
    int pax_id;
    string name;
    string surname;
    TPIMPax(int apax_id, string aname, string asurname):
        pax_id(apax_id),
        name(aname),
        surname(asurname)
    {};
};

struct TPIMDest {
    string target;
    vector<TPIMPax> PaxList;
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

void TPIMDest::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    CheckIn::TPaxDocItem doc;
    vector<string> pax_body;
    for(vector<TPIMPax>::iterator iv = PaxList.begin(); iv != PaxList.end(); iv++) {
        LoadPaxDoc(iv->pax_id, doc);
        string vsurname, vname;
        if(doc.surname.empty()) {
            vname = transliter(iv->name, 1, info.is_lat());
            vsurname = transliter(iv->surname, 1, info.is_lat());
        } else {
            vname = transliter(doc.first_name, 1, info.is_lat());
            if(not vname.empty() and not doc.second_name.empty())
                vname += " ";
            vname += transliter(doc.second_name, 1, info.is_lat());
            vsurname = transliter(doc.surname, 1, info.is_lat());
        }
        string line;
        line =
            vsurname
            + "/" + vname
            + "/" + (doc.type.empty()?"":info.TlgElemIdToElem(etPaxDocType, doc.type))
            + "/" + transliter(convert_char_view(doc.no, info.is_lat()), 1, info.is_lat())
            + "/" + (doc.nationality.empty()?"":info.TlgElemIdToElem(etPaxDocCountry, doc.nationality))
            + "/" + (doc.birth_date!=ASTRA::NoExists?DateTimeToStr(doc.birth_date, "ddmmmyy", info.is_lat()):"")
            + "/" + (doc.gender.empty()?"":info.TlgElemIdToElem(etGenderType, doc.gender)) //?? ????? inf_indicator, ???? ?㤥? - ????????
            + "/" + (doc.expiry_date!=ASTRA::NoExists?DateTimeToStr(doc.expiry_date, "ddmmmyy", info.is_lat()):"")
            + "/" + (doc.issue_country.empty()?"":info.TlgElemIdToElem(etPaxDocCountry, doc.issue_country))
            + "/";
        while(line.size() > LINE_SIZE) {
            body.push_back(line.substr(0, LINE_SIZE));
            line.erase(0, LINE_SIZE);
            if(not line.empty())
                line = ".RN/" + line;
        }
        if(not line.empty())
            body.push_back(line);
    }
}

struct TPIMBody {
    vector<TPIMDest> items;
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

struct TFTLBody {
    vector<TFTLDest> items;
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

void TPIMBody::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    if(items.empty()) {
        body.clear();
        body.push_back("NIL");
    } else
        for(vector<TPIMDest>::iterator iv = items.begin(); iv != items.end(); iv++) {
            ostringstream buf;
            buf << "-" << info.TlgElemIdToElem(etAirp, iv->target);
            body.push_back(buf.str());
            iv->ToTlg(info, body);
        }
}

void TFTLBody::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    if(items.empty()) {
        body.clear();
        body.push_back("NIL");
    } else
        for(vector<TFTLDest>::iterator iv = items.begin(); iv != items.end(); iv++) {
            ostringstream buf;
            buf
                << "-" << info.TlgElemIdToElem(etAirp, iv->target)
                << setw(2) << setfill('0') << iv->PaxList.size()
                << info.TlgElemIdToElem(etSubcls, iv->subcls);
            body.push_back(buf.str());
            iv->ToTlg(info, body);
        }
}

void TPIMBody::get(TypeB::TDetailCreateInfo &info)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT "
        "    pax_grp.airp_arv target, "
        "    pax.pax_id, "
        "    pax.name, "
        "    pax.surname "
        "FROM "
        "    pax_grp, "
        "    pax "
        "WHERE "
        "    pax_grp.grp_id=pax.grp_id AND "
        "    pax_grp.point_dep=:point_id AND "
        "    pax_grp.status NOT IN ('E') AND "
        "    pr_brd = 1"
        "ORDER BY "
        "    pax_grp.airp_arv, "
        "    pax.surname, "
        "    pax.name ";
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.Execute();
    if(!Qry.Eof) {
        int col_target = Qry.FieldIndex("target");
        int col_pax_id = Qry.FieldIndex("pax_id");
        int col_name = Qry.FieldIndex("name");
        int col_surname = Qry.FieldIndex("surname");
        TPIMDest *curr_dest = NULL;
        items.push_back(TPIMDest());
        curr_dest = &items.back();
        for(; !Qry.Eof; Qry.Next()) {
            string target = Qry.FieldAsString(col_target);
            if(curr_dest->target != target) {
                if(
                        not curr_dest->target.empty() and
                        not curr_dest->PaxList.empty()
                  ) {
                    items.push_back(TPIMDest());
                    curr_dest = &items.back();
                }
                curr_dest->target = target;
            }
            curr_dest->PaxList.push_back(
                    TPIMPax (
                        Qry.FieldAsInteger(col_pax_id),
                        Qry.FieldAsString(col_name),
                        Qry.FieldAsString(col_surname)
                        )
                    );
        }
    }
}

void TFTLBody::get(TypeB::TDetailCreateInfo &info)
{
    const TypeB::TMarkInfoOptions &markOptions=*(info.optionsAs<TypeB::TMarkInfoOptions>());

    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT "
        "    pax_grp.airp_arv target, "
        "    crs_pnr.pnr_id, "
        "    crs_pnr.sender crs, "
        "    pax.pax_id, "
        "    pax.surname, "
        "    pax.name, "
        "    nvl(nvl(pax.cabin_subclass, pax.subclass), nvl(pax.cabin_class, pax_grp.class)) subclass "
        "FROM "
        "    pax_grp, "
        "    pax, "
        "    crs_pnr, "
        "    crs_pax, "
        "    classes "
        "WHERE "
        "    pax_grp.grp_id=pax.grp_id AND "
        "    pax.pax_id=crs_pax.pax_id(+) AND "
        "    crs_pax.pr_del(+)=0 AND "
        "    crs_pax.pnr_id=crs_pnr.pnr_id(+) AND "
        "    crs_pnr.system(+) = 'CRS' and "
        "    nvl(pax.cabin_class, pax_grp.class)=classes.code AND "
        "    pax_grp.point_dep=:point_id AND "
        "    pax_grp.status NOT IN ('E') AND "
        "    pr_brd IS NOT NULL "
        "ORDER BY "
        "    pax_grp.airp_arv, "
        "    classes.priority, "
        "    nvl(nvl(pax.cabin_subclass, pax.subclass), nvl(pax.cabin_class, pax_grp.class)), "
        "    pax.surname, "
        "    pax.name ";
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.Execute();
    if(!Qry.Eof) {
        int col_target = Qry.FieldIndex("target");
        int col_pnr_id = Qry.FieldIndex("pnr_id");
        int col_pax_id = Qry.FieldIndex("pax_id");
        int col_crs = Qry.FieldIndex("crs");
        int col_surname = Qry.FieldIndex("surname");
        int col_name = Qry.FieldIndex("name");
        int col_subclass = Qry.FieldIndex("subclass");
        TFTLDest dest;
        TFTLDest *curr_dest = NULL;
        items.push_back(dest);
        curr_dest = &items.back();
        for(; !Qry.Eof; Qry.Next()) {
            string target = Qry.FieldAsString(col_target);
            string subcls = Qry.FieldAsString(col_subclass);
            if(curr_dest->target != target or curr_dest->subcls != subcls) {
                if(not curr_dest->target.empty()) {
                    if(not curr_dest->PaxList.empty()) {
                        items.push_back(dest);
                        curr_dest = &items.back();
                    }
                }
                curr_dest->target = target;
                curr_dest->subcls = subcls;
            }
            TFTLPax pax(curr_dest);
            pax.crs = Qry.FieldAsString(col_crs);
            if(not markOptions.crs.empty() and markOptions.crs != pax.crs)
                continue;
            pax.pax_id = Qry.FieldAsInteger(col_pax_id);
            pax.rems.get(info, pax);
            if(pax.rems.items.empty())
                continue;
            pax.M.get(info, pax.pax_id);
            if(not markOptions.mark_info.empty() and not(pax.M.m_flight == markOptions.mark_info))
                continue;
            if(!Qry.FieldIsNULL(col_pnr_id))
                pax.pnr_id = Qry.FieldAsInteger(col_pnr_id);
            pax.pnrs.get(pax.pnr_id);
            pax.name.surname = Qry.FieldAsString(col_surname);
            pax.name.name = Qry.FieldAsString(col_name);
            curr_dest->PaxList.push_back(pax);
        }
        if(curr_dest->PaxList.empty())
            items.pop_back();
    }
};

struct TASLDest {
    string airp;
    string cls;
    TGrpEmds grpEmds; // grouped by gpr_id
    vector<TASLPax> PaxList;
    TGRPMap *grp_map;
    TInfants *infants;
    TASLDest(TGRPMap *agrp_map, TInfants *ainfants) {
        grp_map = agrp_map;
        infants = ainfants;
    }
    virtual ~TASLDest() {}
    void GetPaxList(TypeB::TDetailCreateInfo &info, vector<TTlgCompLayer> &complayers);
    void PaxListToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
    virtual bool is_report() { return false; }
};

struct TASLReportDest:TASLDest {
    bool is_report() { return true; }
    TASLReportDest(TGRPMap *agrp_map, TInfants *ainfants):
        TASLDest(agrp_map, ainfants)
    {}
};

struct TTPLDest {
    string airp;
    string cls;
    vector<TTPLPax> PaxList;
    TGRPMap *grp_map;
    TInfants *infants;
    TTPLDest(TGRPMap *agrp_map, TInfants *ainfants) {
        grp_map = agrp_map;
        infants = ainfants;
    }
    void GetPaxList(TypeB::TDetailCreateInfo &info, vector<TTlgCompLayer> &complayers);
    void PaxListToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

void TTPLDest::GetPaxList(TypeB::TDetailCreateInfo &info, vector<TTlgCompLayer> &complayers)
{
    TCachedQuery Qry(
            "select "
            "   pax_grp.airp_arv target, "
            "   cls_grp.id cls, "
            "   system.transliter(pax.surname, 1, :pr_lat) surname, "
            "   system.transliter(pax.name, 1, :pr_lat) name, "
            "   crs_pnr.pnr_id, "
            "   pax.pax_id, "
            "   pax.grp_id, "
            "   pax.bag_pool_num "
            "from "
            "   pax, "
            "   pax_grp, "
            "   cls_grp, "
            "   crs_pax, "
            "   crs_pnr "
            "where "
            "   pax_grp.point_dep = :point_id and "
            "   pax_grp.airp_arv = :airp and "
            "   pax_grp.grp_id=pax.grp_id AND "
            "   nvl(pax.cabin_class_grp, pax_grp.class_grp) = cls_grp.id(+) AND "
            "   cls_grp.code = :class and "
            "   pax.pr_brd = 1 and "
            "   pax.pax_id = crs_pax.pax_id(+) and "
            "   crs_pax.pr_del(+)=0 and "
            "   crs_pax.pnr_id = crs_pnr.pnr_id(+) and "
            "   crs_pnr.system(+) = 'CRS' "
            "order by "
            "   target, "
            "   cls, "
            "   surname, "
            "   name nulls first, "
            "   pax.pax_id ",
        QParams()
            << QParam("point_id", otInteger, info.point_id)
            << QParam("airp", otString, airp)
            << QParam("class", otString, cls)
            << QParam("pr_lat", otInteger, info.is_lat()));

    Qry.get().Execute();
    if(not Qry.get().Eof) {
        int col_target = Qry.get().FieldIndex("target");
        int col_cls = Qry.get().FieldIndex("cls");
        int col_surname = Qry.get().FieldIndex("surname");
        int col_name = Qry.get().FieldIndex("name");
        int col_pnr_id = Qry.get().FieldIndex("pnr_id");
        int col_pax_id = Qry.get().FieldIndex("pax_id");
        int col_grp_id = Qry.get().FieldIndex("grp_id");
        int col_bag_pool_num = Qry.get().FieldIndex("bag_pool_num");
        for(; not Qry.get().Eof; Qry.get().Next()) {
            TTPLPax pax(infants);
            pax.target = Qry.get().FieldAsString(col_target);
            if(!Qry.get().FieldIsNULL(col_cls))
                pax.cls_grp_id = Qry.get().FieldAsInteger(col_cls);
            pax.name.surname = Qry.get().FieldAsString(col_surname);
            pax.name.name = Qry.get().FieldAsString(col_name);
            if(!Qry.get().FieldIsNULL(col_pnr_id))
                pax.pnr_id = Qry.get().FieldAsInteger(col_pnr_id);
            pax.pax_id = Qry.get().FieldAsInteger(col_pax_id);
            pax.grp_id = Qry.get().FieldAsInteger(col_grp_id);
            if(not Qry.get().FieldIsNULL(col_bag_pool_num))
                pax.bag_pool_num = Qry.get().FieldAsInteger(col_bag_pool_num);
            pax.pnrs.get(pax.pnr_id);
            pax.rems.get(info, pax);
            if(pax.rems.items.empty())
                continue;
            grp_map->get(pax.grp_id, pax.bag_pool_num);
            PaxList.push_back(pax);
        }
    }
}

void TTPLDest::PaxListToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    for(vector<TTPLPax>::iterator iv = PaxList.begin(); iv != PaxList.end(); iv++) {
        iv->name.ToTlg(info, body);
        iv->pnrs.ToTlg(info, body);
        iv->rems.ToTlg(info, body);
        grp_map->ToTlg(info, iv->grp_id, iv->bag_pool_num, body);
    }
}

struct TETLDest {
    string airp;
    string cls;
    vector<TETLPax> PaxList;
    TGRPMap *grp_map;
    TInfants *infants;
    TETLDest(TGRPMap *agrp_map, TInfants *ainfants) {
        grp_map = agrp_map;
        infants = ainfants;
    }
    void GetPaxList(TypeB::TDetailCreateInfo &info, vector<TTlgCompLayer> &complayers);
    void PaxListToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

void TASLDest::GetPaxList(TypeB::TDetailCreateInfo &info,vector<TTlgCompLayer> &complayers)
{
    const TypeB::TMarkInfoOptions &markOptions=*(info.optionsAs<TypeB::TMarkInfoOptions>());

    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "    pax_grp.airp_arv target, "
        "    cls_grp.id cls, "
        "    system.transliter(pax.surname, 1, :pr_lat) surname, "
        "    system.transliter(pax.name, 1, :pr_lat) name, "
        "    crs_pnr.pnr_id, "
        "    crs_pnr.sender crs, "
        "    pax.pax_id, "
        "    pax.reg_no, " // Used in EMD report only
        "    pax.ticket_no, "
        "    pax.coupon_no, "
        "    pax.grp_id, "
        "    pax.bag_pool_num "
        "from "
        "    pax, "
        "    pax_grp, "
        "    cls_grp, "
        "    crs_pax, "
        "    crs_pnr "
        "WHERE "
        "    pax_grp.point_dep = :point_id and "
        "    pax_grp.status NOT IN ('E') AND "
        "    pax_grp.airp_arv = :airp and "
        "    pax_grp.grp_id=pax.grp_id AND "
        "    nvl(pax.cabin_class_grp, pax_grp.class_grp) = cls_grp.id(+) AND "
        "    cls_grp.code = :class and "
        "    pax.pr_brd = 1 and "
        "    pax.seats>0 and "
        "    pax.pax_id = crs_pax.pax_id(+) and "
        "    crs_pax.pr_del(+)=0 and "
        "    crs_pax.pnr_id = crs_pnr.pnr_id(+) and "
        "    crs_pnr.system(+) = 'CRS' and "
        "    pax.ticket_rem = 'TKNE' "
        "order by "
        "    target, "
        "    cls, "
        "    surname, "
        "    name nulls first, "
        "    pax.pax_id ";
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.CreateVariable("airp", otString, airp);
    Qry.CreateVariable("class", otString, cls);
    Qry.CreateVariable("pr_lat", otInteger, info.is_lat());
    Qry.Execute();
    if(!Qry.Eof) {
        int col_target = Qry.FieldIndex("target");
        int col_cls = Qry.FieldIndex("cls");
        int col_surname = Qry.FieldIndex("surname");
        int col_name = Qry.FieldIndex("name");
        int col_pnr_id = Qry.FieldIndex("pnr_id");
        int col_crs = Qry.FieldIndex("crs");
        int col_pax_id = Qry.FieldIndex("pax_id");
        int col_reg_no = Qry.FieldIndex("reg_no");
        int col_ticket_no = Qry.FieldIndex("ticket_no");
        int col_coupon_no = Qry.FieldIndex("coupon_no");
        int col_grp_id = Qry.FieldIndex("grp_id");
        int col_bag_pool_num = Qry.FieldIndex("bag_pool_num");
        for(; !Qry.Eof; Qry.Next()) {
            TASLPax pax(infants, &grpEmds);
            pax.target = Qry.FieldAsString(col_target);
            if(!Qry.FieldIsNULL(col_cls))
                pax.cls_grp_id = Qry.FieldAsInteger(col_cls);
            pax.name.surname = Qry.FieldAsString(col_surname);
            pax.name.name = Qry.FieldAsString(col_name);
            if(!Qry.FieldIsNULL(col_pnr_id))
                pax.pnr_id = Qry.FieldAsInteger(col_pnr_id);
            pax.crs = Qry.FieldAsString(col_crs);
            if(not markOptions.crs.empty() and markOptions.crs != pax.crs)
                continue;
            pax.pax_id = Qry.FieldAsInteger(col_pax_id);
            pax.reg_no = Qry.FieldAsInteger(col_reg_no);
            pax.M.get(info, pax.pax_id);
            if(not markOptions.mark_info.empty() and not(pax.M.m_flight == markOptions.mark_info))
                continue;
            pax.ticket_no = Qry.FieldAsString(col_ticket_no);
            pax.coupon_no = Qry.FieldAsInteger(col_coupon_no);
            pax.grp_id = Qry.FieldAsInteger(col_grp_id);
            if(not Qry.FieldIsNULL(col_bag_pool_num))
                pax.bag_pool_num = Qry.FieldAsInteger(col_bag_pool_num);

            TGrpEmds::iterator idx = grpEmds.find(pax.grp_id);
            if(idx == grpEmds.end()) {
                CheckIn::TServicePaymentListWithAuto &payment = grpEmds[pax.grp_id];
                payment.fromDB(pax.grp_id);
                for(CheckIn::TServicePaymentListWithAuto::iterator p=payment.begin(); p!=payment.end();)
                {
                  if (p->isEMD() && (is_report() || p->trfer_num==0))  //!!! ⥪?騩 ᥣ???? ??? ???? ⮫쪮 ASVC ?? PNL ??? ?? ?ᥢ?? PNR ⮦??
                    ++p;
                  else
                    p=payment.erase(p);
                };
            }

            pax.pnrs.get(pax.pnr_id);
            pax.rems.get(info, pax);
            if(pax.rems.items.empty())
                continue;
            PaxList.push_back(pax);
        }
    }
}

void TETLDest::GetPaxList(TypeB::TDetailCreateInfo &info,vector<TTlgCompLayer> &complayers)
{
    const TypeB::TMarkInfoOptions &markOptions=*(info.optionsAs<TypeB::TMarkInfoOptions>());
    const TypeB::TETLOptions *ETLOptions=NULL;
    if(info.optionsIs<TypeB::TETLOptions>())
        ETLOptions=info.optionsAs<TypeB::TETLOptions>();

    string SQLText =
        "select "
        "    pax_grp.airp_arv target, ";
    if(ETLOptions and ETLOptions->rbd)
        SQLText += "    NULL cls, ";
    else
        SQLText += "    cls_grp.id cls, ";
    SQLText +=
        "    system.transliter(pax.surname, 1, :pr_lat) surname, "
        "    system.transliter(pax.name, 1, :pr_lat) name, "
        "    crs_pnr.pnr_id, "
        "    crs_pnr.sender crs, "
        "    pax.pax_id, "
        "    pax.ticket_no, "
        "    pax.coupon_no, "
        "    pax.grp_id, "
        "    pax.bag_pool_num "
        "from "
        "    pax, "
        "    pax_grp, ";
    if(not(ETLOptions and ETLOptions->rbd))
        SQLText += "    cls_grp, ";
    SQLText +=
        "    crs_pax, "
        "    crs_pnr "
        "WHERE "
        "    pax_grp.point_dep = :point_id and "
        "    pax_grp.status NOT IN ('E') AND "
        "    pax_grp.airp_arv = :airp and "
        "    pax_grp.grp_id=pax.grp_id AND ";
    if(ETLOptions and ETLOptions->rbd) {
        SQLText +=
            "   nvl(nvl(pax.cabin_subclass, pax.subclass), nvl(pax.cabin_class, pax_grp.class)) = :class and ";
    } else {
        SQLText +=
            "    nvl(pax.cabin_class_grp, pax_grp.class_grp) = cls_grp.id(+) AND "
            "    cls_grp.code = :class and ";
    }
    SQLText +=
        "    pax.pr_brd = 1 and "
        "    pax.seats>0 and "
        "    pax.pax_id = crs_pax.pax_id(+) and "
        "    crs_pax.pr_del(+)=0 and "
        "    crs_pax.pnr_id = crs_pnr.pnr_id(+) and "
        "    crs_pnr.system(+) = 'CRS' and "
        "    pax.ticket_rem = 'TKNE' "
        "order by "
        "    target, "
        "    cls, "
        "    surname, "
        "    name nulls first, "
        "    pax.pax_id ";
    TQuery Qry(&OraSession);
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.CreateVariable("airp", otString, airp);
    Qry.CreateVariable("class", otString, cls);
    Qry.CreateVariable("pr_lat", otInteger, info.is_lat());
    Qry.Execute();
    if(!Qry.Eof) {
        int col_target = Qry.FieldIndex("target");
        int col_cls = Qry.FieldIndex("cls");
        int col_surname = Qry.FieldIndex("surname");
        int col_name = Qry.FieldIndex("name");
        int col_pnr_id = Qry.FieldIndex("pnr_id");
        int col_crs = Qry.FieldIndex("crs");
        int col_pax_id = Qry.FieldIndex("pax_id");
        int col_ticket_no = Qry.FieldIndex("ticket_no");
        int col_coupon_no = Qry.FieldIndex("coupon_no");
        int col_grp_id = Qry.FieldIndex("grp_id");
        int col_bag_pool_num = Qry.FieldIndex("bag_pool_num");
        for(; !Qry.Eof; Qry.Next()) {
            TETLPax pax(infants);
            pax.target = Qry.FieldAsString(col_target);
            if(!Qry.FieldIsNULL(col_cls))
                pax.cls_grp_id = Qry.FieldAsInteger(col_cls);
            pax.name.surname = Qry.FieldAsString(col_surname);
            pax.name.name = Qry.FieldAsString(col_name);
            if(!Qry.FieldIsNULL(col_pnr_id))
                pax.pnr_id = Qry.FieldAsInteger(col_pnr_id);
            pax.crs = Qry.FieldAsString(col_crs);
            if(not markOptions.crs.empty() and markOptions.crs != pax.crs)
                continue;
            pax.pax_id = Qry.FieldAsInteger(col_pax_id);
            pax.M.get(info, pax.pax_id);
            if(not markOptions.mark_info.empty() and not(pax.M.m_flight == markOptions.mark_info))
                continue;
            pax.ticket_no = Qry.FieldAsString(col_ticket_no);
            pax.coupon_no = Qry.FieldAsInteger(col_coupon_no);
            pax.grp_id = Qry.FieldAsInteger(col_grp_id);
            if(not Qry.FieldIsNULL(col_bag_pool_num))
                pax.bag_pool_num = Qry.FieldAsInteger(col_bag_pool_num);
            pax.pnrs.get(pax.pnr_id);
            pax.rems.get(info, pax);
            grp_map->get(pax.grp_id, pax.bag_pool_num);
            PaxList.push_back(pax);
        }
    }
}

void TASLDest::PaxListToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    for(vector<TASLPax>::iterator iv = PaxList.begin(); iv != PaxList.end(); iv++) {
        iv->name.ToTlg(info, body);
        iv->pnrs.ToTlg(info, body);
        iv->rems.ToTlg(info, body);
    }
}

void TETLDest::PaxListToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    for(vector<TETLPax>::iterator iv = PaxList.begin(); iv != PaxList.end(); iv++) {
        iv->name.ToTlg(info, body);
        iv->pnrs.ToTlg(info, body);
        iv->rems.ToTlg(info, body);
        grp_map->ToTlg(info, iv->grp_id, iv->bag_pool_num, body);
    }
}

template <class T>
struct TDestList {
    TInfants infants; // PRL
    TGRPMap grp_map; // PRL, ETL
    vector<T> items;
    void get_subcls_lst(TypeB::TDetailCreateInfo &info, list<string> &lst);
    void get(TypeB::TDetailCreateInfo &info,vector<TTlgCompLayer> &complayers);

    TWItem paxBag(int grp_id, int pax_id, int bag_pool_num); // calculate bagAmount using grp_map, infants

    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
    // grp_map (???⥩??? ?????? ???????? ???ᮢ) ??????? ?? ???⥩???? ??????楢, ?.?.
    // ??? ??????? ?????? ????᫮?? ? ???? ?ਯ??ᮢ뢠???? ᮮ??. ????? ????????
    TDestList(): grp_map(infants) {}
};

template <class T>
TWItem TDestList<T>::paxBag(int grp_id, int pax_id, int bag_pool_num)
{
    TWItem result;
    TGRPItem &grp_i = grp_map.items[grp_id][bag_pool_num];
    result = grp_i.W;
    for(vector<TInfantsItem>::iterator infRow = infants.items.begin(); infRow != infants.items.end(); infRow++)
        if(infRow->grp_id == grp_id and infRow->parent_pax_id == pax_id)
            result += infRow->W;
    return result;
}

void split_n_save(ostringstream &heading, size_t part_len, TTlgDraft &tlg_draft, TTlgOutPartInfo &tlg_row, vector<string> &body) {
    string part_begin;
    for(vector<string>::iterator iv = body.begin(); iv != body.end(); iv++) {
        if(iv->find('-') == 0)
            part_begin = *iv;
        int pax_len = 0;
        if(iv->find('1') == 0) {
            pax_len = iv->size() + TypeB::endl.size();
            for(vector<string>::iterator j = iv + 1; j != body.end() and j->find('1') != 0; j++) {
                pax_len += j->size() + TypeB::endl.size();
            }
        } else
            pax_len = iv->size() + TypeB::endl.size();
        if(part_len + pax_len <= PART_SIZE)
            pax_len = iv->size() + TypeB::endl.size();
        part_len += pax_len;
        if(part_len > PART_SIZE) {
            tlg_draft.Save(tlg_row);
            tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
            tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
            if(iv->find('-') == 0)
                tlg_row.body = *iv + TypeB::endl;
            else
                tlg_row.body = part_begin + TypeB::endl + *iv + TypeB::endl;
            part_len = tlg_row.textSize();
        } else
            tlg_row.body += *iv + TypeB::endl;
    }
}

    template <class T>
void TDestList<T>::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    ostringstream line;
    bool pr_empty = true;
    for(size_t i = 0; i < items.size(); i++) {
        T *iv = &items[i];
        if(iv->PaxList.empty()) {
            line.str("");
            line
                << "-" << info.TlgElemIdToElem(etAirp, iv->airp)
                << "00" << info.TlgElemIdToElem(etSubcls, iv->cls, prLatToElemFmt(efmtCodeNative,true)); //?ᥣ?? ?? ??⨭??? - ⠪ ????
            body.push_back(line.str());
        } else {
            const TypeB::TPRLOptions *PRLOptions=NULL;
            if(info.optionsIs<TypeB::TPRLOptions>())
                PRLOptions=info.optionsAs<TypeB::TPRLOptions>();

            const TypeB::TETLOptions *ETLOptions=NULL;
            if(info.optionsIs<TypeB::TETLOptions>())
                ETLOptions=info.optionsAs<TypeB::TETLOptions>();

            pr_empty = false;
            line.str("");
            line
                << "-" << info.TlgElemIdToElem(etAirp, iv->airp)
                << setw(2) << setfill('0') << iv->PaxList.size();
            if(
                    (PRLOptions and PRLOptions->rbd) or
                    (ETLOptions and ETLOptions->rbd)
                    )
                line << info.TlgElemIdToElem(etSubcls, iv->cls, prLatToElemFmt(efmtCodeNative,true)); //?ᥣ?? ?? ??⨭??? - ⠪ ????
            else
                line << info.TlgElemIdToElem(etClsGrp, iv->PaxList[0].cls_grp_id, prLatToElemFmt(efmtCodeNative,true)); //?ᥣ?? ?? ??⨭??? - ⠪ ????
            body.push_back(line.str());
            iv->PaxListToTlg(info, body);
        }
    }

    if(pr_empty) {
        body.clear();
        body.push_back("NIL");
    }
}

struct TLDMBag {;
    int bag_amount, baggage, cargo, mail;
    void get(TypeB::TDetailCreateInfo &info, int point_arv);
    TLDMBag():
        bag_amount(0),
        baggage(0),
        cargo(0),
        mail(0)
    {};
    TLDMBag &operator += (const TLDMBag &item)
    {
        bag_amount += item.bag_amount;
        baggage += item.baggage;
        cargo += item.cargo;
        mail += item.mail;
        return *this;
    }
};

struct TToRampBag {
    enum Status{ rbBrd, rbReg };
    map<string, pair<int, int> > items; // items[airp] = <amount, weight>
    void get(int point_id, Status st, const string &airp_arv = "");
    pair<int, int> by_flight(); // return <amount, weight> summary
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
    bool empty();
    void dump(const string &file, int line) const
    {
        LogTrace(TRACE5) << "---TToRampBag " << file << ":" << line << " ---";
        for(const auto &i: items)
            LogTrace(TRACE5) << "items[" << i.first << "] = <" << i.second.first << ", " << i.second.second << ">";
        LogTrace(TRACE5) << "---TToRampBag end dump ---";
    }
    TToRampBag &operator += (const TToRampBag &item)
    {
        for(const auto &i: item.items) {
            pair<int, int> &items_val = items[i.first];
            items_val.first += i.second.first;
            items_val.second += i.second.second;
        }
        return *this;
    }
};

void TToRampBag::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    for(map<string, pair<int, int> >::iterator i = items.begin(); i != items.end(); i++) {
        ostringstream res;
        res
            << "-" << info.TlgElemIdToElem(etAirp, i->first)
            << ".DAA/" << i->second.first;
        body.push_back(res.str());
    }
}

pair<int, int> TToRampBag::by_flight()
{
    pair<int, int> result;
    for(map<string, pair<int, int> >::iterator i = items.begin(); i != items.end(); i++) {
        result.first += i->second.first;
        result.second += i->second.second;
    }
    return result;
}

bool TToRampBag::empty()
{
    return items.empty();
}

// ?????஢?? ?????? ?? ????஥???? ⠡??栬:
// ??⥣?ਨ ??????
// ??⥣?ਨ ?????? (???? RFISC)
struct TBagRems {
    typedef map<string, pair<int, int> > TBagRemItems; // <rem_code, bagAmount>
    typedef map<string, TBagRemItems> TARVItems; // <airp_arv, TRFISCItems>
    TARVItems items;
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
    void ToMap(TypeB::TDetailCreateInfo &info, map<string, pair<int, int> > &bag_info);
};

void TBagRems::ToMap(TypeB::TDetailCreateInfo &info, map<string, pair<int, int> > &bag_info)
{
    bag_info.clear();
    for(TARVItems::iterator airp_arv = items.begin(); airp_arv != items.end(); airp_arv++) {
        for(TBagRemItems::iterator bag_rem = airp_arv->second.begin(); bag_rem != airp_arv->second.end(); bag_rem++) {
            if(bag_rem->second.first != 0) {
                bag_info[bag_rem->first].first += bag_rem->second.first;
                bag_info[bag_rem->first].second += bag_rem->second.second;
            }
        }
    }
}

void TBagRems::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    for(TARVItems::iterator airp_arv = items.begin(); airp_arv != items.end(); airp_arv++) {
        for(TBagRemItems::iterator bag_rem = airp_arv->second.begin(); bag_rem != airp_arv->second.end(); bag_rem++) {
            if(bag_rem->second.first != 0) {
                ostringstream res;
                res
                    << "-" << info.TlgElemIdToElem(etAirp, airp_arv->first)
                    << "." << info.TlgElemIdToElem(etCkinRemType, bag_rem->first)
                    << "/" << bag_rem->second.first;
                body.push_back(res.str());
            }
        }
    }
}

void TBagRems::get(TypeB::TDetailCreateInfo &info)
{
    TCachedQuery Qry(
      "SELECT pax_grp.airp_arv, "
      "       bag2.list_id, "
      "       bag2.rfisc, "
      "       bag2.bag_type, "
      "       bag2.bag_type_str, "
      "       bag2.service_type, "
      "       bag2.airline, "
      "       bag_types.rem_code_lci, "
      "       SUM(bag2.amount) AS amount, "
      "       SUM(bag2.weight) AS weight "
      "FROM pax_grp, bag2, bag_types "
      "WHERE pax_grp.grp_id = bag2.grp_id AND "
      "      pax_grp.point_dep = :point_id AND "
      "      pax_grp.status NOT IN ('E') AND "
      "      bag2.pr_cabin=0 AND "
      "      ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse) = 0 AND "
      "      bag2.bag_type = bag_types.code(+) "
      "GROUP BY pax_grp.airp_arv, "
      "         bag2.list_id, "
      "         bag2.rfisc, "
      "         bag2.bag_type, "
      "         bag2.bag_type_str, "
      "         bag2.service_type, "
      "         bag2.airline, "
      "         bag_types.rem_code_lci",
      QParams() << QParam("point_id", otInteger, info.point_id));

    Qry.get().Execute();

    TRFISCListWithPropsCache lists;
    for(; not Qry.get().Eof; Qry.get().Next()) //?? ???????? ??筠? ?????!
    {
      string airp_arv = Qry.get().FieldAsString("airp_arv");
      string rem_code = Qry.get().FieldAsString("rem_code_lci");
      CheckIn::TSimpleBagItem bagItem;
      bagItem.fromDB(Qry.get());
      if (bagItem.pc)
        rem_code=bagItem.get_rem_code_lci(lists);

      if(not rem_code.empty()) {
          items[airp_arv][rem_code].first += bagItem.amount;
          items[airp_arv][rem_code].second += bagItem.weight;
      }
    }
}

void TToRampBag::get(int point_id, Status st, const string &airp_arv)
{
    items.clear();
    QParams QryParams;
    QryParams << QParam("point_id", otInteger, point_id);
    if(not airp_arv.empty())
        QryParams << QParam("airp_arv", otString, airp_arv);
    TCachedQuery Qry(
            (string)
            "select "
            "   pax_grp.airp_arv, "
            "   sum(amount) amount, "
            "   sum(weight) weight "
            "from "
            "   pax_grp, "
            "   bag2 "
            "where "
            "   pax_grp.grp_id = bag2.grp_id and "
            "   pax_grp.point_dep = :point_id and "
            "   pax_grp.status NOT IN ('E') AND "
            "   bag2.pr_cabin=0 AND " +
            (st == rbBrd ?
            "   ckin.bag_pool_boarded(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)<>0 and "
            :
            "   ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse) = 0 and "
            ) +
            (airp_arv.empty() ? "" : " pax_grp.airp_arv = :airp_arv and " ) +
            "   bag2.to_ramp <> 0 "
            "group by "
            "   airp_arv",
            QryParams
            );
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next())
        items[Qry.get().FieldAsString("airp_arv")] =
            make_pair(
                    Qry.get().FieldAsInteger("amount"),
                    Qry.get().FieldAsInteger("weight"));
}

void TLDMBag::get(TypeB::TDetailCreateInfo &info, int point_arv)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT NVL(SUM(weight),0) AS weight, "
        "       NVL(SUM(amount),0) AS amount "
        "FROM pax_grp,bag2 "
        "WHERE pax_grp.grp_id=bag2.grp_id AND "
        "      pax_grp.point_dep=:point_id AND "
        "      pax_grp.point_arv=:point_arv AND "
        "      pax_grp.status NOT IN ('E') AND "
        "      bag2.pr_cabin=0 AND "
        "      ckin.bag_pool_boarded(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)<>0";
    Qry.CreateVariable("point_arv", otInteger, point_arv);
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.Execute();
    if(not Qry.Eof) {
        bag_amount = Qry.FieldAsInteger("amount");
        baggage = Qry.FieldAsInteger("weight");
    }
    Qry.SQLText =
        "SELECT cargo,mail "
        "FROM trip_load "
        "WHERE point_dep=:point_id AND point_arv=:point_arv ";
    Qry.Execute();
    if(!Qry.Eof) {
        cargo = Qry.FieldAsInteger("cargo");
        mail = Qry.FieldAsInteger("mail");
    }
}

struct TExcess {
    TBagKilos kilos;
    TBagPieces pieces;
    void get(int point_id, string airp_arv = "");
    TExcess(): kilos(NoExists), pieces(NoExists) {}
    TExcess &operator += (const TExcess &item)
    {
        kilos += item.kilos;
        pieces += item.pieces;
        return *this;
    }
};

void TExcess::get(int point_id, string airp_arv)
{
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT NVL(SUM(excess_wt),0) excess_wt "
        "FROM pax_grp "
        "WHERE "
        "   point_dep=:point_id AND "
        "   pax_grp.status NOT IN ('E') AND "
        "   ckin.excess_boarded(grp_id,class,bag_refuse)<>0 "; //??? excess_pc ???? ??।????? boarded ?? ??????? ???ᠦ???
    if(not airp_arv.empty()) {
        SQLText += " and airp_arv = :airp_arv ";
        Qry.CreateVariable("airp_arv", otString, airp_arv);
    }
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    kilos = Qry.FieldAsInteger("excess_wt");
}

struct TLDMDests;

struct TLDMDest {
    int point_arv;
    string target;
    int rk_weight;
    int f;
    int c;
    int y;
    int adl;
    int chd;
    int inf;
    int female;
    int male;
    TLDMBag bag;
    TExcess excess;
    TToRampBag to_ramp;

    struct TRem: public set<string> {
        public:
            void add(map<int, CheckIn::TBagMap> &bags, TRFISCListWithPropsCache &lists, int grp_id);
            string str();
    } rems;

    void append(TLDMDests &dests);
    TLDMDest():
        point_arv(NoExists),
        rk_weight(NoExists),
        f(NoExists),
        c(NoExists),
        y(NoExists),
        adl(NoExists),
        chd(NoExists),
        inf(NoExists),
        female(NoExists),
        male(NoExists)
    {};
    TLDMDest &operator += (const TLDMDest &item)
    {
        bag += item.bag;
        rk_weight += item.rk_weight;
        f += item.f;
        excess += item.excess;
        to_ramp += item.to_ramp;
        c += item.c;
        y += item.y;
        adl += item.adl;
        chd += item.chd;
        inf += item.inf;
        female += item.female;
        male += item.male;
        rems.insert(item.rems.begin(), item.rems.end());
        return *this;
    }
};

struct TETLCFG:TCFG {
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        ostringstream cfg;
        if(not empty()) {
            cfg << "CFG/";
            for(vector<TCFGItem>::iterator iv = begin(); iv != end(); iv++)
            {
                cfg
                    << setw(3) << setfill('0') << iv->cfg
                    << info.TlgElemIdToElem(etClass, iv->cls);
            }
        }
        if(not cfg.str().empty())
            body.push_back(cfg.str());
    }
};

class TLDMCrew {
  public:
    int cockpit,cabin;
    TLDMCrew():
          cockpit(NoExists),
          cabin(NoExists)
    {};
    void get(TypeB::TDetailCreateInfo &info);
};

void TLDMCrew::get(TypeB::TDetailCreateInfo &info)
{
    cockpit=NoExists;
    cabin=NoExists;
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT cockpit,cabin FROM trip_crew WHERE point_id=:point_id";
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.Execute();
    if (!Qry.Eof)
    {
      if (!Qry.FieldIsNULL("cockpit")) cockpit=Qry.FieldAsInteger("cockpit");
      if (!Qry.FieldIsNULL("cabin")) cabin=Qry.FieldAsInteger("cabin");
    };
};

struct TLDMCFG:TCFG {
    bool pr_f;
    bool pr_c;
    bool pr_y;
    TLDMCrew crew;
    TLDMCFG():
        pr_f(false),
        pr_c(false),
        pr_y(false),
        crew()
    {};
    void ToTlg(TypeB::TDetailCreateInfo &info, bool &vcompleted, vector<string> &body)
    {
        ostringstream cfg;
        for(vector<TCFGItem>::iterator iv = begin(); iv != end(); iv++)
        {
            if(not cfg.str().empty())
                cfg << "/";
            cfg << iv->cfg;
            if(iv->cls == "?") pr_f = true;
            if(iv->cls == "?") pr_c = true;
            if(iv->cls == "?") pr_y = true;
        }
        if (info.bort.empty() ||
                cfg.str().empty() ||
                (crew.cockpit==NoExists and
                 crew.cabin==NoExists))
            vcompleted = false;

        // ?᫨ ??? NoExists, ?? ?????ᨪ?, ????? ?????塞 ???ﬨ NoExist'?
        int cockpit = NoExists;
        int cabin = NoExists;
        if(not
                (crew.cockpit==NoExists and
                 crew.cabin==NoExists))
        {
            cockpit = (crew.cockpit == NoExists ? 0 : crew.cockpit);
            cabin = (crew.cabin == NoExists ? 0 : crew.cabin);
        }

        ostringstream buf;
        buf
            << info.flight_view() << "/"
            << DateTimeToStr(info.scd_utc, "dd", 1)
            << "." << (info.bort.empty() ? "??" : info.bort)
            << "." << (cfg.str().empty() ? "?" : cfg.str())
            << "." << (cockpit==NoExists ? "?" : IntToString(cockpit))
            << "/" << (cabin==NoExists ? "?" : IntToString(cabin));
        body.push_back(buf.str());
    }
    void get(TypeB::TDetailCreateInfo &info);
};

void TLDMCFG::get(TypeB::TDetailCreateInfo &info)
{
    TCFG::get(info.point_id);
    crew.get(info);
};

struct TLDMDests {
    TLDMCFG cfg;
    vector<TLDMDest> items;
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(TypeB::TDetailCreateInfo &info, bool &vcompleted, vector<string> &body);
};

void TLDMDests::ToTlg(TypeB::TDetailCreateInfo &info, bool &vcompleted, vector<string> &body)
{
    cfg.ToTlg(info, vcompleted, body);
    TToRampBag to_ramp_sum;
    TExcess excess_sum;
    excess_sum.kilos = 0;
    excess_sum.pieces = 0;
    int baggage_sum = 0;
    int cargo_sum = 0;
    int mail_sum = 0;
    ostringstream row;
    //?஢?ਬ LDM ??⮬?????᪨ ???ࠢ?????? ??? ????
    TypeB::TSendInfo sendInfo(info);
    bool pr_send=sendInfo.isSend();
    const TypeB::TLDMOptions &options = *info.optionsAs<TypeB::TLDMOptions>();

    vector<string> si;
    vector<string> si_trzt;
    for(vector<TLDMDest>::iterator iv = items.begin(); iv != items.end(); iv++) {
        row.str("");
        row
            << "-" << info.TlgElemIdToElem(etAirp, iv->target)
            << ".";
        if(options.gender)
            row << iv->male << "/" << iv->female << "/" << iv->chd << "/" << iv->inf;
        else
            row << iv->adl << "/" << iv->chd << "/" << iv->inf;
        if(options.cabin_baggage)
            row << "." << iv->rk_weight;
        row
            << ".T"
            << iv->bag.baggage + iv->bag.cargo + iv->bag.mail;
        if (!pr_send)
        {
            row << ".?/?";   //????।?????? ?? ??????????
            vcompleted = false;
        };

        row << ".PAX";
        if(cfg.pr_f or iv->f != 0)
            row << "/" << iv->f;
        row
            << "/" << iv->c
            << "/" << iv->y
            << ".PAD";
        if(cfg.pr_f)
            row << "/0";
        row
            << "/0"
            << "/0";
        if(options.version == "28ed") row << iv->rems.str();
        if(options.version == "CEK" and info.airp_dep == "???")
            row
                << ".B/" << iv->bag.baggage
                << ".C/" << iv->bag.cargo
                << ".M/" << iv->bag.mail;
        body.push_back(row.str());
        if(options.version == "28ed") {
            row.str("");
            row
                << "SI "
                << info.TlgElemIdToElem(etAirp, iv->target) << " "
                << "B/" << iv->bag.baggage
                << ".C/" << iv->bag.cargo
                << ".M/" << iv->bag.mail;
            si.push_back(row.str());
        }
        to_ramp_sum += iv->to_ramp;
        excess_sum += iv->excess;
        baggage_sum += iv->bag.baggage;
        cargo_sum += iv->bag.cargo;
        mail_sum += iv->bag.mail;
        if(options.version == "AFL") {
            body.push_back("SI");
            ostringstream buf;
            buf
                << info.TlgElemIdToElem(etAirp, iv->target)
                << " C " << iv->bag.cargo
                << " M " << iv->bag.mail
                << " B " << iv->bag.baggage
                << " E " << iv->excess.kilos.getQuantity();
            body.push_back(buf.str());
        }
        if(options.version == "AMADEUS") {
            if(si_trzt.empty())
                si_trzt.push_back("SI");
            row.str("");
            row
                << info.TlgElemIdToElem(etAirp, iv->target) << " "
                << "C " << setw(7) << right << iv->bag.cargo << " "
                << "M " << setw(7) << right << iv->bag.mail << " "
                << "B " << setw(5) << right << iv->bag.bag_amount << "/"
                << setw(7) << right << iv->bag.baggage << " "
                << "O" << setw(8) << 0 << " "
                << "T" << setw(8) << 0;
            si_trzt.push_back(row.str());
        }
    }
    if(options.version == "28ed")
        body.insert(body.end(), si.begin(), si.end());
    if((options.version == "CEK" or options.version == "AMADEUS") and options.exb) {
        row.str("");
        row << "SI: EXB" << excess_sum.kilos.getQuantity() << KG;
        body.push_back(row.str());
    }
    if(options.version != "AMADEUS" and options.version == "CEK" and info.airp_dep != "???") {
        row.str("");
        row << "SI: B";
        if(baggage_sum > 0)
            row << baggage_sum;
        else
            row << "NIL";
        row << ".C";
        if(cargo_sum > 0)
            row << cargo_sum;
        else
            row << "NIL";
        row << ".M";
        if(mail_sum > 0)
            row << mail_sum;
        else
            row << "NIL";
        row << ".DAA";
        if(to_ramp_sum.empty())
            row << "NIL";
        else
            row << to_ramp_sum.by_flight().first << "/" << to_ramp_sum.by_flight().second << KG;
        body.push_back(row.str());
    }
    if(options.version == "AMADEUS")
        body.insert(body.end(), si_trzt.begin(), si_trzt.end());
    //    body.push_back("SI: TRANSFER BAG CPT 0 NS 0");
}

void fillFltDetails(TypeB::TDetailCreateInfo &info)
{

    QParams QryParams;
    QryParams << QParam("vpoint_id", otInteger, info.point_id);
    TCachedQuery Qry(
            "SELECT "
            "   points.airline, "
            "   points.flt_no, "
            "   points.suffix, "
            "   points.scd_out, "
            "   points.est_out, "
            "   points.act_out, "
            "   points.bort, "
            "   points.craft, "
            "   points.airp, "
            "   points.point_num, "
            "   points.first_point, "
            "   points.pr_tranzit, "
            "   nvl(trip_sets.pr_lat_seat, 1) pr_lat_seat "
            "from "
            "   points, "
            "   trip_sets "
            "where "
            "   points.point_id = :vpoint_id AND points.pr_del>=0 and "
            "   points.point_id = trip_sets.point_id(+) ",
        QryParams);
    Qry.get().Execute();
    if(Qry.get().Eof)
        throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND");
    if (Qry.get().FieldIsNULL("scd_out"))
        throw AstraLocale::UserException("MSG.FLIGHT_DATE.NOT_SET");
    info.airline = Qry.get().FieldAsString("airline");
    if (!Qry.get().FieldIsNULL("flt_no"))
        info.flt_no = Qry.get().FieldAsInteger("flt_no");
    info.suffix = Qry.get().FieldAsString("suffix");
    info.bort = Qry.get().FieldAsString("bort");
    info.craft = Qry.get().FieldAsString("craft");
    info.airp_dep = Qry.get().FieldAsString("airp");
    info.point_num = Qry.get().FieldAsInteger("point_num");
    info.first_point = Qry.get().FieldIsNULL("first_point")?NoExists:Qry.get().FieldAsInteger("first_point");
    info.pr_tranzit = Qry.get().FieldAsInteger("pr_tranzit")!=0;
    info.pr_lat_seat = Qry.get().FieldAsInteger("pr_lat_seat") != 0;

    string tz_region=AirpTZRegion(info.airp_dep);
    if (!Qry.get().FieldIsNULL("scd_out"))
    {
        info.scd_utc = getReportSCDOut(info.point_id);
        info.scd_local = UTCToLocal( info.scd_utc, tz_region );
        int Year, Month, Day;
        DecodeDate(info.scd_local, Year, Month, Day);
        info.scd_local_day = Day;
    };

    if(!Qry.get().FieldIsNULL("est_out"))
        info.est_utc = Qry.get().FieldAsDateTime("est_out");
    if(!Qry.get().FieldIsNULL("act_out"))
        info.act_local = UTCToLocal( Qry.get().FieldAsDateTime("act_out"), tz_region );
}

void TLDMDest::append(TLDMDests &dests)
{
    if(dests.items.empty()) return;
    for(const auto &dest: dests.items)
        if(dest.point_arv == point_arv)
            *this += dest;
}

void TLDMDest::TRem::add(map<int, CheckIn::TBagMap> &bags, TRFISCListWithPropsCache &lists, int grp_id)
{
    if(bags.find(grp_id) == bags.end()) {
        auto res = bags.insert(make_pair(grp_id, CheckIn::TBagMap()));
        res.first->second.fromDB(grp_id);
        for(const auto &i: res.first->second) {
            if(i.second.pc) {
                string rem = i.second.get_rem_code_ldm(lists);
                if(not rem.empty()) insert(rem);
            }
            if(i.second.wt) {
                try {
                    int bag_type = ToInt(i.second.wt->bag_type);
                    const TBagTypesRow &row=(const TBagTypesRow&)(base_tables.get("bag_types").get_row("id",bag_type));
                    if(not row.rem_code_ldm.empty())
                        insert(row.rem_code_ldm);
                } catch(...) {}
            }
        }
    }
}

string TLDMDest::TRem::str()
{
    string result;
    for(const auto &i: *this)
        result += "." + i;
    return result;
}

void TLDMDests::get(TypeB::TDetailCreateInfo &info)
{
    TLDMDests before_dests;
    TTripRouteItem priorAirp;
    TTripRoute().GetPriorAirp(NoExists, info.point_id, trtNotCancelled,priorAirp);
    if(priorAirp.point_id != NoExists) {
        TypeB::TDetailCreateInfo before_detail;
        before_detail.point_id = priorAirp.point_id;
        fillFltDetails(before_detail);
        before_dests.get(before_detail);
    }

    cfg.get(info);

    REPORTS::TPaxList pax_list(info.point_id);
    pax_list.options.flags.setFlag(REPORTS::oeRkWeight);
    pax_list.options.pr_brd = boost::in_place(REPORTS::TBrdVal::bvTRUE);
    pax_list.fromDB();
    TTripRoute route;
    map<int, CheckIn::TBagMap> bags;
    TRFISCListWithPropsCache lists;
    if(route.GetRouteAfter(NoExists, info.point_id, trtNotCurrent, trtNotCancelled)) {
        for(const auto &point_arv: route) {
            items.emplace_back();
            auto &item = items.back();
            item.point_arv = point_arv.point_id;
            item.target = point_arv.airp;
            item.bag.get(info, item.point_arv);
            item.excess.get(info.point_id, item.target);
            item.to_ramp.get(info.point_id, TToRampBag::rbBrd, item.target);

            item.rk_weight = 0;
            item.f = 0;
            item.c = 0;
            item.y = 0;
            item.adl = 0;
            item.chd = 0;
            item.inf = 0;
            item.female = 0;
            item.male = 0;

            for(const auto &pax: pax_list) {
                if(pax->grp().point_arv == point_arv.point_id) {
                    item.rk_weight += pax->rk_weight(true);
                    item.f += pax->cl() == "?" and pax->seats();
                    item.c += pax->cl() == "?" and pax->seats();
                    item.y += pax->cl() == "?" and pax->seats();
                    item.adl += pax->simple.pers_type == TPerson::adult;
                    item.chd += pax->simple.pers_type == TPerson::child;
                    item.inf += pax->simple.pers_type == TPerson::baby;
                    item.female += pax->simple.pers_type == TPerson::adult and pax->simple.gender == TGender::Female;
                    item.male += pax->simple.pers_type == TPerson::adult and pax->simple.gender != TGender::Female;
                    item.rems.add(bags, lists, pax->simple.grp_id);
                }
            }
            item.append(before_dests);
        }
    }
}

struct TMVTABodyItem {
    string target;
    TDateTime est_in;
    int seats, inf;
    TMVTABodyItem():
        est_in(NoExists),
        seats(NoExists),
        inf(NoExists)
    {};
};

struct TTripDelayItem {
    int delay_code;
    TDateTime time;
    TTripDelayItem(const int &adelay_code, TDateTime atime):
        delay_code(adelay_code), time(atime) {}
};

struct TTripDelays:vector<TTripDelayItem> {
    private:
        bool pr_MVTC;
    public:
        string delay_code(TypeB::TDetailCreateInfo &info, int delay_code);
        string delay_value(TypeB::TDetailCreateInfo &info, TDateTime prev, TDateTime curr);
        void get(TypeB::TDetailCreateInfo &info);
        void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body, bool extra);
        TTripDelays(bool apr_MVTC): pr_MVTC(apr_MVTC)
    {}
};

bool check_delay_value(TDateTime delay_time)
{
    int hours, mins, secs;
    double f;
    double remain = modf(delay_time, &f);
    DecodeTime( remain, hours, mins, secs );
    return delay_time > 0 && f * 24 * 60 + hours * 60 + mins < MAX_DELAY_TIME;
}

bool check_delay_code(int delay_code)
{
    return delay_code >= 0 and delay_code <= 99;
}

bool check_delay_code(const string &delay_code)
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT num FROM delays WHERE code=:code";
  Qry.CreateVariable( "code", otString, delay_code );
  Qry.Execute();
  return ( !Qry.Eof && check_delay_code( Qry.FieldAsInteger( "num" ) ) );
}

string TTripDelays::delay_value(TypeB::TDetailCreateInfo &info, TDateTime prev, TDateTime curr)
{
    ostringstream result;
    if(check_delay_value(curr - prev)) {
        int hours, mins, secs;
        double f;
        double remain = modf(curr - prev, &f);
        DecodeTime( remain, hours, mins, secs );
        result << setfill('0') << setw(2) << f * 24 + hours << setw(2) << mins;
    } else
        result << info.err_lst.add_err(TypeB::DEFAULT_ERR, "Delay out of range %d mins", MAX_DELAY_TIME);
    return result.str();
}

string TTripDelays::delay_code(TypeB::TDetailCreateInfo &info, int delay_code)
{
    ostringstream result;
    if(check_delay_code(delay_code)) {
        result << setw(2) << setfill('0') << delay_code;
    } else
        result << info.err_lst.add_err(IntToString(delay_code), LexemaData("MSG.MVTDELAY.INVALID_CODE"));
    return result.str();
}

void TTripDelays::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body, bool extra)
{
    vector< pair<string, string> > delays;
    TTripDelays::iterator iv = begin();
    if(extra) {
        if(size() > 2)
            for(int i = 0; i < 2; i++, iv++); // ??⠭????? ???????? ?? 3-? ???????
        else
            iv = end();
    }
    for(int i = 0; iv != end() and i < 2; iv++, i++) {
        delays.push_back(make_pair(
                    delay_code(info, iv->delay_code),
                    delay_value(info, info.scd_utc, iv->time)
                    ));
    }
    if(delays.size() != 0) {
        ostringstream buf;
        string id = extra ? "EDL" : "DL";
        switch(delays.size()) {
            case 1:
                buf << id << delays[0].first;
                if(not pr_MVTC)
                    buf << "/" << delays[0].second;
                break;
            case 2:
                buf << id << delays[0].first << "/" << delays[1].first;
                if(not pr_MVTC)
                    buf << "/" << delays[0].second << "/" << delays[1].second;
                break;
            default:
                throw UserException("wrong delay count");
        }
        body.push_back(buf.str());
    }
}

void TTripDelays::get(TypeB::TDetailCreateInfo &info)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select * from trip_delays, delays where "
        "   point_id = :point_id and "
        "   trip_delays.delay_code = delays.code "
        "order by delay_num";
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.Execute();
    int err_idx = NoExists;
    for(int idx = 0; not Qry.Eof; Qry.Next(), idx++) {
        int adelay_code = Qry.FieldAsInteger("num");
        TDateTime adelay_value = Qry.FieldAsDateTime("time");
        push_back(TTripDelayItem(adelay_code, adelay_value));
        // ? MVTC ?뢮????? ⮫쪮 ???? ????থ?, ???⮬? ?஢????? ??ଠ? ????ࢠ?? ?? ?㦭?
        if(not check_delay_code(adelay_code) or (not pr_MVTC and not check_delay_value(adelay_value - info.scd_utc)))
            err_idx = idx;
    }
    if(err_idx != NoExists) { // ???? ?訡????? ????প?
        if(err_idx == (int)size() - 1) { // ??᫥???? ????প? ? ?訡???
            erase(begin(), begin() + err_idx); // ??⠢?塞 ⮫쪮 ??
        } else
            erase(begin(), begin() + err_idx + 1); // ?모?뢠?? ??? ????প? ?? ??᫥???? ?訡?筮? ??????⥫쭮
    }
    if(size() > 4)
        erase(begin(), (begin() + size() - 4)); // ??⠢?塞 4 ??᫥???? ????প?
}

struct TMVTCBody {
    TTripDelays delays;
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
    TMVTCBody(): delays(true) {};
};

void TMVTCBody::get(TypeB::TDetailCreateInfo &info)
{
    delays.get(info);
}

void TMVTCBody::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    if(delays.empty())
        body.push_back(info.err_lst.add_err(TypeB::DEFAULT_ERR, "delays not found"));
    else if(info.est_utc == NoExists)
        body.push_back(info.err_lst.add_err(TypeB::DEFAULT_ERR, "est_utc not defined"));
    else {
        ostringstream buf;
        buf << "ED" << DateTimeToStr(info.est_utc, "ddhhnn");
        body.push_back(buf.str());
        delays.ToTlg(info, body, false);
        delays.ToTlg(info, body, true);
    }
}

struct TMVTABody {
    TDateTime AD;
    TDateTime act;
    TTripDelays delays;
    vector<TMVTABodyItem> items;
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
    TMVTABody(): act(NoExists), delays(false) {};
};

struct TMVTBBody {
    TDateTime act;
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(bool &vcompleted, vector<string> &body);
    TMVTBBody(): act(NoExists) {};
};

void TMVTBBody::get(TypeB::TDetailCreateInfo &info)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT NVL(act_in,NVL(est_in,scd_in)) AS act_in "
        "FROM points "
        "WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0 "
        "ORDER BY point_num ";
    Qry.CreateVariable("first_point", otInteger, info.pr_tranzit ? info.first_point : info.point_id);
    Qry.CreateVariable("point_num", otInteger, info.point_num);
    Qry.Execute();
    if(!Qry.Eof) {
        if(!Qry.FieldIsNULL("act_in"))
            act = Qry.FieldAsDateTime("act_in");
    }

}

void TMVTBBody::ToTlg(bool &vcompleted, vector<string> &body)
{
    ostringstream buf;
    if(act != NoExists) {
        TDateTime on_block = act + 5./1440;
        int year, month, day1, day2;
        string fmt;
        DecodeDate(act, year, month, day1);
        DecodeDate(on_block, year, month, day2);
        if(day1 != day2)
            fmt = "ddhhnn";
        else
            fmt = "hhnn";
        buf
            << "AA"
            << DateTimeToStr(act, fmt)
            << "/"
            << DateTimeToStr(on_block, fmt);
    } else {
        vcompleted = false;
        buf << "AA\?\?\?\?/\?\?\?\?";
    }
    body.push_back(buf.str());
}

#define get_fmt(x, y) (x != y ? "ddhhnn" : "hhnn")

bool CheckTimeToken(TDateTime scd_utc, TDateTime val, const string &token)
{
    TDateTime parsed = TypeB::MVTParser::TAD::fetch_time(scd_utc, token);
    return parsed == val;
}

void TMVTABody::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    ostringstream buf;
    if(act != NoExists) {
        int year, month, act_day, ad_day, utc_day;
        DecodeDate(act, year, month, act_day);
        DecodeDate(AD, year, month, ad_day);
        DecodeDate(info.scd_utc, year, month, utc_day);

        string str_ad =  DateTimeToStr(AD, get_fmt(ad_day, utc_day));
        string str_act = DateTimeToStr(act, get_fmt(act_day, utc_day));

        if(not CheckTimeToken(info.scd_utc, AD, str_ad))
            throw Exception("cannot set off-block time for %s", DateTimeToStr(AD).c_str());

        if(not CheckTimeToken(info.scd_utc, act, str_act))
            throw Exception("cannot set airborne time for %s", DateTimeToStr(act).c_str());

        buf
            << "AD"
            << DateTimeToStr(AD, get_fmt(ad_day, utc_day))
            << "/"
            << DateTimeToStr(act, get_fmt(act_day, utc_day));
    } else {
        info.vcompleted = false;
        buf << "AD\?\?\?\?/\?\?\?\?";
    }
    for(vector<TMVTABodyItem>::iterator i = items.begin(); i != items.end(); i++) {
        if(i == items.begin()) {
            buf << " EA";
            if(i->est_in == NoExists) {
                info.vcompleted = false;
                buf << "????";
            } else
                buf << DateTimeToStr(i->est_in, "hhnn");
            buf << " " << info.TlgElemIdToElem(etAirp, i->target);
            body.push_back(buf.str());
            delays.ToTlg(info, body, false);
            buf.str("");
            buf << "PX" << i->seats;
        } else {
            buf << "/" << i->seats;
        }
    }
    body.push_back(buf.str());
    delays.ToTlg(info, body, true);
}

void TMVTABody::get(TypeB::TDetailCreateInfo &info)
{
    TTripStage ts;
    TTripStages::LoadStage(info.point_id, sRemovalGangWay, ts);
    AD = (ts.act != ASTRA::NoExists ? ts.act : (ts.est != ASTRA::NoExists ? ts.est : ts.scd));
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT NVL(act_out,NVL(est_out,scd_out)) act FROM points WHERE point_id=:point_id";
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.Execute();
    if(!Qry.Eof)
        act = Qry.FieldAsDateTime("act");
    Qry.Clear();
    Qry.SQLText =
        "select points.airp as target, "
        "       nvl(est_in,scd_in) as est_in, "
        "       nvl(pax.seats,0) as seats, "
        "       nvl(pax.inf,0) as inf "
        "FROM points, "
        "     (SELECT pax_grp.point_arv, "
        "             SUM(pax.seats) AS seats, "
        "             SUM(DECODE(pax.seats,0,1,0)) AS inf "
        "      FROM pax_grp,pax "
        "      WHERE pax_grp.grp_id=pax.grp_id AND "
        "            point_dep=:point_id AND "
        "            pax_grp.status NOT IN ('E') AND "
        "            pr_brd=1 "
        "      GROUP BY pax_grp.point_arv) pax "
        "WHERE points.point_id=pax.point_arv(+) AND "
        "      first_point=:first_point AND point_num>:point_num AND pr_del=0 "
        "ORDER BY point_num ";
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.CreateVariable("first_point", otInteger, info.pr_tranzit ? info.first_point : info.point_id);
    Qry.CreateVariable("point_num", otInteger, info.point_num);
    Qry.Execute();
    if(!Qry.Eof) {
        int col_target = Qry.FieldIndex("target");
        int col_est_in = Qry.FieldIndex("est_in");
        int col_seats = Qry.FieldIndex("seats");
        int col_inf = Qry.FieldIndex("inf");
        for(; !Qry.Eof; Qry.Next()) {
            TMVTABodyItem item;
            item.target = Qry.FieldAsString(col_target);
            if(not Qry.FieldIsNULL(col_est_in))
                item.est_in = Qry.FieldAsDateTime(col_est_in);
            item.seats = Qry.FieldAsInteger(col_seats);
            item.inf = Qry.FieldAsInteger(col_inf);
            items.push_back(item);
        }
    }
    TTripInfo t;
    t.airline = info.airline;
    t.flt_no = info.flt_no;
    t.airp = info.airp_dep;
    if(GetTripSets(tsSendMVTDelays, t))
        delays.get(info);
}

int CPM(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    info.vcompleted = false;
    ostringstream buf;
    buf << "CPM" << TypeB::endl;
    tlg_row.heading = buf.str();
    tlg_row.ending = "PART " + IntToString(tlg_row.num) + " END" + TypeB::endl;
    if(info.bort.empty())
        info.vcompleted = false;
    buf.str("");
    buf
        << info.flight_view() << "/"
        << DateTimeToStr(info.scd_utc, "dd", 1)
        << "." << (info.bort.empty() ? "??" : info.bort)
        << "." << info.airp_arv_view();
    vector<string> body;
    body.push_back(buf.str());
    body.push_back("SI");
    for(vector<string>::iterator iv = body.begin(); iv != body.end(); iv++)
        tlg_row.body += *iv + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

int MVT(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    info.vcompleted = true;
    ostringstream buf;
    buf << "MVT" << TypeB::endl;
    tlg_row.heading = buf.str();

    const TypeB::TMVTOptions &options = *info.optionsAs<TypeB::TMVTOptions>();

    if(not options.noend)
        tlg_row.ending = "PART " + IntToString(tlg_row.num) + " END" + TypeB::endl;
    if(info.bort.empty())
        info.vcompleted = false;
    buf.str("");
    buf
        << info.flight_view() << "/"
        << DateTimeToStr(info.scd_utc, "dd", 1)
        << "." << (info.bort.empty() ? "??" : info.bort);
    if(info.get_tlg_type() == "MVTA")
      buf << "." << info.airp_dep_view();
    if(info.get_tlg_type() == "MVTB")
      buf << "." << info.airp_arv_view();
    if(info.get_tlg_type() == "MVTC")
      buf << "." << info.airp_dep_view();
    vector<string> body;
    body.push_back(buf.str());
    buf.str("");
    try {
        if(info.get_tlg_type() == "MVTA") {
            TMVTABody MVTABody;
            MVTABody.get(info);
            MVTABody.ToTlg(info, body);
        };
        if(info.get_tlg_type() == "MVTB") {
            TMVTBBody MVTBBody;
            MVTBBody.get(info);
            MVTBBody.ToTlg(info.vcompleted, body);
        }
        if(info.get_tlg_type() == "MVTC") {
            TMVTCBody MVTCBody;
            MVTCBody.get(info);
            MVTCBody.ToTlg(info, body);
        }
    } catch(...) {
        ExceptionFilter(body, info);
    }
    for(vector<string>::iterator iv = body.begin(); iv != body.end(); iv++)
        tlg_row.body += *iv + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

int LDM(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    info.vcompleted = true;
    ostringstream buf;
    buf << "LDM" << TypeB::endl;
    tlg_row.heading = buf.str();

    const TypeB::TLDMOptions &options = *info.optionsAs<TypeB::TLDMOptions>();

    if(not options.noend)
        tlg_row.ending = "PART " + IntToString(tlg_row.num) + " END" + TypeB::endl;
    vector<string> body;
    try {
        TLDMDests LDM;
        LDM.get(info);
        LDM.ToTlg(info, info.vcompleted, body);
    } catch(...) {
        ExceptionFilter(body, info);
    }
    for(vector<string>::iterator iv = body.begin(); iv != body.end(); iv++)
        tlg_row.body += *iv + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

int AHL(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    info.vcompleted = false;
    tlg_row.body =
        "AHL" + TypeB::endl
        + "NM" + TypeB::endl
        + "IT" + TypeB::endl
        + "TN" + TypeB::endl
        + "CT" + TypeB::endl
        + "RT" + TypeB::endl
        + "FD" + TypeB::endl
        + "TK" + TypeB::endl
        + "BI" + TypeB::endl
        + "BW" + TypeB::endl
        + "DW" + TypeB::endl
        + "CC" + TypeB::endl
        + "PA" + TypeB::endl
        + "PN" + TypeB::endl
        + "TP" + TypeB::endl
        + "AG" + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

struct TLCICFG:TCFG {
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        ostringstream cfg;
        if(not empty()) {
            info.vcompleted = info.vcompleted and not info.craft.empty();
            cfg
                << "EQT."
                << (info.bort.empty() ? "XXXXX" : info.bort) << "."
                << (info.craft.empty() ? "??" : info.TlgElemIdToElem(etCraft, info.craft)) << ".";
            for(vector<TCFGItem>::iterator iv = begin(); iv != end(); iv++)
            {
                cfg
                    << info.TlgElemIdToElem(etClass, iv->cls)
                    << setw(3) << setfill('0') << iv->cfg;
            }
        }
        if(not cfg.str().empty())
            body.push_back(cfg.str());
    }
};

struct TWA {
    int payload, underload;
    TWA() { clear(); }
    void clear()
    {
        payload = NoExists;
        underload = NoExists;
    }
    void get(TypeB::TDetailCreateInfo &info)
    {
        payload = getCommerceWeight(info.point_id, onlyCheckin, CWTotal);
        TQuery Qry(&OraSession);
        Qry.SQLText=
            "SELECT max_commerce FROM trip_sets WHERE point_id=:point_id";
        Qry.CreateVariable("point_id", otInteger, info.point_id);
        Qry.Execute();
        int max_payload = NoExists;
        Qry.Execute();
        if(not Qry.Eof and not Qry.FieldIsNULL("max_commerce"))
            max_payload = Qry.FieldAsInteger("max_commerce");
        underload = 0;
        if(max_payload != NoExists and max_payload > payload)
            underload = max_payload - payload;
    }
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        const TypeB::TLCIOptions &options = *info.optionsAs<TypeB::TLCIOptions>();
        ostringstream buf;
        buf << "WA.P." << payload << "." << KG;
        if(options.weight_avail.find('P') != string::npos) body.push_back(buf.str());
        buf.str("");
        buf << "WA.U." << underload << "." << KG;
        if(options.weight_avail.find('U') != string::npos) body.push_back(buf.str());
    }
};

struct TSR_S {
    TPassSeats layerSeats;
    void get(TypeB::TDetailCreateInfo &info)
    {
        const TypeB::TLCIOptions &options = *info.optionsAs<TypeB::TLCIOptions>();
        if(not options.seats.empty()) {
            layerSeats = options.seats;
        } else if(options.seat_restrict.find('S') != string::npos) {
            SALONS2::TSalonList salonList;
            salonList.ReadFlight( SALONS2::TFilterRoutesSets( info.point_id, ASTRA::NoExists ), "", NoExists );
            SALONS2::TSectionInfo sectionInfo;
            SALONS2::TGetPassFlags flags;
            flags.clearFlags();
            salonList.getSectionInfo( sectionInfo, flags );
            sectionInfo.GetTotalLayerSeat( cltProtect, layerSeats );
        }
    }

    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        static const string PREFIX = "SR.S";
        string buf = PREFIX;
        for(TPassSeats::iterator i_seat = layerSeats.begin(); i_seat != layerSeats.end(); i_seat++) {
            string seat;
            if(i_seat->Empty()) {
                if(layerSeats.size() != 1)
                    throw Exception("empty seat must be single");
                seat = "N";
            } else {
                seat = i_seat->denorm_view(info.is_lat() or info.pr_lat_seat); // denorm - ?⮡? ?????????? ?? ?㫥?: 002 -> 2
            }
            if(buf.size() + seat.size() + 1 > LINE_SIZE) {
                body.push_back(buf);
                buf = PREFIX;
            }
            if(buf == PREFIX)
                buf += ".";
            else
                buf += "/";
            buf += seat;
        }
        if(buf != PREFIX)
            body.push_back(buf);
    }
};

struct TSR_Z {
    map<string, int> items;

    void get(TypeB::TDetailCreateInfo &info)
    {
        ZoneLoads(info.point_id, items, false);
    }

    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        ostringstream result;
        if(not items.empty()) {
            result << "SR.Z.";
            for(map<string, int>::iterator i = items.begin(); i != items.end(); i++) {
                if(i != items.begin())
                    result << "/";
                result << i->first << i->second;
            }
            body.push_back(result.str());
        }
    }
};

struct TSR_WB_C {
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body) {
        const TypeB::TLCIOptions &options = *info.optionsAs<TypeB::TLCIOptions>();
        if(options.cfg.empty()) return;
        TCFG cfg(info.point_id);

        // ????室??? ???????? ??? ???? ? ???祭?? ?? 㬫?砭??,
        // ?஬? cls ? cfg ?⮡? ?????஢????? ????????? ? opt_cfg
        // ?????? ???ନ?㥬 ⥪?? ??? ⥫??ࠬ??
        ostringstream buf;
        buf << "SR.WB.C.";
        for(TCFG::iterator i = cfg.begin(); i != cfg.end(); i++) {
            TCFGItem new_item;
            // new_item.cls = i->cls; // ????ਬ ??????
            new_item.cfg = i->cfg;
            buf << info.TlgElemIdToElem(etSubcls, i->cls) << i->cfg;
            *i = new_item;
        }
        body.push_back(buf.str());

        sort(cfg.begin(), cfg.end());
        TCFG opt_cfg = options.cfg;
        sort(opt_cfg.begin(), opt_cfg.end());
        set_alarm(info.point_id, Alarm::WBDifferLayout, not(opt_cfg == cfg));
    }
};

struct TSR_C:TCOMClasses {
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        ostringstream av;
        av << "SR.C.";
        for(vector<TCOMClassesItem>::iterator iv = items.begin(); iv != items.end(); iv++)
            av << iv->cls << iv->av;
        body.push_back(av.str());
    }
};

struct TWM {
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

void TWM::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    PersWeightRules pwr;
    ClassesPersWeight cpw;
    pwr.read(info.point_id);
    ostringstream result;
    pwr.weight("?", string(), cpw);
    result
        << "WM.S.P.CG." << info.TlgElemIdToElem(etClass, "?")
        << cpw.male << "/" << cpw.female << "/" << cpw.child << "/" << cpw.infant;
    pwr.weight("?", string(), cpw);
    result
        << "." << info.TlgElemIdToElem(etClass, "?")
        << cpw.male << "/" << cpw.female << "/" << cpw.child << "/" << cpw.infant;
    pwr.weight("?", string(), cpw);
    result
        << "." << info.TlgElemIdToElem(etClass, "?")
        << cpw.male << "/" << cpw.female << "/" << cpw.child << "/" << cpw.infant
        << "." << KG;
    body.push_back(result.str());
    result.str(string());
    result << "WM.A.B." << KG;
    body.push_back(result.str());
}

struct TByGender {
    size_t m, f, c, i;
    TByGender(): m(0), f(0), c(0), i(0) {};
    bool empty() const;
    void append(TTrickyGender::Enum gender, int count = 1);
};

void TByGender::append(TTrickyGender::Enum gender, int count)
{
    switch(gender) {
        case TTrickyGender::Male:     m += count; break;
        case TTrickyGender::Female:   f += count; break;
        case TTrickyGender::Child:    c += count; break;
        case TTrickyGender::Infant:   i += count; break;
        default: break;
    }
}

bool TByGender::empty() const
{
    return not(m or f or c or i);
}

struct TLCITotals {
    size_t pax_size, bag_amount, bag_weight;
    TLCITotals():
        pax_size(0),
        bag_amount(0),
        bag_weight(0)
    {};
};


struct TPaxTotalsItem {
    typedef map<string, TLCITotals> TItems;
    TItems items; // ??????? ?????
    TItems extra_items; // ???. ?????? (extra crew)
    int pax_size();
    int bag_amount();
    int bag_weight();
};

int TPaxTotalsItem::bag_weight()
{
    LogTrace(TRACE5) << "bag_weight() data:";
    LogTrace(TRACE5)
        << items["?"].bag_weight << " "
        << items["?"].bag_weight << " "
        << items["?"].bag_weight << " "
        << extra_items["?"].bag_weight << " "
        << extra_items["?"].bag_weight << " "
        << extra_items["?"].bag_weight;
    return
        items["?"].bag_weight +
        items["?"].bag_weight +
        items["?"].bag_weight +
        extra_items["?"].bag_weight +
        extra_items["?"].bag_weight +
        extra_items["?"].bag_weight;
}

int TPaxTotalsItem::bag_amount()
{
    return
        items["?"].bag_amount +
        items["?"].bag_amount +
        items["?"].bag_amount +
        extra_items["?"].bag_amount +
        extra_items["?"].bag_amount +
        extra_items["?"].bag_amount;
}

int TPaxTotalsItem::pax_size()
{
    return
        items["?"].pax_size +
        items["?"].pax_size +
        items["?"].pax_size;
}

typedef map<string, TByGender> TByClass;

struct TLCIPaxTotalsItem {
    string airp;
    TPaxTotalsItem cls_totals;

    typedef map<string, pair<int, int> > TCategoryBag;
    typedef map<string, TCategoryBag> TClsBag;

    void clsBagToTlg(
            TypeB::TDetailCreateInfo &info,
            const TClsBag &cls_bag,
            vector<string> &si);

    void clsBagToTlg(
            TypeB::TDetailCreateInfo &info,
            vector<string> &si);

    TClsBag bag_category; // class, bag category, [amount, weight]

    int rk_weight;
    TLCIPaxTotalsItem(): rk_weight(0) {}
};

void TLCIPaxTotalsItem::clsBagToTlg(
        TypeB::TDetailCreateInfo &info,
        vector<string> &si)
{
    clsBagToTlg(info, bag_category, si);
}

void TLCIPaxTotalsItem::clsBagToTlg(
        TypeB::TDetailCreateInfo &info,
        const TClsBag &cls_bag,
        vector<string> &si)
{
    for(TLCIPaxTotalsItem::TClsBag::const_iterator
            iCls = cls_bag.begin();
            iCls != cls_bag.end();
            iCls++) {
        for(TLCIPaxTotalsItem::TCategoryBag::const_iterator
                iCat = iCls->second.begin();
                iCat != iCls->second.end();
                iCat++) {
            if(iCat->second.first) { // ???-?? ?????? ?? ?㫥???
                ostringstream result;
                result
                    << "-" << info.TlgElemIdToElem(etAirp, airp)
                    << "." << iCls->first << ".";
                if(not iCat->first.empty())
                    result << iCat->first << ".";
                result
                    << iCat->second.first << "/"
                    << iCat->second.second
                    << "." << KG;
                si.push_back(result.str());
            }
        }
    }
}

// ????? ? ?????? ????ᮬ (??ᮯ? ? ????? ???????)
struct TEmptyClsBag {
    map<string, pair<int, int> > unacc_items; // airp_arv, bag_amount, bag_weight
    map<string, pair<int, int> > crew_items; // airp_arv, bag_amount, bag_weight
    void clear() { unacc_items.clear(); crew_items.clear(); }
    TEmptyClsBag() { clear(); }
    void get(int point_id);
    void dump(TRACE_SIGNATURE);
    int bag_amount(const string &airp);
    int bag_weight(const string &airp);
};

int TEmptyClsBag::bag_amount(const string &airp)
{
    return unacc_items[airp].first + crew_items[airp].first;
}

int TEmptyClsBag::bag_weight(const string &airp)
{
    return unacc_items[airp].second + crew_items[airp].second;
}

void TEmptyClsBag::dump(TRACE_SIGNATURE)
{
    LogTrace(TRACE_PARAMS) << "---TEmptyClsBag::dump---";
    for(int step = 0; step < 2; step++) {
        LogTrace(TRACE_PARAMS) << "---" << (step == 0 ? "unacc_items" : "crew_items") << "---";
        for(const auto &i: (step == 0 ? unacc_items : crew_items)) {
            LogTrace(TRACE_PARAMS)
                << "airp_arv: " << i.first
                << "; bag_amount: " << i.second.first
                << "; bag_weight: " << i.second.second;
        }
    }
    LogTrace(TRACE_PARAMS) << "---------------------";
}

void TEmptyClsBag::get(int point_id)
{
    clear();
    TCachedQuery Qry(
            "select "
            "   decode(status, 'E', 'E', '') status, "
            "   airp_arv, "
            "   sum (nvl(ckin.get_bagAmount2(pax_grp.grp_id,NULL,NULL), 0))  bag_amount, "
            "   sum (nvl(ckin.get_bagWeight2(pax_grp.grp_id,NULL,NULL), 0)) bag_weight "
            "from pax_grp where "
            "   point_dep = :point_id and "
            "   pax_grp.class IS NULL "
            "group by "
            "   decode(status, 'E', 'E', ''), "
            "   airp_arv ",
            QParams() << QParam("point_id", otInteger, point_id));
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next()) {
        map<string, pair<int, int> > &items_ref =
            ((string)Qry.get().FieldAsString("status") == "E" ? crew_items: unacc_items);
        items_ref[Qry.get().FieldAsString("airp_arv")] = make_pair(
                Qry.get().FieldAsInteger("bag_amount"),
                Qry.get().FieldAsInteger("bag_weight"));
    }
}

struct TLCIPaxTotals:public SalonPaxList::TCurrPosHandler {
    private:
        TypeB::TDetailCreateInfo *Finfo;
        string airpByPointId(int point_id);
        bool isTrfer(int grp_id, string &trfer_airline, string &trfer_airp_arv);
        void get_bag_info(map<string, pair<int, int> > &bag_info, int &bag_pool_num, int grp_id, int pax_id);
        pair<int, int> get_bag_totals(map<string, pair<int, int> > &bag_info);
        void add_empty_cls_bag(const string &cat, const map<string, pair<int, int>> &bag_items);
    public:
        vector<TLCIPaxTotalsItem> items;
        TByClass pax_tot_by_cls;
        TEmptyClsBag empty_cls_bag;
        virtual void process();
        void get(TypeB::TDetailCreateInfo &info);
        void ToTlg(
                TypeB::TDetailCreateInfo &info,
                vector<string> &body,
                vector<string> &si
                );
        TLCIPaxTotals(): Finfo(NULL) {}
};

void TLCIPaxTotals::add_empty_cls_bag(const string &cat, const map<string, pair<int, int>> &bag_items)
{
    for(const auto &i: bag_items) {
        size_t idx = 0;
        for(; idx < items.size(); idx++)
            if(items[idx].airp == i.first) break;
        if(idx == items.size()) {
            items.push_back(TLCIPaxTotalsItem());
            items[idx].airp = i.first;
        }
        items[idx].bag_category[cat][""].first += i.second.first;
        items[idx].bag_category[cat][""].second += i.second.second;
    }
}

pair<int, int>  TLCIPaxTotals::get_bag_totals(map<string, pair<int, int> > &bag_info)
{
    pair<int, int> result;
    for(map<string, pair<int, int> >::iterator
            iRem = bag_info.begin();
            iRem != bag_info.end();
            iRem++) {
        result.first += iRem->second.first;
        result.second += iRem->second.second;
    }
    return result;
}

bool TLCIPaxTotals::isTrfer(int grp_id, string &trfer_airline, string &trfer_airp_arv)
{
    TCachedQuery Qry(
            "select "
            "   transfer.airp_arv, "
            "   trfer_trips.airline "
            "from "
            "   transfer, "
            "   trfer_trips "
            "where "
            "   transfer.grp_id = :grp_id and "
            "   transfer.transfer_num = 1 and "
            "   transfer.point_id_trfer = trfer_trips.point_id ",
            QParams() << QParam("grp_id", otInteger, grp_id));
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        trfer_airline = Qry.get().FieldAsString("airline");
        trfer_airp_arv = Qry.get().FieldAsString("airp_arv");
    }
    return not Qry.get().Eof;
}

void TLCIPaxTotals::get_bag_info(map<string, pair<int, int> > &bag_info, int &bag_pool_num, int grp_id, int pax_id)
{
    // bag_pool_num
    bag_pool_num = NoExists;
    TCachedQuery Qry("select bag_pool_num from pax where pax_id = :pax_id",
            QParams() << QParam("pax_id", otInteger, pax_id));
    Qry.get().Execute();
    if(Qry.get().Eof)
        throw Exception("TLCIPaxTotals::get_bag_info: bag_pool_num not found for pax_id %d", pax_id);
    if(not Qry.get().FieldIsNULL("bag_pool_num"))
        bag_pool_num = Qry.get().FieldAsInteger("bag_pool_num");

    if(bag_pool_num == NoExists) return;

    // pool_pax_id
    int pool_pax_id = NoExists;
    TCachedQuery poolPaxIdQry("select ckin.get_bag_pool_pax_id(:grp_id, :bag_pool_num) from dual",
            QParams()
            << QParam("grp_id", otInteger, grp_id)
            << QParam("bag_pool_num", otInteger, bag_pool_num));
    poolPaxIdQry.get().Execute();
    if(not poolPaxIdQry.get().FieldIsNULL(0))
        pool_pax_id = poolPaxIdQry.get().FieldAsInteger(0);

    // bag_info itself
    if(pool_pax_id != NoExists and pool_pax_id == pax_id) {
        TCachedQuery bagInfoQry(
                "select "
                "   decode(bag2.to_ramp, 0, bag_types.rem_code_lci, 'DAA') rem_code, "
                "   sum(decode(pr_cabin,0,amount,null)) bag_amount, "
                "   sum(decode(pr_cabin,0,weight,null)) bag_weight "
                "from "
                "   bag2, "
                "   bag_types "
                "where "
                "   grp_id = :grp_id and "
                "   bag_pool_num = :bag_pool_num and "
                "   bag2.bag_type = bag_types.code(+) "
                "group by "
                "   bag2.to_ramp, "
                "   bag_types.rem_code_lci ",
                QParams()
                << QParam("grp_id", otInteger, grp_id)
                << QParam("bag_pool_num", otInteger, bag_pool_num));
        bagInfoQry.get().Execute();
        for(; not bagInfoQry.get().Eof; bagInfoQry.get().Next())
            bag_info[bagInfoQry.get().FieldAsString("rem_code")] =
                pair<int, int>(
                        bagInfoQry.get().FieldAsInteger("bag_amount"),
                        bagInfoQry.get().FieldAsInteger("bag_weight")
                        );
    }
}

string TLCIPaxTotals::airpByPointId(int point_id)
{
    TCachedQuery Qry("select airp from points where point_id = :point_id",
            QParams() << QParam("point_id", otInteger, point_id));
    Qry.get().Execute();
    if(Qry.get().Eof)
        throw Exception("airpByPointId not found for point_id %d", point_id);
    return Qry.get().FieldAsString("airp");
}

// baggage categories
const string BCAT_BT = "BT"; // ?࠭???? (???. ??ᮯ? ????? ?஬? ?/??), ????? ???. ??????? ?? ????????
const string BCAT_BD = "BD"; // ?࠭???? ?? ?????. ??ॢ????, ? ??⠫쭮? ??? BT
const string BCAT_BI = "BI"; // ?࠭???? ?? ??. ??, ? ??⠫쭮? ??? BT
const string BCAT_BF = "BF"; // ??????
const string BCAT_BC = "BC"; // ??????
const string BCAT_BY = "BY"; // ??????
const string BCAT_BZ = "BZ"; // ????? ???. ???????
const string BCAT_BX = "BX"; // ??ᮯ?. ?????
const string BCAT_DT = "DT"; // ????? ???????

void TLCIPaxTotals::process()
{
    string airp_arv = airpByPointId(pax->point_arv);
    string airline = Finfo->airline;

    size_t idx = 0;
    for(; idx < items.size(); idx++)
        if(items[idx].airp == airp_arv) break;
    if(idx == items.size()) {
        items.push_back(TLCIPaxTotalsItem());
        items[idx].airp = airp_arv;
    }

    string trfer_airline, trfer_airp_arv;
    bool is_trfer = isTrfer(pax->grp_id, trfer_airline, trfer_airp_arv);

    map<string, pair<int, int> > bag_info; // bag_types.rem_code, amount, weight
    int bag_pool_num;
    get_bag_info(bag_info, bag_pool_num, pax->grp_id, pax->pax_id);

    for(map<string, pair<int, int> >::iterator
            iBag = bag_info.begin();
            iBag != bag_info.end();
            iBag++) {
        if(isExtraCrew(pax->crew_type)) {
            items[idx].bag_category[BCAT_BZ][""].first += iBag->second.first;
            items[idx].bag_category[BCAT_BZ][""].second += iBag->second.second;
        } else {
            string category;
            if(is_trfer) {
                TBaseTable &baseairps = base_tables.get( "airps" );
                TBaseTable &basecities = base_tables.get( "cities" );
                bool internal_flight =
                    ((const TCitiesRow&)basecities.get_row("code", ((const TAirpsRow&)baseairps.get_row("code", airp_arv)).city)).country == "??";
                internal_flight = internal_flight and
                    ((const TCitiesRow&)basecities.get_row("code", ((const TAirpsRow&)baseairps.get_row("code", trfer_airp_arv)).city)).country == "??";

                if(internal_flight) {
                    category = BCAT_BD;
                } else if(airline != trfer_airline) {
                    category = BCAT_BI;
                } else {
                    category = BCAT_BT;
                }
            } else {
                if(pax->cabin_cl == "?") {
                    category = BCAT_BF;
                } else
                if(pax->cabin_cl == "?") {
                    category = BCAT_BC;
                } else /*if(pax->cl == "?")*/ {
                    category = BCAT_BY;
                }
            }
            items[idx].bag_category[category][iBag->first].first += iBag->second.first;
            items[idx].bag_category[category][iBag->first].second += iBag->second.second;
        }
    }

    TWItem pax_bag;
    pax_bag.get(pax->grp_id, bag_pool_num);
    items[idx].rk_weight += pax_bag.rkWeight;

    pax_tot_by_cls[pax->cabin_cl].append(getGender(pax->pax_id));

    items[idx].cls_totals.items[pax->cabin_cl].pax_size++;

    if(isExtraCrew(pax->crew_type)) {
        items[idx].cls_totals.extra_items[pax->cabin_cl].bag_amount += get_bag_totals(bag_info).first;
        items[idx].cls_totals.extra_items[pax->cabin_cl].bag_weight += get_bag_totals(bag_info).second;
    } else {
        items[idx].cls_totals.items[pax->cabin_cl].bag_amount += get_bag_totals(bag_info).first;
        items[idx].cls_totals.items[pax->cabin_cl].bag_weight += get_bag_totals(bag_info).second;
    }
}

void TLCIPaxTotals::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body, vector<string> &si)
{
    ostringstream result;
    const TypeB::TLCIOptions &options = *info.optionsAs<TypeB::TLCIOptions>();
    for(vector<TLCIPaxTotalsItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
        if(options.pas_totals) {
            result.str(string());
            result
                << "-" << info.TlgElemIdToElem(etAirp, iv->airp) << ".PT."
                << iv->cls_totals.pax_size()
                << ".C."
                << iv->cls_totals.pax_size();
            body.push_back(result.str());
        }
        if(options.bag_totals) {
            result.str(string());
            result
                << "-" << info.TlgElemIdToElem(etAirp, iv->airp) << ".BT."
                << iv->cls_totals.bag_amount() + empty_cls_bag.bag_amount(iv->airp)
                << "/" << iv->cls_totals.bag_weight() + empty_cls_bag.bag_weight(iv->airp)
                << ".C."
                << iv->cls_totals.items["?"].bag_amount << "/"
                << iv->cls_totals.items["?"].bag_amount << "/"
                << iv->cls_totals.items["?"].bag_amount
                << ".A."
                << iv->cls_totals.items["?"].bag_weight << "/"
                << iv->cls_totals.items["?"].bag_weight << "/"
                << iv->cls_totals.items["?"].bag_weight
                << "." << KG;
            body.push_back(result.str());
            result.str(string());
            result
                << "-" << info.TlgElemIdToElem(etAirp, iv->airp) << ".H."
                << iv->rk_weight
                << "." << KG;
            body.push_back(result.str());

            iv->clsBagToTlg(info, si);
        }
    }
    if(options.pas_distrib) {
        result.str(string());
        result << "PD.C." << info.TlgElemIdToElem(etClass, "?") << "."
            << pax_tot_by_cls["?"].m << "/"
            << pax_tot_by_cls["?"].f << "/"
            << pax_tot_by_cls["?"].c << "/"
            << pax_tot_by_cls["?"].i << "."
            << info.TlgElemIdToElem(etClass, "?") << "."
            << pax_tot_by_cls["?"].m << "/"
            << pax_tot_by_cls["?"].f << "/"
            << pax_tot_by_cls["?"].c << "/"
            << pax_tot_by_cls["?"].i << "."
            << info.TlgElemIdToElem(etClass, "?") << "."
            << pax_tot_by_cls["?"].m << "/"
            << pax_tot_by_cls["?"].f << "/"
            << pax_tot_by_cls["?"].c << "/"
            << pax_tot_by_cls["?"].i;
        body.push_back(result.str());
    }
}

string get_grp_cls(int pax_id)
{
    TCachedQuery Qry(
            "select class from pax_grp, pax where "
            "   pax.pax_id = :pax_id and "
            "   pax.grp_id = pax_grp.grp_id ",
            QParams() << QParam("pax_id", otInteger, pax_id));
    Qry.get().Execute();
    string result;
    if(not Qry.get().Eof)
        result = Qry.get().FieldAsString("class");
    return result;
}

void TLCIPaxTotals::get(TypeB::TDetailCreateInfo &info)
{
    Finfo = &info;
    const TypeB::TLCIOptions &options = *info.optionsAs<TypeB::TLCIOptions>();
    SalonPaxList::TSalonPaxList pax_list;
    pax_list.get(SalonPaxList::curr_point_flags(), info.point_id);
    pax_list.set_handler(this);
    pax_list.iterate();

    if(options.version == "WB") {
        empty_cls_bag.get(info.point_id);
        add_empty_cls_bag(BCAT_BX, empty_cls_bag.unacc_items);
        add_empty_cls_bag(BCAT_DT, empty_cls_bag.crew_items);
    }
}

struct TSeatPlan {
    private:
        template <typename T>
            void fill_seats(TypeB::TDetailCreateInfo &info, const T &inserter);
        string getXCRType(int pax_id);
        void append_crew_type(std::string &seat, ASTRA::TCrewType::Enum crew_type, const std::string &xcr_type);
    public:
        map<int,TCheckinPaxSeats> checkinPaxsSeats;
        void get(TypeB::TDetailCreateInfo &info);
        void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

string TSeatPlan::getXCRType(int pax_id)
{
    string result;
    multiset<CheckIn::TPaxRemItem> rems;
    LoadPaxRem(pax_id, rems);
    for(multiset<CheckIn::TPaxRemItem>::iterator
            i = rems.begin();
            i != rems.end(); i++) {
        if(i->code == TCrewTypes().encode(TCrewType::ExtraCrew)) {
            boost::match_results<std::string::const_iterator> results;
            static const boost::regex e("^XCR ([12])$");
            if(boost::regex_match(i->text, results, e)) {
                result = results[1];
            }
        }
    }
    return result;
}

void TSeatPlan::get(TypeB::TDetailCreateInfo &info)
{
    const TypeB::TLCIOptions &options = *info.optionsAs<TypeB::TLCIOptions>();
    if(options.seat_plan) {
        if(isFreeSeating(info.point_id))
            throw UserException("MSG.SALONS.FREE_SEATING");
        if(isEmptySalons(info.point_id))
            throw UserException("MSG.FLIGHT_WO_CRAFT_CONFIGURE");
        getSalonPaxsSeats(info.point_id, checkinPaxsSeats, true);
    }
}

void TSeatPlan::append_crew_type(string &seat, ASTRA::TCrewType::Enum crew_type, const string &xcr_type)
{
    switch(crew_type) {
        case TCrewType::ExtraCrew:
            {
                if(not xcr_type.empty())
                    seat += "/" + xcr_type;
            }
            break;
        case TCrewType::DeadHeadCrew:
            seat += "/D";
            break;
        case TCrewType::MiscOperStaff:
            seat += "/M";
            break;
        default:
            break;
    }
}

template <typename T>
void TSeatPlan::fill_seats(TypeB::TDetailCreateInfo &info, const T &inserter)
{
    const TypeB::TLCIOptions &options = *info.optionsAs<TypeB::TLCIOptions>();
    for(map<int,TCheckinPaxSeats>::iterator im = checkinPaxsSeats.begin(); im != checkinPaxsSeats.end(); im++) {
        CheckIn::TSimplePaxItem pax;
        pax.getByPaxId(im->first);
        string xcr_type = getXCRType(im->first);
        // g stands for 'gender'; First iteration - seats for adult, second iteration - one seat for infant
        for(int g = 0; g <=1; g++) {
            string gender;
            if(g == 0)
                gender = im->second.gender;
            else {
                if(im->second.pr_infant == NoExists)
                    break;
                else
                    gender = 'I';
            }
            // 横? ?? ???⠬ ⥪?饣? ????
            for(set<TTlgCompLayer,TCompareCompLayers>::iterator is = im->second.seats.begin(); is != im->second.seats.end(); is++) {
                string seat =
                    "." + is->denorm_view(info.is_lat() or info.pr_lat_seat);
                if(options.version == "WB") {
                    if(pax.isCBBG())
                        seat += "/B"; // ⠪ ???????????? CBBG (⠪??, ??? ? extra seats)
                    else {
                        if(is == im->second.seats.begin())
                            seat += "/" + gender;
                        if(is != im->second.seats.begin())
                            seat += "/B"; // ⠪ ???????????? ???. ????? (extra seats)
                        else
                            append_crew_type(seat, im->second.crew_type, xcr_type);
                    }
                } else
                    seat += "/" + gender;
                inserter.do_insert(seat, is->point_arv);
                if(g == 1) break; // ??? ??䠭?? ????⠥? ⮫쪮 ??ࢮ? ?????
            }
        }
    }
    // ??????? JMP
    if(options.version == "WB") {
        TCachedQuery Qry(
                "select "
                "   pax.*, "
                "   nvl(pax.cabin_class, pax_grp.class) class, "
                "   pax_grp.point_arv "
                "from "
                "   pax, "
                "   pax_grp "
                "   where "
                "   pax_grp.grp_id=pax.grp_id and "
                "   pax_grp.point_dep=:point_id and "
                "   pax_grp.status not in ('E') and "
                "   pax.is_jmp <> 0 and "
                "   pax.grp_id = pax_grp.grp_id and "
                "   pax.pr_brd is not null ",
                QParams() << QParam("point_id", otInteger, info.point_id));
        Qry.get().Execute();
        for(; not Qry.get().Eof; Qry.get().Next()) {
            string seat =
                ".J" + info.TlgElemIdToElem(etClass, Qry.get().FieldAsString("class")) + "/" +
                TlgTrickyGenders().encode(getGender(Qry.get()));
            string xcr_type = getXCRType(Qry.get().FieldAsInteger("pax_id"));
            ASTRA::TCrewType::Enum crew_type = TCrewTypes().decode(Qry.get().FieldAsString("crew_type"));
            append_crew_type(seat, crew_type, xcr_type);
            inserter.do_insert(seat, Qry.get().FieldAsInteger("point_arv"));
        }
    }
}

struct TAHMInserter {
    string &buf;
    vector<string> &body;
    void do_insert(const string &seat, int point_arv) const // point_arv not used
    {
        if(buf.size() + seat.size() > LINE_SIZE) {
            body.push_back(buf);
            buf = "SP";
        }
        buf += seat;
    }
    TAHMInserter(string &abuf, vector<string> &abody):
        buf(abuf),
        body(abody)
    {}
};

struct TWBInserter {
    map<int, vector<string> > &wb_seats;
    void do_insert(const string &seat, int point_arv) const
    {
        wb_seats[point_arv].push_back(seat);
    }
    TWBInserter(map<int, vector<string> > &awb_seats):
        wb_seats(awb_seats)
    {}
};

void TSeatPlan::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    const TypeB::TLCIOptions &options = *info.optionsAs<TypeB::TLCIOptions>();
    if(not options.seat_plan) return;
    if(options.version == "AHM") {
        string buf = "SP";
        fill_seats<TAHMInserter>(info, TAHMInserter(buf, body));
        if(buf != "SP")
            body.push_back(buf);
    } else if(options.version == "WB") {
        map<int, vector<string> > wb_seats;
        fill_seats<TWBInserter>(info, TWBInserter(wb_seats));
        TTripRoute route;
        route.GetRouteAfter(NoExists, info.point_id, trtNotCurrent, trtNotCancelled);
        for(TTripRoute::iterator i = route.begin(); i != route.end(); i++) {
            map<int, vector<string> >::iterator idx = wb_seats.find(i->point_id);
            if(idx != wb_seats.end()) {
                string sp_header = "-" + info.TlgElemIdToElem(etAirp, i->airp) + ".SP.WB";
                string buf = sp_header;
                for(vector<string>::iterator seat_i = idx->second.begin(); seat_i != idx->second.end(); ++seat_i) {
                    if(buf.size() + seat_i->size() > LINE_SIZE) {
                        body.push_back(buf);
                        buf = sp_header;
                    }
                    buf += *seat_i;
                }
                if(buf != sp_header) body.push_back(buf);
            }
        }
    }
}

struct TLCI {
    TLCICFG eqt;
    TWA wa;
    TSR_WB_C sr_wb_c; // ???? SR.WB.C - ???䨣 ᠫ??? - ??????頥??? ? WBW
    TSR_C sr_c;
    TSR_Z sr_z;
    TSR_S sr_s;
    TWM wm; // weight mode
    TLCIPaxTotals pax_totals;
    TSeatPlan sp;
    TBagRems bag_rems;
    TToRampBag to_ramp;
    TExtraCrew extra_crew;
    string get_action_code(TypeB::TDetailCreateInfo &info);
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

void TLCI::get(TypeB::TDetailCreateInfo &info)
{
    const TypeB::TLCIOptions &options = *info.optionsAs<TypeB::TLCIOptions>();
    info.vcompleted = true;
    if(options.equipment) {
        eqt.get(info.point_id);
        if(eqt.empty()) throw UserException("MSG.CFG.EMPTY");
    }
    if(options.weight_avail != "N") wa.get(info);
    try {
        sr_c.get(info);
        sr_z.get(info);
        sr_s.get(info);
        pax_totals.get(info);
        sp.get(info);
        extra_crew.get(info.point_id);
        if(options.bag_totals) {
            //bag_rems.get(info);
            //to_ramp.get(info.point_id, TToRampBag::rbReg);
        }
    } catch(AstraLocale::UserException &E) {
        if(E.getLexemaData().lexema_id != "MSG.FLIGHT_WO_CRAFT_CONFIGURE")
            throw;
    }
}

string TLCI::get_action_code(TypeB::TDetailCreateInfo &info)
{
    string result;
    if(info.create_point.time_offset == 0) {
        switch(info.create_point.stage_id) {
            case sOpenCheckIn:
                result = "O";
                break;
            case sCloseCheckIn:
                result = "C";
                break;
            case sCloseBoarding:
                result = "U";
                break;
            case sTakeoff:
                result = "F";
                break;
            case sNoActive:
                {
                    // ??????????? ⫣. . ?⢥⮬ ?? LCI-??????
                    // ?.?. ??? ?맢?? ??????????? TCreatePoint() ??? ??ࠬ??஢
                    // ? ??????? ??????? (lci_parser.cpp)
                    // ?? 㬮?砭??:
                    // create_point.stage_id = sNoActive
                    // create_point.offset = 0
                    TTripStage ts;
                    TTripStages::LoadStage(info.point_id, sCloseCheckIn, ts);
                    bool close_checkin = ts.act != NoExists;
                    if(close_checkin)
                        result = "F";
                    else
                        result = "U";
                    break;
                }
            default:
                result = "U";
                break;
        }
    } else {
        result = "U";
    }
    return result;
}

void TLCI::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    const TypeB::TLCIOptions &options = *info.optionsAs<TypeB::TLCIOptions>();
    body.push_back("C" + get_action_code(info));
    eqt.ToTlg(info, body);
    wa.ToTlg(info, body);
    if(options.seating) body.push_back("SM.S"); // Seating method 'By Seat' always
    sr_wb_c.ToTlg(info, body);
    if(options.seat_restrict.find('C') != string::npos) sr_c.ToTlg(info, body);
    if(options.seat_restrict.find('Z') != string::npos) sr_z.ToTlg(info, body);
    if(options.seat_restrict.find('S') != string::npos) sr_s.ToTlg(info, body);
    if(options.weight_mode) wm.ToTlg(info, body);
    vector<string> si;
    pax_totals.ToTlg(info, body, si);
    sp.ToTlg(info, body);
    if(options.bag_totals) {
//        bag_rems.ToTlg(info, si);
//        to_ramp.ToTlg(info, si);
    }
    extra_crew.ToTlg(info, si);
    if(not si.empty()) {
        body.push_back("SI");
        body.insert(body.end(), si.begin(), si.end());
    }
}

int LCI(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading;
    heading << "LCI" << TypeB::endl
            << info.flight_view() << "/"
            << DateTimeToStr(info.scd_utc, "ddmmm", 1) << "." << info.airp_dep_view() << TypeB::endl;
    tlg_row.heading = heading.str();
    size_t part_len = tlg_row.textSize();
    vector<string> body;
    try {
        TLCI lci;
        lci.get(info);
        lci.ToTlg(info, body);
    } catch(...) {
        ExceptionFilter(body, info);
    }
    for(vector<string>::iterator iv = body.begin(); iv != body.end(); iv++) {
        part_len += iv->size() + TypeB::endl.size();
        if(part_len > PART_SIZE) {
            tlg_draft.Save(tlg_row);
            tlg_row.body = *iv + TypeB::endl;
            part_len = tlg_row.textSize();
        } else
            tlg_row.body += *iv + TypeB::endl;
    }
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

int PIM(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading;
    heading << "PIM" << TypeB::endl
            << info.flight_view() << "/"
            << info.scd_local_view() << " " << info.airp_dep_view() << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
    size_t part_len = tlg_row.textSize();
    vector<string> body;
    try {
        TPIMBody PIM;
        PIM.get(info);
        PIM.ToTlg(info, body);
    } catch(...) {
        ExceptionFilter(body, info);
    }
    split_n_save(heading, part_len, tlg_draft, tlg_row, body);
    tlg_row.ending = "ENDPIM" + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

int FTL(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading;
    heading << "FTL" << TypeB::endl
            << info.flight_view() << "/"
            << info.scd_local_view() << " " << info.airp_dep_view() << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
    size_t part_len = tlg_row.textSize();
    vector<string> body;
    try {
        TFTLBody FTL;
        FTL.get(info);
        FTL.ToTlg(info, body);
    } catch(...) {
        ExceptionFilter(body, info);
    }
    split_n_save(heading, part_len, tlg_draft, tlg_row, body);
    tlg_row.ending = "ENDFTL" + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

int ETL(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading;
    heading << "ETL" << TypeB::endl
            << info.flight_view() << "/"
            << info.scd_local_view() << " " << info.airp_dep_view() << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
    size_t part_len = tlg_row.textSize();
    vector<string> body;
    try {
        TETLCFG cfg;
        cfg.get(info.point_id);
        cfg.ToTlg(info, body);
        if(info.act_local != NoExists) {
            body.push_back("ATD/" + DateTimeToStr(info.act_local, "ddhhnn"));
        }

        vector<TTlgCompLayer> complayers;
        TDestList<TETLDest> dests;
        dests.get(info,complayers);
        dests.ToTlg(info, body);
    } catch(...) {
        ExceptionFilter(body, info);
    }
    split_n_save(heading, part_len, tlg_draft, tlg_row, body);
    tlg_row.ending = "ENDETL" + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

int ASL(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading;
    heading << "ASL" << TypeB::endl
            << info.flight_view() << "/"
            << info.scd_local_view() << " " << info.airp_dep_view() << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
    size_t part_len = tlg_row.textSize();
    vector<string> body;
    try {
        TETLCFG cfg;
        cfg.get(info.point_id);
        cfg.ToTlg(info, body);
        if(info.act_local != NoExists) {
            body.push_back("ATD/" + DateTimeToStr(info.act_local, "ddhhnn"));
        }

        vector<TTlgCompLayer> complayers;
        TDestList<TASLDest> dests;
        dests.get(info,complayers);
        dests.ToTlg(info, body);
    } catch(...) {
        ExceptionFilter(body, info);
    }
    split_n_save(heading, part_len, tlg_draft, tlg_row, body);
    tlg_row.ending = "ENDASL" + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

int FWD(TypeB::TDetailCreateInfo &info)
{
    TypeB::TForwardOptions *forwarderOptions=NULL;
    if (info.optionsIs<TypeB::TForwardOptions>())
        forwarderOptions=info.optionsAs<TypeB::TForwardOptions>();
    if (forwarderOptions==NULL) throw Exception("%s: forwarderOptions expected", __FUNCTION__);
    if (forwarderOptions->typeb_in_id==NoExists ||
            forwarderOptions->typeb_in_num==NoExists) throw Exception("%s: forwarderOptions not defined", __FUNCTION__);
    TCachedQuery Qry("SELECT heading, ending FROM tlgs_in WHERE id=:tlg_id AND num=:tlg_num",
            QParams() << QParam("tlg_id", otInteger, forwarderOptions->typeb_in_id)
            << QParam("tlg_num", otInteger, forwarderOptions->typeb_in_num));
    Qry.get().Execute();
    if (Qry.get().Eof) throw Exception("%s: forwarded telegram not found", __FUNCTION__);
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);

    TMemoryManager mem(STDLOG);
    TypeB::THeadingInfo *HeadingInfo=NULL;

    try
    {
        try
        {
            TypeB::TFlightsForBind bind_flts;
            TypeB::TTlgPartInfo part;
            string heading=Qry.get().FieldAsString("heading");
            part.p=heading.c_str();
            ParseHeading(part,HeadingInfo,bind_flts,mem);
        }
        catch(std::exception &E)
        {
            throw Exception("%s: header's parse error", __FUNCTION__);
        };

        TypeB::TUCMHeadingInfo *UCMHeadingInfo=dynamic_cast<TypeB::TUCMHeadingInfo*>(HeadingInfo);

        if (!forwarderOptions->forwarding)
        {
            mem.destroy(HeadingInfo, STDLOG);
            if (HeadingInfo!=NULL) delete HeadingInfo;
            return NoExists;
        };

        tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
        ostringstream heading;

        // feel the difference
        // UCMHeadingInfo->tlg_type = "UCM"
        // tlg_row.tlg_type = "UCM->>" - for forwarding tlgs

        heading << UCMHeadingInfo->tlg_type << TypeB::endl
            << UCMHeadingInfo->flt_info.src << TypeB::endl;
        tlg_row.heading = heading.str();
        tlg_row.body = getTypeBBody(forwarderOptions->typeb_in_id,
                forwarderOptions->typeb_in_num);
        if (tlg_row.body.size()>4000) throw UserException("MSG.TLG.VERY_BIG_FOR_FORWARDING");
        tlg_row.ending = Qry.get().FieldAsString("ending");

        mem.destroy(HeadingInfo, STDLOG);
        if (HeadingInfo!=NULL) delete HeadingInfo;
    }
    catch(...)
    {
        mem.destroy(HeadingInfo, STDLOG);
        if (HeadingInfo!=NULL) delete HeadingInfo;
        throw;
    };

    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

struct TPaxMapCoord { // coordinates
    int point_dep;
    int point_arv;
    string cls;
    string grp_status;
    void dump();
};

void TPaxMapCoord::dump()
{
    LogTrace(TRACE5) << "-----TPaxMapCoord::dump()-------";
    LogTrace(TRACE5) << "point_dep: " << point_dep;
    LogTrace(TRACE5) << "point_arv: " << point_arv;
    LogTrace(TRACE5) << "cls: " << cls;
    LogTrace(TRACE5) << "grp_status: " << grp_status;
    LogTrace(TRACE5) << "--------------------------------";
}

struct TIDM {
    TypeB::TDetailCreateInfo &info;
    vector<TTlgCompLayer> &complayers;
    bool pr_tranz_reg;

    struct TPax {
        TName name;
        string priority;
        string cls;
        string airp_arv;
        string seat;
    };
    list<TPax> items;

    void appendArv(
            TSalonPassengers::iterator iDep,
            TIntArvSalonPassengers &arvMap,
            TTripRouteItem &routeItem
            );
    void get();
    void ToTlg(vector<string> &body);
    void append_evt_transit(
            TPaxMapCoord &pax_map_coord,
            const TSalonPax &pax,
            TTripRouteItem &routeItem
            );

    TIDM(TypeB::TDetailCreateInfo &_info, vector<TTlgCompLayer> &_complayers):
        info(_info),
        complayers(_complayers),
        pr_tranz_reg(false)
    {}
};

void TIDM::append_evt_transit(
        TPaxMapCoord &pax_map_coord,
        const TSalonPax &pax,
        TTripRouteItem &routeItem
        )
{
    // ?᫨ ???. ????窠 '????ॣ???????? ?࠭????', ??
    // ? ????稪? ???????? pax.grp_status == psTransit
    // ?᫨ ????窠 ?몫?祭?, ?? ?????, ????騥 ??१ ⥪. point_id (routeItem.point_id)
    if(
            (pr_tranz_reg and DecodePaxStatus(pax.grp_status.c_str()) == psTransit)
            or
            (not pr_tranz_reg and
             pax.point_dep == info.point_id and
             pax_map_coord.point_dep == routeItem.point_id)
      ) {
        TCachedQuery Qry(
                "select "
                "   crs_pnr.status, "
                "   crs_pnr.priority "
                "from "
                "   crs_pax, "
                "   crs_pnr "
                "where "
                "   crs_pax.pax_id = :pax_id and "
                "   crs_pax.pnr_id = crs_pnr.pnr_id and "
                "   crs_pnr.system = 'CRS' ",
                QParams() << QParam("pax_id", otInteger, pax.pax_id));
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            string status = Qry.get().FieldAsString("status");
            string priority = Qry.get().FieldAsString("priority");
            if(
                    status == "DG2" or
                    status == "ID2" or
                    status == "RG2"
              ) {
                TPax pax_item;
                pax_item.name.name = pax.name;
                pax_item.name.surname = pax.surname;
                pax_item.priority = priority;
                pax_item.cls = info.TlgElemIdToElem(etClass, pax.cabin_cl);

                TTripInfo trip_info;
                trip_info.getByPointId(pax.point_arv);
                pax_item.airp_arv = info.TlgElemIdToElem(etAirp, trip_info.airp);

                TTlgSeatList seat_no;
                seat_no.add_seats(pax.pax_id, complayers);
                vector<string> seat_list = seat_no.get_seat_vector(info.is_lat());
                if(not seat_list.empty())
                    pax_item.seat = seat_list.front();
                items.push_back(pax_item);
            }
        }
    }
}

void TIDM::appendArv(
        TSalonPassengers::iterator iDep,
        TIntArvSalonPassengers &arvMap,
        TTripRouteItem &routeItem
        )
{
    for(TIntArvSalonPassengers::const_iterator
            iArv = arvMap.begin();
            iArv != arvMap.end();
            iArv++) {
        for(TIntClassSalonPassengers::const_iterator
                iCls = iArv->second.begin();
                iCls != iArv->second.end();
                iCls++) {
            for(TIntStatusSalonPassengers::const_iterator
                    iStatus = iCls->second.begin();
                    iStatus != iCls->second.end();
                    iStatus++) {
                for(set<TSalonPax,ComparePassenger>::const_iterator
                        iPax = iStatus->second.begin();
                        iPax != iStatus->second.end();
                        iPax++) {
                    TPaxMapCoord pax_map_coord;
                    pax_map_coord.point_dep = iDep->first;
                    pax_map_coord.point_arv = iArv->first;
                    pax_map_coord.cls = iCls->first;
                    pax_map_coord.grp_status = iStatus->first;
                    append_evt_transit(pax_map_coord, *iPax, routeItem);
                }
            }
        }
    }
}

void TIDM::get()
{
    // fetch pr_tranz_reg
    TCachedQuery Qry("select pr_tranz_reg from trip_sets where point_id = :point_id",
            QParams() << QParam("point_id", otInteger, info.point_id));
    Qry.get().Execute();
    pr_tranz_reg = false;
    if(not Qry.get().Eof and not Qry.get().FieldIsNULL("pr_tranz_reg"))
        pr_tranz_reg = Qry.get().FieldAsInteger("pr_tranz_reg") != 0;

    SALONS2::TSalonList salonList;
    SALONS2::TGetPassFlags flags;

    try {
        salonList.ReadFlight( SALONS2::TFilterRoutesSets( info.point_id, ASTRA::NoExists ),
                              "", NoExists );
    } catch(const Exception &E) {
        LogTrace(TRACE5) << "TIDM::get: salonList.ReadFlight failed: " << E.what();
    } catch(...) {
        LogTrace(TRACE5) << "TIDM::get: salonList.ReadFlight failed: unexpected";
    }


    TSalonPassengers tranzit_pax_map;
    try {
        flags.setFlag( SALONS2::gpWaitList );
        flags.setFlag( SALONS2::gpTranzits );
        flags.setFlag( SALONS2::gpInfants );
        salonList.getPassengers( tranzit_pax_map, flags );
    } catch(const Exception &E) {
        LogTrace(TRACE5) << "TIDM::get: tranzit_pax_map failed: " << E.what();
    } catch(...) {
        LogTrace(TRACE5) << "TIDM::get: tranzit_pax_map failed: unexpected";
    }

    TTripRoute trip_route;
    try {
        trip_route.GetRouteAfter(NoExists, info.point_id, trtNotCurrent, trtNotCancelled);
    } catch(const Exception &E) {
        LogTrace(TRACE5) << "TIDM::get: trip_route failed: " << E.what();
    } catch(...) {
        LogTrace(TRACE5) << "TIDM::get: trip_route failed: unexpected";
    }

    // ??? ?????????? ?࠭??⭨??, ????騥 ??१ ?㭪?, ᫥???騩 ??᫥ ⥪?饣?.
    TTripRoute::iterator iRoute = trip_route.begin();

    for(TSalonPassengers::iterator
            iDep = tranzit_pax_map.begin();
            iDep != tranzit_pax_map.end();
            iDep++) {
        // tranzit
        appendArv(iDep, iDep->second.infants, *iRoute);
        appendArv(iDep, iDep->second, *iRoute);
    }
}

void TIDM::ToTlg(vector<string> &body)
{
    if(items.empty())
        body.push_back("NIL");
    else {
        for(list<TPax>::iterator i = items.begin(); i != items.end(); i++) {
            ostringstream row;
            row
                << i->name.ToPILTlg(info) << " "
                << i->priority << " "
                << i->cls << " ";
            if(not i->seat.empty())
                row << i->seat << " ";
            row << i->airp_arv;
            body.push_back(row.str());
        }
    }
}

int TPL(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading;
    heading << "TPL" << TypeB::endl
            << info.flight_view() << "/"
            << info.scd_local_view() << " " << info.airp_dep_view() << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
    size_t part_len = tlg_row.textSize();

    vector<string> body;
    try {
        vector<TTlgCompLayer> complayers;
        TDestList<TTPLDest> dests;
        dests.get(info,complayers);
        dests.ToTlg(info, body);
    } catch(...) {
        ExceptionFilter(body, info);
    }

    split_n_save(heading, part_len, tlg_draft, tlg_row, body);
    tlg_row.ending = "ENDTPL" + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

int IDM(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading;
    heading << "IDM" << TypeB::endl
            << info.flight_view() << "/"
            << info.scd_local_view() << " " << info.airp_dep_view() << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
    size_t part_len = tlg_row.textSize();

    vector<string> body;
    try {
        TTlgCompLayerList complayers;
        if(not isFreeSeating(info.point_id) and not isEmptySalons(info.point_id))
            getSalonLayers( info, complayers, false );
        TIDM idm(info, complayers);
        idm.get();
        idm.ToTlg(body);
    } catch(...) {
        ExceptionFilter(body, info);
    }

    split_n_save(heading, part_len, tlg_draft, tlg_row, body);
    tlg_row.ending = "ENDIDM" + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

int PNL(TypeB::TDetailCreateInfo &info)
{
  TypeB::TPNLADLOptions *forwarderOptions=NULL;
  if (info.optionsIs<TypeB::TPNLADLOptions>())
    forwarderOptions=info.optionsAs<TypeB::TPNLADLOptions>();
  if (forwarderOptions==NULL) throw Exception("%s: forwarderOptions expected", __FUNCTION__);

  if (forwarderOptions->typeb_in_id==NoExists ||
      forwarderOptions->typeb_in_num==NoExists) throw Exception("%s: forwarderOptions not defined", __FUNCTION__);

  TCachedQuery Qry("SELECT heading, ending FROM tlgs_in WHERE id=:tlg_id AND num=:tlg_num",
                   QParams() << QParam("tlg_id", otInteger, forwarderOptions->typeb_in_id)
                             << QParam("tlg_num", otInteger, forwarderOptions->typeb_in_num));
  Qry.get().Execute();
  if (Qry.get().Eof) throw Exception("%s: forwarded telegram not found", __FUNCTION__);

  TTlgDraft tlg_draft(info);
  TTlgOutPartInfo tlg_row(info);

  TMemoryManager mem(STDLOG);
  TypeB::THeadingInfo *HeadingInfo=NULL;
  try
  {

    try
    {
      TypeB::TFlightsForBind bind_flts;
      TypeB::TTlgPartInfo part;
      string heading=Qry.get().FieldAsString("heading");
      part.p=heading.c_str();
      ParseHeading(part,HeadingInfo,bind_flts,mem);
    }
    catch(std::exception &E)
    {
      throw Exception("%s: header's parse error", __FUNCTION__);
    };

    TypeB::TDCSHeadingInfo *DCSHeadingInfo=dynamic_cast<TypeB::TDCSHeadingInfo*>(HeadingInfo);
    if (DCSHeadingInfo==NULL) throw Exception("%s: not DCS header", __FUNCTION__);
    if (!forwarderOptions->forwarding ||
        (!forwarderOptions->crs.empty() && DCSHeadingInfo->sender!=forwarderOptions->crs))
    {
      mem.destroy(HeadingInfo, STDLOG);
      if (HeadingInfo!=NULL) delete HeadingInfo;
      return NoExists;
    };

    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    tlg_row.tlg_type = DCSHeadingInfo->tlg_type;
    ostringstream heading;
    heading << tlg_row.tlg_type << TypeB::endl
            << info.flight_view() << "/"
            << info.scd_local_view() << " " << info.airp_dep_view() << " "
            << "PART" << DCSHeadingInfo->part_no << TypeB::endl;
    if (DCSHeadingInfo->association_number!=NoExists)
      heading << "ANA/" << DCSHeadingInfo->association_number << TypeB::endl;
    tlg_row.heading = heading.str();
    tlg_row.body = getTypeBBody(forwarderOptions->typeb_in_id,
                                forwarderOptions->typeb_in_num);
    if (tlg_row.body.size()>4000) throw UserException("MSG.TLG.VERY_BIG_FOR_FORWARDING");
    tlg_row.ending = Qry.get().FieldAsString("ending");

    mem.destroy(HeadingInfo, STDLOG);
    if (HeadingInfo!=NULL) delete HeadingInfo;
  }
  catch(...)
  {
    mem.destroy(HeadingInfo, STDLOG);
    if (HeadingInfo!=NULL) delete HeadingInfo;
    throw;
  };

  tlg_draft.Save(tlg_row);
  tlg_draft.Commit(tlg_row);
  return tlg_row.id;
}

struct TSubclsItem {
    int priority;
    string cls;
    TSubclsItem(int vpriority, string vcls):
        priority(vpriority), cls(vcls) {}
    bool operator < (const TSubclsItem &val) const
    {
        if(priority != val.priority)
            return priority < val.priority;
        return cls < val.cls;
    }
};

template <class T>
void TDestList<T>::get_subcls_lst(TypeB::TDetailCreateInfo &info, list<string> &lst)
{
    const TypeB::TPRLOptions *PRLOptions=NULL;
    if(info.optionsIs<TypeB::TPRLOptions>())
        PRLOptions=info.optionsAs<TypeB::TPRLOptions>();
    const TypeB::TETLOptions *ETLOptions=NULL;
    if(info.optionsIs<TypeB::TETLOptions>())
        ETLOptions=info.optionsAs<TypeB::TETLOptions>();

    set<TSubclsItem> subcls_set;

    if(
            (PRLOptions and PRLOptions->rbd) or
            (ETLOptions and ETLOptions->rbd)
            ) {
        QParams QryParams;
        QryParams << QParam("point_id", otInteger, info.point_id);
        TCachedQuery Qry(
                "select distinct nvl(nvl(pax.cabin_subclass, pax.subclass), nvl(pax.cabin_class, pax_grp.class)) subcls from pax, pax_grp "
                "where pax_grp.point_dep = :point_id and pax_grp.grp_id = pax.grp_id and "
                "   pax_grp.status NOT IN ('E') ",
                QryParams);
        Qry.get().Execute();
        for(; not Qry.get().Eof; Qry.get().Next())
            subcls_set.insert(
                    TSubclsItem(
                        0,
                        Qry.get().FieldAsString("subcls")
                        )
                    );
    } else {
        // ???⠥? ??? ????????? ?????? ? ?????????, ?ᯮ?????騥?? ?? ३??
        TCFG cfg(info.point_id);
        if(cfg.empty()) cfg.get(NoExists);

        for(TCFG::iterator i = cfg.begin(); i != cfg.end(); i++)
            subcls_set.insert(TSubclsItem(i->priority, i->cls));

        QParams QryParams;
        QryParams << QParam("point_id", otInteger, info.point_id);
        TCachedQuery Qry(
                "SELECT DISTINCT cls_grp.priority, cls_grp.code AS class "
                "FROM pax_grp,pax,cls_grp "
                "WHERE nvl(pax.cabin_class_grp, pax_grp.class_grp)=cls_grp.id AND "
                "      pax_grp.grp_id = pax.grp_id and "
                "      pax_grp.point_dep = :point_id AND pax_grp.bag_refuse=0 ",
                QryParams);
        Qry.get().Execute();
        for(; not Qry.get().Eof; Qry.get().Next())
            subcls_set.insert(
                    TSubclsItem(
                        Qry.get().FieldAsInteger("priority"),
                        Qry.get().FieldAsString("class")
                        )
                    );
    }

    // ????? ????? ᯨ᮪ ??? priority, cls
    // ???६ ?㡫????? ? ???????? ?⢥?
    set<string> used;
    for(set<TSubclsItem>::iterator i = subcls_set.begin(); i != subcls_set.end(); i++) {
        if(used.find(i->cls)  == used.end()) {
            used.insert(i->cls);
            lst.insert(lst.end(), i->cls);
        }
    }
}

template <class T>
void TDestList<T>::get(TypeB::TDetailCreateInfo &info,vector<TTlgCompLayer> &complayers)
{
    infants.get(info);
    TTripRoute route;
    route.GetRouteAfter(NoExists, info.point_id, trtNotCurrent, trtNotCancelled);
    list<string> subcls_lst;
    get_subcls_lst(info, subcls_lst);
    for(TTripRoute::iterator i_route = route.begin(); i_route != route.end(); i_route++)
        for(list<string>::iterator i_cfg = subcls_lst.begin(); i_cfg != subcls_lst.end(); i_cfg++) {
            T dest(&grp_map, &infants);
            dest.airp = i_route->airp;
            dest.cls = *i_cfg;
            dest.GetPaxList(info,complayers);
            items.push_back(dest);
        }
}

struct TNumByDestItem {
    int f, c, y;
    void add(string cls, int seats);
    void ToTlg(TypeB::TDetailCreateInfo &info, string airp, vector<string> &body);
    TNumByDestItem():
        f(0),
        c(0),
        y(0)
    {};
};

void TNumByDestItem::ToTlg(TypeB::TDetailCreateInfo &info, string airp, vector<string> &body)
{
    ostringstream buf;
    buf
        << info.TlgElemIdToElem(etAirp, airp)
        << " "
        << setw(2) << setfill('0') << f << "/"
        << setw(3) << setfill('0') << c << "/"
        << setw(3) << setfill('0') << y;
    body.push_back(buf.str());
}

void TNumByDestItem::add(string cls, int seats)
{
    if(cls.empty())
        return;
    switch(cls[0])
    {
        case '?':
            f += seats;
                break;
        case '?':
            c += seats;
                break;
        case '?':
            y += seats;
                break;
        default:
            throw Exception("TNumByDestItem::add: strange cls: %s", cls.c_str());
    }
}

struct TPNLPaxInfo {
    private:
        TQuery Qry;
        int col_pax_id;
        int col_pnr_id;
        int col_surname;
        int col_name;
        int col_seats;
        int col_pers_type;
        int col_subclass;
        int col_target;
        int col_status;
        int col_crs;
    public:
        int pax_id;
        int pnr_id;
        string surname;
        string name;
        string exst_name;
        int seats;
        string pers_type;
        string subclass;
        string target;
        string status;
        string crs;
        void dump();
        void Clear()
        {
            pax_id = NoExists;
            pnr_id = NoExists;
            seats = 0;
            surname.erase();
            name.erase();
            pers_type.erase();
            subclass.erase();
            target.erase();
            crs.erase();
        }
        void get(int apax_id)
        {
            Clear();
            Qry.SetVariable("pax_id", apax_id);
            Qry.Execute();
//            if(Qry.Eof)
//                throw Exception("TPNLPaxInfo::get failed: pax_id %d not found", apax_id);
            if(!Qry.Eof) {
                if(col_pax_id == NoExists) {
                    col_pax_id = Qry.FieldIndex("pax_id");
                    col_pnr_id = Qry.FieldIndex("pnr_id");
                    col_surname = Qry.FieldIndex("surname");
                    col_name = Qry.FieldIndex("name");
                    col_seats = Qry.FieldIndex("seats");
                    col_pers_type = Qry.FieldIndex("pers_type");
                    col_subclass = Qry.FieldIndex("subclass");
                    col_target = Qry.FieldIndex("airp_arv");
                    col_status = Qry.FieldIndex("status");
                    col_crs = Qry.FieldIndex("crs");
                }
                pax_id = Qry.FieldAsInteger(col_pax_id);
                pnr_id = Qry.FieldAsInteger(col_pnr_id);
                surname = Qry.FieldAsString(col_surname);
                name = Qry.FieldAsString(col_name);
                TExtraSeatName exst;
                exst.get(pax_id, true);
                exst_name = exst.value;
                seats = Qry.FieldAsInteger(col_seats);
                pers_type = Qry.FieldAsString(col_pers_type);
                subclass = Qry.FieldAsString(col_subclass);
                target = Qry.FieldAsString(col_target);
                status = Qry.FieldAsString(col_status);
                crs = Qry.FieldAsString(col_crs);
            }
        }
        TPNLPaxInfo():
            Qry(&OraSession),
            col_pax_id(NoExists),
            col_pnr_id(NoExists),
            col_surname(NoExists),
            col_name(NoExists),
            col_seats(NoExists),
            col_pers_type(NoExists),
            col_subclass(NoExists),
            col_target(NoExists),
            col_status(NoExists),
            col_crs(NoExists),
            pax_id(NoExists),
            pnr_id(NoExists)
        {
            Qry.SQLText =
                "select "
                "    crs_pax.pax_id, "
                "    crs_pax.pnr_id, "
                "    crs_pax.surname, "
                "    crs_pax.name, "
                "    crs_pax.seats, "
                "    crs_pax.pers_type, "
                "    crs_pnr.subclass, "
                "    crs_pnr.airp_arv, "
                "    crs_pnr.status, "
                "    crs_pnr.sender crs "
                "from "
                "    crs_pnr, "
                "    crs_pax "
                "where "
                "    crs_pax.pax_id = :pax_id and "
                "    crs_pax.pr_del=0 and "
                "    crs_pnr.pnr_id = crs_pax.pnr_id and "
                "    crs_pnr.system = 'CRS' ";
            Qry.DeclareVariable("pax_id", otInteger);
        }
};

struct TPFSInfoItem {
    int pax_id, pnr_id, pnl_pax_id, pnl_point_id;
    TPFSInfoItem():
        pax_id(NoExists),
        pnr_id(NoExists),
        pnl_pax_id(NoExists),
        pnl_point_id(NoExists)
    {}
};

void doca_list2vector(const list<CheckIn::TPaxDocaItem> &lst, vector<CheckIn::TPaxDocaItem> &v)
{
    for(list<CheckIn::TPaxDocaItem>::const_iterator d=lst.begin(); d!=lst.end(); ++d)
    {
        if (d->type!="D" && d->type!="R") continue;
        v.push_back(*d);
    };
    sort(v.begin(), v.end());
}

void doca_map2vector(const CheckIn::TDocaMap &doca_map, vector<CheckIn::TPaxDocaItem> &v)
{
    for(CheckIn::TDocaMap::const_iterator d = doca_map.begin(); d != doca_map.end(); ++d)
    {
        if (d->second.type!="D" && d->second.type!="R") continue;
        v.push_back(d->second);
    };
    sort(v.begin(), v.end());
}

template<class T>
void get_docX_rem(const T &doc, const T &crs_doc, TypeB::TDetailCreateInfo &info, bool inf_indicator, CheckIn::TPaxRemItem &rem, TRemList &rems) // getting doc* remarks
{
    if(doc.empty() and not crs_doc.empty()) {
        if (getPaxRem(info, crs_doc, inf_indicator, rem)) rems.items.push_back(rem.text);
    } else if(not doc.empty() and crs_doc.empty()) {
        if (getPaxRem(info, doc, inf_indicator, rem)) rems.items.push_back(rem.text);
    } else if(not doc.equal(crs_doc)) {
        if (getPaxRem(info, doc, inf_indicator, rem)) rems.items.push_back(rem.text);
    }
}

bool APIPX_cmp_internal(TypeB::TDetailCreateInfo &info, int pax_id, bool inf_indicator, TRemList &rems)
{
    CheckIn::TPaxRemItem rem;

    CheckIn::TPaxDocItem docs, crs_docs;
    LoadPaxDoc(pax_id, docs);
    LoadCrsPaxDoc(pax_id, crs_docs);
    get_docX_rem(docs, crs_docs, info, inf_indicator, rem, rems);

    CheckIn::TDocaMap doca_map, crs_doca_map;
    vector<CheckIn::TPaxDocaItem> vdoca, vcrs_doca;
    LoadPaxDoca(pax_id, doca_map);
    LoadCrsPaxDoca(pax_id, crs_doca_map);

    doca_map2vector(doca_map, vdoca);
    doca_map2vector(crs_doca_map, vcrs_doca);

    vector<CheckIn::TPaxDocaItem>::const_iterator vdoca_i = vdoca.begin();
    vector<CheckIn::TPaxDocaItem>::const_iterator vcrs_doca_i = vcrs_doca.begin();
    for(; vdoca_i != vdoca.end() || vcrs_doca_i != vcrs_doca.end(); ) {
        if(vdoca_i == vdoca.end()) {
            if (getPaxRem(info, *vcrs_doca_i, inf_indicator, rem)) rems.items.push_back(rem.text);
            vcrs_doca_i++;
        } else if(vcrs_doca_i == vcrs_doca.end()) {
            if (getPaxRem(info, *vdoca_i, inf_indicator, rem)) rems.items.push_back(rem.text);
            vdoca_i++;
        } else if(*vcrs_doca_i == *vdoca_i) {
            vcrs_doca_i++;
            vdoca_i++;
        } else if(*vcrs_doca_i < *vdoca_i) {
            if (getPaxRem(info, *vdoca_i, inf_indicator, rem)) rems.items.push_back(rem.text);
            vdoca_i++;
        } else {
            if (getPaxRem(info, *vcrs_doca_i, inf_indicator, rem)) rems.items.push_back(rem.text);
            vcrs_doca_i++;
        }
    }

    CheckIn::TPaxDocoItem doco, crs_doco;
    LoadPaxDoco(pax_id, doco);
    LoadCrsPaxVisa(pax_id, crs_doco);
    get_docX_rem(doco, crs_doco, info, inf_indicator, rem, rems);

    return rem.code.empty();
};

struct TFQT {
    CheckIn::TPaxFQTCards items;
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
    {
        for(CheckIn::TPaxFQTCards::iterator i = items.begin(); i != items.end(); i++) {
            body.push_back((string)
                    ".F/" +
                    info.TlgElemIdToElem(etAirline, i->first.airline) +
                    " " +
                    transliter(i->first.no, 1, info.is_lat())
                    );
        }
    }
};

struct TCKINPaxInfo {
    private:
        TQuery Qry;
        int col_pax_id;
        int col_grp_id;
        int col_surname;
        int col_name;
        int col_seats;
        int col_pers_type;
        int col_cls;
        int col_subclass;
        int col_target;
        int col_pr_brd;
        int col_status;
    public:
        int pax_id;
        int grp_id;
        string surname;
        string name;
        string exst_name;
        int seats;
        string pers_type;
        string cls;
        string subclass;
        string target;
        int pr_brd;
        TPaxStatus status;
        TPNLPaxInfo crs_pax;
        TRemList rems;
        TFQT fqt;

        void dump();

        bool fqt_diff()
        {
            fqt.items.clear();
            CheckIn::TPaxFQTCards crs_cards, ckin_cards, diff_cards;
            set<CheckIn::TPaxFQTItem> fqts;
            if (CheckIn::LoadPaxFQT(pax_id, fqts))
              CheckIn::GetPaxFQTCards(fqts, ckin_cards);
            if (CheckIn::LoadCrsPaxFQT(pax_id, fqts))
              CheckIn::GetPaxFQTCards(fqts, crs_cards);
            set_symmetric_difference(crs_cards.begin(), crs_cards.end(),
                                     ckin_cards.begin(), ckin_cards.end(),
                                     inserter(diff_cards, diff_cards.end()));
            if (!diff_cards.empty())
            {
              //?뫨 ????????? - ??।??? ??? ?????, ??????? ??ॣ?????஢???
              fqt.items=ckin_cards;
            };
            return fqt.items.size() != 0;
        }

        bool APIPX_cmp(TypeB::TDetailCreateInfo &info)
        {
            vector<int> infants;
            for(vector<TInfantsItem>::iterator infRow = rems.infants->items.begin(); infRow != rems.infants->items.end(); infRow++) {
                if(infRow->grp_id == grp_id and infRow->parent_pax_id == pax_id) {
                    infants.push_back(infRow->pax_id);
                }
            }

            bool result = APIPX_cmp_internal(info, pax_id, false, rems);
            for(vector<int>::iterator i = infants.begin(); i != infants.end(); i++)
                result &= APIPX_cmp_internal(info, *i, true, rems);

            return result;
        }
        bool PAXLST_cmp()
        {
            return
                surname == crs_pax.surname and
                name == crs_pax.name and
                pers_type == crs_pax.pers_type;
        }
        void Clear()
        {
            pax_id = NoExists;
            grp_id = NoExists;
            surname.erase();
            name.erase();
            exst_name.erase();
            seats = 0;
            pers_type.erase();
            cls.erase();
            subclass.erase();
            target.erase();
            pr_brd = NoExists;
            status = psCheckin;
            crs_pax.Clear();
            rems.items.clear();
            fqt.items.clear();
        }
        bool OK_status()
        {
            return status == psCheckin or status == psTCheckin;
        }
        void get(const TPFSInfoItem &item)
        {
            Clear();
            if(item.pax_id != NoExists) {
                Qry.SetVariable("pax_id", item.pax_id);
                Qry.Execute();
                if(!Qry.Eof) {
                    if(col_pax_id == NoExists) {
                        col_pax_id = Qry.FieldIndex("pax_id");
                        col_grp_id = Qry.FieldIndex("grp_id");
                        col_surname = Qry.FieldIndex("surname");
                        col_name = Qry.FieldIndex("name");
                        col_seats = Qry.FieldIndex("seats");
                        col_pers_type = Qry.FieldIndex("pers_type");
                        col_cls = Qry.FieldIndex("cls");
                        col_subclass = Qry.FieldIndex("subclass");
                        col_target = Qry.FieldIndex("target");
                        col_pr_brd = Qry.FieldIndex("pr_brd");
                        col_status = Qry.FieldIndex("status");
                    }
                    pax_id = Qry.FieldAsInteger(col_pax_id);
                    grp_id = Qry.FieldAsInteger(col_grp_id);
                    surname = Qry.FieldAsString(col_surname);
                    name = Qry.FieldAsString(col_name);
                    TExtraSeatName exst;
                    exst.get(pax_id);
                    exst_name = exst.value;
                    seats = Qry.FieldAsInteger(col_seats);
                    pers_type = Qry.FieldAsString(col_pers_type);
                    cls = Qry.FieldAsString(col_cls);
                    subclass = Qry.FieldAsString(col_subclass);
                    target = Qry.FieldAsString(col_target);
                    pr_brd = Qry.FieldAsInteger(col_pr_brd);
                    status = DecodePaxStatus(Qry.FieldAsString(col_status));
                }
            }
            if(item.pnl_pax_id != NoExists)
                crs_pax.get(item.pnl_pax_id);
        }
        TCKINPaxInfo(TInfants *ainfants):
            Qry(&OraSession),
            col_pax_id(NoExists),
            col_surname(NoExists),
            col_name(NoExists),
            col_seats(NoExists),
            col_pers_type(NoExists),
            col_cls(NoExists),
            col_subclass(NoExists),
            col_target(NoExists),
            col_pr_brd(NoExists),
            col_status(NoExists),
            pax_id(NoExists),
            pr_brd(NoExists),
            status(psCheckin),
            rems(ainfants)
        {
            Qry.SQLText =
                "select "
                "    pax.pax_id, "
                "    pax.grp_id, "
                "    pax.surname, "
                "    pax.name, "
                "    pax.seats, "
                "    pax.pers_type, "
                "    nvl(pax.cabin_class, pax_grp.class) cls, "
                "    nvl(nvl(pax.cabin_subclass, pax.subclass), nvl(pax.cabin_class, pax_grp.class)) subclass, "
                "    pax_grp.airp_arv target, "
                "    pax.pr_brd, "
                "    pax_grp.status "
                "from "
                "    pax, "
                "    pax_grp "
                "where "
                "    pax.pax_id = :pax_id and "
                "    pax_grp.grp_id = pax.grp_id ";
            Qry.DeclareVariable("pax_id", otInteger);
        }
};

void TPNLPaxInfo::dump()
{
    ProgTrace(TRACE5, "TPNLPaxInfo::dump()");
    ProgTrace(TRACE5, "pax_id: %d", pax_id);
    ProgTrace(TRACE5, "pnr_id: %d", pnr_id);
    ProgTrace(TRACE5, "surname: %s", surname.c_str());
    ProgTrace(TRACE5, "name: %s", name.c_str());
    ProgTrace(TRACE5, "pers_type: %s", pers_type.c_str());
    ProgTrace(TRACE5, "subclass: %s", subclass.c_str());
    ProgTrace(TRACE5, "target: %s", target.c_str());
    ProgTrace(TRACE5, "status: %s", status.c_str());
    ProgTrace(TRACE5, "crs: %s", crs.c_str());
    ProgTrace(TRACE5, "END OF TPNLPaxInfo::dump()");
}

void TCKINPaxInfo::dump()
{
    ProgTrace(TRACE5, "TCKINPaxInfo::dump()");
    ProgTrace(TRACE5, "pax_id: %d", pax_id);
    ProgTrace(TRACE5, "surname: %s", surname.c_str());
    ProgTrace(TRACE5, "name: %s", name.c_str());
    ProgTrace(TRACE5, "pers_type: %s", pers_type.c_str());
    ProgTrace(TRACE5, "cls: %s", cls.c_str());
    ProgTrace(TRACE5, "subclass: %s", subclass.c_str());
    ProgTrace(TRACE5, "target: %s", target.c_str());
    ProgTrace(TRACE5, "pr_brd: %d", pr_brd);
    ProgTrace(TRACE5, "status: %d", status);
    ProgTrace(TRACE5, "status: %s", EncodePaxStatus(status));
    crs_pax.dump();
    ProgTrace(TRACE5, "END OF TCKINPaxInfo::dump()");
}

struct TPFSPax {
    int pax_id;
    int pnr_id;
    string name, surname;
    string exst_name;
    int seats;
    string target;
    string subcls;
    string crs;
    TMItem M;
    TPNRList pnrs;
    TRemList rems;
    TFQT fqt;
    TPFSPax(TInfants *ainfants): pax_id(NoExists), pnr_id(NoExists), rems(ainfants) {};
    TPFSPax(const TCKINPaxInfo &ckin_pax, TInfants *ainfants);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
    void operator = (const TCKINPaxInfo &ckin_pax);
};

TPFSPax::TPFSPax(const TCKINPaxInfo &ckin_pax, TInfants *ainfants): pax_id(NoExists), rems(ainfants)
{
    *this = ckin_pax;
}

void TPFSPax::operator = (const TCKINPaxInfo &ckin_pax)
{
        if(ckin_pax.pax_id != NoExists) {
            pax_id = ckin_pax.pax_id;
            name = ckin_pax.name;
            surname = ckin_pax.surname;
            exst_name = ckin_pax.exst_name;
            seats = ckin_pax.seats;
            target = ckin_pax.target;
            subcls = ckin_pax.subclass;
            M.m_flight.getByPaxId(pax_id);
        } else {
            if(ckin_pax.crs_pax.pax_id == NoExists)
                throw Exception("TPFSPax::operator =: both ckin and crs pax_id are not exists");
            pax_id = ckin_pax.crs_pax.pax_id;
            name = ckin_pax.crs_pax.name;
            surname = ckin_pax.crs_pax.surname;
            exst_name = ckin_pax.crs_pax.exst_name;
            seats = ckin_pax.crs_pax.seats;
            target = ckin_pax.crs_pax.target;
            subcls = ckin_pax.crs_pax.subclass;
            M.m_flight.getByCrsPaxId(pax_id);
        }
        crs = ckin_pax.crs_pax.crs;
        pnr_id = ckin_pax.crs_pax.pnr_id;
        rems = ckin_pax.rems;
        fqt = ckin_pax.fqt;
}

void nameToTlg(const string &name, const string &asurname, int seats, const string &exst_name, vector<string> &body)
{
    string surname = asurname;
    int name_count = 0;
    int names_sum_size = 0;
    vector<string> names;
    if(not name.empty()) {
        names.push_back(name);
        names_sum_size += name.size();
        name_count = 1;
    }
    for(int i = 0; seats != 1 and i < seats - name_count; i++) {
        names.push_back(exst_name);
        names_sum_size += exst_name.size();
    }
    size_t len = surname.size() + names.size() + names_sum_size + TypeB::endl.size();
    if(len > LINE_SIZE) {
        size_t diff = len - LINE_SIZE;
        if(name.empty()) {
            surname = surname.substr(0, surname.size() - diff);
        } else {
            if(diff >= name.size()) {
                names[0] = names[0].substr(0, 1);
                diff -= name.size() - 1;
                surname = surname.substr(0, surname.size() - diff);
            } else {
                names[0] = names[0].substr(0, names[0].size() - diff);
            }
        }
    }

    ostringstream buf;
    buf << seats << surname;
    for(vector<string>::iterator iv = names.begin(); iv != names.end(); iv++)
        buf << "/" << *iv;
    body.push_back(buf.str());
}


void TPFSPax::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    name = transliter(name, 1, info.is_lat());
    surname = transliter(surname, 1, info.is_lat());
    nameToTlg(name, surname, seats, exst_name, body);
    pnrs.ToTlg(info, body);
    M.ToTlg(info, body);
    rems.ToTlg(info, body);
    fqt.ToTlg(info, body);
}

struct TSubClsCmp {
    bool operator() (const string &l, const string &r) const
    {
        TSubcls &subcls = (TSubcls &)base_tables.get("subcls");
        string l_cls = ((const TSubclsRow&)subcls.get_row("code", l)).cl;
        string r_cls = ((const TSubclsRow&)subcls.get_row("code", r)).cl;
        TClasses &classes = (TClasses &)base_tables.get("classes");
        int l_prior = ((const TClassesRow &)classes.get_row("code", l_cls)).priority;
        int r_prior = ((const TClassesRow &)classes.get_row("code", r_cls)).priority;
        if(l_prior == r_prior)
            return l < r;
        else
            return l_prior < r_prior;
    }
};

struct TPFSPaxList:vector<TPFSPax> {
    size_t size() {
        size_t result = 0;
        for(TPFSPaxList::iterator iv = begin(); iv != end(); iv++)
            result += iv->seats;
        return result;
    }
};

typedef map<string, TPFSPaxList, TSubClsCmp> TPFSClsList;
typedef map<string, TPFSClsList> TPFSCtgryList;
typedef map<string, TPFSCtgryList> TPFSItems;

struct TPFSBody {
    TInfants infants;
    map<string, TNumByDestItem> pfsn;
    TPFSItems items;
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

void TPFSBody::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    vector<string> category_lst;
    TTripRoute route;
    route.GetRouteAfter(NoExists, info.point_id, trtNotCurrent, trtNotCancelled);
    for(TTripRoute::iterator iv = route.begin(); iv != route.end(); iv++) {
        pfsn[iv->airp].ToTlg(info, iv->airp, body);
        if(info.get_tlg_type() == "PFS") {
            TPFSCtgryList &CtgryList = items[iv->airp];
            if(CtgryList.empty())
                continue;
            category_lst.push_back((string)"-" + info.TlgElemIdToElem(etAirp, iv->airp));
            for(TPFSCtgryList::iterator ctgry = CtgryList.begin(); ctgry != CtgryList.end(); ctgry++) {
                TPFSClsList &ClsList = ctgry->second;
                for(TPFSClsList::iterator cls = ClsList.begin(); cls != ClsList.end(); cls++) {
                    ostringstream buf;
                    TPFSPaxList &pax_list = cls->second;
                    buf << ctgry->first << " " << pax_list.size() << info.TlgElemIdToElem(etSubcls, cls->first);
                    category_lst.push_back(buf.str());
                    for(TPFSPaxList::iterator pax = pax_list.begin(); pax != pax_list.end(); pax++)
                        pax->ToTlg(info, category_lst);
                }
            }
        }
    }
    body.insert(body.end(), category_lst.begin(), category_lst.end());
}


struct TPFSInfo {
    map<int, TPFSInfoItem> items;
    void get(int point_id);
    void dump();
};

void TPFSInfo::dump()
{
    ostringstream buf;
    buf
        << setw(20) << "pax_id"
        << setw(20) << "pnr_id"
        << setw(20) << "pnl_pax_id"
        << setw(20) << "pnl_point_id";
    ProgTrace(TRACE5, "%s", buf.str().c_str());
    for(map<int, TPFSInfoItem>::iterator im = items.begin(); im != items.end(); im++) {
        const TPFSInfoItem &item = im->second;
        buf.str("");
        buf
            << setw(20) << item.pax_id
            << setw(20) << item.pnr_id
            << setw(20) << item.pnl_pax_id
            << setw(20) << item.pnl_point_id;
        ProgTrace(TRACE5, "%s", buf.str().c_str());
    }
}

void TPFSInfo::get(int point_id)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select  "
        "    pax.pax_id,  "
        "    crs_pax.pnr_id  "
        "from  "
        "    pax_grp,  "
        "    pax,  "
        "    crs_pax  "
        "where  "
        "    pax_grp.status NOT IN (:psTransit, :psCrew) and  "
        "    pax_grp.point_dep = :point_id and  "
        "    pax_grp.grp_id = pax.grp_id and  "
        "    pax.refuse is null and "
        "    pax.seats > 0 and "
        "    pax.pax_id = crs_pax.pax_id(+) and  "
        "    crs_pax.pr_del(+) = 0  ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("psTransit", otString, EncodePaxStatus(psTransit));
    Qry.CreateVariable("psCrew", otString, EncodePaxStatus(psCrew));
    Qry.Execute();
    if(!Qry.Eof) {
        int col_pax_id = Qry.FieldIndex("pax_id");
        int col_pnr_id = Qry.FieldIndex("pnr_id");
        for(; !Qry.Eof; Qry.Next()) {
            int pax_id = Qry.FieldAsInteger(col_pax_id);
            int pnr_id = NoExists;
            if(!Qry.FieldIsNULL(col_pnr_id))
                pnr_id = Qry.FieldAsInteger(col_pnr_id);
            items[pax_id].pax_id = pax_id;
            items[pax_id].pnr_id = pnr_id;
        }
    }
    Qry.Clear();
    Qry.SQLText =
        "select  "
        "    crs_pax.pax_id pnl_pax_id, "
        "    pax_grp.point_dep pnl_point_id "
        "from  "
        "    tlg_binding,  "
        "    crs_pnr,  "
        "    crs_pax, "
        "    pax, "
        "    pax_grp "
        "where  "
        "    tlg_binding.point_id_spp = :point_id and  "
        "    tlg_binding.point_id_tlg = crs_pnr.point_id and  "
        "    crs_pnr.system = 'CRS' and "
        "    crs_pnr.pnr_id = crs_pax.pnr_id and  "
        "    crs_pax.pr_del = 0 and "
        "    crs_pax.pax_id = pax.pax_id(+) and "
        "    pax.refuse(+) is null and "
        "    nvl(pax.seats, crs_pax.seats) > 0 and "
        "    pax.grp_id = pax_grp.grp_id(+) ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if(!Qry.Eof) {
        int col_pnl_pax_id = Qry.FieldIndex("pnl_pax_id");
        int col_pnl_point_id = Qry.FieldIndex("pnl_point_id");
        for(; !Qry.Eof; Qry.Next()) {
            int pnl_pax_id = Qry.FieldAsInteger(col_pnl_pax_id);
            int pnl_point_id = NoExists;
            if(!Qry.FieldIsNULL(col_pnl_point_id))
                pnl_point_id = Qry.FieldAsInteger(col_pnl_point_id);
            items[pnl_pax_id].pnl_pax_id = pnl_pax_id;
            items[pnl_pax_id].pnl_point_id = pnl_point_id;
        }
    }
}

void TPFSBody::get(TypeB::TDetailCreateInfo &info)
{
    const TypeB::TMarkInfoOptions &markOptions=*(info.optionsAs<TypeB::TMarkInfoOptions>());

    infants.get(info);

    TPFSInfo PFSInfo;
    PFSInfo.get(info.point_id);
    TCKINPaxInfo ckin_pax(&infants);
    for(map<int, TPFSInfoItem>::iterator im = PFSInfo.items.begin(); im != PFSInfo.items.end(); im++) {
        string category;
        const TPFSInfoItem &item = im->second;
        ckin_pax.get(item);
        if(item.pnl_pax_id != NoExists) { // ???ᠦ?? ???????????? ? PNL/ADL ३??
            ckin_pax.fqt_diff(); // get changed FQT
            if(ckin_pax.crs_pax.status == "WL") {
                if(ckin_pax.pr_brd != NoExists and ckin_pax.pr_brd != 0)
                    category = "CFMWL";
            } else if(
                    ckin_pax.crs_pax.status == "DG2" or
                    ckin_pax.crs_pax.status == "RG2" or
                    ckin_pax.crs_pax.status == "ID2"
                    ) {
                if(ckin_pax.pr_brd != NoExists and ckin_pax.pr_brd != 0)
                    category = "IDPAD";
            } else if(item.pax_id == NoExists) { // ?? ??ॣ?????஢?? ?? ?????? ३?
                if(item.pnl_point_id != NoExists and item.pnl_point_id != info.point_id) // ??ॣ?????஢?? ?? ??㣮? ३?
                    category = "CHGFL";
                else
                    category = "NOSHO";
            } else { // ??ॣ?????஢??
                if(ckin_pax.pr_brd != 0) { // ???襫 ??ᠤ??
                    if(not ckin_pax.fqt.items.empty())
                        category = "FQTVN";
                    else if(ckin_pax.subclass != ckin_pax.crs_pax.subclass)
                        category = "INVOL";
                    else if(ckin_pax.target != ckin_pax.crs_pax.target)
                        category = "CHGSG";
                    else if(not ckin_pax.APIPX_cmp(info))
                        category = "APIPX";
                } else { // ?? ???襫 ??ᠤ??
                    if(ckin_pax.OK_status())
                        category = "OFFLK";
                }
            }
        } else { // ???ᠦ?? ?? ???????????? ? PNL/ADL ३??
            ckin_pax.fqt.items.clear();
            set<CheckIn::TPaxFQTItem> fqts;
            if (LoadPaxFQT(item.pax_id, fqts)) // get FQT if any
              CheckIn::GetPaxFQTCards(fqts, ckin_pax.fqt.items);
            if(item.pax_id != NoExists) { // ??ॣ?????஢??
                if(ckin_pax.OK_status()) { // ???ᠦ?? ????? ?????? "?஭?" ??? "᪢????? ॣ????????"
                    if(ckin_pax.pr_brd != 0)
                        category = "NOREC";
                    else
                        category = "OFFLN";
                } else { // ???ᠦ?? ????? ?????? "???ᠤ??"
                    if(ckin_pax.pr_brd != 0)
                        category = "GOSHO";
                    else
                        category = "GOSHN";
                }
            }
        }

        TPFSPax PFSPax(NULL);
        PFSPax = ckin_pax; // PFSPax.M inits within assignment
        if(not markOptions.crs.empty() and markOptions.crs != PFSPax.crs)
            continue;
        if(not markOptions.mark_info.empty() and not(PFSPax.M.m_flight == markOptions.mark_info))
            continue;
        if(item.pax_id != NoExists) // ??? ??ॣ?????஢????? ???ᠦ?஢ ᮡ?ࠥ? ???? ??? ???஢?? PFS
            pfsn[ckin_pax.target].add(ckin_pax.cls, ckin_pax.seats);

        if(category.empty())
            continue;
        PFSPax.pnrs.get(PFSPax.pnr_id);
        items[PFSPax.target][category][PFSPax.subcls].push_back(PFSPax);
    }
}

int PFS(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading;
    heading << "PFS" << TypeB::endl
            << info.flight_view() << "/"
            << info.scd_local_view() << " " << info.airp_dep_view() << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
    size_t part_len = tlg_row.textSize();
    vector<string> body;
    try {
        TPFSBody pfs;
        pfs.get(info);
        pfs.ToTlg(info, body);
    } catch(...) {
        ExceptionFilter(body, info);
    }
    simple_split(heading, part_len, tlg_draft, tlg_row, body);
    tlg_row.ending = "ENDPFS" + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);

    return tlg_row.id;
}

class TRBD:list<pair<string, list<string> > > {
    private:
    public:
        void get(TypeB::TDetailCreateInfo &info);
        void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

void TRBD::get(TypeB::TDetailCreateInfo &info)
{
    const TypeB::TPRLOptions *PRLOptions=NULL;
    if(info.optionsIs<TypeB::TPRLOptions>())
        PRLOptions=info.optionsAs<TypeB::TPRLOptions>();
    if(PRLOptions and PRLOptions->rbd) {
        const TypeB::TMarkInfoOptions *markOptions=NULL;
        if(info.optionsIs<TypeB::TMarkInfoOptions>())
            markOptions=info.optionsAs<TypeB::TMarkInfoOptions>();

        // 1. 蠣 - ??।????? crs_rbd.point_id (??????? ???? tlg_trips.point_id)
        int crs_rbd_point_id_mark = NoExists;
        int crs_rbd_point_id_oper = NoExists;
        QParams QryParams;
        QryParams << QParam("point_id", otInteger, info.point_id);
        TCachedQuery Qry(
                "SELECT tlg_trips.point_id, tlg_trips.airline, tlg_trips.flt_no, tlg_trips.suffix "
                "FROM tlg_trips, tlg_binding "
                "WHERE tlg_trips.point_id=tlg_binding.point_id_tlg AND "
                "             tlg_binding.point_id_spp=:point_id and exists ( "
                "               select * from typeb_data_stat where "
                "                   typeb_data_stat.point_id = tlg_trips.point_id and "
                "                   typeb_data_stat.system = 'CRS' and rownum < 2)",
                QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            for(; not Qry.get().Eof; Qry.get().Next()) {
                TMktFlight flt;
                int tlg_trips_point_id = Qry.get().FieldAsInteger("point_id");
                flt.airline = Qry.get().FieldAsString("airline");
                flt.flt_no = Qry.get().FieldAsInteger("flt_no");
                flt.suffix = Qry.get().FieldAsString("suffix");
                if(markOptions and not markOptions->mark_info.empty()) {
                    if(flt == markOptions->mark_info) {
                        crs_rbd_point_id_mark = tlg_trips_point_id;
                    }
                }
                if(
                        info.airline == flt.airline and
                        info.flt_no == flt.flt_no and
                        info.suffix == flt.suffix
                        ) {
                    crs_rbd_point_id_oper = tlg_trips_point_id;
                }
            }
        }
        // 2. 蠣. ??।????? crs_rbd.sender
        string crs_rbd_sender;
        if(markOptions and not markOptions->mark_info.empty())
            crs_rbd_sender = markOptions->crs;

        // 3. 蠣. ?롮? ???????? ???室???? point_id ? sender ?? CRS_RBD
        QryParams.clear();

        if(crs_rbd_point_id_mark == NoExists)
            QryParams << QParam("point_id_tlg_mark", otInteger, FNull);
        else
            QryParams << QParam("point_id_tlg_mark", otInteger, crs_rbd_point_id_mark);

        if(crs_rbd_point_id_oper == NoExists)
            QryParams << QParam("point_id_tlg_oper", otInteger, FNull);
        else
            QryParams << QParam("point_id_tlg_oper", otInteger, crs_rbd_point_id_oper);

        QryParams
            << QParam("sender", otString, crs_rbd_sender)
            << QParam("point_id_spp", otInteger, info.point_id);

        TCachedQuery Step3Qry(
                "SELECT crs_rbd.point_id, crs_rbd.sender, "
                "  DECODE(crs_rbd.point_id, :point_id_tlg_mark, 2, 0)+ "
                "  DECODE(crs_rbd.sender, :sender, 1, 0) AS priority "
                "FROM crs_rbd, tlg_binding "
                "WHERE crs_rbd.point_id=tlg_binding.point_id_tlg AND "
                "      tlg_binding.point_id_spp=:point_id_spp AND "
                "      crs_rbd.point_id IN (:point_id_tlg_mark, :point_id_tlg_oper) AND crs_rbd.system='CRS' "
                "ORDER BY priority DESC, crs_rbd.point_id, crs_rbd.sender ",
                QryParams);
        Step3Qry.get().Execute();

        if(not Step3Qry.get().Eof) {
            int crs_rbd_point_id = Step3Qry.get().FieldAsInteger("point_id");
            crs_rbd_sender = Step3Qry.get().FieldAsString("sender");

            // ????? ??।????? ??? ??ࠬ???? (point_id ? sender ?? CRS_RBD)
            QryParams.clear();
            QryParams
                << QParam("point_id", otInteger, crs_rbd_point_id)
                << QParam("sender", otString, crs_rbd_sender);
            TCachedQuery Step4Qry(
                    "SELECT fare_class, compartment "
                    "FROM crs_rbd "
                    "WHERE point_id=:point_id AND sender=:sender AND system='CRS' "
                    "ORDER BY view_order",
                    QryParams);
            Step4Qry.get().Execute();
            string compartment;
            for(; not Step4Qry.get().Eof; Step4Qry.get().Next()) {
                string curr_compartment = Step4Qry.get().FieldAsString("compartment");
                string fare_class = Step4Qry.get().FieldAsString("fare_class");
                if(curr_compartment != compartment) {
                    compartment = curr_compartment;
                    push_back(make_pair(compartment, list<string>()));
                }
                back().second.push_back(fare_class);
            }
        }
    }
}

void TRBD::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    if(empty()) return;
    string result = "RBD";
    for(
            list<pair<string, list<string> > >::iterator compartment = begin();
            compartment != end(); compartment++)
    {
        result += " " + info.TlgElemIdToElem(etSubcls, compartment->first, prLatToElemFmt(efmtCodeNative,true)) + "/";
        for(
                list<string>::iterator fare_class = compartment->second.begin();
                fare_class != compartment->second.end();
                fare_class++
           ) {
            result += info.TlgElemIdToElem(etSubcls, *fare_class, prLatToElemFmt(efmtCodeNative,true));
        }
    }
    body.insert(body.end(), result);
}

int PRL(TypeB::TDetailCreateInfo &info)
{
#ifdef SQL_COUNTERS
    ProgTrace(TRACE5, "_prl_ PRL begin: queryCount: %d", queryCount);
#endif
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    ostringstream heading;
    heading << "PRL" << TypeB::endl
            << info.flight_view() << "/"
            << info.scd_local_view() << " " << info.airp_dep_view() << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + TypeB::endl;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + TypeB::endl;
    size_t part_len = tlg_row.textSize();

    vector<string> body;
    try {
        TRBD rbd;
        rbd.get(info);
        rbd.ToTlg(info, body);
        TTlgCompLayerList complayers;
        if(not isFreeSeating(info.point_id) and not isEmptySalons(info.point_id))
            getSalonLayers( info, complayers, false );
        TDestList<TPRLDest> dests;
        dests.get(info,complayers);
        dests.ToTlg(info, body);
    } catch(...) {
        ExceptionFilter(body, info);
    }

    split_n_save(heading, part_len, tlg_draft, tlg_row, body);
    tlg_row.ending = "ENDPRL" + TypeB::endl;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
#ifdef SQL_COUNTERS
    ProgTrace(TRACE5, "_prl_ PRL end: queryCount: %d", queryCount);
#endif
    return tlg_row.id;
}

int Unknown(TypeB::TDetailCreateInfo &info)
{
    TTlgDraft tlg_draft(info);
    TTlgOutPartInfo tlg_row(info);
    tlg_row.origin = info.originator.originSection(tlg_row.time_create, TypeB::endl);
    info.vcompleted = false;
    tlg_draft.Save(tlg_row);
    tlg_draft.Commit(tlg_row);
    return tlg_row.id;
}

int TelegramInterface::create_tlg(const TypeB::TCreateInfo &createInfo,
                                  int typeb_in_id,
                                  TTypeBTypesRow &tlgTypeInfo,
                                  bool manual_creation)
{
    ProgTrace(TRACE5, "createInfo.tlg_type: %s", createInfo.get_tlg_type().c_str());
    if(createInfo.get_tlg_type().empty())
        throw AstraLocale::UserException("MSG.TLG.UNSPECIFY_TYPE");
    try
    {
      const TTypeBTypesRow& row = (const TTypeBTypesRow&)(base_tables.get("typeb_types").get_row("code",createInfo.get_tlg_type()));
      tlgTypeInfo=row;
    }
    catch(const EBaseTableError&)
    {
      throw AstraLocale::UserException("MSG.TLG.TYPE_WRONG_SPECIFIED");
    };

    TQuery Qry(&OraSession);
    TypeB::TDetailCreateInfo info;
    info.create_point = createInfo.create_point;
    info.copy(createInfo);
    info.point_id = createInfo.point_id;
    info.lang = AstraLocale::LANG_RU;
    info.elem_fmt = prLatToElemFmt(efmtCodeNative, info.get_options().is_lat);
    info.time_create = NowUTC();
    info.vcompleted = !tlgTypeInfo.editable;
    info.manual_creation = manual_creation;
    info.typeb_in_id = typeb_in_id;

    if (info.optionsIs<TypeB::TAirpTrferOptions>())
    {
      const TypeB::TAirpTrferOptions &options=*(info.optionsAs<TypeB::TAirpTrferOptions>());
      if (options.airp_trfer.empty())
        throw AstraLocale::UserException("MSG.AIRP.DST_UNSPECIFIED");
    };

    if(info.point_id != NoExists)
    {
        fillFltDetails(info);
    }
    else
    {
      //???ਢ易???? ? ३?? ⥫??ࠬ??
      if (tlgTypeInfo.pr_dep!=NoExists)
        throw Exception("TelegramInterface::create_tlg: point_id not defined (tlg_type=%s)", info.get_tlg_type().c_str());
    };

    //????᫥??? ???ࠢ?⥫?
    //?᫨ ⥫??ࠬ?? ᮧ?????? ? ?⢥? ?? ?室??? (typeb_in_id != NoExists),
    //?? ?ਣ?????? ??????塞 ?????, 祬 ??? ??????樨 ??⮭????? ⫣.
    if(typeb_in_id != NoExists)
        info.originator = TypeB::getOriginator(string(), string(), string(), NowUTC(), true);
    else {
        string orig_airline=info.airline;
        /* ???????? ??????????? ? ????饬
           if (info.optionsIs<TypeB::TMarkInfoOptions>())
           {
           const TypeB::TMarkInfoOptions &options=*(info.optionsAs<TypeB::TMarkInfoOptions>());
           if (!options.mark_info.airline.empty())
           orig_airline=options.mark_info.airline;
           };
           */
        info.originator = TypeB::getOriginator( orig_airline,
                info.airp_dep,
                info.get_tlg_type(),
                info.time_create,
                true);
    }

    if (tlgTypeInfo.basic_type == "CPM" ||
        (tlgTypeInfo.basic_type == "MVT" && info.get_tlg_type() == "MVTB"))
    {
        TTripRoute route;
        TTripRouteItem next_airp;
        route.GetNextAirp(NoExists, info.point_id, trtNotCancelled, next_airp);
        if (!next_airp.airp.empty())
        {
            info.airp_arv = next_airp.airp;
        }
        else throw AstraLocale::UserException("MSG.AIRP.DST_NOT_FOUND");
    };

    info.addrs = format_addr_line(createInfo.get_addrs(), &info);

    if(info.addrs.empty())
        throw AstraLocale::UserException("MSG.TLG.DST_ADDRS_NOT_SET");

    int vid = NoExists;

    TPerfTimer tm("tlg handler");
    tm.Init();
    if(tlgTypeInfo.basic_type == "PTM") vid = PTM(info);
    else if(tlgTypeInfo.basic_type == "LDM") vid = LDM(info);
    else if(tlgTypeInfo.basic_type == "MVT") vid = MVT(info);
    else if(tlgTypeInfo.basic_type == "AHL") vid = AHL(info);
    else if(tlgTypeInfo.basic_type == "CPM") vid = CPM(info);
    else if(tlgTypeInfo.basic_type == "BTM") vid = BTM(info);
    else if(tlgTypeInfo.basic_type == "PRL") vid = PRL(info);
    else if(tlgTypeInfo.basic_type == "TPM") vid = TPM(info);
    else if(tlgTypeInfo.basic_type == "PSM") vid = PSM(info);
    else if(tlgTypeInfo.basic_type == "PIL") vid = PIL(info);
    else if(tlgTypeInfo.basic_type == "PFS") vid = PFS(info);
    else if(tlgTypeInfo.basic_type == "ETL") vid = ETL(info);
    else if(tlgTypeInfo.basic_type == "ASL") vid = ASL(info);
    else if(tlgTypeInfo.basic_type == "FTL") vid = FTL(info);
    else if(tlgTypeInfo.basic_type == "COM") vid = COM(info);
    else if(tlgTypeInfo.basic_type == "SOM") vid = SOM(info);
    else if(tlgTypeInfo.basic_type == "PIM") vid = PIM(info);
    else if(tlgTypeInfo.basic_type == "LCI") vid = LCI(info);
    else if(tlgTypeInfo.basic_type == "PNL") vid = PNL(info);
    else if(tlgTypeInfo.basic_type == "IDM") vid = IDM(info);
    else if(tlgTypeInfo.basic_type == "TPL") vid = TPL(info);
    else if(tlgTypeInfo.basic_type == "->>") vid = FWD(info);
    else vid = Unknown(info);
    ProgTrace(TRACE5, "utg_prl_tst: %s", tm.PrintWithMessage().c_str());

    if (vid!=NoExists)
    {
      info.err_lst.dump();
      info.err_lst.toDB(vid);

      Qry.Clear();
      Qry.SQLText = "update tlg_out set completed = :vcompleted, has_errors = :vhas_errors where id = :vid";
      Qry.CreateVariable("vcompleted", otInteger, info.vcompleted);
      Qry.CreateVariable("vhas_errors", otInteger, not info.err_lst.empty());
      Qry.CreateVariable("vid", otInteger, vid);
      Qry.Execute();
      if (info.point_id!=NoExists)
        check_tlg_out_alarm(info.point_id);
    };

    ProgTrace(TRACE5, "END OF CREATE %s", createInfo.get_tlg_type().c_str());
    return vid;
}

void EMDReport(int point_id, map<int, vector<string> > &tab, size_t &total)
{
    total = 0;

    TypeB::TCreateInfo createInfo("ASL", TypeB::TCreatePoint());

    TypeB::TDetailCreateInfo info;
    info.create_point = createInfo.create_point;
    info.copy(createInfo);
    info.point_id = point_id;
    vector<TTlgCompLayer> complayers;
    TDestList<TASLReportDest> dests;
    dests.get(info,complayers);
    for(vector<TASLReportDest>::iterator i = dests.items.begin(); i != dests.items.end(); i++) {
        for(vector<TASLPax>::iterator pax = i->PaxList.begin(); pax != i->PaxList.end(); pax++) {
            vector<string> &row = tab[pax->reg_no];
            row.push_back(pax->name.surname + " " + pax->name.name);
            row.push_back(pax->tkn.no + "/" + IntToString(pax->tkn.coupon));
            ostringstream buf;
            for(vector<CheckIn::TPaxASVCItem>::iterator i = pax->used_asvc.begin(); i != pax->used_asvc.end(); i++) {
                buf << i->emd_no << "/" << i->emd_coupon << ' ';
            }
            total += pax->used_asvc.size();
            row.push_back(buf.str());
        }
    }
}

void TelegramInterface::CreateTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TypeB::TCreateInfo createInfo;
    createInfo.fromXML(reqNode);

    int tlg_id = NoExists;
    TTypeBTypesRow tlgTypeInfo;
    try {
        tlg_id = create_tlg(createInfo, NoExists, tlgTypeInfo, true);
    } catch(AstraLocale::UserException &E) {
        throw AstraLocale::UserException( "MSG.TLG.CREATE_ERROR", LParams() << LParam("what", getLocaleText(E.getLexemaData())));
    } catch(Exception &E) {
        if(tlgTypeInfo.basic_type == "->>")
            throw AstraLocale::UserException("MSG.TLG.MANUAL_FWD_FORBIDDEN");
        else
            throw;
    }

    if (tlg_id != NoExists)
        TReqInfo::Instance()->LocaleToLog("EVT.TLG.CREATED_MANUALLY", LEvntPrms()
                << PrmElem<std::string>("name", etTypeBType, createInfo.get_tlg_type(), efmtNameShort)
                << PrmSmpl<int>("id", tlg_id) << PrmBool("lat", createInfo.get_options().is_lat),
                evtTlg, createInfo.point_id, tlg_id);
    NewTextChild( resNode, "tlg_id", tlg_id);
};

#include "tlg/lci_parser.h"

void TelegramInterface::kick(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string res;
    int tlg_id = NoExists;
    int lci_data_id = NoExists;

    try {
        lci_data_id = NodeAsInteger("content", reqNode);
    } catch(...) {
        res = NodeAsString("content", reqNode);
    }

    if(lci_data_id != NoExists) {
        TypeB::TLCIContent con;
        con.fromDB(lci_data_id);
        string answer = con.answer();
        if(not answer.empty())
            try {
                tlg_id = ToInt(answer);
            } catch(...) {
                res = answer;
            }
    }

    if(tlg_id != ASTRA::NoExists) {
        QParams QryParams;
        QryParams << QParam("id", otInteger, tlg_id);
        TCachedQuery Qry(
                "SELECT heading, body, ending, has_errors FROM tlg_out WHERE id=:id ORDER BY num",
                QryParams
                );
        Qry.get().Execute();
        string heading, ending;
        bool has_errors = false;
        for(; not Qry.get().Eof; Qry.get().Next()) {
            has_errors |= Qry.get().FieldAsInteger("has_errors") != 0;
            if(heading.empty()) heading = Qry.get().FieldAsString("heading");
            if(ending.empty()) ending = Qry.get().FieldAsString("ending");
            res += Qry.get().FieldAsString("body");
        }
        if(has_errors) {
            TCachedQuery errQry("select text from typeb_out_errors where tlg_id = :tlg_id and lang = :lang order by part_no",
                    QParams()
                    << QParam("tlg_id", otInteger, tlg_id)
                    << QParam("lang", otString, AstraLocale::LANG_EN)
                    );
            errQry.get().Execute();
            ostringstream err_text;
            for(; not errQry.get().Eof; errQry.get().Next()) {
                err_text << errQry.get().FieldAsString("text") << endl;
            }
            res = err_text.str();;
            LogTrace(TRACE5) << "tlg_srv kick: tlg out (tlg_id: " << tlg_id << ") has errors: " << res;
        } else {
            res = heading + res + ending;
            markTlgAsSent(tlg_id);
        }

    }
    NewTextChild(resNode, "content", res);
}

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "astra_context.h"

namespace WBMessages {

    class TMsgType {
        public:
            enum Enum {
                mtLOADSHEET,
                mtNOTOC,
                mtLIR,
                None
            };

            static const std::list< std::pair<Enum, std::string> >& pairs()
            {
                static std::list< std::pair<Enum, std::string> > l;
                if (l.empty())
                {
                    l.push_back(std::make_pair(mtLOADSHEET, "LOADSHEET"));
                    l.push_back(std::make_pair(mtNOTOC,     "NOTOC"));
                    l.push_back(std::make_pair(mtLIR,       "LIR"));
                }
                return l;
            }

    };

    class TMsgTypes : public ASTRA::PairList<TMsgType::Enum, std::string>
    {
        private:
            virtual std::string className() const { return "TMsgTypes"; }
        public:
            TMsgTypes() : ASTRA::PairList<TMsgType::Enum, std::string>(TMsgType::pairs(),
                    boost::none,
                    boost::none) {}
    };

    const TMsgTypes& MsgTypes()
    {
      static TMsgTypes msgTypes;
      return msgTypes;
    }

    void toDB(int point_id, TMsgType::Enum msg_type, const string &content) {
        TCachedQuery Qry(
                "begin "
                "   insert into wb_msg(id, msg_type, point_id, time_receive) values "
                "      (cycle_id__seq.nextval, :msg_type, :point_id, system.utcsysdate) "
                "      returning id into :id; "
                "end; ",
                QParams()
                << QParam("point_id", otInteger, point_id)
                << QParam("msg_type", otString, MsgTypes().encode(msg_type))
                << QParam("id", otInteger)
                );
        Qry.get().Execute();
        int id = Qry.get().GetVariableAsInteger("id");
        TCachedQuery txtQry(
                "INSERT INTO wb_msg_text(id, page_no, text) VALUES(:id, :page_no, :text)",
                QParams()
                << QParam("id", otInteger, id)
                << QParam("page_no", otInteger)
                << QParam("text", otString)
                );
        longToDB(txtQry.get(), "text", content);
        TReqInfo::Instance()->LocaleToLog("EVT.WB.PRINT",
                LEvntPrms() << PrmSmpl<string>("msg_type", MsgTypes().encode(msg_type)),
                evtFlt, point_id);
    }

    void parse_print_message(const string &in_content)
    {
        vector<string> lines;
        boost::split(lines, in_content, boost::is_any_of("\n"));
        if(lines.size() < 2)
            throw Exception("Wrong message format");
        // ? ??ࢮ? ??ப? ????⠭?? PRINT ? ??? ᮮ?饭??, ???? "PRINT LOADSHEET"
        vector<string> hdr_items;
        boost::split(hdr_items, lines[0], boost::is_any_of(" "));
        if(hdr_items.size() != 2)
            throw Exception("Wrong header format: '%s'", lines[0].c_str());
        TMsgType::Enum msg_type = MsgTypes().decode(hdr_items[1]);
        // ?? ???ன ??ப? ????? ३??: N49999/09 C ?/? ?뫥?? ?? ????⭮. ???? ???.
        string flight = lines[1];
        // remove any spaces
        flight.erase(remove_if(flight.begin(), flight.end(), ::isspace), flight.end());
        TypeB::TFlightIdentifier flt;
        flt.parse(flight.c_str());

        string airp;
        // ???⠭?? ?? ?뫥??, ?᫨ ????
        vector<string>::const_iterator line = lines.begin();
        for(; line != lines.end(); line++)
            if(line->substr(0, 4) == "FROM") break;
        if(line != lines.end()) {
            line++;
            if(line != lines.end()) {
                TElemFmt fmt;
                airp = ElemToElemId(etAirp, line->substr(0, 3), fmt);
                if(fmt == efmtUnknown) airp.clear();
            }
        }

        TTripInfo trip_info;
        trip_info.airline = flt.airline;
        trip_info.airp = airp;
        trip_info.flt_no = flt.flt_no;
        if(flt.suffix)
            trip_info.suffix.append(1, flt.suffix);
        trip_info.scd_out = flt.date;

        vector<TTripInfo> franchise_flts;
        get_wb_franchise_flts(trip_info, franchise_flts);
        for(const auto &f: franchise_flts) {
            TSearchFltInfo filter;
            filter.airline = f.airline;
            filter.flt_no = f.flt_no;
            filter.suffix = f.suffix;
            filter.airp_dep = f.airp;
            filter.scd_out = f.scd_out;
            filter.scd_out_in_utc = true;

            list<TAdvTripInfo> flts;
            SearchFlt(filter, flts);

            if(flts.empty())
                throw Exception("flight not found: %s", flight.c_str());
            for(list<TAdvTripInfo>::iterator i = flts.begin(); i != flts.end(); ++i)
                toDB(i->point_id, msg_type, in_content);
        }
    }

}

void TelegramInterface::tlg_srv(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ProgTrace(TRACE5, "%s", __FUNCTION__);
    ProgTrace(TRACE5, "ctxt->GetOpr: %s", ctxt->GetOpr().c_str());
    ProgTrace(TRACE5, "ctxt->GetPult: %s", ctxt->GetPult().c_str());
    xmlNodePtr contentNode = GetNode( "content", reqNode );
    if ( contentNode == NULL ) {
        return;
    }
    string content = NodeAsString( contentNode );
    TrimString(content);
    if(content.substr(0, 6) == "PRINT ") { // AHM 517 (are you sure that?)
        try {
            WBMessages::parse_print_message(content);
        } catch(const Exception &E) {
            LogTrace(TRACE5) << "wb msg parse failed: " << E.what();
            NewTextChild(resNode, "content", E.what());
        }
    } else { // ?????ࠬ??
        TypeB::TOriginatorInfo orig = TypeB::getOriginator(
                string(),
                string(),
                string(),
                NowUTC(),
                true
                );
        string sender = "0" + ctxt->GetPult();
        string tlg_text = orig.addr + "\xa." + sender + "\n" + content;

        string tlg_type, airline, airp;
        try {
            get_tlg_info(tlg_text, tlg_type, airline, airp);
            TReqInfo *reqInfo = TReqInfo::Instance();
            if(not(reqInfo->user.access.airlines().permitted(airline) and
                        (airp.empty() or reqInfo->user.access.airps().permitted(airp)))) {
                NewTextChild(resNode, "content", ACCESS_DENIED);
            } else {
                int tlgs_id = loadTlg(tlg_text);
                if(tlg_type == "LCI") { // ??? LCI ?????訢??? ???????, ??? ??⠫???? - ?????. ???⮩ ?⢥?.
                    TypeBHelpMng::configForPerespros(tlgs_id);
                    NewTextChild(resNode, "content", TIMEOUT_OCCURRED);
                }
            }
        } catch(const Exception &E) {
            NewTextChild(resNode, "content", E.what());
        }

    }
}

void ccccccccccccccccccccc( int point_dep,  const ASTRA::TCompLayerType &layer_type )
{
  //try verify its new code!!!
  SALONS2::TSalonList salonList;
  salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_dep, ASTRA::NoExists ), "", NoExists );
  SALONS2::TSectionInfo sectionInfo;
  SALONS2::TGetPassFlags flags;
  flags.clearFlags();
  salonList.getSectionInfo( sectionInfo, flags );
  TPassSeats layerSeats;
  sectionInfo.GetTotalLayerSeat( layer_type, layerSeats );
};

namespace KUF_STAT {

    class TFileType {
        public:
            enum Enum {ftClose, ftTakeoff, ftPax, ftUnknown};

            static const std::list< std::pair<Enum, std::string> >& pairs()
            {
                static std::list< std::pair<Enum, std::string> > l;
                if (l.empty())
                {
                    l.push_back(std::make_pair(ftClose,     "cl"));
                    l.push_back(std::make_pair(ftTakeoff,   "cc"));
                    l.push_back(std::make_pair(ftPax,       "pax"));
                    l.push_back(std::make_pair(ftUnknown,   "unknown"));
                }
                return l;
            }
    };
}

namespace CKIN_REPORT {

    int error(const string &err = "")
    {
        cout << "error: " << err << endl;
        return 1;
    }

    int usage(const string &prg, const string &err = "")
    {
        cout << "usage: " << prg << " UT001/14OCT DME" << endl;
        return 1;
    }

    int get_point_id(TSearchFltInfo &filter)
    {
        list<TAdvTripInfo> flts;
        SearchFlt(filter, flts);

        TNearestDate nd(filter.scd_out);
        for(list<TAdvTripInfo>::iterator i = flts.begin(); i != flts.end(); ++i)
            nd.sorted_points[i->scd_out] = i->point_id;
        int point_id = nd.get();
        return point_id;
    }

    int get_point_id(const string &val, const string &airp)
    {
        TypeB::TFlightIdentifier flt;
        flt.parse(val.c_str());

        TSearchFltInfo filter;
        filter.airline = flt.airline;
        filter.flt_no = flt.flt_no;
        if(flt.suffix)
            filter.suffix.append(1, flt.suffix);
        filter.airp_dep = getElemId(etAirp, airp);
        filter.scd_out = flt.date;
        filter.scd_out_in_utc = true;
        int point_id = get_point_id(filter);
        if(point_id == NoExists)
            throw Exception("flight not found: %s", val.c_str());
        return point_id;
    }

    struct TPaxCounters {
        TByGender pax_count;
        TByGender seat_count;
        TWItem bag;
        bool empty() const { return pax_count.empty(); }
        void append(const TSalonPax &pax);
    };

    void TPaxCounters::append(const TSalonPax &pax)
    {
        TTrickyGender::Enum gender = getGender(pax.pax_id);
        /* ??? ?? ࠡ?⠥?!
           switch(pax.pers_type) {
           case adult:
           gender = pax.is_female ? gFemale : gMale;
           break;
           case child:
           gender = gChild;
           break;
           case baby:
           gender = gInfant;
           break;
           default:
           gender = gMale;
           }
           */
        pax_count.append(gender);
        seat_count.append(gender, pax.seats);

        // adding baggage
        TWItem add_bag;
        add_bag.get(pax.pax_id);
        bag += add_bag;
    }


    struct TCounters {
        int booking;
        TPaxCounters tranzit;
        TPaxCounters goshow;
        TPaxCounters self_checkin;
        bool ckin_empty();
        TCounters(): booking(0) {}
    };

    bool TCounters::ckin_empty() {
        return
            tranzit.empty() and
            goshow.empty() and
            self_checkin.empty();
    }

    struct TCRSPaxList {
        typedef map<string, int> TClsMap;
        typedef map<string, TClsMap> TAirpMap;

        TAirpMap items; // [airp_arv][class] = count
        void get(int point_id);
        void dump();
    };

    void TCRSPaxList::dump()
    {
        LogTrace(TRACE5) << "---TCRSPaxList::dump()---";
        for(TAirpMap::iterator iAirp = items.begin(); iAirp != items.end(); iAirp++) {
            for(TClsMap::iterator iCls = iAirp->second.begin(); iCls != iAirp->second.end(); iCls++) {
                LogTrace(TRACE5)
                    << "TCRSPaxList items[" << iAirp->first << "]"
                    << "[" << iCls->first << "] = " << iCls->second;
            }
        }
        LogTrace(TRACE5) << "-------------------------";
    }

    struct TSelfCkinPaxList {
        set<int> items;
        void get(int point_id);
    };

    void TSelfCkinPaxList::get(int point_id)
    {
        items.clear();
        TCachedQuery Qry(
                "select "
                "   pax_id "
                "from "
                "   points, "
                "   pax_grp, "
                "   pax "
                "where "
                "   points.point_id = :point_id and "
                "   points.point_id = pax_grp.point_dep and "
                "   pax_grp.client_type in (:ctWeb, :ctMobile, :ctKiosk) and "
                "   pax_grp.status not in ('E') and "
                "   pax_grp.grp_id = pax.grp_id and "
                "   pax.pr_brd is not null ",
                QParams()
                << QParam("point_id", otInteger, point_id)
                << QParam("ctWeb", otString, EncodeClientType(ctWeb))
                << QParam("ctMobile", otString, EncodeClientType(ctMobile))
                << QParam("ctKiosk", otString, EncodeClientType(ctKiosk))
                );
        Qry.get().Execute();
        for(; not Qry.get().Eof; Qry.get().Next())
            items.insert(Qry.get().FieldAsInteger(0));
    }

    void TCRSPaxList::get(int point_id)
    {
        items.clear();
        TCachedQuery Qry(
                "select "
                "   crs_pnr.airp_arv, "
                "   crs_pnr.class, "
                "   count(*) amount "
                "from "
                "   crs_pnr, "
                "   tlg_binding, "
                "   crs_pax "
                "where "
                "   crs_pnr.point_id=tlg_binding.point_id_tlg and "
                "   crs_pnr.system='CRS' and "
                "   crs_pnr.pnr_id=crs_pax.pnr_id and "
                "   crs_pax.pr_del=0 and "
                "   tlg_binding.point_id_spp = :point_id "
                "group by "
                "   crs_pnr.airp_arv, "
                "   crs_pnr.class ",
                QParams() << QParam("point_id", otInteger, point_id)
                );
        Qry.get().Execute();
        LogTrace(TRACE5) << "point_id: " << point_id;
        for(; not Qry.get().Eof; Qry.get().Next()) {
            items
                [Qry.get().FieldAsString("airp_arv")]
                [Qry.get().FieldAsString("class")]
                =
                Qry.get().FieldAsInteger("amount");
        }
    }

    typedef map<string, TCounters> TClsRoute;
    typedef map<string, TClsRoute> TAirpRoute;
    typedef map<int, TAirpRoute> TRoute; // [order][airp][class] = count

    struct TReportData;

    typedef void (*TAppendEvent)(
            TPaxMapCoord &pax_map_coord,
            TReportData &report,
            const TSalonPax &pax,
            int routeIdx,
            TTripRouteItem &routeItem
            );

    bool cmpPaxList(const TSalonPax &p1, const TSalonPax &p2)
    {
        if(p1.reg_no == p2.reg_no)
            return p1.pax_id < p2.pax_id;
        else
            return p1.reg_no < p2.reg_no;
    }

    typedef set<TSalonPax, bool(*)(const TSalonPax &, const TSalonPax &)> TPaxList;

    struct TReportData {
        int point_id;
        string airline;
        int flt_no;
        string suffix;
        TDateTime scd_out;
        bool pr_tranz_reg;

        TSelfCkinPaxList self_ckin_pax_list;

        TRoute route;
        TPaxList pax_list;

        void Clear()
        {
            point_id = NoExists;
            airline.clear();
            flt_no = NoExists;
            suffix.clear();
            scd_out = NoExists;
        }

        TReportData(): pax_list(cmpPaxList)
        {
            Clear();
        }

        void get(int point_id);
        void toXML(xmlNodePtr rootNode, KUF_STAT::TFileType::Enum ft);
        void paxLstToXML(xmlNodePtr rootNode);
        void appendArv(
                TSalonPassengers::iterator iDep,
                TIntArvSalonPassengers &arvMap,
                int routeIdx,
                TTripRouteItem &routeItem,
                TAppendEvent append_evt
                );
    };

    void append_evt_transit(
            TPaxMapCoord &pax_map_coord,
            TReportData &report,
            const TSalonPax &pax,
            int routeIdx,
            TTripRouteItem &routeItem
            )
    {
        // ?᫨ ???. ????窠 '????ॣ???????? ?࠭????', ??
        // ? ????稪? ???????? pax.grp_status == psTransit
        // ?᫨ ????窠 ?몫?祭?, ?? ?????, ????騥 ??१ ⥪. point_id (routeItem.point_id)
        if(
                (report.pr_tranz_reg and DecodePaxStatus(pax.grp_status.c_str()) == psTransit)
                or
                (not report.pr_tranz_reg and
                 pax.point_dep == report.point_id and
                 pax_map_coord.point_dep == routeItem.point_id)
          ) {
            report.route[routeIdx][routeItem.airp][pax.cabin_cl].tranzit.append(pax);
            report.pax_list.insert(pax);
        }
    }

    void append_evt_self_checkin(
            TPaxMapCoord &pax_map_coord,
            TReportData &report,
            const TSalonPax &pax,
            int routeIdx,
            TTripRouteItem &routeItem
            )
    {
        if(
                report.self_ckin_pax_list.items.find(pax.pax_id) != report.self_ckin_pax_list.items.end() and
                pax.point_dep == report.point_id and
                pax.point_arv == routeItem.point_id
          ) {
            report.route[routeIdx][routeItem.airp][pax.cabin_cl].self_checkin.append(pax);
            report.pax_list.insert(pax);
        }
    }

    void append_evt_goshow(
            TPaxMapCoord &pax_map_coord,
            TReportData &report,
            const TSalonPax &pax,
            int routeIdx,
            TTripRouteItem &routeItem
            )
    {
        if(
                DecodePaxStatus(pax.grp_status.c_str()) == psGoshow and
                pax.point_dep == report.point_id and
                pax.point_arv == routeItem.point_id
          ) {
            report.route[routeIdx][routeItem.airp][pax.cabin_cl].goshow.append(pax);
            report.pax_list.insert(pax);
        }
    }

    struct TTranzitPaxList {
        void get(int point_id);
    };

    void TTranzitPaxList::get(int point_id)
    {
        SALONS2::TSalonList salonList;
        salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, ASTRA::NoExists ),
                              "", NoExists );
        TSalonPassengers passengers;
        SALONS2::TGetPassFlags flags;
        //        flags.setFlag( SALONS2::gpPassenger );
        flags.setFlag( SALONS2::gpWaitList );
        flags.setFlag( SALONS2::gpTranzits );
        flags.setFlag( SALONS2::gpInfants );
        salonList.getPassengers( passengers, flags );
        passengers.dump();
    }

    void TReportData::appendArv(
            TSalonPassengers::iterator iDep,
            TIntArvSalonPassengers &arvMap,
            int routeIdx,
            TTripRouteItem &routeItem,
            TAppendEvent append_evt
            )
    {
        for(TIntArvSalonPassengers::const_iterator
                iArv = arvMap.begin();
                iArv != arvMap.end();
                iArv++) {
            for(TIntClassSalonPassengers::const_iterator
                    iCls = iArv->second.begin();
                    iCls != iArv->second.end();
                    iCls++) {
                for(TIntStatusSalonPassengers::const_iterator
                        iStatus = iCls->second.begin();
                        iStatus != iCls->second.end();
                        iStatus++) {
                    for(set<TSalonPax,ComparePassenger>::const_iterator
                            iPax = iStatus->second.begin();
                            iPax != iStatus->second.end();
                            iPax++) {
                        TPaxMapCoord pax_map_coord;
                        pax_map_coord.point_dep = iDep->first;
                        pax_map_coord.point_arv = iArv->first;
                        pax_map_coord.cls = iCls->first;
                        pax_map_coord.grp_status = iStatus->first;
                        (*append_evt)(pax_map_coord, *this, *iPax, routeIdx, routeItem);
                    }
                }
            }
        }
    }

    void TReportData::get(int apoint_id)
    {
        Clear();

        point_id = apoint_id;

        TTripRoute before_route;
        try {
            before_route.GetRouteBefore(NoExists, apoint_id, trtWithCurrent, trtNotCancelled);
            if(not before_route.empty())
                point_id = before_route[0].point_id;
        } catch(const Exception &E) {
            LogTrace(TRACE5) << "TReportData::get: before_route failed: " << E.what();
        } catch(...) {
            LogTrace(TRACE5) << "TReportData::get: before_route failed: unexpected";
        }

        TQuery Qry(&OraSession);
        Qry.SQLText =
            "select "
            "   points.airline, "
            "   points.flt_no, "
            "   points.suffix, "
            "   points.scd_out, "
            "   trip_sets.pr_tranz_reg "
            "from "
            "   points, "
            "   trip_sets "
            "where "
            "   points.point_id = :point_id and "
            "   points.point_id = trip_sets.point_id ";
        Qry.CreateVariable("point_id", otInteger, point_id);
        Qry.Execute();

        if(Qry.Eof)
            throw UserException("flight not found");

        airline = Qry.FieldAsString("airline");
        flt_no = Qry.FieldAsInteger("flt_no");
        suffix = Qry.FieldAsString("suffix");
        scd_out = Qry.FieldAsDateTime("scd_out");
        pr_tranz_reg = Qry.FieldAsInteger("pr_tranz_reg") != 0;


        SALONS2::TSalonList salonList;
        SALONS2::TGetPassFlags flags;

        try {
            salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, ASTRA::NoExists ),
                                  "", NoExists );
        } catch(const Exception &E) {
            LogTrace(TRACE5) << "TReportData::get: salonList.ReadFlight failed: " << E.what();
        } catch(...) {
            LogTrace(TRACE5) << "TReportData::get: salonList.ReadFlight failed: unexpected";
        }

        TSalonPassengers tranzit_pax_map;
        try {
            flags.setFlag( SALONS2::gpWaitList );
            flags.setFlag( SALONS2::gpTranzits );
            flags.setFlag( SALONS2::gpInfants );
            salonList.getPassengers( tranzit_pax_map, flags );
        } catch(const Exception &E) {
            LogTrace(TRACE5) << "TReportData::get: tranzit_pax_map failed: " << E.what();
        } catch(...) {
            LogTrace(TRACE5) << "TReportData::get: tranzit_pax_map failed: unexpected";
        }

        TSalonPassengers pax_map;
        try {
            flags.setFlag( SALONS2::gpPassenger );
            flags.clearFlag(SALONS2::gpTranzits);
            salonList.getPassengers( pax_map, flags );
        } catch(const Exception &E) {
            LogTrace(TRACE5) << "TReportData::get: pax_map failed: " << E.what();
        } catch(...) {
            LogTrace(TRACE5) << "TReportData::get: pax_map failed: unexpected";
        }



        TCRSPaxList crs_pax_list;
        try {
            crs_pax_list.get(point_id);
            crs_pax_list.dump();
        } catch(const Exception &E) {
            LogTrace(TRACE5) << "TReportData::get: crs_pax_list failed: " << E.what();
        } catch(...) {
            LogTrace(TRACE5) << "TReportData::get: crs_pax_list failed: unexpected";
        }

        self_ckin_pax_list.get(point_id);

        TTripRoute trip_route;
        try {
            trip_route.GetRouteAfter(NoExists, point_id, trtWithCurrent, trtNotCancelled);
        } catch(const Exception &E) {
            LogTrace(TRACE5) << "TReportData::get: trip_route failed: " << E.what();
        } catch(...) {
            LogTrace(TRACE5) << "TReportData::get: trip_route failed: unexpected";
        }

        int idx = 1;
        for(TTripRoute::iterator iRoute = trip_route.begin(); iRoute != trip_route.end(); iRoute++, idx++) {
            // booking info
            route[idx][iRoute->airp]["?"].booking = crs_pax_list.items[iRoute->airp]["?"];
            route[idx][iRoute->airp]["?"].booking = crs_pax_list.items[iRoute->airp]["?"];
            route[idx][iRoute->airp]["?"].booking = crs_pax_list.items[iRoute->airp]["?"];

            // checkin info

            for(TSalonPassengers::iterator
                    iDep = pax_map.begin();
                    iDep != pax_map.end();
                    iDep++) {
                // self checkin
                appendArv(iDep, iDep->second.infants, idx, *iRoute, append_evt_self_checkin);
                appendArv(iDep, iDep->second, idx, *iRoute, append_evt_self_checkin);

                // goshow
                appendArv(iDep, iDep->second.infants, idx, *iRoute, append_evt_goshow);
                appendArv(iDep, iDep->second, idx, *iRoute, append_evt_goshow);
            }
            for(TSalonPassengers::iterator
                    iDep = tranzit_pax_map.begin();
                    iDep != tranzit_pax_map.end();
                    iDep++) {
                // tranzit
                appendArv(iDep, iDep->second.infants, idx, *iRoute, append_evt_transit);
                appendArv(iDep, iDep->second, idx, *iRoute, append_evt_transit);
            }
        }
    }

    void genderToXML(xmlNodePtr parentNode, const string &path, const TPaxCounters &gender, const string &cls)
    {
        if(gender.empty()) return;

        vector<string> tokens;
        boost::split(tokens, path, boost::is_any_of("/"));

        xmlNodePtr lvl1Node = NULL;
        xmlNodePtr lvl2Node = NULL;

        if(not (lvl1Node = GetNode(tokens[0].c_str(), parentNode)))
            lvl1Node = NewTextChild(parentNode, tokens[0].c_str());

        if(not (lvl2Node = GetNode(tokens[1].c_str(), lvl1Node)))
            lvl2Node = NewTextChild(lvl1Node, tokens[1].c_str());

        xmlNodePtr classNode = NewTextChild(lvl2Node, "class");
        SetProp(classNode, "code", cls);
        if(gender.pax_count.m) SetProp(NewTextChild(classNode, "male", (int)gender.pax_count.m), "seats", gender.seat_count.m);
        if(gender.pax_count.f)SetProp(NewTextChild(classNode, "female", (int)gender.pax_count.f), "seats", gender.seat_count.f);
        if(gender.pax_count.c)SetProp(NewTextChild(classNode, "chd", (int)gender.pax_count.c), "seats", gender.seat_count.c);
        if(gender.pax_count.i)SetProp(NewTextChild(classNode, "inf", (int)gender.pax_count.i), "seats", gender.seat_count.i);
        if(gender.bag.rkWeight) NewTextChild(classNode, "rk_weight", gender.bag.rkWeight);
        if(gender.bag.bagAmount) NewTextChild(classNode, "bag_amount", gender.bag.bagAmount);
        if(gender.bag.bagWeight) NewTextChild(classNode, "bag_weight", gender.bag.bagWeight);
        if(!gender.bag.excess_wt.zero()) NewTextChild(classNode, "paybag_weight", gender.bag.excess_wt.getQuantity());
    }

    string get_seats(const TCheckinPaxSeats &seats)
    {
        string result;
        for(set<TTlgCompLayer,TCompareCompLayers>::const_iterator is = seats.seats.begin(); is != seats.seats.end(); is++) {
            if(not result.empty()) result += " ";
            result += is->denorm_view(true);
        }
        return result;
    }

    typedef map<int, vector<TSalonPax> > TChilds;

    string get_chd(const TChilds &chd, const TSalonPax &pax, PersWeightRules &pwr)
    {
        LogTrace(TRACE5) << "get_chd pax_id: " << pax.pax_id;

        ClassesPersWeight cpw;
        pwr.weight(pax.cabin_cl, string(), cpw);

        TChilds::const_iterator iChd = chd.find(pax.pax_id);

        ostringstream result;

        if(iChd != chd.end()) {
            int amount = 0;
            int weight = 0;
            string names;
            for(vector<TSalonPax>::const_iterator i = iChd->second.begin(); i != iChd->second.end(); i++) {
                amount++;
                weight += cpw.child;
                if(not names.empty()) names += " ";
                names += i->name;
            }
            if(amount) result << amount << "/" << weight << "/" << names;
        }
        return result.str();
    }

    string get_inf(const TInfants &inf, const TSalonPax &pax, PersWeightRules &pwr)
    {
        ostringstream result;
        for(vector<TInfantsItem>::const_iterator i = inf.items.begin(); i != inf.items.end(); i++) {
            if(i->parent_pax_id == pax.pax_id) {
                ClassesPersWeight cpw;
                pwr.weight(pax.cabin_cl, string(), cpw);
                result << "1/" << cpw.infant << "/" << i->name;
            }
        }
        return result.str();
    }

    void SetChildsToAdults(const TPaxList &pax_list, TChilds &childs)
    {
        set<int> chd_processed;
        for(TPaxList::const_iterator parentPax = pax_list.begin(); parentPax != pax_list.end(); parentPax++) {
            for(int k = 1; k <= 2; k++) {
                for(TPaxList::const_iterator chdPax = pax_list.begin(); chdPax != pax_list.end(); chdPax++) {
                    if(
                            parentPax->pax_id != chdPax->pax_id and
                            parentPax->pers_type == adult and
                            chdPax->pers_type == child and
                            parentPax->grp_id == chdPax->grp_id and
                            chd_processed.find(chdPax->pax_id) == chd_processed.end() and
                            ((k == 1 and parentPax->surname == chdPax->surname) or
                             (k == 2))
                      ) {
                        childs[parentPax->pax_id].push_back(*chdPax);
                        chd_processed.insert(chdPax->pax_id);
                    }
                }
            }
        }
    }

    int get_bag_pool_num(int pax_id)
    {
        TCachedQuery Qry("select bag_pool_num from pax where pax_id = :pax_id",
                QParams() << QParam("pax_id", otInteger, pax_id));
        Qry.get().Execute();
        int result = NoExists;
        if(not Qry.get().Eof and not Qry.get().FieldIsNULL("bag_pool_num"))
            result = Qry.get().FieldAsInteger("bag_pool_num");
        return result;
    }

    string get_bag_tags(const TSalonPax &pax, int bag_pool_num)
    {
        TCachedQuery Qry(
                "select ckin.get_birks2(:grp_id,:pax_id,:bag_pool_num,1) from dual",
                QParams()
                << QParam("grp_id", otInteger, pax.grp_id)
                << QParam("pax_id", otInteger, pax.pax_id)
                << QParam("bag_pool_num", otInteger, bag_pool_num));
        Qry.get().Execute();
        string result;
        if(not Qry.get().Eof and not Qry.get().FieldIsNULL(0))
            result = Qry.get().FieldAsString(0);
        return result;
    }

    string RouteItemToStr(const boost::optional<TCkinRouteItem>& route_item)
    {
        ostringstream result;
        if(route_item) {

            TCachedQuery grpQry("select * from pax_grp where grp_id = :grp_id",
                    QParams() << QParam("grp_id", otInteger, route_item.get().grp_id));
            grpQry.get().Execute();
            string cls = CheckIn::TSimplePaxGrpItem().fromDB(grpQry.get()).cl;

            TTripInfo trip_info;
            trip_info.getByPointId(route_item.get().point_dep);
            TElemFmt fmt;
            result
                << ElemToElemId(etAirline, trip_info.airline, fmt, LANG_EN)
                << setw(3) << setfill('0') << trip_info.flt_no
                << ElemToElemId(etSuffix, trip_info.suffix, fmt, LANG_EN)
                << "/"
                << ElemToElemId(etClass, cls, fmt, LANG_EN) << "/"
                << DateTimeToStr(trip_info.scd_out, "ddmmm", 1) << "/"
                << ElemToElemId(etAirp, trip_info.airp, fmt, LANG_EN);
        }
        return result.str();
    }

    bool get_norec(const TPFSBody &pfs, int pax_id)
    {
        // ? NOREC ???????? ⮫쪮 ??ᠦ????? NOREC
        // LogTrace(TRACE5) << "get_norec pax_id: " << pax_id;
        bool result = false;
        for(TPFSItems::const_iterator
                target = pfs.items.begin();
                target != pfs.items.end();
                target++) {
            for(TPFSCtgryList::const_iterator
                    category = target->second.begin();
                    category != target->second.end();
                    category++) {
                for(TPFSClsList::const_iterator
                        cls = category->second.begin();
                        cls != category->second.end();
                        cls++) {
                    ostringstream pax_lst;
                    for(TPFSPaxList::const_iterator
                            pax = cls->second.begin();
                            pax != cls->second.end();
                            pax++) {
                        if(not pax_lst.str().empty()) pax_lst << ", ";
                        pax_lst << pax->pax_id;
                        if(pax->pax_id == pax_id) {
                            result = category->first == "NOREC";
                            break;
                        }
                    }
                    /*
                    LogTrace(TRACE5)
                        << "pfs[" << target->first << "]"
                        << "[" << category->first << "]"
                        << "[" << cls->first << "] = "
                        << pax_lst.str();
                        */
                }
            }
        }
        return result;
    }

    void TReportData::paxLstToXML(xmlNodePtr rootNode)
    {
        xmlNodePtr airlineNode = NewTextChild(rootNode, "airline");
        SetProp(airlineNode, "code_zrt", airline);
        const TAirlinesRow &row=(const TAirlinesRow&)(base_tables.get("airlines").get_row("code",airline));
        SetProp(airlineNode, "code_iata", row.code_lat);
        NewTextChild(rootNode, "flt_no",  IntToString(flt_no) + suffix);
        NewTextChild(rootNode, "date_scd", DateTimeToStr(scd_out, "dd.mm.yyyy hh:nn:ss"));
        xmlNodePtr paxNode = NewTextChild(rootNode, "passengers");
        if(pax_list.empty()) return;

        map<int,TCheckinPaxSeats> checkinPaxsSeats;
        getSalonPaxsSeats(point_id, checkinPaxsSeats, false);

        TInfants inf;
        {
            TypeB::TCreateInfo createInfo("LCI", TypeB::TCreatePoint());
            TypeB::TDetailCreateInfo info;
            info.create_point = createInfo.create_point;
            info.copy(createInfo);
            info.point_id = point_id;
            inf.get(info);
        }

        TPFSBody pfs;
        {
            TypeB::TCreateInfo createInfo("PFS", TypeB::TCreatePoint());
            TypeB::TDetailCreateInfo info;
            info.create_point = createInfo.create_point;
            info.copy(createInfo);
            info.point_id = point_id;
            pfs.get(info);
        }

        PersWeightRules pwr;
        pwr.read(point_id);

        TChilds childs;
        SetChildsToAdults(pax_list, childs);

        map<int, CheckIn::TSimplePaxGrpItem> grps;
        TCachedQuery grpQry("select * from pax_grp where grp_id = :grp_id",
                QParams() << QParam("grp_id", otInteger));

        for(TPaxList::iterator iPax = pax_list.begin(); iPax != pax_list.end(); iPax++) {
            map<int, CheckIn::TSimplePaxGrpItem>::iterator grp = grps.find(iPax->grp_id);
            if(grp == grps.end()) {
                grpQry.get().SetVariable("grp_id", iPax->grp_id);
                grpQry.get().Execute();
                pair<map<int, CheckIn::TSimplePaxGrpItem>::iterator, bool> res = grps.insert(make_pair(iPax->grp_id, CheckIn::TSimplePaxGrpItem().fromDB(grpQry.get())));
                grp = res.first;
            }

            int bag_pool_num = get_bag_pool_num(iPax->pax_id);

            xmlNodePtr itemNode = NewTextChild(paxNode, "passenger");
            NewTextChild(itemNode, "pnr", TPnrAddrs().getByPaxId(iPax->pax_id));
            NewTextChild(itemNode, "name", iPax->name + " " + iPax->surname);
            NewTextChild(itemNode, "del");
            NewTextChild(itemNode, "grp");
            NewTextChild(itemNode, "sn", get_seats(checkinPaxsSeats[iPax->pax_id]));
            NewTextChild(itemNode, "clss", iPax->cabin_cl);
            NewTextChild(itemNode, "ures");
            NewTextChild(itemNode, "inf", get_inf(inf, *iPax, pwr));
            NewTextChild(itemNode, "chd", get_chd(childs, *iPax, pwr));

            TWItem w;
            w.get(iPax->pax_id);
            ostringstream buf;
            if(w.bagAmount != 0 or w.bagWeight != 0 or w.rkWeight != 0)
                buf << w.bagAmount << "/" << w.bagWeight << "/" << w.rkWeight;
            NewTextChild(itemNode, "bag", buf.str());
            buf.str("");
            if(!w.excess_wt.zero())
                buf << w.excess_wt.getQuantity() << KG;
            NewTextChild(itemNode, "xbag", buf.str());

            NewTextChild(itemNode, "bagtags", get_bag_tags(*iPax, bag_pool_num));

            set<CheckIn::TPaxFQTItem> fqts;
            CheckIn::TPaxFQTCards fqt_cards;
            if (LoadPaxFQT(iPax->pax_id, fqts))
                CheckIn::GetPaxFQTCards(fqts, fqt_cards);
            buf.str("");
            if(not fqt_cards.empty())
                buf << fqt_cards.begin()->first.airline << " " << fqt_cards.begin()->first.no;
            NewTextChild(itemNode, "fqtv", buf.str());

            CheckIn::TPaxTknItem tkn;
            LoadPaxTkn(NoExists, iPax->pax_id, tkn);
            NewTextChild(itemNode, "tkna", ((not tkn.empty() and tkn.rem == "TKNA") ? tkn.no_str(): ""));

            NewTextChild(itemNode, "tknm");

            auto outbound=TCkinRoute::getNextGrp(GrpId_t(iPax->grp_id),
                                                 TCkinRoute::IgnoreDependence,
                                                 TCkinRoute::WithoutTransit);
            auto inbound=TCkinRoute::getPriorGrp(GrpId_t(iPax->grp_id),
                                                 TCkinRoute::IgnoreDependence,
                                                 TCkinRoute::WithoutTransit);
            NewTextChild(itemNode, "outbound", RouteItemToStr(outbound));
            NewTextChild(itemNode, "inbound", RouteItemToStr(inbound));

            NewTextChild(itemNode, "z");

            NewTextChild(itemNode, "passnum", iPax->reg_no);

            const TAirpsRow &row=(const TAirpsRow&)(base_tables.get("airps").get_row("code",grp->second.airp_arv));
            NewTextChild(itemNode, "dest", row.code_lat);
            NewTextChild(itemNode, "sb");
            NewTextChild(itemNode, "nrec", get_norec(pfs, iPax->pax_id));

            string hall;
            if(grp->second.hall != NoExists)
                hall = ElemIdToNameLong(etHall, grp->second.hall);
            NewTextChild(itemNode, "hall", hall);

            NewTextChild(itemNode, "tkne", ((not tkn.empty() and tkn.rem == "TKNE") ? tkn.no_str(): ""));
            NewTextChild(itemNode, "selfcheckin",
                    grp->second.client_type == ctWeb or
                    grp->second.client_type == ctMobile or
                    grp->second.client_type == ctKiosk
                    );
        }
    }

    void TReportData::toXML(xmlNodePtr rootNode, KUF_STAT::TFileType::Enum ft)
    {
        xmlNodePtr airlineNode = NewTextChild(rootNode, "airline");
        SetProp(airlineNode, "code_zrt", airline);
        const TAirlinesRow &row=(const TAirlinesRow&)(base_tables.get("airlines").get_row("code",airline));
        SetProp(airlineNode, "code_iata", row.code_lat);
        NewTextChild(rootNode, "flt_no",  IntToString(flt_no) + suffix);
        NewTextChild(rootNode, "date_scd", DateTimeToStr(scd_out, "dd.mm.yyyy hh:nn:ss"));

        string status;
        if(ft == KUF_STAT::TFileType::ftClose) status = "close";
        if(ft == KUF_STAT::TFileType::ftTakeoff) status = "flight_close";
        if(status.empty())
            throw Exception("TReportData::toXML: unknown file type %d", ft);

        NewTextChild(rootNode, "status",  status);

        for(TRoute::iterator idx = route.begin(); idx != route.end(); idx++) {
            xmlNodePtr routeNode = NewTextChild(rootNode, "route");
            SetProp(routeNode, "num", idx->first);
            for(TAirpRoute::iterator iAirp = idx->second.begin(); iAirp != idx->second.end(); iAirp++) {
                xmlNodePtr airpNode = NewTextChild(routeNode, "airp");
                SetProp(airpNode, "code_zrt", iAirp->first);
                const TAirpsRow &row=(const TAirpsRow&)(base_tables.get("airps").get_row("code",iAirp->first));
                SetProp(airpNode, "code_iata", row.code_lat);
                if(not iAirp->second.empty()) {
                    for(TClsRoute::iterator iCls = iAirp->second.begin(); iCls != iAirp->second.end(); iCls++) {
                        if(iCls->second.booking) {
                            xmlNodePtr bookingNode = getNode(routeNode, "booking");
                            xmlNodePtr classNode = NewTextChild(bookingNode, "class", iCls->second.booking);
                            SetProp(classNode, "code", iCls->first);
                        }
                        genderToXML(routeNode, "checkin/tranzit", iCls->second.tranzit, iCls->first);
                        genderToXML(routeNode, "checkin/goshow", iCls->second.goshow, iCls->first);
                        genderToXML(routeNode, "checkin/self_checkin", iCls->second.self_checkin, iCls->first);
                    }
                }
            }
        }
    }
}

namespace KUF_STAT {

    class TFileTypes : public ASTRA::PairList<TFileType::Enum, std::string>
    {
        private:
            virtual std::string className() const { return "TFileTypes"; }
        public:
            TFileTypes() : ASTRA::PairList<TFileType::Enum, std::string>(TFileType::pairs(),
                    boost::none,
                    boost::none) {}
    };


    class TAirlines
    {
        private:
            set<string> items;
        public:
            TAirlines()
            {
                TCachedQuery airlinesQry("select airline from kuf_stat_airlines");
                airlinesQry.get().Execute();
                for(; not airlinesQry.get().Eof; airlinesQry.get().Next()) {
                    items.insert(airlinesQry.get().FieldAsString("airline"));
                }
            }
            bool find(const string &airline)
            {
                return items.find(airline) != items.end();
            }

    };

    class TAirps
    {
        private:
            set<string> items;
        public:
            TAirps()
            {
                TCachedQuery airpsQry("select airp from kuf_stat_airps");
                airpsQry.get().Execute();
                for(; not airpsQry.get().Eof; airpsQry.get().Next()) {
                    items.insert(airpsQry.get().FieldAsString("airp"));
                }
            }
            bool find(const string &airp)
            {
                return items.find(airp) != items.end();
            }

    };

    string fromDB(int point_id, TFileType::Enum file_type, string &fname)
    {
        fname.clear();
        TCachedQuery Qry(
                "select file_name, text from kuf_stat, kuf_stat_text "
                "where "
                "   kuf_stat.point_id = :point_id and "
                "   kuf_stat.file_type = :file_type and "
                "   kuf_stat.id = kuf_stat_text.id "
                "order by "
                "   page_no",
                QParams()
                << QParam("point_id", otInteger, point_id)
                << QParam("file_type", otString, TFileTypes().encode(file_type))
                );
        Qry.get().Execute();
        string result;
        for (; not Qry.get().Eof; Qry.get().Next()) {
            if(fname.empty()) fname = Qry.get().FieldAsString("file_name");
            result += Qry.get().FieldAsString("text");
        }
        return result;
    }

    void toDB(const TTripInfo tripInfo, TFileType::Enum ftype, const string &data)
    {
        ostringstream fname;
        fname
            << tripInfo.airline
            << setw(3) << setfill('0') << tripInfo.flt_no
            << tripInfo.suffix << '.'
            << DateTimeToStr(tripInfo.scd_out, "dd.mm.yy") << '.'
            << TFileTypes().encode(ftype)
            << ".xml";
        TCachedQuery Qry(
                "begin "
                "   delete from kuf_stat_text where id = (select id from kuf_stat where point_id = :point_id and file_type = :file_type); "
                "   delete from kuf_stat where point_id = :point_id and file_type = :file_type; "
                "   insert into kuf_stat(id, point_id, file_type, file_name)  values(tid__seq.nextval, :point_id, :file_type, :fname) returning id into :id; "
                "end; ",
                QParams()
                << QParam("point_id", otInteger, tripInfo.point_id)
                << QParam("file_type", otString, TFileTypes().encode(ftype))
                << QParam("fname", otString, fname.str())
                << QParam("id", otInteger)
                );
        Qry.get().Execute();
        int id = Qry.get().GetVariableAsInteger("id");
        TCachedQuery txtQry(
                "insert into kuf_stat_text(id, page_no, text) values(:id, :page_no, :text)",
                QParams()
                << QParam("id", otInteger, id)
                << QParam("page_no", otInteger)
                << QParam("text", otString)
                );
        longToDB(txtQry.get(), "text", data);
    }

    string getFileData(int point_id, TFileType::Enum ft)
    {
        XMLDoc doc("flight");
        xmlNodePtr rootNode = doc.docPtr()->children;

        CKIN_REPORT::TReportData data;
        data.get(point_id);
        if(ft == TFileType::ftPax)
            data.paxLstToXML(rootNode);
        else
            data.toXML(rootNode, ft);
        xml_encode_nodelist(rootNode);
        return StrUtils::b64_encode(GetXMLDocText(doc.docPtr()));
    }

    void run(const TTripInfo &tripInfo, TFileType::Enum ftype)
    {
        KUF_STAT::toDB(tripInfo, ftype, KUF_STAT::getFileData(tripInfo.point_id, ftype));
    }

    int fix(int argc, char **argv)
    {
        TQuery delQry(&OraSession);
        delQry.SQLText = "delete from kuf_stat_text where id = :id";
        delQry.DeclareVariable("id", otInteger);

        TQuery txtQry(&OraSession);
        txtQry.SQLText = "insert into kuf_stat_text(id, page_no, text) values(:id, :page_no, :text)",
        txtQry.DeclareVariable("id", otInteger);
        txtQry.DeclareVariable("page_no", otInteger);
        txtQry.DeclareVariable("text", otString);

        TQuery Qry(&OraSession);
        Qry.SQLText =
            "select id, point_id, file_type from kuf_stat where file_type in(:close, :flight_close) order by point_id";
        Qry.CreateVariable("close", otString, TFileTypes().encode(TFileType::ftClose));
        Qry.CreateVariable("flight_close", otString, TFileTypes().encode(TFileType::ftTakeoff));
        Qry.Execute();
        int count = 0;
        int fixed = 0;
        for(; not Qry.Eof; Qry.Next(), count++) {
            int id = Qry.FieldAsInteger("id");
            int point_id = Qry.FieldAsInteger("point_id");
            TFileType::Enum file_type = TFileTypes().decode(Qry.FieldAsString("file_type"));
            string fname;
            string content = StrUtils::b64_decode(KUF_STAT::fromDB(point_id, file_type, fname));

            XMLDoc doc(content);
            if(doc.docPtr()){
                // ????? ??稭????? ???ࠢ????? ???⥭??
                xmlNodePtr flightNode = NodeAsNode("/flight", doc.docPtr());
                xmlNodePtr curNode = flightNode->children;
                xmlNodePtr statusNode = NodeAsNodeFast("status", curNode);
                string status = NodeAsString(statusNode);
                if(status == "CC")
                    status = "flight_close";
                else if(status == "CL")
                    status = "close";
                else
                    continue;

                NodeSetContent(statusNode, status);

                xmlNodePtr routeNode = NodeAsNodeFast("route", curNode);
                for(; routeNode; routeNode = routeNode->next) {
                    xmlNodePtr airpNode = routeNode->children;
                    if(airpNode->children) {
                        CopyNodeList(routeNode, airpNode);
                        RemoveChildNodes(airpNode);
                    }
                }

                content = StrUtils::b64_encode(GetXMLDocText(doc.docPtr()));

                delQry.SetVariable("id", id);
                delQry.Execute();

                txtQry.SetVariable("id", id);
                longToDB(txtQry, "text", content);
                OraSession.Commit();
                fixed++;
            }

        }
        cout << "total reports: " << count << endl;
        cout << fixed << " reports fixed" << endl;

        return 1;
    }
}

void TelegramInterface::kuf_stat_flts(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TDateTime scd_out;
    string str_scd_out = html_get_param("scd_out", reqNode);
    if(StrToDateTime(str_scd_out.c_str(), "dd.mm.yy", scd_out) == EOF)
        throw Exception("kuf_stat_flts: can't convert scd_out: %s", str_scd_out.c_str());
    TCachedQuery Qry("select * from points where scd_out >= :scd_out and scd_out < :scd_out + 1 and pr_del >= 0",
            QParams() << QParam("scd_out", otDate, scd_out));
    Qry.get().Execute();
    xmlNodePtr contentNode = NewTextChild(resNode, "content");
    xmlNodePtr fltsNode = NewTextChild(contentNode, "flts");

    TCachedQuery fileQry("select file_type, file_name from kuf_stat where point_id = :point_id",
            QParams() << QParam("point_id", otInteger));

    for(; not Qry.get().Eof; Qry.get().Next()) {
        int point_id = Qry.get().FieldAsInteger("point_id");

        TTripInfo tripInfo(Qry.get());

        if (not TReqInfo::Instance()->user.access.airlines().permitted(tripInfo.airline) or
                not TReqInfo::Instance()->user.access.airps().permitted(tripInfo.airp) ) continue;

        TTripStage ts;
        TStage stage = sNoActive;
        bool pr_close_checkin = false;
        TTripStages::LoadStage(point_id, sCloseCheckIn, ts);
        if(ts.act != NoExists) {
            stage = sCloseCheckIn;
            pr_close_checkin = true;
        }
        TTripStages::LoadStage(point_id, sTakeoff, ts);
        if(ts.act != NoExists) stage = sTakeoff;

        if(stage == sNoActive) continue;

        fileQry.get().SetVariable("point_id", point_id);
        fileQry.get().Execute();

        map<KUF_STAT::TFileType::Enum, string> files;
        for(; not fileQry.get().Eof; fileQry.get().Next())
            files.insert(make_pair(
                    KUF_STAT::TFileTypes().decode(fileQry.get().FieldAsString("file_type")),
                    fileQry.get().FieldAsString("file_name")));

        if(files.empty()) continue;

        ostringstream flt_name;
        flt_name
            << tripInfo.airline
            << setw(3) << setfill('0') << tripInfo.flt_no
            << tripInfo.suffix << ' '
            << tripInfo.airp;

        xmlNodePtr itemNode = NewTextChild(fltsNode, "item");

        xmlNodePtr fltNode = NewTextChild(itemNode, "flt", flt_name.str());
        SetProp(fltNode, "flt_status", EncodeStage(stage));
        SetProp(fltNode, "point_id", point_id);

        NewTextChild(itemNode, "status", ElemIdToNameLong(etGraphStage, stage));

        for(
                std::list< std::pair<KUF_STAT::TFileType::Enum, std::string> >::const_iterator iFTypes = KUF_STAT::TFileType::pairs().begin();
                iFTypes != KUF_STAT::TFileType::pairs().end(); iFTypes++) {
            if(iFTypes->first == KUF_STAT::TFileType::ftUnknown) continue;
            if(
                    files.find(iFTypes->first) == files.end() or
                    (stage != sTakeoff and iFTypes->first != KUF_STAT::TFileType::ftClose) or
                    (not pr_close_checkin and iFTypes->first == KUF_STAT::TFileType::ftClose)
                    )
                NewTextChild(itemNode, KUF_STAT::TFileTypes().encode(iFTypes->first).c_str());
            else
                NewTextChild(itemNode, KUF_STAT::TFileTypes().encode(iFTypes->first).c_str(), files[iFTypes->first]);
        }

    }
}

void TelegramInterface::kuf_file(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    try {
        string uri_path = NodeAsString("uri_path", reqNode);
        vector<string> items;
        boost::split(items, uri_path, boost::is_any_of("/"));

        if(items.size() != 5 or not items[0].empty())
            throw Exception("kuf_file: wrong query format");
        int qry_file_type = ToInt(items[1]);
        TSearchFltInfo filter;
        filter.airline = "??";
        filter.flt_no = ToInt(items[2]);
        filter.airp_dep = getElemId(etAirp, items[4]);

        if(StrToDateTime(items[3].c_str(), "dd.mm.yy", filter.scd_out) == EOF)
            throw Exception("kuf_file: can't convert scd_out: %s", items[3].c_str());
        filter.scd_out_in_utc = true;

        list<TAdvTripInfo> flts;
        SearchFlt(filter, flts);

        TNearestDate nd(filter.scd_out);
        for(list<TAdvTripInfo>::iterator i = flts.begin(); i != flts.end(); ++i)
            nd.sorted_points[i->scd_out] = i->point_id;
        int point_id = nd.get();
        if(point_id == NoExists)
            throw Exception("kuf_file: flight not found");

        TStage db_stage = sNoActive;
        TTripStage ts;
        TTripStages::LoadStage(point_id, sCloseCheckIn, ts);
        if(ts.act != NoExists) db_stage = sCloseCheckIn;
        TTripStages::LoadStage(point_id, sTakeoff, ts);
        if(ts.act != NoExists) db_stage = sTakeoff;

        xmlNodePtr contentNode = NewTextChild(resNode, "content");

        if(db_stage != sNoActive) {
            KUF_STAT::TFileType::Enum file_type = KUF_STAT::TFileType::ftUnknown;
            switch(qry_file_type) {
                case 1:
                    file_type = db_stage == sCloseCheckIn ? KUF_STAT::TFileType::ftClose : KUF_STAT::TFileType::ftTakeoff;
                    break;
                case 2:
                    if(db_stage == sTakeoff)
                        file_type = KUF_STAT::TFileType::ftPax;
                    break;
                default:
                    throw Exception("kuf_file: unknown file type %d", qry_file_type);
                    break;
            }

            if(file_type != KUF_STAT::TFileType::ftUnknown) {
                string fname;
                NodeSetContent(contentNode, KUF_STAT::fromDB(point_id, file_type, fname));
                SetProp(contentNode, "b64", true);
            }
        }
    } catch(const Exception &E) {
        NewTextChild(resNode, "content", E.what());
    }
}

void TelegramInterface::kuf_stat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id = NoExists;
    point_id = ToInt(html_get_param("point_id", reqNode));
    string file_name = html_get_param("file_name", reqNode);
    KUF_STAT::TFileType::Enum file_type = KUF_STAT::TFileTypes().decode(html_get_param("file_type", reqNode));
    TStage flt_status = DecodeStage(html_get_param("flt_status", reqNode).c_str());

    TStage db_stage = sNoActive;
    TTripStage ts;
    TTripStages::LoadStage(point_id, sCloseCheckIn, ts);
    if(ts.act != NoExists) db_stage = sCloseCheckIn;
    TTripStages::LoadStage(point_id, sTakeoff, ts);
    if(ts.act != NoExists) db_stage = sTakeoff;

    if(db_stage != flt_status)
        throw UserException("?????? ????५?.");

    string fname;
    string data = KUF_STAT::fromDB(point_id, file_type, fname);

    xmlNodePtr contentNode = NewTextChild(resNode, "content");
    xmlNodePtr fileNode = NewTextChild(contentNode, "file");
    NewTextChild(fileNode, "name", fname);
    NewTextChild(fileNode, "data", data);
}

namespace PFS_STAT {
    void toDB(int point_id, TPFSBody &pfs)
    {
        TCachedQuery Qry(
                "insert into pfs_stat ( "
                "   point_id, "
                "   pax_id, "
                "   status, "
                "   airp_arv, "
                "   seats, "
                "   subcls, "
                "   pnr, "
                "   surname, "
                "   name, "
                "   gender, "
                "   birth_date "
                ") values ( "
                "   :point_id, "
                "   :pax_id, "
                "   :status, "
                "   :airp_arv, "
                "   :seats, "
                "   :subcls, "
                "   :pnr, "
                "   :surname, "
                "   :name, "
                "   :gender, "
                "   :birth_date "
                ") ",
                QParams()
                << QParam("point_id", otInteger, point_id)
                << QParam("pax_id", otInteger)
                << QParam("status", otString)
                << QParam("airp_arv", otString)
                << QParam("seats", otInteger)
                << QParam("subcls", otString)
                << QParam("pnr", otString)
                << QParam("surname", otString)
                << QParam("name", otString)
                << QParam("gender", otString)
                << QParam("birth_date", otDate));
        for(TPFSItems::const_iterator
                target = pfs.items.begin();
                target != pfs.items.end();
                target++) {
            for(TPFSCtgryList::const_iterator
                    category = target->second.begin();
                    category != target->second.end();
                    category++) {
                for(TPFSClsList::const_iterator
                        cls = category->second.begin();
                        cls != category->second.end();
                        cls++) {
                    ostringstream pax_lst;
                    for(TPFSPaxList::const_iterator
                            pax = cls->second.begin();
                            pax != cls->second.end();
                            pax++) {
                        if(
                                category->first == "GOSHO" or
                                category->first == "NOSHO" or
                                category->first == "OFFLK" or
                                category->first == "NOREC"
                          ) {
                            Qry.get().SetVariable("pax_id", pax->pax_id);
                            Qry.get().SetVariable("status", category->first);
                            Qry.get().SetVariable("airp_arv", pax->target);
                            Qry.get().SetVariable("seats", pax->seats);
                            Qry.get().SetVariable("subcls", pax->subcls);
                            Qry.get().SetVariable("pnr", TPnrAddrs().getByPaxId(pax->pax_id));
                            Qry.get().SetVariable("surname", pax->surname);
                            Qry.get().SetVariable("name", pax->name);

                            CheckIn::TPaxDocItem doc;
                            if(not LoadPaxDoc(pax->pax_id, doc))
                                LoadCrsPaxDoc(pax->pax_id, doc);

                            Qry.get().SetVariable("gender", doc.gender);

                            if(doc.birth_date == NoExists)
                                Qry.get().SetVariable("birth_date", FNull);
                            else
                                Qry.get().SetVariable("birth_date", doc.birth_date);

                            Qry.get().Execute();
                        }
                    }
                    /*
                       LogTrace(TRACE5)
                       << "pfs[" << target->first << "]"
                       << "[" << category->first << "]"
                       << "[" << cls->first << "] = "
                       << pax_lst.str();
                       */
                }
            }
        }
    }
}


void get_bag_info(map<string, pair<int, int> > &bag_info, int point_id)
{
    TypeB::TDetailCreateInfo info;
    info.point_id = point_id;
    TBagRems bag_rems;
    bag_rems.get(info);
    bag_rems.ToMap(info, bag_info);
}

void get_pfs_stat(int point_id)
{
    TCachedQuery delQry("delete from pfs_stat where point_id = :point_id",
            QParams() << QParam("point_id", otInteger, point_id));
    delQry.get().Execute();

    TPFSBody pfs;
    TypeB::TCreateInfo createInfo("PFS", TypeB::TCreatePoint());
    TypeB::TDetailCreateInfo info;
    info.create_point = createInfo.create_point;
    info.copy(createInfo);
    info.point_id = point_id;
    pfs.get(info);

    PFS_STAT::toDB(point_id, pfs);
}

void get_kuf_stat(int point_id)
{
    TCachedQuery Qry("select * from points where point_id = :point_id",
            QParams() << QParam("point_id", otInteger, point_id));
    Qry.get().Execute();

    TTripInfo tripInfo(Qry.get());

    if(not KUF_STAT::TAirps().find(tripInfo.airp) or
            not KUF_STAT::TAirlines().find(tripInfo.airline)) return;

    bool close = false;
    bool takeoff = false;

    TTripStage ts;

    TTripStages::LoadStage(point_id, sCloseCheckIn, ts);
    close = ts.act != NoExists;

    TTripStages::LoadStage(point_id, sTakeoff, ts);
    takeoff = ts.act != NoExists;

    if(not close and not takeoff) return;

    if(close)
        KUF_STAT::run(tripInfo, KUF_STAT::TFileType::ftClose);

    if(takeoff) {
        KUF_STAT::run(tripInfo, KUF_STAT::TFileType::ftTakeoff);
        KUF_STAT::run(tripInfo, KUF_STAT::TFileType::ftPax);
    }
}
