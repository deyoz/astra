/*
*  C++ Implementation: edi_request
*
* Description: Makes an EDIFACT request structure
*
*
* Author: Komtech-N <rom@sirena2000.ru>, (C) 2007
*
*/
#include <string.h>
#include <vector>
#include "edilib/edi_request.h"
#include "edilib/edi_func_cpp.h"
#include "edilib/edi_except.h"

#define NICKNAME "ROMAN"
#define NICK_TRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace edilib
{

EdifactRequest::EdifactRequest ( edi_msg_types_t msg_type )
        :mhead ( 0 ),pEType ( 0 ),EdiSess ( 0 ), WaitForAnswer(true)
{
    mhead = new edi_mes_head;
    memset ( mhead,0, sizeof ( edi_mes_head ) );
    mhead->msg_type = msg_type;

    pEType =  GetEdiMsgTypeStrByType_ ( GetEdiTemplateMessages(), mhead->msg_type ) ;
    if ( !pEType )
    {
        LogError ( STDLOG ) << "No types defined for type " << mhead->msg_type;
        throw edilib::EdiExcept ( std::string ( "No types defined for type " ) +
                                    boost::lexical_cast<std::string> ( mhead->msg_type ) );
    }

    mhead->msg_type_req = pEType->query_type;
    mhead->answer_type  = pEType->answer_type;
    mhead->msg_type_str = pEType;
    strcpy ( mhead->code,   pEType->code );
}

EdifactRequest::~EdifactRequest()
{
    delete mhead;
    delete EdiSess;
}

EDI_REAL_MES_STRUCT * EdifactRequest::pMes()
{
    return GetEdiMesStructW();
}

std::string EdifactRequest::makeEdifactText()
{
    if(!mesFuncCode().empty())
    {
        drawMsgFuncCode();
    }

    return edilib::WriteEdiMessage ( pMes() );
}

static inline void check_edi_session_for_null ( EdiSessWrData *sess )
{
    if ( !sess )
    {
        EdiError ( STDLOG, "Edifact session pointer is null!" );
        throw EdiExcept ( "Edifact session pointer is null" );
    }
}

const EdiSessWrData * EdifactRequest::ediSess() const
{
    check_edi_session_for_null ( EdiSess );
    return EdiSess;
}

EdiSessWrData * EdifactRequest::ediSess()
{
    check_edi_session_for_null ( EdiSess );
    return EdiSess;
}

edilib::EdiSessionId_t EdifactRequest::ediSessId() const
{
    check_edi_session_for_null ( EdiSess );
    return EdiSess->ediSession()->ida();
}

std::string EdifactRequest::unReference() const
{
    check_edi_session_for_null ( EdiSess );
    return EdiSess->ediSession()->ourCarf();
}

void EdifactRequest::drawMsgFuncCode()
{
    PushEdiPointW(pMes());
    ResetEdiPointW(pMes());
    std::string msg = mesTypeOfService() + ":" + mesFuncCode() + "::";
    std::vector<std::string> subfuncs = mesFuncSubcode();
    for(size_t i = 0; i < subfuncs.size(); i++) {
        msg += subfuncs[i];
        msg += ":";
    }
    edilib::SetEdiFullSegment(pMes(), edilib::SegmElement("MSG"), msg);
    PopEdiPointW(pMes());
}

void EdifactRequest::setEdiSessMesAttr()
{
    ediSess()->SetEdiSessMesAttr();
    if (CreateMesByHead(msgHead()))
    {
        throw edilib::EdiExcept ( "Failed to create message" );
    }
}

void EdifactRequest::setEdiSessionController ( EdiSessWrData * EdiSess_ )
{
    EdiSess = EdiSess_;
}

} // namespace edilib

