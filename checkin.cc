#include "checkin.h"
#include "exceptions.h"
#include "oralib.h"
#include "stl_utils.h"
#include "xml_unit.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "seats.h"
#include "stages.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"

using namespace std;
using namespace EXCEPTIONS;

void CheckInInterface::PaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  TQuery Qry(&OraSession);
  Qry.Clear(); 
  Qry.SQLText= 
    "SELECT "
    "  reg_no,surname,name,target,last_trfer,class,hall, "
    "  LPAD(seat_no,3,'0')||DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') AS seat_no, "
    "  seats,pers_type,document, "
    "  ckin.get_bagAmount(pax.grp_id,pax.reg_no,rownum) AS bag_amount, "
    "  ckin.get_bagWeight(pax.grp_id,pax.reg_no,rownum) AS bag_weight, "
    "  ckin.get_rkWeight(pax.grp_id,pax.reg_no,rownum) AS rk_weight, "
    "  ckin.get_excess(pax.grp_id,pax.reg_no) AS excess, "
    "  ckin.get_birks(pax.grp_id,pax.reg_no) AS tags, "
    "  pax.grp_id "
    "FROM pax_grp,pax,v_last_trfer "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax_grp.grp_id=v_last_trfer.grp_id(+) AND "
    "      point_id=:point_id AND pr_brd IS NOT NULL "
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
    NewTextChild(paxNode,"target",Qry.FieldAsString("target"));    
    NewTextChild(paxNode,"last_trfer",Qry.FieldAsString("last_trfer"));    
    NewTextChild(paxNode,"class",Qry.FieldAsString("class"));    
    NewTextChild(paxNode,"hall",Qry.FieldAsInteger("hall"));    
    NewTextChild(paxNode,"seat_no",Qry.FieldAsString("seat_no"));    
    NewTextChild(paxNode,"pers_type",Qry.FieldAsString("pers_type"));    
    NewTextChild(paxNode,"document",Qry.FieldAsString("document"));    
    NewTextChild(paxNode,"bag_amount",Qry.FieldAsInteger("bag_amount"));    
    NewTextChild(paxNode,"bag_weight",Qry.FieldAsInteger("bag_weight"));    
    NewTextChild(paxNode,"rk_weight",Qry.FieldAsInteger("rk_weight"));    
    NewTextChild(paxNode,"excess",Qry.FieldAsInteger("excess"));    
    NewTextChild(paxNode,"tags",Qry.FieldAsString("tags"));       
    NewTextChild(paxNode,"grp_id",Qry.FieldAsInteger("grp_id"));            
  };        
  Qry.Close();    
};           

void CheckInInterface::SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id,grp_id,ckin_stage,hall;
  string cl,airp_arv;
  TReqInfo *reqInfo = TReqInfo::Instance();     
  reqInfo->user.check_access(amPartialWrite);
  TQuery Qry(&OraSession);
  //определим, открыт ли рейс для регистрации
  
  point_id=NodeAsInteger("point_id",reqNode);
  //лочим рейс
  Qry.Clear();
  Qry.SQLText="SELECT trip_id FROM trips WHERE trip_id=:trip_id FOR UPDATE";
  Qry.CreateVariable("trip_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс не найден. Обновите данные"); 
  
  //map для норм
  map<int,string> norms;

  //определим - новая регистрация или запись изменений
  xmlNodePtr node,node2,remNode; 
  node = GetNode("grp_id",reqNode);
  if (node==NULL||NodeIsNULL(node))
  {
    cl=NodeAsString("class",reqNode);
    airp_arv=NodeAsString("airp_arv",reqNode);
    //новая регистрация 
    //проверка наличия свободных мест
    Qry.Clear();
    Qry.SQLText=
      "SELECT free_ok,free_goshow,nooccupy FROM counters2 "
      "WHERE point_id=:point_id AND class=:class AND airp=:airp_arv ";
    Qry.CreateVariable("point_id", otInteger, point_id);  
    Qry.CreateVariable("class", otString, cl);  
    Qry.CreateVariable("airp_arv", otString, airp_arv);
    Qry.Execute();
    if (Qry.Eof) throw UserException("Рейс изменен. Обновите данные");
    string grp_status(NodeAsString("status",reqNode)),place_status;
    int free,seats_sum=0;
    if (grp_status=="T")
    {
      free=Qry.FieldAsInteger("nooccupy");
      place_status="TR";
    }  
    else
    {
      if (grp_status=="K")
      {
        if (ckin_stage==sOpenCheckIn) 
          free=Qry.FieldAsInteger("free_ok");
        else
          free=Qry.FieldAsInteger("nooccupy");          
      }
      else
      {
        if (ckin_stage==sOpenCheckIn) 
          free=Qry.FieldAsInteger("free_goshow");
        else
          free=Qry.FieldAsInteger("nooccupy");          
      };        
      place_status="FP";  
    };  
    Qry.Next();
    if (!Qry.Eof) 
      throw Exception("Duplicate airport or class code in counters (point_id=%d, airp_arv=%s, class=%s)",
                      point_id,airp_arv.c_str(),cl.c_str());
                      
    hall=NodeAsInteger("hall",reqNode);                  
    Qry.Clear();
    Qry.SQLText="SELECT pr_vip FROM halls2 WHERE id=:hall"; 
    Qry.CreateVariable("hall",otInteger,hall);
    Qry.Execute();
    if (Qry.Eof) throw UserException("Неверно указан зал регистрации");
    bool addVIP=Qry.FieldAsInteger("pr_vip")!=0;                     
                      
    //простановка ремарок VIP,EXST, если нужно                  
    //подсчет seats
    int seats;
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
    { 
      if (free<0) free=0;
      throw UserException("Доступных мест осталось %d",free);
    };            
    node=NodeAsNode("passengers",reqNode);       
    Passengers.Clear();          
    //заполним массив для рассадки                                    
    for(node=node->children;node!=NULL;node=node->next)       
    {                                                 
        node2=node->children;
        if (NodeAsIntegerFast("seats",node2)==0) continue;              
        
        TPassenger pas;         
        pas.clname=cl;
        if (place_status=="FP"&&!NodeIsNULLFast("pax_id",node2))
          pas.placeStatus="BR";
        else  
          pas.placeStatus=place_status;
        pas.placeName=NodeAsStringFast("seat_no",node2);
        pas.countPlace=NodeAsIntegerFast("seats",node2);
        pas.placeRem=NodeAsStringFast("seat_type",node2);       
        remNode=GetNodeFast("rems",node2);              
        if (remNode!=NULL)
        {
          for(remNode=remNode->children;remNode!=NULL;remNode=remNode->next)              
          {
            node2=remNode->children;    
            pas.rems.push_back(NodeAsStringFast("rem_code",node2));               
          };  
        };              
        Passengers.Add(pas);
    };    
    // начитка салона
    TSalons Salons;
    Salons.trip_id = point_id;
    Salons.ClName = cl;
    Salons.Read( rTripSalons );    
    //рассадка      
    SEATS::SeatsPassengers( &Salons );          
    SEATS::SavePlaces( );    
    //заполним номера мест после рассадки
    node=NodeAsNode("passengers",reqNode);       
    int i=0;    
    for(node=node->children;node!=NULL&&i<Passengers.getCount();node=node->next)              
    {                                                 
        node2=node->children;
        if (NodeAsIntegerFast("seats",node2)==0) continue;   
        if (Passengers.Get(i).placeName=="") throw Exception("SeatsPassengers: empty placeName");
        ReplaceTextChild(node,"seat_no",Passengers.Get(i).placeName);
        i++;
    };      
    //if (node!=NULL||i<Passengers.getCount()) throw Exception("SeatsPassengers: wrong count");    
    //получим рег. номера и признак совместной регистрации и посадки
    Qry.Clear();
    Qry.SQLText=
      "SELECT NVL(MAX(reg_no)+1,1) AS reg_no FROM pax_grp,pax "
      "WHERE pax_grp.grp_id=pax.grp_id AND point_id=:point_id";        
    Qry.CreateVariable("point_id",otInteger,point_id);        
    Qry.Execute();
    int reg_no = Qry.FieldAsInteger("reg_no");
    bool pr_with_reg=false; 
    Qry.Clear();
    Qry.SQLText=
      "SELECT pr_with_reg FROM trip_brd "
      "WHERE point_id=:point_id AND (hall=:hall OR hall IS NULL) "
      "ORDER BY DECODE(hall,NULL,1,0)";
    Qry.CreateVariable("point_id",otInteger,point_id);        
    Qry.CreateVariable("hall",otInteger,hall);          
    Qry.Execute();
    if (!Qry.Eof) pr_with_reg=Qry.FieldAsInteger("pr_with_reg")!=0;
    
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "   
      "  SELECT pax_grp__seq.nextval INTO :grp_id FROM dual; "   
      "  INSERT INTO pax_grp(grp_id,point_id,target,class,class_bag, "   
      "                      status,excess,pr_wl,hall,pr_refuse,tid) "   
      "  VALUES(:grp_id,:point_id,:target,:class,:class_bag, "   
      "         :status,:excess,0,:hall,0,tid__seq.nextval); "   
      "END;";   
    Qry.CreateVariable("grp_id",otInteger,FNull);
    Qry.CreateVariable("point_id",otInteger,point_id);  
    Qry.CreateVariable("target",otString,airp_arv);  
    Qry.CreateVariable("class",otString,cl);  
    Qry.CreateVariable("class_bag",otString,cl);  
    Qry.CreateVariable("status",otString,grp_status);  
    Qry.CreateVariable("excess",otInteger,NodeAsInteger("excess",reqNode));  
    Qry.CreateVariable("hall",otInteger,hall);      
    Qry.Execute();    
    grp_id=Qry.GetVariableAsInteger("grp_id");            
    ReplaceTextChild(reqNode,"grp_id",grp_id);
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  IF :pax_id IS NULL THEN "
      "    SELECT pnl_id.nextval INTO :pax_id FROM dual; "
      "  END IF; "
      "  INSERT INTO pax(pax_id,grp_id,surname,name,pers_type,seat_no,seat_type,seats,pr_brd, "
      "                  refuse,reg_no,ticket_no,coupon_no,document,doc_check,subclass,tid) "
      "  VALUES(:pax_id,pax_grp__seq.currval,:surname,:name,:pers_type,:seat_no,:seat_type,:seats,:pr_brd, "
      "         NULL,:reg_no,:ticket_no,:coupon_no,:document,0,:subclass,tid__seq.currval); "
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
        Qry.SetVariable("pr_brd",(int)pr_with_reg);
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
            throw UserException((string)"Пассажир "+surname+(*name!=0?" ":"")+name+
                                        " уже зарегистрирован с другой стойки");
          else
            throw;
        };        
        ReplaceTextChild(node,"pax_id",Qry.GetVariableAsInteger("pax_id"));
        
        //запись ремарок
        SavePaxRem(node);
        //запись норм                                       
        string normStr=SavePaxNorms(node,norms);
        //запись информации по пассажиру в лог
        TLogMsg msg; 
        msg.ev_type=ASTRA::evtPax;
        msg.id1=point_id;
        msg.id2=reg_no;
        msg.id3=grp_id;     
        msg.msg=(string)"Пассажир "+surname+(*name!=0?" ":"")+name+" ("+pers_type+") зарегистрирован"+
                (pr_with_reg?" и посажен. ":". ")+     
                "П/н: "+airp_arv+", класс: "+cl+", статус: "+grp_status+", место: "+
                (seats>0?seat_no+(seats>1?"+"+IntToString(seats-1):""):"нет")+
                msg.msg+=". Баг.нормы: "+normStr;      
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
      throw UserException("Изменения в группе производились с другой стойки. Обновите данные");  
          
        
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
                   "    pr_brd=DECODE(:refuse,NULL,pr_brd,NULL), "
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
    
    TQuery SalonQry(&OraSession);
    SalonQry.Clear();
    SalonQry.SQLText=
      "BEGIN "
      "  salons.seatpass( :point_id, :pax_id, :seat_no, 0 ); "
      "  UPDATE pax SET seat_no=NULL,prev_seat_no=seat_no WHERE pax_id=:pax_id; "
      "END;";
    SalonQry.CreateVariable("point_id",otInteger,point_id);
    SalonQry.DeclareVariable("pax_id",otInteger);
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
          //были изменения в информации по пассажиру                                
          if (!NodeIsNULLFast("refuse",node2)&&!Qry.FieldIsNULL("seat_no"))
          {
            SalonQry.SetVariable("pax_id",pax_id);
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
          PaxQry.Execute();
          if (PaxQry.RowsProcessed()<=0) 
            throw UserException((string)"Изменения по пассажиру "+surname+(*name!=0?" ":"")+name+
                                        " производились с другой стойки. Обновите данные");             
          
          //запись информации по пассажиру в лог                              
          if (old_refuse!=refuse)
          {
            if (old_refuse=="")
              reqInfo->MsgToLog((string)"Пассажир "+surname+(*name!=0?" ":"")+name+" ("+pers_type+") разрегистрирован. "+
                                "Причина отказа в регистрации: "+refuse+". ", 
                                ASTRA::evtPax,point_id,reg_no,grp_id);
            else
              reqInfo->MsgToLog((string)"Пассажир "+surname+(*name!=0?" ":"")+name+" ("+pers_type+"). "+
                                "Изменена причина отказа в регистрации: "+refuse+". ",
                                ASTRA::evtPax,point_id,reg_no,grp_id);                              
          }
          else
          {
            //проверить на PaxUpdatespending!!!      
            reqInfo->MsgToLog((string)"Пассажир "+surname+(*name!=0?" ":"")+name+" ("+pers_type+"). "+
                              "Изменены данные пассажира.",
                              ASTRA::evtPax,point_id,reg_no,grp_id);      
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
            throw UserException((string)"Изменения по пассажиру "+surname+(*name!=0?" ":"")+name+
                                        " производились с другой стойки. Обновите данные");                             
        };
        //запись ремарок
        SavePaxRem(node);
        //запись норм                                       
        string normStr=SavePaxNorms(node,norms);                              
        if (normStr!="")
          reqInfo->MsgToLog((string)"Пассажир "+surname+(*name!=0?" ":"")+name+". "+
                            "Баг.нормы: "+normStr,ASTRA::evtPax,point_id,reg_no,grp_id);                                                          
      };        
    };  
    
    
  };
    
  SaveBag(reqNode);
  SavePaidBag(reqNode);
  SaveBagToLog(reqNode);
  SaveTagPacks(reqNode);
  
    //проверим дублирование билетов
    Qry.Clear();
    Qry.SQLText=    
      "SELECT COUNT(*),ticket_no FROM pax "
      "WHERE ticket_no IS NOT NULL AND coupon_no IS NOT NULL "
      "GROUP BY ticket_no HAVING COUNT(*)>1";
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())   
    {
      node=NodeAsNode("passengers",reqNode);       
      for(node=node->children;node!=NULL;node=node->next)             
      {                                               
        node2=node->children;
        if (!NodeIsNULLFast("coupon_no",node2) &&
            strcmp(Qry.FieldAsString("ticket_no"),NodeAsStringFast("ticket_no",node2))==0)      
          throw UserException("Эл. билет №%s дублируется",NodeAsStringFast("ticket_no",node2));
      };                
    };  
    //обновление counters
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  mvd.sync_pax_grp(:grp_id,:desk); "
      "  ckin.check_grp(:grp_id); "  
      "  ckin.recount(:point_id); "
      "END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("grp_id",otInteger,grp_id);  
    Qry.CreateVariable("desk",otString,reqInfo->desk.code);        
    Qry.Execute();
    Qry.Close();    
  //пересчитать данные по группе и отправить на клиент  
  LoadPax(ctxt,reqNode,resNode);     
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
      "point_id=:point_id AND reg_no=:reg_no";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("reg_no",otInteger,reg_no);
    Qry.Execute();
    if (Qry.Eof) throw UserException(1,"Регистрационный номер не найден");
    grp_id=Qry.FieldAsInteger("grp_id");
    Qry.Next();
    if (!Qry.Eof) throw Exception("Duplicate reg_no (point_id=%d reg_no=%d)",point_id,reg_no);
  }
  else grp_id=NodeAsInteger(node);  
  
  Qry.Clear();
  Qry.SQLText=
    "SELECT target AS airp_arv,airps.city AS city_arv, "
    "       class,class_bag,status,hall,pax_grp.tid "
    "FROM pax_grp,airps "
    "WHERE pax_grp.target=airps.cod AND grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (Qry.Eof) return; //это бывает когда разрегистрация всей группы по ошибке агента
  NewTextChild(resNode,"grp_id",grp_id);
  NewTextChild(resNode,"airp_arv",Qry.FieldAsString("airp_arv"));
  NewTextChild(resNode,"city_arv",Qry.FieldAsString("city_arv"));
  NewTextChild(resNode,"class",Qry.FieldAsString("class"));
  NewTextChild(resNode,"class_bag",Qry.FieldAsString("class_bag"));
  NewTextChild(resNode,"status",Qry.FieldAsString("status"));
  NewTextChild(resNode,"hall",Qry.FieldAsInteger("hall"));
  NewTextChild(resNode,"tid",Qry.FieldAsInteger("tid"));
  
  Qry.Clear();
  Qry.SQLText="SELECT pax_id FROM bp_print WHERE pax_id=:pax_id AND rownum=1";
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
    //ищем норму в norms
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
            normStr=normStr+" "+Qry.FieldAsString("amount")+"м"+Qry.FieldAsString("weight");
          else
            normStr=normStr+" "+Qry.FieldAsString("weight");
          if (!Qry.FieldIsNULL("per_unit")&&Qry.FieldAsInteger("per_unit")>0)
            normStr+="кг/м";
          else
            normStr+="кг";            
        }
        else
        {
          if (!Qry.FieldIsNULL("amount"))       
            normStr=normStr+" "+Qry.FieldAsString("amount")+"м";                
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
    //составим строчку для записи в журнал операций
    if (!NodeIsNULLFast("bag_type",node2))
    {                 
      if (!logStr.empty()) logStr+=", ";
      if (!NodeIsNULLFast("norm_id",node2))
        logStr=logStr+NodeAsStringFast("bag_type",node2)+": "+norms[NodeAsIntegerFast("norm_id",node2)];
      else  
        logStr=logStr+NodeAsStringFast("bag_type",node2)+": нет";       
    }
    else  
    {
      if (!logStr.empty()) logStr=", "+logStr;  
      if (!NodeIsNULLFast("norm_id",node2))
        logStr=norms[NodeAsIntegerFast("norm_id",node2)]+logStr;
      else
        logStr="нет"+logStr;    
    };        
  };                    
  NormQry.Close();         
  if (logStr=="") logStr="нет";
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
    if (trferNode->next!=NULL)  
      TrferQry.SetVariable("transfer_num",i);
    else
      TrferQry.SetVariable("transfer_num",9);      
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
  int point_id=NodeAsInteger("point_id",grpNode);       
  int grp_id=NodeAsInteger("grp_id",grpNode);   
  
  xmlNodePtr valueBagNode=GetNode("value_bags",grpNode);
  xmlNodePtr bagNode=GetNode("bags",grpNode);
  xmlNodePtr tagNode=GetNode("tags",grpNode);
  if (bagNode==NULL&&tagNode==NULL) return;      
  //подсчитаем кол-во багажа и баг. бирок        
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
  bool pr_tag_print=tagNode!=NULL&&GetNode("@type",tagNode)!=NULL;
  TQuery Qry(&OraSession); 
  if (bagAmount!=tagCount)
  {    
    if (pr_tag_print && tagCount<bagAmount )
    {      
      const char* tag_type=NodeAsString("@type",tagNode);        
      //получим номера печатаемых бирок
      Qry.Clear();
      Qry.SQLText=             
        "DECLARE "
        "  vairline avia.kod_ak%TYPE; " 
        "  vaircode avia.aircode%TYPE; "
        "BEGIN "
        "  BEGIN "
        "    SELECT company INTO vairline FROM trips WHERE trip_id=:point_id; "
        "    SELECT aircode INTO vaircode FROM avia WHERE kod_ak=vairline; "
        "  EXCEPTION "
        "    WHEN OTHERS THEN vaircode:=NULL; "
        "  END; "
        "  ckin.get__tag_no(:desk,vaircode,:tag_count,:first_no,:last_no); "
        "END;";
      Qry.CreateVariable("point_id",otInteger,point_id);
      Qry.CreateVariable("desk",otString,point_id);
      Qry.CreateVariable("tag_count",otInteger,bagAmount-tagCount);
      Qry.DeclareVariable("first_no",otInteger);
      Qry.DeclareVariable("last_no",otInteger);
      Qry.Execute();
      int first_no=Qry.GetVariableAsInteger("first_no");
      int last_no=Qry.GetVariableAsInteger("last_no");      
      if (tagNode==NULL) tagNode=NewTextChild(grpNode,"tags");
      if ((first_no/1000)==(last_no/1000))
      {
        //первый и последний номер из одного диапазона        
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
    }
    else throw UserException(1,"Кол-во бирок и мест багажа не совпадает");      
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
          throw UserException("Бирка %s %s%010.f уже зарегистрирована.",tag_type,color,no);         
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
    "SELECT num,tag_type,no_len,no,color,bag_num,pr_ier,pr_print "
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
    NewTextChild(tagNode,"printable",(int)(BagQry.FieldAsInteger("pr_ier")!=0));
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

//запись багажа в лог
void CheckInInterface::SaveBagToLog(xmlNodePtr grpNode)        
{                
  if (grpNode==NULL) return;  
  int point_id=NodeAsInteger("point_id",grpNode);       
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
    //строка по общему кол-ву багажа
    Qry.Clear();
    Qry.SQLText=
      "SELECT "
      "       NVL(ckin.get_bagAmount(grp_id,NULL),0) AS bagAmount, "
      "       NVL(ckin.get_bagWeight(grp_id,NULL),0) AS bagWeight, "
      "       NVL(ckin.get_rkAmount(grp_id,NULL),0) AS rkAmount, "
      "       NVL(ckin.get_rkWeight(grp_id,NULL),0) AS rkWeight, "
      "       ckin.get_birks(grp_id,NULL) AS tags "
      "FROM pax_grp where grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);   
    Qry.Execute();    
    if (!Qry.Eof)
    {      
      msg.msg=(string)"Багаж: "+Qry.FieldAsString("bagAmount")+"/"+Qry.FieldAsString("bagWeight")+", "+
                      "р/кладь: "+Qry.FieldAsString("rkAmount")+"/"+Qry.FieldAsString("rkWeight")+". ";           
      if (!Qry.FieldIsNULL("tags"))
        msg.msg=msg.msg+"Бирки: "+Qry.FieldAsString("tags")+". ";
      reqInfo->MsgToLog(msg);          
    };        
  };      
  if (bagNode!=NULL || paidBagNode!=NULL)
  {
    //строка по типам багажа и оплачиваемому багажу
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
      "ORDER BY paid_bag.bag_type ";    
    Qry.CreateVariable("grp_id",otInteger,grp_id);   
    Qry.Execute();      
    msg.msg.clear();
    for(;!Qry.Eof;Qry.Next())
    {
      if (Qry.FieldAsInteger("bag_amount")==0 && 
          Qry.FieldAsInteger("bag_weight")==0 &&
          Qry.FieldAsInteger("paid_weight")==0) continue; 
      if (!Qry.FieldIsNULL("bag_type"))
      {
        if (!msg.msg.empty()) msg.msg+=", "; 
        msg.msg=msg.msg+Qry.FieldAsString("bag_type")+": "+
                        Qry.FieldAsString("bag_amount")+"/"+
                        Qry.FieldAsString("bag_weight")+"/"+
                        Qry.FieldAsString("paid_weight");
      }      
      else
      {
        if (!msg.msg.empty()) msg.msg=", "+msg.msg;
        msg.msg=(string)Qry.FieldAsString("bag_amount")+"/"+
                        Qry.FieldAsString("bag_weight")+"/"+
                        Qry.FieldAsString("paid_weight")+msg.msg;        
      };        
    };    
    if (!msg.msg.empty())
    {
      msg.msg="Багаж по типам (мест/вес/опл): "+msg.msg;
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
    "    WHERE term=:desk AND airline=:airline AND target=:target; "
    "  ELSE "
    "    UPDATE tag_packs "
    "    SET tag_type=:tag_type, color=:color, no=:no "
    "    WHERE term=:desk AND airline=:airline AND target=:target; "
    "    IF SQL%NOTFOUND THEN "
    "      INSERT INTO tag_packs(term,airline,target,tag_type,color,no) "
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




