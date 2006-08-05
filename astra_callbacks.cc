#include "setup.h"
#include "astra_callbacks.h"  
#include "adm.h"
#include "cache.h" 
#include "pay.h"
#include "brd.h"
#include "season.h"
#include "etick.h" 
#include "images.h" 
#include "seats.h" 
#include "astra_utils.h" 
#include "basic.h"
#define NICKNAME "VLAD"
#include "test.h"

#include "jxtlib.h"
#include "oralib.h"
#include "ocilocal.h"
#include "xml_unit.h"

using namespace jxtlib;
using namespace BASIC;

void AstraJxtCallbacks::InitInterfaces()
{
  ProgTrace(TRACE3, "AstraJxtCallbacks::InitInterfaces");
  new AdmInterface();
  new PayInterface();
  new CacheInterface();  
  new BrdInterface();
  new SeasonInterface();
  new ETSearchInterface();
  new ImagesInterface();      
  new SeatsInterface();      
  new SysReqInterface();      
};

void AstraJxtCallbacks::UserBefore(const char *body, int blen, const char *head,
                          int hlen, char **res, int len)
{      
    XMLRequestCtxt *xmlRC = getXmlCtxt();
    std::string screen = NodeAsString("/term/query/@screen", xmlRC->reqDoc);
    TReqInfo *reqInfo = TReqInfo::Instance();    
    reqInfo->Initialize( screen, xmlRC->pult, xmlRC->opr, 
                         GetNode( "/term/query/CheckBasicInfo", xmlRC->reqDoc ) != NULL );    
    if ( xmlRC->opr.empty() ) 
    { /* оператор пришел пустой - отправляем инфу по оператору */
      xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
      resNode = NewTextChild(resNode,"basic_info");
      xmlNodePtr node;
      if (!reqInfo->user.login.empty())
      {
        node = NewTextChild(resNode,"user");
        NewTextChild(node, "access_code",reqInfo->user.access_code);
        NewTextChild(node, "login",reqInfo->user.login);
      };  
      node = NewTextChild(resNode,"desk");
      NewTextChild(node,"airp",reqInfo->desk.airp);
      NewTextChild(node,"city",reqInfo->desk.city);
      NewTextChild(node,"time",DateTimeToStr( reqInfo->desk.time ) );
    }
}
   


void AstraJxtCallbacks::HandleException(std::exception *e)
{
	ProgTrace(TRACE3, "AstraJxtCallbacks::HandleException");

	XMLRequestCtxt *ctxt = getXmlCtxt();
	xmlNodePtr resNode = ctxt->resDoc->children->children;

	EOracleError *orae = dynamic_cast<EOracleError*>(e);
	if (orae)
	{
		ProgError(STDLOG,"EOracleError: %s (code=%d)",orae->what(),orae->Code);
		xmlNodePtr node = NewTextChild( resNode, "command" );
		NewTextChild( node, "error", "Ошибка обработки запроса. Обратитесь к разработчикам" );
//		addXmlBM(*ctxt);
                return;
	};
	EXCEPTIONS::UserException *ue = dynamic_cast<EXCEPTIONS::UserException*>(e);
	if (ue)
	{
                ProgTrace( TRACE5, "UserException: %s", ue->what() );
                xmlNodePtr node = NewTextChild( resNode, "command" );
                NewTextChild( node, "usererror", ue->what() );
		//addXmlBM(*ctxt);
                return;
	}
	std::logic_error *exp = dynamic_cast<std::logic_error*>(e);
	if (exp)
	{		
		ProgError(STDLOG,"logic_error: %s",exp->what());
                xmlNodePtr node = NewTextChild( resNode, "command" );		
		NewTextChild( node, "error", "Ошибка обработки запроса. Обратитесь к разработчикам" );
		return;
	};
}
