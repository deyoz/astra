#include "ComponCreator.h"


#include "images.h"
#include "salons.h"
#include "comp_props.h"
#include "astra_elems.h"
#include "counters.h"
#include "alarms.h"
#include "points.h"
#include "seat_number.h"
#include "astra_locale_adv.h"

#define NICKNAME "DJEK"
#include "serverlib/slogger.h"
namespace ComponCreator {

const //AHM
  std::string AISLE_LEFT_CODE_SEAT = "AL";
  std::string AISLE_RIGHT_CODE_SEAT = "AR";
  std::string EMERGENCY_EXIT_CODE_SEAT = "E"; // ���਩�� ��室 elem_type="A"
  std::string BASSINET_POSITIONS_CODE_SEAT = "B"; //��쪠 remark "BSCT"
  std::string BULKHEAD_POSITIONS_CODE_SEAT = "F"; //-??? ���।� ��ॣ�த�� - �� �� ��砫� ᠫ��� ���� �� �⮡ࠦ��� �⤥�쭮 ��ॣ�த��?
  std::string INCAPACITATED_PASS_CODE_SEAT = "H"; //-??? �� ���ᯮᮡ�� ���ᠦ��
  std::string INFANT_PREFERENCE_CODE_SEAT = "I"; // ���� ��� ������楢 remark "INFT"
  std::string REAR_FACING_CODE_SEAT = "J"; // ���� ���饭��� �����  agle = 180
  std::string NEAR_GALLEY_CODE_SEAT = "K"; //-??? ����� ����
  std::string LEGS_SPACE_CODE_SEAT = "L"; // ���� ��� ���
  std::string WHEEL_CHAIR_CODE_SEAT = "M"; //??? ���������� ����᪠ remark "WCHC"
  std::string OVER_WING_CODE_SEAT = "O"; //-??? ��뫮 remark "WING"
  std::string STRETCHER_LOCATION_CODE_SEAT = "P"; //��ᨫ�� remark "STCR"
  std::string QUIET_ZONE_CODE_SEAT = "Q"; //???-��� ����
  std::string SMOKING_CODE_SEAT = "S"; // ��� ������
  std::string NEAR_TOILET_CODE_SEAT = "�"; //???-�冷� � �㠫�⮬ ���ᮢ뢠�� �㠫���?
  std::string UNNACOMPANIED_MINOR_CODE_SEAT = "U"; //��ᮢ��饭����⭨� ��� ᮯ஢������� remark UMNR
  std::string LEFT_VACANT_CODE_SEAT = "U"; //��� ������ � ��᫥���� ��।�
  std::string NOT_MOVIE_CODE_SEAT = "W"; //???-
  std::string NOT_AVAILABLE_CODE_SEAT = "X";//not available - ����� ���� '000'. �᫨ ��� �।���� ������ X, � ��� ��� � ���� �ਦ������� ��� � ���� �� 業���
  std::string DISABLE_CODE_SEAT = "Y";//not fitted �� ��⠭������ - ����� ���� '000'. �᫨ � �।��� �⮨� Y, � ����砥� '0 0' ����
  std::string BUFFER_ZONE_CODE_SEAT = "Z";//buffer zone

int SIGN( int a ) {
    return (a > 0) - (a < 0);
}

struct TComp {
  int sum;
  int comp_id;
  TComp() {
    sum = 99999;
    comp_id = ASTRA::NoExists;
  }
};

struct CompRoute {
  int point_id;
  std::string airline;
  int flt_no;
  std::string airp;
  std::string craft;
  std::string bort;
  bool pr_reg;
  bool pr_alarm;
  bool auto_comp_chg;
  bool inRoutes;
  CompRoute() {
     point_id = ASTRA::NoExists;
     flt_no = ASTRA::NoExists;
     pr_alarm = false;
     pr_reg = false;
     auto_comp_chg = false;
     inRoutes = false;
  }
};

bool isAutoCompChg( int point_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT auto_comp_chg FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  return ( !Qry.Eof && Qry.FieldAsInteger( "auto_comp_chg" ) ); // ��⮬���᪮� �����祭�� ����������
}


void setManualCompChg( int point_id )
{
  //set flag auto change in false state
  ComponSetter compSetter( point_id );
  if ( compSetter.isLibraMode() ) {
    return; //???
  }
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "UPDATE trip_sets SET auto_comp_chg=0 WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
}

class TCompsRoutes: public std::vector<CompRoute>
{
private:
  bool CompRouteinRoutes( const CompRoute &item1, const CompRoute &item2 ) {
    return ( (item1.craft == item2.craft || item2.craft.empty()) &&
             (item1.bort == item2.bort || item2.bort.empty()) &&
             item1.airline == item2.airline );
  }

  void push_routes( const CompRoute &currroute,
                    const TTripRoute &routes,
                    bool pr_before,
                    TQuery &Qry ) {
    Qry.Clear();
    Qry.SQLText =
      "SELECT airline,flt_no,airp,bort,craft,pr_reg FROM points "
      " WHERE point_id=:point_id AND pr_del!=-1";
    Qry.DeclareVariable( "point_id", otInteger );
    for ( const auto& i : routes ) {
      //�뤥�塞 ������� 㤮���⢮���騩 ᫥�. �᫮���:
      //1. ����� �ਧ��� ��� �����祭�� ����������
      //2. ����� �ਧ��� ॣ����樨
      //3. ���� � ⨯ ��, ������������  ᮢ������ � ��室�� �㭪⮬
      //4. �� ��᫥���� �㭪� � �࠭��⭮� �������
      CompRoute route;
      bool inRoutes = true;
      route.point_id = i.point_id;
      route.auto_comp_chg = isAutoCompChg( i.point_id );
      Qry.SetVariable( "point_id", i.point_id );
      Qry.Execute();
      route.airline = Qry.FieldAsString( "airline" );
      route.flt_no = Qry.FieldAsInteger( "flt_no" );
      route.airp = Qry.FieldAsString( "airp" );
      route.craft = Qry.FieldAsString( "craft" );
      route.bort = Qry.FieldAsString( "bort" );
      route.pr_reg = ( Qry.FieldAsInteger( "pr_reg" ) == 1 );
      route.inRoutes = ( CompRouteinRoutes( currroute, route ) && inRoutes);
      if ( !pr_before && (!route.inRoutes || i.point_id == routes.back().point_id ) ) { // last element
        inRoutes = false;
      }
      push_back( route );
    }
  }

public:
  const TCompsRoutes& get( bool pr_tranzit_routes, int point_id, TQuery &Qry ) {
    clear();
    Qry.Clear();
    Qry.SQLText =
    "SELECT point_num,first_point,pr_tranzit,"
    "       airline,flt_no,suffix,airp,scd_out,bort,craft,pr_reg "
    " FROM points "
    " WHERE point_id=:point_id AND pr_del!=-1";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    if ( Qry.Eof )
      return *this;
    int point_num = Qry.FieldAsInteger( "point_num" );
    int first_point = Qry.FieldIsNULL("first_point")?ASTRA::NoExists:Qry.FieldAsInteger("first_point");
    bool pr_tranzit = Qry.FieldAsInteger( "pr_tranzit" ) != 0;
    CompRoute currroute;
    currroute.point_id = point_id;
    currroute.airline = Qry.FieldAsString( "airline" );
    currroute.flt_no = Qry.FieldAsInteger( "flt_no" );
    currroute.airp = Qry.FieldAsString( "airp" );
    currroute.bort = Qry.FieldAsString( "bort" );
    currroute.craft = Qry.FieldAsString( "craft" );
    currroute.pr_reg = ( Qry.FieldAsInteger( "pr_reg" ) == 1 );
    TTripInfo fltInfo;
    fltInfo.Init(Qry);
    currroute.auto_comp_chg = isAutoCompChg( point_id ) || LibraComps::isLibraMode( fltInfo );
    currroute.inRoutes = true;
    TTripRoute routesB, routesA;
    if ( pr_tranzit ) {
      routesB.GetRouteBefore( ASTRA::NoExists,
                              point_id,
                              point_num,
                              first_point,
                              pr_tranzit,
                              trtNotCurrent,
                              trtNotCancelled );
    }
    routesA.GetRouteAfter( ASTRA::NoExists,
                           point_id,
                           point_num,
                           first_point,
                           pr_tranzit,
                           trtNotCurrent,
                           trtNotCancelled );
    if ( routesA.empty() ) { // ३� �� �ਫ��
      push_back( currroute );
      LogTrace( TRACE5 ) << __func__ << ": routesA.empty()";
      return *this;
    }
    if ( pr_tranzit_routes ) { //������� ���������� ��� �ᥣ� �������
      push_routes( currroute, routesB, true, Qry );
    }
    push_back( currroute );
    if ( pr_tranzit_routes ) { //������� ���������� ��� �ᥣ� �������
      push_routes( currroute, routesA, false, Qry );
    }
    for ( const auto& i : *this ) {
      LogTrace( TRACE5 ) << __func__ << ": point_id=" << i.point_id << ",inRoutes=" << i.inRoutes << ",pr_reg=" << i.pr_reg;
    }
    return *this;
  }
  void calc_diffcomp_alarm( ) {
    TCompsRoutes::iterator iprior = end();
    for ( TCompsRoutes::iterator i=begin(); i!=end(); i++ ) {
      i->pr_alarm = false;
      if ( !i->pr_reg ) {
        continue;
      }
      if ( iprior != end() && i != end()-1 ) { //����� ����� �ࠢ������ ⮫쪮 crc32 � ��⮬ ���� �� ���� ��� ��ᠤ�� ���ᠦ�஢ � �����஢��� ⨯ ����
        int crc_comp1 = SALONS2::CompCheckSum::keyFromDB( iprior->point_id ).base_crc32;
        int crc_comp2 = SALONS2::CompCheckSum::keyFromDB( i->point_id ).base_crc32;
        if ( !CompRouteinRoutes( *iprior, *i ) ||
             ( crc_comp1 != 0 &&
               crc_comp2 != 0 &&
               crc_comp1 != crc_comp2 ) ) {
           i->pr_alarm = true;
        }
      }
      iprior = i;
    }
  }
  void check_diffcomp_alarm( ) {
    calc_diffcomp_alarm( );
    for ( const auto& i : *this ) {
      set_alarm( i.point_id, Alarm::DiffComps, i.pr_alarm );
    }
  }
  static void check_diffcomp_alarm( int point_id ) {
    LogTrace(TRACE5) << __func__;
    TCompsRoutes routes;
    TQuery Qry(&OraSession);
    routes.get( true, point_id, Qry );
    routes.check_diffcomp_alarm();
  }
};

void check_diffcomp_alarm( int point_id ) {
  TCompsRoutes::check_diffcomp_alarm( point_id );
}

class TCounter {
public:
  int f, c, y;
  TCounter() {
    f = 0;
    c = 0;
    y = 0;
  }
  bool operator == ( const TCounter &value ) const {
    return ( f == value.f &&
             c == value.c &&
             y == value.y );
  }
};

class TCounters: public std::map<int,TCounter> {
private:
  int point_id;
public:
  TCounters& init( int point_id ) {
    clear();
    point_id = -1;
    emplace( point_id, TCounter() );
    return *this;
  }
  void init( std::vector<int> & point_ids ) {
    clear();
    point_id = -1;
    for ( const auto& i : point_ids ) {
      emplace( i, TCounter() );
    }
  }
  int pointId() {
    return point_id;
  }
  void setPointId( int apoint_id ) {
    point_id = apoint_id;
  }
  virtual void get( TQuery &Qry ) = 0;
  virtual ~TCounters(){}
};

class TCrsCounters: public TCounters {
public:
  virtual void get( TQuery &Qry  ) {
    Qry.Clear();
    Qry.SQLText =
    "SELECT NVL( MAX( DECODE( class, '�', cfg, 0 ) ), 0 ) f, "
    "       NVL( MAX( DECODE( class, '�', cfg, 0 ) ), 0 ) c, "
    "       NVL( MAX( DECODE( class, '�', cfg, 0 ) ), 0 ) y "
    " FROM crs_data,tlg_binding,points "
    " WHERE crs_data.point_id=tlg_binding.point_id_tlg AND "
    "       points.point_id=:point_id AND "
    "       point_id_spp=:point_id AND system='CRS' AND airp_arv=points.airp ";
    Qry.DeclareVariable( "point_id", otInteger );

    for ( auto& i : *this ) {
      Qry.SetVariable( "point_id", i.first );
      Qry.Execute();
      i.second.f = Qry.FieldAsInteger( "f" );
      i.second.c = Qry.FieldAsInteger( "c" );
      i.second.y = Qry.FieldAsInteger( "y" );
      if ( pointId() < 0 ||
            (*this)[ pointId() ].f + (*this)[ pointId() ].c + (*this)[ pointId() ].y <
           i.second.f + i.second.c + i.second.y ) {
        setPointId( i.first );
      }
    }
  }
  virtual ~TCrsCounters(){}
};


class TCountersCounters: public TCounters {
public:
  virtual void get( TQuery &Qry  ) {
    Qry.Clear();
    Qry.SQLText =
    "SELECT airp_arv,class, "
    "       0 AS priority, "
    "       crs_ok + crs_tranzit AS c "
    " FROM crs_counters "
    "WHERE point_dep=:point_id "
    "UNION "
    "SELECT airp_arv,class,1,resa + tranzit "
    " FROM trip_data "
    "WHERE point_id=:point_id "
    "ORDER BY priority DESC ";
    Qry.DeclareVariable( "point_id", otInteger );

    std::string vclass;
    for ( auto &i : *this ) {
      Qry.SetVariable( "point_id", i.first );
      Qry.Execute();
      int priority = -1;
      for ( ; !Qry.Eof; Qry.Next() ) {
        if ( Qry.FieldAsInteger( "c" ) > 0 ) {
          priority = Qry.FieldAsInteger( "priority" );
          if ( priority != Qry.FieldAsInteger( "priority" ) ) {
            break;
          }
          vclass = Qry.FieldAsString( "class" );
          if ( vclass == "�" ) i.second.f += Qry.FieldAsInteger( "c" );
          if ( vclass == "�" ) i.second.c += Qry.FieldAsInteger( "c" );
          if ( vclass == "�" ) i.second.y += Qry.FieldAsInteger( "c" );
        }
      }
      if ( pointId() < 0 ||
           (*this)[ pointId() ].f + (*this)[ pointId() ].c + (*this)[ pointId() ].y <
           i.second.f + i.second.c + i.second.y ) {
        setPointId( i.first );
      }
    }
  }
  virtual ~TCountersCounters(){}
};

class TSeasonCounters: public TCounters {
public:
  virtual void get( TQuery &Qry  ) {
    Qry.Clear();
    Qry.SQLText =
    "SELECT ABS(f) f, ABS(c) c, ABS(y) y FROM trip_sets WHERE point_id=:point_id";
    Qry.DeclareVariable( "point_id", otInteger );
    TCounter pcounter;
    for ( auto &i : *this ) {
      Qry.SetVariable( "point_id", i.first );
      Qry.Execute();
      if ( !Qry.Eof ) {
        TCounter counter;
        counter.f = Qry.FieldAsInteger( "f" );
        counter.c = Qry.FieldAsInteger( "c" );
        counter.y = Qry.FieldAsInteger( "y" );
        if ( pcounter ==  counter ) {
          continue;
        }
        pcounter = counter;
        i.second = counter;
        if ( pointId() < 0 ||
             (*this)[ pointId() ].f + (*this)[ pointId() ].c + (*this)[pointId() ].y <
             (*this)[ i.first ].f + (*this)[ i.first ].c + (*this)[ i.first ].y ) {
          setPointId( i.first );
        }
      }
    }
  }
  virtual ~TSeasonCounters(){}
};

struct TTripClass {
  int block;
  int protect;
  int cfg;
  TTripClass() {
    block = 0;
    protect = 0;
    cfg = 0;
  }
  bool operator == ( const TTripClass &value ) const {
    return ( block == value.block &&
             protect == value.protect &&
             cfg == value.cfg );
  }
};

typedef std::map<std::string,TTripClass> mapTripClasses_t;

class TTripClasses: private mapTripClasses_t {
  private:
    int point_id;
  public:
    enum TReadElems { reAll, reCfg };
    enum TReadType { rtFlight, rtBase };
    TTripClasses( int vpoint_id ) {
      point_id = vpoint_id;
  }
  void deleteCfg( ) {
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
      "DELETE trip_classes WHERE point_id = :point_id ";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
  }
  void read( TReadElems elemsType = reAll ) {
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
      "SELECT class, cfg, block, prot FROM trip_classes "
      " WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    clear();
    for ( ; !Qry.Eof; Qry.Next() ) {
      TTripClass cl;
      cl.cfg = Qry.FieldAsInteger( "cfg" );
      if ( elemsType == reAll ) {
        cl.block = Qry.FieldAsInteger( "block" );
        cl.protect = Qry.FieldAsInteger( "prot" );
      }
      emplace( Qry.FieldAsString( "class" ), cl );
    }
  }
  bool operator == ( const TTripClasses &trip_classes ) const {
    for ( TTripClasses::const_iterator i=begin(); i!=end(); i++ ) {
      TTripClasses::const_iterator j = trip_classes.find( i->first );
      if ( j == trip_classes.end() ||
           !(j->second == i->second) ) {
        return false;
      }
    }
    for ( TTripClasses::const_iterator i=trip_classes.begin(); i!=trip_classes.end(); i++ ) {
      TTripClasses::const_iterator j = find( i->first );
      if ( j == end() ||
           !(j->second == i->second) ) {
        return false;
      }
    }
    return true;
  }
  void readLayerCfg( ) {
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
    "SELECT t.num, t.x, t.y, t.class, t.elem_type, r.layer_type"
    " FROM trip_comp_ranges r, trip_comp_elems t "
    "WHERE r.point_id=:point_id AND "
    "      t.point_id=r.point_id AND "
    "      t.num=r.num AND "
    "      t.x=r.x AND "
    "      t.y=r.y AND "
    "      r.layer_type in (:blockcent_layer,:disable_layer,:protect_layer)";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "blockcent_layer", otString, EncodeCompLayerType(ASTRA::cltBlockCent) );
    Qry.CreateVariable( "disable_layer", otString, EncodeCompLayerType(ASTRA::cltDisable) );
    Qry.CreateVariable( "protect_layer", otString, EncodeCompLayerType(ASTRA::cltProtect) );
    Qry.Execute();
    int idx_num = Qry.FieldIndex( "num" );
    int idx_x = Qry.FieldIndex( "x" );
    int idx_y = Qry.FieldIndex( "y" );
    int idx_elem_type = Qry.FieldIndex( "elem_type" );
    int idx_class = Qry.FieldIndex( "class" );
    int idx_layer_type = Qry.FieldIndex( "layer_type" );
    std::map<std::string,std::vector<SALONS2::TPlace> > seats;
    for ( ; !Qry.Eof; Qry.Next() ) {
      if ( !BASIC_SALONS::TCompElemTypes::Instance()->isSeat( Qry.FieldAsString( idx_elem_type ) ) ) {
        continue;
      }
      SALONS2::TPlace seat;
      seat.num = Qry.FieldAsInteger( idx_num );
      seat.x = Qry.FieldAsInteger( idx_x );
      seat.y = Qry.FieldAsInteger( idx_y );
      seat.clname = Qry.FieldAsString( idx_class );
      SALONS2::TLayerSeat seatLayer;
      seatLayer.point_id = point_id;
      seatLayer.layer_type = DecodeCompLayerType( Qry.FieldAsString( idx_layer_type ) );
      std::vector<SALONS2::TPlace>::iterator iseat;
      for ( iseat=seats[ seat.clname ].begin(); iseat!=seats[ seat.clname ].end(); iseat++ ) {
        if ( iseat->num == seat.num &&
             iseat->x == seat.x &&
             iseat->y == seat.y ) {
          break;
        }
      }
      SALONS2::TLayerPrioritySeat layerPrioritySeat( seatLayer,
                                                    BASIC_SALONS::TCompLayerTypes::Instance()->priority( BASIC_SALONS::TCompLayerTypes::LayerKey("",seatLayer.layer_type),
                                                                                                         BASIC_SALONS::TCompLayerTypes::Enum::ignoreAirline ) );
      if ( iseat != seats[ seat.clname ].end() ) {
        iseat->AddLayer( point_id, layerPrioritySeat );
      }
      else {
        seat.AddLayer( point_id, layerPrioritySeat );
        seats[ seat.clname ].push_back( seat );
      }
    }
    for ( const auto& iclass : seats ) {
      for ( const auto& iseat : iclass.second ) {
        if ( iseat.getCurrLayer( point_id ).layerType() == ASTRA::cltBlockCent ||
             iseat.getCurrLayer( point_id ).layerType() == ASTRA::cltDisable ) {
          TTripClasses::iterator tc = find( iclass.first );
          if ( tc == end() ) {
            tc = emplace( iclass.first, TTripClass() ).first;
          }
          tc->second.block++;
        }
        if ( iseat.getCurrLayer( point_id ).layerType() == ASTRA::cltProtect ) {
          TTripClasses::iterator tc = find( iclass.first );
          if ( tc == end() ) {
            tc = emplace( iclass.first, TTripClass() ).first;
          }
          tc->second.protect++;
        }
      }
    }
  }
  void setCfg( const mapTripClasses_t &values ) {
    clear();
    insert( values.begin(), values.end() );
  }
  void readClassCfg( int id, TReadType readType ) {
    TQuery Qry(&OraSession);
    Qry.Clear();
    if ( readType == rtFlight ) {
      Qry.SQLText =
        "SELECT class, elem_type, COUNT( elem_type ) cfg "
        " FROM trip_comp_elems "
        "WHERE trip_comp_elems.point_id=:id AND "
        "      class IS NOT NULL "
        "GROUP BY class, elem_type";
    }
    else {
      Qry.SQLText =
        "SELECT class, elem_type, COUNT( elem_type ) cfg "
        " FROM comp_elems "
        "WHERE comp_elems.comp_id=:id AND "
        "      class IS NOT NULL "
        "GROUP BY class, elem_type";
    }
    Qry.CreateVariable( "id", otInteger, id );
    Qry.Execute();
    for ( ; !Qry.Eof; Qry.Next() ) {
      if ( !BASIC_SALONS::TCompElemTypes::Instance()->isSeat( Qry.FieldAsString( "elem_type" ) ) ) {
        continue;
      }
      TTripClasses::iterator tc = find( Qry.FieldAsString( "class" ) );
      if ( tc == end() ) {
        tc = emplace( Qry.FieldAsString( "class" ), TTripClass() ).first;
      }
      tc->second.cfg += Qry.FieldAsInteger( "cfg" );
    }
  }
  void writeCfg( ) {
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
      "INSERT INTO trip_classes(point_id,class,cfg,block,prot) "
      " VALUES(:point_id,:class,:cfg,:block,:prot)";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.DeclareVariable( "class", otString );
    Qry.DeclareVariable( "cfg", otInteger );
    Qry.DeclareVariable( "block", otInteger );
    Qry.DeclareVariable( "prot", otInteger );
    for( const auto& iclass : *this ) {
      Qry.SetVariable( "class", iclass.first );
      Qry.SetVariable( "cfg", iclass.second.cfg );
      Qry.SetVariable( "block", iclass.second.block );
      Qry.SetVariable( "prot", iclass.second.protect );
      Qry.Execute();
    }

    CheckIn::TCounters().recount(point_id, CheckIn::TCounters::Total, __FUNCTION__);
  }
  void processBaseCompCfg( int id ) {
    deleteCfg( );
    readClassCfg( id, rtBase );
    writeCfg( );
  }
  void processSalonsCfg( ) {
    deleteCfg( );
    readLayerCfg( );
    readClassCfg( point_id, rtFlight );
    writeCfg( );
  }
 std:: string toString() {
    std::string res = "cfg:";
    for ( const auto& i : *this ) {
      res = res + " class=" + i.first + ",cfg=" + IntToString( i.second.cfg ) +
                  ",protect=" + IntToString( i.second.protect ) +
                  ",block=" + IntToString( i.second.block );
    }
    return res;
  }
};

void setFlightClasses( int point_id ) {
  TTripClasses tripClasses( point_id );
  tripClasses.processSalonsCfg();
}

bool ComponFinder::isExistsCraft( const std::string& craft, TQuery& Qry ) {
  Qry.Clear();
  Qry.SQLText =
  "SELECT comp_id FROM comps "
  " WHERE craft=:craft AND rownum < 2";
  Qry.CreateVariable( "craft", otString, craft );
  Qry.Execute();
  return !Qry.Eof;
}

int ComponFinder::GetCompIdFromBort( const std::string& craft,
                                     const std::string& bort,
                                     TQuery& Qry ) {
 Qry.Clear();
 Qry.SQLText =
   "SELECT comp_id FROM comps "
   "WHERE craft=:craft AND bort=:bort";
 Qry.CreateVariable( "craft", otString, craft );
 Qry.CreateVariable( "bort", otString, bort );
 Qry.Execute();
 return Qry.Eof?ASTRA::NoExists:Qry.FieldAsInteger( "comp_id" );
}


void ComponLibraFinder::AstraSearchResult::getLibraCompStatus( const std::string& airline,
                                                               const std::string& bort,
                                                               TQuery& Qry,
                                                               const int& comp_id )
{
  //!!! ��� �஢�ન �� ⨯ ��, ⮫쪮 ����
  plan_id = ASTRA::NoExists;
  conf_id = ASTRA::NoExists;
  ReadFromAHMCompId( comp_id, Qry ); //���⠫� ���䨣, ���ண� ����� � �� ����
  int p_id = ComponLibraFinder::getPlanId( bort, Qry );
  if ( p_id == ASTRA::NoExists ) {
    clear();
    return;
  }
  if ( plan_id == ASTRA::NoExists ||
       plan_id != p_id ) {
    plan_id = p_id;
    conf_id = ASTRA::NoExists;
  }
  std::vector<int> configs;
  ComponLibraFinder::GetLibraConfigs( airline, bort, conf_id, configs, Qry );
  for ( const auto& vconf_id : configs )  {
    evtStatus = TEventLibraStatus::evChange;
    if ( vconf_id == conf_id ) {
      tst();
      evtStatus = isOk()?TEventLibraStatus::evNoChange:TEventLibraStatus::evChange;
      return;
    }
  }
  return;
}

int ComponLibraFinder::getPlanId( const std::string& bort, TQuery& Qry ) {
  if ( bort.empty() ) {
      return ASTRA::NoExists;
  }
  //!!! ��� �஢�ન �� ⨯ ��, ⮫쪮 ����. ����� �롨ࠥ��� ���������� �� �ᯨᠭ�� ���⮬� ⠪�� �᫮��� �� ��� + ���஢��. �� ��������!!!
  Qry.Clear();
  Qry.SQLText =
  "SELECT brw.S_L_ADV_ID AS PLAN_ID "
  "  FROM WB_REF_AIRCO_WS_BORTS b join WB_REF_WS_AIR_REG_WGT brw "
  "    on b.bort_num=:bort AND "
  "       brw.id_bort=b.id AND "
  "       brw.date_from<=system.UTCSYSDATE "
  " ORDER BY brw.date_from desc";
  Qry.CreateVariable( "bort", otString, bort );
  Qry.Execute();
  return Qry.Eof?ASTRA::NoExists:Qry.FieldAsInteger( "plan_id" );
}

int ComponLibraFinder::GetLibraConfigs( const std::string& airline, const std::string& bort,
                                        int conf_id, std::vector<int>& configs, TQuery& Qry )
{
  configs.clear();
  Qry.Clear();
  int plan_id = ComponLibraFinder::getPlanId( bort, Qry );
  if ( plan_id == ASTRA::NoExists ) {
    return plan_id;
  }
  Qry.Clear();
  Qry.SQLText = getConfigSQLText( conf_id != ASTRA::NoExists );
  Qry.CreateVariable( "plan_id", otInteger, plan_id );
  if ( conf_id != ASTRA::NoExists ) {
    Qry.CreateVariable( "conf_id", otInteger, conf_id );
  }
  Qry.CreateVariable( "airline", otString, airline ); //��� �஢�ન �������� ����ᮢ
  Qry.CreateVariable( "bort", otString, bort );
  Qry.Execute();
  for ( ; !Qry.Eof; Qry.Next() ) {
    if ( Qry.FieldAsInteger( "invalid_class" ) ) {
      continue;
    }
    LogTrace(TRACE5) << "plan_id=" << plan_id << " conf_id=" << Qry.FieldAsInteger( "comp_id" );
    configs.emplace_back( Qry.FieldAsInteger( "comp_id" ) );
  }
  return plan_id;
}

std::string ComponLibraFinder::getConvertClassSQLText() {
  std::string res =
    " SELECT libra_class, class FROM "
    "  ( "
    "     SELECT libra_class, class, "
    "            ROW_NUMBER() OVER (PARTITION BY libra_class ORDER BY priority DESC ) rank "
    " FROM "
    " ( "
    "  SELECT libra_class, code_lat as class, MAX(DECODE(bort,NULL,1,:bort,2)) priority "
    "   FROM libra_classes, classes "
    "   WHERE ( bort=:bort OR bort IS NULL) AND airline=:airline AND libra_classes.class=classes.code "
    "  GROUP BY libra_class, code_lat "
    "  UNION "
    "  SELECT code_lat, code_lat, 0 "
    " FROM classes "
    " ) "
    " GROUP BY libra_class, class, priority "
    " ) WHERE rank=1 ";
  return res;
}

std::string ComponLibraFinder::getConfigSQLText( bool withConfId ) {
  std::string res =
    " SELECT a.id as comp_id, "
    "      NVL( SUM( DECODE( ls.class, 'F', tt.num_of_seats, 0 )), 0 ) AS F,"
    "      NVL( SUM( DECODE( ls.class, 'C', tt.num_of_seats, 0 )), 0 ) AS C,"
    "      NVL( SUM( DECODE( ls.class, 'Y', tt.num_of_seats, 0 )), 0 ) AS Y, "
    "      NVL( SUM( DECODE( ls.class, NULL,tt.num_of_seats,0)), 0 ) AS invalid_class "
    " FROM WB_REF_WS_AIR_S_L_C_ADV a join WB_REF_WS_AIR_S_L_C_IDN i "
    "  on a.adv_id=:plan_id AND "
    "     i.id=a.idn join WB_REF_WS_AIR_SL_CAI_TT tt "
    "  on tt.adv_id=a.id ";
  if ( withConfId ) {
    res += " AND a.id=:conf_id ";
  }
  res +=
    " left join "
    " ( ";
  res += getConvertClassSQLText();
  res +=
    ") ls "
    " on tt.class_code=ls.libra_class "
    " GROUP BY a.id ";
  LogTrace(TRACE5) << res;
  return res;
}

int ComponLibraFinder::getConfig( int planId,
                                  const std::string& airline, const std::string& bort,
                                  int f, int c, int y,
                                  bool pr_ignore_fcy, TQuery& Qry ) {
  LogTrace(TRACE5) << __func__ << " planId=" << planId;
  if ( planId == ASTRA::NoExists ) {
      return planId;
    }
  if ( f + c + y == 0 ) {
      return ASTRA::NoExists;
    }
  if ( pr_ignore_fcy ) {
      f = 0;
      c = 0;
      y = 0;
    }
  std::map<int,TComp,std::less<int> > CompMap;
  int idx;

  std::string sql =
    " SELECT * FROM ( ";
  sql += getConfigSQLText( false );
  sql +=
    ") WHERE invalid_class = 0 AND "
    "        F - :vf >= 0 AND "
    "        C - :vc >= 0 AND "
    "        Y - :vy >= 0 AND "
    "        F < 1000 AND C < 1000 AND Y < 1000 "
    " ORDER BY comp_id ";

  Qry.Clear();
  Qry.SQLText = sql;
  Qry.CreateVariable( "plan_id", otInteger, planId );
  Qry.CreateVariable( "airline", otString, airline );
  Qry.CreateVariable( "bort", otString, bort );
  Qry.CreateVariable( "vf", otString, f );
  Qry.CreateVariable( "vc", otString, c );
  Qry.CreateVariable( "vy", otString, y );
  Qry.Execute();
  for ( ;!Qry.Eof; Qry.Next() ) {
    idx = 0; // ����� ᮢ������ ����+������������ OR ��ய���
    // ᮢ������� �� ���-�� ���� ��� ������� �����
    if ( SIGN( Qry.FieldAsInteger( "f" ) ) == SIGN( f ) &&
         SIGN( Qry.FieldAsInteger( "c" ) ) == SIGN( c ) &&
         SIGN( Qry.FieldAsInteger( "y" ) ) == SIGN( y ) &&
         Qry.FieldAsInteger( "f" ) >= f &&
         Qry.FieldAsInteger( "c" ) >= c &&
         Qry.FieldAsInteger( "y" ) >= y &&
         CompMap[ idx ].sum > Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y ) {
      CompMap[ idx ].sum = Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y;
      CompMap[ idx ].comp_id = Qry.FieldAsInteger( "comp_id" );
    }

    idx += 3;
    // ᮢ������� �� ����ᠬ � �������� ���� >= ��饥 ���-�� ����
    if ( SIGN( Qry.FieldAsInteger( "f" ) ) == SIGN( f ) &&
         SIGN( Qry.FieldAsInteger( "c" ) ) == SIGN( c ) &&
         SIGN( Qry.FieldAsInteger( "y" ) ) == SIGN( y ) &&
         CompMap[ idx ].sum > Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y ) {
      CompMap[ idx ].sum = Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y;
      CompMap[ idx ].comp_id = Qry.FieldAsInteger( "comp_id" );
    }
    // ᮢ������� �� �������� ���� >= ��饥 ���-�� ����
    idx += 3;
    if ( CompMap[ idx ].sum > Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y ) {
      CompMap[ idx ].sum = Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y;
      CompMap[ idx ].comp_id = Qry.FieldAsInteger( "comp_id" );
    }
  }
  if ( !CompMap.size() ) {
    return ASTRA::NoExists;
  }
  else {
    ProgTrace( TRACE5, "GetCompId:  CompMap begin (idx=%d,comp_id=%d) sum=%d", CompMap.begin()->first, CompMap.begin()->second.comp_id, CompMap.begin()->second.sum );
    return CompMap.begin()->second.comp_id; // ��������� ����� - ���஢�� ���� �� �����⠭��
  }
}

void ComponLibraFinder::AstraSearchResult::Write( TQuery& Qry ) {
  Qry.Clear();
  Qry.SQLText =
    "BEGIN "
    "UPDATE libra_comps SET plan_id=:plan_id, conf_id=:conf_id, time_change=system.UTCSYSDATE "
    " WHERE comp_id=:comp_id; "
    "  IF SQL%NOTFOUND THEN "
    " INSERT INTO libra_comps(plan_id,conf_id,comp_id,time_create,time_change) "
    "  SELECT :plan_id,:conf_id,:comp_id,system.UTCSYSDATE,system.UTCSYSDATE from dual;"
    "  END IF; "
    "END;";
  Qry.CreateVariable( "plan_id", otInteger, plan_id );
  Qry.CreateVariable( "conf_id", otInteger, conf_id );
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  LogTrace(TRACE5) << __func__ << " plan_id=" << plan_id << " conf_id=" << conf_id << " comp_id=" << comp_id;
  Qry.Execute();
}

void ComponLibraFinder::AstraSearchResult::ReadFromAHMIds( int plan_id, int conf_id, TQuery& Qry ) {
  this->plan_id = plan_id;
  this->conf_id = conf_id;
  Qry.Clear();
  Qry.SQLText =
    "SELECT l.time_create, l.time_change, c.comp_id FROM libra_comps l, comps c"
    " WHERE l.comp_id=c.comp_id AND "
    "       l.plan_id=:plan_id AND "
    "       l.conf_id=:conf_id ";
  Qry.CreateVariable( "plan_id", otInteger, plan_id );
  Qry.CreateVariable( "conf_id", otInteger, conf_id );
  Qry.Execute();
  if ( Qry.Eof ) { //�� ��諨 ����������
    LogTrace(TRACE5) << __func__ << " plan_id=" << plan_id << ",conf_id=" << conf_id << " compon not found";
    comp_id = ASTRA::NoExists;
  }
  else {
    comp_id = Qry.FieldAsInteger( "comp_id" );
    time_create = Qry.FieldAsDateTime( "time_create" );
    time_change = Qry.FieldIsNULL( "time_change" )?ASTRA::NoExists:Qry.FieldAsDateTime( "time_change" );
  }
}

void ComponLibraFinder::AstraSearchResult::ReadFromAHMCompId( int comp_id, TQuery& Qry ) {
  clear();
  this->comp_id = comp_id;
  if ( comp_id == ASTRA::NoExists ) {
    return;
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT l.time_create, l.time_change, l.plan_id, l.conf_id FROM libra_comps l, comps c"
    " WHERE l.comp_id=c.comp_id AND "
    "       c.comp_id=:comp_id";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.Execute();
  if ( Qry.Eof ) { //�� ��諨 ����������
    LogTrace(TRACE5) << __func__ << " comp_id=" << comp_id << ",conf_id=" << conf_id << " compon not found";
  }
  else {
    plan_id = Qry.FieldAsInteger( "plan_id" );
    conf_id = Qry.FieldAsInteger( "conf_id" );
    time_create = Qry.FieldAsDateTime( "time_create" );
    time_change = Qry.FieldIsNULL( "time_change" )?ASTRA::NoExists:Qry.FieldAsDateTime( "time_change" );
  }
}

bool LibraComps::isLibraComps( int id, bool isComps, BASIC::date_time::TDateTime& time_create ) {
  TQuery Qry(&OraSession);
  if ( !isComps ) {
    Qry.SQLText =
      "SELECT comp_id FROM trip_sets WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, id );
    Qry.Execute();
    if ( Qry.Eof ) {
      throw AstraLocale::UserException( "MSG.FLIGHT.NOT_FOUND.REFRESH_DATA" );
    }
    if ( Qry.FieldIsNULL( "comp_id" ) ) {
      time_create = ASTRA::NoExists;
      return false;
    }
    id = Qry.FieldAsInteger( "comp_id" );
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT time_create FROM libra_comps WHERE comp_id=:comp_id";
  Qry.CreateVariable( "comp_id", otInteger, id );
  Qry.Execute();
  time_create = Qry.Eof?ASTRA::NoExists:Qry.FieldAsDateTime( "time_create" );
  LogTrace(TRACE5) << __func__ << " time_create=" << time_create << " SELECT " << !Qry.Eof;
  return !Qry.Eof;
}

bool LibraComps::isLibraComps( int id, CompType compType ) {
  BASIC::date_time::TDateTime time_create;
  return LibraComps::isLibraComps( id, compType == LibraComps::CompType::ctComp, time_create );
}

bool LibraComps::isLibraMode( const TTripInfo &info  ) {
  return GetTripSets( tsLIBRACent, info );
}

ComponLibraFinder::AstraSearchResult ComponLibraFinder::checkChangeAHMFromCompId( int comp_id, TQuery& Qry ) {
  AstraSearchResult res;
  res.ReadFromAHMCompId( comp_id, Qry );
  return res;
}

ComponLibraFinder::AstraSearchResult ComponLibraFinder::checkChangeAHMFromAHMIds( int plan_id, int conf_id, TQuery& Qry ) {
  AstraSearchResult res;
  res.ReadFromAHMIds( plan_id, conf_id, Qry );
  return res;
}

void signalChangesComp( TQuery &Qry, int plan_id, int conf_id ) {
  Qry.Clear();
  std::string sql =
    "UPDATE libra_comps SET time_change=NULL "
    " WHERE plan_id = :plan_id ";
  if ( conf_id != ASTRA::NoExists ) {
    sql += " AND conf_id = :conf_id ";
  }
  Qry.SQLText = sql;
  Qry.CreateVariable( "plan_id", otInteger, plan_id );
  if ( conf_id != ASTRA::NoExists ) {
    Qry.CreateVariable( "conf_id", otInteger, conf_id );
  }
  Qry.Execute();
  OraSession.Commit();
  sql =
    "SELECT t.point_id, l.comp_id, l.plan_id, l.conf_id "
    " FROM libra_comps l "
    " JOIN trip_sets t ON l.comp_id=t.comp_id "
    "      AND plan_id = :plan_id ";
  if ( conf_id != ASTRA::NoExists ) {
    sql += " AND conf_id = :conf_id ";
  }
  sql += " ORDER BY plan_id, conf_id ";
  Qry.SQLText = sql;
  Qry.CreateVariable( "plan_id", otInteger, plan_id );
  if ( conf_id != ASTRA::NoExists ) {
    Qry.CreateVariable( "conf_id", otInteger, conf_id );
  }
  Qry.Execute();
  int prior_plan_id = ASTRA::NoExists, prior_conf_id = ASTRA::NoExists;
  ComponCreator::ComponSetter::TStatus prior_status = ComponCreator::ComponSetter::TStatus::NotFound;
  for ( ; !Qry.Eof; Qry.Next() ) {
    ComponCreator::ComponSetter componSetter( Qry.FieldAsInteger( "point_id" ), ComponCreator::ComponSetter::TModeCreate::mReCreate );
    if ( !componSetter.isLibraMode()  ) {
      continue;
    }
    if ( prior_plan_id == Qry.FieldAsInteger( "plan_id" ) &&
         prior_conf_id == Qry.FieldAsInteger( "conf_id" ) &&
         prior_status == ComponCreator::ComponSetter::TStatus::NotCrafts ) {
      // �� ᬮ��� ���� � ᮧ���� ������� ����������
      continue;
    }
    prior_status = componSetter.SetCraft(true);
    prior_plan_id = Qry.FieldAsInteger( "plan_id" );
    prior_conf_id = Qry.FieldAsInteger( "conf_id" );
    OraSession.Commit();
  }
}


/*
 *
 CREATE table LIBRA_COMPS (
 PLAN_ID NUMBER(9) NOT NULL,
 CONF_ID NUMBER(9) NOT NULL,
 COMP_ID NUMBER(9) NOT NULL,
 time_create DATE NOT NULL
);
alter table LIBRA_COMPS add constraint LIBRA_COMPS__PK primary key(PLAN_ID,CONF_ID);
alter table LIBRA_COMPS add constraint LIBRA_COMPS__COMPS__FK foreign key (COMP_ID) references COMPS(COMP_ID);
CREATE INDEX LIBRA_COMPS_COMP_ID__IDX ON LIBRA_COMPS
(
       comp_id                         ASC
);
MSG.CHANGE_NOT_AVAILABLE_LIBRA_COMPS
MSG.CHANGE_SECTION_NOT_AVAILABLE_LIBRA_COMPS

ARCH delete libra_comps �� �������� � trip_sets

*/

int ComponFinder::GetCompId( const std::string& craft, const std::string& bort, const std::string& airline,
                             const std::vector<std::string>& airps,  int f, int c, int y, bool pr_ignore_fcy,
                             TQuery& Qry ) {
  if ( f + c + y == 0 ) {
    return ASTRA::NoExists;
  }
  if ( pr_ignore_fcy ) {
    f = 0;
    c = 0;
    y = 0;
  }
  std::map<int,TComp,std::less<int> > CompMap;
  int idx;
  Qry.Clear();
  Qry.SQLText =
  "SELECT * FROM "
  "( SELECT COMPS.COMP_ID, BORT, AIRLINE, AIRP, "
  "         NVL( SUM( DECODE( CLASS, '�', CFG, 0 )), 0 ) AS F, "
  "         NVL( SUM( DECODE( CLASS, '�', CFG, 0 )), 0 ) AS C, "
  "         NVL( SUM( DECODE( CLASS, '�', cfg, 0 )), 0 ) AS Y "
  "   FROM COMPS, COMP_CLASSES "
  "  WHERE COMP_CLASSES.COMP_ID = COMPS.COMP_ID AND COMPS.CRAFT = :craft "
  "  GROUP BY COMPS.COMP_ID, BORT, AIRLINE, AIRP ) "
  "WHERE  f - :vf >= 0 AND "
  "       c - :vc >= 0 AND "
  "       y - :vy >= 0 AND "
  "       f < 1000 AND c < 1000 AND y < 1000 "
  " ORDER BY comp_id ";
  Qry.CreateVariable( "craft", otString, craft );
  Qry.CreateVariable( "vf", otString, f );
  Qry.CreateVariable( "vc", otString, c );
  Qry.CreateVariable( "vy", otString, y );
  LogTrace(TRACE5)<< "f=" << f << " c=" << c << " y=" << y << " craft=" << craft;
  Qry.Execute();
  for ( ;!Qry.Eof; Qry.Next() ) {
    std::string comp_airline = Qry.FieldAsString( "airline" );
    std::string comp_airp = Qry.FieldAsString( "airp" );
    bool airline_OR_airp = ( !comp_airline.empty() && airline == comp_airline ) ||
                           ( comp_airline.empty() &&!comp_airp.empty() &&
                             find( airps.begin(), airps.end(), comp_airp ) != airps.end() );
    if ( !bort.empty() && bort == Qry.FieldAsString( "bort" ) && airline_OR_airp ) {
      idx = 0; // ����� ᮢ������ ����+������������ OR ��ய���
    }
    else {
      if ( !bort.empty() && bort == Qry.FieldAsString( "bort" ) ) {
        idx = 1; // ����� ᮢ������ ����
      }
      else {
        if ( airline_OR_airp && !pr_ignore_fcy ) {
          idx = 2; // ����� ᮢ������ ������������ ��� ��ய���
        }
        else {
          continue;
        }
      }
    }
    // ᮢ������� �� ���-�� ���� ��� ������� �����
    if ( SIGN( Qry.FieldAsInteger( "f" ) ) == SIGN( f ) &&
         SIGN( Qry.FieldAsInteger( "c" ) ) == SIGN( c ) &&
         SIGN( Qry.FieldAsInteger( "y" ) ) == SIGN( y ) &&
         Qry.FieldAsInteger( "f" ) >= f &&
         Qry.FieldAsInteger( "c" ) >= c &&
         Qry.FieldAsInteger( "y" ) >= y &&
         CompMap[ idx ].sum > Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y ) {
      CompMap[ idx ].sum = Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y;
      CompMap[ idx ].comp_id = Qry.FieldAsInteger( "comp_id" );
    }

    idx += 3;
    // ᮢ������� �� ����ᠬ � �������� ���� >= ��饥 ���-�� ����
    if ( SIGN( Qry.FieldAsInteger( "f" ) ) == SIGN( f ) &&
         SIGN( Qry.FieldAsInteger( "c" ) ) == SIGN( c ) &&
         SIGN( Qry.FieldAsInteger( "y" ) ) == SIGN( y ) &&
         CompMap[ idx ].sum > Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y ) {
      CompMap[ idx ].sum = Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y;
      CompMap[ idx ].comp_id = Qry.FieldAsInteger( "comp_id" );
    }
    // ᮢ������� �� �������� ���� >= ��饥 ���-�� ����
    idx += 3;
    if ( CompMap[ idx ].sum > Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y ) {
      CompMap[ idx ].sum = Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y;
      CompMap[ idx ].comp_id = Qry.FieldAsInteger( "comp_id" );
    }
  } //end for
  if ( !CompMap.size() ) {
    return ASTRA::NoExists;
  }
  else {
    ProgTrace( TRACE5, "GetCompId:  CompMap begin (idx=%d,comp_id=%d) sum=%d", CompMap.begin()->first, CompMap.begin()->second.comp_id, CompMap.begin()->second.sum );
    return CompMap.begin()->second.comp_id; // ��������� ����� - ���஢�� ���� �� �����⠭��
  }
}

void ComponSetter::Init( int point_id ) {
  TSetsCraftPoints();
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT points.point_id, point_num,first_point,pr_tranzit,bort,flt_no,suffix,airline,craft,scd_out,airp,comp_id "
    " FROM points, trip_sets "
    " WHERE points.point_id=:point_id AND points.point_id=trip_sets.point_id(+)";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof )
    throw AstraLocale::UserException( "MSG.FLIGHT.NOT_FOUND.REFRESH_DATA" );
  fltInfo.Init(Qry);
  fcomp_id = Qry.FieldIsNULL( "comp_id" )?ASTRA::NoExists:Qry.FieldAsInteger( "comp_id" );
}

ComponSetter::TStatus ComponSetter::IntSetCraftFreeSeating( ) { //᢮������ ��ᠤ��, �������� ⮫쪮 ��஢� �����
  LogTrace( TRACE5 ) << __func__ << ", point_id=" << fltInfo.point_id <<",craft=" << fltInfo.craft << ",bort=" << fltInfo.bort;
  TQuery Qry(&OraSession);
  if ( !fltInfo.bort.empty() && !fltInfo.craft.empty() ) {
    int comp_id = ComponCreator::ComponFinder::GetCompIdFromBort( fltInfo.craft, fltInfo.bort, Qry );
    if ( comp_id != ASTRA::NoExists ) {
      TTripClasses trip_classes1( fltInfo.point_id ), trip_classes2( fltInfo.point_id );
      trip_classes1.read( TTripClasses::reCfg ); //���⪠ old cfg from trip_classes
      trip_classes2.readClassCfg( comp_id, TTripClasses::rtBase ); //���⪠ new cfg from comp_elems
      if ( trip_classes1 == trip_classes2 ) {
        LogTrace( TRACE5 ) << __func__ << " NoChanges " << " trip_classes1=" << trip_classes1.toString() << ",trip_classes2=" << trip_classes2.toString();
        return NoChanges;
      }
      trip_classes2.deleteCfg( ); //delete old
      trip_classes2.writeCfg( ); //write new
      LogTrace( TRACE5 ) << __func__ << " Created ";
      LEvntPrms prms;
      TCFG(fltInfo.point_id).param(prms);
      TReqInfo::Instance()->LocaleToLog("EVT.LAYOUT_BASED_ON_BOARD_NUM", prms, ASTRA::evtFlt, fltInfo.point_id);
      return Created;
    }
  }
  //�� ����� ���� ��� ⨯ ��
  TCrsCounters counters;
  counters.init( fltInfo.point_id );
  counters.get( Qry );
  //���� ����� �� �஭� - ��࠭塞 �� ��� ���䨣���� � trip_classes ?
  if ( counters[ fltInfo.point_id ].f + counters[ fltInfo.point_id ].c + counters[ fltInfo.point_id ].y > 0 )
  {
    TTripClasses trip_classes1( fltInfo.point_id ), trip_classes2( fltInfo.point_id );
    trip_classes1.read( TTripClasses::reCfg ); //���⪠ old cfg
    mapTripClasses_t cfgs;
    cfgs[ "�" ].cfg = counters[ fltInfo.point_id ].f;
    cfgs[ "�" ].cfg = counters[ fltInfo.point_id ].c;
    cfgs[ "�" ].cfg = counters[ fltInfo.point_id ].y;
    trip_classes2.setCfg( cfgs );
    if ( trip_classes1 == trip_classes2 ) {
      LogTrace( TRACE5 ) << __func__ << " found PNL cfg, return NoChanges";
      return NoChanges;
    }
    trip_classes2.deleteCfg( ); //delete old
    trip_classes2.writeCfg( ); //write new
    LEvntPrms prms;
    TCFG(fltInfo.point_id).param(prms);
    TReqInfo::Instance()->LocaleToLog("EVT.LAYOUT_BASED_ON_PNL_ADL_DATA", prms, ASTRA::evtFlt, fltInfo.point_id);
    return Created;
  }
  LogTrace( TRACE5 ) << __func__ << " return NotFound";
  return NotFound;
}


int ComponSetter::SearchCompon( bool pr_tranzit_routes,
                                const std::vector<std::string>& airps,
                                TQuery &Qry,
                                int libra_plan_id ) {
  LogTrace(TRACE5) << __func__ << "plan_id=" << libra_plan_id;
  int comp_id = ASTRA::NoExists;
  std::shared_ptr<TCounters> p;
  for ( int step=0; step<=5; step++ ) {
    // �롨ࠥ� ����. ���������� �� CFG �� PNL/ADL ��� ��� 業�஢ �஭�஢����
    if ( step == 0 ) {
      p = std::make_shared<TCrsCounters>();
    }
    if ( step == 2 ) {
      p = std::make_shared<TCountersCounters>();
    }
    if ( step == 4 ) {
      p = std::make_shared<TSeasonCounters>();
    }
    p.get()->init( *this );
    p.get()->get( Qry );
    for ( const auto& i : *p.get() ) {
      if ( step == 0 || step == 2 || step == 4 ) {
        if ( p.get()->pointId() != i.first ||
             i.second.f + i.second.c + i.second.y <= 0 ) {
          continue;
        }
      }
      if ( step == 1 || step == 3 || step == 5 ) {
        if ( !pr_tranzit_routes ||
             p.get()->pointId() == i.first  ||
             i.second.f + i.second.c + i.second.y <= 0 ) {
          continue;
        }
      }
      bool pr_ignore_fcy = ( step == 4 && i.second.f == 0 && i.second.c == 0 && i.second.y == 1 ); // ᥧ���� 㬮�砭�� - ������㥬
      if ( isLibraMode() ) {
        comp_id = ComponCreator::ComponLibraFinder::getConfig( libra_plan_id,
                                                               fltInfo.airline,
                                                               fltInfo.bort,
                                                               i.second.f, i.second.c, i.second.y,
                                                               pr_ignore_fcy, Qry );
      }
      else {
        comp_id = ComponCreator::ComponFinder::GetCompId( fltInfo.craft, fltInfo.bort, fltInfo.airline, airps,
                                                          i.second.f, i.second.c, i.second.y,
                                                          pr_ignore_fcy, Qry );
      }
      LogTrace(TRACE5) << comp_id;
      if ( comp_id != ASTRA::NoExists ) {
        return comp_id;
      }
    }
    if ( comp_id != ASTRA::NoExists ) {
      return comp_id;
    }
  }
  if ( comp_id == ASTRA::NoExists ) {
    if ( !fltInfo.bort.empty() && !fltInfo.craft.empty() ) {
      if ( isLibraMode() ) {
        return ComponCreator::ComponLibraFinder::getConfig( libra_plan_id,
                                                            fltInfo.airline,
                                                            fltInfo.bort,
                                                            0, 0, 1, true, Qry );
      }
      else {
        return ComponCreator::ComponFinder::GetCompId( fltInfo.craft, fltInfo.bort, fltInfo.airline,
                                                       airps, 0, 0, 1, true, Qry );
      }
    }
  }
  return ASTRA::NoExists;
}



const int REM_VIP_F = 1;
const int REM_VIP_C = 1;
const int REM_VIP_Y = 3;

void InitVIP( const CompRoute &route, TQuery &Qry ) {
  TTripInfo info;
  info.airline = route.airline;
  info.flt_no = route.flt_no;
  info.airp = route.airp;
  InitVIP( info, Qry );
}

void InitVIP( const TTripInfo &fltInfo, TQuery &Qry ) {
  if ( !GetTripSets( tsCraftInitVIP, fltInfo ) ) {
    return;
  }
    // ���樠������ - ࠧ��⪠ ᠫ��� �� 㬮�砭�
  Qry.Clear();
  Qry.SQLText =
    "SELECT num, class, MIN( y ) miny, MAX( y ) maxy "
    " FROM trip_comp_elems, comp_elem_types "
    "WHERE trip_comp_elems.elem_type = comp_elem_types.code AND "
    "      comp_elem_types.pr_seat <> 0 AND "
    "      trip_comp_elems.point_id = :point_id "
    "GROUP BY trip_comp_elems.num, trip_comp_elems.class";
  Qry.CreateVariable( "point_id", otInteger, fltInfo.point_id );
  Qry.Execute();

  TQuery QryVIP(&OraSession);
  QryVIP.SQLText =
    "INSERT INTO trip_comp_rem(point_id,num,x,y,rem,pr_denial) "
    " SELECT :point_id,num,x,y,'VIP', 0 FROM "
    " ( SELECT :point_id,num,x,y FROM trip_comp_elems "
    "  WHERE point_id = :point_id AND num = :num AND "
    "        elem_type IN ( SELECT code FROM comp_elem_types WHERE pr_seat <> 0 ) AND "
    "        trip_comp_elems.y = :y "
    "   MINUS "
    "  SELECT :point_id,num,x,y FROM trip_comp_ranges "
    "   WHERE point_id=:point_id AND layer_type IN (:block_cent_layer, :checkin_layer) "
    " ) "
    " MINUS "
    " SELECT :point_id,num,x,y,rem, 0 FROM trip_comp_rem "
    "  WHERE point_id = :point_id AND num = :num AND "
    "        trip_comp_rem.y = :y "; //??? �᫨ ���� ६�ઠ �����祭��� ���짮��⥫��, � �� �ண��� ����
  QryVIP.CreateVariable( "point_id", otInteger, fltInfo.point_id );
  QryVIP.DeclareVariable( "num", otInteger );
  QryVIP.DeclareVariable( "y", otInteger );
  QryVIP.CreateVariable( "block_cent_layer", otString, EncodeCompLayerType(ASTRA::cltBlockCent) );
  QryVIP.CreateVariable( "checkin_layer", otString, EncodeCompLayerType(ASTRA::cltCheckin) );
  for ( ; !Qry.Eof; Qry.Next() ) {
    int ycount;
    switch( DecodeClass( Qry.FieldAsString( "class" ) ) ) {
        case ASTRA::F: ycount = REM_VIP_F;
            break;
        case ASTRA::C: ycount = REM_VIP_C;
            break;
        case ASTRA::Y: ycount = REM_VIP_Y;
            break;
        default: ycount = 0;
    };
    if ( ycount ) {
      int vy = 0;
      while ( Qry.FieldAsInteger( "miny" ) + vy <= Qry.FieldAsInteger( "maxy" ) && vy < ycount ) {
        QryVIP.SetVariable( "num", Qry.FieldAsInteger( "num" ) );
        QryVIP.SetVariable( "y", Qry.FieldAsInteger( "miny" ) + vy );
        QryVIP.Execute();
        if ( !QryVIP.RowsProcessed( ) )
          break;
        vy++;
      }
    }
  }
}

typedef std::map<int,std::vector<std::string>> tprops;
typedef std::map<int,int> tseat_props;


void setLibraProps( SALONS2::CraftSeats& seats, const tprops& props, const tseat_props& seat_props  ) {
  //seat_props <getSeatKey, libra_seat_idx>
  //props <libra_seat_idx,vector<seat_prop_code>> ᯨ᮪ ᢮��� ����
  for ( const auto& seatSalon : seats ) {
    //std::string lines_row, prior_lines_row;
    for ( auto& seat : seatSalon->places ) {
      if ( !seat.visible ) {
        continue;
      }
      tseat_props::const_iterator sps = seat_props.find( SALONS2::getSeatKey::get( seatSalon->num, seat.x, seat.y ) );
      if ( sps == seat_props.end() ) {
        continue;
      }
      LogTrace(TRACE5) << seatSalon->num << " " << seat.x << " " << seat.y << " " << sps->second;
      tprops::const_iterator seatProps = props.find( sps->second );
      if ( seatProps == props.end() ) {
        continue;
      }
      for ( const auto& p : seatProps->second ) { //properties list
        LogTrace(TRACE5) << p;
        if ( p == EMERGENCY_EXIT_CODE_SEAT ) {
          seat.elem_type = SALONS2::ARMCHAIR_EMERGENCY_EXIT_TYPE;
        }
        if ( p == BASSINET_POSITIONS_CODE_SEAT ) { //��쪠
          seat.AddRemark( ASTRA::NoExists, SALONS2::TSeatRemark( "BSCT", 0 )  );
        }
        if ( p == INFANT_PREFERENCE_CODE_SEAT ) {
          seat.AddRemark( ASTRA::NoExists, SALONS2::TSeatRemark( "INFT", 0 )  );
        }
        if ( p == REAR_FACING_CODE_SEAT ) {
          seat.agle = 180;
        }
        if ( p == LEGS_SPACE_CODE_SEAT ) {
          seat.AddRemark( ASTRA::NoExists, SALONS2::TSeatRemark( "LEGS", 0 )  );
        }
        if ( p == WHEEL_CHAIR_CODE_SEAT ) {
          seat.AddRemark( ASTRA::NoExists, SALONS2::TSeatRemark( "WCHC", 0 )  );
        }
        if ( p == STRETCHER_LOCATION_CODE_SEAT ) {
          seat.AddRemark( ASTRA::NoExists, SALONS2::TSeatRemark( "STCR", 0 )  );
        }
        if ( p == SMOKING_CODE_SEAT ) {
          SALONS2::TLayerSeat seatlayer;
          seatlayer.layer_type = ASTRA::TCompLayerType::cltSmoke;
          SALONS2::TLayerPrioritySeat layerPrioritySeat( seatlayer,
                                                         BASIC_SALONS::TCompLayerTypes::Instance()->priority(
                                                             BASIC_SALONS::TCompLayerTypes::LayerKey( "", seatlayer.layer_type ),
                                                             BASIC_SALONS::TCompLayerTypes::ignoreAirline ) );
          seat.AddLayer( ASTRA::NoExists, layerPrioritySeat );
        }
        if ( p == UNNACOMPANIED_MINOR_CODE_SEAT ) {
          seat.AddRemark( ASTRA::NoExists, SALONS2::TSeatRemark( "UMNR", 0 )  );
        }
        if ( p == LEFT_VACANT_CODE_SEAT ) {
          SALONS2::TLayerSeat seatlayer;
          seatlayer.layer_type = ASTRA::TCompLayerType::cltProtect;
          SALONS2::TLayerPrioritySeat layerPrioritySeat( seatlayer,
                                                         BASIC_SALONS::TCompLayerTypes::Instance()->priority(
                                                             BASIC_SALONS::TCompLayerTypes::LayerKey( "", seatlayer.layer_type ),
                                                             BASIC_SALONS::TCompLayerTypes::ignoreAirline ) );
          seat.AddLayer( ASTRA::NoExists, layerPrioritySeat );
        }
      }
    }
  }
}

std::string getLibraCfg( int plan_id, int conf_id,
                         const std::string& airline,
                         const std::string& bort,
                         TQuery &Qry ) {
  Qry.Clear();
  Qry.SQLText = ComponLibraFinder::getConfigSQLText( true );
  Qry.CreateVariable( "plan_id", otInteger, plan_id );
  Qry.CreateVariable( "conf_id", otInteger, conf_id );
  Qry.CreateVariable( "airline", otString, airline );
  Qry.CreateVariable( "bort", otString, bort );
  Qry.Execute();
  if ( Qry.Eof ) {
    throw EXCEPTIONS::Exception( "getLibraCfg: conf not found, conf_id=%d", conf_id );
  }
  if ( Qry.FieldAsInteger( "invalid_class" ) != 0 ) {
    throw EXCEPTIONS::Exception( "getLibraCfg: conf has invalid clases, conf_id=%d", conf_id );
  }
  std::string res;
  if ( Qry.FieldAsInteger( "F" ) > 0 ) {
    res += '�' + IntToString( Qry.FieldAsInteger( "F" ) );
  }
  if ( Qry.FieldAsInteger( "C" ) > 0 ) {
    if ( !res.empty() ) {
      res += " ";
    }
    res += '�' + IntToString( Qry.FieldAsInteger( "C" ) );
  }
  if ( Qry.FieldAsInteger( "Y" ) > 0 ) {
    if ( !res.empty() ) {
      res += " ";
    }
    res += '�' + IntToString( Qry.FieldAsInteger( "Y" ) );
  };
  return res;
}

void ComponSetter::createBaseLibraCompon( ComponLibraFinder::AstraSearchResult& res,
                                          const std::string& airline,
                                          const std::string& craft,
                                          const std::string& bort,
                                          TQuery &Qry ) {
  LogTrace( TRACE5 ) << __func__ << " plan_id=" << res.plan_id << ", conf_id=" << res.conf_id << ", comp_id=" << res.conf_id;
  res.ReadFromAHMIds( res.plan_id, res.conf_id, Qry ); //
  //����뢠�� ���� �� ������ �冷�
  //����뢠�� ���� �� ���⠬
  //ᮧ���� ������� ����������
  Qry.Clear();
  Qry.SQLText = ComponLibraFinder::getConvertClassSQLText();
  Qry.CreateVariable( "airline", otString, airline );
  Qry.CreateVariable( "bort", otString, bort );
  Qry.Execute();
  std::map<std::string,std::string> cls;
  for ( ; !Qry.Eof; Qry.Next() ) {
    cls[ Qry.FieldAsString( "libra_class" ) ] = Qry.FieldAsString( "class" );
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT class_code,first_row,last_row "
    " FROM WB_REF_WS_AIR_SL_CI_T "
    " WHERE adv_id=:conf_id";
  Qry.CreateVariable( "conf_id", otInteger, res.conf_id );
  Qry.Execute();
  if ( Qry.Eof ) {
    throw EXCEPTIONS::Exception( "createBaseLibraCompon: data not found in  WB_REF_WS_AIR_SL_CI_T, confId=%d", res.conf_id );
  }
  SALONS2::seacherProps classProps;
  for (; !Qry.Eof; Qry.Next() ) {
    std::string libra_class = Qry.FieldAsString( "class_code" );
    if ( cls.find( libra_class ) != cls.end() ) {
      libra_class = cls[ libra_class ];
    }
    else
      EXCEPTIONS::Exception( "createBaseLibraCompon: invalid class %s in compon, confId=%d", libra_class.c_str(), res.conf_id );
    SALONS2::SimpleProp p( libra_class,
                           Qry.FieldAsInteger( "first_row"),
                           Qry.FieldAsInteger( "last_row") );
    LogTrace(TRACE5) << Qry.FieldAsString( "class_code" ) << " " <<  Qry.FieldAsInteger( "first_row") << "-" << Qry.FieldAsInteger( "last_row");
    classProps.emplace_back( p );
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT S_SEAT_ID, u.name "
    " FROM WB_REF_WS_AIR_S_L_P_S_P t join WB_REF_WS_SEATS_PARAMS u "
    "on t.ADV_ID=:plan_id AND "
    "   u.id=t.PARAM_ID "
    " ORDER by S_SEAT_ID, u.name ";
  Qry.CreateVariable( "plan_id", otInteger, res.plan_id );
  Qry.Execute();
  tprops props; //᢮��⢠ ����
  tseat_props seat_props; //��뫪� ������ ���� �� ��. ᢮��⢠
  for ( ; !Qry.Eof; Qry.Next() ) {
    std::pair<tprops::iterator,bool> im =
                            props.insert( std::make_pair( Qry.FieldAsInteger( "s_seat_id" ),
                                          std::vector<std::string>() ) );
    im.first->second.emplace_back( Qry.FieldAsString( "name" ) );
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT s.id, cabin_section, row_number, name "
    " FROM WB_REF_WS_AIR_S_L_PLAN p  join WB_REF_WS_AIR_S_L_A_U_S slaus "
    " on slaus.adv_id=p.adv_id AND "
    "  p.adv_id=:plan_id "
    " join WB_REF_WS_SEATS_NAMES sn "
    " on sn.id=slaus.SEAT_ID left outer join WB_REF_WS_AIR_S_L_P_S s "
    " on p.id=s.PLAN_ID AND "
    " sn.id=s.SEAT_ID "
    " ORDER BY p.CABIN_SECTION, "
    " p.ROW_NUMBER, "
    "sn.SORT_PRIOR";
  Qry.CreateVariable( "plan_id", otInteger, res.plan_id );
  Qry.Execute();
  SALONS2::simpleProps sections( SALONS2::SECTION );
  std::string lastSectionName;
  int first_row = -1, last_row = -1;
  int x = 0, y = -1;
  int salon_num = -1;
  std::string prior_class_code, class_code;
  int rus_count = 0, lat_count = 0;
  SALONS2::TSalonList salonList;
  SALONS2::CraftSeats seats;
  SALONS2::TPlaceList* seatSalon = nullptr;

  class Aisle {
    private:
      bool useInvisibleSeat;
      std::vector<SALONS2::TPlace> rowseats;
      std::string lines;
      std::string prior_lines;
      std::string intLines( const std::string& l ) {
        bool startIdx, stopIdx;
        startIdx = ( !lines.empty() && lines.front() == '=' );
        stopIdx  = ( !lines.empty() && lines.back() == '=' );
        return lines.substr( startIdx, lines.length() - startIdx - stopIdx );
      }
    public:
      Aisle() {
        useInvisibleSeat = true; //false - �����஢���� ��������� ���� �� ᤥ����!
        clear();
      }
      void clear() {
        prior_lines.clear();
        lines.clear();
      }
      void AddRight() {
        if ( lines.back() != '=' ) {
          lines += "=";
          LogTrace(TRACE5) << "add right " << lines;
        }
      }
      void AddLeft() {
        int idx = lines.length() - 2;
        if (idx < 0) {
          idx = 0;
        }
       if ( lines.substr( idx, 1 ) != "=" ) {
          lines.insert( idx, "=" );
          LogTrace(TRACE5) << "add left " << lines;
        }
      }
      void Add( const SALONS2::TPlace &p ) {
        if ( useInvisibleSeat || p.visible ) {
          lines += p.xname;
        }
      }
      std::string getPriorLines() {
        return intLines( prior_lines );
      }
      std::string getCurrLines() {
        return intLines( lines );
      }
      void SetPriorLines() {
        prior_lines = lines;
        lines.clear();
      }
      int CurrX(int x) {
       if ( useInvisibleSeat ) {
          return x+1;
        }
        return getCurrLines().size();
      }
      int getDiff() {
        int res = 0;
        if ( !useInvisibleSeat ) {
          return res;
        }
        std::string l = getCurrLines();
        for ( std::string::const_iterator s = l.begin(); s != l.end(); s++ ) {
          res += ( *s == '=' );
        }
        res -= ( l.back() == '=' );
        //LogTrace(TRACE5) << l << " " << res;
        return res;
      }
      void AddSeat( const SALONS2::TPlace& seat ) {
        if ( useInvisibleSeat || seat.visible ) {
          rowseats.emplace_back( seat );
        }
      }
      void PutSeats( SALONS2::TPlaceList* seats ) {
        if ( seats == nullptr ) {
          return;
        }
        bool isEmpty = seats->isEmpty();
        LogTrace(TRACE5) << __func__ << " add rows isEmpty=" << isEmpty;
        for ( auto &p : rowseats ) {
          if ( isEmpty ) {
            p.y = 0;
          }
          seats->Add( p );
        }
        rowseats.clear();
      }
  };
  Aisle aisle;
  int first_rowidx = 0, last_rowidx = 0;
  for ( ; !Qry.Eof; Qry.Next() ) {
    LogTrace(TRACE5) << Qry.FieldAsString( "cabin_section" ) << " " << Qry.FieldAsInteger( "row_number" );
    if ( lastSectionName != Qry.FieldAsString( "cabin_section" ) ) {
      if ( !lastSectionName.empty() &&
           first_rowidx <= last_rowidx  ) {
        //LogTrace(TRACE5) << lastSectionName << " " << first_row  << "-" << last_row << " rowidx=" << first_rowidx << "-" << last_rowidx;
        sections.emplace_back( SALONS2::SimpleProp( lastSectionName,
                                                    first_rowidx,
                                                    last_rowidx - 1 ) );
        first_rowidx = last_rowidx;
      }
      lastSectionName = Qry.FieldAsString( "cabin_section" );
      first_row = Qry.FieldAsInteger( "row_number" );
    }
    if ( last_row != Qry.FieldAsInteger( "row_number" ) ) { //���� ��
      std::string plines = aisle.getPriorLines(), lines = aisle.getCurrLines();
      bool prChangeLines = ( !plines.empty() &&
                             plines != lines );
      //LogTrace(TRACE5) << "prChangeLines=" << prChangeLines << " prior=" << plines << " lines=" << lines;
      if ( !prChangeLines ) {
        aisle.PutSeats( seatSalon );
      }
      last_rowidx++;
      last_row = Qry.FieldAsInteger( "row_number" );
      classProps.get( last_row, class_code );
      if ( prior_class_code != class_code || prChangeLines ) { //���� ᠫ��
        //LogTrace(TRACE5) << plines << " " << lines;
        prior_class_code = class_code;
        if ( seatSalon == nullptr ||
             !seatSalon->isEmpty() ) {
          seatSalon = new SALONS2::TPlaceList;
          seats.push_back( seatSalon );
          aisle.PutSeats( seatSalon );
        }
        x = 0;
        y = 0;
        if ( !seatSalon->isEmpty() ) {
          aisle.SetPriorLines();
          y++;
        }
        else {
          aisle.clear();
        }
        salon_num++;
        seatSalon->num = salon_num;
      }
      else {
        if ( last_row >= 1 ) {
          y++;
          aisle.SetPriorLines();
          x = 0;
        }
      }
    }
    else {
      x = aisle.CurrX(x);
    }
    //add seat
    //LogTrace(TRACE5) << "x=" << x << " id=" << Qry.FieldAsInteger( "id" );
    /*if ( Qry.FieldIsNULL( "id" ) ) {
      LogTrace(TRACE5) << Qry.FieldAsString( "name" ) << Qry.FieldAsInteger( "row_number" );
      continue; //� ����� ���� ᠫ��� � ������, � ���ன ��� ����� ����???
    }*/
    SALONS2::TPlace seat;
    seat.x = x;
    seat.y = y;
    seat.isplace = !Qry.FieldIsNULL( "id" );
    seat.visible = seat.isplace;
    seat.elem_type = SALONS2::ARMCHAIR_ELEM_TYPE;
    if ( seat.visible ) {
      LogTrace(TRACE5) << Qry.FieldAsInteger( "row_number" ) << Qry.FieldAsString( "name" ) << ",id=" << Qry.FieldAsInteger( "id" );
      TElemFmt fmt;
      seat.clname = ElemToElemId(etClass, class_code, fmt);
      if ( seat.clname.empty() ) {
        throw EXCEPTIONS::Exception( "createBaseLibraCompon: invalid seat class '%s'", class_code.c_str() );
      }
    }
    seat.xname = Qry.FieldAsString( "name" );
    seat.yname = IntToString( Qry.FieldAsInteger( "row_number" ) );
    boost::optional<bool> isLatin=SeatNumber::isLatinIataLine(seat.xname);
    if (isLatin) {
      isLatin.get()?lat_count++:rus_count++;
    }
    seat.xname = SeatNumber::tryNormalizeLine( seat.xname );
    seat.yname = SeatNumber::tryNormalizeRow( seat.yname );
    aisle.Add( seat );


    LogTrace( TRACE5 ) << "(x=" << seat.x << ",y=" << seat.y << ")" << ",xname=" << seat.xname << ",yname=" << seat.yname <<",class=" << seat.clname <<",visible=" <<seat.visible;

    //�᫨ ���� ��室 �ࠢ�, � ���न���� ᫥� ���� �� x 㢥��稢��� �� 1, �᫨ ᫥��, � ⥪���� ���न���� 㢥��稢��� �� 1
    seat.x += aisle.getDiff( );
    if ( seat.visible ) {
      if ( seat.isplace ) {
        tprops::iterator im = props.find( Qry.FieldAsInteger( "id" ) );
        if ( find( im->second.begin(), im->second.end(), AISLE_LEFT_CODE_SEAT ) != im->second.end() ) {
          aisle.AddLeft( );
        }
        if ( find( im->second.begin(), im->second.end(), AISLE_RIGHT_CODE_SEAT ) != im->second.end() ) {
           aisle.AddRight( );
        }
        if (seat.visible = ( find( im->second.begin(), im->second.end(), DISABLE_CODE_SEAT ) == im->second.end() )) {
          seat_props.emplace( SALONS2::getSeatKey::get( seatSalon->num, seat.x, seat.y ), im->first );
        }
      }
    }
    //LogTrace(TRACE5) << aisle.getDiff( ) << " " << seat.x;
    aisle.AddSeat(seat);
  } // end for
  if ( seatSalon != nullptr ) {
    aisle.PutSeats( seatSalon );
    if ( seatSalon->isEmpty() ) {
      delete seatSalon;
      seats.pop_back( );
    }
  }
  salonList.setCraftLang( lat_count >= rus_count );
  if ( !lastSectionName.empty() &&
       first_rowidx <= last_rowidx  ) {
    LogTrace(TRACE5) << lastSectionName << " " << first_row  << "-" << last_row;
    sections.emplace_back( SALONS2::SimpleProp( lastSectionName,
                                                first_rowidx,
                                                last_rowidx ) );
  }
  //����稫� ���� � ⥯��� ��ࠥ� � ᠯ�� - �஡㥬 �������� �㠫���, ���਩�� ��室�, ��室� � ��祥, ॠ��������!!!
  setLibraProps( seats, props, seat_props  );
  salonList._seats = seats;
  SALONS2::TComponSets componSets;
  componSets.bort = bort;
  componSets.airline = airline;
  componSets.classes = getLibraCfg( res.plan_id, res.conf_id,
                                    airline,
                                    bort,
                                    Qry );
  componSets.descr = "AHM ����";
  componSets.craft = craft;
  componSets.modify = (res.comp_id != ASTRA::NoExists)?SALONS2::mChange:SALONS2::mAdd;
  salonList.WriteCompon( res.comp_id, componSets, true );
  sections.write( res.comp_id );
  res.Write( Qry );
}

void CreateFlightCompon( const TCompsRoutes &routes, int comp_id, TQuery &Qry ) {
  LogTrace(TRACE5) << __func__ << ",comp_id=" << comp_id;
  if ( comp_id == ASTRA::NoExists ) {
    LogError(STDLOG) << __func__ << " comp_id is not set";
    return;
  }
  TQuery QryVIP(&OraSession);
  Qry.Clear();
  Qry.SQLText =
      "SELECT pr_lat_seat FROM comps WHERE comp_id=:comp_id";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.Execute();
  int pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );

  TQuery QryTripSets(&OraSession);
  QryTripSets.SQLText =
    "UPDATE trip_sets SET comp_id=:comp_id,pr_lat_seat=:pr_lat_seat,crc_comp=:crc_comp,crc_base_comp=:crc_base_comp WHERE point_id=:point_id";
  QryTripSets.CreateVariable( "comp_id", otInteger, comp_id==ASTRA::NoExists?FNull:comp_id );
  QryTripSets.CreateVariable( "pr_lat_seat", otInteger, pr_lat_seat );
  QryTripSets.DeclareVariable( "point_id", otInteger );
  QryTripSets.DeclareVariable( "crc_comp", otInteger );
  QryTripSets.DeclareVariable( "crc_base_comp", otInteger );
  Qry.Clear();
  Qry.SQLText =
    "BEGIN "
    "DELETE trip_comp_rates WHERE point_id = :point_id;"
    "DELETE trip_comp_rfisc WHERE point_id = :point_id;"
    "DELETE trip_comp_rem WHERE point_id = :point_id; "
    "DELETE trip_comp_baselayers WHERE point_id = :point_id; "
    "DELETE trip_comp_elems WHERE point_id = :point_id; "
    "DELETE trip_comp_layers "
    " WHERE point_id=:point_id AND layer_type IN ( SELECT code from comp_layer_types where del_if_comp_chg<>0 ); "
    "INSERT INTO trip_comp_elems(point_id,num,x,y,elem_type,xprior,yprior,agle,class,xname,yname) "
    " SELECT :point_id,num,x,y,elem_type,xprior,yprior,agle,class,xname,yname "
    "  FROM comp_elems "
    " WHERE comp_id = :comp_id; "
    "INSERT INTO trip_comp_rem(point_id,num,x,y,rem,pr_denial) "
    " SELECT :point_id,num,x,y,rem,pr_denial "
    "  FROM comp_rem "
    " WHERE comp_id = :comp_id; "
    "INSERT INTO trip_comp_rfisc(point_id,num,x,y,color) "
    " SELECT :point_id,num,x,y,color FROM comp_rfisc "
    " WHERE comp_id = :comp_id; "
    "INSERT INTO trip_comp_rates(point_id,num,x,y,color,rate,rate_cur) "
    " SELECT :point_id,num,x,y,color,rate,rate_cur FROM comp_rates "
    " WHERE comp_id = :comp_id; "
    "END;";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.DeclareVariable( "point_id", otInteger );
  TQuery QryBaseLayers(&OraSession);
  QryBaseLayers.SQLText =
    "INSERT INTO trip_comp_baselayers(point_id,num,x,y,layer_type) "
    " SELECT :point_id,num,x,y,layer_type "
    "  FROM comp_baselayers "
    " WHERE comp_id=:comp_id AND layer_type=:layer_type ";
  QryBaseLayers.CreateVariable( "comp_id", otInteger, comp_id );
  QryBaseLayers.DeclareVariable( "point_id", otInteger );
  QryBaseLayers.DeclareVariable( "layer_type", otString );
  TQuery QryLayers(&OraSession);
  QryLayers.SQLText =
    "INSERT INTO trip_comp_layers("
    "       range_id,point_id,point_dep,point_arv,layer_type, "
    "       first_xname,last_xname,first_yname,last_yname,crs_pax_id,pax_id,time_create)"
    "SELECT comp_layers__seq.nextval,:point_id,:point_id,NULL,:layer_type, "
    "       xname,xname,yname,yname,NULL,NULL,system.UTCSYSDATE "
    " FROM comp_baselayers, comp_elems "
    " WHERE comp_elems.comp_id=:comp_id AND "
    "       comp_elems.comp_id=comp_baselayers.comp_id AND "
    "       comp_elems.num=comp_baselayers.num AND "
    "       comp_elems.x=comp_baselayers.x AND "
    "       comp_elems.y=comp_baselayers.y AND "
    "       comp_baselayers.layer_type=:layer_type ";
  QryLayers.CreateVariable( "comp_id", otInteger, comp_id );
  QryLayers.DeclareVariable( "point_id", otInteger );
  QryLayers.DeclareVariable( "layer_type", otString );
  SALONS2::CompCheckSum checksum(0,0);
  std::vector<int> points_tranzit_check_wait_alarm;
  for ( const auto& i : routes ) {
    if ( i.inRoutes && i.auto_comp_chg && i.pr_reg ) {
      Qry.SetVariable( "point_id", i.point_id );
      Qry.Execute();
      for ( int ilayer=0; ilayer<ASTRA::cltTypeNum; ilayer++ ) {
        if ( SALONS2::isBaseLayer( (ASTRA::TCompLayerType)ilayer, true ) ) { // �롨ࠥ� �� ������ ᫮� ��� ������� ����������
          if ( SALONS2::isBaseLayer( (ASTRA::TCompLayerType)ilayer, false ) ) { // ������ ᫮� ��� ���������� ३�
            QryBaseLayers.SetVariable( "point_id", i.point_id );
            QryBaseLayers.SetVariable( "layer_type", EncodeCompLayerType( (ASTRA::TCompLayerType)ilayer ) );
            QryBaseLayers.Execute();
          }
          else { //�� ������
            QryLayers.SetVariable( "point_id", i.point_id );
            QryLayers.SetVariable( "layer_type", EncodeCompLayerType( (ASTRA::TCompLayerType)ilayer ) );
            QryLayers.Execute();
          }
        }
      }
      if ( checksum.base_crc32 == 0 ) {
        checksum = SALONS2::CompCheckSum::calcFromDB( i.point_id );
      }
      InitVIP( i, QryVIP );
      TTripClasses trip_classes( i.point_id );
      trip_classes.processSalonsCfg( );
      QryTripSets.SetVariable( "point_id", i.point_id );
      QryTripSets.SetVariable( "crc_comp", checksum.total_crc32 );
      QryTripSets.SetVariable( "crc_base_comp", checksum.base_crc32 );
      QryTripSets.Execute();
      LEvntPrms prms;
      TCFG(i.point_id).param(prms);
      prms << PrmSmpl<int>("id", comp_id);
      TReqInfo::Instance()->LocaleToLog("EVT.ASSIGNE_BASE_LAYOUT", prms, ASTRA::evtFlt, i.point_id);
      if ( find( points_tranzit_check_wait_alarm.begin(),
                 points_tranzit_check_wait_alarm.end(),
                 i.point_id) == points_tranzit_check_wait_alarm.end() ) {
        points_tranzit_check_wait_alarm.push_back( i.point_id );
      }
    }
  }
  TCompsRoutes r(routes);
  r.check_diffcomp_alarm();
  SALONS2::check_waitlist_alarm_on_tranzit_routes( points_tranzit_check_wait_alarm, __func__ );
}

bool ComponSetter::isLibraMode() {
  return LibraComps::isLibraMode( fltInfo );
}

ComponSetter::TStatus ComponSetter::IntSetCraft( bool pr_tranzit_routes ) {
  // �஢�ઠ �� ����⢮����� �������� ���������� �� ⨯� ��
  LogTrace(TRACE5) << __func__ << " fcomp_id=" << fcomp_id;
  TQuery Qry(&OraSession);
  Clear();
  ComponLibraFinder::AstraSearchResult compState;
  if ( isLibraMode() ) {
    if ( fltInfo.bort.empty() ) {
      LogTrace( TRACE5 ) << __func__ << " bort is empty";
      return NotCrafts;
    }
    //����뢠�� ��. ���������� � ���䨣��樨 �� ⠡���� libra_comps, �᫨ ���� plan_id, conf_id
    compState.getLibraCompStatus( fltInfo.airline, fltInfo.bort, Qry, fcomp_id ); //
    LogTrace(TRACE5) << "plan_id=" << compState.plan_id;
    switch ( compState.evtStatus ) {
      case ComponLibraFinder::AstraSearchResult::TEventLibraStatus::evNoExists: //���������� � ���䨣��樨 �� �������, ��祣� ��������
          LogTrace( TRACE5 ) << __func__ << " comp not exists";
          return NotCrafts;
      case ComponLibraFinder::AstraSearchResult::TEventLibraStatus::evNoChange:
          LogTrace( TRACE5 ) << __func__ << " comp not changed";
          if ( fmode == mNone ) { //�� ���� ���ᮧ������ ���������� �� ३�
            return NoChanges;
          }
          break;
      default:;
    }
  }
  else {
    if ( fltInfo.craft.empty() || !ComponFinder::isExistsCraft( fltInfo.craft, Qry ) ) {
      LogTrace( TRACE5 ) << __func__ << " craft is empty or not found in base comps, craft=" << fltInfo.craft;
      return NotCrafts;
    }
  }
  TCompsRoutes routes;
  routes.get( pr_tranzit_routes, fltInfo.point_id, Qry );
  std::vector<std::string> airps;
  for ( const auto& i : routes ) {
    if ( i.inRoutes && i.auto_comp_chg && i.pr_reg ) {
      push_back( i.point_id );
      airps.push_back( i.airp );
    }
    if ( i.point_id == fltInfo.point_id && !i.auto_comp_chg ) {
      return NoChanges;
    }
  }
  if ( empty() ) {
    LogTrace(TRACE5) << __func__ << ": return NoChanges";
    return NoChanges;
  }
  int search_comp_id;
  if ( isLibraMode() && compState.evtStatus == ComponLibraFinder::AstraSearchResult::evNoChange ) {
    search_comp_id = compState.conf_id;
  }
  else {
    search_comp_id = SearchCompon( pr_tranzit_routes, airps, Qry, isLibraMode()?compState.plan_id:ASTRA::NoExists );
    LogTrace(TRACE5) << search_comp_id;
    if ( search_comp_id == ASTRA::NoExists ) {
      LogTrace( TRACE5 ) << __func__ << ": return NotFound";
      return NotFound;
    }
  }
  if ( isLibraMode() ) { // search_comp_id = conf_id - ����⠥��� ���� � ������� ����������� ����� ���������� ����� � ������� �� �� ���������
    if ( compState.evtStatus == ComponLibraFinder::AstraSearchResult::evChange ) {
      compState.conf_id = search_comp_id;
      createBaseLibraCompon( compState, fltInfo.airline, fltInfo.craft, fltInfo.bort, Qry  );
      if ( compState.comp_id == ASTRA::NoExists ) {
        LogTrace( TRACE5 ) << __func__ << " invalid comps in libra plan_id=" << compState.plan_id << ", conf_id=" << compState.conf_id;
        return NotCrafts;
      }
    }
    search_comp_id = compState.comp_id;
  }
  else {
    // ������ ��ਠ�� ����������
    if ( fcomp_id == search_comp_id ) {
      LogTrace( TRACE5 ) << __func__ << ": return NoChanges";
      return NoChanges; // �� �㦭� �������� ����������
    }
  }

  fcomp_id = search_comp_id;
  LogTrace(TRACE5) << fcomp_id;
  CreateFlightCompon( routes, fcomp_id, Qry );
  LogTrace( TRACE5 ) << __func__ << ": return Created";
  return Created;
}

ComponSetter::TStatus ComponSetter::SetCraft( bool pr_tranzit_routes ) {
  TFlights flights;
  flights.Get( fltInfo.point_id, ftTranzit );
  flights.Lock(__FUNCTION__);
  if ( SALONS2::isFreeSeating( fltInfo.point_id ) ) { //᢮������ ��ᠤ��
    return IntSetCraftFreeSeating( );
  }
  else {
    return IntSetCraft( pr_tranzit_routes );
  }
}

ComponSetter::TStatus ComponSetter::AutoSetCraft( bool pr_tranzit_routes ) {
  LogTrace( TRACE5 ) << __func__ << ", point_id=" << fltInfo.point_id;
  try {
    if ( isAutoCompChg( fltInfo.point_id ) ||
         isLibraMode() ) {
      return SetCraft( pr_tranzit_routes );
    }
    return NoChanges; // �� �ॡ���� �����祭�� ����������
  }
  catch( EXCEPTIONS::Exception &e ) {
    LogError( STDLOG ) << __func__ << ", point_id=" << fltInfo.point_id << ",error=" << e.what();
  }
  catch( ... ) {
   LogError( STDLOG ) << __func__ << " unknown error, point_id" << fltInfo.point_id;
  }
  return NoChanges;
}
ComponSetter::TStatus AutoSetCraft( int point_id, bool pr_tranzit_routes ) {
   ComponCreator::ComponSetter componSetter( point_id, ComponCreator::ComponSetter::TModeCreate::mNone );
   return componSetter.AutoSetCraft( pr_tranzit_routes );
}

void SychAHMCompsFromBort( const std::string& airline, const std::string& craft, const std::string& bort  )
{
  LogTrace(TRACE5) << __func__ << " airline=" <<airline << " craft=" << craft << " bort=" << bort;
  TQuery Qry(&OraSession);
  std::vector<int> configs;
  //��ࠫ� �� ���䨣�
  int plan_id = ComponLibraFinder::GetLibraConfigs( airline, bort, ASTRA::NoExists, configs, Qry );
  //㤠��� �, ����� 㦥 ���� � ������� �����������
  for ( const auto& conf_id : configs ) {
    ComponLibraFinder::AstraSearchResult res;
    res.ReadFromAHMIds( plan_id, conf_id, Qry );
    if ( !res.isOk() ) { //ᮧ���� ���������� � �������
      ComponSetter::createBaseLibraCompon( res, airline, craft, bort, Qry );
    }
  }
}


} //end namespace ComponCreator
