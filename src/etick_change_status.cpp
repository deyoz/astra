//
// C++ Implementation: etick_change_status
//
// Description: Функции запроса на смену статуса
//
// Roman

#define NICKNAME "ROMAN"
#include "test.h"
#include "etick_change_status.h"
#include "astra_ticket.h"
#include "edilib/edi_func_cpp.h"
#include "tlg/edi_tlg.h"
#include "astra_tick_read_edi.h"
#include "exceptions.h"

namespace Ticketing
{
namespace ChangeStatus
{
    using namespace std;
    using namespace edilib;

    void ETChangeStatus(const OrigOfRequest &org, const std::list<Ticket> &lTick,
                        Ticketing::Itin* itin)
    {
        ProgTrace(TRACE2,"request for change of status from:");
        org.Trace(TRACE2);
        Ticket::Trace(TRACE2, lTick);
        if(itin)
        {
            itin->Trace(TRACE2);
        }

        ChngStatData chngData(org,lTick,itin);
        SendEdiTlgTKCREQ_ChangeStat(chngData);
    }

    ChngStatAnswer ChngStatAnswer::readEdiTlg(EDI_REAL_MES_STRUCT *pMes)
    {
        if(*GetDBFName(pMes, DataElement(4343), SegmElement("MSG"), "PROG_ERR") != '3')
        {
            string GlobErr = GetDBFName(pMes, DataElement(9321), "PROG_ERR",CompElement("C901"), SegmElement("ERC"));
            list<FreeTextInfo> lIft;
            TickReader::readEdiIFT(pMes, lIft);
            return ChngStatAnswer(pair<string, string>
                    (GlobErr, lIft.front().fullText()));
        }

        list<Ticket> lTick;
        map<string, string> errMap;
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
            int cnum = GetNumSegGr(pMes, 2); // Сколько купонов для данного билета
            for(int j=0;j<cnum;j++)
            {
                tst();
                SetEdiPointToSegGrG(pMes, SegGrElement(2, j), "PROG_ERR");
                Coupon_info ci = TickReader::MakeCouponInfo(pMes);
                lCpn.push_back(Coupon(ci));
                PopEdiPoint_wdG(pMes);
            }
            PopEdiPointG(pMes);
            lTick.push_back(Ticket(ticketnum, lCpn));
            PopEdiPoint_wdG(pMes);

            errMap[ticketnum] = GetDBFName(pMes,
                                    DataElement(9321),
                                    SegmElement("ERC"));

        }
        PopEdiPointG(pMes);
        return ChngStatAnswer(lTick, errMap);
    }
}
}
