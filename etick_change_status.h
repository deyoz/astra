//
// C++ Interface: etick_change_status
//
// Description: �㭪樨 ����� �� ᬥ�� �����
//
// Roman
//
#ifndef _ETICK_CHANGE_STATUS_H_
#define _ETICK_CHANGE_STATUS_H_
#include <list>
#include <string>
#include <map>
#include "astra_ticket.h"
#include "edilib/edi_func_cpp.h"

namespace Ticketing
{
namespace ChangeStatus
{
    class ChngStatAnswer
    {
        std::list<Ticket> lTick;
        std::map <std::string, std::string> ErrMap;
        std::pair<std::string, std::string> GlobalError; //err_code, err_str
    public:
        ChngStatAnswer(std::pair<std::string, std::string> GErr)
            :GlobalError(GErr)
        {
        }

        ChngStatAnswer(std::list<Ticket> ltick,
                       std::map <std::string, std::string> errm)
            :lTick(ltick), ErrMap(errm)
        {
        }
        // ��⠥�� ����� �� ⥫��ࠬ��
        static ChngStatAnswer readEdiTlg(EDI_REAL_MES_STRUCT *pMes);

        bool isGlobErr() const { return !GlobalError.first.empty(); }
        std::pair<std::string, std::string> globErr() const { return GlobalError; }
        const std::list<Ticket> &ltick() const { return lTick; }
        std::string err2Tick(const std::string &tnum) const
        {
            std::map <std::string, std::string>::const_iterator i;
            i=ErrMap.find(tnum);
            if(i!=ErrMap.end()){
                return (*i).second;
            } else {
                return "";
            }
        }

        void Trace(int level, const char *nick, const char *file, int line) const
        {
            ProgTrace(level, nick, file, line, "Global Error :%s:%s",
                      globErr().first.c_str(),globErr().second.c_str());
            Ticket::Trace(level, nick, file, line, ltick());
            ProgTrace(level, nick, file, line, "Errors:");
            for(std::map <std::string, std::string>::const_iterator i=ErrMap.begin();
                i!= ErrMap.end();i++)
            {
                ProgTrace(level, nick, file, line, "%s", (*i).second.c_str());
            }
        }
    };

    void ETChangeStatus(const OrigOfRequest &org, const std::list<Ticket> &lTick,
                        Ticketing::Itin* itin=NULL);
}
}
#endif /*_ETICK_CHANGE_STATUS_H_*/
