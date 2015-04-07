#include <stdlib.h>
#include "astra_consts.h"
#include "basic.h"
#include "exceptions.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "stl_utils.h"
#include "oralib.h"
#include "term_version.h"
#include "pers_weights.h"
#include "points.h"
#include "events.h"
#include "qrys.h"

#include "serverlib/perfom.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;
using namespace ASTRA;
using namespace EXCEPTIONS;
using namespace boost::local_time;

////////////////////////////////////////TPersWeights////////////////////////////
void TPersWeights::Update()
{
  weights.clear();
  TPerTypeWeight p;
  TQuery Qry(&OraSession);
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
    catch( EConvertError ) { // high year????
      TDateTime ddd = date + diffday;
      DecodeDate( ddd, Tmp, Month, Day );
      EncodeDate( Year, Month, Day, date );
      return date;
    }
  }
}

void TPersWeights::getRules( const BASIC::TDateTime &scd_utc, const std::string &airline,
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
        good_cond = ( is_dst( scd_utc, region ) == p->pr_summer );
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
  TQuery Qry(&OraSession);
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
  TQuery Qry(&OraSession);
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

void PersWeightRules::write( int point_id )
{
  ProgTrace( TRACE5, "PersWeightRules::write point_id=%d", point_id );
  TFlights fligths;
  fligths.Get( point_id, ftTranzit );
  if ( fligths.empty() )
    throw Exception( "Flight not found" );
  fligths.Lock();
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "DELETE trip_pers_weights WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO trip_pers_weights(point_id,class,subclass,male,female,child,infant)"
    " VALUES(:point_id,:class,:subclass,:male,:female,:child,:infant)";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.DeclareVariable( "class", otString );
  Qry.DeclareVariable( "subclass", otString );
  Qry.DeclareVariable( "male", otInteger );
  Qry.DeclareVariable( "female", otInteger );
  Qry.DeclareVariable( "child", otInteger );
  Qry.DeclareVariable( "infant", otInteger );
  ProgTrace( TRACE5, "weights.size()=%zu", weights.size() );
  std::string lexema_id = "EVT.PERS_WEIGHTS";
  PrmEnum prmenum("weights", ", ");
  for ( std::vector<ClassesPersWeight>::iterator i=weights.begin(); i!=weights.end(); i++ ) {
    Qry.SetVariable( "class", i->cl );
    Qry.SetVariable( "subclass", i->subcl );
    Qry.SetVariable( "male", i->male );
    if ( i->female == ASTRA::NoExists )
      Qry.SetVariable( "female", FNull );
    else
      Qry.SetVariable( "female", i->female );
    Qry.SetVariable( "child", i->child );
    Qry.SetVariable( "infant", i->infant );
    Qry.Execute();
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
    TReqInfo::Instance()->LocaleToLog(lexema_id, LEvntPrms() << prmenum, evtFlt, point_id);
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
  TQuery Qry(&OraSession);

  string str;
  bool pr_female = r.isFemale();
  if ( pr_female ) // надо учитывать отдельно веса мужчин и женщин
    str =
      "SELECT class, subclass, "
      "       NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax.is_female,0,1,NULL,1,0),0)),0) AS male, "
      "       NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax.is_female,0,0,NULL,0,1),0)),0) AS female, "
      "       NVL(SUM(DECODE(pax.pers_type,:chd,1,0)),0) AS child, "
      "       NVL(SUM(DECODE(pax.pers_type,:inf,1,0)),0) AS infant, "
      "       NVL(SUM(ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum)),0) rkweight,"
      "       NVL(SUM(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum)),0) bagweight"
      " FROM pax_grp, pax "
      " WHERE pax_grp.grp_id=pax.grp_id";
  else
    str =
      "SELECT class, subclass, "
      "       NVL(SUM(DECODE(pax.pers_type,:adl,1,0)),0) AS male, "
      "       0 AS female, "
      "       NVL(SUM(DECODE(pax.pers_type,:chd,1,0)),0) AS child, "
      "       NVL(SUM(DECODE(pax.pers_type,:inf,1,0)),0) AS infant, "
      "       NVL(SUM(ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum)),0) rkweight,"
      "       NVL(SUM(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum)),0) bagweight"
      " FROM pax_grp, pax"
      " WHERE pax_grp.grp_id=pax.grp_id";
  if ( weight_type == withBrd )
    str += " AND pr_brd=1 ";
  else
    str += " AND pr_brd IS NOT NULL ";
  if(not include_wait_list)
      str += " and salons.is_waitlist(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,rownum)=0 ";
  str +=
    " AND point_dep=:point_dep "
    " AND pax_grp.status NOT IN ('E') "
    " GROUP BY class, subclass";
  ProgTrace( TRACE5, "str=%s", str.c_str() );
  Qry.SQLText = str.c_str();
  Qry.CreateVariable( "point_dep", otInteger, point_id );
  Qry.CreateVariable( "adl", otString, string(EncodePerson( ASTRA::adult )) );
  Qry.CreateVariable( "chd", otString, string(EncodePerson( ASTRA::child )) );
  Qry.CreateVariable( "inf", otString, string(EncodePerson( ASTRA::baby )) );
  Qry.Execute();
  int m,f,c,i;
  while ( !Qry.Eof ) {
    m = Qry.FieldAsInteger( "male" );
    f = Qry.FieldAsInteger( "female" );
    c = Qry.FieldAsInteger( "child" );
    i = Qry.FieldAsInteger( "infant" );
    male += m;
    female += f;
    child += c;
    infant += i;
    ClassesPersWeight weight;
    if ( r.weight( Qry.FieldAsString( "class" ), Qry.FieldAsString( "subclass" ), weight ) ) {
      weight_male += m*weight.male;
      weight_female += f*weight.female;
      weight_child += c*weight.child;
      weight_infant += i*weight.infant;
      weight_cabin_bag += Qry.FieldAsInteger( "rkweight" );
      weight_bag += Qry.FieldAsInteger( "bagweight" );
    }
    Qry.Next();
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT NVL(SUM(ckin.get_bagWeight2(grp_id,NULL,NULL,rownum)),0) unnacomp_bag "
    " FROM pax_grp "
      " WHERE point_dep=:point_dep AND class IS NULL AND pax_grp.status NOT IN ('E')";
  Qry.CreateVariable( "point_dep", otInteger, point_id );
  Qry.Execute();
  if ( !Qry.Eof )
    weight_bag += Qry.FieldAsInteger( "unnacomp_bag" );
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
        stat.insert(make_pair(TStatBySubclsKey(pax.subcl, pax.cl), TStatBySubclsItem())).first;
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

  TCachedQuery Qry("BEGIN "
                   "  UPDATE counters_by_subcls "
                   "  SET male=male+:male, female=female+:female, "
                   "      child=child+:child, infant=infant+:infant, "
                   "      bag_weight=bag_weight+:bag_weight, rk_weight=rk_weight+:rk_weight "
                   "  WHERE point_id=:point_id AND "
                   "        (subclass IS NULL AND :subclass IS NULL OR subclass=:subclass) AND "
                   "        (class IS NULL AND :class IS NULL OR class=:class); "
                   "  IF SQL%NOTFOUND THEN "
                   "    INSERT INTO counters_by_subcls(point_id, subclass, class, male, female, child, infant, bag_weight, rk_weight) "
                   "    VALUES(:point_id, :subclass, :class, :male, :female, :child, :infant, :bag_weight, :rk_weight); "
                   "  END IF; "
                   "END; ",
                   QParams() << QParam("point_id", otInteger, point_id)
                             << QParam("subclass", otString)
                             << QParam("class", otString)
                             << QParam("male", otInteger)
                             << QParam("female", otInteger)
                             << QParam("child", otInteger)
                             << QParam("infant", otInteger)
                             << QParam("bag_weight", otInteger)
                             << QParam("rk_weight", otInteger));


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
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  DELETE FROM counters_by_subcls WHERE point_id=:point_id; "
    "  INSERT INTO counters_by_subcls(point_id, class, subclass, male, female, child, infant, rk_weight, bag_weight)"
    "  SELECT :point_id, class, subclass, "
    "         NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax.is_female,0,1,NULL,1,0),0)),0) AS male, "
    "         NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax.is_female,0,0,NULL,0,1),0)),0) AS female, "
    "         NVL(SUM(DECODE(pax.pers_type,:chd,1,0)),0) AS child, "
    "         NVL(SUM(DECODE(pax.pers_type,:inf,1,0)),0) AS infant, "
    "         NVL(SUM(ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum)),0) rk_weight,"
    "         NVL(SUM(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum)),0) bag_weight"
    "  FROM pax_grp, pax "
    "  WHERE pax_grp.grp_id=pax.grp_id AND "
    "        pr_brd IS NOT NULL AND "
    "        point_dep=:point_id AND "
    "        pax_grp.status NOT IN ('E') "
    "  GROUP BY class, subclass "
    "  UNION "
    "  SELECT :point_id, class, NULL, 0, 0, 0, 0, "
    "         NVL(SUM(ckin.get_rkWeight2(grp_id,NULL,NULL,rownum)),0) rk_weight, "
    "         NVL(SUM(ckin.get_bagWeight2(grp_id,NULL,NULL,rownum)),0) bag_weight "
    "  FROM pax_grp "
    "  WHERE point_dep=:point_id AND class IS NULL AND pax_grp.status NOT IN ('E') AND bag_refuse=0 "
    "  GROUP BY class; "
    "END;";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "adl", otString, string(EncodePerson( ASTRA::adult )) );
  Qry.CreateVariable( "chd", otString, string(EncodePerson( ASTRA::child )) );
  Qry.CreateVariable( "inf", otString, string(EncodePerson( ASTRA::baby )) );
  Qry.Execute();
}

//!!! потом удалить check_counters_by_subcls и alter_wait2

void alter_wait2(int processed, bool commit_before_sleep=false, int work_secs=5, int sleep_secs=5)
{
  static time_t start_time=time(NULL);
  if (time(NULL)-start_time>=work_secs)
  {
    if (commit_before_sleep) OraSession.Commit();
    printf("%d iterations processed. sleep...", processed);
    fflush(stdout);
    sleep(sleep_secs);
    printf("go!\n");
    start_time=time(NULL);
  };
};

int check_counters_by_subcls(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT point_id AS max_point_id FROM last_processed_point_id";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_point_id"))
  {
    Qry.Clear();
    Qry.SQLText="SELECT max(point_id) AS max_point_id FROM points";
    Qry.Execute();
    if (Qry.Eof || Qry.FieldIsNULL("max_point_id")) return 0;
  };
  int max_point_id=Qry.FieldAsInteger("max_point_id");

  Qry.Clear();
  Qry.SQLText="SELECT min(point_id) AS min_point_id FROM points";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_point_id")) return 0;
  int min_point_id=Qry.FieldAsInteger("min_point_id");

  Qry.Clear();
  Qry.SQLText=
    "SELECT point_id FROM points "
    "WHERE point_id>:low_point_id AND point_id<=:high_point_id AND "
    "      pr_reg<>0 AND pr_del>=0 ";
  Qry.DeclareVariable("low_point_id", otInteger);
  Qry.DeclareVariable("high_point_id", otInteger);

  TQuery UpdQry(&OraSession);
  UpdQry.Clear();
  UpdQry.SQLText="UPDATE last_processed_point_id SET point_id=:point_id";
  UpdQry.DeclareVariable("point_id", otInteger);

  TQuery CountersQry(&OraSession);
  CountersQry.Clear();
  CountersQry.SQLText="SELECT * FROM counters_by_subcls WHERE point_id=:point_id";
  CountersQry.DeclareVariable("point_id", otInteger);


  int processed=0;
  for(int curr_point_id=max_point_id; curr_point_id>=min_point_id; curr_point_id-=50000)
  {
    Qry.SetVariable("low_point_id", curr_point_id-50000);
    Qry.SetVariable("high_point_id", curr_point_id);
    Qry.Execute();
    list<int> point_ids;
    for(;!Qry.Eof;Qry.Next()) point_ids.push_back(Qry.FieldAsInteger("point_id"));
    Qry.Close();

    printf("processed range: (%d; %d] count=%zu\n", curr_point_id-50000, curr_point_id, point_ids.size());

    for(list<int>::const_iterator i=point_ids.begin(); i!=point_ids.end(); ++i)
    {
      try
      {
        TFlights fligths;
        fligths.Get( *i, ftTranzit );
        fligths.Lock();

        map<TStatBySubclsKey, TStatBySubclsItem> stat0;
        map<TStatBySubclsKey, TStatBySubclsItem> stat1;
        for(int pass=0; pass<2; pass++)
        {
          if (pass==1) recountBySubcls(*i);
          CountersQry.SetVariable("point_id", *i);
          CountersQry.Execute();
          for(; !CountersQry.Eof; CountersQry.Next())
          {
            TStatBySubclsKey key(CountersQry.FieldAsString("subclass"),
                                 CountersQry.FieldAsString("class"));
            TStatBySubclsItem item;
            item.male=CountersQry.FieldAsInteger("male");
            item.female=CountersQry.FieldAsInteger("female");
            item.child=CountersQry.FieldAsInteger("child");
            item.infant=CountersQry.FieldAsInteger("infant");
            item.bag_weight=CountersQry.FieldAsInteger("bag_weight");
            item.rk_weight=CountersQry.FieldAsInteger("rk_weight");
            if (item==TStatBySubclsItem()) continue;
            if (pass==0)
              stat0.insert(make_pair(key, item));
            else
              stat1.insert(make_pair(key, item));
          };
        };
        OraSession.Rollback();

        if (!(stat0==stat1))
        {
          printf("wrong counters! point_id=%d\n", *i);
          ProgError(STDLOG, "wrong counters! point_id=%d", *i);
        };

        UpdQry.SetVariable("point_id", *i);
        UpdQry.Execute();
        OraSession.Commit();

        alter_wait2(processed++/*, false, 9, 1*/);
      }
      catch(...)
      {
        OraSession.Rollback();
        printf("error! point_id=%d\n", *i);
        ProgError(STDLOG, "error! point_id=%d", *i);
        throw;
      };
    };

  };
  return 0;
}



