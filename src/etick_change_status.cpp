#include "etick_change_status.h"
#include "astra_ticket.h"
#include "astra_tick_read_edi.h"
#include "exceptions.h"
#include "tlg/edi_tlg.h"
#include "tlg/et_cos_request.h"
#include "tlg/postpone_edifact.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#include <serverlib/slogger.h>

namespace Ticketing
{
namespace ChangeStatus
{
    using namespace std;
    using namespace edilib;

    void ETChangeStatus(const OrigOfRequest &org,
                        const std::list<Ticket> &lTick,
                        const std::string &ediSessCtxt,
                        const edifact::KickInfo &kickInfo,
                        const std::string& airline,
                        const Ticketing::FlightNum_t& flNum,
                        Ticketing::Itin* itin)
    {
        ProgTrace(TRACE2,"request for change of status from:");
        org.Trace(TRACE2);
        LogTrace(TRACE3) << "airline: " << airline;
        LogTrace(TRACE3) << "flNum: " << flNum;
        Ticket::Trace(TRACE2, lTick);
        if(itin)
        {
            itin->Trace(TRACE2);
        }

        edifact::EtCosParams chngData(org,
                                      ediSessCtxt,
                                      kickInfo,
                                      airline,
                                      flNum,
                                      lTick,
                                      itin);

        edilib::EdiSessionId_t sessIda = edifact::SendEtCosRequest(chngData);
        // ��� ���� edifact �㦭� ��࠭��� �ਧ��� �⫮������ ��ࠡ�⪨ ⫣
        if(TReqInfo::Instance()->api_mode)
        {
            LogTrace(TRACE3) << "throw TlgToBePostponed for edi_session=" << sessIda;
            throw TlgHandling::TlgToBePostponed(sessIda);
        }
    }

    ChngStatAnswer ChngStatAnswer::readEdiTlg(EDI_REAL_MES_STRUCT *pMes)
    {
        char status_code = *GetDBFName(pMes, DataElement(4343), SegmElement("MSG"), "PROG_ERR");
        if(status_code != '3' && status_code != '6' /* particulary */)
        {
            string GlobErr = GetDBFName(pMes, DataElement(9321),
                                        "PROG_ERR",CompElement("C901"), SegmElement("ERC"));
            list<FreeTextInfo> lIft;
            TickReader::readEdiIFT(pMes, lIft);
            return ChngStatAnswer(pair<string, string>
                    (GlobErr, lIft.empty() ? "" : lIft.front().fullText()));
        }

        list<Ticket> lTick;
        ErrMap_t errMap;
        int tnum = GetNumSegGr(pMes, 1);
        if (!tnum)
            throw Exception("There are no tickets in positive answer");
        PushEdiPointG(pMes);
        for(int i = 0; i< tnum; i++)
        {
            list<Coupon> lCpn;
            SetEdiPointToSegGrG(pMes, SegGrElement(1, i), "PROG_ERR");

            string ticketnum = GetDBFName(pMes,
                                          DataElement(1004),
                                          "PROG_ERR",
                                          CompElement("C667"),
                                          SegmElement("TKT"));

            PushEdiPointG(pMes);
            int cnum = GetNumSegGr(pMes, 2); // ����쪮 �㯮��� ��� ������� �����
            for(int j=0;j<cnum;j++)
            {
                tst();
                SetEdiPointToSegGrG(pMes, SegGrElement(2, j), "PROG_ERR");
                Coupon_info ci = TickReader::MakeCouponInfo(pMes, TickStatAction::newtick);
                lCpn.push_back(Coupon(ci));

                errMap[make_pair(ticketnum, ci.num())] = GetDBFName(pMes,
                                    DataElement(9321),
                                    CompElement("C901"),
                                    SegmElement("ERC"));

                PopEdiPoint_wdG(pMes);
            }
            PopEdiPointG(pMes);
            lTick.push_back(Ticket(ticketnum, lCpn));
            errMap[make_pair(ticketnum, 0)] = GetDBFName(pMes,
                                         DataElement(9321),
                                         CompElement("C901"),
                                         SegmElement("ERC"));

            PopEdiPoint_wdG(pMes);
        }
        PopEdiPointG(pMes);
        return ChngStatAnswer(lTick, errMap);
    }

    ChngStatAnswer ChngStatAnswer::readEdiTlg(const std::string& tlgText)
    {
        int ret = ReadEdiMessage(tlgText.c_str());
        if(ret == EDI_MES_STRUCT_ERR){
          throw EXCEPTIONS::Exception("Error in message structure: %s",EdiErrGetString());
        } else if( ret == EDI_MES_NOT_FND){
          throw EXCEPTIONS::Exception("No message found in template: %s",EdiErrGetString());
        } else if( ret == EDI_MES_ERR) {
          throw EXCEPTIONS::Exception("Edifact error ");
        }

        return readEdiTlg(GetEdiMesStruct());
    }

    std::string ChngStatAnswer::err2Tick(const std::string & tnum, unsigned cpn) const
    {
        ErrMap_t::const_iterator i;
        i=ErrMap.find(make_pair(tnum,cpn));
        if(i!=ErrMap.end())
        {
            return i->second;
        }
        else
        {
            return "";
        }
    }

}
}

