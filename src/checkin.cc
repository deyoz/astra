#include "checkin.h"
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "stl_utils.h"
#include "xml_unit.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "seats.h"
#include "stages.h"
#include "tripinfo.h"
#include "telegram.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;

void CheckInInterface::LoadTagPacks(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT airline,target,tag_type,no,color FROM tag_packs WHERE desk=:desk";
  Qry.CreateVariable("desk",otString,TReqInfo::Instance()->desk.code);
  Qry.Execute();
  xmlNodePtr node,packNode;
  packNode=NewTextChild(resNode,"tag_packs");
  for(;!Qry.Eof;Qry.Next())
  {
    node=NewTextChild(packNode,"tag_pack");
    NewTextChild(node,"airline",Qry.FieldAsString("airline"));
    NewTextChild(node,"target",Qry.FieldAsString("target"));
    NewTextChild(node,"tag_type",Qry.FieldAsString("tag_type"));
    NewTextChild(node,"no",Qry.FieldAsFloat("no"));
    NewTextChild(node,"color",Qry.FieldAsString("color"));
  };
};

void CheckInInterface::SearchGrp(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_dep = NodeAsInteger("point_dep",reqNode);
  int point_arv = NodeAsInteger("point_arv",reqNode);
  string cl = NodeAsString("class",reqNode);
  char grp_status = DecodeStatus(NodeAsString("status",reqNode));
  int seats_sum = NodeAsInteger("seats",reqNode);

  readTripCounters(point_dep,resNode);

  string airp_arv = NodeAsString("airp_arv",reqNode);

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airp FROM points "
    "WHERE point_id=:point_id AND airp=:airp AND pr_del=0";
  Qry.CreateVariable("point_id",otInteger,point_arv);
  Qry.CreateVariable("airp",otString,airp_arv);
  Qry.Execute();
  if (Qry.Eof) throw UserException("���� �������. ������� �����");

  xmlNodePtr node;
  TQuery PaxQry(&OraSession);
  PaxQry.Clear();
  string sql;
  string select_sql=
    "SELECT crs_pnr.pnr_id, "
    "       MIN(crs_pnr.grp_name) AS grp_name, "
    "       MIN(DECODE(crs_pax.pers_type,'��', "
    "                  crs_pax.surname||' '||crs_pax.name,'')) AS pax_name, "
    "       COUNT(*) AS seats_all, "
    "       SUM(DECODE(pax.pax_id,NULL,1,0)) AS seats ";

  //����� ����
  sql = select_sql +
    "FROM crs_pnr,crs_pax,pax, "
    "  (SELECT DISTINCT crs_pnr.pnr_id ";


  if (grp_status=='K')
  {
    sql+=
        "   FROM crs_pnr,tlg_binding "
        "   WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
        "         tlg_binding.point_id_spp= :point_id AND "
        "         crs_pnr.target= :airp_arv AND "
        "         crs_pnr.class= :class AND "
        "         crs_pnr.wl_priority IS NULL "
        "   UNION "
        "   SELECT DISTINCT crs_pnr.pnr_id ";
  };

  sql+= "   FROM crs_pnr,tlg_binding,crs_displace "
        "   WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
        "         tlg_binding.point_id_spp=crs_displace.point_from AND "
        "         crs_pnr.target=crs_displace.airp_arv_from AND "
        "         crs_pnr.class=crs_displace.class_from AND "
        "         crs_displace.point_to= :point_id AND "
        "         crs_displace.airp_arv_to= :airp_arv AND "
        "         crs_displace.class_to= :class AND "
        "         NOT(crs_displace.point_to= crs_displace.point_from AND "
        "             crs_displace.airp_arv_to= crs_displace.airp_arv_from AND "
        "             crs_displace.class_to= crs_displace.class_from) AND "
        "         crs_displace.pr_goshow= :pr_goshow AND "
        "         crs_pnr.wl_priority IS NULL) ids "
        "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
        "      crs_pax.pax_id=pax.pax_id(+) AND "
        "      ids.pnr_id=crs_pnr.pnr_id AND "
        "      crs_pax.pr_del=0 AND "
        "      crs_pax.seats>0 "
        "GROUP BY crs_pnr.pnr_id "
        "HAVING COUNT(*)>= :seats ";

  PaxQry.SQLText = sql;
  PaxQry.CreateVariable("point_id",otInteger,point_dep);
  PaxQry.CreateVariable("airp_arv",otString,airp_arv);
  PaxQry.CreateVariable("class",otString,cl);
  PaxQry.CreateVariable("pr_goshow",otInteger,(int)(grp_status=='P'));
  PaxQry.CreateVariable("seats",otInteger,seats_sum);
  PaxQry.Execute();

  TQuery PnrAddrQry(&OraSession);
  PnrAddrQry.SQLText =
    "SELECT DECODE(pnr_addrs.airline,tlg_trips.airline,NULL,pnr_addrs.airline) AS airline, "
    "       pnr_addrs.addr "
    "FROM tlg_trips,crs_pnr,pnr_addrs "
    "WHERE tlg_trips.point_id=crs_pnr.point_id AND "
    "      crs_pnr.pnr_id=pnr_addrs.pnr_id AND "
    "      pnr_addrs.pnr_id=:pnr_id "
    "ORDER BY DECODE(pnr_addrs.airline,tlg_trips.airline,0,1),pnr_addrs.airline";
  PnrAddrQry.DeclareVariable("pnr_id",otInteger);

  xmlNodePtr pnrNode=NewTextChild(resNode,"groups");
  for(;!PaxQry.Eof;PaxQry.Next())
  {
    node=NewTextChild(pnrNode,"pnr");
    NewTextChild(node,"pnr_id",PaxQry.FieldAsInteger("pnr_id"));
    NewTextChild(node,"pax_name",PaxQry.FieldAsString("pax_name"));
    NewTextChild(node,"grp_name",PaxQry.FieldAsString("grp_name"));
    NewTextChild(node,"seats",PaxQry.FieldAsInteger("seats"));
    NewTextChild(node,"seats_all",PaxQry.FieldAsInteger("seats_all"));

    PnrAddrQry.SetVariable("pnr_id",PaxQry.FieldAsInteger("pnr_id"));
    PnrAddrQry.Execute();
    if (!PnrAddrQry.Eof)
    {
      if (!PnrAddrQry.FieldIsNULL("airline"))
      {
        node=NewTextChild(node,"pnr_addr");
        NewTextChild(node,"airline",PnrAddrQry.FieldAsString("airline"));
        NewTextChild(node,"addr",PnrAddrQry.FieldAsString("addr"));
      }
      else
        NewTextChild(node,"pnr_addr",PnrAddrQry.FieldAsString("addr"));
    };
  };

  int free=CheckCounters(point_dep,point_arv,(char*)cl.c_str(),grp_status);
  if (free<seats_sum)
    showErrorMessage("����㯭�� ���� ��⠫��� "+IntToString(free));
};

void CheckInInterface::SearchPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_dep = NodeAsInteger("point_dep",reqNode);
  int point_arv = NodeAsInteger("point_arv",reqNode);
  string cl = NodeAsString("class",reqNode);
  char grp_status = DecodeStatus(NodeAsString("status",reqNode));
  int seats_sum = NodeAsInteger("seats",reqNode);

  readTripCounters(point_dep,resNode);

  xmlNodePtr node;
  TQuery PaxQry(&OraSession);
  PaxQry.Clear();
  string sql;
  string select_sql=
    "SELECT crs_pax.pax_id,crs_pnr.point_id,crs_pnr.target,crs_pnr.subclass, "
    "       crs_pnr.class,crs_pax.surname,crs_pax.name,crs_pax.pers_type, "
    "       crs_pax.seat_no,crs_pax.preseat_no, "
    "       crs_pax.seat_type,crs_pax.seats, "
    "       crs_pnr.pnr_id, "
    "       tlg_trips.airline,tlg_trips.flt_no,tlg_trips.scd,tlg_trips.airp_dep, "
    "       report.get_PSPT(crs_pax.pax_id) AS document ";
  if (GetNode("pnr_id",reqNode)==NULL)
  {
    int nPax = NodeAsInteger("count",reqNode);
    string airp_arv = NodeAsString("airp_arv",reqNode);

    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
      "SELECT airp FROM points "
      "WHERE point_id=:point_id AND airp=:airp AND pr_del=0";
    Qry.CreateVariable("point_id",otInteger,point_arv);
    Qry.CreateVariable("airp",otString,airp_arv);
    Qry.Execute();
    if (Qry.Eof) throw UserException("���� �������. ������� �����");

    for(int i=0;i<2;i++)
    {
      string surnames;
      char surname[5];
      surname[4]=0;
      node=NodeAsNode("surnames",reqNode)->children;
      for(;node!=NULL;node=node->next)
      {
        if (!surnames.empty()) surnames+=" OR ";
        strncpy(surname,NodeAsString(node),i==0?4:1);
        surnames+=(string)"crs_pax.surname LIKE '"+surname+"%'";
      };

      //����� ����
      sql = select_sql +
            "FROM tlg_trips,crs_pnr,crs_pax,pax, ";
      if (nPax>1)
        sql+=
            "  (SELECT DISTINCT crs_pnr.pnr_id ";
      else
        sql+=
            "  (SELECT DISTINCT crs_pax.pax_id ";

      if (grp_status=='K')
      {
        sql+=
            "   FROM crs_pnr,tlg_binding,crs_pax "
            "   WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
            "         crs_pnr.pnr_id=crs_pax.pnr_id AND "
            "         tlg_binding.point_id_spp= :point_id AND "
            "         crs_pnr.target= :airp_arv AND "
            "         crs_pnr.class= :class AND "
            "         crs_pnr.wl_priority IS NULL AND"
            "         crs_pax.pr_del=0 AND "
            "         ("+surnames+") "
            "   UNION ";
        if (nPax>1)
          sql+=
            "   SELECT DISTINCT crs_pnr.pnr_id ";
        else
          sql+=
            "   SELECT DISTINCT crs_pax.pax_id ";
      };

      sql+= "   FROM crs_pnr,tlg_binding,crs_displace,crs_pax "
            "   WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
            "         tlg_binding.point_id_spp=crs_displace.point_from AND "
            "         crs_pnr.target=crs_displace.airp_arv_from AND "
            "         crs_pnr.class=crs_displace.class_from AND "
            "         crs_pnr.pnr_id=crs_pax.pnr_id AND "
            "         crs_displace.point_to= :point_id AND "
            "         crs_displace.airp_arv_to= :airp_arv AND "
            "         crs_displace.class_to= :class AND "
            "         NOT(crs_displace.point_to= crs_displace.point_from AND "
            "             crs_displace.airp_arv_to= crs_displace.airp_arv_from AND "
            "             crs_displace.class_to= crs_displace.class_from) AND "
            "         crs_displace.pr_goshow= :pr_goshow AND "
            "         crs_pnr.wl_priority IS NULL AND "
            "         crs_pax.pr_del=0 AND "
            "         ("+surnames+")) ids "
            "WHERE tlg_trips.point_id=crs_pnr.point_id AND "
            "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
            "      crs_pax.pax_id=pax.pax_id(+) AND ";
      if (nPax>1)
        sql+=
            "      ids.pnr_id=crs_pnr.pnr_id AND ";
      else
        sql+=
            "      ids.pax_id=crs_pax.pax_id AND ";

      sql+= "      crs_pax.pr_del=0 AND "
            "      pax.pax_id IS NULL "
            "ORDER BY tlg_trips.point_id,crs_pax.pnr_id,crs_pax.surname,crs_pax.pax_id ";

      PaxQry.SQLText = sql;
      PaxQry.CreateVariable("point_id",otInteger,point_dep);
      PaxQry.CreateVariable("airp_arv",otString,airp_arv);
      PaxQry.CreateVariable("class",otString,cl);
      PaxQry.CreateVariable("pr_goshow",otInteger,(int)(grp_status=='P'));
      PaxQry.Execute();
      if (!PaxQry.Eof) break;
    };
  }
  else
  {
    //���� ����让 ��㯯�
    sql+= select_sql +
      "FROM tlg_trips,crs_pnr,crs_pax,pax "
      "WHERE tlg_trips.point_id=crs_pnr.point_id AND "
      "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      crs_pnr.pnr_id=:pnr_id AND "
      "      crs_pax.pr_del=0 AND "
      "      pax.pax_id IS NULL "
      "ORDER BY tlg_trips.point_id,crs_pax.pnr_id,crs_pax.surname,crs_pax.pax_id ";
    PaxQry.SQLText = sql;
    PaxQry.CreateVariable("pnr_id",otInteger,NodeAsInteger("pnr_id",reqNode));
    PaxQry.Execute();
  };

  TQuery TrferQry(&OraSession);
  TrferQry.SQLText =
    "SELECT transfer_num,airline,flt_no,suffix, "
    "       local_date,airp_arv,subclass "
    "FROM crs_transfer WHERE pnr_id=:pnr_id AND transfer_num>0";
  TrferQry.DeclareVariable("pnr_id",otInteger);

  TQuery PnrAddrQry(&OraSession);
  PnrAddrQry.SQLText =
    "SELECT airline,addr FROM pnr_addrs WHERE pnr_id=:pnr_id";
  PnrAddrQry.DeclareVariable("pnr_id",otInteger);

  TQuery RemQry(&OraSession);
  RemQry.SQLText =
    "SELECT rem_code,rem FROM crs_pax_rem WHERE pax_id=:pax_id";
  RemQry.DeclareVariable("pax_id",otInteger);

  int point_id=-1;
  int pnr_id=-1, pax_id;
  xmlNodePtr tripNode,pnrNode,paxNode;
  string airp_dep="",airp_dep_lat="";

  tripNode=NewTextChild(resNode,"trips");
  for(;!PaxQry.Eof;PaxQry.Next())
  {
    if (PaxQry.FieldAsInteger("point_id")!=point_id)
    {
      node=NewTextChild(tripNode,"trip");
      point_id=PaxQry.FieldAsInteger("point_id");
      NewTextChild(node,"point_id",point_id);
      NewTextChild(node,"airline",PaxQry.FieldAsString("airline"));
      NewTextChild(node,"flt_no",PaxQry.FieldAsInteger("flt_no"));
      NewTextChild(node,"scd",DateTimeToStr(PaxQry.FieldAsDateTime("scd")));
      NewTextChild(node,"airp_dep",PaxQry.FieldAsString("airp_dep"));
      pnrNode=NewTextChild(node,"groups");

      TQuery Qry(&OraSession);
      Qry.SQLText =
        "SELECT code,code_lat FROM airps WHERE :code IN (code,code_lat)";
      Qry.CreateVariable("code",otString,PaxQry.FieldAsString("airp_dep"));
      Qry.Execute();
      if (!Qry.Eof)
      {
        airp_dep=Qry.FieldAsString("code");
        airp_dep_lat=Qry.FieldAsString("code_lat");
      }
      else
      {
        airp_dep="";
        airp_dep_lat="";
      };
    };
    if (PaxQry.FieldAsInteger("pnr_id")!=pnr_id)
    {
      node=NewTextChild(pnrNode,"pnr");
      pnr_id=PaxQry.FieldAsInteger("pnr_id");
      NewTextChild(node,"pnr_id",pnr_id);
      NewTextChild(node,"airp_arv",PaxQry.FieldAsString("target"));
      NewTextChild(node,"subclass",PaxQry.FieldAsString("subclass"));
      NewTextChild(node,"class",PaxQry.FieldAsString("class"));
      paxNode=NewTextChild(node,"passengers");

      TrferQry.SetVariable("pnr_id",pnr_id);
      TrferQry.Execute();
      if (!TrferQry.Eof)
      {
        string airp_arv;
        xmlNodePtr trferNode=NewTextChild(node,"transfer");
        for(;!TrferQry.Eof;TrferQry.Next())
        {
          airp_arv=TrferQry.FieldAsString("airp_arv");
          if (!airp_arv.empty() &&
              (airp_arv==airp_dep || airp_arv==airp_dep_lat)) break;
          xmlNodePtr node2=NewTextChild(trferNode,"segment");
          NewTextChild(node2,"num",TrferQry.FieldAsInteger("transfer_num"));
          NewTextChild(node2,"airline",TrferQry.FieldAsString("airline"));
          NewTextChild(node2,"flt_no",TrferQry.FieldAsInteger("flt_no"));
          NewTextChild(node2,"suffix",TrferQry.FieldAsString("suffix"),"");
          NewTextChild(node2,"local_date",TrferQry.FieldAsInteger("local_date"));
          NewTextChild(node2,"airp_arv",TrferQry.FieldAsString("airp_arv"));
          NewTextChild(node2,"subclass",TrferQry.FieldAsString("subclass"));
        };
      };

      PnrAddrQry.SetVariable("pnr_id",pnr_id);
      PnrAddrQry.Execute();
      if (!PnrAddrQry.Eof)
      {
        string airline=PnrAddrQry.FieldAsString("airline");
        string addr=PnrAddrQry.FieldAsString("addr");
        PnrAddrQry.Next();

        xmlNodePtr pnrAddrNode,node2;
        if (!PnrAddrQry.Eof)
          pnrAddrNode=NewTextChild(node,"pnr_addrs");
        else
          pnrAddrNode=node;
        if (!PnrAddrQry.Eof||airline!=PaxQry.FieldAsString("airline"))
        {
          node2=NewTextChild(pnrAddrNode,"pnr_addr");
          NewTextChild(node2,"airline",airline);
          NewTextChild(node2,"addr",addr);
        }
        else
          NewTextChild(pnrAddrNode,"pnr_addr",addr);

        for(;!PnrAddrQry.Eof;PnrAddrQry.Next())
        {
          node2=NewTextChild(pnrAddrNode,"pnr_addr");
          NewTextChild(node2,"airline",PnrAddrQry.FieldAsString("airline"));
          NewTextChild(node2,"addr",PnrAddrQry.FieldAsString("addr"));
        };
      };
    };
    node=NewTextChild(paxNode,"pax");
    pax_id=PaxQry.FieldAsInteger("pax_id");
    NewTextChild(node,"pax_id",pax_id);
    NewTextChild(node,"surname",PaxQry.FieldAsString("surname"));
    NewTextChild(node,"name",PaxQry.FieldAsString("name"),"");
    NewTextChild(node,"pers_type",PaxQry.FieldAsString("pers_type"),EncodePerson(ASTRA::adult));
    NewTextChild(node,"seat_no",PaxQry.FieldAsString("seat_no"),"");
    NewTextChild(node,"preseat_no",PaxQry.FieldAsString("preseat_no"),"");
    NewTextChild(node,"seat_type",PaxQry.FieldAsString("seat_type"),"");
    NewTextChild(node,"seats",PaxQry.FieldAsInteger("seats"),1);
    NewTextChild(node,"document",PaxQry.FieldAsString("document"),"");

    RemQry.SetVariable("pax_id",pax_id);
    RemQry.Execute();
    if (!RemQry.Eof)
    {
      xmlNodePtr remNode=NewTextChild(node,"rems");
      for(;!RemQry.Eof;RemQry.Next())
      {
        xmlNodePtr node2=NewTextChild(remNode,"rem");
        NewTextChild(node2,"rem_code",RemQry.FieldAsString("rem_code"),"");
        NewTextChild(node2,"rem_text",RemQry.FieldAsString("rem"));
      };
    };
  };

  int free=CheckCounters(point_dep,point_arv,(char*)cl.c_str(),grp_status);
  if (free<seats_sum)
    showErrorMessage("����㯭�� ���� ��⠫��� "+IntToString(free));

  if (GetNode("pnr_id",reqNode)==NULL)
  {
    xmlNodePtr foundPnr=NULL,foundTrip=NULL;
    int nPax = NodeAsInteger("count",reqNode);
    if (nPax==1) return;
    //����ந� ����� ��१����� 䠬���� ���᪮���� �����
    vector<string> surnames;
    vector<string>::iterator i;
    char surname[5];
    surname[4]=0;
    node=NodeAsNode("surnames",reqNode)->children;
    for(;node!=NULL;node=node->next)
    {
      strncpy(surname,NodeAsString(node),4);
      surnames.push_back(surname);
    };
    tripNode=NodeAsNode("trips",resNode)->children;
    for(;tripNode!=NULL;tripNode=tripNode->next)
    {
      pnrNode=NodeAsNode("groups",tripNode)->children;
      //横� �� PNR
      for(;pnrNode!=NULL;pnrNode=pnrNode->next)
      {
        //�஢�ਬ �� ���-�� 祫���� � ��㯯�
        int n=0;
        paxNode=NodeAsNode("passengers",pnrNode)->children;
        for(node=paxNode;node!=NULL;node=node->next) n++;
        if (n!=nPax) continue;

        //�஢�ਬ �� � �⮡� �� 䠬���� ������⢮���� � ��㯯�
        for(i=surnames.begin();i!=surnames.end();i++)
        {
          for(node=paxNode;node!=NULL;node=node->next)
          {
            xmlNodePtr node2 = node->children;
            if (i->size()<3)
            {
              if (strcmp(i->c_str(),NodeAsStringFast("surname",node2))==0) break;
            }
            else
            {
              if (strncmp(i->c_str(),NodeAsStringFast("surname",node2),i->size())==0) break;
            };
          };
          if (node==NULL) break; //�� ������� 䠬����
        };
        if (i==surnames.end())
        {
          //�� 䠬���� �������
          if (foundPnr!=NULL) return; // ������ 2 ���室��� ��㯯�
          foundPnr=pnrNode;
          foundTrip=tripNode;
        };
      };
    };
    if (foundPnr==NULL||foundTrip==NULL) return;
    xmlUnlinkNode(foundTrip);
    tripNode=NodeAsNode("trips",resNode);
    xmlUnlinkNode(tripNode);
    xmlFreeNode(tripNode);
    tripNode=NewTextChild(resNode,"trips");
    xmlAddChild(tripNode,foundTrip);

    xmlUnlinkNode(foundPnr);
    pnrNode=NodeAsNode("groups",foundTrip);
    xmlUnlinkNode(pnrNode);
    xmlFreeNode(pnrNode);
    pnrNode=NewTextChild(foundTrip,"groups");
    xmlAddChild(pnrNode,foundPnr);
  };

};

int CheckInInterface::CheckCounters(int point_dep, int point_arv, char* cl, char grp_status)
{
    //�஢�ઠ ������ ᢮������ ����
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
      "SELECT free_ok,free_goshow,nooccupy FROM counters2 "
      "WHERE point_dep=:point_dep AND point_arv=:point_arv AND class=:class ";
    Qry.CreateVariable("point_dep", otInteger, point_dep);
    Qry.CreateVariable("point_arv", otInteger, point_arv);
    Qry.CreateVariable("class", otString, cl);
    Qry.Execute();
    if (Qry.Eof) throw UserException("���� �������. ������� �����");
    TTripStages tripStages( point_dep );
    TStage ckin_stage =  tripStages.getStage( stCheckIn );
    int free;
    switch (grp_status)
    {
      case 'T': free=Qry.FieldAsInteger("nooccupy");
                break;
      case 'K': if (ckin_stage==sOpenCheckIn)
                  free=Qry.FieldAsInteger("free_ok");
                else
                  free=Qry.FieldAsInteger("nooccupy");
                break;
      default:  if (ckin_stage==sOpenCheckIn)
                  free=Qry.FieldAsInteger("free_goshow");
                else
                  free=Qry.FieldAsInteger("nooccupy");
    };
    if (free>=0)
      return free;
    else
      return 0;
};


void CheckInInterface::PaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  readPaxLoad( point_id, reqNode, resNode );

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT "
    "  reg_no,surname,name,pax_grp.airp_arv,last_trfer,class,pax.subclass, "
    "  LPAD(seat_no,3,'0')||DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') AS seat_no, "
    "  seats,pers_type,document, "
    "  ticket_no||DECODE(coupon_no,NULL,NULL,'/'||coupon_no) AS ticket_no, "
    "  ckin.get_bagAmount(pax.grp_id,pax.reg_no,rownum) AS bag_amount, "
    "  ckin.get_bagWeight(pax.grp_id,pax.reg_no,rownum) AS bag_weight, "
    "  ckin.get_rkWeight(pax.grp_id,pax.reg_no,rownum) AS rk_weight, "
    "  ckin.get_excess(pax.grp_id,pax.reg_no) AS excess, "
    "  ckin.get_birks(pax.grp_id,pax.reg_no) AS tags, "
    "  report.get_remarks(pax_id,0) AS rems, "
    "  pax.grp_id, "
    "  pax_grp.class_grp AS cl_grp_id,pax_grp.hall AS hall_id, "
    "  pax_grp.point_arv,pax_grp.user_id "
    "FROM pax_grp,pax,v_last_trfer "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax_grp.grp_id=v_last_trfer.grp_id(+) AND "
    "      point_dep=:point_id AND pr_brd IS NOT NULL "
    "ORDER BY reg_no";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  xmlNodePtr node=NewTextChild(resNode,"passengers");
  for(;!Qry.Eof;Qry.Next())
  {
    xmlNodePtr paxNode=NewTextChild(node,"pax");
    NewTextChild(paxNode,"reg_no",Qry.FieldAsInteger("reg_no"));
    NewTextChild(paxNode,"surname",Qry.FieldAsString("surname"));
    NewTextChild(paxNode,"name",Qry.FieldAsString("name"));
    NewTextChild(paxNode,"airp_arv",Qry.FieldAsString("airp_arv"));
    NewTextChild(paxNode,"last_trfer",
                 convertLastTrfer(Qry.FieldAsString("last_trfer")),"");
    NewTextChild(paxNode,"class",Qry.FieldAsString("class"));
    NewTextChild(paxNode,"subclass",Qry.FieldAsString("subclass"),"");
    NewTextChild(paxNode,"seat_no",Qry.FieldAsString("seat_no"));
    NewTextChild(paxNode,"pers_type",Qry.FieldAsString("pers_type"));
    NewTextChild(paxNode,"document",Qry.FieldAsString("document"));
    NewTextChild(paxNode,"ticket_no",Qry.FieldAsString("ticket_no"),"");
    NewTextChild(paxNode,"bag_amount",Qry.FieldAsInteger("bag_amount"),0);
    NewTextChild(paxNode,"bag_weight",Qry.FieldAsInteger("bag_weight"),0);
    NewTextChild(paxNode,"rk_weight",Qry.FieldAsInteger("rk_weight"),0);
    NewTextChild(paxNode,"excess",Qry.FieldAsInteger("excess"),0);
    NewTextChild(paxNode,"tags",Qry.FieldAsString("tags"),"");
    NewTextChild(paxNode,"rems",Qry.FieldAsString("rems"),"");
    //�����䨪����
    NewTextChild(paxNode,"grp_id",Qry.FieldAsInteger("grp_id"));
    NewTextChild(paxNode,"cl_grp_id",Qry.FieldAsInteger("cl_grp_id"));
    NewTextChild(paxNode,"hall_id",Qry.FieldAsInteger("hall_id"));
    NewTextChild(paxNode,"point_arv",Qry.FieldAsInteger("point_arv"));
    NewTextChild(paxNode,"user_id",Qry.FieldAsInteger("user_id"));
  };
  Qry.Close();
};

bool GetUsePS()
{
	return false; //!!!
}

void CheckInInterface::SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_dep,point_arv,grp_id,hall;
  string cl,airp_dep,airp_arv;
  TReqInfo *reqInfo = TReqInfo::Instance();
  //reqInfo->user.check_access(amPartialWrite);
  TQuery Qry(&OraSession);
  //��।����, ����� �� ३� ��� ॣ����樨

  point_dep=NodeAsInteger("point_dep",reqNode);
  airp_dep=NodeAsString("airp_dep",reqNode);
  //��稬 ३�
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,flt_no,airp,point_num, "
    "       DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
    "FROM points "
    "WHERE point_id=:point_id AND airp=:airp AND pr_reg<>0 AND pr_del=0 FOR UPDATE";
  Qry.CreateVariable("point_id",otInteger,point_dep);
  Qry.CreateVariable("airp",otString,airp_dep);
  Qry.Execute();
  if (Qry.Eof) throw UserException("���� �������. ������� �����");
  string airline=Qry.FieldAsString("airline");

  TTypeBSendInfo sendInfo;
  sendInfo.airline=Qry.FieldAsString("airline");
  sendInfo.flt_no=Qry.FieldAsInteger("flt_no");
  sendInfo.airp_dep=Qry.FieldAsString("airp");
  sendInfo.point_num=Qry.FieldAsInteger("point_num");
  sendInfo.first_point=Qry.FieldAsInteger("first_point");
  sendInfo.tlg_type="BSM";

  //BSM
  map<bool,string> BSMaddrs;
  TBSMContent BSMContentBefore;
  bool BSMsend=TelegramInterface::IsBSMSend(sendInfo,BSMaddrs);

  point_arv=NodeAsInteger("point_arv",reqNode);
  airp_arv=NodeAsString("airp_arv",reqNode);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airp FROM points "
    "WHERE point_id=:point_id AND airp=:airp AND pr_del=0";
  Qry.CreateVariable("point_id",otInteger,point_arv);
  Qry.CreateVariable("airp",otString,airp_arv);
  Qry.Execute();
  if (Qry.Eof) throw UserException("���� �������. ������� �����");

  //map ��� ���
  map<int,string> norms;

  //��।���� - ����� ॣ������ ��� ������ ���������
  xmlNodePtr node,node2,remNode;
  node = GetNode("grp_id",reqNode);
  if (node==NULL||NodeIsNULL(node))
  {
    cl=NodeAsString("class",reqNode);
    //����� ॣ������
    //�஢�ઠ ������ ᢮������ ����
    char grp_status=DecodeStatus(NodeAsString("status",reqNode));
    int free=CheckCounters(point_dep,point_arv,(char*)cl.c_str(),grp_status);
    string place_status;
    if (grp_status=='T')
      place_status="TR";
    else
      place_status="FP";

    hall=NodeAsInteger("hall",reqNode);
    Qry.Clear();
    Qry.SQLText="SELECT pr_vip FROM halls2 WHERE id=:hall";
    Qry.CreateVariable("hall",otInteger,hall);
    Qry.Execute();
    if (Qry.Eof) throw UserException("����୮ 㪠��� ��� ॣ����樨");
    bool addVIP=Qry.FieldAsInteger("pr_vip")!=0;

    //���⠭���� ६�ப VIP,EXST, �᫨ �㦭�
    //������ seats
    int seats,seats_sum=0;
    string rem_code;
    node=NodeAsNode("passengers",reqNode);
    for(node=node->children;node!=NULL;node=node->next)
    {
      node2=node->children;
      seats=NodeAsIntegerFast("seats",node2);
      seats_sum+=seats;
      bool flagVIP=false, flagSTCR=false, flagEXST=false;
      remNode=GetNodeFast("rems",node2);
      if (remNode!=NULL)
        for(remNode=remNode->children;remNode!=NULL;remNode=remNode->next)
        {
          node2=remNode->children;
          rem_code=NodeAsStringFast("rem_code",node2);
          if (rem_code=="VIP") flagVIP=true;
          if (rem_code=="STCR") flagSTCR=true;
          if (rem_code=="EXST") flagEXST=true;
        };
      if (addVIP && !flagVIP)
      {
        node2=node->children;
        if ((remNode=GetNodeFast("rems",node2))==NULL) remNode=NewTextChild(node,"rems");
        remNode=NewTextChild(remNode,"rem");
        NewTextChild(remNode,"rem_code","VIP");
        NewTextChild(remNode,"rem_text","VIP");
      };
      if (seats>1 && !flagEXST && !flagSTCR)
      {
        node2=node->children;
        if ((remNode=GetNodeFast("rems",node2))==NULL) remNode=NewTextChild(node,"rems");
        remNode=NewTextChild(remNode,"rem");
        NewTextChild(remNode,"rem_code","EXST");
        NewTextChild(remNode,"rem_text","EXST");
      };
      if (flagEXST && (seats<=1 || flagSTCR))
      {
        node2=node->children;
        remNode=GetNodeFast("rems",node2);
        if (remNode!=NULL)
        {
          remNode=remNode->children;
          while (remNode!=NULL)
          {
            node2=remNode->children;
            rem_code=NodeAsStringFast("rem_code",node2);
            if (rem_code=="EXST")
            {
              node2=remNode;
              remNode=remNode->next;
              xmlUnlinkNode(node2);
              xmlFreeNode(node2);
            }
            else
              remNode=remNode->next;
          };
        };
      };
    };
    if (free<seats_sum)
      throw UserException("����㯭�� ���� ��⠫��� %d",free);

    node=NodeAsNode("passengers",reqNode);
    Passengers.Clear();
    //�������� ���ᨢ ��� ��ᠤ��
    for(node=node->children;node!=NULL;node=node->next)
    {
        node2=node->children;
        if (NodeAsIntegerFast("seats",node2)==0) continue;
        const char *subclass=NodeAsStringFast("subclass",node2);
        TPassenger pas;
        pas.clname=cl;
        if (place_status=="FP"&&!NodeIsNULLFast("pax_id",node2)) {
          pas.placeStatus="BR";
          pas.pax_id = NodeAsIntegerFast( "pax_id", node2 );
        }
        else {
          pas.placeStatus=place_status;
          pas.pax_id = 0;
        }
        pas.placeName=NodeAsStringFast("seat_no",node2);
        pas.preseat=NodeAsStringFast("preseat_no",node2);
        pas.countPlace=NodeAsIntegerFast("seats",node2);
        pas.placeRem=NodeAsStringFast("seat_type",node2);
        remNode=GetNodeFast("rems",node2);
        bool flagMCLS=false;
        if (remNode!=NULL)
        {
          for(remNode=remNode->children;remNode!=NULL;remNode=remNode->next)
          {
            node2=remNode->children;
            const char *rem_code=NodeAsStringFast("rem_code",node2);
            if (airline=="��" && strcmp(rem_code,"MCLS")==0) flagMCLS=true;
            pas.rems.push_back(rem_code);
          };
        };
        ProgTrace(TRACE5,"airline=%s, subclass=%s, flagMCLS=%d",airline.c_str(),subclass,flagMCLS!=0);
        if (airline=="��" && strcmp(subclass,"�")==0 && !flagMCLS)
          pas.rems.push_back("MCLS");

        Passengers.Add(pas);
    };
    // ���⪠ ᠫ���
    TSalons Salons;
    Salons.trip_id = point_dep;
    Salons.ClName = cl;
    Salons.Read( rTripSalons );
    //��ᠤ��
    SEATS::SeatsPassengers( &Salons, GetUsePS()/*!!! ������ True - �������� ��ᠦ�� �� ���஭�஢���� ����, ����� */
    	                              /* ���� �ࠢ� �� ॣ������, ����� ३� ����砭��, ���� �ࠢ� ᠦ��� �� �㦨� ���஭. ���� */ );
    SEATS::SavePlaces( );
    //�������� ����� ���� ��᫥ ��ᠤ��
    node=NodeAsNode("passengers",reqNode);
    int i=0;
    for(node=node->children;node!=NULL&&i<Passengers.getCount();node=node->next)
    {
        node2=node->children;
        if (NodeAsIntegerFast("seats",node2)==0) continue;
        if (Passengers.Get(i).placeName=="") throw Exception("SeatsPassengers: empty placeName");
        string seat_no=NodeAsStringFast("seat_no",node2);
        if (seat_no!=""&&seat_no!=Passengers.Get(i).placeName)
          showErrorMessage("����� ����訢����� ���� ������㯭�. ���ᠦ��� ��ᠦ��� �� ᢮�����");
        ReplaceTextChild(node,"seat_no",Passengers.Get(i).placeName);
        i++;
    };
    //if (node!=NULL||i<Passengers.getCount()) throw Exception("SeatsPassengers: wrong count");
    //����稬 ॣ. ����� � �ਧ��� ᮢ���⭮� ॣ����樨 � ��ᠤ��
    Qry.Clear();
    Qry.SQLText=
      "SELECT NVL(MAX(reg_no)+1,1) AS reg_no FROM pax_grp,pax "
      "WHERE pax_grp.grp_id=pax.grp_id AND point_dep=:point_dep";
    Qry.CreateVariable("point_dep",otInteger,point_dep);
    Qry.Execute();
    int reg_no = Qry.FieldAsInteger("reg_no");
    bool pr_brd_with_reg=false,pr_exam_with_brd=false;
    Qry.Clear();
    Qry.SQLText=
      "SELECT pr_misc FROM trip_hall "
      "WHERE point_id=:point_id AND type=:type AND (hall=:hall OR hall IS NULL) "
      "ORDER BY DECODE(hall,NULL,1,0)";
    Qry.CreateVariable("point_id",otInteger,point_dep);
    Qry.CreateVariable("hall",otInteger,hall);
    Qry.DeclareVariable("type",otInteger);

    Qry.SetVariable("type",1);
    Qry.Execute();
    if (!Qry.Eof) pr_brd_with_reg=Qry.FieldAsInteger("pr_misc")!=0;
    Qry.SetVariable("type",2);
    Qry.Execute();
    if (!Qry.Eof) pr_exam_with_brd=Qry.FieldAsInteger("pr_misc")!=0;

    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  SELECT pax_grp__seq.nextval INTO :grp_id FROM dual; "
      "  INSERT INTO pax_grp(grp_id,point_dep,point_arv,airp_dep,airp_arv,class, "
      "                      status,excess,hall,pr_refuse,user_id,tid) "
      "  VALUES(:grp_id,:point_dep,:point_arv,:airp_dep,:airp_arv,:class, "
      "         :status,:excess,:hall,0,:user_id,tid__seq.nextval); "
      "END;";
    Qry.CreateVariable("grp_id",otInteger,FNull);
    Qry.CreateVariable("point_dep",otInteger,point_dep);
    Qry.CreateVariable("point_arv",otInteger,point_arv);
    Qry.CreateVariable("airp_dep",otString,airp_dep);
    Qry.CreateVariable("airp_arv",otString,airp_arv);
    Qry.CreateVariable("class",otString,cl);
    Qry.CreateVariable("status",otString,NodeAsString("status",reqNode));
    Qry.CreateVariable("excess",otInteger,NodeAsInteger("excess",reqNode));
    Qry.CreateVariable("hall",otInteger,hall);
    Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
    Qry.Execute();
    grp_id=Qry.GetVariableAsInteger("grp_id");
    ReplaceTextChild(reqNode,"grp_id",grp_id);
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  IF :pax_id IS NULL THEN "
      "    SELECT pax_id.nextval INTO :pax_id FROM dual; "
      "  END IF; "
      "  INSERT INTO pax(pax_id,grp_id,surname,name,pers_type,seat_no,seat_type,seats,pr_brd, "
      "                  refuse,reg_no,ticket_no,coupon_no,document,pr_exam,doc_check,subclass,tid) "
      "  VALUES(:pax_id,pax_grp__seq.currval,:surname,:name,:pers_type,:seat_no,:seat_type,:seats,:pr_brd, "
      "         NULL,:reg_no,:ticket_no,:coupon_no,:document,:pr_exam,0,:subclass,tid__seq.currval); "
      "END;";
    Qry.DeclareVariable("pax_id",otInteger);
    Qry.DeclareVariable("surname",otString);
    Qry.DeclareVariable("name",otString);
    Qry.DeclareVariable("pers_type",otString);
    Qry.DeclareVariable("seat_no",otString);
    Qry.DeclareVariable("seat_type",otString);
    Qry.DeclareVariable("seats",otInteger);
    Qry.DeclareVariable("pr_brd",otInteger);
    Qry.DeclareVariable("reg_no",otInteger);
    Qry.DeclareVariable("ticket_no",otString);
    Qry.DeclareVariable("coupon_no",otInteger);
    Qry.DeclareVariable("document",otString);
    Qry.DeclareVariable("subclass",otString);
    for(i=0;i<=1;i++)
    {
      node=NodeAsNode("passengers",reqNode);
      for(node=node->children;node!=NULL;node=node->next)
      {
        node2=node->children;
        seats=NodeAsIntegerFast("seats",node2);
        if (seats<=0&&i==0||seats>0&&i==1) continue;
        const char* surname=NodeAsStringFast("surname",node2);
        const char* name=NodeAsStringFast("name",node2);
        const char* pers_type=NodeAsStringFast("pers_type",node2);
        const char* seat_no=NodeAsStringFast("seat_no",node2);
        if (!NodeIsNULLFast("pax_id",node2))
          Qry.SetVariable("pax_id",NodeAsIntegerFast("pax_id",node2));
        else
          Qry.SetVariable("pax_id",FNull);
        Qry.SetVariable("surname",surname);
        Qry.SetVariable("name",name);
        Qry.SetVariable("pers_type",pers_type);
        if (seats>0)
        {
          Qry.SetVariable("seat_no",seat_no);
          Qry.SetVariable("seat_type",NodeAsStringFast("seat_type",node2));
        }
        else
        {
          Qry.SetVariable("seat_no",FNull);
          Qry.SetVariable("seat_type",FNull);
        };
        Qry.SetVariable("seats",seats);
        Qry.SetVariable("pr_brd",(int)pr_brd_with_reg);
        Qry.SetVariable("pr_exam",(int)(pr_brd_with_reg && pr_exam_with_brd));
        Qry.SetVariable("reg_no",reg_no);
        Qry.SetVariable("ticket_no",NodeAsStringFast("ticket_no",node2));
        if (!NodeIsNULLFast("coupon_no",node2))
          Qry.SetVariable("coupon_no",NodeAsIntegerFast("coupon_no",node2));
        else
          Qry.SetVariable("coupon_no",FNull);
        Qry.SetVariable("document",NodeAsStringFast("document",node2));
        Qry.SetVariable("subclass",NodeAsStringFast("subclass",node2));
        try
        {
          Qry.Execute();
        }
        catch(EOracleError E)
        {
          if (E.Code==1)
            throw UserException((string)"���ᠦ�� "+surname+(*name!=0?" ":"")+name+
                                        " 㦥 ��ॣ����஢�� � ��㣮� �⮩��");
          else
            throw;
        };
        ReplaceTextChild(node,"pax_id",Qry.GetVariableAsInteger("pax_id"));

        //������ ६�ப
        SavePaxRem(node);
        //������ ���
        string normStr=SavePaxNorms(node,norms);
        //������ ���ଠ樨 �� ���ᠦ��� � ���
        TLogMsg msg;
        msg.ev_type=ASTRA::evtPax;
        msg.id1=point_dep;
        msg.id2=reg_no;
        msg.id3=grp_id;
        msg.msg=(string)"���ᠦ�� "+surname+(*name!=0?" ":"")+name+" ("+pers_type+") ��ॣ����஢��"+
                ((pr_brd_with_reg && pr_exam_with_brd)?", ��襫 ��ᬮ��":"")+
                (pr_brd_with_reg?" , ��襫 ��ᠤ��":"")+
                ". "+
                "�/�: "+airp_arv+", �����: "+cl+", �����: "+grp_status+", ����: "+
                (seats>0?seat_no+(seats>1?"+"+IntToString(seats-1):""):"���")+
                msg.msg+=". ���.����: "+normStr;
        reqInfo->MsgToLog(msg);
        reg_no++;
      };
    };
    SaveTransfer(reqNode);
  }
  else
  {
    grp_id=NodeAsInteger(node);

    Qry.Clear();
    Qry.SQLText=
      "UPDATE pax_grp "
      "SET excess=:excess,tid=tid__seq.nextval "
      "WHERE grp_id=:grp_id AND tid=:tid";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.CreateVariable("tid",otInteger,NodeAsInteger("tid",reqNode));
    Qry.CreateVariable("excess",otInteger,NodeAsInteger("excess",reqNode));
    Qry.Execute();
    if (Qry.RowsProcessed()<=0)
      throw UserException("��������� � ��㯯� �ந��������� � ��㣮� �⮩��. ������� �����");

    //BSM
    if (BSMsend)
      TelegramInterface::LoadBSMContent(grp_id,BSMContentBefore);

    TQuery PaxQry(&OraSession);
    PaxQry.Clear();
    PaxQry.SQLText="UPDATE pax "
                   "SET surname=:surname, "
                   "    name=:name, "
                   "    pers_type=:pers_type, "
                   "    refuse=:refuse, "
                   "    ticket_no=:ticket_no, "
                   "    coupon_no=:coupon_no, "
                   "    document=:document, "
                   "    subclass=:subclass, "
                   "    pr_brd=DECODE(:refuse,NULL,pr_brd,NULL), "
                   "    pr_exam=DECODE(:refuse,NULL,pr_exam,0), "
                   "    tid=tid__seq.currval "
                   "WHERE pax_id=:pax_id AND tid=:tid";
    PaxQry.DeclareVariable("pax_id",otInteger);
    PaxQry.DeclareVariable("tid",otInteger);
    PaxQry.DeclareVariable("surname",otString);
    PaxQry.DeclareVariable("name",otString);
    PaxQry.DeclareVariable("pers_type",otString);
    PaxQry.DeclareVariable("refuse",otString);
    PaxQry.DeclareVariable("ticket_no",otString);
    PaxQry.DeclareVariable("coupon_no",otInteger);
    PaxQry.DeclareVariable("document",otString);
    PaxQry.DeclareVariable("subclass",otString);

    TQuery SalonQry(&OraSession);
    SalonQry.Clear();
    SalonQry.SQLText=
      "BEGIN "
      "  salons.seatpass( :point_id, :pax_id, :seat_no, 0,:agent_error ); "
      "  UPDATE pax SET seat_no=NULL,prev_seat_no=seat_no WHERE pax_id=:pax_id; "
      "END;";
    SalonQry.CreateVariable("point_id",otInteger,point_dep);
    SalonQry.DeclareVariable("pax_id",otInteger);
    SalonQry.DeclareVariable("agent_error",otInteger); //???
    SalonQry.DeclareVariable("seat_no",otString);
    node=GetNode("passengers",reqNode);
    if (node!=NULL)
    {
      for(node=node->children;node!=NULL;node=node->next)
      {
        node2=node->children;
        int pax_id=NodeAsIntegerFast("pax_id",node2);
        const char* surname=NodeAsStringFast("surname",node2);
        const char* name=NodeAsStringFast("name",node2);
        Qry.Clear();
        Qry.SQLText="SELECT refuse,reg_no,seat_no FROM pax WHERE pax_id=:pax_id";
        Qry.CreateVariable("pax_id",otInteger,pax_id);
        Qry.Execute();
        string old_refuse=Qry.FieldAsString("refuse");
        int reg_no=Qry.FieldAsInteger("reg_no");
        if (GetNodeFast("refuse",node2)!=NULL)
        {
          //�뫨 ��������� � ���ଠ樨 �� ���ᠦ���
          if (!NodeIsNULLFast("refuse",node2)&&!Qry.FieldIsNULL("seat_no"))
          {
            SalonQry.SetVariable("pax_id",pax_id);
            if ( !strcmp( NodeAsStringFast("refuse",node2), "�" ) ) //???
              SalonQry.SetVariable("agent_error", 1 );
            else
            	SalonQry.SetVariable("agent_error", 0 );
            SalonQry.SetVariable("seat_no",Qry.FieldAsString("seat_no"));
            SalonQry.Execute();
          };
          const char* pers_type=NodeAsStringFast("pers_type",node2);
          const char* refuse=NodeAsStringFast("refuse",node2);
          PaxQry.SetVariable("pax_id",pax_id);
          PaxQry.SetVariable("tid",NodeAsIntegerFast("tid",node2));
          PaxQry.SetVariable("surname",surname);
          PaxQry.SetVariable("name",name);
          PaxQry.SetVariable("pers_type",pers_type);
          PaxQry.SetVariable("refuse",refuse);
          PaxQry.SetVariable("ticket_no",NodeAsStringFast("ticket_no",node2));
          if (!NodeIsNULLFast("coupon_no",node2))
            PaxQry.SetVariable("coupon_no",NodeAsIntegerFast("coupon_no",node2));
          else
            PaxQry.SetVariable("coupon_no",FNull);
          PaxQry.SetVariable("document",NodeAsStringFast("document",node2));
          PaxQry.SetVariable("subclass",NodeAsStringFast("subclass",node2));
          PaxQry.Execute();
          if (PaxQry.RowsProcessed()<=0)
            throw UserException((string)"��������� �� ���ᠦ��� "+surname+(*name!=0?" ":"")+name+
                                        " �ந��������� � ��㣮� �⮩��. ������� �����");

          //������ ���ଠ樨 �� ���ᠦ��� � ���
          if (old_refuse!=refuse)
          {
            if (old_refuse=="")
              reqInfo->MsgToLog((string)"���ᠦ�� "+surname+(*name!=0?" ":"")+name+" ("+pers_type+") ࠧॣ����஢��. "+
                                "��稭� �⪠�� � ॣ����樨: "+refuse+". ",
                                ASTRA::evtPax,point_dep,reg_no,grp_id);
            else
              reqInfo->MsgToLog((string)"���ᠦ�� "+surname+(*name!=0?" ":"")+name+" ("+pers_type+"). "+
                                "�������� ��稭� �⪠�� � ॣ����樨: "+refuse+". ",
                                ASTRA::evtPax,point_dep,reg_no,grp_id);
          }
          else
          {
            //�஢���� �� PaxUpdatespending!!!
            reqInfo->MsgToLog((string)"���ᠦ�� "+surname+(*name!=0?" ":"")+name+" ("+pers_type+"). "+
                              "�������� ����� ���ᠦ��.",
                              ASTRA::evtPax,point_dep,reg_no,grp_id);
          };
        }
        else
        {
          Qry.Clear();
          Qry.SQLText="UPDATE pax SET tid=tid__seq.currval WHERE pax_id=:pax_id AND tid=:tid";
          Qry.CreateVariable("pax_id",otInteger,pax_id);
          Qry.CreateVariable("tid",otInteger,NodeAsIntegerFast("tid",node2));
          Qry.Execute();
          if (Qry.RowsProcessed()<=0)
            throw UserException((string)"��������� �� ���ᠦ��� "+surname+(*name!=0?" ":"")+name+
                                        " �ந��������� � ��㣮� �⮩��. ������� �����");
        };
        //������ ६�ப
        SavePaxRem(node);
        //������ ���
        string normStr=SavePaxNorms(node,norms);
        if (normStr!="")
          reqInfo->MsgToLog((string)"���ᠦ�� "+surname+(*name!=0?" ":"")+name+". "+
                            "���.����: "+normStr,ASTRA::evtPax,point_dep,reg_no,grp_id);
      };
    };


  };

  SaveBag(reqNode);
  SavePaidBag(reqNode);
  SaveBagToLog(reqNode);
  SaveTagPacks(reqNode);

  //�஢�ਬ �㡫�஢���� ����⮢
  Qry.Clear();
  Qry.SQLText=
    "SELECT COUNT(*),ticket_no,coupon_no FROM pax "
    "WHERE ticket_no IS NOT NULL AND coupon_no IS NOT NULL "
    "GROUP BY ticket_no,coupon_no HAVING COUNT(*)>1";
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    node=NodeAsNode("passengers",reqNode);
    for(node=node->children;node!=NULL;node=node->next)
    {
      node2=node->children;
      if (!NodeIsNULLFast("coupon_no",node2) &&
          strcmp(Qry.FieldAsString("ticket_no"),NodeAsStringFast("ticket_no",node2))==0)
        throw UserException("��. ����� �%s/%s �㡫������",
                            NodeAsStringFast("ticket_no",node2),
                            NodeAsStringFast("coupon_no",node2));
    };
  };
  //���������� counters
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  mvd.sync_pax_grp(:grp_id,:desk); "
    "  ckin.check_grp(:grp_id); "
    "  ckin.recount(:point_id); "
    "END;";
  Qry.CreateVariable("point_id",otInteger,point_dep);
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.CreateVariable("desk",otString,reqInfo->desk.code);
  Qry.Execute();
  Qry.Close();

  //����⠥� ������ ��㯯� �������ᮢ
  TQuery PaxQry(&OraSession);
  PaxQry.Clear();
  PaxQry.SQLText=
    "SELECT subclass,class FROM pax_grp,pax "
    "WHERE pax_grp.grp_id=pax.grp_id AND pax.grp_id=:grp_id "
    "ORDER BY reg_no";
  PaxQry.CreateVariable("grp_id",otInteger,grp_id);
  PaxQry.Execute();
  if (!PaxQry.Eof)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT MAX(DECODE(airline,NULL,0,4)+ "
      "           DECODE(airp,NULL,0,2)) AS priority "
      "FROM cls_grp "
      "WHERE (airline IS NULL OR airline=:airline) AND "
      "      (airp IS NULL OR airp=:airp) AND "
      "      pr_del=0";
    Qry.CreateVariable("airline",otString,airline);
    Qry.CreateVariable("airp",otString,airp_dep);
    Qry.Execute();
    if (Qry.Eof||Qry.FieldIsNULL("priority"))
    {
      ProgError(STDLOG,"Class group not found (airline=%s, airp=%s)",airline.c_str(),airp_dep.c_str());
      throw UserException("�� ����� ३� ॣ������ �� � ����� �� ����ᮢ �� �ந��������");

    };
    int priority=Qry.FieldAsInteger("priority");


    int class_grp=-1;
    for(;!PaxQry.Eof;PaxQry.Next())
    {
      if (!PaxQry.FieldIsNULL("subclass"))
      {
        Qry.Clear();
        Qry.SQLText=
          "SELECT cls_grp.id "
          "FROM cls_grp,subcls_grp "
          "WHERE cls_grp.id=subcls_grp.id AND "
          "      (airline IS NULL OR airline=:airline) AND "
          "      (airp IS NULL OR airp=:airp) AND "
          "      DECODE(airline,NULL,0,4)+ "
          "      DECODE(airp,NULL,0,2) = :priority AND "
          "      subcls_grp.subclass=:subclass AND "
          "      cls_grp.pr_del=0";
        Qry.CreateVariable("airline",otString,airline);
        Qry.CreateVariable("airp",otString,airp_dep);
        Qry.CreateVariable("priority",otInteger,priority);
        Qry.CreateVariable("subclass",otString,PaxQry.FieldAsString("subclass"));
        Qry.Execute();
        if (!Qry.Eof)
        {
          if (class_grp==-1) class_grp=Qry.FieldAsInteger("id");
          else
            if (class_grp!=Qry.FieldAsInteger("id"))
              throw UserException("���������� ��ॣ����஢��� ���ᠦ�஢ � 㪠����묨 �������ᠬ� ����� ��㯯��");
          Qry.Next();
          if (!Qry.Eof)
            throw Exception("More than one class group found (airline=%s, airp=%s, subclass=%s)",
                            airline.c_str(),airp_dep.c_str(),PaxQry.FieldAsString("subclass"));
          continue;
        };
      };

      Qry.Clear();
      Qry.SQLText=
        "SELECT cls_grp.id "
        "FROM cls_grp "
        "WHERE (airline IS NULL OR airline=:airline) AND "
        "      (airp IS NULL OR airp=:airp) AND "
        "      DECODE(airline,NULL,0,4)+ "
        "      DECODE(airp,NULL,0,2) = :priority AND "
        "      class=:class AND pr_del=0";
      Qry.CreateVariable("airline",otString,airline);
      Qry.CreateVariable("airp",otString,airp_dep);
      Qry.CreateVariable("priority",otInteger,priority);
      Qry.CreateVariable("class",otString,PaxQry.FieldAsString("class"));
      Qry.Execute();
      if (Qry.Eof)
      {
        if (!PaxQry.FieldIsNULL("subclass"))
          throw UserException("�� ����� ३� ॣ������ � �������� %s �� �ந��������",
                              PaxQry.FieldAsString("subclass"));
        else
          throw UserException("�� ����� ३� ॣ������ � ����� %s �� �ந��������",
                              PaxQry.FieldAsString("class"));
      };
      if (class_grp==-1) class_grp=Qry.FieldAsInteger("id");
      else
        if (class_grp!=Qry.FieldAsInteger("id"))
          throw UserException("���������� ��ॣ����஢��� ���ᠦ�஢ � 㪠����묨 �������ᠬ� ����� ��㯯��");
      Qry.Next();
      if (!Qry.Eof)
        throw Exception("More than one class group found (airline=%s, airp=%s, class=%s)",
                        airline.c_str(),airp_dep.c_str(),PaxQry.FieldAsString("class"));
    };
    Qry.Clear();
    Qry.SQLText="UPDATE pax_grp SET class_grp=:class_grp WHERE grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.CreateVariable("class_grp",otInteger,class_grp);
    Qry.Execute();
  };

  //BSM
  if (BSMsend) TelegramInterface::SendBSM(point_dep,grp_id,BSMContentBefore,BSMaddrs);

  //�������� ����� �� ��㯯� � ��ࠢ��� �� ������
  LoadPax(ctxt,reqNode,resNode);
  //��ࠢ��� �� ������ ���稪�
  readTripCounters(point_dep,resNode);
};

void CheckInInterface::LoadPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node,paxNode;
  int grp_id;
  TQuery Qry(&OraSession);
  TQuery PaxQry(&OraSession);
  node = GetNode("grp_id",reqNode);
  if (node==NULL||NodeIsNULL(node))
  {
    int point_id=NodeAsInteger("point_id",reqNode);
    int reg_no=NodeAsInteger("reg_no",reqNode);
    Qry.Clear();
    Qry.SQLText=
      "SELECT pax_grp.grp_id FROM pax_grp,pax "
      "WHERE pax_grp.grp_id=pax.grp_id AND "
      "point_dep=:point_id AND reg_no=:reg_no";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("reg_no",otInteger,reg_no);
    Qry.Execute();
    if (Qry.Eof) throw UserException(1,"�������樮��� ����� �� ������");
    grp_id=Qry.FieldAsInteger("grp_id");
    Qry.Next();
    if (!Qry.Eof) throw Exception("Duplicate reg_no (point_id=%d reg_no=%d)",point_id,reg_no);
  }
  else grp_id=NodeAsInteger(node);

  Qry.Clear();
  Qry.SQLText=
    "SELECT point_arv,airp_arv,airps.city AS city_arv, "
    "       class,status,hall,pax_grp.tid "
    "FROM pax_grp,airps "
    "WHERE pax_grp.airp_arv=airps.code AND grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (Qry.Eof) return; //�� �뢠�� ����� ࠧॣ������ �ᥩ ��㯯� �� �訡�� �����
  NewTextChild(resNode,"grp_id",grp_id);
  NewTextChild(resNode,"point_arv",Qry.FieldAsInteger("point_arv"));
  NewTextChild(resNode,"airp_arv",Qry.FieldAsString("airp_arv"));
  NewTextChild(resNode,"city_arv",Qry.FieldAsString("city_arv"));
  NewTextChild(resNode,"class",Qry.FieldAsString("class"));
  NewTextChild(resNode,"status",Qry.FieldAsString("status"));
  NewTextChild(resNode,"hall",Qry.FieldAsInteger("hall"));
  NewTextChild(resNode,"tid",Qry.FieldAsInteger("tid"));

  Qry.Clear();
  Qry.SQLText="SELECT pax_id FROM bp_print WHERE pax_id=:pax_id AND pr_print=1 AND rownum=1";
  Qry.DeclareVariable("pax_id",otInteger);
  PaxQry.Clear();
  PaxQry.SQLText=
    "SELECT pax_id,surname,name,pers_type,seat_no,seat_type, "
    "       seats,refuse,reg_no,ticket_no,coupon_no,document,subclass,tid "
    "FROM pax WHERE grp_id=:grp_id ORDER BY reg_no";
  PaxQry.CreateVariable("grp_id",otInteger,grp_id);
  PaxQry.Execute();
  node=NewTextChild(resNode,"passengers");
  for(;!PaxQry.Eof;PaxQry.Next())
  {
    paxNode=NewTextChild(node,"pax");
    int pax_id=PaxQry.FieldAsInteger("pax_id");
    NewTextChild(paxNode,"pax_id",pax_id);
    NewTextChild(paxNode,"surname",PaxQry.FieldAsString("surname"));
    NewTextChild(paxNode,"name",PaxQry.FieldAsString("name"));
    NewTextChild(paxNode,"pers_type",PaxQry.FieldAsString("pers_type"));
    NewTextChild(paxNode,"seat_no",PaxQry.FieldAsString("seat_no"));
    NewTextChild(paxNode,"seat_type",PaxQry.FieldAsString("seat_type"));
    NewTextChild(paxNode,"seats",PaxQry.FieldAsInteger("seats"));
    NewTextChild(paxNode,"refuse",PaxQry.FieldAsString("refuse"));
    NewTextChild(paxNode,"reg_no",PaxQry.FieldAsInteger("reg_no"));
    NewTextChild(paxNode,"ticket_no",PaxQry.FieldAsString("ticket_no"));
    if (!PaxQry.FieldIsNULL("coupon_no"))
      NewTextChild(paxNode,"coupon_no",PaxQry.FieldAsInteger("coupon_no"));
    else
      NewTextChild(paxNode,"coupon_no");
    NewTextChild(paxNode,"document",PaxQry.FieldAsString("document"));
    NewTextChild(paxNode,"subclass",PaxQry.FieldAsString("subclass"));
    NewTextChild(paxNode,"tid",PaxQry.FieldAsInteger("tid"));

    Qry.SetVariable("pax_id",pax_id);
    Qry.Execute();
    NewTextChild(paxNode,"pr_bp_print",(int)(!Qry.Eof));
    LoadPaxRem(paxNode);
    LoadPaxNorms(paxNode);
  };
  LoadTransfer(resNode);
  LoadBag(resNode);
  LoadPaidBag(resNode);

  Qry.Close();
  PaxQry.Close();
};



void CheckInInterface::SavePaxRem(xmlNodePtr paxNode)
{
  if (paxNode==NULL) return;
  xmlNodePtr node2=paxNode->children;
  int pax_id=NodeAsIntegerFast("pax_id",node2);

  xmlNodePtr remNode=GetNodeFast("rems",node2);
  if (remNode==NULL) return;

  TQuery RemQry(&OraSession);
  RemQry.Clear();
  RemQry.SQLText="DELETE FROM pax_rem WHERE pax_id=:pax_id";
  RemQry.CreateVariable("pax_id",otInteger,pax_id);
  RemQry.Execute();

  RemQry.SQLText=
    "INSERT INTO pax_rem(pax_id,rem,rem_code) VALUES (:pax_id,:rem,:rem_code)";
  RemQry.DeclareVariable("rem",otString);
  RemQry.DeclareVariable("rem_code",otString);
  for(remNode=remNode->children;remNode!=NULL;remNode=remNode->next)
  {
    node2=remNode->children;
    RemQry.SetVariable("rem",NodeAsStringFast("rem_text",node2));
    RemQry.SetVariable("rem_code",NodeAsStringFast("rem_code",node2));
    RemQry.Execute();
  };
  RemQry.Close();
};

void CheckInInterface::LoadPaxRem(xmlNodePtr paxNode)
{
  if (paxNode==NULL) return;
  xmlNodePtr node2=paxNode->children;
  int pax_id=NodeAsIntegerFast("pax_id",node2);

  xmlNodePtr node=NewTextChild(paxNode,"rems");
  TQuery RemQry(&OraSession);
  RemQry.Clear();
  RemQry.SQLText="SELECT rem_code,rem FROM pax_rem WHERE pax_id=:pax_id";
  RemQry.CreateVariable("pax_id",otInteger,pax_id);
  RemQry.Execute();
  for(;!RemQry.Eof;RemQry.Next())
  {
    xmlNodePtr remNode=NewTextChild(node,"rem");
    NewTextChild(remNode,"rem_text",RemQry.FieldAsString("rem"));
    NewTextChild(remNode,"rem_code",RemQry.FieldAsString("rem_code"));
  };
  RemQry.Close();
};

void CheckInInterface::LoadPaxNorms(xmlNodePtr paxNode)
{
  if (paxNode==NULL) return;
  xmlNodePtr node2=paxNode->children;
  int pax_id=NodeAsIntegerFast("pax_id",node2);

  xmlNodePtr node=NewTextChild(paxNode,"norms");
  TQuery NormQry(&OraSession);
  NormQry.Clear();
  NormQry.SQLText=
    "SELECT norm_id,pax_norms.bag_type,norm_type,amount,weight,per_unit "
    "FROM pax_norms,bag_norms "
    "WHERE pax_norms.norm_id=bag_norms.id(+) AND pax_norms.pax_id=:pax_id ";
  NormQry.CreateVariable("pax_id",otInteger,pax_id);
  NormQry.Execute();
  for(;!NormQry.Eof;NormQry.Next())
  {
    xmlNodePtr normNode=NewTextChild(node,"norm");
    if (!NormQry.FieldIsNULL("bag_type"))
      NewTextChild(normNode,"bag_type",NormQry.FieldAsInteger("bag_type"));
    else
      NewTextChild(normNode,"bag_type");
    if (!NormQry.FieldIsNULL("norm_id"))
      NewTextChild(normNode,"norm_id",NormQry.FieldAsInteger("norm_id"));
    else
      NewTextChild(normNode,"norm_id");
    NewTextChild(normNode,"norm_type",NormQry.FieldAsString("norm_type"));
    if (!NormQry.FieldIsNULL("amount"))
      NewTextChild(normNode,"amount",NormQry.FieldAsInteger("amount"));
    else
      NewTextChild(normNode,"amount");
    if (!NormQry.FieldIsNULL("weight"))
      NewTextChild(normNode,"weight",NormQry.FieldAsInteger("weight"));
    else
      NewTextChild(normNode,"weight");
    if (!NormQry.FieldIsNULL("per_unit"))
      NewTextChild(normNode,"per_unit",(int)(NormQry.FieldAsInteger("per_unit")!=0));
    else
      NewTextChild(normNode,"per_unit");
  };
  NormQry.Close();
};

string CheckInInterface::SavePaxNorms(xmlNodePtr paxNode, map<int,string> &norms)
{
  if (paxNode==NULL) return "";
  xmlNodePtr node2=paxNode->children;
  int pax_id=NodeAsIntegerFast("pax_id",node2);

  xmlNodePtr normNode=GetNodeFast("norms",node2);
  if (normNode==NULL) return "";

  TQuery NormQry(&OraSession);
  NormQry.Clear();
  NormQry.SQLText="DELETE FROM pax_norms WHERE pax_id=:pax_id";
  NormQry.CreateVariable("pax_id",otInteger,pax_id);
  NormQry.Execute();
  NormQry.SQLText=
    "INSERT INTO pax_norms(pax_id,bag_type,norm_id) VALUES(:pax_id,:bag_type,:norm_id)";
  NormQry.DeclareVariable("bag_type",otInteger);
  NormQry.DeclareVariable("norm_id",otInteger);
  string logStr;
  for(normNode=normNode->children;normNode!=NULL;normNode=normNode->next)
  {
    node2=normNode->children;
    //�饬 ���� � norms
    if (!NodeIsNULLFast("norm_id",node2) && norms.find(NodeAsIntegerFast("norm_id",node2))==norms.end())
    {
        int norm_id=NodeAsIntegerFast("norm_id",node2);
        TQuery Qry(&OraSession);
        Qry.Clear();
        Qry.SQLText="SELECT norm_type,amount,weight,per_unit FROM bag_norms WHERE id=:norm_id";
        Qry.CreateVariable("norm_id",otInteger,norm_id);
        Qry.Execute();
        if (Qry.Eof) throw Exception("Baggage norm not found (norm_id=%d)",norm_id);
        string normStr=lowerc(Qry.FieldAsString("norm_type"));
        if (!Qry.FieldIsNULL("weight"))
        {
          if (!Qry.FieldIsNULL("amount"))
            normStr=normStr+" "+Qry.FieldAsString("amount")+"�"+Qry.FieldAsString("weight");
          else
            normStr=normStr+" "+Qry.FieldAsString("weight");
          if (!Qry.FieldIsNULL("per_unit")&&Qry.FieldAsInteger("per_unit")>0)
            normStr+="��/�";
          else
            normStr+="��";
        }
        else
        {
          if (!Qry.FieldIsNULL("amount"))
            normStr=normStr+" "+Qry.FieldAsString("amount")+"�";
        };
        norms[norm_id]=normStr;
        ProgTrace(TRACE5,"Added baggage norm %s",normStr.c_str());
    };
    if (!NodeIsNULLFast("bag_type",node2))
      NormQry.SetVariable("bag_type",NodeAsIntegerFast("bag_type",node2));
    else
      NormQry.SetVariable("bag_type",FNull);
    if (!NodeIsNULLFast("norm_id",node2))
      NormQry.SetVariable("norm_id",NodeAsIntegerFast("norm_id",node2));
    else
      NormQry.SetVariable("norm_id",FNull);
    NormQry.Execute();
    //��⠢�� ����� ��� ����� � ��ୠ� ����権
    if (!NodeIsNULLFast("bag_type",node2))
    {
      if (!logStr.empty()) logStr+=", ";
      if (!NodeIsNULLFast("norm_id",node2))
        logStr=logStr+NodeAsStringFast("bag_type",node2)+": "+norms[NodeAsIntegerFast("norm_id",node2)];
      else
        logStr=logStr+NodeAsStringFast("bag_type",node2)+": ���";
    }
    else
    {
      if (!logStr.empty()) logStr=", "+logStr;
      if (!NodeIsNULLFast("norm_id",node2))
        logStr=norms[NodeAsIntegerFast("norm_id",node2)]+logStr;
      else
        logStr="���"+logStr;
    };
  };
  NormQry.Close();
  if (logStr=="") logStr="���";
  return logStr;
};

void CheckInInterface::SaveTransfer(xmlNodePtr grpNode)
{
  if (grpNode==NULL) return;
  xmlNodePtr node2=grpNode->children;
  int grp_id=NodeAsIntegerFast("grp_id",node2);

  xmlNodePtr trferNode=GetNodeFast("transfer",node2);
  if (trferNode==NULL) return;

  TQuery TrferQry(&OraSession);
  TrferQry.Clear();
  TrferQry.SQLText="DELETE FROM transfer WHERE grp_id=:grp_id";
  TrferQry.CreateVariable("grp_id",otInteger,grp_id);
  TrferQry.Execute();

  TrferQry.SQLText=
    "INSERT INTO transfer(grp_id,transfer_num,airline,flt_no,suffix,local_date,airp_arv,subclass) "
    "VALUES(:grp_id,:transfer_num,:airline,:flt_no,:suffix,:local_date,:airp_arv,:subclass)";
  TrferQry.DeclareVariable("transfer_num",otInteger);
  TrferQry.DeclareVariable("airline",otString);
  TrferQry.DeclareVariable("flt_no",otInteger);
  TrferQry.DeclareVariable("suffix",otString);
  TrferQry.DeclareVariable("local_date",otInteger);
  TrferQry.DeclareVariable("airp_arv",otString);
  TrferQry.DeclareVariable("subclass",otString);
  int i=1;
  for(trferNode=trferNode->children;trferNode!=NULL;trferNode=trferNode->next,i++)
  {
    node2=trferNode->children;
    TrferQry.SetVariable("transfer_num",i);
    TrferQry.SetVariable("airline",NodeAsStringFast("airline",node2));
    TrferQry.SetVariable("flt_no",NodeAsIntegerFast("flt_no",node2));
    TrferQry.SetVariable("suffix",NodeAsStringFast("suffix",node2));
    TrferQry.SetVariable("local_date",NodeAsIntegerFast("local_date",node2));
    TrferQry.SetVariable("airp_arv",NodeAsStringFast("airp_arv",node2));
    TrferQry.SetVariable("subclass",NodeAsStringFast("subclass",node2));
    TrferQry.Execute();
  };
  TrferQry.Close();
};

void CheckInInterface::LoadTransfer(xmlNodePtr grpNode)
{
  if (grpNode==NULL) return;
  xmlNodePtr node2=grpNode->children;
  int grp_id=NodeAsIntegerFast("grp_id",node2);

  xmlNodePtr node=NewTextChild(grpNode,"transfer");
  TQuery TrferQry(&OraSession);
  TrferQry.Clear();
  TrferQry.SQLText=
    "SELECT airline,flt_no,suffix,local_date,airp_arv,subclass FROM transfer "
    "WHERE grp_id=:grp_id AND transfer_num>0 ORDER BY transfer_num";
  TrferQry.CreateVariable("grp_id",otInteger,grp_id);
  TrferQry.Execute();
  for(;!TrferQry.Eof;TrferQry.Next())
  {
    xmlNodePtr trferNode=NewTextChild(node,"segment");
    NewTextChild(trferNode,"airline",TrferQry.FieldAsString("airline"));
    NewTextChild(trferNode,"flt_no",TrferQry.FieldAsInteger("flt_no"));
    NewTextChild(trferNode,"suffix",TrferQry.FieldAsString("suffix"));
    NewTextChild(trferNode,"local_date",TrferQry.FieldAsInteger("local_date"));
    NewTextChild(trferNode,"airp_arv",TrferQry.FieldAsString("airp_arv"));
    NewTextChild(trferNode,"subclass",TrferQry.FieldAsString("subclass"));
  };
  TrferQry.Close();
};

void CheckInInterface::SaveBag(xmlNodePtr grpNode)
{
  if (grpNode==NULL) return;
  xmlNodePtr node,node2;
  int point_id=NodeAsInteger("point_dep",grpNode);
  int grp_id=NodeAsInteger("grp_id",grpNode);

  xmlNodePtr valueBagNode=GetNode("value_bags",grpNode);
  xmlNodePtr bagNode=GetNode("bags",grpNode);
  xmlNodePtr tagNode=GetNode("tags",grpNode);
  if (bagNode==NULL&&tagNode==NULL) return;
  //�����⠥� ���-�� ������ � ���. ��ப
  int bagAmount=0,tagCount=0;
  if (bagNode!=NULL)
    for(node=bagNode->children;node!=NULL;node=node->next)
    {
      node2=node->children;
      if (NodeAsIntegerFast("pr_cabin",node2)==0) bagAmount+=NodeAsIntegerFast("amount",node2);
    };
  if (tagNode!=NULL)
    for(node=tagNode->children;node!=NULL;node=node->next,tagCount++);

  ProgTrace(TRACE5,"bagAmount=%d tagCount=%d",bagAmount,tagCount);
  bool pr_tag_print=NodeAsInteger("@pr_print",tagNode)!=0;
  TQuery Qry(&OraSession);
  if (bagAmount!=tagCount)
  {
    if (pr_tag_print && tagCount<bagAmount )
    {
      Qry.Clear();
      Qry.SQLText=
        "SELECT tag_type FROM trip_bt WHERE point_id=:point_id";
      Qry.CreateVariable("point_id",otInteger,point_id);
      Qry.Execute();
      if (Qry.Eof) throw UserException("�� ३� �� �����祭 ����� ���⠥��� �������� ��ન");
      string tag_type = Qry.FieldAsString("tag_type");
      //����稬 ����� ���⠥��� ��ப
      Qry.Clear();
      Qry.SQLText=
        "DECLARE "
        "  vairline airlines.code%TYPE; "
        "  vaircode airlines.aircode%TYPE; "
        "BEGIN "
        "  BEGIN "
        "    SELECT airline INTO vairline FROM points WHERE point_id=:point_id; "
        "    SELECT aircode INTO vaircode FROM airlines WHERE code=vairline; "
        "  EXCEPTION "
        "    WHEN OTHERS THEN vaircode:=NULL; "
        "  END; "
        "  ckin.get__tag_no(:desk,vaircode,:tag_count,:first_no,:last_no); "
        "END;";
      Qry.CreateVariable("point_id",otInteger,point_id);
      Qry.CreateVariable("desk",otString,TReqInfo::Instance()->desk.code);
      Qry.CreateVariable("tag_count",otInteger,bagAmount-tagCount);
      Qry.DeclareVariable("first_no",otInteger);
      Qry.DeclareVariable("last_no",otInteger);
      Qry.Execute();
      int first_no=Qry.GetVariableAsInteger("first_no");
      int last_no=Qry.GetVariableAsInteger("last_no");
      if (tagNode==NULL) tagNode=NewTextChild(grpNode,"tags");
      if ((first_no/1000)==(last_no/1000))
      {
        //���� � ��᫥���� ����� �� ������ ���������
        for(int i=first_no;i<=last_no;i++,tagCount++)
        {
          node=NewTextChild(tagNode,"tag");
          NewTextChild(node,"num",tagCount+1);
          NewTextChild(node,"tag_type",tag_type);
          NewTextChild(node,"no",i);
          NewTextChild(node,"color");
          NewTextChild(node,"bag_num");
          NewTextChild(node,"pr_print",(int)false);
        };
      }
      else
      {
        int j;
        j=(first_no/1000)*1000+999;
        for(int i=first_no;i<=j;i++,tagCount++)
        {
          node=NewTextChild(tagNode,"tag");
          NewTextChild(node,"num",tagCount+1);
          NewTextChild(node,"tag_type",tag_type);
          NewTextChild(node,"no",i);
          NewTextChild(node,"color");
          NewTextChild(node,"bag_num");
          NewTextChild(node,"pr_print",(int)false);
        }
        j=(last_no/1000)*1000;
        if ((j%1000000)==0) j++;
        for(int i=j;i<=last_no;i++,tagCount++)
        {
          node=NewTextChild(tagNode,"tag");
          NewTextChild(node,"num",tagCount+1);
          NewTextChild(node,"tag_type",tag_type);
          NewTextChild(node,"no",i);
          NewTextChild(node,"color");
          NewTextChild(node,"bag_num");
          NewTextChild(node,"pr_print",(int)false);
        };
      };
      xmlNodePtr bNode,tNode;
      int bag_num,bag_amount;

      //�஡㥬 �ਢ易�� � ������
      if (bagNode!=NULL && tagNode!=NULL)
      {
        tNode=tagNode->last;
        for(bNode=bagNode->last;bNode!=NULL;bNode=bNode->prev)
        {
          if (tNode==NULL) break;
          node2=bNode->children;
          bag_num=NodeAsIntegerFast("num",node2);
          bag_amount=NodeAsIntegerFast("amount",node2);
          if (NodeAsIntegerFast("pr_cabin",node2)!=0) continue;

          //�஢�ਬ �⮡� �� ��� ����� �� �뫮 �����祭� �� ����� ��ન
          for(node=tagNode->children;node!=NULL&&node!=tNode->next;node=node->next)
          {
            node2=node->children;
            if (!NodeIsNULLFast("bag_num",node2) &&
                bag_num==NodeAsIntegerFast("bag_num",node2)) break;
          };
          if (node!=NULL&&node!=tNode->next) break; //�멤��, �᫨ �� ⥪�騩 ����� ��뫠���� ��ઠ

          int k=0;
          for(;k<bag_amount;k++)
          {
            if (tNode==NULL) break;
            node2=tNode->children;
            if (/*NodeAsIntegerFast("printable",node2)==0 ||*/ !NodeIsNULLFast("bag_num",node2)) break;
            ReplaceTextChild(tNode,"bag_num",bag_num);
            tNode=tNode->prev;
          };
          if (k<bag_amount) break; //�멤��, �᫨ ⥪��� ��ઠ ��뫠���� �� ����� ��� ��� �� ���⠥���
        };
      };
    }
    else throw UserException(1,"���-�� ��ப � ���� ������ �� ᮢ������");
  };

  TQuery BagQry(&OraSession);
  if (valueBagNode!=NULL)
  {
    BagQry.Clear();
    BagQry.SQLText="DELETE FROM value_bag WHERE grp_id=:grp_id";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    BagQry.SQLText=
      "INSERT INTO value_bag(grp_id,num,value,value_cur,tax_id,tax) "
      "VALUES(:grp_id,:num,:value,:value_cur,:tax_id,:tax)";
    BagQry.DeclareVariable("num",otInteger);
    BagQry.DeclareVariable("value",otFloat);
    BagQry.DeclareVariable("value_cur",otString);
    BagQry.DeclareVariable("tax_id",otInteger);
    BagQry.DeclareVariable("tax",otFloat);
    for(node=valueBagNode->children;node!=NULL;node=node->next)
    {
      node2=node->children;
      BagQry.SetVariable("num",NodeAsIntegerFast("num",node2));
      BagQry.SetVariable("value",NodeAsFloatFast("value",node2));
      BagQry.SetVariable("value_cur",NodeAsStringFast("value_cur",node2));
      if (!NodeIsNULLFast("tax_id",node2))
      {
        BagQry.SetVariable("tax_id",NodeAsIntegerFast("tax_id",node2));
        BagQry.SetVariable("tax",NodeAsFloatFast("tax",node2));
      }
      else
      {
        BagQry.SetVariable("tax_id",FNull);
        BagQry.SetVariable("tax",FNull);
      };
      BagQry.Execute();
    };
  };
  if (bagNode!=NULL)
  {
    BagQry.Clear();
    BagQry.SQLText="DELETE FROM bag2 WHERE grp_id=:grp_id";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    BagQry.SQLText=
      "INSERT INTO bag2 (grp_id,num,bag_type,pr_cabin,amount,weight,value_bag_num) "
      "VALUES (:grp_id,:num,:bag_type,:pr_cabin,:amount,:weight,:value_bag_num)";
    BagQry.DeclareVariable("num",otInteger);
    BagQry.DeclareVariable("bag_type",otInteger);
    BagQry.DeclareVariable("pr_cabin",otInteger);
    BagQry.DeclareVariable("amount",otInteger);
    BagQry.DeclareVariable("weight",otInteger);
    BagQry.DeclareVariable("value_bag_num",otInteger);
    for(node=bagNode->children;node!=NULL;node=node->next)
    {
      node2=node->children;
      BagQry.SetVariable("num",NodeAsIntegerFast("num",node2));
      if (!NodeIsNULLFast("bag_type",node2))
        BagQry.SetVariable("bag_type",NodeAsIntegerFast("bag_type",node2));
      else
        BagQry.SetVariable("bag_type",FNull);
      BagQry.SetVariable("pr_cabin",NodeAsIntegerFast("pr_cabin",node2));
      BagQry.SetVariable("amount",NodeAsIntegerFast("amount",node2));
      BagQry.SetVariable("weight",NodeAsIntegerFast("weight",node2));
      if (!NodeIsNULLFast("value_bag_num",node2))
        BagQry.SetVariable("value_bag_num",NodeAsIntegerFast("value_bag_num",node2));
      else
        BagQry.SetVariable("value_bag_num",FNull);
      BagQry.Execute();
    };
  };
  if (tagNode!=NULL)
  {
    BagQry.Clear();
    BagQry.SQLText="DELETE FROM bag_tags WHERE grp_id=:grp_id";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    BagQry.SQLText=
      "INSERT INTO bag_tags(grp_id,num,tag_type,no,color,bag_num,pr_print) "
      "VALUES (:grp_id,:num,:tag_type,:no,:color,:bag_num,:pr_print)";
    BagQry.DeclareVariable("num",otInteger);
    BagQry.DeclareVariable("tag_type",otString);
    BagQry.DeclareVariable("no",otFloat);
    BagQry.DeclareVariable("color",otString);
    BagQry.DeclareVariable("bag_num",otInteger);
    BagQry.DeclareVariable("pr_print",otInteger);
    for(node=tagNode->children;node!=NULL;node=node->next)
    {
      node2=node->children;
      const char* tag_type = NodeAsStringFast("tag_type",node2);
      const char* color = NodeAsStringFast("color",node2);
      double no = NodeAsFloatFast("no",node2);
      BagQry.SetVariable("num",NodeAsIntegerFast("num",node2));
      BagQry.SetVariable("tag_type",tag_type);
      BagQry.SetVariable("no",no);
      BagQry.SetVariable("color",color);
      if (!NodeIsNULLFast("bag_num",node2))
        BagQry.SetVariable("bag_num",NodeAsIntegerFast("bag_num",node2));
      else
        BagQry.SetVariable("bag_num",FNull);
      BagQry.SetVariable("pr_print",NodeAsIntegerFast("pr_print",node2));
      try
      {
        BagQry.Execute();
      }
      catch(EOracleError E)
      {
        if (E.Code==1)
          throw UserException("��ઠ %s %s%010.f 㦥 ��ॣ����஢���.",tag_type,color,no);
        else
          throw;
      };
    };
  };
  BagQry.Close();
  Qry.Close();
};

void CheckInInterface::LoadBag(xmlNodePtr grpNode)
{
  if (grpNode==NULL) return;
  xmlNodePtr node2=grpNode->children;
  int grp_id=NodeAsIntegerFast("grp_id",node2);

  TQuery BagQry(&OraSession);

  xmlNodePtr node=NewTextChild(grpNode,"value_bags");
  BagQry.Clear();
  BagQry.SQLText="SELECT num,value,value_cur,tax_id,tax FROM value_bag "
                 "WHERE grp_id=:grp_id ORDER BY num";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
  {
    xmlNodePtr valueBagNode=NewTextChild(node,"value_bag");
    NewTextChild(valueBagNode,"num",BagQry.FieldAsInteger("num"));
    NewTextChild(valueBagNode,"value",BagQry.FieldAsFloat("value"));
    NewTextChild(valueBagNode,"value_cur",BagQry.FieldAsString("value_cur"));
    if (!BagQry.FieldIsNULL("tax_id"))
    {
      NewTextChild(valueBagNode,"tax_id",BagQry.FieldAsInteger("tax_id"));
      NewTextChild(valueBagNode,"tax",BagQry.FieldAsFloat("tax"));
    }
    else
    {
      NewTextChild(valueBagNode,"tax_id");
      NewTextChild(valueBagNode,"tax");
    };
  };
  node=NewTextChild(grpNode,"bags");
  BagQry.Clear();
  BagQry.SQLText="SELECT num,bag_type,pr_cabin,amount,weight,value_bag_num "
                 "FROM bag2 WHERE grp_id=:grp_id ORDER BY num";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
  {
    xmlNodePtr bagNode=NewTextChild(node,"bag");
    NewTextChild(bagNode,"num",BagQry.FieldAsInteger("num"));
    if (!BagQry.FieldIsNULL("bag_type"))
      NewTextChild(bagNode,"bag_type",BagQry.FieldAsInteger("bag_type"));
    else
      NewTextChild(bagNode,"bag_type");
    NewTextChild(bagNode,"pr_cabin",(int)(BagQry.FieldAsInteger("pr_cabin")!=0));
    NewTextChild(bagNode,"amount",BagQry.FieldAsInteger("amount"));
    NewTextChild(bagNode,"weight",BagQry.FieldAsInteger("weight"));
    if (!BagQry.FieldIsNULL("value_bag_num"))
      NewTextChild(bagNode,"value_bag_num",BagQry.FieldAsInteger("value_bag_num"));
    else
      NewTextChild(bagNode,"value_bag_num");
  };
  node=NewTextChild(grpNode,"tags");
  BagQry.Clear();
  BagQry.SQLText=
    "SELECT num,tag_type,no_len,no,color,bag_num,printable,pr_print "
    "FROM bag_tags,tag_types "
    "WHERE bag_tags.tag_type=tag_types.code AND grp_id=:grp_id "
    "ORDER BY num";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
  {
    xmlNodePtr tagNode=NewTextChild(node,"tag");
    NewTextChild(tagNode,"num",BagQry.FieldAsInteger("num"));
    NewTextChild(tagNode,"tag_type",BagQry.FieldAsString("tag_type"));
    NewTextChild(tagNode,"no_len",BagQry.FieldAsInteger("no_len"));
    NewTextChild(tagNode,"no",BagQry.FieldAsFloat("no"));
    NewTextChild(tagNode,"color",BagQry.FieldAsString("color"));
    if (!BagQry.FieldIsNULL("bag_num"))
      NewTextChild(tagNode,"bag_num",BagQry.FieldAsInteger("bag_num"));
    else
      NewTextChild(tagNode,"bag_num");
    NewTextChild(tagNode,"printable",(int)(BagQry.FieldAsInteger("printable")!=0));
    NewTextChild(tagNode,"pr_print",(int)(BagQry.FieldAsInteger("pr_print")!=0));
  };
  BagQry.Close();
};

void CheckInInterface::SavePaidBag(xmlNodePtr grpNode)
{
  if (grpNode==NULL) return;
  xmlNodePtr node,node2;
  int grp_id=NodeAsInteger("grp_id",grpNode);
  xmlNodePtr paidBagNode=GetNode("paid_bags",grpNode);
  if (paidBagNode!=NULL)
  {
    TQuery BagQry(&OraSession);
    BagQry.Clear();
    BagQry.SQLText="DELETE FROM paid_bag WHERE grp_id=:grp_id";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    BagQry.SQLText=
      "INSERT INTO paid_bag(grp_id,bag_type,weight,rate_id) "
      "VALUES(:grp_id,:bag_type,:weight,:rate_id)";
    BagQry.DeclareVariable("bag_type",otInteger);
    BagQry.DeclareVariable("weight",otInteger);
    BagQry.DeclareVariable("rate_id",otInteger);
    for(node=paidBagNode->children;node!=NULL;node=node->next)
    {
      node2=node->children;
      if (!NodeIsNULLFast("bag_type",node2))
        BagQry.SetVariable("bag_type",NodeAsIntegerFast("bag_type",node2));
      else
        BagQry.SetVariable("bag_type",FNull);
      BagQry.SetVariable("weight",NodeAsIntegerFast("weight",node2));
      if (!NodeIsNULLFast("rate_id",node2))
        BagQry.SetVariable("rate_id",NodeAsIntegerFast("rate_id",node2));
      else
        BagQry.SetVariable("rate_id",FNull);
      BagQry.Execute();
    };
    BagQry.Close();
  };
};

void CheckInInterface::LoadPaidBag(xmlNodePtr grpNode)
{
  if (grpNode==NULL) return;
  xmlNodePtr node2=grpNode->children;
  int grp_id=NodeAsIntegerFast("grp_id",node2);

  xmlNodePtr node=NewTextChild(grpNode,"paid_bags");
  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText=
    "SELECT NVL(paid_bag.bag_type,-1) AS bag_type,paid_bag.weight, "
    "       NVL(rate_id,-1) AS rate_id,rate,rate_cur "
    "FROM paid_bag,bag_rates "
    "WHERE paid_bag.rate_id=bag_rates.id(+) AND grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
  {
    xmlNodePtr paidBagNode=NewTextChild(node,"paid_bag");
    if (!BagQry.FieldIsNULL("bag_type"))
      NewTextChild(paidBagNode,"bag_type",BagQry.FieldAsInteger("bag_type"));
    else
      NewTextChild(paidBagNode,"bag_type");
    NewTextChild(paidBagNode,"weight",BagQry.FieldAsInteger("weight"));
    if (!BagQry.FieldIsNULL("rate_id"))
    {
      NewTextChild(paidBagNode,"rate_id",BagQry.FieldAsInteger("rate_id"));
      NewTextChild(paidBagNode,"rate",BagQry.FieldAsFloat("rate"));
      NewTextChild(paidBagNode,"rate_cur",BagQry.FieldAsString("rate_cur"));
    }
    else
    {
      NewTextChild(paidBagNode,"rate_id");
      NewTextChild(paidBagNode,"rate");
      NewTextChild(paidBagNode,"rate_cur");
    };
  };
  BagQry.Close();
};

//������ ������ � ���
void CheckInInterface::SaveBagToLog(xmlNodePtr grpNode)
{
  if (grpNode==NULL) return;
  int point_id=NodeAsInteger("point_dep",grpNode);
  int grp_id=NodeAsInteger("grp_id",grpNode);
  xmlNodePtr paidBagNode=GetNode("paid_bags",grpNode);
  xmlNodePtr bagNode=GetNode("bags",grpNode);
  xmlNodePtr tagNode=GetNode("tags",grpNode);
  TReqInfo* reqInfo = TReqInfo::Instance();
  TLogMsg msg;
  msg.ev_type=ASTRA::evtPax;
  msg.id1=point_id;
  msg.id2=0;
  msg.id3=grp_id;
  TQuery Qry(&OraSession);
  if (bagNode!=NULL || tagNode!=NULL)
  {
    //��ப� �� ��饬� ���-�� ������
    Qry.Clear();
    Qry.SQLText=
      "SELECT "
      "       NVL(ckin.get_bagAmount(grp_id,NULL),0) AS bagAmount, "
      "       NVL(ckin.get_bagWeight(grp_id,NULL),0) AS bagWeight, "
      "       NVL(ckin.get_rkAmount(grp_id,NULL),0) AS rkAmount, "
      "       NVL(ckin.get_rkWeight(grp_id,NULL),0) AS rkWeight, "
      "       ckin.get_birks(grp_id,NULL) AS tags, "
      "       excess "
      "FROM pax_grp where grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    if (!Qry.Eof)
    {
      ostringstream msgh;
      msgh << "�����: " << Qry.FieldAsInteger("bagAmount") << "/" << Qry.FieldAsInteger("bagWeight") << ", "
           << "�/�����: " << Qry.FieldAsInteger("rkAmount") << "/" << Qry.FieldAsInteger("rkWeight") << ". ";
      if (Qry.FieldAsInteger("excess")!=0)
        msgh << "���. ���: " << Qry.FieldAsInteger("excess") << " ��. ";
      if (!Qry.FieldIsNULL("tags"))
        msgh << "��ન: " << Qry.FieldAsString("tags") << ". ";
      msg.msg=msgh.str();
      reqInfo->MsgToLog(msg);
    };
  };
  if (bagNode!=NULL || paidBagNode!=NULL)
  {
    //��ப� �� ⨯�� ������ � ����稢������ ������
    Qry.Clear();
    Qry.SQLText=
      "SELECT LPAD(paid_bag.bag_type,2,'0' ) AS bag_type, "
      "       MAX(paid_bag.weight) AS paid_weight, "
      "       NVL(SUM(bag2.amount),0) AS bag_amount, "
      "       NVL(SUM(bag2.weight),0) AS bag_weight "
      "FROM paid_bag,bag2 "
      "WHERE paid_bag.grp_id=bag2.grp_id(+) AND  "
      "      NVL(paid_bag.bag_type,-1)=NVL(bag2.bag_type(+),-1) AND  "
      "      paid_bag.grp_id=:grp_id "
      "GROUP BY paid_bag.bag_type "
      "ORDER BY DECODE(paid_bag.bag_type,NULL,0,1),paid_bag.bag_type ";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    ostringstream msgh1,msgh2;
    for(;!Qry.Eof;Qry.Next())
    {
      if (Qry.FieldAsInteger("bag_amount")==0 &&
          Qry.FieldAsInteger("bag_weight")==0 &&
          Qry.FieldAsInteger("paid_weight")==0) continue;
      if (Qry.FieldIsNULL("bag_type"))
      {
        msgh1 << ", "
              << Qry.FieldAsInteger("bag_amount") << "/"
              << Qry.FieldAsInteger("bag_weight") << "/"
              << Qry.FieldAsInteger("paid_weight");
      }
      else
      {
        msgh2 << ", "
              << Qry.FieldAsInteger("bag_type") << ": "
              << Qry.FieldAsInteger("bag_amount") << "/"
              << Qry.FieldAsInteger("bag_weight") << "/"
              << Qry.FieldAsInteger("paid_weight");
      };
    };
    if (!msgh2.str().empty())
    {
      msgh1 << msgh2.str();
      msg.msg="����� �� ⨯�� (����/���/���): "+msgh1.str().substr(2);
      reqInfo->MsgToLog(msg);

    };
  };
  Qry.Close();
};

void CheckInInterface::SaveTagPacks(xmlNodePtr node)
{
  if (node==NULL) return;
  node=GetNode("tag_packs",node);
  if (node==NULL) return;
  TReqInfo* reqInfo = TReqInfo::Instance();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  IF :no IS NULL THEN "
    "    DELETE FROM tag_packs "
    "    WHERE desk=:desk AND airline=:airline AND target=:target; "
    "  ELSE "
    "    UPDATE tag_packs "
    "    SET tag_type=:tag_type, color=:color, no=:no "
    "    WHERE desk=:desk AND airline=:airline AND target=:target; "
    "    IF SQL%NOTFOUND THEN "
    "      INSERT INTO tag_packs(desk,airline,target,tag_type,color,no) "
    "      VALUES (:desk,:airline,:target,:tag_type,:color,:no); "
    "    END IF; "
    "  END IF;   "
    "END; ";
  Qry.CreateVariable("desk",otString,reqInfo->desk.code);
  Qry.DeclareVariable("airline",otString);
  Qry.DeclareVariable("target",otString);
  Qry.DeclareVariable("tag_type",otString);
  Qry.DeclareVariable("color",otString);
  Qry.DeclareVariable("no",otFloat);
  for(node=node->children;node!=NULL;node=node->next)
  {
    xmlNodePtr node2=node->children;
    Qry.SetVariable("airline",NodeAsStringFast("airline",node2));
    Qry.SetVariable("target",NodeAsStringFast("target",node2));
    Qry.SetVariable("tag_type",NodeAsStringFast("tag_type",node2));
    Qry.SetVariable("color",NodeAsStringFast("color",node2));
    if (!NodeIsNULLFast("no",node2))
      Qry.SetVariable("no",NodeAsFloatFast("no",node2));
    else
      Qry.SetVariable("no",FNull);
    Qry.Execute();
  };
  Qry.Close();
};

void CheckInInterface::TestDateTime(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
 // TReqInfo* reqInfo = TReqInfo::Instance();
  NewTextChild(resNode,"LocalDateTime");
  NewTextChild(resNode,"UTCDateTime");
  if (!NodeIsNULL("UTCDateTime",reqNode))
  {
    try
    {
      TDateTime ud = NodeAsDateTime("UTCDateTime",reqNode);
      ud = NowLocal();
      //ud = UTCToLocal(ud,reqInfo->desk.tz_region);
      ReplaceTextChild(resNode,"LocalDateTime",DateTimeToStr(ud,"dd.mm.yyyy hh:nn:ss"));
    }
    catch(std::logic_error e)
    {
      showErrorMessage(e.what());
    };
  };

  if (!NodeIsNULL("LocalDateTime",reqNode))
  {
    try
    {
      TDateTime ld = NodeAsDateTime("LocalDateTime",reqNode);
      ld = NowUTC();
      //ld = LocalToUTC(ld,reqInfo->desk.tz_region);
      ReplaceTextChild(resNode,"UTCDateTime",DateTimeToStr(ld,"dd.mm.yyyy hh:nn:ss"));
    }
    catch(std::logic_error e)
    {
      showErrorMessage(e.what());
    };
  };
};


void CheckInInterface::readTripCounters( int point_id, xmlNodePtr dataNode )
{
  xmlNodePtr node = NewTextChild( dataNode, "tripcounters" );
  xmlNodePtr itemNode;

  TQuery Qry( &OraSession );
  Qry.SQLText =
     "SELECT point_arv, class, "
     "       crs_ok-ok AS noshow, "
     "       crs_tranzit-tranzit AS trnoshow, "
     "       tranzit+ok+goshow AS show, "
     "       free_ok, "
     "       free_goshow, "
     "       nooccupy "
     "FROM counters2 "
     "WHERE point_dep=:point_id";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();

  for(;!Qry.Eof;Qry.Next())
  {
    itemNode = NewTextChild( node, "item" );
    NewTextChild( itemNode, "point_arv", Qry.FieldAsInteger( "point_arv" ) );
    NewTextChild( itemNode, "class", Qry.FieldAsString( "class" ) );
    NewTextChild( itemNode, "noshow", Qry.FieldAsInteger( "noshow" ) );
    NewTextChild( itemNode, "trnoshow", Qry.FieldAsInteger( "trnoshow" ) );
    NewTextChild( itemNode, "show", Qry.FieldAsInteger( "show" ) );
    NewTextChild( itemNode, "free_ok", Qry.FieldAsInteger( "free_ok" ) );
    NewTextChild( itemNode, "free_goshow", Qry.FieldAsInteger( "free_goshow" ) );
    NewTextChild( itemNode, "nooccupy", Qry.FieldAsInteger( "nooccupy" ) );
  };
}

void CheckInInterface::readTripData( int point_id, xmlNodePtr dataNode )
{
  xmlNodePtr tripdataNode = NewTextChild( dataNode, "tripdata" );
  xmlNodePtr itemNode,node;

  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT airp, point_num, DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
    "FROM points WHERE point_id=:point_id";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("���� �� ������. ������� �����");
  int first_point=Qry.FieldAsInteger("first_point");
  int point_num=Qry.FieldAsInteger("point_num");
  string airp_dep=Qry.FieldAsString("airp");

  Qry.Clear();
  Qry.SQLText =
    "SELECT points.point_id, "
    "       airps.code AS airp_code, "
    "       airps.name AS airp_name, "
    "       cities.code AS city_code, "
    "       cities.name AS city_name "
    "FROM points,airps,cities "
    "WHERE points.first_point=:first_point AND points.point_num>:point_num AND points.pr_del=0 AND "
    "      points.airp=airps.code AND airps.city=cities.code "
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
      if (*i==Qry.FieldAsString( "airp_code" )) break;
    if (i!=airps.end()) continue;

    itemNode = NewTextChild( node, "airp" );
    NewTextChild( itemNode, "point_id", Qry.FieldAsInteger( "point_id" ) );
    NewTextChild( itemNode, "airp_code", Qry.FieldAsString( "airp_code" ) );
    NewTextChild( itemNode, "airp_name", Qry.FieldAsString( "airp_name" ) );
    NewTextChild( itemNode, "city_code", Qry.FieldAsString( "city_code" ) );
    NewTextChild( itemNode, "city_name", Qry.FieldAsString( "city_name" ) );
    airps.push_back(Qry.FieldAsString( "airp_code" ));
  };

  Qry.Clear();
  Qry.SQLText =
    "SELECT class AS class_code, "
    "       name AS class_name, "
    "       cfg "
    "FROM trip_classes,classes "
    "WHERE classes.code=trip_classes.class AND point_id= :point_id "
    "ORDER BY priority";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  node = NewTextChild( tripdataNode, "classes" );
  for(;!Qry.Eof;Qry.Next())
  {
    itemNode = NewTextChild( node, "class" );
    NewTextChild( itemNode, "code", Qry.FieldAsString( "class_code" ) );
    NewTextChild( itemNode, "name", Qry.FieldAsString( "class_name" ) );
    NewTextChild( itemNode, "cfg", Qry.FieldAsInteger( "cfg" ) );
  };

  Qry.Clear();
  Qry.SQLText =
    "SELECT name AS gate_name "
    "FROM stations,trip_stations "
    "WHERE stations.desk=trip_stations.desk AND "
    "      stations.work_mode=trip_stations.work_mode AND "
    "      trip_stations.point_id=:point_id AND "
    "      trip_stations.work_mode='�' ";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  node = NewTextChild( tripdataNode, "gates" );
  for(;!Qry.Eof;Qry.Next())
  {
    NewTextChild( node, "gate_name", Qry.FieldAsString( "gate_name" ) );
  };

  Qry.Clear();
  Qry.SQLText =
    "SELECT id,name FROM halls2 WHERE airp=:airp_dep";
  Qry.CreateVariable("airp_dep",otString,airp_dep);
  Qry.Execute();
  node = NewTextChild( tripdataNode, "halls" );
  for(;!Qry.Eof;Qry.Next())
  {
    itemNode = NewTextChild( node, "hall" );
    NewTextChild( itemNode, "id", Qry.FieldAsInteger( "id" ) );
    NewTextChild( itemNode, "name", Qry.FieldAsString( "name" ) );
  };
}

void CheckInInterface::GetTripCounters(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  readTripCounters(point_id,resNode);
};



