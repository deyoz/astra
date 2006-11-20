#include <vector>
#include <boost/date_time/local_time/local_time.hpp>
#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"
#include "setup.h"
#include "logger.h"
#include "telegram.h"
#include "xml_unit.h"
#include "oralib.h"
#include "exceptions.h"
#include "astra_utils.h"
#include "tlg/tlg.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace boost::local_time;

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

void TelegramInterface::GetAddrs(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  string addrs;
  TQuery AddrQry(&OraSession);
  AddrQry.SQLText=
  "SELECT addr FROM tlg_addrs "
  "WHERE (tlg_type=:tlg_type OR tlg_type IS NULL) AND "
  "      (airline=:airline OR airline IS NULL) AND "
  "      (flt_no=:flt_no OR flt_no IS NULL) AND "
  "      (airp_dep=:airp_dep OR airp_dep IS NULL) AND "
  "      (airp_arv=:airp_arv OR airp_arv IS NULL OR :airp_arv IS NULL) AND "
  "      (crs=:crs OR crs IS NULL OR :crs IS NULL) AND "
  "      (pr_auto=0 OR pr_auto IS NULL) AND "
  "       pr_lat=:pr_lat AND pr_numeric=:pr_numeric AND pr_cancel=0 ";

  if (point_id!=-1)
  {
    TQuery Qry(&OraSession);
    Qry.SQLText="SELECT airline,flt_no,airp FROM points WHERE point_id=:point_id";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.Eof) throw UserException("Рейс не найден. Обновите данные");
    AddrQry.CreateVariable("airline",otString,Qry.FieldAsString("airline"));
    AddrQry.CreateVariable("flt_no",otInteger,Qry.FieldAsInteger("flt_no"));
    AddrQry.CreateVariable("airp_dep",otString,Qry.FieldAsString("airp"));
    AddrQry.CreateVariable("tlg_type",otString,NodeAsString( "tlg_type", reqNode));
    AddrQry.CreateVariable("airp_arv",otString,NodeAsString( "airp_arv", reqNode));
    AddrQry.CreateVariable("crs",otString,NodeAsString( "crs", reqNode));
    AddrQry.CreateVariable("pr_lat",otInteger,(int)(NodeAsInteger( "pr_lat", reqNode)!=0));
    AddrQry.CreateVariable("pr_numeric",otInteger,(int)(NodeAsInteger( "pr_numeric", reqNode)!=0));
    AddrQry.Execute();

    for(;!AddrQry.Eof;AddrQry.Next())
    {
      addrs=addrs+AddrQry.FieldAsString("addr")+" ";
    };
  };

  NewTextChild(resNode,"addrs",addrs);
  return;
};

void TelegramInterface::CreateTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  TQuery TlgQry(&OraSession);
  TlgQry.SQLText=
    "BEGIN "
    "  :id:=tlg.create_tlg(:type,:point_id,:pr_dep,:scd_local,:airp_arv,:crs, "
    "                      :pr_lat,:pr_numeric,:addrs,:sender,:pr_summer,:time_send); "
    "END; ";
  TlgQry.DeclareVariable("id",otInteger);
  bool pr_summer=false;
  if (point_id!=-1)
  {
    TQuery Qry(&OraSession);
    Qry.SQLText=
      "SELECT scd_out,system.AirpTZRegion(airp) AS tz_region "
      "FROM points WHERE point_id=:point_id";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.Eof) throw UserException("Рейс не найден. Обновите данные");
    string tz_region=Qry.FieldAsString("tz_region");

    TlgQry.CreateVariable("point_id",otInteger,point_id);
    TlgQry.CreateVariable("pr_dep",otInteger,1); //!!!
    TDateTime scd_local = UTCToLocal( Qry.FieldAsDateTime("scd_out"), tz_region );
    TlgQry.CreateVariable("scd_local",otDate,scd_local);
    //вычисляем признак летней/зимней навигации
    tz_database &tz_db = get_tz_database();
    time_zone_ptr tz = tz_db.time_zone_from_region( tz_region );
    if (tz==NULL) throw Exception("Region '%s' not found",tz_region.c_str());
    if (tz->has_dst())
    {
      local_date_time ld(DateTimeToBoost(Qry.FieldAsDateTime("scd_out")),tz);
      pr_summer=ld.is_dst();
    };
  }
  else
  {
    TlgQry.CreateVariable("point_id",otInteger,FNull);
    TlgQry.CreateVariable("pr_dep",otInteger,FNull);
    TlgQry.CreateVariable("scd_local",otDate,FNull);
  };
  TlgQry.CreateVariable("type",otString,NodeAsString( "tlg_type", reqNode));
  TlgQry.CreateVariable("airp_arv",otString,NodeAsString( "airp_arv", reqNode));
  TlgQry.CreateVariable("crs",otString,NodeAsString( "crs", reqNode));
  TlgQry.CreateVariable("pr_lat",otInteger,(int)(NodeAsInteger( "pr_lat", reqNode)!=0));
  TlgQry.CreateVariable("pr_numeric",otInteger,(int)(NodeAsInteger( "pr_numeric", reqNode)!=0));
  TlgQry.CreateVariable("addrs",otString,NodeAsString( "addrs", reqNode));
  TlgQry.CreateVariable("sender",otString,OWN_SITA_ADDR());
  TlgQry.CreateVariable("pr_summer",otInteger,(int)pr_summer);
  ProgTrace(TRACE5,"pr_summer=%d",(int)pr_summer);
  TlgQry.CreateVariable("time_send",otDate,FNull);
  try
  {
    TlgQry.Execute();
  }
  catch(EOracleError E)
  {
    if ( E.Code > 20000 )
      throw UserException(E.what());
    else
      throw;
  };
  if (TlgQry.VariableIsNULL("id")) throw Exception("tlg.create_tlg without result");
  int tlg_id=TlgQry.GetVariableAsInteger("id");
  ostringstream msg;
  msg << "Телеграмма " << TlgQry.GetVariableAsString("type")
      << " (ид=" << tlg_id << ") сформирована: ";
  if (!TlgQry.VariableIsNULL("airp_arv"))
    msg << "а/п: " << TlgQry.GetVariableAsString("airp_arv") << ", ";
  if (!TlgQry.VariableIsNULL("crs"))
    msg << "центр: " << TlgQry.GetVariableAsString("crs") << ", ";
  msg << "адреса: " << TlgQry.GetVariableAsString("addrs") << ", "
      << "лат.: " << (TlgQry.GetVariableAsInteger("pr_lat")==0?"нет":"да") << ", "
      << "цифр.: " << (TlgQry.GetVariableAsInteger("pr_numeric")==0?"нет":"да");
  TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
  NewTextChild( resNode, "tlg_id", tlg_id);
};

void TelegramInterface::LoadTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string text = NodeAsString("tlg_text",reqNode);
    if (text.empty()) throw UserException("Телеграмма пуста");
    loadTlg(text);
    showMessage("Телеграмма загружена");
};

void TelegramInterface::SaveTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int tlg_id = NodeAsInteger( "tlg_id", reqNode );
  string tlg_body = NodeAsString( "tlg_body", reqNode );
  if (tlg_body.size()>2000)
    throw UserException("Общая длина телеграммы не может превышать 2000 символов");
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT type,point_id FROM tlg_out WHERE id=:id AND num=1 FOR UPDATE";
  Qry.CreateVariable( "id", otInteger, tlg_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Телеграмма не найдена. Обновите данные");
  string tlg_type=Qry.FieldAsString("type");
  int point_id=Qry.FieldAsInteger("point_id");

  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  DELETE FROM tlg_out WHERE id=:id AND num<>1; "
    "  UPDATE tlg_out SET body=:body WHERE id=:id AND num=1; "
    "END;";
  Qry.CreateVariable( "id", otInteger, tlg_id);
  Qry.CreateVariable( "body", otString, tlg_body );
  Qry.Execute();

  ostringstream msg;
  msg << "Телеграмма " << tlg_type << " (ид=" << tlg_id << ") изменена";
  TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
  showMessage("Телеграмма успешно сохранена");
};

void TelegramInterface::SendTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int tlg_id = NodeAsInteger( "tlg_id", reqNode );
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT type,point_id FROM tlg_out WHERE id=:id AND num=1 FOR UPDATE ";
  Qry.CreateVariable( "id", otInteger, tlg_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Телеграмма не найдена. Обновите данные");
  string tlg_type=Qry.FieldAsString("type");
  int point_id=Qry.FieldAsInteger("point_id");

  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  tlg.send_tlg(:own_canon_name,:id); "
    "END;";
  Qry.CreateVariable( "own_canon_name", otString, OWN_CANON_NAME());
  Qry.CreateVariable( "id", otInteger, tlg_id);
  try
  {
    Qry.Execute();
  }
  catch(EOracleError E)
  {
    if ( E.Code > 20000 )
      throw UserException(E.what());
    else
      throw;
  };

  ostringstream msg;
  msg << "Телеграмма " << tlg_type << " (ид=" << tlg_id << ") отправлена";
  TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
  showMessage("Телеграмма отправлена");
}

void TelegramInterface::DeleteTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int tlg_id = NodeAsInteger( "tlg_id", reqNode );
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT type,point_id FROM tlg_out WHERE id=:id AND num=1 FOR UPDATE ";
  Qry.CreateVariable( "id", otInteger, tlg_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Телеграмма не найдена. Обновите данные");
  string tlg_type=Qry.FieldAsString("type");
  int point_id=Qry.FieldAsInteger("point_id");

  Qry.Clear();
  Qry.SQLText=
    "DELETE FROM tlg_out WHERE id=:id AND time_send_act IS NULL ";
  Qry.CreateVariable( "id", otInteger, tlg_id);
  Qry.Execute();
  if (Qry.RowsProcessed()>0)
  {
    ostringstream msg;
    msg << "Телеграмма " << tlg_type << " (ид=" << tlg_id << ") удалена";
    TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
    showMessage("Телеграмма удалена");
  };
};


