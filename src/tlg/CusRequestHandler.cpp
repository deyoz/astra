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
    ASSERT(m_data);
    const Cusres& cusres = *m_data;

    viewBgmElement(pMesW(), BgmElem("312", ""));

    if (cusres.m_rff)
      viewRffElement( pMesW(), cusres.m_rff.get() );

    int segmGroupNum=0;
    for(const Cusres::SegGr3& sg3 : cusres.m_vSegGr3)
    {
      SetEdiSegGr( pMesW(), SegGrElement( 3, segmGroupNum ) );

      PushEdiPointW( pMesW() );
      SetEdiPointToSegGrW( pMesW(), SegGrElement( 3, segmGroupNum ) );

      viewRffElement( pMesW(), sg3.m_rff );

      viewDtmElement( pMesW(), sg3.m_dtm1, 0 );
      viewDtmElement( pMesW(), sg3.m_dtm2, 1 );
      viewLocElement( pMesW(), sg3.m_loc1, 0 );
      viewLocElement( pMesW(), sg3.m_loc2, 1 );

      PopEdiPointW( pMesW() );

      segmGroupNum++;
    }

    segmGroupNum=0;
    for(const Cusres::SegGr4& sg4 : cusres.m_vSegGr4)
    {
      SetEdiSegGr( pMesW(), SegGrElement( 4, segmGroupNum ) );

      PushEdiPointW( pMesW() );
      SetEdiPointToSegGrW( pMesW(), SegGrElement( 4, segmGroupNum ) );

      SetEdiFullSegment(pMesW(), SegmElement("ERP"), sg4.m_erp.m_msgSectionCode);

      int elemNum=0;
      for(const RffElem& rffElem : sg4.m_vRff)
        viewRffElement( pMesW(), rffElem, elemNum++);

      SetEdiFullSegment(pMesW(), SegmElement("ERC"), sg4.m_erc.m_errorCode);

      PopEdiPointW( pMesW() );

      segmGroupNum++;
    }
}

}//namespace TlgHandling
