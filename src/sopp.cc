#include <stdlib.h>
#include "sopp.h"
#define NICKNAME "DJEK"
#include "setup.h"
#include "test.h"
#include "stages.h"
#include "astra_utils.h"
#include "stl_utils.h"
#include "oralib.h"
#include "xml_unit.h"
#include "basic.h"
#include "exceptions.h"
#include "sys/times.h"
#include <map>

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;

struct TSoppStage {
  int stage_id;
  TDateTime scd;
  TDateTime est;
  TDateTime act;
};

struct TTrip {
  int move_row_id;
  string company;
  int flt_no;
  string suffix;
  TDateTime scd;
  TDateTime est;
  TDateTime act;
  string bc;
  string bort;
  string park;
  int status;
  string triptype;
  string litera;
  string classes;
  string remark;
  int reg;
  int resa;
  vector<string> places;
  vector<TSoppStage> stages;
  TTrip() {
    move_row_id = -1;
    flt_no = -1;
    scd = -1;
    est = -1;
    act = -1;
    reg = 0;
    resa = 0;
    status = -1;
  }
};

inline int fillTrip( TTrip &trip, TQuery &Qry )
{
  if ( Qry.FieldIsNULL( 1 ) )
    trip.move_row_id = -1;
  else
    trip.move_row_id = Qry.FieldAsInteger( 1 );
  trip.company = Qry.FieldAsString( 2 );
  trip.flt_no = Qry.FieldAsInteger( 3 );
  trip.suffix = Qry.FieldAsString( 4 );
  trip.scd = Qry.FieldAsDateTime( 5 );
  if ( Qry.FieldIsNULL( 6 ) )
    trip.est = -1;
  else
    trip.est = Qry.FieldAsDateTime( 6 );
  if ( Qry.FieldIsNULL( 7 ) )
    trip.act = -1;
  else
    trip.act = Qry.FieldAsDateTime( 7 );
  trip.bc = Qry.FieldAsString( 8 );
  trip.bort = Qry.FieldAsString( 9 );
  trip.park = Qry.FieldAsString( 10 );
  trip.status = Qry.FieldAsInteger( 11 );
  trip.triptype = Qry.FieldAsString( 12 );
  trip.litera = Qry.FieldAsString( 13 );
  trip.classes = Qry.FieldAsString( 14 );
  trip.remark = Qry.FieldAsString( 15 );
  return Qry.FieldAsInteger( 0 );
}

inline void buildTrip( TTrip &trip, xmlNodePtr outNode )
{
  if ( trip.move_row_id >= 0 )
    NewTextChild( outNode, "move_row_id", trip.move_row_id );
  NewTextChild( outNode, "company", trip.company );
  NewTextChild( outNode, "flt_no", trip.flt_no );
  if ( !trip.suffix.empty() )
    NewTextChild( outNode, "suffix", trip.suffix );
  NewTextChild( outNode, "scd", DateTimeToStr( trip.scd ) );
  if ( trip.est >= 0 )
    NewTextChild( outNode, "est", DateTimeToStr( trip.est ) );
  if ( trip.act >= 0 )
    NewTextChild( outNode, "act", DateTimeToStr( trip.act ) );
  NewTextChild( outNode, "bc", trip.bc );
  if ( !trip.bort.empty() )
    NewTextChild( outNode, "bort", trip.bort );
  if ( !trip.park.empty() )
    NewTextChild( outNode, "park", trip.park );
  if ( trip.status )
    NewTextChild( outNode, "status", trip.status );
  NewTextChild( outNode, "triptype", trip.triptype );
  if ( !trip.litera.empty() )
    NewTextChild( outNode, "litera", trip.litera );
  if ( !trip.classes.empty() )
    NewTextChild( outNode, "classes", trip.classes );
  if ( !trip.remark.empty() )
    NewTextChild( outNode, "remark", trip.remark );
  if ( trip.reg )
    NewTextChild( outNode, "reg", trip.reg );
  if ( trip.resa )
    NewTextChild( outNode, "resa", trip.resa );
  xmlNodePtr node = NewTextChild( outNode, "places" );
  for ( vector<string>::iterator isp=trip.places.begin(); isp!=trip.places.end(); isp++ ) {
    NewTextChild( node, "cod", *isp );
  }
  if ( !trip.stages.empty() ) {
    xmlNodePtr node = NewTextChild( outNode, "stages" );
    for ( vector<TSoppStage>::iterator iss=trip.stages.begin(); iss!=trip.stages.end(); iss++ ) {
      xmlNodePtr n = NewTextChild( node, "stage" );
      NewTextChild( n, "stage_id", iss->stage_id );
      NewTextChild( n, "scd", DateTimeToStr( iss->scd ) );
      if ( iss->est >= 0 )
        NewTextChild( n, "est", DateTimeToStr( iss->est ) );
      if ( iss->act >= 0 )
        NewTextChild( n, "act", DateTimeToStr( iss->act ) );
    }
  }
}

void SoppInterface::ReadTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace( TRACE5, "ReadTrips" );
  TQuery OutQry( &OraSession );
  OutQry.SQLText =
    "SELECT trip_id,move_row_id,company,flt_no,suffix,scd,est,act,bc,bort,park,status,triptype,litera, "\
    "       SUBSTR( ckin.get_classes( trips.trip_id ), 1, 24 ) as classes, "\
    "       remark "\
    " FROM trips "
    "WHERE status!=-1 "\
    "ORDER BY trip_id ";
  TQuery InQry( &OraSession );
  InQry.SQLText =
    "SELECT trip_id,move_row_id,company,flt_no,suffix,scd,est,act,bc,bort,park,status,triptype,litera, "\
    "       '',remark "\
    " FROM trips_in "\
    "WHERE status!=-1 "\
    "ORDER BY trip_id ";
  TQuery RegQry( &OraSession );
  RegQry.SQLText =
    "SELECT pax_grp.point_id as trip_id, SUM(pax.seats) as reg FROM pax_grp, pax "\
    " WHERE pax_grp.grp_id=pax.grp_id AND pax.pr_brd IS NOT NULL "\
    "GROUP BY pax_grp.point_id "\
    "ORDER BY pax_grp.point_id ";
  TQuery ResaQry( &OraSession );
  ResaQry.SQLText =
    "SELECT point_id as trip_id,SUM(crs_ok) as resa FROM counters2 "\
    " GROUP BY point_id "\
    "ORDER BY point_id ";
  TQuery PlacesQry( &OraSession );
  PlacesQry.SQLText =
    "SELECT trip_id,num,cod FROM place"\
    " WHERE num > 0 "\
    " UNION "\
    "SELECT trip_id,num,cod FROM place_in "\
    " WHERE num < 0 "\
    " ORDER BY trip_id,num ";
  TQuery StagesQry( &OraSession );
  StagesQry.SQLText =
    "SELECT point_id,stage_id,scd,est,act FROM trip_stages "\
    " ORDER BY point_id,stage_id ";
  OutQry.Execute();
  InQry.Execute();
  RegQry.Execute();
  ResaQry.Execute();
  PlacesQry.Execute();
  StagesQry.Execute();
  map<int,TTrip> trips, trips_in;
  int trip_id;
  tst();
  while ( !OutQry.Eof ) {
    TTrip trip;
    trip_id = fillTrip( trip, OutQry );
    tst();
    while ( !RegQry.Eof && trip_id >= RegQry.FieldAsInteger( "trip_id" ) ) {
      if ( trip_id == RegQry.FieldAsInteger( "trip_id" ) ) {
      	trip.reg = RegQry.FieldAsInteger( "reg" );
      	break;
      }
      RegQry.Next();
    }
    tst();
    while ( !ResaQry.Eof && trip_id >= ResaQry.FieldAsInteger( "trip_id" ) ) {
      if ( trip_id == ResaQry.FieldAsInteger( "trip_id" ) ) {
      	trip.resa = ResaQry.FieldAsInteger( "resa" );
      	break;
      }
      ResaQry.Next();
    }
    tst();
    while ( !PlacesQry.Eof && trip_id >= PlacesQry.FieldAsInteger( "trip_id" ) ) {
      if ( trip_id == PlacesQry.FieldAsInteger( "trip_id" ) ) {
      	trip.places.push_back( PlacesQry.FieldAsString( "cod" ) );
      }
      PlacesQry.Next();
    }
    tst();
    TSoppStage stage;
    while ( !StagesQry.Eof && trip_id >= StagesQry.FieldAsInteger( "point_id" ) ) {
      if ( trip_id == StagesQry.FieldAsInteger( "point_id" ) ) {
      	stage.stage_id = StagesQry.FieldAsInteger( "stage_id" );
      	stage.scd = StagesQry.FieldAsDateTime( "scd" );
      	if ( StagesQry.FieldIsNULL( "est" ) )
      	  stage.est = -1;
      	else
      	  stage.est = StagesQry.FieldAsDateTime( "est" );
      	if ( StagesQry.FieldIsNULL( "act" ) )
      	  stage.act = -1;
      	else
      	  stage.act = StagesQry.FieldAsDateTime( "act" );
      	trip.stages.push_back( stage );
      }

      StagesQry.Next();
    }
    tst();
    trips.insert( make_pair( trip_id, trip ) );
    OutQry.Next();
  }
  while ( !InQry.Eof ) {
    TTrip trip;
    trip_id = fillTrip( trip, InQry );
    trips_in.insert( make_pair( trip_id, trip ) );
    InQry.Next();
  }
  tst();
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  dataNode = NewTextChild( dataNode, "trips" );
  map<int,TTrip>::iterator itrip=trips.begin(), jtrip=trips_in.begin();
  while ( itrip != trips.end() || jtrip != trips_in.end() ) {
    xmlNodePtr tripNode = NewTextChild( dataNode, "trip" );
    if ( itrip != trips.end() && jtrip != trips_in.end() ) {
      if ( itrip->first == jtrip->first ) {
        NewTextChild( tripNode, "trip_id", itrip->first );
        buildTrip( itrip->second, NewTextChild( tripNode, "out" ) );
        buildTrip( jtrip->second, NewTextChild( tripNode, "in" ) );
        itrip++;
        jtrip++;
      }
      else
        if ( itrip->first < jtrip->first ) {
          NewTextChild( tripNode, "trip_id", itrip->first );
          buildTrip( itrip->second, NewTextChild( tripNode, "out" ) );
          itrip++;
        }
        else {
          NewTextChild( tripNode, "trip_id", jtrip->first );
          buildTrip( jtrip->second, NewTextChild( tripNode, "in" ) );
          jtrip++;
        }
    }
    else
      if ( itrip == trips.end() ) {
        NewTextChild( tripNode, "trip_id", jtrip->first );
        buildTrip( jtrip->second, NewTextChild( tripNode, "in" ) );
        jtrip++;
      }
      else {
        NewTextChild( tripNode, "trip_id", itrip->first );
        buildTrip( itrip->second, NewTextChild( tripNode, "out" ) );
        itrip++;
      }
  }

}

void SoppInterface::GetTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode,
                                bool pr_bag)
{
  TQuery Qry(&OraSession);
  bool pr_out=NodeAsInteger("pr_out",reqNode)!=0;
  int point_id=NodeAsInteger("point_id",reqNode);
  if (pr_out)



    Qry.SQLText=
      "SELECT company AS airline,flt_no,suffix, "
      "       TO_CHAR(NVL(act,NVL(est,scd)),'DD')|| "
      "       DECODE(TRUNC(NVL(act,NVL(est,scd))),TRUNC(scd),'', "
      "              TO_CHAR(scd,'(DD)')) AS scd "
      "FROM trips "
      "WHERE trip_id=:point_id AND status>=0";
  else
    Qry.SQLText=
      "SELECT company AS airline,flt_no,suffix, "
      "       TO_CHAR(NVL(act,NVL(est,scd)),'DD')|| "
      "       DECODE(TRUNC(NVL(act,NVL(est,scd))),TRUNC(scd),'', "
      "              TO_CHAR(scd,'(DD)')) AS scd "
      "FROM trips_in "
      "WHERE trip_id=:point_id AND status>=0";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс не найден");
  ostringstream trip;
  trip << Qry.FieldAsString("airline")
       << Qry.FieldAsInteger("flt_no")
       << Qry.FieldAsString("suffix") << "/"
       << Qry.FieldAsString("scd");


  NewTextChild(resNode,"trip",trip.str());

  Qry.Clear();
  if (pr_out)
    Qry.SQLText=
      "SELECT tlg_trips.point_id,tlg_trips.airline,tlg_trips.flt_no,tlg_trips.suffix, "
      "       tlg_trips.scd,tlg_trips.airp_dep AS airp, "
      "       tlg_transfer.trfer_id,tlg_transfer.subcl_in AS subcl, "
      "       trfer_grp.grp_id,seats,bag_amount,bag_weight,rk_weight,weight_unit "
      "FROM trfer_grp,tlgs_in,tlg_trips,tlg_transfer,tlg_binding "
      "WHERE tlg_transfer.tlg_id=tlgs_in.id AND tlgs_in.num=1 AND "
      "      tlgs_in.type=:tlg_type AND "
      "      tlg_transfer.point_id_out=tlg_binding.point_id_tlg AND "
      "      tlg_binding.point_id_spp=:point_id AND "
      "      tlg_trips.point_id=tlg_transfer.point_id_in AND "
      "      tlg_transfer.trfer_id=trfer_grp.trfer_id "
      "ORDER BY tlg_trips.scd,tlg_trips.airline,tlg_trips.flt_no,tlg_trips.suffix, "
      "         tlg_trips.airp_dep,tlg_transfer.trfer_id ";
  else
    Qry.SQLText=
      "SELECT tlg_trips.point_id,tlg_trips.airline,tlg_trips.flt_no,tlg_trips.suffix, "
      "       tlg_trips.scd,tlg_trips.airp_arv AS airp, "
      "       tlg_transfer.trfer_id,tlg_transfer.subcl_out AS subcl, "
      "       trfer_grp.grp_id,seats,bag_amount,bag_weight,rk_weight,weight_unit "
      "FROM trfer_grp,tlgs_in,tlg_trips,tlg_transfer,tlg_binding "
      "WHERE tlg_transfer.tlg_id=tlgs_in.id AND tlgs_in.num=1 AND "
      "      tlgs_in.type=:tlg_type AND "
      "      tlg_transfer.point_id_in=tlg_binding.point_id_tlg AND "
      "      tlg_binding.point_id_spp=:point_id AND "
      "      tlg_trips.point_id=tlg_transfer.point_id_out AND "
      "      tlg_transfer.trfer_id=trfer_grp.trfer_id "
      "ORDER BY tlg_trips.scd,tlg_trips.airline,tlg_trips.flt_no,tlg_trips.suffix, "
      "         tlg_trips.airp_dep,tlg_transfer.trfer_id ";
  Qry.CreateVariable("point_id",otInteger,point_id);
  if (pr_bag)
    Qry.CreateVariable("tlg_type",otString,"BTM");
  else
    Qry.CreateVariable("tlg_type",otString,"PTM");

  TQuery PaxQry(&OraSession);
  PaxQry.SQLText="SELECT surname,name FROM trfer_pax WHERE grp_id=:grp_id ORDER BY surname,name";
  PaxQry.DeclareVariable("grp_id",otInteger);

  TQuery TagQry(&OraSession);
  TagQry.SQLText=
    "SELECT TRUNC(no/1000) AS pack, "
    "       MOD(no,1000) AS no "
    "FROM trfer_tags WHERE grp_id=:grp_id ORDER BY trfer_tags.no";
  TagQry.DeclareVariable("grp_id",otFloat);

  Qry.Execute();
  xmlNodePtr trferNode=NewTextChild(resNode,"transfer");
  xmlNodePtr grpNode,paxNode,node,node2;
  int grp_id;
  char subcl[2];
  point_id=-1;
  *subcl=0;
  for(;!Qry.Eof;Qry.Next())
  {
    if (point_id==-1 ||
        point_id!=Qry.FieldAsInteger("point_id") ||
        strcmp(subcl,Qry.FieldAsString("subcl"))!=0)
    {
      node=NewTextChild(trferNode,"trfer_flt");
      ostringstream trip;
      trip << Qry.FieldAsString("airline")
           << Qry.FieldAsInteger("flt_no")
           << Qry.FieldAsString("suffix") << "/"
           << DateTimeToStr(Qry.FieldAsDateTime("scd"),"dd");

      NewTextChild(node,"trip",trip.str());
      NewTextChild(node,"airp",Qry.FieldAsString("airp"));
      NewTextChild(node,"subcl",Qry.FieldAsString("subcl"));
      grpNode=NewTextChild(node,"grps");
      point_id=Qry.FieldAsInteger("point_id");
      strcpy(subcl,Qry.FieldAsString("subcl"));
    };

    node=NewTextChild(grpNode,"grp");
    if (!Qry.FieldIsNULL("seats"))
      NewTextChild(node,"seats",Qry.FieldAsInteger("seats"));
    else
      NewTextChild(node,"seats");
    if (!Qry.FieldIsNULL("bag_amount"))
      NewTextChild(node,"bag_amount",Qry.FieldAsInteger("bag_amount"));
    else
      NewTextChild(node,"bag_amount");
    if (!Qry.FieldIsNULL("bag_weight"))
      NewTextChild(node,"bag_weight",Qry.FieldAsInteger("bag_weight"));
    else
      NewTextChild(node,"bag_weight");
    if (!Qry.FieldIsNULL("rk_weight"))
      NewTextChild(node,"rk_weight",Qry.FieldAsInteger("rk_weight"));
    else
      NewTextChild(node,"rk_weight");
    NewTextChild(node,"weight_unit",Qry.FieldAsString("weight_unit"));

    grp_id=Qry.FieldAsInteger("grp_id");

    PaxQry.SetVariable("grp_id",grp_id);
    PaxQry.Execute();
    if (!PaxQry.Eof)
    {
      paxNode=NewTextChild(node,"passengers");
      for(;!PaxQry.Eof;PaxQry.Next())
      {
        node2=NewTextChild(paxNode,"pax");
        NewTextChild(node2,"surname",PaxQry.FieldAsString("surname"));
        NewTextChild(node2,"name",PaxQry.FieldAsString("name"),"");
      };
    };

    if (pr_bag)
    {
      TagQry.SetVariable("grp_id",grp_id);
      TagQry.Execute();
      if (!TagQry.Eof)
      {
        int first_no,first_pack,curr_no,curr_pack;
        first_no=TagQry.FieldAsInteger("no");
        first_pack=TagQry.FieldAsInteger("pack");
        int num=0;
        node2=NewTextChild(node,"tag_ranges");
        for(;;TagQry.Next())
        {
          if (!TagQry.Eof)
          {
            curr_no=TagQry.FieldAsInteger("no");
            curr_pack=TagQry.FieldAsInteger("pack");
          };

          if (TagQry.Eof||
              first_pack!=curr_pack||
              first_no+num!=curr_no)
          {
            ostringstream range;
            range.setf(ios::fixed);
            range << setw(10) << setfill('0') << setprecision(0)
                  << (first_pack*1000.0+first_no);
            if (num!=1)
              range << "-"
                    << setw(3)  << setfill('0')
                    << (first_no+num-1);
            NewTextChild(node2,"range",range.str());

            if (TagQry.Eof) break;
            first_no=curr_no;
            first_pack=curr_pack;
            num=0;
          };
          num++;
        };
      };
    };
  };
};

void SoppInterface::GetPaxTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  GetTransfer(ctxt,reqNode,resNode,false);
};

void SoppInterface::GetBagTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  GetTransfer(ctxt,reqNode,resNode,true);
};
