#ifndef _SEAT_DESCRIPT_H_
#define _SEAT_DESCRIPT_H_

#include "astra_utils.h"
#include "astra_consts.h"
#include "base_tables.h"
#include "seats_utils.h"
#include "salons.h"

namespace SEAT_DESCR
{

const std::string REMARK_CHILDREN_MEDA = "CHIN";
const std::string REMARK_INFANT = "INFT";
const std::string REMARK_COMFORT = "SPC+";
const std::string REMARK_UNACCOMPANIED_CHILDREN = "UMNR";
const std::string REMARK_MEDA = "MEDA";
const std::string REMARK_LEGSPACE = "LEGS";
const std::string REMARK_WITHOUT_WINDOW = "WOWI";

class SeatDescrService
{
  public:
    enum Enum
    {
      LeftAirCraftSide,       // Левая сторона воздушного судна
      RightAirCraftSide,      // Правая сторона воздушного судна
      WindowSeat,             // Места у окна
      AisleSeat,              // Место у прохода
      SmokingSeat,            // Место для курящих
      PartitionSeat,          // Место у перегородки
      FacingTailSeat,         // Место лицом к хвосту
      ComforttableSeat,       // Комфортное место
      NotChildrenSeat,        // Место не для детей
      NotBabySeat,            // Не для пассажирами с младенцами
      NotEscortChildrenSeat,  // Не для сопровождающих детей
      NotSickSeat,            // Не для больных
      LegSpaceSeat,           // Место с пространством для ног
      WindowWOWindowSeat,     // Место у окна без окна
      EmergencyExitRowSeat,   // Место в ряду аварийного выхода
      NotSaleOfferedLastSeat, // Не продается или предлагается последним
      PaySeat,                // Платное место
      LocationToiletSeat,     // Место, соседнее с туалетом
      FrontToiletSeat,        // Место перед туалетом
      BehindToiletSeat,       // Место позади туалета
      MiddleSeat,             // Место посередине
      MiddleSectorSeat        // Место в центральном секторе
    };

    static const std::list< std::pair<Enum, std::string> >& pairs()
    {
      static std::list< std::pair<Enum, std::string> > l =
      {
        {LeftAirCraftSide, "LS"},
        {RightAirCraftSide, "RS"},
        {WindowSeat, "W"},
        {AisleSeat, "A"},
        {SmokingSeat, "S"},
        {PartitionSeat, "K"},
        {FacingTailSeat, "J"},
        {ComforttableSeat, "P"},
        {NotChildrenSeat, "IE"},
        {NotBabySeat, "1A"},
        {NotEscortChildrenSeat, "1C"},
        {NotSickSeat, "1B"},
        {LegSpaceSeat, "L"},
        {WindowWOWindowSeat, "1W"},
        {EmergencyExitRowSeat, "E"},
        {NotSaleOfferedLastSeat, "V"},
        {PaySeat, "CH"},
        {LocationToiletSeat, "AL"},
        {FrontToiletSeat, "7A"},
        {BehindToiletSeat, "7B"},
        {MiddleSeat, "MS"},
        {MiddleSectorSeat, "CC"}
      };
      return l;
    }
/*    static const std::list< std::pair<Enum, std::string> > &names()
    {
      static std::list< std::pair<Enum, std::string> > s =
      {
        {LeftAirCraftSide, "Левая сторона ВС"},
        {RightAirCraftSide, "RS"},
        {WindowSeat, "W"},
        {AisleSeat, "A"},
        {SmokingSeat, "S"},
        {PartitionSeat, "K"},
        {FacingTailSeat, "J"},
        {ComforttableSeat, "P"},
        {NotChildrenSeat, "IE"},
        {NotBabySeat, "1A"},
        {NotEscortChildrenSeat, "1C"},
        {NotSickSeat, "1B"},
        {LegSpaceSeat, "L"},
        {WindowWOWindowSeat, "1W"},
        {EmergencyExitRowSeat, "E"},
        {NotSaleOfferedLastSeat, "V"},
        {PaySeat, "CH"},
        {LocationToiletSeat, "AL"},
        {FrontToiletSeat, "7A"},
        {BehindToiletSeat, "7B"},
        {MiddleSeat, "MS"},
        {MiddleSectorSeat, "CC"}
      };
      return s;

    }*/

    static const std::set<Enum>& sets()
    {
      static std::set<Enum> s;
      if (s.empty())
      {
        for(const auto& i : pairs())
          s.insert(i.first);
      }
      return s;
    }
};

// класс для хранения ремарок обозначающих комфортные места для разных АК
#define all(v) (v).begin(), (v).end()
#define vecstr std::vector<std::string>

class ComforttableSeatDefines: public std::map<std::string,vecstr> {
  private:
    vecstr def;
    bool isComfort( vecstr &v1, vecstr &v2 ) {
      vecstr v3;
      sort( all(v1) );
      sort( all(v2) );
      set_intersection( all(v1), all(v2), back_inserter(v3) );
      return !v3.empty();
    }
    vecstr& get( const std::string &airline ) {
      ComforttableSeatDefines::iterator f = find( airline );
      if ( f != end() ) {
        return f->second;
      }
      return def;
    }
  public:
    ComforttableSeatDefines( ) {
     def.push_back( REMARK_COMFORT );
     //insert( make_pair() );
    }
    bool isComfort( std::string airline, vecstr &remarks ) {
      return isComfort( remarks, get( airline ) );
    }
};

static ComforttableSeatDefines comforttableSeatDefines;

struct CraftProps {
  std::string airline;
  int point_dep;
  SALONS2::TRFISCMode RFISCMode;
  CraftProps(const std::string &vairline,
             int vpoint_dep, SALONS2::TRFISCMode vRFISCMode):
             airline(vairline), point_dep(vpoint_dep), RFISCMode(vRFISCMode){}
};

struct SeatCoord:public CraftProps {
  SALONS2::TPlaceList *placeList;
  int salon_num;
  int line;
  int row;
  SeatCoord( const std::string &vairline,
             int vpoint_dep, SALONS2::TRFISCMode vRFISCMode,
             SALONS2::TPlaceList *vplaceList,
             int vline, int vrow ):CraftProps(vairline, vpoint_dep, vRFISCMode),
         placeList(vplaceList),salon_num(vplaceList->num), line(vline), row(vrow){}
  bool isValidSeat( int vline, int vrow, SALONS2::TPlace &seat ) const {
    seat = SALONS2::TPlace();
    SALONS2::TPoint p( vline, vrow );
    if ( placeList->ValidPlace( p ) ) {
      seat = *placeList->place( p );
      return  ( seat.isplace && seat.visible );
    }
    return false;
  }
  bool isValidSeat( SALONS2::TPlace &seat ) const {
    return isValidSeat( line, row, seat );
  }
  bool isValidSeat( ) const {
    SALONS2::TPlace seat;
    return isValidSeat( seat );
  }
  bool isValidElemType( int vline, int vrow, SALONS2::TPlace &seat, const std::string &elem_type ) const {
    seat = SALONS2::TPlace();
    SALONS2::TPoint p( vline, vrow );
    BASIC_SALONS::TCompElemType elemType;
    if ( placeList->ValidPlace( p ) &&
         BASIC_SALONS::TCompElemTypes::Instance()->getElem( elem_type, elemType ) ) {
      seat = *placeList->place( p );
      return  ( seat.visible && seat.elem_type == elem_type );
    }
    return false;
  }
  bool isValidElemType( SALONS2::TPlace &seat, const std::string &elem_type ) const {
    return isValidElemType( line, row, seat, elem_type );
  }
};

class SeatDescrServices : public ASTRA::PairList<SeatDescrService::Enum, std::string>
{
  private:
    virtual std::string className() const { return "SeatDescrServices"; }
  public:
    SeatDescrServices() : ASTRA::PairList<SeatDescrService::Enum, std::string>(SeatDescrService::pairs(),
                                                                               boost::none,
                                                                               boost::none) {}
};

const SeatDescrServices& seatDescrServices();

class descriptSeatCreator { //класс реализует вычисление свойств мест
  private:
    //common algos
    static bool existsRemark( const SeatCoord &seatCoord, const std::string &remark, bool pr_denial );
    static bool existsLayer(  const SeatCoord &seatCoord, ASTRA::TCompLayerType layer_type );
    static bool locationLeftElemTypeSeat( const SeatCoord &seatCoord, int line, int row, const std::string &elem_type  );
    static bool locationRightElemTypeSeat( const SeatCoord &seatCoord, int line, int row, const std::string &elem_type  );
    static bool locationElemTypeSeat( const SeatCoord &seatCoord, int line, int row, const std::string &elem_type );
    //====================================
    static bool leftAirCraftSide( const  SeatCoord &seatCoord );
    static bool rightAirCraftSide( const  SeatCoord &seatCoord );
    static bool windowSeat( const  SeatCoord &seatCoord );
    static bool aisleSeat( const  SeatCoord &seatCoord );
    static bool smokingSeat( const  SeatCoord &seatCoord );
    static bool partitionSeat( const  SeatCoord &seatCoord );
    static bool facingTailSeat( const  SeatCoord &seatCoord );
    static bool comforttableSeat( const  SeatCoord &seatCoord );
    static bool notChildrenSeat( const  SeatCoord &seatCoord );
    static bool notBabySeat( const  SeatCoord &seatCoord );
    static bool notEscortChildrenSeat( const  SeatCoord &seatCoord );
    static bool notSickSeat( const  SeatCoord &seatCoord );
    static bool legSpaceSeat( const  SeatCoord &seatCoord );
    static bool windowWOWindowSeat( const  SeatCoord &seatCoord );
    static bool emergencyExitRowSeat( const  SeatCoord &seatCoord );
    static bool notSaleOfferedLastSeat( const  SeatCoord &seatCoord );
    static bool paySeat( const  SeatCoord &seatCoord );
    static bool locationToiletSeat( const  SeatCoord &seatCoord );
    static bool frontToiletSeat( const  SeatCoord &seatCoord );
    static bool behindToiletSeat( const  SeatCoord &seatCoord );
    static bool middleSeat( const  SeatCoord &seatCoord );
    static bool middleSectorSeat( const  SeatCoord &seatCoord );
  public:
    static std::pair<SeatDescrService::Enum,bool> translate( SeatDescrService::Enum valueType, const SeatCoord &seatCoord );
};

class descrSeatType:public std::map<SeatDescrService::Enum,bool>  { // создается экземпляр и хранит свойства мест Источник БД
  public:
    void init() {
      for ( const auto e : SeatDescrService::sets() ) {
        insert( std::make_pair( e, false ) );
      }
    }
    descrSeatType() {
      init();
    }
    descrSeatType( bool pr_init ) {
      if ( pr_init ) {
        init();
      }
    }
    void add( SeatDescrService::Enum valueType ) {
      (*this)[ valueType ] = true;
    }
    void add( const std::string code ) {
      add( seatDescrServices().decode( code ) );
    }
    std::string traceStr() const {
      std::ostringstream s;
      s << "=========================================" << std::endl << "descrSeatType description: " << std::endl;
      for ( const auto e : SeatDescrService::sets() ) {
        s << seatDescrServices().encode( e ) << "=" << find( e )->second <<"|";
      }
      s << "========================================="  << std::endl;
      return s.str();
    }
    xmlNodePtr toXML( xmlNodePtr node );
    std::string toPropHintFormatStr();
    void toXMLPropHintFormatStr( xmlNodePtr node );
    void Trace();
};

class descrSeatTypeOrigin:public descrSeatType, public SeatCoord  { // создается экземпляр и хранит свойства мест Источник компоновка, начального заполнения элементами нет
  public:
    descrSeatTypeOrigin( const std::string &airline,
                         int point_dep, SALONS2::TRFISCMode RFISCMode,
                         SALONS2::TPlaceList *placeList,
                         int line, int row ):descrSeatType(false),SeatCoord(airline,point_dep,RFISCMode,placeList,line,row) {
      for ( const auto e : SeatDescrService::sets() ) {
        insert( descriptSeatCreator::translate( e, *this ) );
      }
    }
    std::string traceStr() const {
      std::ostringstream s;
      SALONS2::TPlace seat;
      if ( isValidSeat( seat ) ) {
        s << "descrSeatTypeOrigin: salon num=" << salon_num << ",seat=" << seat.yname << "," << seat.xname << std::endl << "description: " << std::endl;
        for ( const auto e : SeatDescrService::sets() ) {
          s << seatDescrServices().encode( e ) << " " << find( e )->second << std::endl;
        }
      }
      else {
        s << "descrSeatTypeOrigin: salon num=" << salon_num << ",seat=" << seat.yname << "," << seat.xname << std::endl << "description: invalid seat" << std::endl;
      }
      return s.str();
    }
};

class descrSeatTypeContainer:public std::map<int,descrSeatType> { //key = SALONS2::getSeatKey, value = множество свойств мест салона
  private:
    static const std::string SELECT_SQL;
    static const std::string CLEAR_SQL;
    static const std::string INSERT_SQL;
    int point_dep;
  public:
    descrSeatTypeContainer(int vpoint_dep):point_dep(vpoint_dep) {}
    descrSeatTypeContainer():point_dep(ASTRA::NoExists) {}
    void toDB();
    void fromDB();
    void toXML( xmlNodePtr node );
    descrSeatTypeContainer& operator << (const descrSeatTypeOrigin &descrSeat) { //заливаем из компоновки - авто расчет свойств
      (*this)[ SALONS2::getSeatKey::get( descrSeat.salon_num, descrSeat.line, descrSeat.row ) ] = descrSeat;
      return *this;
    }
};

class descrSeatTypePaxContainer:public std::map<int,descrSeatType> { //key = SALONS2::getSeatKey, value = множество свойств мест салона
  private:
    static const std::string SELECT_SQL;
    static const std::string CLEAR_SQL;
    static const std::string INSERT_SQL;
    int point_dep;
    int pax_id;
  public:
    descrSeatTypePaxContainer(int vpoint_dep, int vpax_id):point_dep(vpoint_dep), pax_id(vpax_id){}
    void toDB();
    void fromDB();
    void toXML() {}
    descrSeatTypePaxContainer& operator << (const descrSeatTypeOrigin &descrSeat) { //заливаем из компоновки - авто расчет свойств
      (*this)[ SALONS2::getSeatKey::get( descrSeat.salon_num, descrSeat.line, descrSeat.row ) ] = descrSeat;
      return *this;
    }
};

bool isConstructiveRow( SALONS2::TPlaceList* isalon, int row );

class paxsWaitListDescrSeat:public std::map<int, std::map<TSeat,std::string,CompareSeat> > {
  public:
    void get( int point_dep );
    std::string getDescr( const std::string &format, int pax_id );
    void getDescr( int pax_id, std::vector<std::string> &res );
};

typedef std::map<TSeat,std::string,CompareSeat> paxDescrSeats;


// настройка на рейс, чтобы этот механизм начал работать

} //namespace SEAT_DESCR
#endif //_SEAT_DESCRIPT_H_
