#include <map>
#include <set>
#include "telegram.h"
#include "xml_unit.h"
#include "tlg/tlg.h"
#include "astra_utils.h"
#include "stl_utils.h"
#include "convert.h"
#include "salons.h"
#include "salonform.h"
#include "astra_consts.h"
#include "passenger.h"
#include "remarks.h"
#include "pers_weights.h"
#include "misc.h"
#include "qrys.h"
#include "typeb_utils.h"
#include "serverlib/logger.h"

#define NICKNAME "DEN"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

#include "alarms.h"
#include "TypeBHelpMng.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC;
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
  std::set<TTlgCompLayer,TCompareCompLayers> seats;
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

void getSalonPaxsSeats( int point_dep, std::map<int,TCheckinPaxSeats> &checkinPaxsSeats )
{
  checkinPaxsSeats.clear();
  std::set<ASTRA::TCompLayerType> search_layers;
  for ( int ilayer=0; ilayer<(int)cltTypeNum; ilayer++ ) {
    ASTRA::TCompLayerType layer_type = (ASTRA::TCompLayerType)ilayer;
    BASIC_SALONS::TCompLayerType layer_elem;
    if ( BASIC_SALONS::TCompLayerTypes::Instance()->getElem( layer_type, layer_elem ) &&
         layer_elem.getOccupy() ) {
      search_layers.insert( layer_type );
    }
  }

//  if ( SALONS2::isTranzitSalons( point_dep ) ) {     //!!!удалить при установке без новых салонов
    SALONS2::TSalonList salonList;
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_dep, ASTRA::NoExists ),
                          SALONS2::isTranzitSalons( point_dep )?SALONS2::rfTranzitVersion:SALONS2::rfNoTranzitVersion,
                          "" );
    TSalonPassengers passengers;
    SALONS2::TGetPassFlags flags;
    flags.setFlag( SALONS2::gpPassenger ); //только пассажиров с местами
    TSectionInfo sectionInfo;
    salonList.getSectionInfo( sectionInfo, flags );
    TLayersSeats layerSeats;
    sectionInfo.GetLayerSeats( layerSeats );
    std::map<int,TSalonPax> paxs;
    sectionInfo.GetPaxs( paxs );
    for ( TLayersSeats::iterator ilayer=layerSeats.begin();
          ilayer!=layerSeats.end(); ilayer++ ) {
      if ( search_layers.find( ilayer->first.layer_type ) == search_layers.end() ) {
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
      for ( SALONS2::TPassSeats::const_iterator iseat=ilayer->second.begin();
            iseat!=ilayer->second.end(); iseat++ ) {
        TTlgCompLayer compLayer;
        compLayer.pax_id = ilayer->first.getPaxId();
          compLayer.point_dep = ilayer->first.point_dep;
          compLayer.point_arv = ilayer->first.point_arv;
          compLayer.layer_type = ilayer->first.layer_type;
          compLayer.xname = iseat->line;
          compLayer.yname = iseat->row;
        checkinPaxsSeats[ ilayer->first.getPaxId() ].gender = gender;
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
       "       salons.is_waitlist(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,rownum)=0 AND "
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
  for ( vector<TPlaceList*>::iterator isalonList=Salons.placelists.begin(); isalonList!=Salons.placelists.end(); isalonList++) { // пробег по салонам
    for ( IPlace iseat=(*isalonList)->places.begin(); iseat!=(*isalonList)->places.end(); iseat++ ) { // пробег по местам в салоне
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
                     vector<TTlgCompLayer> &complayers,
                     bool pr_blocked )
{
  complayers.clear();
  SALONS2::TSalonList salonList;
  salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, ASTRA::NoExists ),
                        SALONS2::isTranzitSalons( point_id )?SALONS2::rfTranzitVersion:SALONS2::rfNoTranzitVersion,
                        "" );
  SALONS2::TGetPassFlags flags;
  TSectionInfo sectionInfo;
  salonList.getSectionInfo( sectionInfo, flags );
  std::vector<std::pair<TSeatLayer,TPassSeats> > layersSeats;
  TTlgCompLayer comp_layer;
  int next_point_arv = ASTRA::NoExists;

  for ( int ilayer=0; ilayer<(int)cltTypeNum; ilayer++ ) {
    ASTRA::TCompLayerType layer_type = (ASTRA::TCompLayerType)ilayer;
    BASIC_SALONS::TCompLayerType layer_elem;
    if ( (pr_blocked && (layer_type == cltBlockCent || layer_type == cltDisable)) ||
         (!pr_blocked &&
            BASIC_SALONS::TCompLayerTypes::Instance()->getElem( layer_type, layer_elem ) &&
            layer_elem.getOccupy()) ) {
      sectionInfo.GetCurrentLayerSeat( layer_type, layersSeats );
      for ( std::vector<std::pair<TSeatLayer,TPassSeats> >::iterator ilayer=layersSeats.begin();
            ilayer!=layersSeats.end(); ilayer++ ) {
        if ( ilayer->first.point_id != point_id ) {
          continue;
        }
        comp_layer.pax_id = ilayer->first.getPaxId();
        comp_layer.point_dep = ilayer->first.point_dep;
        if ( comp_layer.point_dep == ASTRA::NoExists ) {
          comp_layer.point_dep = ilayer->first.point_id;
        }
        comp_layer.point_arv = ilayer->first.point_arv;
        if ( comp_layer.point_arv == ASTRA::NoExists ) { //до след. пункта
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
        comp_layer.layer_type = ilayer->first.layer_type;
        for ( TPassSeats::iterator iseat=ilayer->second.begin();
              iseat!=ilayer->second.end(); iseat++ ) {
          comp_layer.xname = iseat->line;
          comp_layer.yname = iseat->row;
          complayers.push_back( comp_layer );
        }
      }
    }
  }
  // сортировка по yname, xname
  sort( complayers.begin(), complayers.end(), CompareCompLayers );
}

void getSalonLayers(const TypeB::TDetailCreateInfo &info,
                    vector<TTlgCompLayer> &complayers,
                    bool pr_blocked)
{
  getSalonLayers(info.point_id,
                 info.point_num,
                 info.first_point,
                 info.pr_tranzit,
                 complayers,
                 pr_blocked);
};


//!!!удалить при установке без новых салонов
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
        if ( search_layers.find( ilayer->layer_type ) != search_layers.end() ) { //надо добавить
          comp_layer.pax_id = ilayer->getPaxId();
          comp_layer.point_dep = ilayer->point_dep;
          if ( comp_layer.point_dep == ASTRA::NoExists ) {
            comp_layer.point_dep = ilayer->point_id;
          }
          comp_layer.point_arv = ilayer->point_arv;
          if ( comp_layer.point_arv == ASTRA::NoExists ) { //до след. пункта
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
  // сортировка по yname, xname
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
    if ( SALONS2::isTranzitSalons( point_id ) ) { //!!!удалить при установке без новых салонов
      ReadTranzitSalons( point_id, //!!!удалить при установке без новых салонов - ReadTranzitSalons
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
    for ( vector<TPlaceList*>::iterator ipl=Salons.placelists.begin(); ipl!=Salons.placelists.end(); ipl++ ) { // пробег по салонам
        for ( IPlace ip=(*ipl)->places.begin(); ip!=(*ipl)->places.end(); ip++ ) { // пробег по местам в салоне
            bool pr_break = false;
            for ( vector<ASTRA::TCompLayerType>::iterator ilayer=layers.begin(); ilayer!=layers.end(); ilayer++ ) { // пробег по слоям where pr_occupy<>0
                for ( vector<TPlaceLayer>::iterator il=ip->layers.begin(); il!=ip->layers.end(); il++ ) { // пробег по слоям места
                    if ( il->layer_type == *ilayer ) { // нашли нужный слой
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
                if ( pr_break ) // закончили бежать по слоям места
                    break;
            }
        }
    }
    // сортировка по yname, xname
    sort( complayers.begin(), complayers.end(), CompareCompLayers );
}
*/
struct TTlgDraft {
    private:
        TypeB::TDetailCreateInfo &tlg_info;
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

void TTlgDraft::Commit(TTlgOutPartInfo &tlg_row)
{
    // В процессе создания телеграммы части, содержащие ошибки, могли не
    // попасть в итоговый текст. Поэтому надо синхронизировать список ошибок
    // с текстом телеграммы. Т.е. удалить из списка отсутствующие в тексте ошибки.
    tlg_info.err_lst.fix(parts);
    bool no_errors = tlg_info.err_lst.empty();
    tlg_row.num = 1;
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
        tlg_row.addr = iv->addr;
        tlg_row.origin = iv->origin;
        tlg_row.heading = iv->heading;
        tlg_row.body = iv->body;
        tlg_row.ending = iv->ending;
        TelegramInterface::SaveTlgOutPart(tlg_row, false, false);
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
    int rkWeight;
    void get(int grp_id, int bag_pool_num);
    void ToTlg(vector<string> &body);
    TWItem():
        bagAmount(0),
        bagWeight(0),
        rkWeight(0)
    {};
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

    if(markOptions.mark_info.empty()) { // Нет инфы о маркетинг рейсе
        if(info == m_flight) // если факт. (info) и комм. (m_flight) совпадают, поле не выводим
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
        << info.TlgElemIdToElem(etSubcls, m_flight.subcls)
        << setw(2) << setfill('0') << m_flight.scd_day_local
        << info.TlgElemIdToElem(etAirp, m_flight.airp_dep)
        << info.TlgElemIdToElem(etAirp, m_flight.airp_arv);
    body.push_back(result.str());
}

struct TFTLPax;
struct TETLPax;

struct TName {
    string surname;
    string name;
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body, string postfix = "");
    string ToPILTlg(TypeB::TDetailCreateInfo &info) const;
};

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
        for(vector<TPNRItem>::iterator iv = items.begin(); iv != items.end(); iv++)
            if(info.airline == iv->airline) {
                iv->ToTlg(info, body);
                break;
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
        int reg_no;
        string surname;
        string name;
        int parent_pax_id;
        int temp_parent_id;
        string ticket_no;
        int coupon_no;
        string ticket_rem;
        void dump();
        TInfantsItem() {
            pax_id = NoExists;
            grp_id = NoExists;
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
                "     pax.pers_type='ВЗ' AND ";
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
        public:
            TInfants *infants;
            vector<string> items;
            void get(TypeB::TDetailCreateInfo &info, TFTLPax &pax);
            void get(TypeB::TDetailCreateInfo &info, TETLPax &pax);
            void get(TypeB::TDetailCreateInfo &info, TPRLPax &pax, vector<TTlgCompLayer> &complayers);
            void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
            TRemList(TInfants *ainfants): infants(ainfants) {};
    };

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
        if(bag_pool_num == 1) // непривязанные бирки приобщаем к bag_pool_num = 1
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
        map<int, map<int, TGRPItem> > items;
        int items_count;
        bool find(int grp_id, int bag_pool_num);
        void get(int grp_id, int bag_pool_num);
        void ToTlg(TypeB::TDetailCreateInfo &info, int grp_id, int bag_pool_num, vector<string> &body);
        TGRPMap(): items_count(0) {};
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

    enum TGender {gMale, gFemale, gChild, gInfant};

    struct TPRLPax {
        string target;
        int cls_grp_id;
        TName name;
        int pnr_id;
        string crs;
        int pax_id;
        int grp_id;
        int bag_pool_num;
        string subcls;
        TPNRList pnrs;
        TMItem M;
        TFirmSpaceAvail firm_space_avail;
        TRemList rems;
        TPRLOnwardList OList;
        TGender gender;
        TPerson pers_type;
        TPRLPax(TInfants *ainfants): rems(ainfants) {
            cls_grp_id = NoExists;
            pnr_id = NoExists;
            pax_id = NoExists;
            grp_id = NoExists;
            bag_pool_num = NoExists;
            gender = gMale;
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

    void TRemList::get(TypeB::TDetailCreateInfo &info, TPRLPax &pax, vector<TTlgCompLayer> &complayers )
    {
        items.clear();
        if(pax.pax_id == NoExists) return;
        // rems must be push_backed exactly in this order. Don't swap!
        for(vector<TInfantsItem>::iterator infRow = infants->items.begin(); infRow != infants->items.end(); infRow++) {
            if(infRow->grp_id == pax.grp_id and infRow->parent_pax_id == pax.pax_id) {
                string rem;
                rem = "1INF " + transliter(infRow->surname, 1, info.is_lat());
                if(!infRow->name.empty()) {
                    rem += "/" + transliter(infRow->name, 1, info.is_lat());
                }
                items.push_back(rem);
            }
        }
        if(pax.pers_type == child or pax.pers_type == baby)
            items.push_back("1CHD");
        TTlgSeatList seats;
        seats.add_seats(pax.pax_id, complayers);
        string seat_list = seats.get_seat_list(info.is_lat() or info.pr_lat_seat);
        if(!seat_list.empty())
            items.push_back("SEAT " + seat_list);
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
            TRemCategory cat=getRemCategory(Qry.get().FieldAsString("rem_code"),
                    Qry.get().FieldAsString("rem"));
            if (isDisabledRemCategory(cat)) continue;
            if (cat==remFQT) continue;
            items.push_back(transliter(Qry.get().FieldAsString("rem"), 1, info.is_lat()));
        };

        bool inf_indicator=false; //сюда попадают только люди не infant и ремарки выводим только для этих людей
                                  //для infant пока не выводим, а может быть надо?
        CheckIn::TPaxRemItem rem;
        //билет
        CheckIn::TPaxTknItem tkn;
        LoadPaxTkn(pax.pax_id, tkn);
        if (getPaxRem(info, tkn, inf_indicator, rem)) items.push_back(rem.text);
        //документ
        CheckIn::TPaxDocItem doc;
        LoadPaxDoc(pax.pax_id, doc);
        if (getPaxRem(info, doc, inf_indicator, rem)) items.push_back(rem.text);
        //виза
        CheckIn::TPaxDocoItem doco;
        LoadPaxDoco(pax.pax_id, doco);
        if (getPaxRem(info, doco, inf_indicator, rem)) items.push_back(rem.text);
        //адреса
        list<CheckIn::TPaxDocaItem> doca;
        LoadPaxDoca(pax.pax_id, doca);
        for(list<CheckIn::TPaxDocaItem>::const_iterator d=doca.begin(); d!=doca.end(); ++d)
        {
          if (d->type!="D" && d->type!="R") continue;
          if (getPaxRem(info, *d, inf_indicator, rem)) items.push_back(rem.text);
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
            "    NVL(pax.subclass,pax_grp.class) subclass, "
            "    pax.is_female, "
            "    pax.pers_type "
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
                "   NVL(pax.subclass,pax_grp.class) = :class and ";
        } else {
            SQLText +=
                "    pax_grp.class_grp = cls_grp.id(+) AND "
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
            "    crs_pax.pnr_id = crs_pnr.pnr_id(+) "
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
            int col_is_female = Qry.get().FieldIndex("is_female");
            int col_pers_type = Qry.get().FieldIndex("pers_type");
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
                pax.rems.get(info, pax, complayers); // Обязательно после инициализации pers_type выше
                switch(pax.pers_type) {
                    case NoPerson:
                        break;
                    case adult:
                        pax.gender = (Qry.get().FieldIsNULL(col_is_female) ? gMale : (Qry.get().FieldAsInteger(col_is_female) != 0 ? gFemale : gMale));
                        break;
                    case child:
                        pax.gender = gChild;
                        break;
                    case baby:
                        pax.gender = gInfant;
                        break;
                }
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
        int child;
        int baby;
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
        TCOMStatsItem()
        {
            f = 0;
            c = 0;
            y = 0;
            adult = 0;
            child = 0;
            baby = 0;
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
        void get(TypeB::TDetailCreateInfo &info);
        void ToTlg(TypeB::TDetailCreateInfo &info, ostringstream &body);
    };

    void TCOMStats::ToTlg(TypeB::TDetailCreateInfo &info, ostringstream &body)
    {
        TCOMStatsItem sum;
        sum.target = "TTL";
        for(vector<TCOMStatsItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
            if(info.get_tlg_type() == "COM")
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
        }
        if(info.get_tlg_type() == "COM")
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
                << "0/0/0 0/0 0/0 0/0/0 0 "
                << total_pax_weight.weight << TypeB::endl;
    }

    void TCOMStats::get(TypeB::TDetailCreateInfo &info)
    {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "SELECT "
            "   points.airp target, "
            "   NVL(f, 0) f, "
            "   NVL(c, 0) c, "
            "   NVL(y, 0) y, "
            "   NVL(adult, 0) adult, "
            "   NVL(child, 0) child, "
            "   NVL(baby, 0) baby, "
            "   NVL(f_child, 0) f_child, "
            "   NVL(f_baby, 0) f_baby, "
            "   NVL(c_child, 0) c_child, "
            "   NVL(c_baby, 0) c_baby, "
            "   NVL(y_child, 0) y_child, "
            "   NVL(y_baby, 0) y_baby, "
            "   NVL(bag_amount, 0) bag_amount, "
            "   NVL(bag_weight, 0) bag_weight, "
            "   NVL(rk_weight, 0) rk_weight, "
            "   NVL(f_bag_weight, 0) f_bag_weight, "
            "   NVL(f_rk_weight, 0) f_rk_weight, "
            "   NVL(c_bag_weight, 0) c_bag_weight, "
            "   NVL(c_rk_weight, 0) c_rk_weight, "
            "   NVL(y_bag_weight, 0) y_bag_weight, "
            "   NVL(y_rk_weight, 0) y_rk_weight, "
            "   NVL(f_add_pax, 0) f_add_pax, "
            "   NVL(c_add_pax, 0) c_add_pax, "
            "   NVL(y_add_pax, 0) y_add_pax "
            "FROM "
            "   points, "
            "   ( "
            "SELECT "
            "   pax_grp.point_arv, "
            "   SUM(DECODE(pax_grp.class, 'П', DECODE(seats,0,0,1), 0)) f, "
            "   SUM(DECODE(pax_grp.class, 'Б', DECODE(seats,0,0,1), 0)) c, "
            "   SUM(DECODE(pax_grp.class, 'Э', DECODE(seats,0,0,1), 0)) y, "
            "   SUM(DECODE(pax.pers_type, 'ВЗ', 1, 0)) adult, "
            "   SUM(DECODE(pax.pers_type, 'РБ', 1, 0)) child, "
            "   SUM(DECODE(pax.pers_type, 'РМ', 1, 0)) baby, "
            "   SUM(DECODE(pax_grp.class||pax.pers_type, 'ПРБ', 1, 0)) f_child, "
            "   SUM(DECODE(pax_grp.class||pax.pers_type, 'ПРМ', 1, 0)) f_baby, "
            "   SUM(DECODE(pax_grp.class||pax.pers_type, 'БРБ', 1, 0)) c_child, "
            "   SUM(DECODE(pax_grp.class||pax.pers_type, 'БРМ', 1, 0)) c_baby, "
            "   SUM(DECODE(pax_grp.class||pax.pers_type, 'ЭРБ', 1, 0)) y_child, "
            "   SUM(DECODE(pax_grp.class||pax.pers_type, 'ЭРМ', 1, 0)) y_baby, "
            "   SUM(DECODE(pax_grp.class, 'П', decode(SIGN(1-pax.seats), -1, 1, 0), 0)) f_add_pax, "
            "   SUM(DECODE(pax_grp.class, 'Б', decode(SIGN(1-pax.seats), -1, 1, 0), 0)) c_add_pax, "
            "   SUM(DECODE(pax_grp.class, 'Э', decode(SIGN(1-pax.seats), -1, 1, 0), 0)) y_add_pax "
            "FROM "
            "   pax, pax_grp "
            "WHERE "
            "   pax_grp.point_dep = :point_id AND "
            "   pax_grp.status NOT IN ('E') AND "
            "   pax_grp.grp_id = pax.grp_id AND "
            "   salons.is_waitlist(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,rownum)=0 AND "
            "   pax.refuse IS NULL "
            "GROUP BY "
            "   pax_grp.point_arv "
            "   ) a, "
            "   ( "
            "SELECT "
            "   pax_grp.point_arv, "
            "   SUM(DECODE(bag2.pr_cabin, 0, amount, 0)) bag_amount, "
            "   SUM(DECODE(bag2.pr_cabin, 0, weight, 0)) bag_weight, "
            "   SUM(DECODE(bag2.pr_cabin, 0, 0, weight)) rk_weight, "
            "   SUM(DECODE(pax_grp.class||bag2.pr_cabin, 'П0', weight, 0)) f_bag_weight, "
            "   SUM(DECODE(pax_grp.class||bag2.pr_cabin, 'П1', weight, 0)) f_rk_weight, "
            "   SUM(DECODE(pax_grp.class||bag2.pr_cabin, 'Б0', weight, 0)) c_bag_weight, "
            "   SUM(DECODE(pax_grp.class||bag2.pr_cabin, 'Б1', weight, 0)) c_rk_weight, "
            "   SUM(DECODE(pax_grp.class||bag2.pr_cabin, 'Э0', weight, 0)) y_bag_weight, "
            "   SUM(DECODE(pax_grp.class||bag2.pr_cabin, 'Э1', weight, 0)) y_rk_weight "
            "FROM "
            "   pax_grp, bag2 "
            "WHERE "
            "   pax_grp.point_dep = :point_id AND "
            "   pax_grp.status NOT IN ('E') AND "
            "   pax_grp.grp_id = bag2.grp_id AND "
            "   ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse) = 0 "
            "GROUP BY "
            "   pax_grp.point_arv "
            "   ) b "
            "WHERE "
            "   points.point_id = a.point_arv(+) AND "
            "   points.point_id = b.point_arv(+) AND "
            "   first_point=:first_point AND point_num>:point_num AND pr_del=0 "
            "ORDER BY "
            "   point_num ";
        Qry.CreateVariable("point_id", otInteger, info.point_id);
        Qry.CreateVariable("first_point", otInteger, info.pr_tranzit ? info.first_point : info.point_id);
        Qry.CreateVariable("point_num", otInteger, info.point_num);
        Qry.Execute();
        if(!Qry.Eof) {
            int col_target = Qry.FieldIndex("target");
            int col_f = Qry.FieldIndex("f");
            int col_c = Qry.FieldIndex("c");
            int col_y = Qry.FieldIndex("y");
            int col_adult = Qry.FieldIndex("adult");
            int col_child = Qry.FieldIndex("child");
            int col_baby = Qry.FieldIndex("baby");
            int col_f_child = Qry.FieldIndex("f_child");
            int col_f_baby = Qry.FieldIndex("f_baby");
            int col_c_child = Qry.FieldIndex("c_child");
            int col_c_baby = Qry.FieldIndex("c_baby");
            int col_y_child = Qry.FieldIndex("y_child");
            int col_y_baby = Qry.FieldIndex("y_baby");
            int col_bag_amount = Qry.FieldIndex("bag_amount");
            int col_bag_weight = Qry.FieldIndex("bag_weight");
            int col_rk_weight = Qry.FieldIndex("rk_weight");
            int col_f_bag_weight = Qry.FieldIndex("f_bag_weight");
            int col_f_rk_weight = Qry.FieldIndex("f_rk_weight");
            int col_c_bag_weight = Qry.FieldIndex("c_bag_weight");
            int col_c_rk_weight = Qry.FieldIndex("c_rk_weight");
            int col_y_bag_weight = Qry.FieldIndex("y_bag_weight");
            int col_y_rk_weight = Qry.FieldIndex("y_rk_weight");
            int col_f_add_pax = Qry.FieldIndex("f_add_pax");
            int col_c_add_pax = Qry.FieldIndex("c_add_pax");
            int col_y_add_pax = Qry.FieldIndex("y_add_pax");
            for(; !Qry.Eof; Qry.Next()) {
                TCOMStatsItem item;
                item.target = info.TlgElemIdToElem(etAirp, Qry.FieldAsString(col_target));
                item.f = Qry.FieldAsInteger(col_f);
                item.c = Qry.FieldAsInteger(col_c);
                item.y = Qry.FieldAsInteger(col_y);
                item.adult = Qry.FieldAsInteger(col_adult);
                item.child = Qry.FieldAsInteger(col_child);
                item.baby = Qry.FieldAsInteger(col_baby);
                item.f_child = Qry.FieldAsInteger(col_f_child);
                item.f_baby = Qry.FieldAsInteger(col_f_baby);
                item.c_child = Qry.FieldAsInteger(col_c_child);
                item.c_baby = Qry.FieldAsInteger(col_c_baby);
                item.y_child = Qry.FieldAsInteger(col_y_child);
                item.y_baby = Qry.FieldAsInteger(col_y_baby);
                item.bag_amount = Qry.FieldAsInteger(col_bag_amount);
                item.bag_weight = Qry.FieldAsInteger(col_bag_weight);
                item.rk_weight = Qry.FieldAsInteger(col_rk_weight);
                item.f_bag_weight = Qry.FieldAsInteger(col_f_bag_weight);
                item.f_rk_weight = Qry.FieldAsInteger(col_f_rk_weight);
                item.c_bag_weight = Qry.FieldAsInteger(col_c_bag_weight);
                item.c_rk_weight = Qry.FieldAsInteger(col_c_rk_weight);
                item.y_bag_weight = Qry.FieldAsInteger(col_y_bag_weight);
                item.y_rk_weight = Qry.FieldAsInteger(col_y_rk_weight);
                item.f_add_pax = Qry.FieldAsInteger(col_f_add_pax);
                item.c_add_pax = Qry.FieldAsInteger(col_c_add_pax);
                item.y_add_pax = Qry.FieldAsInteger(col_y_add_pax);
                items.push_back(item);
            }
        }
        total_pax_weight.get(info);
    }

    struct TCOMClassesItem {
        string cls;
        int cfg;
        int av;
        TCOMClassesItem() {
            cfg = NoExists;
            av = NoExists;
        }
    };

    struct TCOMClasses {
        vector<TCOMClassesItem> items;
        void get(TypeB::TDetailCreateInfo &info);
        void ToTlg(TypeB::TDetailCreateInfo &info, ostringstream &body);
    };

    void TCOMClasses::ToTlg(TypeB::TDetailCreateInfo &info, ostringstream &body)
    {
        ostringstream classes, av, padc;
        for(vector<TCOMClassesItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
            classes << iv->cls << iv->cfg;
            av << iv->cls << iv->av;
            padc << iv->cls << '0';
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
            << QParam("class", otString);
        TCachedQuery Qry(
                "SELECT "
                "   SUM(pax.seats) seats "
                "FROM "
                "   pax_grp, "
                "   pax "
                "WHERE "
                "   :point_id = pax_grp.point_dep and "
                "   :class = pax_grp.class and "
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
            items.push_back(item);
        }
    }
}

using namespace PRL_SPACE;

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
        ostringstream body;
        body
            << info.flight_view() << "/"
            << DateTimeToStr(info.scd_utc, "ddmmm", 1) << " " << info.airp_dep_view()
            << "/0 OP/NAM" << TypeB::endl;
        TCOMClasses classes;
        TCOMZones zones;
        TCOMStats stats;
        classes.get(info);
        stats.get(info);
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
        LineOverflow( ):Exception( "ПЕРЕПОЛНЕНИЕ СТРОКИ ТЕЛЕГРАММЫ" ) { };
};

void TWItem::ToTlg(vector<string> &body)
{
    ostringstream buf;
    buf << ".W/K/" << bagAmount << '/' << bagWeight;
    if(rkWeight != 0)
        buf << '/' << rkWeight;
    body.push_back(buf.str());
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
        << QParam("rkWeight", otInteger);
    TCachedQuery Qry(
            "declare "
            "   bag_pool_pax_id pax.pax_id%type; "
            "begin "
            "   bag_pool_pax_id := ckin.get_bag_pool_pax_id(:grp_id, :bag_pool_num); "
            "   :bagAmount := ckin.get_bagAmount2(:grp_id, bag_pool_pax_id, :bag_pool_num); "
            "   :bagWeight := ckin.get_bagWeight2(:grp_id, bag_pool_pax_id, :bag_pool_num); "
            "   :rkWeight := ckin.get_rkWeight2(:grp_id, bag_pool_pax_id, :bag_pool_num); "
            "end;",
            QryParams);
    Qry.get().Execute();
    bagAmount = Qry.get().GetVariableAsInteger("bagAmount");
    bagWeight = Qry.get().GetVariableAsInteger("bagWeight");
    rkWeight = Qry.get().GetVariableAsInteger("rkWeight");
}

struct TBTMGrpList;
// .F - блок информации для данного трансфера. Используется в BTM и PTM
// абстрактный класс
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
        map<string, TPSurname> surnames; // пассажиры сгруппированы по фамилии
        // этот оператор нужен для sort вектора TPSurname
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
        // Конструктор копирования нужен, чтобы PList.grp содержал правильный указатель
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

// Представление списка полей .P/ как он будет в телеграмме.
// причем список этот будет представлять отдельную группу пассажиров
// объединенную по grp_id и bag_pool_num
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
        vector<TPPax> one; // одно место
        vector<TPPax> many_noname; // без имени, больше одного места
        vector<TPPax> many_name; // с именем, больше одного места
        {
            TPSurname &pax_list = im->second;
            // Разложим список пассажиров на 3 подсписка
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
            // обработка one
            // Записываем в строку столько имен, сколько влезет, остальные обрубаем.
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
            // Обработка many_name. Записываем в строку сколько влезет.
            // Потом на след строку. Если и один не влезает, обрезаем имя до одного символа, затем фамилию.
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
                            // Один пассажир на всю строку не поместился
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
                            // Пассажир не влез в строку к другим пассажирам
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
            // Записываем каждого пассажира по отдельности
            // Если не влезает, обрезаем фамилию
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
    // Это был сложный алгоритм объединения имен под одну фамилию и все такое
    // теперь он выродился в список из всего одного пассажира с main_pax_id
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
            if((curLine + *iv).get_line_size() > LINE_SIZE) {// все, строка переполнена
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

    // В полученном векторе строк, обрезаем слишком длинные
    // фамилии, объединяем между собой, если найдутся
    // совпадения обрезанных фамилий.
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

// Список направлений трансфера
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
    Qry.Execute();
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
        // все типы телеграмм кроме PTMN проверяют grp_list_body.
        // для PTMN он всегда пустой.
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
          ) // Нашли новую группу
            break;
        result += j->size() + TypeB::endl.size();
    }
    return result;
}

struct TSSRItem {
    string code;
    string free_text;
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
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   rem_code, "
        "   rem "
        "from "
        "   pax_rem "
        "where "
        "   pax_id = :pax_id "
        "order by "
        "   rem_code ";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.Execute();
    if(!Qry.Eof) {
        int col_rem_code = Qry.FieldIndex("rem_code");
        int col_rem = Qry.FieldIndex("rem");
        for(; !Qry.Eof; Qry.Next()) {
            TSSRItem item;
            item.code = Qry.FieldAsString(col_rem_code);
            if(not ssr_rem_grp.exists(item.code))
                continue;
            item.free_text = Qry.FieldAsString(col_rem);
            if (isDisabledRem(item.code, item.free_text)) continue;
            if(item.code == item.free_text)
                item.free_text.erase();
            else
                item.free_text = item.free_text.substr(item.code.size() + 1);
            TrimString(item.free_text);
            items.push_back(item);
        }
    }
}

struct TClsCmp {
    bool operator() (const string &l, const string &r) const
    {
        TClasses &classes = (TClasses &)base_tables.get("classes");
        int l_prior = ((TClassesRow &)classes.get_row("code", l)).priority;
        int r_prior = ((TClassesRow &)classes.get_row("code", r)).priority;
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
    vector<TTlgCompLayer> complayers;
    if(not isFreeSeating(info.point_id) and not isEmptySalons(info.point_id))
        getSalonLayers( info, complayers, false );
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   pax_id, "
        "   pax.surname, "
        "   pax.name, "
        "   pax_grp.airp_arv, "
        "   pax_grp.class "
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
    vector<TTlgCompLayer> complayers;
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
        "   pax_grp.class "
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
          ) { // Нашли новую группу
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
    // Разделим список мест на список групп мест с одинаковыми горизонтальными интервалами
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
                if(cur != i_split->end() and prev_iata_line(cur->line1) == norm_iata_line(prev->line1)) {
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
    // Разделим список мест на список групп мест с одинаковыми горизонтальными интервалами
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
                if(cur != i_split->end() and prev_iata_row(cur->row1) == norm_iata_row(prev->row1)) {
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
    // Записываем полученные области мест в результат.
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
    // Записываем в result,
    // попутно ищем одиночные места, чтобы
    // попробовать записать их в краткой форме напр. 3ABД
    // Аккумулируем такие места в векторе alone
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
            if(line1 == prev_iata_line(line2))
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
    if(is_iata_row(yname) && is_iata_line(xname)) {
        TTlgPlace place;
        place.xname = norm_iata_line(xname);
        place.yname = norm_iata_row(yname);
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
            result.push_back(denorm_iata_row(ay->first,NULL) + denorm_iata_line(ax->first, pr_lat));
    }
    return result;
}

string  TTlgSeatList::get_seat_one(bool pr_lat) const
{
    string result;
    if(!comp.empty()) {
        t_tlg_comp::const_iterator ay = comp.begin();
        t_tlg_row::const_iterator ax = ay->second.begin();
        result = denorm_iata_row(ay->first,NULL) + denorm_iata_line(ax->first, pr_lat);
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
    // Пробег карты мест по горизонтали
    // определение минимальной и максимальной координаты линии в которых есть
    // занятые места (используются для последующего вертикального пробега)
    string min_col, max_col;
    for(t_tlg_comp::iterator ay = comp.begin(); ay != comp.end(); ay++) {
        map<int, TSeatListContext> ctxt;
        string *first_xname = NULL;
        string *last_xname = NULL;
        TSeatRectList *SeatRectList = NULL;
        TSeatListContext *cur_ctxt = NULL;
        t_tlg_row &row = ay->second;
        if(min_col.empty() or less_iata_line(row.begin()->first, min_col))
            min_col = row.begin()->first;
        if(max_col.empty() or not less_iata_line(row.rbegin()->first, max_col))
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
                if(prev_iata_line(ax->first) == *last_xname)
                    *last_xname = ax->first;
                else {
                    cur_ctxt->seat_to_str(*SeatRectList, ax->second.yname, *first_xname, *last_xname, pr_lat);
                    *first_xname = ax->first;
                    *last_xname = *first_xname;
                }
            }
        }
        // Дописываем последние оставшиеся места в ряду для каждого направления
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

    // Пробег карты мест по вертикали
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
                    if(prev_iata_row(col_pos->second.yname) == *last_xname)
                        *last_xname = col_pos->second.yname;
                    else {
                        cur_ctxt->vert_seat_to_str(*SeatRectList, col_pos->first, *first_xname, *last_xname, pr_lat);
                        *first_xname = col_pos->second.yname;
                        *last_xname = *first_xname;
                    }
                }
            }
        }
        // Дописываем последние оставшиеся места в ряду для каждого направления
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
        i_col = next_iata_line(i_col);
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
    yname = denorm_iata_line(yname, pr_lat);
    first_xname = denorm_iata_row(first_xname,NULL);
    last_xname = denorm_iata_row(last_xname,NULL);
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
    yname = denorm_iata_row(yname,NULL);
    first_xname = denorm_iata_line(first_xname, pr_lat);
    last_xname = denorm_iata_line(last_xname, pr_lat);
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

  vector<TTlgCompLayer> complayers;
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
    /*name = "Ден";
    surname = "тут был";*/
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

void TRemList::get(TypeB::TDetailCreateInfo &info, TETLPax &pax)
{
    CheckIn::TPaxRemItem rem;
    //билет
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

bool getPaxRem(TypeB::TDetailCreateInfo &info, const CheckIn::TPaxTknItem &tkn, bool inf_indicator, CheckIn::TPaxRemItem &rem)
{
  if (tkn.empty() || tkn.rem.empty()) return false;
  rem.clear();
  rem.code=tkn.rem;
  ostringstream text;
  text << rem.code << " HK1 " << (inf_indicator?"INF":"")
       << transliter(convert_char_view(tkn.no, info.is_lat()), 1, info.is_lat());
  if (tkn.coupon!=ASTRA::NoExists)
    text << "/" << tkn.coupon;
  rem.text=text.str();
  rem.calcPriority();
  return true;
};

bool getPaxRem(TypeB::TDetailCreateInfo &info, const CheckIn::TPaxDocItem &doc, bool inf_indicator, CheckIn::TPaxRemItem &rem)
{
  if (doc.empty()) return false;
  rem.clear();
  rem.code="DOCS";
  ostringstream text;
  text << rem.code
       << " " << "HK1"
       << "/" << (doc.type.empty()?"":info.TlgElemIdToElem(etPaxDocType, doc.type))
       << "/" << (doc.issue_country.empty()?"":info.TlgElemIdToElem(etPaxDocCountry, doc.issue_country))
       << "/" << transliter(convert_char_view(doc.no, info.is_lat()), 1, info.is_lat())
       << "/" << (doc.nationality.empty()?"":info.TlgElemIdToElem(etPaxDocCountry, doc.nationality))
       << "/" << (doc.birth_date!=ASTRA::NoExists?DateTimeToStr(doc.birth_date, "ddmmmyy", info.is_lat()):"")
       << "/" << (doc.gender.empty()?"":info.TlgElemIdToElem(etGenderType, doc.gender)) << (inf_indicator?"I":"")
       << "/" << (doc.expiry_date!=ASTRA::NoExists?DateTimeToStr(doc.expiry_date, "ddmmmyy", info.is_lat()):"")
       << "/" << transliter(doc.surname, 1, info.is_lat())
       << "/" << transliter(doc.first_name, 1, info.is_lat())
       << "/" << transliter(doc.second_name, 1, info.is_lat())
       << "/" << (doc.pr_multi?"H":"");
  rem.text=text.str();
  for(int i=rem.text.size()-1;i>=0;i--)
    if (rem.text[i]!='/')
    {
      rem.text.erase(i+1);
      break;
    };
  rem.calcPriority();
  return true;
};

bool getPaxRem(TypeB::TDetailCreateInfo &info, const CheckIn::TPaxDocoItem &doco, bool inf_indicator, CheckIn::TPaxRemItem &rem)
{
  if (doco.empty()) return false;
  rem.clear();
  rem.code="DOCO";
  ostringstream text;
  text << rem.code
       << " " << "HK1"
       << "/" << transliter(doco.birth_place, 1, info.is_lat())
       << "/" << (doco.type.empty()?"":info.TlgElemIdToElem(etPaxDocType, doco.type))
       << "/" << transliter(convert_char_view(doco.no, info.is_lat()), 1, info.is_lat())
       << "/" << transliter(doco.issue_place, 1, info.is_lat())
       << "/" << (doco.issue_date!=ASTRA::NoExists?DateTimeToStr(doco.issue_date, "ddmmmyy", info.is_lat()):"")
       << "/" << (doco.applic_country.empty()?"":info.TlgElemIdToElem(etPaxDocCountry, doco.applic_country))
       << "/" << (inf_indicator?"I":"");
  rem.text=text.str();
  for(int i=rem.text.size()-1;i>=0;i--)
    if (rem.text[i]!='/')
    {
      rem.text.erase(i+1);
      break;
    };
  rem.calcPriority();
  return true;
};

bool getPaxRem(TypeB::TDetailCreateInfo &info, const CheckIn::TPaxDocaItem &doca, bool inf_indicator, CheckIn::TPaxRemItem &rem)
{
  if (doca.empty()) return false;
  rem.clear();
  rem.code="DOCA";
  ostringstream text;
  text << rem.code
       << " " << "HK1"
       << "/" << doca.type
       << "/" << (doca.country.empty()?"":info.TlgElemIdToElem(etPaxDocCountry, doca.country))
       << "/" << transliter(doca.address, 1, info.is_lat())
       << "/" << transliter(doca.city, 1, info.is_lat())
       << "/" << transliter(doca.region, 1, info.is_lat())
       << "/" << transliter(doca.postal_code, 1, info.is_lat())
       << "/" << (inf_indicator?"I":"");
  rem.text=text.str();
  for(int i=rem.text.size()-1;i>=0;i--)
    if (rem.text[i]!='/')
    {
      rem.text.erase(i+1);
      break;
    };
  rem.calcPriority();
  return true;
};

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
    for(vector<TFTLPax>::iterator iv = PaxList.begin(); iv != PaxList.end(); iv++) {
        iv->name.ToTlg(info, body);
        iv->pnrs.ToTlg(info, body);
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
            + "/" + (doc.gender.empty()?"":info.TlgElemIdToElem(etGenderType, doc.gender)) //не клеим inf_indicator, надо будет - подклеим
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
        "    NVL(pax.subclass,pax_grp.class) subclass "
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
        "    pax_grp.class=classes.code AND "
        "    pax_grp.point_dep=:point_id AND "
        "    pax_grp.status NOT IN ('E') AND "
        "    pr_brd IS NOT NULL "
        "ORDER BY "
        "    pax_grp.airp_arv, "
        "    classes.priority, "
        "    NVL(pax.subclass,pax_grp.class), "
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

void TETLDest::GetPaxList(TypeB::TDetailCreateInfo &info,vector<TTlgCompLayer> &complayers)
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
        "    pax_grp.class_grp = cls_grp.id(+) AND "
        "    cls_grp.code = :class and "
        "    pax.pr_brd = 1 and "
        "    pax.seats>0 and "
        "    pax.pax_id = crs_pax.pax_id(+) and "
        "    crs_pax.pr_del(+)=0 and "
        "    crs_pax.pnr_id = crs_pnr.pnr_id(+) and "
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
    TGRPMap grp_map; // PRL, ETL
    TInfants infants; // PRL
    vector<T> items;
    void get_subcls_lst(TypeB::TDetailCreateInfo &info, list<string> &lst);
    void get(TypeB::TDetailCreateInfo &info,vector<TTlgCompLayer> &complayers);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

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
                << "00" << info.TlgElemIdToElem(etSubcls, iv->cls, prLatToElemFmt(efmtCodeNative,true)); //всегда на латинице - так надо
            body.push_back(line.str());
        } else {
            const TypeB::TPRLOptions *PRLOptions=NULL;
            if(info.optionsIs<TypeB::TPRLOptions>())
                PRLOptions=info.optionsAs<TypeB::TPRLOptions>();
            pr_empty = false;
            line.str("");
            line
                << "-" << info.TlgElemIdToElem(etAirp, iv->airp)
                << setw(2) << setfill('0') << iv->PaxList.size();
            if(PRLOptions and PRLOptions->rbd)
                line << info.TlgElemIdToElem(etSubcls, iv->cls, prLatToElemFmt(efmtCodeNative,true)); //всегда на латинице - так надо
            else
                line << info.TlgElemIdToElem(etClsGrp, iv->PaxList[0].cls_grp_id, prLatToElemFmt(efmtCodeNative,true)); //всегда на латинице - так надо
            body.push_back(line.str());
            iv->PaxListToTlg(info, body);
        }
    }

    if(pr_empty) {
        body.clear();
        body.push_back("NIL");
    }
}

struct TLDMBag {
    int baggage, cargo, mail;
    void get(TypeB::TDetailCreateInfo &info, int point_arv);
    TLDMBag():
        baggage(0),
        cargo(0),
        mail(0)
    {};
};

struct TToRampBag {
    int amount, weight;
    TToRampBag():
        amount(0),
        weight(0)
    {}
    void get(int point_id);
    bool empty();
};

bool TToRampBag::empty()
{
    return not amount and not weight;
}

void TToRampBag::get(int point_id)
{
    amount = 0;
    weight = 0;
    QParams QryParams;
    QryParams << QParam("point_id", otInteger, point_id);
    TCachedQuery Qry(
            "select "
            "   sum(amount) amount, "
            "   sum(weight) weight "
            "from "
            "   pax_grp, "
            "   bag2 "
            "where "
            "   pax_grp.grp_id = bag2.grp_id and "
            "   pax_grp.point_dep = :point_id and "
            "   pax_grp.status NOT IN ('E') AND "
            "   bag2.pr_cabin=0 AND "
            "   ckin.bag_pool_boarded(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)<>0 and "
            "   bag2.to_ramp <> 0 ",
            QryParams
            );
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        amount = Qry.get().FieldAsInteger("amount");
        weight = Qry.get().FieldAsInteger("weight");
    }
}

void TLDMBag::get(TypeB::TDetailCreateInfo &info, int point_arv)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT NVL(SUM(weight),0) AS weight "
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
    baggage = Qry.FieldAsInteger("weight");
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
    int excess;
    void get(int point_id, string airp_arv = "");
    TExcess(): excess(NoExists) {};
};

void TExcess::get(int point_id, string airp_arv)
{
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT NVL(SUM(excess),0) excess FROM pax_grp "
        "WHERE "
        "   point_dep=:point_id AND "
        "   pax_grp.status NOT IN ('E') AND "
        "   ckin.excess_boarded(grp_id,class,bag_refuse)<>0 ";
    if(not airp_arv.empty()) {
        SQLText += " and airp_arv = :airp_arv ";
        Qry.CreateVariable("airp_arv", otString, airp_arv);
    }
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    excess = Qry.FieldAsInteger("excess");
}

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
            if(iv->cls == "П") pr_f = true;
            if(iv->cls == "Б") pr_c = true;
            if(iv->cls == "Э") pr_y = true;
        }
        if (info.bort.empty() ||
                cfg.str().empty() ||
                (crew.cockpit==NoExists and
                 crew.cabin==NoExists))
            vcompleted = false;

        // если оба NoExists, то вопросики, иначе заменяем нулями NoExist'ы
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
    TExcess excess;
    TToRampBag to_ramp;
    vector<TLDMDest> items;
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(TypeB::TDetailCreateInfo &info, bool &vcompleted, vector<string> &body);
};

void TLDMDests::ToTlg(TypeB::TDetailCreateInfo &info, bool &vcompleted, vector<string> &body)
{
    cfg.ToTlg(info, vcompleted, body);
    int baggage_sum = 0;
    int cargo_sum = 0;
    int mail_sum = 0;
    ostringstream row;
    //проверим LDM автоматически отправляется или нет?
    TypeB::TSendInfo sendInfo(info);
    bool pr_send=sendInfo.isSend();
    const TypeB::TLDMOptions &options = *info.optionsAs<TypeB::TLDMOptions>();

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
            row << ".?/?";   //распределение по багажникам
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
        if(options.version == "CEK" and info.airp_dep == "ЧЛБ")
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
            body.push_back(row.str());
        }
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
                << " E " << iv->excess.excess;
            body.push_back(buf.str());
        }
    }
    if(options.version == "CEK") {
        row.str("");
        row << "SI: EXB" << excess.excess << KG;
        body.push_back(row.str());
    }
    if(options.version == "CEK" and info.airp_dep != "ЧЛБ") {
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
        if(to_ramp.empty())
            row << "NIL";
        else
            row << to_ramp.amount << "/" << to_ramp.weight << KG;
        body.push_back(row.str());
    }
    //    body.push_back("SI: TRANSFER BAG CPT 0 NS 0");
}

void TLDMDests::get(TypeB::TDetailCreateInfo &info)
{
    cfg.get(info);
    excess.get(info.point_id);
    to_ramp.get(info.point_id);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT points.point_id AS point_arv, "
        "       points.airp AS target, "
        "       NVL(pax.rk_weight,0) AS rk_weight, "
        "       NVL(pax.f,0) AS f, "
        "       NVL(pax.c,0) AS c, "
        "       NVL(pax.y,0) AS y, "
        "       NVL(pax.adl,0) AS adl, "
        "       NVL(pax.chd,0) AS chd, "
        "       NVL(pax.inf,0) AS inf, "
        "       NVL(pax.female,0) AS female, "
        "       NVL(pax.male,0) AS male "
        "FROM points, "
        "     (SELECT point_arv, "
        "             SUM(ckin.get_rkWeight2(pax_grp.grp_id, pax.pax_id, pax.bag_pool_num, rownum)) rk_weight, "
        "             SUM(DECODE(class,'П',DECODE(seats,0,0,1),0)) AS f, "
        "             SUM(DECODE(class,'Б',DECODE(seats,0,0,1),0)) AS c, "
        "             SUM(DECODE(class,'Э',DECODE(seats,0,0,1),0)) AS y, "
        "             SUM(DECODE(pers_type,'ВЗ',1,0)) AS adl, "
        "             SUM(DECODE(pers_type,'РБ',1,0)) AS chd, "
        "             SUM(DECODE(pers_type,'РМ',1,0)) AS inf, "
        "             sum(decode(pers_type, 'ВЗ', decode(is_female, null, 0, 0, 0, 1), 0)) female, "
        "             sum(decode(pers_type, 'ВЗ', decode(is_female, null, 1, 0, 1, 0), 0)) male "
        "      FROM pax_grp,pax "
        "      WHERE pax_grp.grp_id=pax.grp_id AND "
        "            point_dep=:point_id AND "
        "            pax_grp.status NOT IN ('E') AND "
        "            pr_brd=1 "
        "      GROUP BY point_arv) pax "
        "WHERE points.point_id=pax.point_arv(+) AND "
        "      first_point=:first_point AND point_num>:point_num AND pr_del=0 "
        "ORDER BY points.point_num ";
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.CreateVariable("point_num", otInteger, info.point_num);
    Qry.CreateVariable("first_point", otInteger, info.pr_tranzit ? info.first_point : info.point_id);
    Qry.Execute();
    if(!Qry.Eof) {
        int col_point_arv = Qry.FieldIndex("point_arv");
        int col_target = Qry.FieldIndex("target");
        int col_rk_weight = Qry.FieldIndex("rk_weight");
        int col_f = Qry.FieldIndex("f");
        int col_c = Qry.FieldIndex("c");
        int col_y = Qry.FieldIndex("y");
        int col_adl = Qry.FieldIndex("adl");
        int col_chd = Qry.FieldIndex("chd");
        int col_inf = Qry.FieldIndex("inf");
        int col_female = Qry.FieldIndex("female");
        int col_male = Qry.FieldIndex("male");
        for(; !Qry.Eof; Qry.Next()) {
            TLDMDest item;
            item.point_arv = Qry.FieldAsInteger(col_point_arv);
            item.bag.get(info, item.point_arv);
            item.rk_weight = Qry.FieldAsInteger(col_rk_weight);
            item.f = Qry.FieldAsInteger(col_f);
            item.target = Qry.FieldAsString(col_target);
            item.excess.get(info.point_id, item.target);
            item.c = Qry.FieldAsInteger(col_c);
            item.y = Qry.FieldAsInteger(col_y);
            item.adl = Qry.FieldAsInteger(col_adl);
            item.chd = Qry.FieldAsInteger(col_chd);
            item.inf = Qry.FieldAsInteger(col_inf);
            item.female = Qry.FieldAsInteger(col_female);
            item.male = Qry.FieldAsInteger(col_male);
            items.push_back(item);
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
    BASIC::DecodeTime( remain, hours, mins, secs );
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
        BASIC::DecodeTime( remain, hours, mins, secs );
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
            for(int i = 0; i < 2; i++, iv++); // Установить итератор на 3-й элемент
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
        // В MVTC выводятся только коды задержек, поэтому проверять формат интервала не нужно
        if(not check_delay_code(adelay_code) or (not pr_MVTC and not check_delay_value(adelay_value - info.scd_utc)))
            err_idx = idx;
    }
    if(err_idx != NoExists) { // есть ошибочные задержки
        if(err_idx == (int)size() - 1) { // последняя задержка с ошибкой
            erase(begin(), begin() + err_idx); // оставляем только ее
        } else
            erase(begin(), begin() + err_idx + 1); // выкидываем все задержки до последней ошибочной включительно
    }
    if(size() > 4)
        erase(begin(), (begin() + size() - 4)); // оставляем 4 последние задержки
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

void TMVTABody::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    ostringstream buf;
    if(act != NoExists) {
        int year, month, day1, day2;
        string fmt;
        DecodeDate(act, year, month, day1);
        DecodeDate(AD, year, month, day2);
        if(day1 != day2)
            fmt = "ddhhnn";
        else
            fmt = "hhnn";
        buf
            << "AD"
            << DateTimeToStr(AD, fmt)
            << "/"
            << DateTimeToStr(act, fmt);
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
    TWA(): payload(NoExists), underload(NoExists) {};
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
        if(options.seat_restrict.find('S') != string::npos) {
            SALONS2::TSalonList salonList;
            salonList.ReadFlight( SALONS2::TFilterRoutesSets( info.point_id, ASTRA::NoExists ), SALONS2::rfTranzitVersion, "" );
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
            string seat =
                denorm_iata_row(i_seat->row, NULL) + // denorm - чтобы избавиться от нулей: 002 -> 2
                i_seat->line + "/";
            if(buf.size() + seat.size() > LINE_SIZE) {
                body.push_back(buf);
                buf = PREFIX;
            }
            if(buf == PREFIX)
                buf += ".";
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
    pwr.weight("П", string(), cpw);
    result
        << "WM.S.P.CG." << info.TlgElemIdToElem(etClass, "П")
        << cpw.male << "/" << cpw.female << "/" << cpw.child << "/" << cpw.infant;
    pwr.weight("Б", string(), cpw);
    result
        << "." << info.TlgElemIdToElem(etClass, "Б")
        << cpw.male << "/" << cpw.female << "/" << cpw.child << "/" << cpw.infant;
    pwr.weight("Э", string(), cpw);
    result
        << "." << info.TlgElemIdToElem(etClass, "Э")
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
};

struct TLCITotals {
    size_t pax_size, bag_amount, bag_weight;
    TLCITotals():
        pax_size(0),
        bag_amount(0),
        bag_weight(0)
    {};
};

typedef map<string, TLCITotals> TPaxTotalsItem;

typedef map<string, TByGender> TByClass;

struct TLCIPaxTotalsItem {
    string airp;
    TPaxTotalsItem cls_totals;
    int rk_weight;
    TLCIPaxTotalsItem(): rk_weight(0) {}
};

struct TLCIPaxTotals {
    vector<TLCIPaxTotalsItem> items;
    TByClass pax_tot_by_cls;
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

void TLCIPaxTotals::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    ostringstream result;
    const TypeB::TLCIOptions &options = *info.optionsAs<TypeB::TLCIOptions>();
    for(vector<TLCIPaxTotalsItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
        if(options.pas_totals) {
            result.str(string());
            result
                << "-" << info.TlgElemIdToElem(etAirp, iv->airp) << ".PT."
                <<
                iv->cls_totals["П"].pax_size +
                iv->cls_totals["Б"].pax_size +
                iv->cls_totals["Э"].pax_size
                << ".C."
                << iv->cls_totals["П"].pax_size << "/"
                << iv->cls_totals["Б"].pax_size << "/"
                << iv->cls_totals["Э"].pax_size;
            body.push_back(result.str());
        }
        if(options.bag_totals) {
            result.str(string());
            result
                << "-" << info.TlgElemIdToElem(etAirp, iv->airp) << ".BT."
                <<
                iv->cls_totals["П"].bag_amount +
                iv->cls_totals["Б"].bag_amount +
                iv->cls_totals["Э"].bag_amount
                << "/" <<
                iv->cls_totals["П"].bag_weight +
                iv->cls_totals["Б"].bag_weight +
                iv->cls_totals["Э"].bag_weight
                << ".C."
                << iv->cls_totals["П"].bag_amount << "/"
                << iv->cls_totals["Б"].bag_amount << "/"
                << iv->cls_totals["Э"].bag_amount
                << ".A."
                << iv->cls_totals["П"].bag_weight << "/"
                << iv->cls_totals["Б"].bag_weight << "/"
                << iv->cls_totals["Э"].bag_weight
                << "." << KG;
            body.push_back(result.str());
            result.str(string());
            result
                << "-" << info.TlgElemIdToElem(etAirp, iv->airp) << ".H."
                << iv->rk_weight
                << "." << KG;
            body.push_back(result.str());
        }
    }
    if(options.pas_distrib) {
        result.str(string());
        result << "PD.C." << info.TlgElemIdToElem(etClass, "П") << "."
            << pax_tot_by_cls["П"].m << "/"
            << pax_tot_by_cls["П"].f << "/"
            << pax_tot_by_cls["П"].c << "/"
            << pax_tot_by_cls["П"].i << "."
            << info.TlgElemIdToElem(etClass, "Б") << "."
            << pax_tot_by_cls["Б"].m << "/"
            << pax_tot_by_cls["Б"].f << "/"
            << pax_tot_by_cls["Б"].c << "/"
            << pax_tot_by_cls["Б"].i << "."
            << info.TlgElemIdToElem(etClass, "Э") << "."
            << pax_tot_by_cls["Э"].m << "/"
            << pax_tot_by_cls["Э"].f << "/"
            << pax_tot_by_cls["Э"].c << "/"
            << pax_tot_by_cls["Э"].i;
        body.push_back(result.str());
    }
}

void TLCIPaxTotals::get(TypeB::TDetailCreateInfo &info)
{
    // complayers нужен внутри PRL для вытаскивания номера места для ремарки SEAT
    // в данном случае, нужны только totals, поэтому передаем пустой вектор
    vector<TTlgCompLayer> complayers;
    TDestList<TPRLDest> dests;
    dests.get(info,complayers);
    for(vector<TPRLDest>::iterator iv = dests.items.begin(); iv != dests.items.end(); iv++) {
        size_t idx = 0;
        for(; idx < items.size(); idx++)
            if(items[idx].airp == iv->airp) break;
        if(idx == items.size()) {
            items.push_back(TLCIPaxTotalsItem());
            items[idx].airp = iv->airp;
        }
        for(vector<TPRLPax>::iterator pax_i = iv->PaxList.begin(); pax_i != iv->PaxList.end(); pax_i++) {
            TGRPItem &grp_map = iv->grp_map->items[pax_i->grp_id][pax_i->bag_pool_num];
            items[idx].cls_totals[iv->cls].bag_amount += grp_map.W.bagAmount;
            items[idx].cls_totals[iv->cls].bag_weight += grp_map.W.bagWeight;
            items[idx].rk_weight += grp_map.W.rkWeight;
            switch(pax_i->gender) {
                case gMale:
                    pax_tot_by_cls[iv->cls].m++;
                    break;
                case gFemale:
                    pax_tot_by_cls[iv->cls].f++;
                    break;
                case gChild:
                    pax_tot_by_cls[iv->cls].c++;
                    break;
                case gInfant:
                    pax_tot_by_cls[iv->cls].i++;
                    break;
            }
        }

        items[idx].cls_totals[iv->cls].pax_size = iv->PaxList.size();
    }

}

struct TSeatPlan {
    map<int,TCheckinPaxSeats> checkinPaxsSeats;
    void get(TypeB::TDetailCreateInfo &info);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
};

void TSeatPlan::get(TypeB::TDetailCreateInfo &info)
{
    const TypeB::TLCIOptions &options = *info.optionsAs<TypeB::TLCIOptions>();
    if(options.seat_plan) {
        if(isFreeSeating(info.point_id))
            throw UserException("MSG.SALONS.FREE_SEATING");
        if(isEmptySalons(info.point_id))
            throw UserException("MSG.FLIGHT_WO_CRAFT_CONFIGURE");
        getSalonPaxsSeats(info.point_id, checkinPaxsSeats);
    }
}

void TSeatPlan::ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body)
{
    string buf = "SP";
    for(map<int,TCheckinPaxSeats>::iterator im = checkinPaxsSeats.begin(); im != checkinPaxsSeats.end(); im++) {
        for(set<TTlgCompLayer,TCompareCompLayers>::iterator is = im->second.seats.begin(); is != im->second.seats.end(); is++) {
            string seat =
                "." + denorm_iata_row(is->yname, NULL) +
                denorm_iata_line(is->xname, info.is_lat() or info.pr_lat_seat) +
                "/" + im->second.gender;
            if(buf.size() + seat.size() > LINE_SIZE) {
                body.push_back(buf);
                buf = "SP";
            }
            buf += seat;
        }
    }
    if(buf != "SP")
        body.push_back(buf);
}

struct TLCI {
    TLCICFG eqt;
    TWA wa;
    TSR_C sr_c;
    TSR_Z sr_z;
    TSR_S sr_s;
    TWM wm; // weight mode
    TLCIPaxTotals pax_totals;
    TSeatPlan sp;
    string get_action_code(const TypeB::TCreatePoint &cp);
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
    sr_c.get(info);
    sr_z.get(info);
    sr_s.get(info);
    pax_totals.get(info);
    sp.get(info);
}

string TLCI::get_action_code(const TypeB::TCreatePoint &cp)
{
    string result;
    if(cp.time_offset == 0) {
        switch(cp.stage_id) {
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
    body.push_back("C" + get_action_code(info.create_point));
    eqt.ToTlg(info, body);
    wa.ToTlg(info, body);
    if(options.seating) body.push_back("SM.S"); // Seating method 'By Seat' always
    if(options.seat_restrict.find('C') != string::npos) sr_c.ToTlg(info, body);
    if(options.seat_restrict.find('Z') != string::npos) sr_z.ToTlg(info, body);
    if(options.seat_restrict.find('S') != string::npos) sr_s.ToTlg(info, body);
    if(options.weight_mode) wm.ToTlg(info, body);
    pax_totals.ToTlg(info, body);
    sp.ToTlg(info, body);
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
    vector<string> body;
    try {
        TLCI lci;
        lci.get(info);
        lci.ToTlg(info, body);
    } catch(...) {
        ExceptionFilter(body, info);
    }
    for(vector<string>::iterator iv = body.begin(); iv != body.end(); iv++)
        tlg_row.body += *iv + TypeB::endl;
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

int PNL(TypeB::TDetailCreateInfo &info)
{
  TypeB::TPNLADLOptions *forwarderOptions=NULL;
  if (info.optionsIs<TypeB::TPNLADLOptions>())
    forwarderOptions=info.optionsAs<TypeB::TPNLADLOptions>();
  if (forwarderOptions==NULL) throw Exception("%s: forwarderOptions expected", __FUNCTION__);

  if (forwarderOptions->typeb_in_id==NoExists ||
      forwarderOptions->typeb_in_num==NoExists) throw Exception("%s: forwarderOptions not defined", __FUNCTION__);

  TCachedQuery Qry("SELECT heading, body, ending FROM tlgs_in WHERE id=:tlg_id AND num=:tlg_num",
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
                                forwarderOptions->typeb_in_num,
                                Qry.get());
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

    set<TSubclsItem> subcls_set;

    if(PRLOptions and PRLOptions->rbd) {
        QParams QryParams;
        QryParams << QParam("point_id", otInteger, info.point_id);
        TCachedQuery Qry(
                "select distinct nvl(pax.subclass, pax_grp.class) subcls from pax, pax_grp "
                "where pax_grp.point_dep = :point_id and pax_grp.grp_id = pax.grp_id",
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
        // достает все возможные классы и подклассы, использующиеся на рейсе
        TCFG cfg(info.point_id);
        if(cfg.empty()) cfg.get(NoExists);

        for(TCFG::iterator i = cfg.begin(); i != cfg.end(); i++)
            subcls_set.insert(TSubclsItem(i->priority, i->cls));

        QParams QryParams;
        QryParams << QParam("point_id", otInteger, info.point_id);
        TCachedQuery Qry(
                "SELECT DISTINCT cls_grp.priority, cls_grp.code AS class "
                "FROM pax_grp,cls_grp "
                "WHERE pax_grp.class_grp=cls_grp.id AND "
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

    // Здесь имеем список пар priority, cls
    // Уберем дубликаты и заполним ответ
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
        case 'П':
            f += seats;
                break;
        case 'Б':
            c += seats;
                break;
        case 'Э':
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
                "    crs_pnr.pnr_id = crs_pax.pnr_id ";
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

struct TCKINPaxInfo {
    private:
        TQuery Qry;
        int col_pax_id;
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
        void dump();
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
        TCKINPaxInfo():
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
            status(psCheckin)
        {
            Qry.SQLText =
                "select "
                "    pax.pax_id, "
                "    pax.surname, "
                "    pax.name, "
                "    pax.seats, "
                "    pax.pers_type, "
                "    pax_grp.class cls, "
                "    nvl(pax.subclass, pax_grp.class) subclass, "
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
    TPFSPax(): pax_id(NoExists), pnr_id(NoExists) {};
    TPFSPax(const TCKINPaxInfo &ckin_pax);
    void ToTlg(TypeB::TDetailCreateInfo &info, vector<string> &body);
    void operator = (const TCKINPaxInfo &ckin_pax);
};

TPFSPax::TPFSPax(const TCKINPaxInfo &ckin_pax): pax_id(NoExists)
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
}

struct TSubClsCmp {
    bool operator() (const string &l, const string &r) const
    {
        TSubcls &subcls = (TSubcls &)base_tables.get("subcls");
        string l_cls = ((TSubclsRow&)subcls.get_row("code", l)).cl;
        string r_cls = ((TSubclsRow&)subcls.get_row("code", r)).cl;
        TClasses &classes = (TClasses &)base_tables.get("classes");
        int l_prior = ((TClassesRow &)classes.get_row("code", l_cls)).priority;
        int r_prior = ((TClassesRow &)classes.get_row("code", r_cls)).priority;
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


bool fqt_compare(int pax_id)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "(select airline, no, extra, rem_code from pax_fqt where pax_id = :pax_id "
        "minus "
        "select airline, no, extra, rem_code from crs_pax_fqt where pax_id = :pax_id) "
        "union "
        "(select airline, no, extra, rem_code from crs_pax_fqt where pax_id = :pax_id "
        "minus "
        "select airline, no, extra, rem_code from pax_fqt where pax_id = :pax_id) ";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.Execute();
    return Qry.Eof;
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

    TPFSInfo PFSInfo;
    PFSInfo.get(info.point_id);
    TCKINPaxInfo ckin_pax;
    for(map<int, TPFSInfoItem>::iterator im = PFSInfo.items.begin(); im != PFSInfo.items.end(); im++) {
        string category;
        const TPFSInfoItem &item = im->second;
        ckin_pax.get(item);
        if(item.pnl_pax_id != NoExists) { // Пассажир присутствует в PNL/ADL рейса
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
            } else if(item.pax_id == NoExists) { // Не зарегистрирован на данный рейс
                if(item.pnl_point_id != NoExists and item.pnl_point_id != info.point_id) // зарегистрирован на другой рейс
                    category = "CHGFL";
                else
                    category = "NOSHO";
            } else { // Зарегистрирован
                if(ckin_pax.pr_brd != 0) { // Прошел посадку
                    if(not fqt_compare(item.pax_id))
                        category = "FQTVN";
                    else if(ckin_pax.subclass != ckin_pax.crs_pax.subclass)
                        category = "INVOL";
                    else if(ckin_pax.target != ckin_pax.crs_pax.target)
                        category = "CHGSG";
                    else if(not ckin_pax.PAXLST_cmp())
                        category = "PXLST";
                } else { // Не прошел посадку
                    if(ckin_pax.OK_status())
                        category = "OFFLK";
                }
            }
        } else { // Пассажир НЕ присутствует в PNL/ADL рейса
            if(item.pax_id != NoExists) { // Зарегистрирован
                if(ckin_pax.OK_status()) { // Пассажир имеет статус "бронь" или "сквозная регистрация"
                    if(ckin_pax.pr_brd != 0)
                        category = "NOREC";
                    else
                        category = "OFFLN";
                } else { // Пассажир имеет статус "подсадка"
                    if(ckin_pax.pr_brd != 0)
                        category = "GOSHO";
                    else
                        category = "GOSHN";
                }
            }
        }
        ProgTrace(TRACE5, "category: %s", category.c_str());

        TPFSPax PFSPax = ckin_pax; // PFSPax.M inits within assignment
        if(not markOptions.crs.empty() and markOptions.crs != PFSPax.crs)
            continue;
        if(not markOptions.mark_info.empty() and not(PFSPax.M.m_flight == markOptions.mark_info))
            continue;
        if(item.pax_id != NoExists) // для зарегистрированных пассажиров собираем инфу для цифровой PFS
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

        // 1. шаг - определить crs_rbd.point_id (который есть tlg_trips.point_id)
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
        // 2. шаг. Определить crs_rbd.sender
        string crs_rbd_sender;
        if(markOptions and not markOptions->mark_info.empty())
            crs_rbd_sender = markOptions->crs;

        // 3. шаг. Выбор наиболее подходящих point_id и sender из CRS_RBD
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

            // Здесь определены оба параметра (point_id и sender из CRS_RBD)
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
        vector<TTlgCompLayer> complayers;
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
      const TTypeBTypesRow& row = (TTypeBTypesRow&)(base_tables.get("typeb_types").get_row("code",createInfo.get_tlg_type()));
      tlgTypeInfo=row;
    }
    catch(EBaseTableError)
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

    if (info.optionsIs<TypeB::TAirpTrferOptions>())
    {
      const TypeB::TAirpTrferOptions &options=*(info.optionsAs<TypeB::TAirpTrferOptions>());
      if (options.airp_trfer.empty())
        throw AstraLocale::UserException("MSG.AIRP.DST_UNSPECIFIED");
    };

    if(info.point_id != NoExists)
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
            info.scd_utc = Qry.get().FieldAsDateTime("scd_out");
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
    else
    {
      //непривязанная к рейсу телеграмма
      if (tlgTypeInfo.pr_dep!=NoExists)
        throw Exception("TelegramInterface::create_tlg: point_id not defined (tlg_type=%s)", info.get_tlg_type().c_str());
    };

    //вычисление отправителя
    //Если телеграмма создается в ответ на входную (typeb_in_id != NoExists),
    //то оригинатор вычисляем иначе, чем при герерации автономной тлг.
    if(typeb_in_id != NoExists)
        info.originator = TypeB::getOriginator(TypeBHelpMng::getOriginatorId(typeb_in_id));
    else {
        string orig_airline=info.airline;
        /* возможно понадобится в будущем
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
    else if(tlgTypeInfo.basic_type == "FTL") vid = FTL(info);
    else if(tlgTypeInfo.basic_type == "COM") vid = COM(info);
    else if(tlgTypeInfo.basic_type == "SOM") vid = SOM(info);
    else if(tlgTypeInfo.basic_type == "PIM") vid = PIM(info);
    else if(tlgTypeInfo.basic_type == "LCI") vid = LCI(info);
    else if(tlgTypeInfo.basic_type == "PNL") vid = PNL(info);
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
    }

    if (tlg_id == NoExists) throw Exception("TelegramInterface::CreateTlg: create_tlg without result");

    TReqInfo::Instance()->LocaleToLog("EVT.TLG.CREATED", LEvntPrms()
                                      << PrmElem<std::string>("name", etTypeBType, createInfo.get_tlg_type(), efmtNameShort)
                                      << PrmSmpl<int>("id", tlg_id) << PrmBool("lat", createInfo.get_options().is_lat),
                                      evtTlg, createInfo.point_id, tlg_id);
    NewTextChild( resNode, "tlg_id", tlg_id);
};

void TelegramInterface::kick(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int tlg_id =  NodeAsInteger("content", reqNode);
    string res;
    if(tlg_id == ASTRA::NoExists)
        res = INTERNAL_SERVER_ERROR;
    else {
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
        if(has_errors)
            res = INTERNAL_SERVER_ERROR;
        else {
            res = heading + res + ending;
            markTlgAsSent(tlg_id);
        }
    }
    NewTextChild(resNode, "content", res);
}

void TelegramInterface::tlg_srv(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ProgTrace(TRACE5, "%s", __FUNCTION__);
    xmlNodePtr contentNode = GetNode( "content", reqNode );
    if ( contentNode == NULL ) {
        return;
    }
    string content = NodeAsString( contentNode );
    TrimString(content);
    TypeB::TOriginatorInfo orig = TypeB::getOriginator(
            string(),
            string(),
            string(),
            NowUTC(),
            true
            );
    string sender = "0" + ctxt->GetPult();
    int tlgs_id = loadTlg( orig.addr + "\xa." + sender + "\n" + content);
    if(content.substr(0, 4) == "LCI\xa") { // Для LCI подвешиваем процесс, для остальных - возвр. пустой ответ.
        TypeBHelpMng::configForPerespros(tlgs_id, orig.id);
        NewTextChild(resNode, "content", TIMEOUT_OCCURRED);
    }
}

void ccccccccccccccccccccc( int point_dep,  const ASTRA::TCompLayerType &layer_type )
{
  //try verify its new code!!!
  SALONS2::TSalonList salonList;
  salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_dep, ASTRA::NoExists ), SALONS2::rfTranzitVersion, "" );
  SALONS2::TSectionInfo sectionInfo;
  SALONS2::TGetPassFlags flags;
  flags.clearFlags();
  salonList.getSectionInfo( sectionInfo, flags );
  TPassSeats layerSeats;
  sectionInfo.GetTotalLayerSeat( layer_type, layerSeats );
};

