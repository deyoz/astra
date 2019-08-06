#include "web_craft.h"
#include "astra_locale_adv.h"

#define NICKNAME "DJEK"
#include "serverlib/slogger.h"

using namespace std;
using namespace AstraLocale;
using namespace ASTRA;
using namespace SALONS2;
using namespace SEATS2;

namespace AstraWeb
{
namespace WebCraft {
  const int FIRST_VERSION               =  0;
  const int ALL_CLASS_CRAFT_VERSION     =  1;
  const int LAST_VERSION                =  1;

  bool isCompatibleVersion( int version_value, int curr_version ) {
    return ( curr_version >= version_value );
  }

  int getVersion( int version ) {
    if ( version < FIRST_VERSION ||
         version > LAST_VERSION ) {
      return FIRST_VERSION;
    }
    return version;
  }

  struct CraftFilter {
    std::pair<int,int> leg;
    CraftFilter( ){}
    CraftFilter( int point_dep, int point_arv ) {
      leg = std::make_pair(point_dep, point_arv);
    }
    std::string toString() const {
      std::ostringstream buf;
      buf << "CraftFilter: point_dep=" << leg.first;
      if ( leg.second != ASTRA::NoExists ) {
        buf << ",point_arv=" << leg.second;
      }
      return buf.str();
    }
  };

  struct WebCraftFilters {
    CraftFilter filterCraft;
    FilterWebSeat filterWebSeat;
    std::string cabin_subclass;
    WebCraftFilters( int point_id, const std::vector<AstraWeb::TWebPax> &pnr) {
      filterWebSeat.point_id = point_id;
      filterWebSeat.pnr = pnr;
      int point_arv = NoExists;
      for ( vector<TWebPax>::const_iterator ipax=pnr.begin(); ipax!=pnr.end(); ipax++ ) {
        if ( !ipax->cabin_class.empty() && filterWebSeat.cabin_classes.find( ipax->cabin_class ) == filterWebSeat.cabin_classes.end() ) {
          filterWebSeat.cabin_classes.insert( ipax->cabin_class );
        }
        if ( !ipax->cabin_subclass.empty() ) {
          cabin_subclass = ipax->cabin_subclass;
        }
        TPerson p=DecodePerson(ipax->pers_type_extended.c_str());
        filterWebSeat.pr_CHIN=(filterWebSeat.pr_CHIN || p==ASTRA::child || p==ASTRA::baby); //среди типов может быть БГ (CBBG) который приравнивается к взрослому
        if ( point_arv == ASTRA::NoExists ) {
          point_arv = SALONS2::getCrsPaxPointArv( ipax->crs_pax_id, point_id );
        }
      }
      filterCraft = CraftFilter( point_id, point_arv );
    }
    void setPassRem( const std::string airline ) {
      TSublsRems subcls_rems( airline );
      subcls_rems.IsSubClsRem( cabin_subclass, filterWebSeat.pass_rem );
    }
    void setPrLat( bool pr_lat ) {
      filterWebSeat.pr_lat = pr_lat;
    }
    std::string toString() const {
      std::ostringstream buf;
      buf << filterCraft.toString() << "\n" << filterWebSeat.toString();
      return buf.str();
    }
  };

  void TWebPlace::layerFromSeats( SALONS2::IPlace &seat, const FilterWebSeat &filterWebSeat ) { //вычисляем слой
    bool pr_first = true;
    for( std::vector<TPlaceLayer>::iterator ilayer=seat->layers.begin(); ilayer!=seat->layers.end(); ilayer++ ) { // сортировка по приоритетам
      LogTrace(TRACE5) << "TWebPlace::" <<__FUNCTION__ << ": " << EncodeCompLayerType(ilayer->layer_type) << "," << string(seat->yname+seat->xname).c_str();
      if ( layer_type == cltUnknown ) { //задаем самый вверхний и приоритетный слой
        layer_type = ilayer->layer_type;
        layer_pax_id = ilayer->pax_id;
      }
      if ( pr_first &&
           ilayer->layer_type != cltUncomfort &&
           ilayer->layer_type != cltSmoke &&
           ilayer->layer_type != cltUnknown ) { //место используется, определяем статус резерва пассажиром
        pr_first = false;
        pr_free = false;
        bool isOwnerFreePlace = false;
        reserv_owner =  ( ( ilayer->layer_type == cltPNLCkin ||
                            SALONS2::isUserProtectLayer( ilayer->layer_type ) ) && (isOwnerFreePlace=SALONS2::isOwnerFreePlace<TWebPax>( ilayer->pax_id, filterWebSeat.pnr )) );
        LogTrace(TRACE5) << __FUNCTION__ << ": layer(" << ilayer->toString() << "), isOwnerFreePlace=" << isOwnerFreePlace;
      }
      if ( ilayer->layer_type == cltCheckin ||
           ilayer->layer_type == cltTCheckin ||
           ilayer->layer_type == cltGoShow ||
           ilayer->layer_type == cltTranzit ) { //занято зарегистрированным пассажиром
        pr_first = false;
        if ( SALONS2::isOwnerPlace<TWebPax>( ilayer->pax_id, filterWebSeat.pnr ) ) {
          pax_id = ilayer->pax_id;
        }
      }
    }
    pr_free = ( pr_free || pr_first ); // 0 - занято, 1 - свободно, 2 - частично занято
    reserv_owner = ( reserv_owner || pr_first );
  }

  std::string TWebPlace::getSubCls( const vector<TRem> &rems ) { //вычисляем подкласс места
     for ( vector<TRem>::const_iterator irem=rems.begin(); irem!=rems.end(); irem++ ) {
       if ( SALONS2::isREM_SUBCLS( irem->rem ) && !irem->pr_denial ) {
         return irem->rem;
       }
     }
     return "";
  }

  void TWebPlace::propsFromPlace( SALONS2::IPlace &seat, const FilterWebSeat &filterWebSeat ) { //вычисляем все свойства мест
    x = seat->x;
    y = seat->y;
    xname = seat->xname;
    yname = seat->yname;
    seat_no = seat->denorm_view(filterWebSeat.pr_lat);
    elem_type = seat->elem_type;
    cls = seat->clname;
    rems = seat->rems;
    pr_free = 0;
    pr_CHIN = false;
    pax_id = NoExists;
    SeatTariff = seat->SeatTariff;
    rfisc = seat->getRFISC( filterWebSeat.point_id );
    if ( seat->isplace &&
         !seat->clname.empty() &&
         filterWebSeat.cabin_classes.find( seat->clname ) != filterWebSeat.cabin_classes.end() ) {
      layerFromSeats( seat, filterWebSeat );
      if ( pr_free ) { //место свободно
        string seat_subcls = getSubCls( seat->rems );
        if ( !filterWebSeat.pass_rem.empty() ) { //у пассажира есть подкласс
          if ( filterWebSeat.pass_rem == seat_subcls ) {
            pr_free = 3;  // свободно с учетом подкласса
          }
          else
            if ( seat_subcls.empty() ) { // нет подкласса у места
              pr_free = 2; // свободно без учета подкласса
            }
            else { // подклассы не совпали
              pr_free = 0; // занято
              reserv_owner = false;
            }
        }
        else { // у пассажира нет подкласса
          if ( !seat_subcls.empty() ) { // подкласс у места
              pr_free = 0; // занято
              reserv_owner = false;
          }
        }
      } //место свободно
      if ( filterWebSeat.pr_CHIN ) { // встречаются в группе пассажиры с детьми
        if ( seat->elem_type == ARMCHAIR_EMERGENCY_EXIT_TYPE ) { // место у аварийного выхода
          pr_CHIN = true;
        }
        else {
          for ( vector<TRem>::const_iterator irem=seat->rems.begin(); irem!=seat->rems.end(); irem++ ) {
            if ( irem->pr_denial && irem->rem == "CHIN" ) {
              pr_CHIN = true;
              break;
            }
          }
        }
      }
    } // end if ( seat->isplace && !seat->clname.empty() && seat->clname == filterWebSeat.cabin_class )
  }

  int TWebPlace::get_seat_status( bool pr_find_free_subcls_place, bool view_craft ) const {
    int status = 0;
    switch( pr_free ) {
      case 0: // занято
          if ( view_craft ) {
            status = 1;
          }
          else {
            status = !reserv_owner;
          }
          break;
      case 1: // свободно
          status = 0;
          break;
      case 2: // свободно без учета подкласса
          status = pr_find_free_subcls_place;
          break;
      case 3: // свободно с учетом подкласса
          status = !pr_find_free_subcls_place;
          break;
    };
    if ( status == 0 && pr_CHIN ) {
      status = 2;
    }
    if ( TReqInfo::Instance()->client_type == ctKiosk && status != 1 && !SeatTariff.empty() ) {
      status = 1;
    }
    return status;
  }

  void WebCraft::add( int num, const TWebPlaceList &placeList ) {
    if ( !placeList.empty() ) {
      pr_find_free_subcls_place |= placeList.FreeSubclsExists();
      insert( make_pair( num, placeList ) );
    }
  }
  void WebCraft::Read( int point_id, const std::vector<AstraWeb::TWebPax> &pnr ) {
    LogTrace(TRACE5) << "WebCraft:" <<__FUNCTION__ << " point_id=" << point_id << ",version=" << version;
    clear();
    WebCraftFilters filters( point_id, pnr );
    if ( filters.filterWebSeat.cabin_classes.empty() ) {
      throw UserException( "MSG.CLASS.NOT_SET" );
    }
    if ( version == FIRST_VERSION &&
         filters.filterWebSeat.cabin_classes.size() > 1 ) {
      throw EXCEPTIONS::Exception( "more then 1 cabin classes in pnr" );
    }
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( filters.filterCraft.leg.first, filters.filterCraft.leg.second ),
                          isCompatibleVersion( ALL_CLASS_CRAFT_VERSION, version )?"":(*filters.filterWebSeat.cabin_classes.begin()), NoExists );
    filters.setPassRem( salonList.getAirline() );
    filters.setPrLat( salonList.isCraftLat() );
    LogTrace(TRACE5) << __FUNCTION__ << filters.toString();
    //задаем все возможные статусы для разметки группы
    vector<ASTRA::TCompLayerType> grp_layers;
    grp_layers.push_back( cltCheckin );
    grp_layers.push_back( cltTCheckin );
    grp_layers.push_back( cltTranzit );
    grp_layers.push_back( cltProtBeforePay );
    grp_layers.push_back( cltProtAfterPay );
    grp_layers.push_back( cltPNLBeforePay );
    grp_layers.push_back( cltPNLAfterPay );
    grp_layers.push_back( cltProtSelfCkin );
    TFilterRoutesSets filterRoutes = salonList.getFilterRoutes();
    TDropLayersFlags dropLayersFlags;
    TCreateSalonPropFlags propFlags;
    propFlags.setFlag( clDepOnlyTariff );
    TSalons SalonsN;
    salonList.CreateSalonsForAutoSeats<TWebPax>( SalonsN,
                                                 filterRoutes,
                                                 propFlags,
                                                 grp_layers,
                                                 pnr,
                                                 dropLayersFlags );
    TSalons *Salons = &SalonsN;
    if ( salonList.getRFISCMode() != rTariff  ) {
      Salons->SetTariffsByRFISC(point_id);
    }
    for( vector<TPlaceList*>::iterator placeList = Salons->placelists.begin(); // пробег по салонам
         placeList != Salons->placelists.end(); placeList++ ) {
      TWebPlaceList webPlaceList;

      for ( IPlace place = (*placeList)->places.begin(); //пробег по местам
            place != (*placeList)->places.end(); place++ ) {
        if ( !place->visible ) {
          continue;
        }
        if ( !place->isplace &&
             ((!place->yname.empty() &&
                place->yname.find("=") != std::string::npos) ||
               (!place->xname.empty() &&
                 place->xname.find("=") != std::string::npos)) ) {
          place->xname.clear();
          place->yname.clear();
        };
        webPlaceList.add( TWebPlace( place, filters.filterWebSeat ) );
      }
      add( (*placeList)->num, webPlaceList );
    }
  }

  bool WebCraft::findSeat( std::string seat_no, TWebPlace &wsp ) {
    for ( WebCraft::const_iterator iwebSalon=begin(); iwebSalon!=end(); iwebSalon++ ) {
      for ( TWebPlaces::const_iterator wp = iwebSalon->second.begin(); wp != iwebSalon->second.end(); wp++ ) {
        if ( seat_no == wp->getSeatNo() ) {
           wsp = *wp;
           return true;
        }
      }
    }
    return false;
  }

  void TWebPlace::toXML( int version, bool isFreeSubclsSeats, xmlNodePtr placeListNode ) const {
    xmlNodePtr placeNode = NewTextChild( placeListNode, "place" );
    NewTextChild( placeNode, "x", x );
    NewTextChild( placeNode, "y", y );
    NewTextChild( placeNode, "seat_no", seat_no );
    NewTextChild( placeNode, "elem_type", elem_type );
    if ( !isCompatibleVersion( ALL_CLASS_CRAFT_VERSION, version ) ) {
      NewTextChild( placeNode, "status", get_seat_status( isFreeSubclsSeats, true ) );
    }
    else {
      NewTextChild( placeNode, "cls", cls );
    }
    if ( pax_id != NoExists ) {
      NewTextChild( placeNode, "pax_id", pax_id );
    }
    if ( !SeatTariff.empty() ) { // если платная регистрация отключена, value=0.0 в любом случае
      xmlNodePtr rateNode = NewTextChild( placeNode, "rate" );
      NewTextChild( rateNode, "color", SeatTariff.color );
      NewTextChild( rateNode, "value", SeatTariff.rateView() );
      NewTextChild( rateNode, "currency", SeatTariff.currencyView(TReqInfo::Instance()->desk.lang) );
    }
    if ( !rfisc.empty() ) {
      NewTextChild( placeNode, "rfisc", rfisc.code );
    }
    if ( layer_type != cltUnknown ) {
      xmlNodePtr layerNode = NewTextChild( placeNode, "layer" );
      NewTextChild( layerNode, "layer_type", EncodeCompLayerType( layer_type ) );
      if ( layer_pax_id != ASTRA::NoExists ) {
        NewTextChild( layerNode, "pax_id", layer_pax_id );
      }
    }
    if ( !rems.empty() ) {
      xmlNodePtr remsNode = NewTextChild( placeNode, "remarks" );
      for ( std::vector<TRem>::const_iterator irem=rems.begin(); irem!=rems.end(); irem++ ) {
        xmlNodePtr rNode = NewTextChild( remsNode, "remark" );
        NewTextChild( rNode, "code", irem->rem );
        NewTextChild( rNode, "pr_denial", irem->pr_denial );
      }
    }
  }

  void TWebPlaceList::toXML( int version, int num, bool isFreeSubclsSeats, xmlNodePtr viewCraftNode ) const {
    xmlNodePtr placeListNode = NewTextChild( viewCraftNode, "placelist" );
    SetProp( placeListNode, "num", num );
    SetProp( placeListNode, "xcount", xcount + 1 );
    SetProp( placeListNode, "ycount", ycount + 1 );
    for ( TWebPlaces::const_iterator wp = begin(); wp != end(); wp++ ) {
      wp->toXML( version, isFreeSubclsSeats, placeListNode );
    }
  }

  void WebCraft::toXML( xmlNodePtr craftNode ) {
     xmlNodePtr viewCraftNode = NewTextChild( craftNode, "salons" );
     for( WebCraft::const_iterator iwebSalon=begin(); iwebSalon!=end(); iwebSalon++ ) {
       iwebSalon->second.toXML( version, iwebSalon->first, pr_find_free_subcls_place, viewCraftNode  );
     }
  }

  /*
  1. Что делать если пассажир имеет спец. подкласс (ремарки MCLS) - Пока выбираем только места с ремарками нужного подкласса.
  Что делать , если салон не размечен?
  2. Есть группа пассажиров, некоторые уже зарегистрированы, некоторые нет.
  */
  void ViewCraft( const vector<TWebPax> &paxs, xmlNodePtr reqNode, xmlNodePtr resNode)
  {
    int point_id = NodeAsInteger( "point_id", reqNode );
  //  xmlNodePtr flagsNode = GetNode( "passengers/flags", reqNode );
    xmlNodePtr flagsNode = GetNode( "passengers", reqNode );
    if ( SALONS2::isFreeSeating( point_id ) ) { //???
      throw UserException( "MSG.SALONS.FREE_SEATING" );
    }
    int version = FIRST_VERSION;
    xmlNodePtr versionNode = GetNode( "version", reqNode );
    if ( versionNode ) {
      version = NodeAsInteger( versionNode );
    }
    WebCraft webCraft( getVersion( version ) );
    webCraft.Read( point_id, paxs );

    xmlNodePtr nodeViewCraft = NewTextChild( resNode, "ViewCraft" );
    ProgTrace( TRACE5, "FreeSubclsExists=%d", webCraft.FreeSubclsExists() );
    if ( isCheckinWOChoiceSeats( point_id ) ) {
      NewTextChild(nodeViewCraft, "checkin_wo_choice_seats");
    }

    webCraft.toXML( nodeViewCraft );

    TSalonList &salonList = webCraft.getSalonList();
    if ( !webCraft.empty() ) {
      //salon_descriptions
      TSalonDesrcs descrs;
      getSalonDesrcs( point_id, descrs );
      if ( !descrs.empty() ) {
        xmlNodePtr salonDescrsNode = NULL;
        for ( TSalonDesrcs::iterator idescr=descrs.begin(); idescr!=descrs.end(); idescr++ ) {
           if ( !idescr->second.empty() ) {
             if ( salonDescrsNode == NULL ) {
               salonDescrsNode = NewTextChild( nodeViewCraft, "salon_properties" );
             }
             xmlNodePtr salonListNode = NewTextChild( salonDescrsNode, "placelist" );
             SetProp( salonListNode, "num", idescr->first );
             for ( std::set<std::string>::iterator icompart=idescr->second.begin(); icompart!=idescr->second.end(); icompart++ ) {
               NewTextChild( salonListNode, "property", *icompart );
             }
           }
        }
      }
      SALONS2::checkBuildSections( point_id, salonList.getCompId(), nodeViewCraft, TAdjustmentRows().get( salonList ), false );
    }
    ProgTrace( TRACE5, "flagsNode %d", flagsNode != NULL );
    xmlNodePtr passesNode = NULL;
    TSalonPassengers passengers;
    TGetPassFlags flags;
  /*  flagsNode = flagsNode->children;
    while ( flagsNode != NULL && string("flag") == (char*)flagsNode->name ) {
      string mode = NodeAsString( flagsNode );
      if ( "Passengers" == mode ) {
        flags.setFlag( gpPassenger );
      }
      if ( "Tranzits" == mode ) {
        flags.setFlag( gpTranzits );
      }
      if ( "Infants" == mode ) {
        flags.setFlag( gpInfants );
      }
      if ( "WaitList" == mode ) {
        flags.setFlag( gpWaitList );
      }
      flagsNode = flagsNode->next;
    }*/
    flags.setFlag( gpPassenger );
    salonList.getPassengers( passengers, flags );
    ProgTrace( TRACE5, "salon list passengers is empty %d", passengers.empty() );
    for ( SALONS2::TSalonPassengers::iterator ipass_dep=passengers.begin(); //point_dep
          ipass_dep!=passengers.end(); ipass_dep++ ) {
      tst();
      if ( ipass_dep->first != point_id ) {
        continue;
      }
      for ( SALONS2::TIntArvSalonPassengers::iterator ipass_arv=ipass_dep->second.begin();
            ipass_arv!=ipass_dep->second.end(); ipass_arv++ ) { // point_arv
        for ( SALONS2::TIntClassSalonPassengers::iterator ipass_class=ipass_arv->second.begin(); //class,grp_status,TSalonPax
              ipass_class!=ipass_arv->second.end(); ipass_class++ ) {
          for ( SALONS2::TIntStatusSalonPassengers::iterator ipass_status=ipass_class->second.begin(); //grp_status,TSalonPax
                ipass_status!=ipass_class->second.end(); ipass_status++ ) { //
            for ( std::set<SALONS2::TSalonPax,SALONS2::ComparePassenger>::iterator ipass=ipass_status->second.begin();
                  ipass!=ipass_status->second.end(); ipass++ ) {
              TWaitListReason waitListReason;
              TPassSeats seats;
              ipass->get_seats( waitListReason, seats );
              if ( !passesNode ) {
                passesNode = NewTextChild( nodeViewCraft, "passengers" );
              }
              xmlNodePtr pNode = NewTextChild( passesNode, "passenger" );
              //NewTextChild( pNode, "grp_id", ipass->grp_id );
              NewTextChild( pNode, "pax_id", ipass->pax_id );
              //NewTextChild( pNode, "point_dep", ipass_dep->first );
              //NewTextChild( pNode, "point_arv", ipass_arv->first );
              //NewTextChild( pNode, "pr_tranzit", ipass_dep->first != point_id );
              //NewTextChild( pNode, "class", ipass_class->first );
              //NewTextChild( pNode, "grp_status", ipass_status->first );
              //NewTextChild( pNode, "pr_infant", ipass->pr_infant );
              //NewTextChild( pNode, "parent_pax_id", ipass->parent_pax_id );
              //NewTextChild( pNode, "reg_no", ipass->reg_no );
              NewTextChild( pNode, "gender", CheckIn::isFemaleStr( ipass->is_female ) );
  //            NewTextChild( pNode, "pers_type", EncodePerson( ipass->pers_type ) );
              CheckIn::TPaxDocItem doc;
              CheckIn::LoadPaxDoc( ipass->pax_id, doc);
              NewTextChild( pNode, "birth_date", doc.birth_date!=ASTRA::NoExists?DateTimeToStr(doc.birth_date, "dd.mm.yyyy"):string("") );
    //        xmlNodePtr seatsNode = NewTextChild( pNode, "seats" );
      //      SetProp( seatsNode, "count", ipass->seats );
        //    for (TPassSeats::const_iterator iseat=seats.begin(); iseat!=seats.end(); iseat++) {
          //    NewTextChild( seatsNode, "seat", GetSeatView(*iseat, "one", salonList.isCraftLat()) );
            //}
            }
          }
        }
      }
    }
  }

  // передается заполненные поля crs_pax_id, crs_seat_no, class, subclass
  // на выходе заполнено TWebPlace по пассажиру
  void GetCrsPaxSeats( int point_id, const vector<TWebPax> &pnr,
                       vector< pair<TWebPlace, LexemaData> > &pax_seats )
  {
    pax_seats.clear();
    if ( pnr.empty() ) {
      return;
    }
    WebCraft webCraft( ALL_CLASS_CRAFT_VERSION );
    webCraft.Read( point_id, pnr );
    for ( vector<TWebPax>::const_iterator ipax=pnr.begin(); ipax!=pnr.end(); ipax++ ) { // пробег по пассажирам
      LexemaData ld;
      TWebPlace wsp;
      bool pr_find = webCraft.findSeat( ipax->crs_seat_no, wsp );
      if ( pr_find ) {
        if ( wsp.get_seat_status( webCraft.FreeSubclsExists(), false ) == 1 ) {
           ld.lexema_id = "MSG.SEATS.SEAT_NO.NOT_AVAIL";
        }
         wsp.setPaxId( ipax->crs_pax_id );
         pax_seats.push_back( make_pair( wsp, ld ) );
      }
      else {
        TWebPlace wsp;
        wsp.setPaxId( ipax->crs_pax_id );
        wsp.setSeatNo( ipax->crs_seat_no );
        ld.lexema_id = "MSG.SEATS.SEAT_NO.NOT_FOUND";
        pax_seats.push_back( make_pair( wsp, ld ) );
      }
  /*    for( map<int, TWebPlaceList>::iterator isal=web_salons.begin(); isal!=web_salons.end(); isal++ ) {
        for ( TWebPlaces::iterator wp = isal->second.places.begin();
              wp != isal->second.places.end(); wp++ ) {
          if ( i->crs_seat_no == wp->seat_no ) {
            if ( get_seat_status( *wp, pr_find_free_subcls_place, false ) == 1 )
              ld.lexema_id = "MSG.SEATS.SEAT_NO.NOT_AVAIL";
            wp->pax_id = i->crs_pax_id;
            pr_find = true;
            pax_seats.push_back( make_pair( *wp, ld ) );
              break;
          }
        } // пробег по салону
        if ( pr_find )
          break;
      } // пробег по салону
      if ( !pr_find ) {
        TWebPlace wp;
        wp.pax_id = i->crs_pax_id;
        wp.seat_no = i->crs_seat_no;
        ld.lexema_id = "MSG.SEATS.SEAT_NO.NOT_FOUND";
        pax_seats.push_back( make_pair( wp, ld ) );
      }*/
    } // пробег по пассажирам
  }

}
}

