#include "astra_callbacks.h"
#include "cache.h"

#define NICKNAME "VLAD"
#include "test.h"
#include "setup.h"
#include "jxtlib.h"
#include "oralib.h"
#include "ocilocal.h"
#include "xml_unit.h"

using namespace jxtlib;

void AstraCallbacks::InitInterfaces()
{
  ProgTrace(TRACE3, "AstraCallbacks::InitInterfaces");
  new CacheInterface();  
};	

void AstraCallbacks::HandleException(std::exception *e)
{
	ProgTrace(TRACE3, "AstraCallbacks::HandleException");

	XMLRequestCtxt *ctxt = getXmlCtxt();
	xmlNodePtr resNode = ctxt->resDoc->children->children;
	
	EOracleError *orae = dynamic_cast<EOracleError*>(e);
	if (orae)
	{
		ProgTrace(TRACE5,"EOracleError: %s (code=%d)",orae->what(),orae->Code);
		NewTextChild( resNode, "exception", orae->what() );
//		addXmlBM(*ctxt);
                return;
	};
	EXCEPTIONS::UserException *ue = dynamic_cast<EXCEPTIONS::UserException*>(e);
	if (ue)
	{
                ProgTrace( TRACE5, "UserException: %s", ue->what() );
		NewTextChild( resNode, "userexception", ue->what() );                
		//addXmlBM(*ctxt);
                return;
	}
	EXCEPTIONS::Exception *exp = dynamic_cast<EXCEPTIONS::Exception*>(e);
	if (exp)
	{
		ProgTrace(TRACE5,"Exception: %s",exp->what());
		NewTextChild( resNode, "exception", exp->what() );		
		return;
	};
}

