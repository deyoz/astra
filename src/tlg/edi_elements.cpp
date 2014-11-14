#include "edi_elements.h"
#include "etick/tick_data.h"

namespace edifact
{

RciElements RciElements::operator+( const RciElements &other ) const
{
    RciElements res = *this;
    res.m_lFoid.insert(res.m_lFoid.end(), other.m_lFoid.begin(), other.m_lFoid.end());
    res.m_lReclocs.insert(res.m_lReclocs.end(), other.m_lReclocs.begin(), other.m_lReclocs.end());

    return res;
}

//-----------------------------------------------------------------------------

MonElements MonElements::operator+( const MonElements &other ) const
{
    MonElements res = *this;
    res.m_lMon.insert(res.m_lMon.end(), other.m_lMon.begin(), other.m_lMon.end());

    return res;
}

//-----------------------------------------------------------------------------

TxdElements TxdElements::operator +(const TxdElements &other) const
{
    TxdElements res = *this;
    res.m_lTax.insert(res.m_lTax.end(), other.m_lTax.begin(), other.m_lTax.end());

    return res;
}

//-----------------------------------------------------------------------------

FopElements FopElements::operator +(const FopElements &other) const
{
    FopElements res = *this;
    res.m_lFop.insert(res.m_lFop.end(), other.m_lFop.begin(), other.m_lFop.end());

    return res;
}

//-----------------------------------------------------------------------------

IftElements IftElements::operator +(const IftElements &other) const
{
    IftElements res = *this;
    res.m_lIft.insert(res.m_lIft.end(), other.m_lIft.begin(), other.m_lIft.end());

    return res;
}

//-----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream &os, const TktElem &tkt)
{
    os << "TKT: ";
    os << "tnum: " << tkt.m_ticketNum
       << " docType: " << tkt.m_docType;

    os << " Nbooklets: ";
    if(tkt.m_nBooklets)
        os << *tkt.m_nBooklets;
    else
        os << "NO";

    os << " TickAct: ";
    if(tkt.m_tickStatAction)
        os << Ticketing::TickStatAction::TickActionStr(*tkt.m_tickStatAction);
    else
        os << "NO";

    os << " tnumConn: ";
    if(tkt.m_inConnectionTicketNum)
        os << *tkt.m_inConnectionTicketNum;
    else
        os << "NO";

    return os;
}

std::ostream& operator<<(std::ostream &os, const CpnElem& cpn)
{
    os << "CPN: ";
    os << "num: " << cpn.m_num << "; ";
    if(cpn.m_media)
        os << cpn.m_media->code() << "; ";
    else
        os << "NO; ";
    os << "amount: " << (cpn.m_amount.isValid() ? cpn.m_amount.amStr() : "NO") << "; ";
    os << "status: ";
    if(cpn.m_status)
        os << cpn.m_status;
    else
        os << "NO";
    os << "; "
       << "action request:" << (!cpn.m_action.empty() ? cpn.m_action : "NO") << "; "
       << "SAC:" << (!cpn.m_sac.empty() ? cpn.m_sac : "NO") << "; ";
    return os;
}

}//namespace edifact
