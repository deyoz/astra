#include "etick.h"
#include <string>
#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"
#include "xml_unit.h"
#include "tlg/edi_tlg.h"
#include "edilib/edi_func_cpp.h"
#include "astra_ticket.h"
#include "etick_change_status.h"
#include "astra_tick_view_xml.h"
#include "astra_tick_read_edi.h"
#include "jxt_cont.h"

using namespace std;
using namespace Ticketing;
using namespace edilib;
using namespace Ticketing::TickReader;
using namespace Ticketing::TickView;
using namespace Ticketing::TickMng;
using namespace Ticketing::ChangeStatus;
using namespace Ticketing::CouponStatus;
using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace JxtContext;

void ETSearchInterface::SearchETByTickNo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  string tick_no=NodeAsString("TickNoEdit",reqNode);

  OrigOfRequest org(*TReqInfo::Instance());
  TickDispByNum tickDisp(org, tick_no);
  SendEdiTlgTKCREQ_Disp( tickDisp );
  showProgError("Нет связи с сервером эл. билетов");
};

void ETSearchInterface::KickHandler(XMLRequestCtxt *ctxt,
                                    xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ProgTrace(TRACE3,"KickHandler ...");
    JxtCont *sysCont = getJxtContHandler()->sysContext();
    const char *edi_tlg = sysCont->readC("ETDisplayTlg");        
    if(!edi_tlg)    
      throw EXCEPTIONS::Exception("Can't read context 'ETDisplayTlg'");            
    int ret = ReadEdiMessage(edi_tlg);
    sysCont->remove("ETDisplayTlg");
    if(ret == EDI_MES_STRUCT_ERR){
      throw EXCEPTIONS::Exception("Error in message structure: %s",EdiErrGetString());      
    } else if( ret == EDI_MES_NOT_FND){
        throw EXCEPTIONS::Exception("No message found in template: %s",EdiErrGetString());        
    } else if( ret == EDI_MES_ERR) {        
        throw EXCEPTIONS::Exception("Edifact error ");
    }

    EDI_REAL_MES_STRUCT *pMes= GetEdiMesStruct();
    int num = GetNumSegGr(pMes, 3);
    if(!num){
        if(GetNumSegment(pMes, "ERC")){
            const char *errc = GetDBFName(pMes, DataElement(9321),
                                          "ET_NEG",
                                          CompElement("C901"),
                                          SegmElement("ERC"));
            ProgTrace(TRACE3,"err code=%s", errc);
            const char * err_msg = GetDBFName(pMes,
                                              DataElement(4440),
                                              SegmElement("IFT"), "ET_NEG");
            ProgTrace(TRACE1, "СЭБ: %s", err_msg);
            throw EXCEPTIONS::UserException(err_msg);
        }
        throw EXCEPTIONS::Exception("ETS error"); 
    } else if(num==1){
        try{
            xmlNodePtr dataNode=getNode(astra_iface(resNode, "ETViewForm"),"data");
            Pnr pnr = PnrRdr::doRead<Pnr>
                    (PnrEdiRead(GetEdiMesStruct()));
            Pnr::Trace(TRACE2, pnr);

            PnrDisp::doDisplay
                    (PnrXmlView(dataNode), pnr);
        }
        catch(edilib::EdiExcept &e)
        {            
            throw EXCEPTIONS::Exception("edilib: %s", e.what());
        }
    } else {
        throw EXCEPTIONS::UserException("Просмотр списка эл. билетов не поддерживается"); //пока не поддерживается
    }
}

void ETStatusInterface::ChangePaxStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{  	  
  xmlNodePtr node=GetNode("check_point_id",reqNode);	
  int point_id=-1;
  if (node!=NULL) point_id=NodeAsInteger(node);
  if (ETCheckStatus(OrigOfRequest(*TReqInfo::Instance()),NodeAsInteger("pax_id",reqNode),csaPax,point_id))     
    showProgError("Нет связи с сервером эл. билетов");  	     
};

void ETStatusInterface::ChangeGrpStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{  	  
  xmlNodePtr node=GetNode("check_point_id",reqNode);	
  int point_id=-1;
  if (node!=NULL) point_id=NodeAsInteger(node);
  if (ETCheckStatus(OrigOfRequest(*TReqInfo::Instance()),NodeAsInteger("grp_id",reqNode),csaGrp,point_id))     
    showProgError("Нет связи с сервером эл. билетов");  	     
};

void ETStatusInterface::ChangeFltStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{  	  
  xmlNodePtr node=GetNode("check_point_id",reqNode);	
  int point_id=-1;
  if (node!=NULL) point_id=NodeAsInteger(node);
  if (ETCheckStatus(OrigOfRequest(*TReqInfo::Instance()),NodeAsInteger("point_id",reqNode),csaFlt,point_id))     
    showProgError("Нет связи с сервером эл. билетов");  	     
};

void ETStatusInterface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
   ProgTrace(TRACE3,"Kick on change of status request");
};

bool ETCheckStatus(const OrigOfRequest &org, int id, TETCheckStatusArea area, int point_id)
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
      Coupon cpn(ci);
      list<Coupon> lcpn;
      lcpn.push_back(cpn);

      Ticket tick(Qry.FieldAsString("ticket_no"), lcpn);
      ltick.push_back(tick);        
      ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
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
      Coupon cpn(ci);
      list<Coupon> lcpn;
      lcpn.push_back(cpn);

      Ticket tick(Qry.FieldAsString("ticket_no"), lcpn);
      ltick.push_back(tick);        
      ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
                       Qry.FieldAsString("ticket_no"),
                       Qry.FieldAsInteger("coupon_no"),
                       "O");
    };	    	                    
  };	 	
  
    
  if (!ltick.empty())   
    ChangeStatus::ETChangeStatus(org, ltick);  
    
    

  return (!ltick.empty());      
}





