#pragma once

#include "astra_ticket.h"
#include "tlg/EdifactRequest.h"

#include <list>
#include <string>
#include <map>

#include <edilib/edi_func_cpp.h>

namespace Ticketing
{
namespace ChangeStatus
{
    class ChngStatAnswer
    {
    public:
        typedef std::pair<std::string, unsigned> TicketCoupon_t;
        typedef std::map<TicketCoupon_t, std::string> ErrMap_t;
    private:
        std::list<Ticket> lTick;
        ErrMap_t ErrMap;
        std::pair<std::string, std::string> GlobalError; //err_code, err_str
    public:

        ChngStatAnswer(std::pair<std::string, std::string> GErr)
            :GlobalError(GErr)
        {
        }

        ChngStatAnswer(std::list<Ticket> ltick, const ErrMap_t &errm)
            :lTick(ltick), ErrMap(errm)
        {
        }
        // ��⠥�� ����� �� ⥫��ࠬ��
        static ChngStatAnswer readEdiTlg(EDI_REAL_MES_STRUCT *pMes);
        static ChngStatAnswer readEdiTlg(const std::string& tlgText);

        bool isGlobErr() const { return !GlobalError.first.empty(); }
        std::pair<std::string, std::string> globErr() const { return GlobalError; }
        const std::list<Ticket> &ltick() const { return lTick; }
        std::string err2Tick(const std::string &tnum, unsigned cpn) const;

        void Trace(int level, const char *nick, const char *file, int line) const
        {
            ProgTrace(level, nick, file, line, "Global Error :%s:%s",
                      globErr().first.c_str(),globErr().second.c_str());
            Ticket::Trace(level, nick, file, line, ltick());
            ProgTrace(level, nick, file, line, "Errors:");
            for(ErrMap_t::const_iterator i=ErrMap.begin();
                i!= ErrMap.end();i++)
            {
                ProgTrace(level, nick, file, line, "%s", (*i).second.c_str());
            }
        }
    };

    edilib::EdiSessionId_t ETChangeStatus(const OrigOfRequest &org,
                                          const std::list<Ticket> &lTick,
                                          const std::string &ediSessCtxt,
                                          const edifact::KickInfo &kickInfo,
                                          const std::string& airline,
                                          const Ticketing::FlightNum_t& flNum,
                                          const edifact::SpecBaseOurrefName_t& specBaseOurrefName,
                                          Ticketing::Itin* itin=NULL);
}
}
