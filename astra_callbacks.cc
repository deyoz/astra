#include "astra_callbacks.h"
#include "cache.h"
#include "etick.h"

#define NICKNAME "VLAD"
#include "test.h"

#include "jxtlib.h"
#include "oralib.h"
#include "ocilocal.h"

using namespace jxtlib;

void AstraCallbacks::InitInterfaces()
{
  ProgTrace(TRACE3, "AstraCallbacks::InitInterfaces");
  new CacheInterface();  
  new ETSearchInterface();
};	

void AstraCallbacks::HandleException(std::exception *e)
{
	ProgTrace(TRACE3, "AstraCallbacks::HandleException");

	XMLRequestCtxt *ctxt = getXmlCtxt();
	xmlNodePtr resNode = ctxt->resDoc->children->children;
	
	
	EXCEPTIONS::CustomException *ce = dynamic_cast<EXCEPTIONS::CustomException*>(e);
	if (ce)
	{
/*		if(!ctxt->resDoc || !ctxt->resDoc->children ||
			!ctxt->resDoc->children->children)
		{
			xmlFreeDoc(ctxt->resDoc);
			ctxt->resDoc = createExceptionDoc(getTextByNum(FATAL_ERR), 0);
		}
		else
		{
			char *tmp = getprop(resNode = ctxt->resDoc->children->children, "handle");
			if (!tmp)
				xmlSetPropInt(resNode, "handle", ctxt->GetQueryHandle());
		}
		addXmlBM(*ctxt);*/
		ProgTrace(TRACE5,"CustomException: %s",ce->what());
		return;
	};
	EOracleError *orae = dynamic_cast<EOracleError*>(e);
	if (orae)
	{
		ProgTrace(TRACE5,"EOracleError: %s (code=%d)",orae->what(),orae->Code);
		return;
	/*	ctxt->resDoc = createExceptionDoc(orae->what(),ctxt->GetQueryHandle());
		addXmlBM(*ctxt);*/
	};
	EXCEPTIONS::Exception *exp = dynamic_cast<EXCEPTIONS::Exception*>(e);
	if (exp)
	{
		ProgTrace(TRACE5,"Exception: %s",exp->what());
		return;
	};
/*	EXCEPTIONS::UserException *ue = dynamic_cast<EXCEPTIONS::UserException*>(e);
	if (ue)
	{
		ctxt->resDoc = createExceptionDoc(ue->what(),ctxt->GetQueryHandle());
		addXmlBM(*ctxt);
	}*/
	/*OciCpp::ociexception *oe = dynamic_cast<OciCpp::ociexception*>(e);
	if (oe)
	{
		ProgTrace(TRACE1, "ociexception: '%s'", oe->what());
		ctxt->resDoc = createExceptionDoc(getTextByNum(FATAL_ERR),
					ctxt->GetQueryHandle(),
					"Ошибка обращения к базе данных");
	}*/
	/*Ticketing::TickExceptions::tick_soft_except *te 
		= dynamic_cast<Ticketing::TickExceptions::tick_soft_except*>(e);
	if (te)
	{
		ProgTrace(TRACE1,"tick_soft_exception: '%s'", te->what());
		ctxt->resDoc = createExceptionDoc(getTextByNum(te->get_err()), ctxt->GetQueryHandle());
	}*/
}

