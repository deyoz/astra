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
    SeatsPassengers( &Salons );          
    SavePlaces( );    
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
  }
  else
  {
    grp_id=NodeAsInteger(node);	
    
    //проверим pr_refuse
    bool pr_refuse=true;
    node=NodeAsNode("passengers",reqNode);       
    for(node=node->children;node!=NULL;node=node->next)	      
    {
      node2=node->children;
      if (NodeIsNULLFast("refuse",node2)) 
      {
        pr_refuse=false;
        break;
      };  
    };
        
    Qry.Clear();
    Qry.SQLText=
      "UPDATE pax_grp "
      "SET excess=:excess,pr_refuse=:pr_refuse,tid=tid__seq.nextval "
      "WHERE grp_id=:grp_id AND tid=:tid";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.CreateVariable("tid",otInteger,NodeAsInteger("tid",reqNode));
    Qry.CreateVariable("excess",otInteger,NodeAsInteger("excess",reqNode));    
    Qry.CreateVariable("pr_refuse",otInteger,(int)pr_refuse);                       
    Qry.Execute();
    if (Qry.RowsProcessed()<=0) 
      throw UserException("Изменения в группе производились с другой стойки. Обновите данные");  
      
    Qry.Clear();
    Qry.SQLText="SELECT refuse,reg_no,seat_no FROM pax WHERE pax_id=:pax_id";
    Qry.DeclareVariable("pax_id",otInteger);  
        
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
      "END;";
    SalonQry.CreateVariable("point_id",otInteger,point_id);
    SalonQry.DeclareVariable("pax_id",otInteger);
    SalonQry.DeclareVariable("seat_no",otString);
                  
    node=GetNode("passengers",reqNode);
    if (node!=NULL)
    {
      node=node->children;	
      for(node=node->children;node!=NULL;node=node->next)	      
      {
      	node2=node->children;      	      	
      	int pax_id=NodeAsIntegerFast("pax_id",node2);
      	Qry.SetVariable("pax_id",pax_id);
      	Qry.Execute();
      	string old_refuse=Qry.FieldAsString("refuse");      	
      	int reg_no=Qry.FieldAsInteger("reg_no");      	
      	if (!Qry.FieldIsNULL("seat_no"))
      	{
      	  SalonQry.SetVariable("pax_id",pax_id);
      	  SalonQry.SetVariable("seat_no",Qry.FieldAsString("seat_no"));
      	  SalonQry.Execute();      
        };          
      	
      	const char* surname=NodeAsStringFast("surname",node2);
      	const char* name=NodeAsStringFast("name",node2);
      	const char* pers_type=NodeAsStringFast("pers_type",node2);
      	const char* refuse=NodeAsStringFast("refuse",node2);
      	PaxQry.SetVariable("pax_id",NodeAsIntegerFast("pax_id",node2));
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

