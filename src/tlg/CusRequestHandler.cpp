#include "CusRequestHandler.h"
#include "apis_edi_file.h"
#include "apps_interaction.h"
#include "view_edi_elements.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling
{

using namespace edilib;
using namespace edifact;


CusRequestHandler::CusRequestHandler(_EDI_REAL_MES_STRUCT_ *PMes,
                                     const edilib::EdiSessRdData *edisess)
    : AstraEdiRequestHandler(PMes, edisess)
{}

std::string CusRequestHandler::mesFuncCode() const
{
    return "";
}

bool CusRequestHandler::fullAnswer() const
{
    return true;
}

void CusRequestHandler::parse()
{
   m_data.reset(new edifact::Cusres(readCUSRES(pMes())));
}

void CusRequestHandler::handle()
{
    ASSERT(m_data);
    ProcessChinaCusres(*m_data); 
}

void CusRequestHandler::makeAnAnswer()
{
    viewBgmElement(pMesW(), BgmElem("132", ""));
     
    PushEdiPointW(pMesW());
    SetEdiSegGr(pMesW(), SegGrElement(4));
    SetEdiPointToSegGrW(pMesW(), SegGrElement(4), "SegGr4 not found");
    
    SetEdiFullSegment(pMesW(), SegmElement("ERP"), "1"); // TODO
    SetEdiFullSegment(pMesW(), SegmElement("ERC"), "1"); // TODO   
    
    PopEdiPointW(pMesW());
}

}//namespace TlgHandling
