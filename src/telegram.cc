#include <vector>
#include <utility>
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
#include "misc.h"
#include "astra_utils.h"
#include "tlg/tlg.h"
#include "tlg/tlg_parser.h"
#include "base_tables.h"
#include "astra_misc.h"

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
  if (Qry.Eof) throw UserException("���� �� ������. ������� �����");
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
    //�஢�ਬ �� �㡫�஢���� ����� ��ய��⮢ � ࠬ��� ������ ३�
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
    if (RegionQry.Eof) throw UserException("���� �� ������. ������� �����");
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
    sql+="(SELECT id FROM tlgs_in WHERE time_receive>=TRUNC(system.UTCSYSDATE)-2 "
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
    throw;
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
      sql+="WHERE point_id IS NULL AND time_create>=TRUNC(system.UTCSYSDATE)-2 ";
    };

  };
  sql+="ORDER BY id,num";

  Qry.SQLText=sql;
  Qry.Execute();
  xmlNodePtr tlgsNode = NewTextChild( resNode, "tlgs" );

  if (!Qry.Eof)
  {
    if (!Qry.FieldIsNULL("point_id"))
    {
      point_id = Qry.FieldAsInteger("point_id");
      TQuery RegionQry(&OraSession);
      RegionQry.SQLText="SELECT system.AirpTZRegion(airp) AS tz_region FROM points WHERE point_id=:point_id";
      RegionQry.CreateVariable("point_id",otInteger,point_id);
      RegionQry.Execute();
      if (RegionQry.Eof) throw UserException("���� �� ������. ������� �����");
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

    NewTextChild( node, "body", Qry.FieldAsString("body") );
  };
};

void TelegramInterface::GetAddrs(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  xmlNodePtr node=reqNode->children;
  string addrs;
  TQuery AddrQry(&OraSession);

  if (point_id!=-1)
  {
    TQuery Qry(&OraSession);
    Qry.SQLText=
      "SELECT airline,flt_no,airp,point_num, "
      "       DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
      "FROM points WHERE point_id=:point_id";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.Eof) throw UserException("���� �� ������. ������� �����");

    AddrQry.SQLText=
      "SELECT addr FROM typeb_addrs "
      "WHERE tlg_type=:tlg_type AND "
      "      (airline=:airline OR airline IS NULL) AND "
      "      (flt_no=:flt_no OR flt_no IS NULL) AND "
      "      (airp_dep=:airp_dep OR airp_dep IS NULL) AND "
      "      (:airp_arv IS NULL AND airp_arv IN "
      "        (SELECT airp FROM points "
      "         WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0) OR "
      "       :airp_arv IS NOT NULL AND airp_arv=:airp_arv OR "
      "       airp_arv IS NULL) AND "
      "      (:crs IS NOT NULL AND crs=:crs OR crs IS NULL) AND "
      "      (:pr_numeric IS NOT NULL AND pr_numeric=:pr_numeric OR pr_numeric IS NULL) AND "
      "       pr_lat=:pr_lat ";

    AddrQry.CreateVariable("airline",otString,Qry.FieldAsString("airline"));
    AddrQry.CreateVariable("flt_no",otInteger,Qry.FieldAsInteger("flt_no"));
    AddrQry.CreateVariable("airp_dep",otString,Qry.FieldAsString("airp"));
    AddrQry.CreateVariable("first_point",otInteger,Qry.FieldAsInteger("first_point"));
    AddrQry.CreateVariable("point_num",otInteger,Qry.FieldAsInteger("point_num"));
    AddrQry.CreateVariable("tlg_type",otString,NodeAsStringFast( "tlg_type", node));
    AddrQry.CreateVariable("airp_arv",otString,NodeAsStringFast( "airp_arv", node, ""));
    AddrQry.CreateVariable("crs",otString,NodeAsStringFast( "crs", node, ""));
    if (!NodeIsNULLFast( "pr_numeric", node, true))
      AddrQry.CreateVariable("pr_numeric",otInteger,(int)(NodeAsIntegerFast( "pr_numeric", node)!=0));
    else
      AddrQry.CreateVariable("pr_numeric",otInteger,FNull);
    AddrQry.CreateVariable("pr_lat",otInteger,(int)(NodeAsIntegerFast( "pr_lat", node)!=0));

  }
  else
  {
    AddrQry.SQLText=
      "SELECT addr FROM typeb_addrs "
      "WHERE tlg_type=:tlg_type AND pr_lat=:pr_lat";
    AddrQry.CreateVariable("tlg_type",otString,NodeAsStringFast( "tlg_type", node));
    AddrQry.CreateVariable("pr_lat",otInteger,(int)(NodeAsIntegerFast( "pr_lat", node)!=0));
  };
  AddrQry.Execute();

  for(;!AddrQry.Eof;AddrQry.Next())
  {
    addrs=addrs+AddrQry.FieldAsString("addr")+" ";
  };

  NewTextChild(resNode,"addrs",addrs);
  return;
};

void TelegramInterface::CreateTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  xmlNodePtr node=reqNode->children;
  TQuery TlgQry(&OraSession);
  TlgQry.SQLText=
    "BEGIN "
    "  :id:=tlg.create_tlg(:tlg_type,:point_id,:pr_dep,:scd_local,:airp_arv,:crs, "
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
    if (Qry.Eof) throw UserException("���� �� ������. ������� �����");
    string tz_region=Qry.FieldAsString("tz_region");

    TlgQry.CreateVariable("point_id",otInteger,point_id);
    TlgQry.CreateVariable("pr_dep",otInteger,1); //!!!
    TDateTime scd_local = UTCToLocal( Qry.FieldAsDateTime("scd_out"), tz_region );
    TlgQry.CreateVariable("scd_local",otDate,scd_local);
    //����塞 �ਧ��� ��⭥�/������ ������樨
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
  TlgQry.CreateVariable("tlg_type",otString,NodeAsStringFast( "tlg_type", node));
  TlgQry.CreateVariable("airp_arv",otString,NodeAsStringFast( "airp_arv", node, ""));
  TlgQry.CreateVariable("crs",otString,NodeAsStringFast( "crs", node, ""));
  if (!NodeIsNULLFast( "pr_numeric", node, true))
    TlgQry.CreateVariable("pr_numeric",otInteger,(int)(NodeAsIntegerFast( "pr_numeric", node)!=0));
  else
    TlgQry.CreateVariable("pr_numeric",otInteger,FNull);
  TlgQry.CreateVariable("pr_lat",otInteger,(int)(NodeAsIntegerFast( "pr_lat", node)!=0));
  TlgQry.CreateVariable("addrs",otString,NodeAsStringFast( "addrs", node));
  TlgQry.CreateVariable("sender",otString,OWN_SITA_ADDR());
  TlgQry.CreateVariable("pr_summer",otInteger,(int)pr_summer);
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
  msg << "�����ࠬ�� " << TlgQry.GetVariableAsString("tlg_type")
      << " (��=" << tlg_id << ") ��ନ஢���: ";
  msg << "����: " << TlgQry.GetVariableAsString("addrs") << ", ";
  if (!TlgQry.VariableIsNULL("airp_arv") &&
      *TlgQry.GetVariableAsString("airp_arv")!=0)
    msg << "�/�: " << TlgQry.GetVariableAsString("airp_arv") << ", ";
  if (!TlgQry.VariableIsNULL("crs") &&
      *TlgQry.GetVariableAsString("crs")!=0)
    msg << "業��: " << TlgQry.GetVariableAsString("crs") << ", ";
  if (!TlgQry.VariableIsNULL("pr_numeric"))
    msg << "���.: " << (TlgQry.GetVariableAsInteger("pr_numeric")==0?"���":"��") << ", ";
  msg << "���.: " << (TlgQry.GetVariableAsInteger("pr_lat")==0?"���":"��");
  if (point_id==-1) point_id=0;
  TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
  NewTextChild( resNode, "tlg_id", tlg_id);
///  GetTlgOut(ctxt,resNode,resNode);
};

#include "base_tables.h"

void TelegramInterface::LoadTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
/*  TBaseTable &airlines=base_tables.get("aIrLiNeS");
  tst();
  TBaseTableRow &row1=airlines.get_row("coDe_lat","UT");
  tst();
  ProgTrace(TRACE5,"id=%d code=%s code_lat=%s name=%s name_lat=%s short_name=%s "
                   "short_name_lat=%s",
                   row1.AsInteger("Id"),
                   row1.AsString("cOde").c_str(),row1.AsString("codE",true).c_str(),
                   row1.AsString("naMe",false).c_str(),row1.AsString("nAme",true).c_str(),
                   row1.AsString("shoRt_name",false).c_str(),row1.AsString("short_naMe",true).c_str());
  TAirlinesRow &row2=(TAirlinesRow&)airlines.get_row("codE","��");
  ProgTrace(TRACE5,"id=%d code=%s code_lat=%s name=%s name_lat=%s short_name=%s "
                   "short_name_lat=%s",
                   row2.id,
                   row2.code.c_str(),row2.code_lat.c_str(),row2.name.c_str(),row2.name_lat.c_str(),
                   row2.short_name.c_str(),row2.short_name_lat.c_str());
  TAirlinesRow &row3=(TAirlinesRow&)airlines.get_row("Id",226);
  ProgTrace(TRACE5,"id=%d code=%s code_lat=%s name=%s name_lat=%s short_name=%s "
                   "short_name_lat=%s",
                   row3.id,
                   row3.code.c_str(),row3.code_lat.c_str(),row3.name.c_str(),row3.name_lat.c_str(),
                   row3.short_name.c_str(),row3.short_name_lat.c_str());

  TBaseTable &airps=base_tables.get("aIrPs");
  tst();
  TBaseTableRow &row4=airps.get_row("coDe_lat","DME");
  tst();
  ProgTrace(TRACE5,"id=%d code=%s code_lat=%s name=%s name_lat=%s city=%s "
                   "city=%s",
                   row4.AsInteger("Id"),
                   row4.AsString("cOde").c_str(),row4.AsString("codE",true).c_str(),
                   row4.AsString("naMe",false).c_str(),row4.AsString("nAme",true).c_str(),
                   row4.AsString("City",false).c_str(),row4.AsString("ciTy",true).c_str());
  TAirpsRow &row5=(TAirpsRow&)airps.get_row("codE","���");
  ProgTrace(TRACE5,"id=%d code=%s code_lat=%s name=%s name_lat=%s city=%s "
                   "city=%s",
                   row5.id,
                   row5.code.c_str(),row5.code_lat.c_str(),row5.name.c_str(),row5.name_lat.c_str(),
                   row5.city.c_str(),row5.city.c_str());

  TBaseTable &cities=base_tables.get("CiTiEs");
  tst();
  TBaseTableRow &row6=cities.get_row("coDe_lat","MOW");
  tst();
  ProgTrace(TRACE5,"id=%d code=%s code_lat=%s name=%s name_lat=%s country=%s region=%s "
                   "tz=%d",
                   row6.AsInteger("Id"),
                   row6.AsString("cOde").c_str(),row6.AsString("codE",true).c_str(),
                   row6.AsString("naMe",false).c_str(),row6.AsString("nAme",true).c_str(),
                   row6.AsString("Country").c_str(),row6.AsString("reGion").c_str(),
                   row6.AsInteger("tZ"));
  TCitiesRow &row7=(TCitiesRow&)cities.get_row("codE","���",true);
  ProgTrace(TRACE5,"id=%d code=%s code_lat=%s name=%s name_lat=%s country=%s region=%s "
                   "tz=%d",
                   row7.id,
                   row7.code.c_str(),row7.code_lat.c_str(),row7.name.c_str(),row7.name_lat.c_str(),
                   row7.country.c_str(),row7.region.c_str(),row7.tz);

  TBaseTable &classes=base_tables.get("cLAsses");
  tst();
  TBaseTableRow &row8=classes.get_row("coDe_lat","Y");
  tst();
  ProgTrace(TRACE5,"code=%s code_lat=%s name=%s name_lat=%s priority=%d ",
                   row8.AsString("cOde").c_str(),row8.AsString("codE",true).c_str(),
                   row8.AsString("naMe",false).c_str(),row8.AsString("nAme",true).c_str(),
                   row8.AsInteger("priOrity"));
  TClassesRow &row9=(TClassesRow&)classes.get_row("codE","�");
  ProgTrace(TRACE5,"code=%s code_lat=%s name=%s name_lat=%s priority=%d ",
                   row9.code.c_str(),row9.code_lat.c_str(),row9.name.c_str(),row9.name_lat.c_str(),
                   row9.priority);

  TBaseTable& tab=base_tables.get("perS_types");
  TBaseTableRow& row10=tab.get_row("coDe_lat","CHD");

  row10=base_tables.get("craftS").get_row("Code","��5");*/


  string text = NodeAsString("tlg_text",reqNode);
  if (text.empty()) throw UserException("�����ࠬ�� ����");
  loadTlg(text);
  showMessage("�����ࠬ�� ����㦥��");
};

void TelegramInterface::SaveTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int tlg_id = NodeAsInteger( "tlg_id", reqNode );
  string tlg_body = NodeAsString( "tlg_body", reqNode );
  if (tlg_body.size()>2000)
    throw UserException("���� ����� ⥫��ࠬ�� �� ����� �ॢ���� 2000 ᨬ�����");
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT type,point_id FROM tlg_out WHERE id=:id AND num=1 FOR UPDATE";
  Qry.CreateVariable( "id", otInteger, tlg_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("�����ࠬ�� �� �������. ������� �����");
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
  msg << "�����ࠬ�� " << tlg_type << " (��=" << tlg_id << ") ��������";
  TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
  showMessage("�����ࠬ�� �ᯥ譮 ��࠭���");
};

void TelegramInterface::SendTlg(int tlg_id)
{
  TQuery TlgQry(&OraSession);
  TlgQry.Clear();
  TlgQry.SQLText=
    "SELECT id,num,type,point_id,addr,heading,body,ending "
    "FROM tlg_out WHERE id=:id FOR UPDATE";
  TlgQry.CreateVariable( "id", otInteger, tlg_id);
  TlgQry.Execute();
  if (TlgQry.Eof) throw UserException("�����ࠬ�� �� �������. ������� �����");
  string tlg_type=TlgQry.FieldAsString("type");
  int point_id=TlgQry.FieldAsInteger("point_id");

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT canon_name FROM addrs WHERE addr=:addr";
  Qry.DeclareVariable("addr",otString);

  TQuery AddrQry(&OraSession);
  AddrQry.Clear();
  AddrQry.SQLText=
    "BEGIN "
    "  tlg.format_addr_line(:addrs); "
    "END;";
  AddrQry.DeclareVariable("addrs",otString);

  string old_addrs,canon_name,tlg_text;
  map<string,string> recvs;
  map<string,string>::iterator i;
  TTlgParser tlg;
  char *addrs,*line_p;

  for(;!TlgQry.Eof;TlgQry.Next())
  {
    if (TlgQry.FieldAsString("addr")!=old_addrs)
    {
      recvs.clear();
      line_p=TlgQry.FieldAsString("addr");
      try
      {
        do
        {
          addrs=tlg.GetLexeme(line_p);
          while (addrs!=NULL)
          {
            if (strlen(tlg.lex)!=7)
              throw UserException("����୮ 㪠��� SITA-���� %s",tlg.lex);
            for(char *p=tlg.lex;*p!=0;p++)
              if (!(IsUpperLetter(*p)&&*p>='A'&&*p<='Z'||IsDigit(*p)))
                throw UserException("����୮ 㪠��� SITA-���� %s",tlg.lex);
            char addr[8];
            strcpy(addr,tlg.lex);
            Qry.SetVariable("addr",addr);
            Qry.Execute();
            if (!Qry.Eof) canon_name=Qry.FieldAsString("canon_name");
            else
            {
              addr[5]=0; //��१��� �� 5-� ᨬ�����
              Qry.SetVariable("addr",addr);
              Qry.Execute();
              if (!Qry.Eof) canon_name=Qry.FieldAsString("canon_name");
              else
              {
                if (*(DEF_CANON_NAME())!=0)
                  canon_name=DEF_CANON_NAME();
                else
                  throw UserException("�� ��।���� �������᪨� ���� ��� SITA-���� %s",tlg.lex);
              };
            };
            if (recvs.find(canon_name)==recvs.end())
            	recvs[canon_name]=tlg.lex;
            else
            	recvs[canon_name]=recvs[canon_name]+' '+tlg.lex;

            addrs=tlg.GetLexeme(addrs);
          };
        }
        while ((line_p=tlg.NextLine(line_p))!=NULL);
      }
      catch(ETlgError)
      {
        throw UserException("������ �ଠ� ���᭮� ��ப�");
      };
      old_addrs=TlgQry.FieldAsString("addr");
    };
    if (recvs.empty()) throw UserException("�� 㪠���� ���� �����⥫�� ⥫��ࠬ��");

    //�ନ�㥬 ⥫��ࠬ��
    tlg_text=(string)TlgQry.FieldAsString("heading")+
             TlgQry.FieldAsString("body")+TlgQry.FieldAsString("ending");

    for(i=recvs.begin();i!=recvs.end();i++)
    {
    	AddrQry.SetVariable("addrs",i->second); //�८�ࠧ㥬
    	AddrQry.Execute();
      if (OWN_CANON_NAME()==i->first)
        /* �ࠧ� ����頥� �� �室��� ��।� */
        loadTlg(AddrQry.GetVariableAsString("addrs")+tlg_text);
      else
        sendTlg(i->first.c_str(),OWN_CANON_NAME(),false,0,
                AddrQry.GetVariableAsString("addrs")+tlg_text);
    };
  };

  TlgQry.Clear();
  TlgQry.SQLText=
    "UPDATE tlg_out SET time_send_act=system.UTCSYSDATE WHERE id=:id";
  TlgQry.CreateVariable( "id", otInteger, tlg_id);
  TlgQry.Execute();
  ostringstream msg;
  msg << "�����ࠬ�� " << tlg_type << " (��=" << tlg_id << ") ��ࠢ����";
  TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
};

void TelegramInterface::SendTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  SendTlg(NodeAsInteger( "tlg_id", reqNode ));
  showMessage("�����ࠬ�� ��ࠢ����");
  GetTlgOut(ctxt,reqNode,resNode);
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
  if (Qry.Eof) throw UserException("�����ࠬ�� �� �������. ������� �����");
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
    msg << "�����ࠬ�� " << tlg_type << " (��=" << tlg_id << ") 㤠����";
    TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
    showMessage("�����ࠬ�� 㤠����");
  };
  GetTlgOut(ctxt,reqNode,resNode);
};

void TelegramInterface::SendTlg( int point_id, vector<string> &tlg_types )
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,flt_no,airp,point_num,scd_out,system.AirpTZRegion(airp) AS tz_region, "
    "       DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
    "FROM points WHERE point_id=:point_id";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("���� �� ������. ������� �����");

  TQuery SendQry(&OraSession);
  SendQry.Clear();
  SendQry.SQLText=
    "SELECT pr_denial, "
    "       DECODE(airline,NULL,0,8)+ "
    "       DECODE(flt_no,NULL,0,1)+ "
    "       DECODE(airp_dep,NULL,0,4)+ "
    "       DECODE(airp_arv,NULL,0,2) AS priority "
    "FROM typeb_send "
    "WHERE tlg_type=:tlg_type AND "
    "      (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) AND "
    "      (airp_arv IS NULL OR airp_arv IN "
    "        (SELECT airp FROM points "
    "         WHERE first_point=:first_point AND point_num=:point_num AND pr_del=0)) "
    "ORDER BY priority DESC";
  SendQry.DeclareVariable("tlg_type",otString);
  SendQry.CreateVariable("airline",otString,Qry.FieldAsString("airline"));
  SendQry.CreateVariable("flt_no",otInteger,Qry.FieldAsInteger("flt_no"));
  SendQry.CreateVariable("airp_dep",otString,Qry.FieldAsString("airp"));
  SendQry.CreateVariable("first_point",otInteger,Qry.FieldAsInteger("first_point"));
  SendQry.CreateVariable("point_num",otInteger,Qry.FieldAsInteger("point_num"));

  TQuery AddrQry(&OraSession);
  AddrQry.Clear();
  AddrQry.SQLText=
    "SELECT addr FROM typeb_addrs "
    "WHERE tlg_type=:tlg_type AND "
    "      (airline=:airline OR airline IS NULL) AND "
    "      (flt_no=:flt_no OR flt_no IS NULL) AND "
    "      (airp_dep=:airp_dep OR airp_dep IS NULL) AND "
    "      (:airp_arv IS NULL AND airp_arv IN "
    "        (SELECT airp FROM points "
    "         WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0) OR "
    "       :airp_arv IS NOT NULL AND airp_arv=:airp_arv OR "
    "       airp_arv IS NULL) AND "
    "      (:crs IS NOT NULL AND crs=:crs OR crs IS NULL) AND "
    "      (:pr_numeric IS NOT NULL AND pr_numeric=:pr_numeric OR pr_numeric IS NULL) AND "
    "       pr_lat=:pr_lat ";

  AddrQry.CreateVariable("airline",otString,Qry.FieldAsString("airline"));
  AddrQry.CreateVariable("flt_no",otInteger,Qry.FieldAsInteger("flt_no"));
  AddrQry.CreateVariable("airp_dep",otString,Qry.FieldAsString("airp"));
  AddrQry.CreateVariable("first_point",otInteger,Qry.FieldAsInteger("first_point"));
  AddrQry.CreateVariable("point_num",otInteger,Qry.FieldAsInteger("point_num"));
  AddrQry.DeclareVariable("tlg_type",otString);
  AddrQry.DeclareVariable("airp_arv",otString);
  AddrQry.DeclareVariable("crs",otString);
  AddrQry.DeclareVariable("pr_numeric",otInteger);
  AddrQry.DeclareVariable("pr_lat",otInteger);

  TQuery ParamQry(&OraSession);
  //����稬 �� ��ய���� �� ��������
  vector<string> airp_arv;
  ParamQry.Clear();
  ParamQry.SQLText=
    "SELECT DISTINCT airp FROM points "
    "WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0 ";
  ParamQry.CreateVariable("first_point",otInteger,Qry.FieldAsInteger("first_point"));
  ParamQry.CreateVariable("point_num",otInteger,Qry.FieldAsInteger("point_num"));
  ParamQry.Execute();
  for(;!ParamQry.Eof;ParamQry.Next())
    airp_arv.push_back(ParamQry.FieldAsString("airp"));

  //����稬 �� ��⥬� �஭�஢���� �� ���. �뫠 ���뫪�
  vector<string> crs;
  ParamQry.Clear();
  ParamQry.SQLText=
    "SELECT DISTINCT crs FROM crs_set "
    "WHERE airline=:airline AND "
    "      (flt_no=:flt_no OR flt_no IS NULL) AND "
    "      (airp_dep=:airp_dep OR airp_dep IS NULL) ";
  ParamQry.CreateVariable("airline",otString,Qry.FieldAsString("airline"));
  ParamQry.CreateVariable("flt_no",otInteger,Qry.FieldAsInteger("flt_no"));
  ParamQry.CreateVariable("airp_dep",otString,Qry.FieldAsString("airp"));
  ParamQry.Execute();
  for(;!ParamQry.Eof;ParamQry.Next())
    crs.push_back(ParamQry.FieldAsString("crs"));

  TQuery TlgQry(&OraSession);
  TlgQry.Clear();
  TlgQry.SQLText=
    "BEGIN "
    "  :id:=tlg.create_tlg(:tlg_type,:point_id,:pr_dep,:scd_local,:airp_arv,:crs, "
    "                      :pr_lat,:pr_numeric,:addrs,:sender,:pr_summer,:time_send); "
    "END; ";

  TlgQry.CreateVariable("point_id",otInteger,point_id);
  TlgQry.CreateVariable("pr_dep",otInteger,1); //!!!
  string tz_region=Qry.FieldAsString("tz_region");
  TDateTime scd_local = UTCToLocal( Qry.FieldAsDateTime("scd_out"), tz_region );
  TlgQry.CreateVariable("scd_local",otDate,scd_local);
  //����塞 �ਧ��� ��⭥�/������ ������樨
  tz_database &tz_db = get_tz_database();
  time_zone_ptr tz = tz_db.time_zone_from_region( tz_region );
  if (tz==NULL) throw Exception("Region '%s' not found",tz_region.c_str());
  bool pr_summer=false;
  if (tz->has_dst())
  {
    local_date_time ld(DateTimeToBoost(Qry.FieldAsDateTime("scd_out")),tz);
    pr_summer=ld.is_dst();
  };
  TlgQry.DeclareVariable("id",otInteger);
  TlgQry.DeclareVariable("tlg_type",otString);
  TlgQry.DeclareVariable("airp_arv",otString);
  TlgQry.DeclareVariable("crs",otString);
  TlgQry.DeclareVariable("pr_numeric",otInteger);
  TlgQry.DeclareVariable("pr_lat",otInteger);
  TlgQry.DeclareVariable("addrs",otString);
  TlgQry.CreateVariable("sender",otString,OWN_SITA_ADDR());
  TlgQry.CreateVariable("pr_summer",otInteger,(int)pr_summer);
  TlgQry.CreateVariable("time_send",otDate,FNull);

  vector<string>::iterator t;
  for(t=tlg_types.begin();t!=tlg_types.end();t++)
  {
    SendQry.SetVariable("tlg_type",*t);
    SendQry.Execute();
    if (SendQry.Eof||SendQry.FieldAsInteger("pr_denial")!=0) continue;

    //�ନ�㥬 ⥫��ࠬ��
    vector<string> airp_arvh;
    vector<string>::iterator i;
    if (*t=="PTM" || *t=="BTM")
      airp_arvh=airp_arv;
    else
      airp_arvh.push_back("");

    vector<string> crsh;
    vector<string>::iterator j;
    if (*t=="PFS")
      crsh=crs;
    else
      crsh.push_back("");

    vector<int> pr_numerich;
    vector<int>::iterator k;
    if (*t=="PTM" || *t=="PFS")
    {
      pr_numerich.push_back(0);
      pr_numerich.push_back(1);
    }
    else
      pr_numerich.push_back(-1);

    AddrQry.SetVariable("tlg_type",*t);
    TlgQry.SetVariable("tlg_type",*t);

    for(int pr_lat=0;pr_lat<=1;pr_lat++)
    {
      AddrQry.SetVariable("pr_lat",pr_lat);
      TlgQry.SetVariable("pr_lat",pr_lat);
      for(i=airp_arvh.begin();i!=airp_arvh.end();i++)
      {
        if (!i->empty())
        {
          AddrQry.SetVariable("airp_arv",*i);
          TlgQry.SetVariable("airp_arv",*i);
        }
        else
        {
          AddrQry.SetVariable("airp_arv",FNull);
          TlgQry.SetVariable("airp_arv",FNull);
        };
        for(j=crsh.begin();j!=crsh.end();j++)
        {
          AddrQry.SetVariable("crs",*j);
          TlgQry.SetVariable("crs",*j);
          for(k=pr_numerich.begin();k!=pr_numerich.end();k++)
          {
            if (*k>=0)
            {
              AddrQry.SetVariable("pr_numeric",*k);
              TlgQry.SetVariable("pr_numeric",*k);
            }
            else
            {
              AddrQry.SetVariable("pr_numeric",FNull);
              TlgQry.SetVariable("pr_numeric",FNull);
            };

            AddrQry.Execute();
            string addrs;
            for(;!AddrQry.Eof;AddrQry.Next())
            {
              addrs=addrs+AddrQry.FieldAsString("addr")+" ";
            };
            if (addrs.empty()) continue;

            try
            {
              TlgQry.SetVariable("addrs",addrs.c_str());
              TlgQry.Execute();
              if (TlgQry.VariableIsNULL("id")) throw Exception("tlg.create_tlg without result");
              int tlg_id=TlgQry.GetVariableAsInteger("id");
              ostringstream msg;
              msg << "�����ࠬ�� " << TlgQry.GetVariableAsString("tlg_type")
                  << " (��=" << tlg_id << ") ��ନ஢���: ";
              msg << "����: " << TlgQry.GetVariableAsString("addrs") << ", ";
              if (!TlgQry.VariableIsNULL("airp_arv") &&
                  *TlgQry.GetVariableAsString("airp_arv")!=0)
                msg << "�/�: " << TlgQry.GetVariableAsString("airp_arv") << ", ";
              if (!TlgQry.VariableIsNULL("crs") &&
                  *TlgQry.GetVariableAsString("crs")!=0)
                msg << "業��: " << TlgQry.GetVariableAsString("crs") << ", ";
              if (!TlgQry.VariableIsNULL("pr_numeric"))
                msg << "���.: " << (TlgQry.GetVariableAsInteger("pr_numeric")==0?"���":"��") << ", ";
              msg << "���.: " << (TlgQry.GetVariableAsInteger("pr_lat")==0?"���":"��");

              TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
              SendTlg(tlg_id);
            }
            catch( Exception E )
            {
              ProgError(STDLOG,"SendTlg (point_id=%d, type=%s): %s",point_id,t->c_str(),E.what());
            }
            catch(...)
            {
              ProgError(STDLOG,"SendTlg (point_id=%d, type=%s): unknown error",point_id,t->c_str());
            };
          };
        };
      };
    };

  };

};

vector<TBSMContent>& TelegramInterface::CreateBSMContent(TBSMContent& con1, TBSMContent& con2)
{
  static vector<TBSMContent> bsms;
  bsms.clear();

  TBSMContent conADD,conCHG,conDEL;
  conADD=con2;
  conADD.indicator=None;
  conADD.tags.clear();

  conCHG=con2;
  conCHG.indicator=CHG;
  conCHG.tags.clear();

  conDEL=con1;
  conDEL.indicator=DEL;
  conDEL.tags.clear();

  //�஢��塞 ३�
  if (strcmp(con1.OutFlt.airline,con2.OutFlt.airline)==0 &&
      con1.OutFlt.flt_no==con2.OutFlt.flt_no &&
      strcmp(con1.OutFlt.suffix,con2.OutFlt.suffix)==0 &&
      con1.OutFlt.scd==con2.OutFlt.scd &&
      strcmp(con1.OutFlt.airp_dep,con2.OutFlt.airp_dep)==0)
  {
    //�஢�ਬ �⫨砫��� �� ���ଠ�� �� ���ᠦ���
    int pr_chd=!(strcmp(con1.OutFlt.airp_arv,con2.OutFlt.airp_arv)==0 &&
                 strcmp(con1.OutFlt.subcl,con2.OutFlt.subcl)==0 &&
                 con1.pax.reg_no==con2.pax.reg_no &&
                 con1.pax.surname==con2.pax.surname &&
                 con1.pax.name==con2.pax.name &&
                 con1.pax.seat_no==con2.pax.seat_no &&
                 con1.pax.pnr_addr==con2.pax.pnr_addr &&
                 con1.bag.rk_weight==con2.bag.rk_weight);
    if (!pr_chd)
    {
      //�ਤ���� �஢���� ��������� � ��몮����� ३��
      vector<TTransferItem>::iterator i1,i2;
      i1=con1.OnwardFlt.begin();
      i2=con2.OnwardFlt.begin();
      for(;i1!=con1.OnwardFlt.end()&&i2!=con2.OnwardFlt.end();i1++,i2++)
      {
        if (!(strcmp(i1->airline,i2->airline)==0 &&
              i1->flt_no==i2->flt_no &&
              strcmp(i1->suffix,i2->suffix)==0 &&
              i1->scd==i2->scd &&
              strcmp(i1->airp_dep,i2->airp_dep)==0 &&
              strcmp(i1->airp_arv,i2->airp_arv)==0 &&
              strcmp(i1->subcl,i2->subcl)==0)) break;
      };
      pr_chd= i1!=con1.OnwardFlt.end() || i2!=con2.OnwardFlt.end();
    };

    vector<TBSMTagItem>::iterator i1,i2;
    i1=con1.tags.begin();
    i2=con2.tags.begin();
    int res;
    while(i1!=con1.tags.end() || i2!=con2.tags.end())
    {
      res=0;
      if (i1==con1.tags.end() ||
          i2!=con2.tags.end() && i1->no>i2->no) res=-1;
      if (i2==con2.tags.end() ||
          i1!=con1.tags.end() && i1->no<i2->no) res=1;

      if (res>0) conDEL.tags.push_back(*i1);

      if (res<0) conADD.tags.push_back(*i2);

      if (res==0 &&
          (pr_chd ||
           i1->bag_amount != i2->bag_amount ||
           i1->bag_weight != i2->bag_weight)) conCHG.tags.push_back(*i2);

      if (res>=0) i1++;
      if (res<=0) i2++;
    };
  }
  else
  {
    conDEL.tags=con1.tags;
    conADD.tags=con2.tags;
  };
  if (!conDEL.tags.empty())
    bsms.push_back(conDEL);
  vector<TBSMTagItem>::iterator i;
  for(i=conCHG.tags.begin();i!=conCHG.tags.end();i++)
  {
    bsms.push_back(conCHG);
    TBSMContent &con=bsms.back();
    con.tags.clear();
    con.tags.push_back(*i);
  };
  for(i=conADD.tags.begin();i!=conADD.tags.end();i++)
  {
    bsms.push_back(conADD);
    TBSMContent &con=bsms.back();
    con.tags.clear();
    con.tags.push_back(*i);
  };
  return bsms;
};

void TelegramInterface::LoadBSMContent(int grp_id, TBSMContent& con)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,flt_no,suffix,airp,scd_out, "
    "       airp_arv,class "
    "FROM points,pax_grp "
    "WHERE point_id=point_dep AND grp_id=:grp_id AND pr_refuse=0";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (Qry.Eof) return;
  strcpy(con.OutFlt.airline,Qry.FieldAsString("airline"));
  con.OutFlt.flt_no=Qry.FieldAsInteger("flt_no");
  strcpy(con.OutFlt.suffix,Qry.FieldAsString("suffix"));
  strcpy(con.OutFlt.airp_dep,Qry.FieldAsString("airp"));
  strcpy(con.OutFlt.airp_arv,Qry.FieldAsString("airp_arv"));
  strcpy(con.OutFlt.subcl,Qry.FieldAsString("class"));
  con.OutFlt.scd=UTCToLocal(Qry.FieldAsDateTime("scd_out"),AirpTZRegion(con.OutFlt.airp_dep));

  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,flt_no,suffix,airp_arv,local_date,subclass "
    "FROM transfer "
    "WHERE grp_id=:grp_id "
    "ORDER BY transfer_num";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  TBaseTable &airlines=base_tables.get("airlines");
  TBaseTable &airps=base_tables.get("airps");
  TBaseTable &subcls=base_tables.get("subcls");
  TDateTime d=con.OutFlt.scd-1;
  for(;!Qry.Eof;Qry.Next())
  {
    TTransferItem flt;

    strcpy(flt.airline,Qry.FieldAsString("airline"));
    string airline=airlines.get_row("code/code_lat",flt.airline).AsString("code");
    strcpy(flt.airline,airline.c_str());

    flt.flt_no=Qry.FieldAsInteger("flt_no");
    strcpy(flt.suffix,Qry.FieldAsString("suffix"));

    strcpy(flt.airp_arv,Qry.FieldAsString("airp_arv"));
    string airp=airps.get_row("code/code_lat",flt.airp_arv).AsString("code");
    strcpy(flt.airp_arv,airp.c_str());

    strcpy(flt.subcl,Qry.FieldAsString("subclass"));
    string subcl=subcls.get_row("code/code_lat",flt.subcl).AsString("code");
    strcpy(flt.subcl,subcl.c_str());

    flt.scd=DayToDate(Qry.FieldAsInteger("local_date"),d);
    d=flt.scd-1;

    con.OnwardFlt.push_back(flt);
  };

  Qry.Clear();
  Qry.SQLText=
    "SELECT no,amount,weight FROM bag2,bag_tags "
    "WHERE bag_tags.grp_id=bag2.grp_id(+) AND "
    "      bag_tags.bag_num=bag2.num(+) AND "
    "      bag_tags.grp_id=:grp_id "
    "ORDER BY no";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TBSMTagItem tag;
    tag.no=Qry.FieldAsInteger("no");
    if (!Qry.FieldIsNULL("amount"))
      tag.bag_amount=Qry.FieldAsInteger("amount");
    if (!Qry.FieldIsNULL("weight"))
      tag.bag_weight=Qry.FieldAsInteger("weight");

    con.tags.push_back(tag);
  };

  Qry.Clear();
  Qry.SQLText =
      "select "
      "   pax_id "
      "from "
      "   pax "
      "where "
      "   grp_id = :grp_id and "
      "   refuse is null "
      "order by "
      "   decode(seats, 0, 1, 0), "
      "   decode(pers_type, '��', 0, '��', 1, 2), "
      "   reg_no ";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (!Qry.Eof)
  {
    int pax_id=Qry.FieldAsInteger("pax_id");
    Qry.Clear();
    Qry.SQLText =
      "SELECT reg_no,surname,name,seat_no, "
      "       DECODE(pr_brd,NULL,'N',0,'C','B') AS status "
      "FROM pax WHERE pax_id=:pax_id";
    Qry.CreateVariable("pax_id",otInteger,pax_id);
    Qry.Execute();
    if (!Qry.Eof)
    {
      con.pax.reg_no=Qry.FieldAsInteger("reg_no");
      con.pax.surname=Qry.FieldAsString("surname");
      con.pax.name=Qry.FieldAsString("name");
      con.pax.seat_no=Qry.FieldAsString("seat_no");
      con.pax.status=Qry.FieldAsString("status");
    };
    vector<TPnrAddrItem> pnrs;
    con.pax.pnr_addr=GetPaxPnrAddr(pax_id,pnrs);
  };

  Qry.Clear();
  Qry.SQLText =
    "SELECT SUM(weight) AS weight FROM bag2 "
    "WHERE grp_id=:grp_id AND pr_cabin<>0 ";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (!Qry.Eof && !Qry.FieldIsNULL("weight"))
  {
    con.bag.rk_weight=Qry.FieldAsInteger("weight");
  };
};

/*
class TTlgFltInfo
{
  public:
    std::string airline,suffix,airp,tz_region;
    int flt_no;
    BASIC::TDateTime scd_out;
};

TTlgFltInfo& GetFltInfo(int point_id, TTlgFltInfo &info)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,flt_no,suffix,airp, "
    "       system.AirpTZRegion(points.airp) AS tz_region, "
    "       scd_out "
    "FROM points WHERE point_id=:point_id";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) ; //???
  info.airline=Qry.FieldAsString("airline");
  info.flt_no=Qry.FieldAsInteger("flt_no");
  info.suffix=Qry.FieldAsString("suffix");
  info.airp=Qry.FieldAsString("airp");
  info.tz_region=Qry.FieldAsString("tz_region");
  info.scd_out=Qry.FieldAsDateTime("scd_out");
  return info;
};*/

void TelegramInterface::CreateBSMBody(TBSMContent& con, bool pr_lat)
{
  TBaseTable &airlines=base_tables.get("airlines");
  TBaseTable &airps=base_tables.get("airps");
  TBaseTable &subcls=base_tables.get("subcls");

  ostringstream body;

  body << ".V/1L"
       << airps.get_row("code",con.OutFlt.airp_dep).AsString("code",pr_lat) << endl;


  body << ".F/"
       << airlines.get_row("code",con.OutFlt.airline).AsString("code",pr_lat)
       << setw(3) << setfill('0') << con.OutFlt.flt_no
       << convert_suffix(con.OutFlt.suffix,pr_lat) << '/'
       << DateTimeToStr( con.OutFlt.scd, "ddmmm", pr_lat) << '/'
       << airps.get_row("code",con.OutFlt.airp_arv).AsString("code",pr_lat) << '/'
       << subcls.get_row("code",con.OutFlt.subcl).AsString("code",pr_lat) << endl;

  for(vector<TTransferItem>::iterator i=con.OnwardFlt.begin();i!=con.OnwardFlt.end();i++)
  {
    body << ".O/"
         << airlines.get_row("code",i->airline).AsString("code",pr_lat)
         << setw(3) << setfill('0') << i->flt_no
         << convert_suffix(i->suffix,pr_lat) << '/'
         << DateTimeToStr( i->scd, "ddmmm", pr_lat) << '/'
         << airps.get_row("code",i->airp_arv).AsString("code",pr_lat) << '/'
         << subcls.get_row("code",i->subcl).AsString("code",pr_lat) << endl;
  };

  if (!con.tags.empty())
  {
    double first_no;
    int num;
    vector<TBSMTagItem>::iterator i=con.tags.begin();
    while(true)
    {
      if (i!=con.tags.begin() &&
          (i==con.tags.end() || i->no!=first_no+num+1))
      {
        body << ".N/"
             << setw(10) << setfill('0') << setprecision(0) << first_no << '/'
             << setw(3) << setfill('0') << num << endl;
      };
      if (i==con.tags.end()) break;
      if (i==con.tags.begin() || i->no!=first_no+num+1)
      {
        first_no=i->no;
        num=1;
      }
      else num++;
      i++;
    };
  };

  if (con.pax.reg_no!=-1)
    body << ".S/"
         << (char)(con.indicator==DEL?'N':'Y') << '/'
         << convert_seat_no(con.pax.seat_no,pr_lat) << '/'
         << con.pax.status << '/'
         << setw(3) << setfill('0') << con.pax.reg_no << endl;

  //if





};


