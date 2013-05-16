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
         priority_class[ cl ].priority == priority && priority_class[ cl ].id < p->id ) {
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
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT point_id FROM points WHERE point_id=:point_id FOR UPDATE";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof )
    throw Exception( "Flight not found" );
  Qry.Clear();
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
  string msg = "Назначение весов пассажиров на рейс: ";
  bool pr_sep = false;
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
      if ( pr_sep )
        msg += ",";
      msg += string("кл.=") + i->cl;
      pr_sep =true;
    }
    if ( !i->subcl.empty() ) {
      if ( pr_sep )
        msg += ",";
      msg += string("подкл.=") + i->subcl;
      pr_sep = true;
    }
    if ( pr_sep )
      msg += ",";
    if ( i->female == ASTRA::NoExists )
      msg += string("ВЗ=") + IntToString( i->male );
    else
      msg += string("М=") + IntToString( i->male ) + ",Ж=" + IntToString( i->female );
    msg += ",";
    msg += string("РБ=") + IntToString( i->child );
    msg += ",";
    msg += string("РМ=") + IntToString( i->infant );
    pr_sep = true;
  }
  vector<string> strs;
  SeparateString( msg, 250, strs );
  for ( vector<string>::iterator i=strs.begin(); i!=strs.end(); i++ ) {
    TReqInfo::Instance()->MsgToLog( *i, evtFlt, point_id );
  }
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
         max_priority == priority && max_id < i->id ) {
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

void TFlightWeights::read( int point_id, TTypeFlightWeight weight_type )
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
      "       NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax_doc.gender,:male,1,NULL,1,0),0)),0) AS male, "
      "       NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax_doc.gender,:female,1,0),0)),0) AS female, "
      "       NVL(SUM(DECODE(pax.pers_type,:chd,1,0)),0) AS child, "
      "       NVL(SUM(DECODE(pax.pers_type,:inf,1,0)),0) AS infant, "
      "       NVL(SUM(ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum)),0) rkweight,"
      "       NVL(SUM(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum)),0) bagweight"
      " FROM pax_grp, pax, pax_doc"
      " WHERE pax_grp.grp_id=pax.grp_id AND "
      "       pax.pax_id=pax_doc.pax_id(+)";
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
  str +=
    " AND point_dep=:point_dep "
    " GROUP BY class, subclass";
  ProgTrace( TRACE5, "str=%s", str.c_str() );
  Qry.SQLText = str.c_str();
  Qry.CreateVariable( "point_dep", otInteger, point_id );
  Qry.CreateVariable( "adl", otString, string(EncodePerson( ASTRA::adult )) );
  Qry.CreateVariable( "chd", otString, string(EncodePerson( ASTRA::child )) );
  Qry.CreateVariable( "inf", otString, string(EncodePerson( ASTRA::baby )) );
  if ( pr_female ) {
    Qry.CreateVariable( "male", otString, "M" );
    Qry.CreateVariable( "female", otString, "F" );
  }
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
	  " WHERE point_dep=:point_dep AND class IS NULL";
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
	w.read( point_id, weight_type );
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

