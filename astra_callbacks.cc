#include "setup.h"
#include "astra_callbacks.h"  
#include "maindcs.h"		
#include "adm.h"
#include "cache.h" 
#include "pay.h"
#include "brd.h"
#include "season.h"
#include "etick.h" 
#include "images.h" 
#include "tripinfo.h" 
#include "cent.h" 
#include "prepreg.h" 
#include "salonform.h" 
#include "sopp.h" 
#include "checkin.h"
#include "astra_utils.h" 
#include "basic.h"
#include "exceptions.h"
#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"

#include "jxtlib.h"
#include "oralib.h"
#include "ocilocal.h"
#include "xml_unit.h"
#include "perfom.h"

using namespace jxtlib;
using namespace BASIC;

void AstraJxtCallbacks::InitInterfaces()
{
  ProgTrace(TRACE3, "AstraJxtCallbacks::InitInterfaces");
  new SysReqInterface();     
  new MainDCSInterface(); 
  new AdmInterface();
  new PayInterface();
  new CacheInterface();  
  new BrdInterface();
  new SeasonInterface();
  new ETSearchInterface();
  new ETStatusInterface();
  new ImagesInterface();      
  new CheckInInterface();
  new TripsInterface();        
  new SalonsInterface();        
  new CentInterface();           
  new PrepRegInterface();          
  new SoppInterface();            
};

void AstraJxtCallbacks::UserBefore(const char *body, int blen, const char *head,
                          int hlen, char **res, int len)
{   
    OraSession.ClearQuerys();	   
    XMLRequestCtxt *xmlRC = getXmlCtxt();
    std::string screen = NodeAsString("/term/query/@screen", xmlRC->reqDoc);
    TReqInfo *reqInfo = TReqInfo::Instance();    
    ProgTrace(TRACE3,"Before reqInfo->Initialize");
    bool checkUserLogon =
      GetNode( "/term/query/CheckUserLogon", xmlRC->reqDoc ) == NULL &&
      GetNode( "/term/query/UserLogon", xmlRC->reqDoc ) == NULL &&
      GetNode( "/term/query/ClientError", xmlRC->reqDoc ) == NULL;
          
    reqInfo->Initialize( screen, xmlRC->pult, xmlRC->opr, checkUserLogon );                         
    if ( xmlRC->opr.empty() ) 
    { /* ������ ��襫 ���⮩ - ��ࠢ�塞 ���� �� ������� */
      showBasicInfo();
    }
    PerfomInit();
}
   
void AstraJxtCallbacks::UserAfter()
{
  PerfomTest( 2007 );
}


void AstraJxtCallbacks::HandleException(std::exception *e)
{
	ProgTrace(TRACE3, "AstraJxtCallbacks::HandleException");

	XMLRequestCtxt *ctxt = getXmlCtxt();
	xmlNodePtr resNode = ctxt->resDoc->children->children;
	xmlNodePtr node = resNode->children;
	
	UserException2 *ue2 = dynamic_cast<UserException2*>(e);	
	if (ue2) return;	
	
	xmlNodePtr node2;
	while(node!=NULL)	
	{	  
	  if (strcmp((char*)node->name,"basic_info")!=0&&strcmp((char*)node->name,"command")!=0) 
	  {	    
	    node2=node;	
	    node=node->next; 
	    xmlUnlinkNode(node2);
	    xmlFreeNode(node2);
	  }
	  else node=node->next; 	  
	};				

	EOracleError *orae = dynamic_cast<EOracleError*>(e);
	if (orae)
	{		
		ProgError(STDLOG,"EOracleError: %s (code=%d)",orae->what(),orae->Code);		
		showProgError("�訡�� ��ࠡ�⪨ �����. ������� � ࠧࠡ��稪��");		
//		addXmlBM(*ctxt);
                return;
	};
	EXCEPTIONS::UserException *ue = dynamic_cast<EXCEPTIONS::UserException*>(e);
	if (ue)
	{
                ProgTrace( TRACE5, "UserException: %s", ue->what() );
                showError(ue->what(), ue->Code());
		//addXmlBM(*ctxt);
                return;
	}
	std::logic_error *exp = dynamic_cast<std::logic_error*>(e);
	if (exp)	
	  ProgError(STDLOG,"logic_error: %s",exp->what());	
	else
	  ProgError(STDLOG,"Unknown error");
	  
	showProgError("�訡�� ��ࠡ�⪨ �����. ������� � ࠧࠡ��稪��");		  
	return;	
}
