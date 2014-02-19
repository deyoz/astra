#include "edi_elements.h"
#include "etick/tick_data.h"
namespace ASTRA
{
namespace edifact
{

std::ostream & operator<<(std::ostream &os, const TktElement &tkt)
{
    os << "TKT: ";
    os << "tnum: " << tkt.ticketNum
       << " docType: " << tkt.docType;

    os << " Nbooklets: ";
    if(tkt.nBooklets)
        os << *tkt.nBooklets;
    else
        os << "NO";

    os << " TickAct: ";
    if(tkt.tickStatAction)
        os << Ticketing::TickStatAction::TickActionStr(*tkt.tickStatAction);
    else
        os << "NO";

    os << " tnumConn: ";
    if(tkt.inConnectionTicketNum)
        os << *tkt.inConnectionTicketNum;
    else
        os << "NO";

    return os;
}

std::ostream &operator << (std::ostream &os, const CpnElement& cpn)
{
    os << "CPN: ";
    os << "num: " << cpn.num << "; ";
    if(cpn.media)
        os << cpn.media->code() << "; ";
    else
        os << "NO; ";
    os << "amount: " << (cpn.amount.isValid() ? cpn.amount.amStr() : "NO") << "; ";
    os << "status: ";
    if(cpn.status)
        os << cpn.status;
    else
        os << "NO";
    os << "; "
       << "action request:" << (!cpn.action.empty() ? cpn.action : "NO") << "; "
       << "SAC:" << (!cpn.sac.empty() ? cpn.sac : "NO") << "; ";
    return os;
}

}//namespace edifact
}//namespace ASTRA
