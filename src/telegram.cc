#include <vector>
#include <utility>
#include <boost/date_time/local_time/local_time.hpp>
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
#include "astra_service.h"
#include "serverlib/logger.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

#define ENDL "\015\012"

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
    "SELECT airline,flt_no,suffix,airp,scd_out, "
    "       point_num, first_point, pr_tranzit "
    "FROM points WHERE point_id=:point_id AND pr_del>=0";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс не найден. Обновите данные");

  TTripInfo fltInfo(Qry);
  TTripRoute route;

  route.GetRouteAfter(point_id,
                      Qry.FieldAsInteger("point_num"),
                      Qry.FieldAsInteger("first_point"),
                      Qry.FieldAsInteger("pr_tranzit")!=0,
                      trtNotCurrent,trtNotCancelled);

  node = NewTextChild( tripdataNode, "airps" );
  vector<string> airps;
  vector<string>::iterator i;
  for(vector<TTripRouteItem>::iterator r=route.begin();r!=route.end();r++)
  {
    //проверим на дублирование кодов аэропортов в рамках одного рейса
    for(i=airps.begin();i!=airps.end();i++)
      if (*i==r->airp) break;
    if (i!=airps.end()) continue;

    NewTextChild( node, "airp", r->airp );

    airps.push_back(r->airp);
  };

  vector<TTripInfo> markFltInfo;
  GetMktFlights(fltInfo,markFltInfo);
  if (!markFltInfo.empty())
  {
    node = NewTextChild( tripdataNode, "mark_flights" );
    for(vector<TTripInfo>::iterator f=markFltInfo.begin();f!=markFltInfo.end();f++)
    {
      xmlNodePtr fltNode=NewTextChild(node,"flight");
      NewTextChild(fltNode,"airline",f->airline);
      NewTextChild(fltNode,"flt_no",f->flt_no);
      NewTextChild(fltNode,"suffix",f->suffix);
      ostringstream flt_str;
      flt_str << f->airline
              << setw(3) << setfill('0') << f->flt_no
              << f->suffix;
      NewTextChild(fltNode,"flt_str",flt_str.str());
    };
  };

  //зачитаем все источники PNL на данный рейс
  vector<string> crs;
  GetCrsList(point_id,crs);
  if (!crs.empty())
  {
    node = NewTextChild( tripdataNode, "crs_list" );
    for(vector<string>::iterator c=crs.begin();c!=crs.end();c++)
      NewTextChild(node,"crs",*c);
  };
};

void TelegramInterface::GetTlgIn(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo &info = *(TReqInfo::Instance());
  int point_id = NodeAsInteger( "point_id", reqNode );

  TQuery Qry(&OraSession);
  string tz_region =  info.desk.tz_region;
  string sql="SELECT tlgs_in.id,num,type,addr,heading,body,ending,time_receive "
             "FROM tlgs_in, ";
  if (point_id!=-1)
  {
    TQuery RegionQry(&OraSession);
    RegionQry.SQLText="SELECT airp FROM points WHERE point_id=:point_id AND pr_del>=0";
    RegionQry.CreateVariable("point_id",otInteger,point_id);
    RegionQry.Execute();
    if (RegionQry.Eof) throw UserException("Рейс не найден. Обновите данные");
    tz_region = AirpTZRegion(RegionQry.FieldAsString("airp"));

    sql+="(SELECT DISTINCT tlg_source.tlg_id AS id "
         " FROM tlg_source,tlg_binding "
         " WHERE tlg_source.point_id_tlg=tlg_binding.point_id_tlg AND "
         "       tlg_binding.point_id_spp=:point_id) ids ";
    Qry.CreateVariable("point_id",otInteger,point_id);
  }
  else
  {
    sql+="( ";
    if (!info.user.access.airlines.empty()||
        !info.user.access.airps.empty())
    {
      sql+="SELECT DISTINCT ids.id "
           "FROM tlg_trips ";
      sql+=",( ";

    };
    sql+="SELECT DISTINCT tlgs_in.id ";
    if (!info.user.access.airlines.empty()||
        !info.user.access.airps.empty()) sql+=",tlg_source.point_id_tlg ";
    sql+="FROM tlgs_in,tlg_source,tlg_binding "
         "WHERE tlgs_in.id=tlg_source.tlg_id AND "
         "      tlg_source.point_id_tlg=tlg_binding.point_id_tlg(+) AND "
         "      tlg_binding.point_id_tlg IS NULL AND "
         "      time_receive>=TRUNC(system.UTCSYSDATE)-2 ";
    if (!info.user.access.airlines.empty()||
        !info.user.access.airps.empty())
    {
      sql+="ORDER BY tlgs_in.id) ids "
           "WHERE ids.point_id_tlg=tlg_trips.point_id ";
      if (!info.user.access.airlines.empty())
      {
        if (info.user.access.airlines_permit)
          sql+="AND tlg_trips.airline IN "+GetSQLEnum(info.user.access.airlines);
        else
          sql+="AND tlg_trips.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
      };
      if (!info.user.access.airps.empty())
      {
        if (info.user.access.airps_permit)
          sql+="AND (tlg_trips.airp_dep IS NULL AND tlg_trips.airp_arv IS NULL OR "
               "     tlg_trips.airp_dep IN "+GetSQLEnum(info.user.access.airps)+" OR "+
               "     tlg_trips.airp_arv IN "+GetSQLEnum(info.user.access.airps)+")" ;
        else
          sql+="AND (tlg_trips.airp_dep IS NULL AND tlg_trips.airp_arv IS NULL OR "
               "     tlg_trips.airp_dep NOT IN "+GetSQLEnum(info.user.access.airps)+" OR "+
               "     tlg_trips.airp_arv NOT IN "+GetSQLEnum(info.user.access.airps)+")" ;
      };
    };
    sql+=" UNION "
         " SELECT DISTINCT tlgs_in.id "
         " FROM tlgs_in,tlg_source "
         " WHERE tlgs_in.id=tlg_source.tlg_id(+) AND tlg_source.tlg_id IS NULL AND "
         "       time_receive>=TRUNC(system.UTCSYSDATE)-2 "
         ") ids ";
  };
  sql+="WHERE tlgs_in.id=ids.id "
       "ORDER BY id,num ";

  xmlNodePtr tlgsNode = NewTextChild( resNode, "tlgs" );
  if (info.user.access.airps_permit && info.user.access.airps.empty() ||
      info.user.access.airlines_permit && info.user.access.airlines.empty() ) return;

  Qry.SQLText=sql;
  Qry.Execute();
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
  string sql="SELECT point_id,id,num,addr,heading,body,ending,extra, "
             "       pr_lat,completed,time_create,time_send_scd,time_send_act, "
             "       typeb_types.code AS tlg_type,typeb_types.short_name AS tlg_short_name, "
             "       typeb_types.basic_type, typeb_types.editable "
             "FROM tlg_out,typeb_types "
             "WHERE tlg_out.type=typeb_types.code ";
  if (node==NULL)
  {
    int tlg_id = NodeAsInteger( "tlg_id", reqNode );
    sql+="AND id=:tlg_id ";
    Qry.CreateVariable("tlg_id",otInteger,tlg_id);
  }
  else
  {
    point_id = NodeAsInteger( node );
    if (point_id!=-1)
    {
      sql+="AND point_id=:point_id ";
      Qry.CreateVariable("point_id",otInteger,point_id);
    }
    else
    {
      sql+="AND point_id IS NULL AND time_create>=TRUNC(system.UTCSYSDATE)-2 ";
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
      RegionQry.SQLText="SELECT airp FROM points WHERE point_id=:point_id AND pr_del>=0";
      RegionQry.CreateVariable("point_id",otInteger,point_id);
      RegionQry.Execute();
      if (RegionQry.Eof) throw UserException("Рейс не найден. Обновите данные");
      tz_region = AirpTZRegion(RegionQry.FieldAsString("airp"));
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
    string basic_type = Qry.FieldAsString("basic_type");

    NewTextChild( node, "id", Qry.FieldAsInteger("id") );
    NewTextChild( node, "num", Qry.FieldAsInteger("num") );
    NewTextChild( node, "tlg_type", Qry.FieldAsString("tlg_type"), basic_type );
    NewTextChild( node, "tlg_short_name", Qry.FieldAsString("tlg_short_name"), basic_type );
    NewTextChild( node, "basic_type", basic_type );
    NewTextChild( node, "editable", (int)Qry.FieldAsInteger("editable")!=0, (int)false );

    //потом удалить !!! (обновление терминала 13.03.08)

  /*  NewTextChild( node, "type", basic_type );

    if (basic_type=="PFS")
      NewTextChild( node, "crs", Qry.FieldAsString("extra") );
    else
      NewTextChild( node, "crs" );
    if (basic_type=="PTM" ||
        basic_type=="BTM")
      NewTextChild( node, "airp", Qry.FieldAsString("extra") );
    else
      NewTextChild( node, "airp" );*/
    //потом удалить !!!

    NewTextChild( node, "addr", Qry.FieldAsString("addr") );
    NewTextChild( node, "heading", Qry.FieldAsString("heading") );
    NewTextChild( node, "ending", Qry.FieldAsString("ending") );
    NewTextChild( node, "extra", Qry.FieldAsString("extra"), "" );
    NewTextChild( node, "pr_lat", (int)(Qry.FieldAsInteger("pr_lat")!=0) );
    NewTextChild( node, "completed", (int)(Qry.FieldAsInteger("completed")!=0), (int)true );

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

  if (point_id!=-1)
  {
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
      "SELECT airline,flt_no,airp,point_num,first_point,pr_tranzit "
      "FROM points WHERE point_id=:point_id AND pr_del>=0";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.Eof) throw UserException("Рейс не найден. Обновите данные");

    TTypeBAddrInfo info;

    info.airline=Qry.FieldAsString("airline");
    info.flt_no=Qry.FieldAsInteger("flt_no");
    info.airp_dep=Qry.FieldAsString("airp");
    info.point_id=point_id;
    info.point_num=Qry.FieldAsInteger("point_num");
    info.first_point=Qry.FieldAsInteger("first_point");
    info.pr_tranzit=Qry.FieldAsInteger("pr_tranzit")!=0;

    //с клиента
    info.tlg_type=NodeAsStringFast( "tlg_type", node);
    info.airp_trfer=NodeAsStringFast( "airp_arv", node, "");
    info.crs=NodeAsStringFast( "crs", node, "");
    info.pr_lat=NodeAsIntegerFast( "pr_lat", node)!=0;
    info.mark_info.init(reqNode);

    //!!!потом удалить (17.03.08)
    if (GetNodeFast("pr_numeric",node)!=NULL)
    {
      if (NodeAsIntegerFast("pr_numeric",node)!=0)
      {
        if (info.tlg_type=="PFS") info.tlg_type="PFSN";
        if (info.tlg_type=="PTM") info.tlg_type="PTMN";
      };
    };
    if (info.tlg_type=="MVT") info.tlg_type="MVTA";
    //!!!потом удалить (17.03.08)

    addrs=TelegramInterface::GetTypeBAddrs(info);
  }
  else
  {
    addrs=TelegramInterface::GetTypeBAddrs(NodeAsStringFast( "tlg_type", node),NodeAsIntegerFast( "pr_lat", node)!=0);
  };

  NewTextChild(resNode,"addrs",addrs);
  return;
};

#include "base_tables.h"

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
    "SELECT typeb_types.short_name,point_id FROM tlg_out,typeb_types "
    "WHERE tlg_out.type=typeb_types.code AND id=:id AND num=1 FOR UPDATE";
  Qry.CreateVariable( "id", otInteger, tlg_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Телеграмма не найдена. Обновите данные");
  string tlg_short_name=Qry.FieldAsString("short_name");
  int point_id=Qry.FieldAsInteger("point_id");

  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  DELETE FROM tlg_out WHERE id=:id AND num<>1; "
    "  UPDATE tlg_out SET body=:body,completed=1 WHERE id=:id; "
    "END;";
  Qry.CreateVariable( "id", otInteger, tlg_id);
  Qry.CreateVariable( "body", otString, tlg_body );
  Qry.Execute();

  ostringstream msg;
  msg << "Телеграмма " << tlg_short_name << " (ид=" << tlg_id << ") изменена";
  TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
  showMessage("Телеграмма успешно сохранена");
};

void TelegramInterface::SendTlg(int tlg_id)
{
  try
  {
    TQuery TlgQry(&OraSession);
    TlgQry.Clear();
    TlgQry.SQLText=
      "SELECT id,num,type,typeb_types.short_name,point_id,addr,heading,body,ending,completed "
      "FROM tlg_out,typeb_types "
      "WHERE tlg_out.type=typeb_types.code AND id=:id FOR UPDATE";
    TlgQry.CreateVariable( "id", otInteger, tlg_id);
    TlgQry.Execute();
    if (TlgQry.Eof) throw UserException("Телеграмма не найдена. Обновите данные");
    if (TlgQry.FieldAsInteger("completed")==0)
      throw UserException("Текст телеграммы требует ручной коррекции");

    string tlg_type=TlgQry.FieldAsString("type");
    string tlg_short_name=TlgQry.FieldAsString("short_name");
    int point_id=TlgQry.FieldAsInteger("point_id");

    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText="SELECT canon_name FROM addrs WHERE addr=:addr";
    Qry.DeclareVariable("addr",otString);

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
                throw UserException("Неверно указан SITA-адрес %s",tlg.lex);
              for(char *p=tlg.lex;*p!=0;p++)
                if (!(IsUpperLetter(*p)&&*p>='A'&&*p<='Z'||IsDigit(*p)))
                  throw UserException("Неверно указан SITA-адрес %s",tlg.lex);
              char addr[8];
              strcpy(addr,tlg.lex);
              Qry.SetVariable("addr",addr);
              Qry.Execute();
              if (!Qry.Eof) canon_name=Qry.FieldAsString("canon_name");
              else
              {
                addr[5]=0; //обрезаем до 5-ти символов
                Qry.SetVariable("addr",addr);
                Qry.Execute();
                if (!Qry.Eof) canon_name=Qry.FieldAsString("canon_name");
                else
                {
                  if (*(DEF_CANON_NAME())!=0)
                    canon_name=DEF_CANON_NAME();
                  else
                    throw UserException("Не определен канонический адрес для SITA-адреса %s",tlg.lex);
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
          throw UserException("Неверный формат адресной строки");
        };
        old_addrs=TlgQry.FieldAsString("addr");
      };
      if (recvs.empty()) throw UserException("Не указаны адреса получателей телеграммы");

      //формируем телеграмму
      tlg_text=(string)TlgQry.FieldAsString("heading")+
               TlgQry.FieldAsString("body")+TlgQry.FieldAsString("ending");

      for(i=recvs.begin();i!=recvs.end();i++)
      {
        string addrs=format_addr_line(i->second);
      	if (i->first.size()<=5)
      	{
          if (OWN_CANON_NAME()==i->first)
            /* сразу помещаем во входную очередь */
            loadTlg(addrs+tlg_text);
          else
            sendTlg(i->first.c_str(),OWN_CANON_NAME(),false,0,
                    addrs+tlg_text);
        }
        else
        {
          //это передача файлов
          //string data=addrs+tlg_text; без заголовка
          string data=TlgQry.FieldAsString("body");
          map<string,string> params;
          putFile(i->first,OWN_POINT_ADDR(),tlg_type,params,data);
        };
      };
    };

    TlgQry.Clear();
    TlgQry.SQLText=
      "UPDATE tlg_out SET time_send_act=system.UTCSYSDATE WHERE id=:id";
    TlgQry.CreateVariable( "id", otInteger, tlg_id);
    TlgQry.Execute();
    ostringstream msg;
    msg << "Телеграмма " << tlg_short_name << " (ид=" << tlg_id << ") отправлена";
    TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
  }
  catch(EOracleError &E)
  {
    if ( E.Code > 20000 )
    {
      string str = E.what();
      throw UserException(EOracleError2UserException(str));
    }
    else
      throw;
  };
};

void TelegramInterface::SendTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  SendTlg(NodeAsInteger( "tlg_id", reqNode ));
  showMessage("Телеграмма отправлена");
  GetTlgOut(ctxt,reqNode,resNode);
}

void TelegramInterface::DeleteTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int tlg_id = NodeAsInteger( "tlg_id", reqNode );
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
        "SELECT typeb_types.basic_type, typeb_types.short_name,point_id FROM tlg_out,typeb_types "
        "WHERE tlg_out.type=typeb_types.code AND id=:id AND num=1 FOR UPDATE";
    Qry.CreateVariable( "id", otInteger, tlg_id);
    Qry.Execute();
    if (Qry.Eof) throw UserException("Телеграмма не найдена. Обновите данные");
    string tlg_short_name=Qry.FieldAsString("short_name");
    string tlg_basic_type=Qry.FieldAsString("basic_type");
    int point_id=Qry.FieldAsInteger("point_id");

    Qry.Clear();
    Qry.SQLText=
        "DELETE FROM tlg_out WHERE id=:id AND time_send_act IS NULL ";
    Qry.CreateVariable( "id", otInteger, tlg_id);
    Qry.Execute();
    if (Qry.RowsProcessed()>0)
    {
        ostringstream msg;
        msg << "Телеграмма " << tlg_short_name << " (ид=" << tlg_id << ") удалена";
        TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
        showMessage("Телеграмма удалена");
    };
    GetTlgOut(ctxt,reqNode,resNode);
};

bool TelegramInterface::IsTypeBSend( TTypeBSendInfo &info )
{
  ostringstream sql;
  TQuery SendQry(&OraSession);
  SendQry.Clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT basic_type,pr_dep FROM typeb_types WHERE code=:tlg_type";
  Qry.CreateVariable("tlg_type",otString,info.tlg_type);
  Qry.Execute();
  if (Qry.Eof) return false;

  bool pr_dep=false,pr_arv=false;
  if (Qry.FieldIsNULL("pr_dep"))
  {
    //не привязывается к прилету и вылету
    pr_dep=true;
    pr_arv=true;
    sql << "SELECT pr_denial, "
           "       DECODE(airline,NULL,0,8)+ "
           "       DECODE(flt_no,NULL,0,1)+ "
           "       DECODE(airp_dep,NULL,0,4)+ "
           "       DECODE(airp_arv,NULL,0,2) AS priority ";
  }
  else
  {
    if (Qry.FieldAsInteger("pr_dep")!=0)
    {
      pr_dep=true;
      sql << "SELECT pr_denial, "
             "       DECODE(airline,NULL,0,8)+ "
             "       DECODE(flt_no,NULL,0,1)+ "
             "       DECODE(airp_dep,NULL,0,4)+ "
             "       DECODE(airp_arv,NULL,0,2) AS priority ";
    }
    else
    {
      pr_arv=true;
      sql << "SELECT pr_denial, "
             "       DECODE(airline,NULL,0,8)+ "
             "       DECODE(flt_no,NULL,0,1)+ "
             "       DECODE(airp_dep,NULL,0,2)+ "
             "       DECODE(airp_arv,NULL,0,4) AS priority ";
    };
  };

  sql << "FROM typeb_send "
         "WHERE tlg_type=:tlg_type AND "
         "      (airline IS NULL OR airline=:airline) AND "
         "      (flt_no IS NULL OR flt_no=:flt_no) ";
  SendQry.CreateVariable("tlg_type",otString,info.tlg_type);
  SendQry.CreateVariable("airline",otString,info.airline);
  SendQry.CreateVariable("flt_no",otInteger,info.flt_no);

  if (pr_dep)
  {
    //привязывается к вылету
    sql << " AND "
           "(airp_dep=:airp_dep OR airp_dep IS NULL) AND "
           "(airp_arv IN "
           "  (SELECT airp FROM points "
           "   WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0) OR airp_arv IS NULL)";
    SendQry.CreateVariable("airp_dep",otString,info.airp_dep);
  };
  if (pr_arv)
  {
    //привязывается к прилету
    sql << " AND "
           "(airp_arv=:airp_arv OR airp_arv IS NULL) AND "
           "(airp_dep IN "
           "  (SELECT airp FROM points "
           "   WHERE :first_point IN (point_id,first_point) AND point_num<=:point_num AND pr_del=0) OR airp_dep IS NULL)";
    if (info.airp_arv.empty())
    {
      TTripRoute route;
      route.GetRouteAfter(info.point_id,
                          info.point_num,
                          info.first_point,
                          info.pr_tranzit,
                          trtNotCurrent,trtNotCancelled);
      if (!route.empty())
        info.airp_arv=route.begin()->airp;
    };
    SendQry.CreateVariable("airp_arv",otString,info.airp_arv);
  };

  if (!info.pr_tranzit)
    SendQry.CreateVariable("first_point",otInteger,info.point_id);
  else
    SendQry.CreateVariable("first_point",otInteger,info.first_point);
  SendQry.CreateVariable("point_num",otInteger,info.point_num);

  sql << "ORDER BY priority DESC";

  SendQry.SQLText=sql.str().c_str();
  SendQry.Execute();
  if (SendQry.Eof||SendQry.FieldAsInteger("pr_denial")!=0) return false;
  return true;
};

string TelegramInterface::GetTypeBAddrs( TTypeBAddrInfo &info )
{
  ostringstream sql;
  TQuery AddrQry(&OraSession);
  AddrQry.Clear();
  sql << "SELECT addr FROM typeb_addrs "
         "WHERE pr_mark_flt=:pr_mark_flt AND tlg_type=:tlg_type AND "
         "      (airline=:airline OR airline IS NULL) AND "
         "      (flt_no=:flt_no OR flt_no IS NULL) AND "
         "      pr_lat=:pr_lat ";
  AddrQry.CreateVariable("tlg_type",otString,info.tlg_type);
  if (info.mark_info.IsNULL())
  {
    AddrQry.CreateVariable("pr_mark_flt",otString,(int)false);
    AddrQry.CreateVariable("airline",otString,info.airline);
    AddrQry.CreateVariable("flt_no",otInteger,info.flt_no);
  }
  else
  {
    sql << " AND pr_mark_header=:pr_mark_header ";
    AddrQry.CreateVariable("pr_mark_flt",otString,(int)true);
    AddrQry.CreateVariable("pr_mark_header",otString,(int)info.mark_info.pr_mark_header);
    AddrQry.CreateVariable("airline",otString,info.mark_info.airline);
    AddrQry.CreateVariable("flt_no",otInteger,info.mark_info.flt_no);
  }
  AddrQry.CreateVariable("pr_lat",otInteger,(int)info.pr_lat);

//  ProgTrace(TRACE5,"GetTypeBAddrs: tlg_type=%s flt_no=%d first_point=%d point_num=%d airp_dep=%s airp_arv=%s",
//                   info.tlg_type.c_str(),info.flt_no,info.first_point,info.point_num,info.airp_dep.c_str(),info.airp_arv.c_str());

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT basic_type,pr_dep FROM typeb_types WHERE code=:tlg_type";
  Qry.CreateVariable("tlg_type",otString,info.tlg_type);
  Qry.Execute();
  if (Qry.Eof) return "";

  string basic_type=Qry.FieldAsString("basic_type");

  bool pr_dep=false,pr_arv=false;
  if (Qry.FieldIsNULL("pr_dep"))
  {
    //не привязывается к прилету и вылету
    pr_dep=true;
    pr_arv=true;
  }
  else
  {
    if (Qry.FieldAsInteger("pr_dep")!=0)
      pr_dep=true;
    else
      pr_arv=true;
  };

  if (pr_dep)
  {
    //привязывается к вылету
    sql << " AND "
           "(airp_dep=:airp_dep OR airp_dep IS NULL) AND "
           "(airp_arv IN "
           "  (SELECT airp FROM points "
           "   WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0) OR airp_arv IS NULL)";
    AddrQry.CreateVariable("airp_dep",otString,info.airp_dep);
  };
  if (pr_arv)
  {
    //привязывается к прилету
    sql << " AND "
           "(airp_arv=:airp_arv OR airp_arv IS NULL) AND "
           "(airp_dep IN "
           "  (SELECT airp FROM points "
           "   WHERE :first_point IN (point_id,first_point) AND point_num<=:point_num AND pr_del=0) OR airp_dep IS NULL)";
    if (info.airp_arv.empty())
    {
      TTripRoute route;
      route.GetRouteAfter(info.point_id,
                          info.point_num,
                          info.first_point,
                          info.pr_tranzit,
                          trtNotCurrent,trtNotCancelled);
      if (!route.empty())
        info.airp_arv=route.begin()->airp;
    };
    AddrQry.CreateVariable("airp_arv",otString,info.airp_arv);
  };
  if (!info.pr_tranzit)
    AddrQry.CreateVariable("first_point",otInteger,info.point_id);
  else
    AddrQry.CreateVariable("first_point",otInteger,info.first_point);
  AddrQry.CreateVariable("point_num",otInteger,info.point_num);

  if ((basic_type=="PTM" || basic_type=="BTM") && !info.airp_trfer.empty())
  {
    sql << " AND (airp_arv=:airp_trfer OR airp_arv IS NULL)";
    AddrQry.CreateVariable("airp_trfer",otString,info.airp_trfer);
  };
  if (basic_type=="PFS" ||
      basic_type=="PRL" ||
      basic_type=="FTL")
  {
    sql << " AND (crs=:crs OR crs IS NULL AND :crs IS NULL)";
    AddrQry.CreateVariable("crs",otString,info.crs);
  };

  AddrQry.SQLText=sql.str().c_str();
  AddrQry.Execute();

  vector<string> addrs;
  string addrs2;
  string addr;
  for(;!AddrQry.Eof;AddrQry.Next())
  {
    addrs2=AddrQry.FieldAsString("addr");
    addr = fetch_addr(addrs2);
    while(!addr.empty())
    {
      addrs.push_back(addr);
      addr = fetch_addr(addrs2);
    };
  };
  sort(addrs.begin(),addrs.end());

  addrs2.clear();
  addr.clear();
  for(vector<string>::iterator i=addrs.begin();i!=addrs.end();i++)
  {
    if (*i==addr) continue;
    addr=*i;
    addrs2+=addr+" ";
  };
  return addrs2;
};

string TelegramInterface::GetTypeBAddrs( std::string tlg_type, bool pr_lat )
{
  TQuery AddrQry(&OraSession);
  AddrQry.SQLText=
    "SELECT addr FROM typeb_addrs "
    "WHERE tlg_type=:tlg_type AND pr_lat=:pr_lat";
  AddrQry.CreateVariable("tlg_type",otString,tlg_type);
  AddrQry.CreateVariable("pr_lat",otInteger,(int)pr_lat);
  AddrQry.Execute();

  string addrs;
  for(;!AddrQry.Eof;AddrQry.Next())
  {
    addrs=addrs+AddrQry.FieldAsString("addr")+" ";
  };
  return addrs;
};

void TelegramInterface::SendTlg( int point_id, vector<string> &tlg_types )
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,flt_no,suffix,airp,scd_out,act_out, "
    "       point_num,first_point,pr_tranzit "
    "FROM points WHERE point_id=:point_id AND pr_del>=0";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс не найден. Обновите данные");

  TTripInfo fltInfo(Qry);

  TTypeBSendInfo sendInfo(fltInfo);
  sendInfo.point_id=point_id;
  sendInfo.first_point=Qry.FieldAsInteger("first_point");
  sendInfo.point_num=Qry.FieldAsInteger("point_num");
  sendInfo.pr_tranzit=Qry.FieldAsInteger("pr_tranzit")!=0;

  TTypeBAddrInfo addrInfo(sendInfo);

  //получим все аэропорты по маршруту
  vector<string> airp_arv;
  TTripRoute route;
  route.GetRouteAfter(point_id,
                      Qry.FieldAsInteger("point_num"),
                      Qry.FieldAsInteger("first_point"),
                      Qry.FieldAsInteger("pr_tranzit")!=0,
                      trtNotCurrent,trtNotCancelled);
  if (!route.empty())
  {
    addrInfo.airp_arv=route.begin()->airp;
    sendInfo.airp_arv=route.begin()->airp;
  };
  for(vector<TTripRouteItem>::iterator r=route.begin();r!=route.end();r++)
    if (find(airp_arv.begin(),airp_arv.end(),r->airp)==airp_arv.end())
      airp_arv.push_back(r->airp);

  //получим все системы бронирования из кот. была посылка
  vector<string> crs;
  GetCrsList(point_id, crs);

  //получим список коммерческих рейсов если таковые имеются
  vector<TTripInfo> markFltInfo;
  GetMktFlights(fltInfo,markFltInfo);

  Qry.Clear();
  Qry.SQLText="SELECT basic_type,short_name FROM typeb_types WHERE code=:tlg_type";
  Qry.DeclareVariable("tlg_type",otString);

  time_t time_start,time_end;

  vector<string>::iterator t;
  for(t=tlg_types.begin();t!=tlg_types.end();t++)
  {
    Qry.SetVariable("tlg_type",*t); //выбор информации по типу телеграммы
    Qry.Execute();
    if (Qry.Eof)
    {
      ProgError(STDLOG,"SendTlg: Unknown telegram type %s",t->c_str());
      continue;
    };
    string basic_type=Qry.FieldAsString("basic_type");
    string short_name=Qry.FieldAsString("short_name");

    sendInfo.tlg_type=*t;
    if (!IsTypeBSend(sendInfo)) continue;

    //формируем телеграмму
    vector<string> airp_arvh;
    if (basic_type=="PTM" ||
        basic_type=="BTM")
      airp_arvh=airp_arv;
    else
      airp_arvh.push_back("");

    vector<string> crsh;
    crsh.push_back("");
    if (basic_type=="PFS" ||
        basic_type=="PRL" ||
        basic_type=="FTL" )
      crsh.insert(crsh.end(),crs.begin(),crs.end());

    vector<TCodeShareInfo> codeshare;
    TCodeShareInfo codeshareInfo;
    codeshare.push_back(codeshareInfo); //добавляем пустую это не ошибка!
    for(vector<TTripInfo>::iterator f=markFltInfo.begin();f!=markFltInfo.end();f++)
    {
      codeshareInfo.airline=f->airline;
      codeshareInfo.flt_no=f->flt_no;
      codeshareInfo.suffix=f->suffix;
      codeshare.push_back(codeshareInfo);
    };

    addrInfo.tlg_type=*t;

    for(int pr_lat=0;pr_lat<=1;pr_lat++)
    {
      addrInfo.pr_lat=pr_lat!=0;

      for(vector<string>::iterator i=airp_arvh.begin();i!=airp_arvh.end();i++)
      {
        addrInfo.airp_trfer=*i;

        for(vector<string>::iterator j=crsh.begin();j!=crsh.end();j++)
        {
          addrInfo.crs=*j;

          //цикл по фактическому и коммерческим рейсам
          vector<TCodeShareInfo>::iterator k=codeshare.begin();
          for(;k!=codeshare.end();k++)
          {
            addrInfo.mark_info=*k;
            for(int pr_mark_header=0;
                pr_mark_header<=(addrInfo.mark_info.IsNULL()?0:1);
                pr_mark_header++)
            {
              addrInfo.mark_info.pr_mark_header=pr_mark_header!=0;

              TCreateTlgInfo createInfo;
              createInfo.type=addrInfo.tlg_type;
              createInfo.point_id=point_id;
              createInfo.airp_trfer=addrInfo.airp_trfer;
              createInfo.crs=addrInfo.crs;
              createInfo.extra="";
              createInfo.pr_lat=addrInfo.pr_lat;
              createInfo.mark_info=addrInfo.mark_info;

              createInfo.addrs=GetTypeBAddrs(addrInfo);
              if (createInfo.addrs.empty()) continue;

              try
              {
                  int tlg_id=0;
                  ostringstream msg;
                  try
                  {
                      time_start=time(NULL);
                      tlg_id = create_tlg( createInfo );

                      time_end=time(NULL);
                      if (time_end-time_start>1)
                          ProgTrace(TRACE5,"Attention! c++ create_tlg execute time: %ld secs, type=%s, point_id=%d",
                                  time_end-time_start,
                                  createInfo.type.c_str(),
                                  point_id);

                      msg << "Телеграмма " << short_name
                          << " (ид=" << tlg_id << ") сформирована: ";
                  }
                  catch(UserException E)
                  {
                      msg << "Ошибка формирования телеграммы " << short_name
                          << ": " << E.what() << ", ";
                  }
                  msg << GetTlgLogMsg(createInfo);

                  TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);

                  if (tlg_id!=0)
                  {
                      time_start=time(NULL);
                      try
                      {
                          SendTlg(tlg_id);
                      }
                      catch(UserException &E)
                      {
                          msg.str("");
                          msg << "Ошибка отправки телеграммы " << short_name
                              << " (ид=" << tlg_id << ")"
                              << ": " << E.what();
                          TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
                      };
                      time_end=time(NULL);
                      if (time_end-time_start>1)
                          ProgTrace(TRACE5,"Attention! SendTlg execute time: %ld secs, tlg_id=%d",
                                  time_end-time_start,tlg_id);
                  };
              }
              catch( Exception &E )
              {
                ProgError(STDLOG,"SendTlg (point_id=%d, type=%s): %s",point_id,t->c_str(),E.what());
              }
              catch(...)
              {
                ProgError(STDLOG,"SendTlg (point_id=%d, type=%s): unknown error",point_id,t->c_str());
              };
            };  //for pr_mark_header
          };  //for k
        };  //for j
      };  //for i
    };  //for pr_lat

  };

};

void TelegramInterface::CompareBSMContent(TBSMContent& con1, TBSMContent& con2, vector<TBSMContent>& bsms)
{
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

  //проверяем рейс
  if (strcmp(con1.OutFlt.airline,con2.OutFlt.airline)==0 &&
      con1.OutFlt.flt_no==con2.OutFlt.flt_no &&
      strcmp(con1.OutFlt.suffix,con2.OutFlt.suffix)==0 &&
      con1.OutFlt.scd==con2.OutFlt.scd &&
      strcmp(con1.OutFlt.airp_dep,con2.OutFlt.airp_dep)==0)
  {
    //проверим отличалась ли информация по пассажиру
    bool pr_chd=!(strcmp(con1.OutFlt.airp_arv,con2.OutFlt.airp_arv)==0 &&
                 strcmp(con1.OutFlt.subcl,con2.OutFlt.subcl)==0 &&
                 con1.pax.reg_no==con2.pax.reg_no &&
                 con1.pax.surname==con2.pax.surname &&
                 con1.pax.name==con2.pax.name &&
                 con1.pax.seat_no.get_seat_one(con1.pr_lat_seat)==
                   con2.pax.seat_no.get_seat_one(con2.pr_lat_seat) &&
                 con1.pax.pnr_addr==con2.pax.pnr_addr &&
                 con1.bag.rk_weight==con2.bag.rk_weight);
    if (!pr_chd)
    {
      //придется проверить изменения в стыковочных рейсах
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
    ProgTrace(TRACE5,"OutFlt1 != OutFlt2 pr_chd=%d",(int)pr_chd);

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
  {
    bsms.push_back(conDEL);
  };
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
  return;
};

void TelegramInterface::LoadBSMContent(int grp_id, TBSMContent& con)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT points.point_id,points.point_num,points.first_point,"
    "       airline,flt_no,suffix,airp,scd_out, "
    "       airp_arv,class,NVL(trip_sets.pr_lat_seat,1) AS pr_lat_seat "
    "FROM points,pax_grp,trip_sets "
    "WHERE points.point_id=pax_grp.point_dep AND points.pr_del>=0 AND "
    "      points.point_id=trip_sets.point_id(+) AND "
    "      grp_id=:grp_id AND bag_refuse=0";

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
  con.pr_lat_seat=Qry.FieldAsInteger("pr_lat_seat")!=0;

  TTlgInfo info;

  info.point_id=Qry.FieldAsInteger("point_id");
  info.point_num=Qry.FieldAsInteger("point_num");
  info.first_point=Qry.FieldAsInteger("first_point");

  bool pr_unaccomp=Qry.FieldIsNULL("class");

  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,flt_no,suffix,scd,airp_dep,airp_arv "
    "FROM transfer,trfer_trips "
    "WHERE transfer.point_id_trfer=trfer_trips.point_id AND "
    "      grp_id=:grp_id AND transfer_num>0 "
    "ORDER BY transfer_num";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  TBaseTable &airlines=base_tables.get("airlines");
  TBaseTable &airps=base_tables.get("airps");
  //TBaseTable &subcls=base_tables.get("subcls");

  vector<TTlgCompLayer> complayers;
  ReadSalons( info, complayers );

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

   /* strcpy(flt.subcl,Qry.FieldAsString("subclass")); по причине того что подкласс у каждого пассажира свой
    if (*(flt.subcl)!=0)
    {
      string subcl=subcls.get_row("code/code_lat",flt.subcl).AsString("code");
      strcpy(flt.subcl,subcl.c_str());
    };*/

    flt.scd=Qry.FieldAsDateTime("scd");

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
    tag.no=Qry.FieldAsFloat("no");
    if (!Qry.FieldIsNULL("amount"))
      tag.bag_amount=Qry.FieldAsInteger("amount");
    if (!Qry.FieldIsNULL("weight"))
      tag.bag_weight=Qry.FieldAsInteger("weight");

    con.tags.push_back(tag);
  };

  if (!pr_unaccomp)
  {
    Qry.Clear();
    Qry.SQLText =
        "SELECT ckin.get_main_pax_id(:grp_id,0) AS pax_id FROM dual";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    if (!(Qry.Eof||Qry.FieldIsNULL("pax_id")))
    {
      int pax_id=Qry.FieldAsInteger("pax_id");
      Qry.Clear();
      Qry.SQLText =
        "SELECT reg_no,surname,name, "
        "       DECODE(pr_brd,NULL,'N',0,'C','B') AS status "
        "FROM pax WHERE pax_id=:pax_id";
      Qry.CreateVariable("pax_id",otInteger,pax_id);
      Qry.Execute();
      if (!Qry.Eof)
      {
        con.pax.reg_no=Qry.FieldAsInteger("reg_no");
        con.pax.surname=Qry.FieldAsString("surname");
        con.pax.name=Qry.FieldAsString("name");
        con.pax.status=Qry.FieldAsString("status");
      };
//???      get_seat_list(pax_id,cltCheckin,con.pax.seat_no);
      for ( vector<TTlgCompLayer>::iterator il=complayers.begin(); il!=complayers.end(); il++ ) {
      	if ( pax_id != il->pax_id ) continue;
        ProgTrace( TRACE5, "yname=%s, xname=%s", il->yname.c_str(), il->xname.c_str() );
        con.pax.seat_no.add_seat( il->xname, il->yname );
      }


      vector<TPnrAddrItem> pnrs;
      con.pax.pnr_addr=GetPaxPnrAddr(pax_id,pnrs);
    };
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
    "       scd_out "
    "FROM points WHERE point_id=:point_id AND pr_del>=0";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) ; //???
  info.airline=Qry.FieldAsString("airline");
  info.flt_no=Qry.FieldAsInteger("flt_no");
  info.suffix=Qry.FieldAsString("suffix");
  info.airp=Qry.FieldAsString("airp");
  info.tz_region=AirpTZRegion(Qry.FieldAsString("airp"));
  info.scd_out=Qry.FieldAsDateTime("scd_out");
  return info;
};*/

string TelegramInterface::CreateBSMBody(TBSMContent& con, bool pr_lat)
{
  TBaseTable &airlines=base_tables.get("airlines");
  TBaseTable &airps=base_tables.get("airps");
  TBaseTable &subcls=base_tables.get("subcls");

  ostringstream body;

  body.setf(ios::fixed);

  body << "BSM" << ENDL;

  switch(con.indicator)
  {
    case CHG: body << "CHG" << ENDL;
              break;
    case DEL: body << "DEL" << ENDL;
              break;
     default: ;
  };

  bool pr_unaccomp=*(con.OutFlt.subcl)==0;

  body << ".V/1L"
       << airps.get_row("code",con.OutFlt.airp_dep).AsString("code",pr_lat) << ENDL;


  body << ".F/"
       << airlines.get_row("code",con.OutFlt.airline).AsString("code",pr_lat)
       << setw(3) << setfill('0') << con.OutFlt.flt_no
       << convert_suffix(con.OutFlt.suffix,pr_lat) << '/'
       << DateTimeToStr( con.OutFlt.scd, "ddmmm", pr_lat) << '/'
       << airps.get_row("code",con.OutFlt.airp_arv).AsString("code",pr_lat);
  if (*(con.OutFlt.subcl)!=0)
    body  << '/'
          << subcls.get_row("code",con.OutFlt.subcl).AsString("code",pr_lat);
  body << ENDL;

  for(vector<TTransferItem>::iterator i=con.OnwardFlt.begin();i!=con.OnwardFlt.end();i++)
  {
    body << ".O/"
         << airlines.get_row("code",i->airline).AsString("code",pr_lat)
         << setw(3) << setfill('0') << i->flt_no
         << convert_suffix(i->suffix,pr_lat) << '/'
         << DateTimeToStr( i->scd, "ddmmm", pr_lat) << '/'
         << airps.get_row("code",i->airp_arv).AsString("code",pr_lat);
    if (*(i->subcl)!=0)
      body  << '/'
            << subcls.get_row("code",i->subcl).AsString("code",pr_lat);
    body << ENDL;
  };

  if (!con.tags.empty())
  {
    double first_no;
    int num;
    vector<TBSMTagItem>::iterator i=con.tags.begin();
    while(true)
    {
      if (i!=con.tags.begin() &&
          (i==con.tags.end() || i->no!=first_no+num))
      {
        body << ".N/"
             << setw(10) << setfill('0') << setprecision(0) << first_no
             << setw(3) << setfill('0') << num << ENDL;
      };
      if (i==con.tags.end()) break;
      if (i==con.tags.begin() || i->no!=first_no+num)
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
         << (con.indicator==DEL?'N':'Y') << '/'
         << con.pax.seat_no.get_seat_one(con.pr_lat_seat || pr_lat) << '/'
         << con.pax.status << '/'
         << setw(3) << setfill('0') << con.pax.reg_no << ENDL;

  int bag_amount=0,bag_weight=0;
  bool pr_W=true;
  if (con.tags.size()==1)
  {
    vector<TBSMTagItem>::iterator i=con.tags.begin();
    bag_amount=i->bag_amount;
    bag_weight=i->bag_weight;
  }
  else
  {
    for(vector<TBSMTagItem>::iterator i=con.tags.begin();i!=con.tags.end();i++)
    {
      if (i->bag_amount!=1)
      {
        pr_W=false;
        break;
      };
      bag_amount+=i->bag_amount;
      bag_weight+=i->bag_weight;
    };
  };

  if (pr_W && (bag_amount>0 || bag_weight>0 || con.bag.rk_weight>0))
  {
    body << ".W/K/";
    if (bag_amount>0) body << bag_amount;
    if (bag_weight>0 || con.bag.rk_weight>0)
    {
      body << '/';
      if (bag_weight>0) body << bag_weight;
      if (con.bag.rk_weight>0)
        body << '/' << con.bag.rk_weight;
    };
    body << ENDL;
  };

  if (con.pax.reg_no!=-1)
  {
    body << ".P/"
         << transliter(con.pax.surname,pr_lat);
    if (!con.pax.name.empty())
      body << '/' << transliter(con.pax.name,pr_lat);
    body  << ENDL;

    if (!con.pax.pnr_addr.empty())
      body << ".L/" << convert_pnr_addr(con.pax.pnr_addr,pr_lat) << ENDL;
  }
  else
  {
    if (pr_unaccomp)
      body << ".P/UNACCOMPANIED" << ENDL;
  };


  body << "ENDBSM" << ENDL;

  ProgTrace(TRACE5,"/n%s",body.str().c_str());

  return body.str();
};

void TelegramInterface::SaveTlgOutPart( TTlgOutPartInfo &info )
{
  TQuery Qry(&OraSession);

  if (info.id<0)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT tlg_in_out__seq.nextval AS id FROM dual";
    Qry.Execute();
    if (Qry.Eof) return;
    info.id=Qry.FieldAsInteger("id");
  };

  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO tlg_out(id,num,type,point_id,addr,heading,body,ending,extra, "
    "                    pr_lat,completed,time_create,time_send_scd,time_send_act) "
    "VALUES(:id,:num,:type,:point_id,:addr,:heading,:body,:ending,:extra, "
    "       :pr_lat,0,NVL(:time_create,system.UTCSYSDATE),:time_send_scd,NULL)";
  Qry.CreateVariable("id",otInteger,info.id);
  Qry.CreateVariable("num",otInteger,info.num);
  Qry.CreateVariable("type",otString,info.tlg_type);
  Qry.CreateVariable("point_id",otInteger,info.point_id);
  Qry.CreateVariable("addr",otString,info.addr);
  Qry.CreateVariable("heading",otString,info.heading);
  Qry.CreateVariable("body",otString,info.body);
  Qry.CreateVariable("ending",otString,info.ending);
  Qry.CreateVariable("extra",otString,info.extra);
  Qry.CreateVariable("pr_lat",otInteger,(int)info.pr_lat);
  if (info.time_create!=NoExists)
    Qry.CreateVariable("time_create",otDate,info.time_create);
  else
    Qry.CreateVariable("time_create",otDate,FNull);
  if (info.time_send_scd!=NoExists)
    Qry.CreateVariable("time_send_scd",otDate,info.time_send_scd);
  else
    Qry.CreateVariable("time_send_scd",otDate,FNull);
  Qry.Execute();

  info.num++;
};

bool TelegramInterface::IsBSMSend( TTypeBSendInfo info, map<bool,string> &addrs )
{
  info.tlg_type="BSM";
  if (!IsTypeBSend(info)) return false;

  TTypeBAddrInfo addrInfo(info);

  addrInfo.airp_trfer="";
  addrInfo.crs="";
  for(int pr_lat=0; pr_lat<=1; pr_lat++)
  {
    addrInfo.pr_lat=(bool)pr_lat;
    addrs[addrInfo.pr_lat]=TelegramInterface::GetTypeBAddrs(addrInfo);
  };
  return (!addrs[false].empty() || !addrs[true].empty());
};

void TelegramInterface::SendBSM
  (int point_dep, int grp_id, TBSMContent &con1, map<bool,string> &addrs )
{
    TBSMContent con2;
    TelegramInterface::LoadBSMContent(grp_id,con2);
    vector<TBSMContent> bsms;
    TelegramInterface::CompareBSMContent(con1,con2,bsms);
    TTlgOutPartInfo p;
    p.tlg_type="BSM";
    p.point_id=point_dep;
    p.time_create=NowUTC();
    ostringstream heading;
    heading << '.' << OWN_SITA_ADDR() << ' ' << DateTimeToStr(p.time_create,"ddhhnn") << ENDL;
    p.heading=heading.str();

    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText="UPDATE tlg_out SET completed=1 WHERE id=:id";
    Qry.DeclareVariable("id",otInteger);

    for(vector<TBSMContent>::iterator i=bsms.begin();i!=bsms.end();i++)
    {
      for(map<bool,string>::iterator j=addrs.begin();j!=addrs.end();j++)
      {
        if (j->second.empty()) continue;
        p.id=-1;
        p.num=1;
        p.pr_lat=j->first;
        p.addr=format_addr_line(j->second);
        p.body=TelegramInterface::CreateBSMBody(*i,p.pr_lat);
        TelegramInterface::SaveTlgOutPart(p);
        Qry.SetVariable("id",p.id);
        Qry.Execute();
        TelegramInterface::SendTlg(p.id);
      };
    };
};

void TelegramInterface::TestSeatRanges(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  vector<TSeatRange> ranges;
  try
  {
    ParseSeatRange(NodeAsString("lexeme",reqNode),ranges,true);

    xmlNodePtr rangesNode,rangeNode;
    rangesNode=NewTextChild(resNode,"ranges");

    sort(ranges.begin(),ranges.end());

    if (ranges.begin()!=ranges.end())
    {
      TSeatRange range=*(ranges.begin());
      TSeat seat=range.first;
      do
      {
        TSeatRange range2;
        range2.first=seat;
        range2.second=seat;
        ranges.push_back(range2);
      }
      while (NextSeatInRange(range,seat));
    };

    for(vector<TSeatRange>::iterator i=ranges.begin();i!=ranges.end();i++)
    {
      rangeNode=NewTextChild(rangesNode,"range");
      NewTextChild(rangeNode,"first_row",i->first.row);
      NewTextChild(rangeNode,"first_line",i->first.line);
      NewTextChild(rangeNode,"second_row",i->second.row);
      NewTextChild(rangeNode,"second_line",i->second.line);
    };
  }
  catch(Exception &e)
  {
    throw UserException(e.what());
  };
};

