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
#define NICKNAME "VLAD"
#include "test.h"

#include "jxtlib.h"
#include "oralib.h"
#include "ocilocal.h"
#include "xml_unit.h"

using namespace jxtlib;

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
};

void AstraJxtCallbacks::UserBefore(const char *body, int blen, const char *head,
                          int hlen, char **res, int len)
{  
    XMLRequestCtxt *xmlRC = getXmlCtxt();
    //xmlNodePtr node = xmlRC.reqDoc->children->children;
    std::string screen = NodeAsString("/term/query/@screen", xmlRC->reqDoc);
    TReqInfo *reqInfo = TReqInfo::Instance();
    reqInfo->Initialize( screen, xmlRC->pult, xmlRC->opr );
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
		NewTextChild( node, "error", "�訡�� ��ࠡ�⪨ �����. ������� � ࠧࠡ��稪��" );
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
		NewTextChild( node, "error", "�訡�� ��ࠡ�⪨ �����. ������� � ࠧࠡ��稪��" );
		return;
	};
}
