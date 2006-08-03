#include "etick.h"
#include <string>
#define NICKNAME "VLAD"
#include "test.h"
#include "xml_unit.h"
#include "tlg/edi_tlg.h"
#include "edilib/edi_func_cpp.h"
#include "astra_ticket.h"
#include "cont_tools.h"

#include "astra_tick_view_xml.h"
#include "astra_tick_read_edi.h"

using namespace std;
using namespace Ticketing;
using namespace edilib;
using namespace Ticketing::TickReader;
using namespace Ticketing::TickView;
using namespace Ticketing::TickMng;


void ETSearchInterface::SearchETByTickNo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  string tick_no=NodeAsString("TickNoEdit",reqNode);

  TickDispByNum tickDisp(OrigOfRequest("1H", "MOW", "12345678",
                         "01MOW","MOW",'N',
                         ctxt->pult,
                         ctxt->opr.substr(ctxt->opr.size()-4, ctxt->opr.size())), tick_no);
  SendEdiTlgTKCREQ_Disp( tickDisp );
};

void ETSearchInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};

void ETSearchInterface::KickHandler(XMLRequestCtxt *ctxt,
                                    xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ProgTrace(TRACE3,"KickHandler ...");
    const char *edi_tlg = readSysContext("TlgSource");
    if(!edi_tlg)
    {
        // Ошибку на терминал
        throw EXCEPTIONS::Exception("СЭБ оветил отказом");
    }
    int ret = ReadEdiMessage(edi_tlg);
    if(ret == EDI_MES_STRUCT_ERR){
        ProgError(STDLOG,"Error in message structure: %s",EdiErrGetString());
        throw EXCEPTIONS::Exception("Ошибка в ответе от СЭБ");
    } else if( ret == EDI_MES_NOT_FND){
        ProgError(STDLOG,"No message found in template: %s",
                  EdiErrGetString());
        throw  EXCEPTIONS::Exception("Неизвестный ответ от СЭБ");
    } else if( ret == EDI_MES_ERR) {
        ProgError(STDLOG, "Ошибка в программе");
        throw EXCEPTIONS::Exception("Ошибка в программе");
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
        throw EXCEPTIONS::Exception("Неизвестная ошибка от СЭБ");
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
            ProgError(STDLOG, "edilib: %s", e.what());
            throw EXCEPTIONS::Exception("Ошибка обработки ответа от СЭБ");
        }
    } else {
        throw EXCEPTIONS::Exception("Просмотр списка ЭБ не поддерживается");
    }
}

//!!!
void ETSearchInterface::ChangeETStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  string tick_no=NodeAsString("tick_num",reqNode);
  int coupon_num=NodeAsInteger("coupon_num",reqNode);
  string coupon_status=NodeAsString("coupon_status",reqNode);
  ProgTrace( TRACE5, "tick_no=%s, coupon_num=%d, coupon_status=%s", tick_no.c_str(), coupon_num, coupon_status.c_str() );
  
}

