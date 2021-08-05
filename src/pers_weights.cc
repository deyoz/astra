#include <stdlib.h>
#include "astra_consts.h"
#include "date_time.h"
#include "exceptions.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "astra_date_time.h"
#include "stl_utils.h"
#include "oralib.h"
#include "term_version.h"
#include "pers_weights.h"
#include "points.h"
#include "events.h"
#include "qrys.h"
#include "date_time.h"
#include "flt_settings.h"

#include "serverlib/perfom.h"

#define NICKNAME "DJEK"
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace ASTRA::date_time;
using namespace EXCEPTIONS;
using namespace boost::local_time;

////////////////////////////////////////TPersWeights////////////////////////////
void TPersWeights::Update()
{
  weights.clear();
  TPerTypeWeight p;
  DB::TQuery Qry(PgOra::getROSession("PERS_WEIGHTS"), STDLOG);
  Qry.SQLText =
    "SELECT id,airline,craft,bort,class,subclass,pr_summer,first_date,"
    "       last_date,male,female,child,infant FROM pers_weights";
  Qry.Execute();
  for ( ; !Qry.Eof; Qry.Next() ) {
    TPerTypeWeight p;
    p.id = Qry.FieldAsInteger( "id" );
    if ( Qry.FieldIsNULL( "first_date" ) )
      p.first_date = NoExists;
    else
      p.first_date = Qry.FieldAsDateTime( "first_date" );
    if ( Qry.FieldIsNULL( "last_date" ) )
      p.last_date = NoExists;
    else
      p.last_date = Qry.FieldAsDateTime( "last_date" );
    if ( p.first_date == NoExists && p.last_date == NoExists && Qry.FieldIsNULL( "pr_summer" ) )
      continue;
    p.pr_summer = Qry.FieldAsInteger( "pr_summer" );
    p.airline = Qry.FieldAsString( "airline" );
    p.craft = Qry.FieldAsString( "craft" );
    p.bort = Qry.FieldAsString( "bort" );
    p.weight.id = p.id;
    p.weight.cl = Qry.FieldAsString( "class" );
    p.weight.subcl = Qry.FieldAsString( "subclass" );
    p.weight.male = Qry.FieldAsInteger( "male" );
    if ( Qry.FieldIsNULL( "female" ) )
      p.weight.female = NoExists;
    else
      p.weight.female = Qry.FieldAsInteger( "female" );
    p.weight.child = Qry.FieldAsInteger( "child" );
    p.weight.infant = Qry.FieldAsInteger( "infant" );
    weights.push_back( p );
  }
}


TPersWeights::TPersWeights()
{
  Update();
}

inline TDateTime SetDate( TDateTime date, int Year, int diffday )
{
  if ( date == ASTRA::NoExists )
    return ASTRA::NoExists;
  else {
    int Tmp; int Month; int Day;
    DecodeDate( date, Tmp, Month, Day );
    try {
      EncodeDate( Year, Month, Day, date );
      return date;
    }
    catch(const EConvertError&) { // high year????
      TDateTime ddd = date + diffday;
      DecodeDate( ddd, Tmp, Month, Day );
      EncodeDate( Year, Month, Day, date );
      return date;
    }
  }
}

void TPersWeights::getRules( const TDateTime &scd_utc, const std::string &airline,
                             const std::string &craft, const std::string &bort,
                             PersWeightRules &rweights )
{
  rweights.Clear();
  const int equal_craft = 8;
  const int equal_airline = 16;
  const int equal_bort = 32;
  map<string,ClassesPersWeight> priority_class;
  TDateTime scd_local, first_date, last_date;
  int Year, Month;
  string region = CityTZRegion( "МОВ" );
  scd_local = UTCToLocal( scd_utc, region );
  DecodeDate( scd_local, Year, Month, Month );
  for ( vector<TPerTypeWeight>::iterator p=weights.begin(); p!=weights.end(); p++ ) {
    bool good_cond = false;
    first_date = SetDate( p->first_date, Year, 1 );
    int vYear = Year;
    if ( p->first_date != ASTRA::NoExists && p->last_date != ASTRA::NoExists )
      vYear += ( p->first_date > p->last_date );
    last_date = SetDate( p->last_date, vYear, -1 );
    if ( last_date != ASTRA::NoExists )
      last_date += 1.0;
    if ( ( first_date == ASTRA::NoExists || scd_local >= first_date ) &&
         ( last_date == ASTRA::NoExists || scd_local < last_date ) ) {
      if ( p->first_date == ASTRA::NoExists &&
           p->last_date == ASTRA::NoExists ) {
        tst();
        good_cond = ( season( UTCToLocal(scd_utc, region) ).isSummer() == p->pr_summer );
      }
      else
        good_cond = true;
    }
    if ( !good_cond )
      continue;
    ProgTrace( TRACE5, "id=%d, bort=%s, p->bort=%s, airline=%s, p->airline=%s, craft=%s, p->craft=%s",
               p->id, bort.c_str(), p->bort.c_str(), airline.c_str(), p->airline.c_str(), craft.c_str(), p->craft.c_str() );
    if ( !( ( p->bort.empty() || p->bort == bort ) &&
            ( p->airline.empty() || p->airline == airline ) &&
            ( p->craft.empty() || p->craft == craft ) ) )
      continue;
    ProgTrace( TRACE5, "id=%d", p->id );

    int priority = 0;
    priority += ( !bort.empty() && p->bort == bort )*equal_bort;
    priority += ( !airline.empty() && p->airline == airline )*equal_airline;
    priority += ( !craft.empty() && p->craft == craft )*equal_craft;
    string cl;
    if ( p->weight.cl.empty() )
      cl += " ";
    else
      cl += p->weight.cl;
    if ( p->weight.subcl.empty() )
      cl += " ";
    else
      cl += p->weight.subcl;
    if ( priority_class.find( cl ) == priority_class.end() ||
         priority_class[ cl ].priority < priority ||
         (priority_class[ cl ].priority == priority && priority_class[ cl ].id < p->id) ) {
      priority_class[ cl ] = p->weight;
      priority_class[ cl ].priority = priority;
    }
  }
  for ( map<string,ClassesPersWeight>::iterator i=priority_class.begin(); i!=priority_class.end(); i++ ) {
    rweights.Add( i->second );
  }
}

void TPersWeights::getRules( int point_id, PersWeightRules &weights )
{
  weights.Clear();
  DB::TQuery Qry(PgOra::getROSession("POINTS"), STDLOG);
  Qry.SQLText =
    "SELECT airline,bort,craft,scd_out FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof )
    throw Exception( "Flight not found" );
  if ( Qry.FieldIsNULL( "scd_out" ) )
    return;
  getRules( Qry.FieldAsDateTime( "scd_out" ), Qry.FieldAsString( "airline" ),
            Qry.FieldAsString( "craft" ), Qry.FieldAsString( "bort" ), weights );
}

void PersWeightRules::read( int point_id )
{
  Clear();
  DB::TQuery Qry(PgOra::getROSession("TRIP_PERS_WEIGHTS"), STDLOG);
  Qry.SQLText =
    "SELECT class,subclass,male,female,child,infant "
    " FROM trip_pers_weights WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof )
    return;
  while ( !Qry.Eof ) {
    ClassesPersWeight w;
    w.id = 0;
    w.cl = Qry.FieldAsString( "class" );
    w.subcl = Qry.FieldAsString( "subclass" );
    w.male = Qry.FieldAsInteger( "male" );
    if ( Qry.FieldIsNULL( "female" ) )
      w.female = ASTRA::NoExists;
    else
      w.female = Qry.FieldAsInteger( "female" );
    w.child = Qry.FieldAsInteger( "child" );
    w.infant = Qry.FieldAsInteger( "infant" );
    weights.push_back( w );
    Qry.Next();
  }
}

bool PersWeightRules::intequal( PersWeightRules *p )
{
  if ( weights.size() != p->weights.size() )
    return false;
  for ( std::vector<ClassesPersWeight>::iterator i=weights.begin(); i!=weights.end(); i++ ) {
    std::vector<ClassesPersWeight>::iterator j=p->weights.end();
    for ( j=p->weights.begin(); j!=p->weights.end(); j++ ) {
      if ( i->cl == j->cl && i->subcl == j->subcl &&
           i->male == j->male && i->female == j->female && i->child == j->child &&
           i->infant == j->infant ) {
        break;
      }
    }
    if ( j == p->weights.end() )
      return false;
  }
  return true;
}

bool PersWeightRules::really_write(int point_id)
{
    std::string trip_sets_src;
    auto cur = make_db_curs("SELECT weight_src FROM trip_sets WHERE point_id=:point_id",
                            PgOra::getROSession("TRIP_SETS"));
    cur.defNull(trip_sets_src,PERS_WEIGHT_NOT_SET_SRC)
     .bind(":point_id", point_id)
     .exec();

    if(!!cur.fen()) // нет строки в таблице trip_sets, нечего обновлять
      return false;

    TTripInfo info;
    info.getByPointId(point_id);
    std::string src_set;
    //находим настройку на рейс
    if ( GetTripSets( tsLIBRACent, info ) ) {
      src_set = PERS_WEIGHT_LIBRA_SRC;
    }
    else
      if ( GetTripSets(tsLCIPersWeights,info) ) {
        src_set = PERS_WEIGHT_LCI_SRC;
      }
      else
        src_set = PERS_WEIGHT_ASTRA_SRC;
    if ( src_set != source ) { // если режим настройки и источник данных не совпадают, то не обновляем
      return false;
    }
    if ( trip_sets_src != source ) { // если не указан либо указан другой источник информации, то  обновить источник
      auto cur = make_db_curs("UPDATE trip_sets SET weight_src=:weight_src"
                              " WHERE point_id=:point_id",
                              PgOra::getRWSession("TRIP_SETS"));
      cur.bind(":weight_src",source)
         .bind(":point_id", point_id)
         .exec();
    }
    return true;
}

void PersWeightRules::write( int point_id )
{
  ProgTrace( TRACE5, "PersWeightRules::write point_id=%d", point_id );

  if(not really_write(point_id)) return;

  TFlights fligths;
  fligths.Get( point_id, ftTranzit );
  if ( fligths.empty() )
    throw Exception( "Flight not found" );
  fligths.Lock(__FUNCTION__);
  DB::TQuery DelQry(PgOra::getRWSession("TRIP_PERS_WEIGHTS"), STDLOG);
  DelQry.SQLText =
    "DELETE FROM trip_pers_weights WHERE point_id=:point_id";
  DelQry.CreateVariable( "point_id", otInteger, point_id );
  DelQry.Execute();

  DB::TQuery InsQry(PgOra::getRWSession("TRIP_PERS_WEIGHTS"), STDLOG);
  InsQry.SQLText =
    "INSERT INTO trip_pers_weights(point_id,class,subclass,male,female,child,infant)"
    " VALUES(:point_id,:class,:subclass,:male,:female,:child,:infant)";
  InsQry.CreateVariable( "point_id", otInteger, point_id );
  InsQry.DeclareVariable( "class", otString );
  InsQry.DeclareVariable( "subclass", otString );
  InsQry.DeclareVariable( "male", otInteger );
  InsQry.DeclareVariable( "female", otInteger );
  InsQry.DeclareVariable( "child", otInteger );
  InsQry.DeclareVariable( "infant", otInteger );
  ProgTrace( TRACE5, "weights.size()=%zu", weights.size() );
  std::string lexema_id = "EVT.PERS_WEIGHTS";
  PrmEnum prmenum("weights", ", ");
  for ( std::vector<ClassesPersWeight>::iterator i=weights.begin(); i!=weights.end(); i++ ) {
    InsQry.SetVariable( "class", i->cl );
    InsQry.SetVariable( "subclass", i->subcl );
    InsQry.SetVariable( "male", i->male );
    if ( i->female == ASTRA::NoExists )
      InsQry.SetVariable( "female", FNull );
    else
      InsQry.SetVariable( "female", i->female );
    InsQry.SetVariable( "child", i->child );
    InsQry.SetVariable( "infant", i->infant );
    InsQry.Execute();
    if ( !i->cl.empty() ) {
      PrmLexema lexema("", "EVT.PERS_WEIGHTS_CLASS");
      lexema.prms << PrmElem<std::string>("cl", etClass, i->cl);
      prmenum.prms << lexema;
    }
    if ( !i->subcl.empty() ) {
      PrmLexema lexema("", "EVT.PERS_WEIGHTS_SUBCLASS");
      lexema.prms << PrmElem<std::string>("subcl", etSubcls, i->subcl);
      prmenum.prms << lexema;
    }
    PrmLexema lexema("", "EVT.PERS_WEIGHTS_ADULT");
    if ( i->female == ASTRA::NoExists )
      lexema.prms << PrmSmpl<int>("adult", i->male);
    else {
      lexema.ChangeLexemaId("EVT.PERS_WEIGHTS_MALE_FEMALE");
      lexema.prms << PrmSmpl<int>("male", i->male) << PrmSmpl<int>("female", i->female);
    }
    lexema.prms << PrmSmpl<int>("child", i->child);
    lexema.prms << PrmSmpl<int>("infant", i->infant);
    prmenum.prms << lexema;
  }
    TReqInfo::Instance()->LocaleToLog(lexema_id,
            LEvntPrms()
            << PrmSmpl<std::string>("source", (source==PERS_WEIGHT_ASTRA_SRC)?"": source + ": ")
            << prmenum,
            evtFlt, point_id);
}

bool PersWeightRules::weight( std::string cl, std::string subcl, ClassesPersWeight &weight )
{
  const int equal_subcl = 2;
  const int equal_cl = 4;
  std::vector<ClassesPersWeight>::iterator curr = weights.end();
  int max_priority = -1, max_id = -1;
  for ( std::vector<ClassesPersWeight>::iterator i=weights.begin(); i!=weights.end(); i++ ) {
    if ( !( ( i->cl.empty() || i->cl == cl ) &&
            ( i->subcl.empty() || i->subcl == subcl ) ) )
      continue;
    int priority = 0;
    priority += ( !cl.empty() && i->cl == cl )*equal_cl;
    priority += ( !subcl.empty() && i->subcl == subcl )*equal_subcl;
    if ( max_priority < priority ||
         (max_priority == priority && max_id < i->id) ) {
      max_id = i->id;
      max_priority = priority;
      curr = i;
    }
  }
  if ( curr != weights.end() ) {
    weight = *curr;
    if ( weight.female == ASTRA::NoExists )
      weight.female = weight.male;
    return true;
  }
  return false;
}

void TFlightWeights::read( int point_id, TTypeFlightWeight weight_type, bool include_wait_list )
{
  Clear();
  PersWeightRules r;
  r.read( point_id );
  if ( r.empty() ) {  //назначить правила
     TPersWeights persWeights;
     persWeights.getRules( point_id, r );
     r.write( point_id );
  }

  bool use_counters_by_subcls = weight_type != withBrd && include_wait_list;

  DB::TQuery Qry(use_counters_by_subcls ? PgOra::getROSession("COUNTERS_BY_SUBCLS")
                                        : PgOra::getROSession({"PAX_GRP", "PAX", "BAG2"}), STDLOG);
  std::string sql;
  if (use_counters_by_subcls)
  {
    sql =  "SELECT class, subclass, "
                  "male, female, child, infant, "
                  "rk_weight, bag_weight "
             "FROM counters_by_subcls "
            "WHERE point_id = :point_id";
  }
  else
  {
    sql = "WITH cte_grps AS ( "
           "SELECT grp_id, "
                  "class, "
                  "status, "
                  "bag_refuse "
             "FROM pax_grp "
            "WHERE pax_grp.point_dep = :point_id "
              "AND pax_grp.status <> :status), "
          "cte_pax AS ( "
           "SELECT COALESCE(pax.cabin_class, cte_grps.class) AS class, "
                  "subclass, "
                  "is_female, "
                  "pers_type, "
                  "pax.grp_id, "
                  "pax.bag_pool_num "
             "FROM cte_grps JOIN pax "
               "ON cte_grps.grp_id = pax.grp_id ";

    if (!include_wait_list) {
      if (DEMO_MODE()) {
        TST();
      } else {
        sql +="AND salons.is_waitlist(pax.pax_id, pax.seats, pax.is_jmp, cte_grps.status, :point_id, rownum) = 0 ";
      }
    }

    sql += weight_type == withBrd
            ? "AND pr_brd = 1) "
            : "AND pr_brd IS NOT NULL) ";
    sql += "SELECT a.class AS class, "
                  "a.subclass AS subclass, "
                  "a.male, "
                  "a.female, "
                  "a.child, "
                  "a.infant, "
                  "b.rk_weight, "
                  "b.bag_weight "
           "FROM ( "
           "SELECT class, "
                  "subclass, "
                  "COALESCE(SUM(CASE WHEN pers_type = :adl "
                                     "AND COALESCE(is_female, 0) = 0 THEN 1 END), 0) AS male, "
                  "COALESCE(SUM(CASE WHEN pers_type = :adl "
                                     "AND is_female = 1  THEN 1 END), 0) AS female, "
                  "COALESCE(SUM(CASE pers_type WHEN :chd THEN 1 END), 0) AS child, "
                  "COALESCE(SUM(CASE pers_type WHEN :inf THEN 1 END), 0) AS infant "
             "FROM cte_pax "
            "GROUP BY class, subclass) a "
            "INNER JOIN ( "
           "SELECT class as class, "
                  "subclass as subclass, "
                  "COALESCE(SUM(CASE WHEN pr_cabin = 1 THEN weight END), 0) rk_weight, "
                  "COALESCE(SUM(CASE WHEN pr_cabin = 0 THEN weight END), 0) bag_weight "
             "FROM cte_pax c LEFT OUTER JOIN bag2 "
               "ON c.grp_id = bag2.grp_id "
              "AND c.bag_pool_num = bag2.bag_pool_num "
            "GROUP BY class, subclass) b "
               "ON a.class = b.class "
              "AND a.subclass = b.subclass "
          "UNION "
           "SELECT class, "
                  "NULL AS subclass, "
                  "0 AS male, "
                  "0 AS female, "
                  "0 AS child, "
                  "0 AS infant, "
                  "COALESCE(SUM(CASE WHEN pr_cabin = 1 THEN weight END), 0) rk_weight, "
                  "COALESCE(SUM(CASE WHEN pr_cabin = 0 THEN weight END), 0) bag_weight "
             "FROM cte_grps JOIN bag2 "
               "ON cte_grps.grp_id = bag2.grp_id "
              "AND class IS NULL "
              "AND bag_refuse = 0 "
            "GROUP BY class ";
    Qry.CreateVariable( "adl", otString, string(EncodePerson( ASTRA::adult )) );
    Qry.CreateVariable( "chd", otString, string(EncodePerson( ASTRA::child )) );
    Qry.CreateVariable( "inf", otString, string(EncodePerson( ASTRA::baby )) );
    Qry.CreateVariable( "status", otString, string(EncodePaxStatus(ASTRA::psCrew)) );
  };
  //ProgTrace( TRACE5, "TFlightWeights::read: sql=%s", sql.str().c_str() );
  Qry.SQLText = sql;
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  int m,f,c,i;
  while ( !Qry.Eof ) {
    m = Qry.FieldAsInteger( "male" );
    f = Qry.FieldAsInteger( "female" );
    c = Qry.FieldAsInteger( "child" );
    i = Qry.FieldAsInteger( "infant" );
    if (!r.isFemale())
    {
      m+=f;
      f=0;
    };
    male += m;
    female += f;
    child += c;
    infant += i;
    if (!Qry.FieldIsNULL( "class" ))
    {
      ClassesPersWeight weight;
      if ( r.weight( Qry.FieldAsString( "class" ), Qry.FieldAsString( "subclass" ), weight ) ) {
        weight_male += m*weight.male;
        weight_female += f*weight.female;
        weight_child += c*weight.child;
        weight_infant += i*weight.infant;
        weight_cabin_bag += Qry.FieldAsInteger( "rk_weight" );
        weight_bag += Qry.FieldAsInteger( "bag_weight" );
      }
    }
    else
    {
      weight_cabin_bag += Qry.FieldAsInteger( "rk_weight" );
      weight_bag += Qry.FieldAsInteger( "bag_weight" );
    };
    Qry.Next();
  }
}

int getCommerceWeight( int point_id, TTypeFlightWeight weight_type, TTypeCalcCommerceWeight calc_type )
{
  PersWeightRules r;
  ClassesPersWeight weight;
  r.read( point_id );
    TFlightWeights w;
    w.read( point_id, weight_type, true );
    TPointsDest dest;
    BitSet<TUseDestData> UseData;
    UseData.setFlag( udCargo );
    if ( calc_type == CWResidual )
      UseData.setFlag( udMaxCommerce );
    dest.Load( point_id, UseData );
  TFlightCargos cargos;
  //cargos.Load( point_id, dest.pr_tranzit, dest.first_point, dest.point_num, dest.pr_del ); //  dest.pr_del == 1 ???
  std::vector<TPointsDestCargo> cargs;
  dest.cargos.Get( cargs );
  int weight_cargos = 0;
  for ( vector<TPointsDestCargo>::iterator c=cargs.begin(); c!=cargs.end(); c++ ) {
    weight_cargos += c->cargo;
    weight_cargos += c->mail;
  }
  weight_cargos += w.weight_male +
                   w.weight_female +
                   w.weight_child +
                   w.weight_infant +
                   w.weight_bag +
                   w.weight_cabin_bag;
  ProgTrace( TRACE5, "getCommerceWeight: weight_cargos=%d", weight_cargos );
  if ( calc_type == CWTotal )
    return weight_cargos;
  ProgTrace( TRACE5, "getCommerceWeight: max_commerce=%d", dest.max_commerce.GetValue() );
  if ( dest.max_commerce.GetValue() == ASTRA::NoExists )
    return ASTRA::NoExists;
  else
    return dest.max_commerce.GetValue() - weight_cargos;
}

struct TStatBySubclsKey
{
  string subcl, cl;
  TStatBySubclsKey(const string &p1,
                   const string &p2): subcl(p1), cl(p2) {};
  bool operator < (const TStatBySubclsKey &item) const
  {
    if (subcl!=item.subcl)
      return subcl<item.subcl;
    return cl<item.cl;
  };
  bool operator == (const TStatBySubclsKey &item) const
  {
    return subcl==item.subcl &&
           cl==item.cl;
  };
};
struct TStatBySubclsItem
{
  int male, female, child, infant, bag_weight, rk_weight;
  TStatBySubclsItem(): male(0), female(0), child(0), infant(0), bag_weight(0), rk_weight(0) {};
  bool operator == (const TStatBySubclsItem &item) const
  {
    return male==item.male &&
           female==item.female &&
           child==item.child &&
           infant==item.infant &&
           bag_weight==item.bag_weight &&
           rk_weight==item.rk_weight;
  };
};

void recountBySubcls(int point_id,
                     const TGrpToLogInfo &grpInfoBefore,
                     const TGrpToLogInfo &grpInfoAfter)
{
  map<TStatBySubclsKey, TStatBySubclsItem> stat;
  for(int pass=0; pass<2; pass++)
  {
    map<TPaxToLogInfoKey, TPaxToLogInfo>::const_iterator p=(pass==0?grpInfoBefore.pax.begin():
                                                                    grpInfoAfter.pax.begin());
    int sign=pass==0?-1:+1;
    for(;p!=(pass==0?grpInfoBefore.pax.end():grpInfoAfter.pax.end()); ++p)
    {
      const TPaxToLogInfo &pax=p->second;
      if (!pax.refuse.empty()) continue;
      if (DecodePaxStatus(pax.status.c_str())==psCrew) continue;
      map<TStatBySubclsKey, TStatBySubclsItem>::iterator i=
        stat.insert(make_pair(TStatBySubclsKey(pax.subcl, pax.cabin_cl), TStatBySubclsItem())).first;
      if (i==stat.end()) throw Exception("%s: i==stat.end()", __FUNCTION__);
      switch(DecodePerson(pax.pers_type.c_str()))
      {
        case adult:
          if (pax.is_female==NoExists || pax.is_female==0)
            i->second.male+=sign;
          else
            i->second.female+=sign;
          break;
        case child:
          i->second.child+=sign;
          break;
        case baby:
          i->second.infant+=sign;
          break;
        default:
          break;
      };
      i->second.bag_weight+=sign*pax.bag_weight;
      i->second.rk_weight+=sign*pax.rk_weight;
    };
  };

  DB::TCachedQuery Qry(
    PgOra::getRWSession("COUNTERS_BY_SUBCLS"),
    PgOra::supportsPg("COUNTERS_BY_SUBCLS")
     ? "INSERT INTO counters_by_subcls( point_id, subclass, class, male, female, child, infant, bag_weight, rk_weight) "
                              "VALUES (:point_id,:subclass,:class,:male,:female,:child,:infant,:bag_weight,:rk_weight) "
       "ON CONFLICT (point_id, subclass, class) DO UPDATE "
           "SET male = excluded.male + :male, female = excluded.female + :female, "
               "child = excluded.child + :child, infant = excluded.infant + :infant, "
               "bag_weight = excluded.bag_weight + :bag_weight, rk_weight = excluded.rk_weight + :rk_weight "
     : "BEGIN "
         "UPDATE counters_by_subcls "
         "SET male  = male + :male, female = female + :female, "
             "child = child + :child, infant = infant + :infant, "
             "bag_weight = bag_weight + :bag_weight, rk_weight = rk_weight + :rk_weight "
         "WHERE point_id = :point_id AND "
               "(subclass IS NULL AND :subclass IS NULL OR subclass = :subclass) AND "
               "(class IS NULL AND :class IS NULL OR class = :class); "
         "IF SQL%NOTFOUND THEN "
           "INSERT INTO counters_by_subcls( point_id, subclass, class, male, female, child, infant, bag_weight, rk_weight) "
                                   "VALUES(:point_id,:subclass,:class,:male,:female,:child,:infant,:bag_weight,:rk_weight); "
         "END IF; "
       "END; ",
    QParams() << QParam("point_id", otInteger, point_id)
              << QParam("subclass", otString)
              << QParam("class", otString)
              << QParam("male", otInteger)
              << QParam("female", otInteger)
              << QParam("child", otInteger)
              << QParam("infant", otInteger)
              << QParam("bag_weight", otInteger)
              << QParam("rk_weight", otInteger), STDLOG);

  for(map<TStatBySubclsKey, TStatBySubclsItem>::const_iterator i=stat.begin(); i!=stat.end(); ++i)
  {
    Qry.get().SetVariable("subclass", i->first.subcl);
    Qry.get().SetVariable("class", i->first.cl);
    Qry.get().SetVariable("male", i->second.male);
    Qry.get().SetVariable("female", i->second.female);
    Qry.get().SetVariable("child", i->second.child);
    Qry.get().SetVariable("infant", i->second.infant);
    Qry.get().SetVariable("bag_weight", i->second.bag_weight);
    Qry.get().SetVariable("rk_weight", i->second.rk_weight);
    Qry.get().Execute();
  };
}

void recountBySubcls(int point_id)
{
  DbCpp::Session& sess = PgOra::getRWSession({"COUNTERS_BY_SUBCLS", "PAX_GRP", "PAX", "BAG2"});
  make_db_curs(
     "DELETE FROM counters_by_subcls WHERE point_id = :point_id",
      sess)
     .stb()
     .bind(":point_id", point_id)
     .exec();

  std::string cte =
    "WITH cte_grps AS ( "
     "SELECT grp_id, "
            "class, "
            "bag_refuse "
       "FROM pax_grp "
      "WHERE pax_grp.point_dep = :point_id "
        "AND pax_grp.status <> :status), "
    "cte_pax AS ( "
     "SELECT COALESCE(pax.cabin_class, cte_grps.class) AS class, "
            "subclass, "
            "is_female, "
            "pers_type, "
            "pax.grp_id, "
            "pax.bag_pool_num "
       "FROM cte_grps JOIN pax "
         "ON cte_grps.grp_id = pax.grp_id "
        "AND pr_brd IS NOT NULL) ";
  std::string insert =
     "INSERT INTO counters_by_subcls(point_id, class, subclass, male, female, child, infant, rk_weight, bag_weight) ";
  std::string main =
     "SELECT :point_id as point_id, "
            "a.class, "
            "a.subclass, "
            "a.male, "
            "a.female, "
            "a.child, "
            "a.infant, "
            "b.rk_weight, "
            "b.bag_weight "
     "FROM ( "
     "SELECT class, "
            "subclass, "
            "COALESCE(SUM(CASE WHEN pers_type = :adl "
                               "AND COALESCE(is_female, 0) = 0 THEN 1 END), 0) AS male, "
            "COALESCE(SUM(CASE WHEN pers_type = :adl "
                               "AND is_female = 1  THEN 1 END), 0) AS female, "
            "COALESCE(SUM(CASE pers_type WHEN :chd THEN 1 END), 0) AS child, "
            "COALESCE(SUM(CASE pers_type WHEN :inf THEN 1 END), 0) AS infant "
       "FROM cte_pax "
      "GROUP BY class, subclass) a "
      "INNER JOIN ( "
     "SELECT class as class, "
            "subclass as subclass, "
            "COALESCE(SUM(CASE WHEN pr_cabin = 1 THEN weight END), 0) rk_weight, "
            "COALESCE(SUM(CASE WHEN pr_cabin = 0 THEN weight END), 0) bag_weight "
       "FROM cte_pax c LEFT OUTER JOIN bag2 "
         "ON c.grp_id = bag2.grp_id "
        "AND c.bag_pool_num = bag2.bag_pool_num "
      "GROUP BY class, subclass) b "
         "ON a.class = b.class "
        "AND a.subclass = b.subclass "
    "UNION "
     "SELECT :point_id as point_id, "
            "class, "
            "NULL AS subclass, "
            "0 AS male, "
            "0 AS female, "
            "0 AS child, "
            "0 AS infant, "
            "COALESCE(SUM(CASE WHEN pr_cabin = 1 THEN weight END), 0) rk_weight, "
            "COALESCE(SUM(CASE WHEN pr_cabin = 0 THEN weight END), 0) bag_weight "
       "FROM cte_grps JOIN bag2 "
         "ON cte_grps.grp_id = bag2.grp_id "
        "AND class IS NULL "
        "AND bag_refuse = 0 "
      "GROUP BY class ";

  make_db_curs(
      sess.isOracle()
        ? (insert + cte + main)
        : (cte + insert + main),
      sess)
     .stb()
     .bind(":point_id", point_id)
     .bind(":adl",      EncodePerson(ASTRA::adult))
     .bind(":chd",      EncodePerson(ASTRA::child))
     .bind(":inf",      EncodePerson(ASTRA::baby))
     .bind(":status",   EncodePaxStatus(ASTRA::psCrew))
     .exec();
}

