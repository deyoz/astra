#include <string>
#include "checkin.h"
#include "oralib.h"
#include "xml_unit.h"
#include "etick/tick_data.h"
#include "astra_ticket.h"
#include "etick_change_status.h"
#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"

using namespace std;
using namespace Ticketing;
using namespace Ticketing::CouponStatus;

void CheckInInterface::SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
   int point_id;	   
   xmlNodePtr node;   
   node=GetNode("point_id",reqNode); 
   if (node!=NULL)
     point_id=NodeAsInteger(node);
   else
     point_id=-1;   
     
   node=GetNode("pax_id",reqNode); 
   if (node!=NULL) 
   {
     if (ETCheckStatus(NodeAsInteger(node),csaPax,point_id))     
       showProgError("Нет связи с сервером эл. билетов");  	    
     return;
   };
   
   node=GetNode("grp_id",reqNode); 
   if (node!=NULL) 
   {
     if (ETCheckStatus(NodeAsInteger(node),csaGrp,point_id))     
       showProgError("Нет связи с сервером эл. билетов");  	    
     return;
   };
   
   node=GetNode("point_id",reqNode); 
   if (node!=NULL) 
   {
     if (ETCheckStatus(NodeAsInteger(node),csaFlt,point_id))
       showProgError("Нет связи с сервером эл. билетов");  	    
     return;
   };        
}

void CheckInInterface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
   ProgTrace(TRACE3,"Kick on change of status request");
}

bool ETCheckStatus(int id, TETCheckStatusArea area, int point_id)
{
  TQuery Qry(&OraSession);
  string sql=
    "SELECT trips.trip_id AS point_id, trips.company, trips.flt_no, trips.scd, trips.act, "
    "       options.cod AS airp_dep, pax_grp.target AS airp_arv, pax_grp.class, " 
    "       pax.ticket_no, pax.coupon_no, "
    "       pax.refuse, pax.pr_brd, "
  //  "       etickets.ticket_no AS conf_ticket_no, etickets.coupon_no AS conf_coupon_no, "
    "       etickets.point_id AS tick_point_id, etickets.coupon_status AS coupon_status "
    "FROM trips,pax_grp,pax,options,etickets "
    "WHERE trips.trip_id=pax_grp.point_id AND pax_grp.grp_id=pax.grp_id AND "
    "      pax.ticket_no IS NOT NULL AND pax.coupon_no IS NOT NULL AND "
    "      pax.ticket_no=etickets.ticket_no(+) AND "
    "      pax.coupon_no=etickets.coupon_no(+) AND ";    
  switch (area) 
  {
    case csaFlt:
      sql=sql+"pax_grp.point_id=:point_id ";   
      Qry.CreateVariable("point_id",otInteger,id);
      break;
    case csaGrp:
      sql=sql+"pax.grp_id=:grp_id ";   
      Qry.CreateVariable("grp_id",otInteger,id);
      break;
    case csaPax:    	
      sql=sql+"pax.pax_id=:pax_id ";   
      Qry.CreateVariable("pax_id",otInteger,id);
      break;
    default: ;  
  } 
  
  TQuery UpdQry(&OraSession);
  UpdQry.SQLText=
    "BEGIN "
    "  UPDATE etickets SET point_id=:point_id "
    "  WHERE ticket_no=:ticket_no AND coupon_no=:coupon_no; "
    "  IF SQL%NOTFOUND THEN "
    "    INSERT INTO etickets(ticket_no,coupon_no,point_id,coupon_status) "
    "    VALUES(:ticket_no,:coupon_no,:point_id,NULL); "
    "  END IF; "
    "END;";
  UpdQry.DeclareVariable("ticket_no",otString);  
  UpdQry.DeclareVariable("coupon_no",otInteger);  
  UpdQry.DeclareVariable("point_id",otInteger);  
    
    
  Qry.SQLText=sql;
  Qry.Execute();
  list<Ticket> ltick;     
  
  for(;!Qry.Eof;Qry.Next())
  {    	          
    coupon_status status;
    if (Qry.FieldIsNULL("coupon_status"))
      status=coupon_status(OriginalIssue);
    else
      coupon_status(Qry.FieldAsString("coupon_status"));
      
    coupon_status real_status;   	            	
    if (!Qry.FieldIsNULL("refuse"))    
      //разрегистрирован	
      real_status=coupon_status(OriginalIssue);
    else
    {
      if (Qry.FieldAsInteger("pr_brd")==0)      
      	//не посажен
        real_status=coupon_status(Checked); 
      else
      {
        if (Qry.FieldIsNULL("act"))
           //самолет не улетел
          real_status=coupon_status(Boarded);
        else  
          real_status=coupon_status(Flown);
      };	        	
    };
    
    //if (status!=real_status)
    {          	          	
      Coupon_info ci (Qry.FieldAsInteger("coupon_no"),real_status);
      //       Itin itin("P2","", 1009,-1, date(2006,6,5), time_duration(23,15,0),
//                 "VKO", "LED");  
      Coupon cpn(ci, "");
      list<Coupon> lcpn;
      lcpn.push_back(cpn);

      Ticket tick(Qry.FieldAsString("ticket_no"), lcpn);
      ltick.push_back(tick);        
      ProgTrace(TRACE5,"ETChangeStatus %s/%d->%s",
                       Qry.FieldAsString("ticket_no"),
                       Qry.FieldAsInteger("coupon_no"),
                       real_status.dispCode());
                       
    };
    if (Qry.FieldIsNULL("tick_point_id") || 
        Qry.FieldAsInteger("point_id")!=Qry.FieldAsInteger("tick_point_id"))
    {
      UpdQry.SetVariable("ticket_no",Qry.FieldAsString("ticket_no"));	
      UpdQry.SetVariable("coupon_no",Qry.FieldAsInteger("coupon_no"));	
      UpdQry.SetVariable("point_id",Qry.FieldAsInteger("point_id"));	
      UpdQry.Execute();
    };	         
  };    
    
  
  if (point_id>=0)
  {
    Qry.Clear();
    Qry.SQLText=          
      "SELECT etickets.ticket_no,etickets.coupon_no "
      "FROM etickets,pax "
      "WHERE etickets.ticket_no=pax.ticket_no(+) AND "
      "      etickets.coupon_no=pax.coupon_no(+) AND "
      "      etickets.point_id=:point_id AND "
      "      pax.ticket_no IS NULL ";                  	
    Qry.CreateVariable("point_id",otInteger,point_id);  
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      Coupon_info ci (Qry.FieldAsInteger("coupon_no"),OriginalIssue);
      //       Itin itin("P2","", 1009,-1, date(2006,6,5), time_duration(23,15,0),
//                 "VKO", "LED");  
      Coupon cpn(ci, "");
      list<Coupon> lcpn;
      lcpn.push_back(cpn);

      Ticket tick(Qry.FieldAsString("ticket_no"), lcpn);
      ltick.push_back(tick);        
      ProgTrace(TRACE5,"ETChangeStatus %s/%d->%s",
                       Qry.FieldAsString("ticket_no"),
                       Qry.FieldAsInteger("coupon_no"),
                       "O");
    };	    	                    
  };	 	
  
    
  if (!ltick.empty())   
    ChangeStatus::ETChangeStatus(TReqInfo::Instance(), ltick);  

  return (!ltick.empty());      
}

