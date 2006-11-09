#include <vector>
#define NICKNAME "VLAD"
#include "setup.h"
#include "telegram.h"
#include "xml_unit.h"
#include "oralib.h"
#include "exceptions.h"
#include "astra_utils.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;

void TelegramInterface::readTripData( int point_id, xmlNodePtr dataNode )
{
  xmlNodePtr tripdataNode = NewTextChild( dataNode, "tripdata" );
  xmlNodePtr node;

  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT point_num, DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
    "FROM points WHERE point_id=:point_id";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс не найден. Обновите данные");
  int first_point=Qry.FieldAsInteger("first_point");
  int point_num=Qry.FieldAsInteger("point_num");

  Qry.Clear();
  Qry.SQLText =
    "SELECT points.airp "
    "FROM points "
    "WHERE points.first_point=:first_point AND points.point_num>:point_num AND points.pr_del=0 "
    "ORDER BY point_num";
  Qry.CreateVariable("first_point",otInteger,first_point);
  Qry.CreateVariable("point_num",otInteger,point_num);
  Qry.Execute();
  node = NewTextChild( tripdataNode, "airps" );
  vector<string> airps;
  vector<string>::iterator i;
  for(;!Qry.Eof;Qry.Next())
  {
    //проверим на дублирование кодов аэропортов в рамках одного рейса
    for(i=airps.begin();i!=airps.end();i++)
      if (*i==Qry.FieldAsString( "airp" )) break;
    if (i!=airps.end()) continue;

    NewTextChild( node, "airp", Qry.FieldAsString("airp") );

    airps.push_back(Qry.FieldAsString( "airp" ));
  };
};

void TelegramInterface::GetTlgIn(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );

  TQuery Qry(&OraSession);
  string tz_region;
  string sql="SELECT tlgs_in.id,num,type,addr,heading,body,ending,time_receive "
             "FROM tlgs_in, ";
  if (point_id!=-1)
  {
    TQuery RegionQry(&OraSession);
    RegionQry.SQLText="SELECT system.AirpTZRegion(airp) AS tz_region FROM points WHERE point_id=:point_id";
    RegionQry.CreateVariable("point_id",otInteger,point_id);
    RegionQry.Execute();
    if (RegionQry.Eof) throw UserException("Рейс не найден. Обновите данные");
    tz_region = RegionQry.FieldAsString("tz_region");

    sql+="(SELECT DISTINCT tlg_source.tlg_id AS id "
         " FROM tlg_source,tlg_binding "
         " WHERE tlg_source.point_id_tlg=tlg_binding.point_id_tlg AND "
         "       tlg_binding.point_id_spp=:point_id) ids ";
    Qry.CreateVariable("point_id",otInteger,point_id);
  }
  else
  {
    tz_region =  TReqInfo::Instance()->desk.tz_region;
    sql+="(SELECT id FROM tlgs_in WHERE time_receive>=TRUNC(sysdate)-15 "
         " MINUS "
         " SELECT tlg_source.tlg_id AS id "
         " FROM tlg_source,tlg_binding "
         " WHERE tlg_source.point_id_tlg=tlg_binding.point_id_tlg) ids ";
  };
  sql+="WHERE tlgs_in.id=ids.id "
       "ORDER BY id,num ";

  Qry.SQLText=sql;
  Qry.Execute();
  xmlNodePtr tlgsNode = NewTextChild( resNode, "tlgs" );
  xmlNodePtr node;
  int len,bufLen=0;
  char *ph,*buf=NULL;
  try
  {
    for(;!Qry.Eof;Qry.Next())
    {
      node = NewTextChild( tlgsNode, "tlg" );
      NewTextChild( node, "id", Qry.FieldAsInteger("id") );
      NewTextChild( node, "num", Qry.FieldAsInteger("num") );
      NewTextChild( node, "type", Qry.FieldAsString("type") );
      NewTextChild( node, "addr", Qry.FieldAsString("addr") );
      NewTextChild( node, "heading", Qry.FieldAsString("heading") );
      NewTextChild( node, "ending", Qry.FieldAsString("ending") );
      TDateTime time_receive = UTCToClient( Qry.FieldAsDateTime("time_receive"), tz_region );
      NewTextChild( node, "time_receive", DateTimeToStr( time_receive ) );

      len=Qry.GetSizeLongField("body")+1;
      if (len>bufLen)
      {
        if (buf==NULL)
          ph=(char*)malloc(len);
        else
          ph=(char*)realloc(buf,len);
        if (ph==NULL) throw EMemoryError("Out of memory");
        buf=ph;
        bufLen=len;
      };
      Qry.FieldAsLong("body",buf);
      buf[len-1]=0;
      NewTextChild( node, "body", buf);
    };
    if (buf!=NULL) free(buf);
  }
  catch(...)
  {
    if (buf!=NULL) free(buf);
  };
};

void TelegramInterface::GetTlgOut(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node = GetNode( "point_id", reqNode );
  int point_id;
  TQuery Qry(&OraSession);
  string tz_region;
  string sql="SELECT point_id,id,num,type,airp,crs,addr,heading,body,ending, "
             "pr_lat,time_create,time_send_scd,time_send_act "
             "FROM tlg_out ";
  if (node==NULL)
  {
    int tlg_id = NodeAsInteger( "tlg_id", reqNode );
    sql+="WHERE id=:tlg_id ";
    Qry.CreateVariable("tlg_id",otInteger,tlg_id);
  }
  else
  {
    point_id = NodeAsInteger( node );
    if (point_id!=-1)
    {
      sql+="WHERE point_id=:point_id ";
      Qry.CreateVariable("point_id",otInteger,point_id);
    }
    else
    {
      sql+="WHERE point_id IS NULL AND time_create>=TRUNC(sysdate)-15 ";
    };

  };
  sql+="ORDER BY id,num";

  Qry.SQLText=sql;
  Qry.Execute();
  xmlNodePtr tlgsNode = NewTextChild( resNode, "tlgs" );
  int len,bufLen=0;
  char *ph,*buf=NULL;
  try
  {
    if (!Qry.Eof)
    {
      if (!Qry.FieldIsNULL("point_id"))
      {
        point_id = Qry.FieldAsInteger("point_id");
        TQuery RegionQry(&OraSession);
        RegionQry.SQLText="SELECT system.AirpTZRegion(airp) AS tz_region FROM points WHERE point_id=:point_id";
        RegionQry.CreateVariable("point_id",otInteger,point_id);
        RegionQry.Execute();
        if (RegionQry.Eof) throw UserException("Рейс не найден. Обновите данные");
        tz_region = RegionQry.FieldAsString("tz_region");
      }
      else
      {
        point_id = -1;
        tz_region =  TReqInfo::Instance()->desk.tz_region;
      };
    };
    for(;!Qry.Eof;Qry.Next())
    {
      node = NewTextChild( tlgsNode, "tlg" );
      NewTextChild( node, "id", Qry.FieldAsInteger("id") );
      NewTextChild( node, "num", Qry.FieldAsInteger("num") );
      NewTextChild( node, "type", Qry.FieldAsString("type") );
      NewTextChild( node, "airp", Qry.FieldAsString("airp") );
      NewTextChild( node, "crs", Qry.FieldAsString("crs") );
      NewTextChild( node, "addr", Qry.FieldAsString("addr") );
      NewTextChild( node, "heading", Qry.FieldAsString("heading") );
      NewTextChild( node, "ending", Qry.FieldAsString("ending") );
      NewTextChild( node, "pr_lat", (int)(Qry.FieldAsInteger("pr_lat")!=0) );

      TDateTime time_create = UTCToClient( Qry.FieldAsDateTime("time_create"), tz_region );
      NewTextChild( node, "time_create", DateTimeToStr( time_create ) );

      if (!Qry.FieldIsNULL("time_send_scd"))
      {
        TDateTime time_send_scd = UTCToClient( Qry.FieldAsDateTime("time_send_scd"), tz_region );
        NewTextChild( node, "time_send_scd", DateTimeToStr( time_send_scd ) );
      }
      else
        NewTextChild( node, "time_send_scd" );

      if (!Qry.FieldIsNULL("time_send_act"))
      {
        TDateTime time_send_act = UTCToClient( Qry.FieldAsDateTime("time_send_act"), tz_region );
        NewTextChild( node, "time_send_act", DateTimeToStr( time_send_act ) );
      }
      else
        NewTextChild( node, "time_send_act" );

      len=Qry.GetSizeLongField("body")+1;
      if (len>bufLen)
      {
        if (buf==NULL)
          ph=(char*)malloc(len);
        else
          ph=(char*)realloc(buf,len);
        if (ph==NULL) throw EMemoryError("Out of memory");
        buf=ph;
        bufLen=len;
      };
      Qry.FieldAsLong("body",buf);
      buf[len-1]=0;
      NewTextChild( node, "body", buf);
    };
    if (buf!=NULL) free(buf);
  }
  catch(...)
  {
    if (buf!=NULL) free(buf);
  };
};

