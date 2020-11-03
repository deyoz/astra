#include <regex>

#ifdef HAVE_CONFIG_H
#endif

#include "tlgnum.h"
#include "hth.h"
#include "tlg_outbox.h"

#include <serverlib/slogger_nonick.h>
#ifdef XP_TESTING

namespace xp_testing {

    /***************************************************************************
     * OutgoingTlg
     **************************************************************************/

    OutgoingTlg::OutgoingTlg(const tlgnum_t& tn, const std::string& txt, 
                             const hth::HthInfo *_hth, unsigned seqNum):
        tlgNum_(tn),text_(txt), seqNum_(seqNum)
    {
        if(_hth)
            HthInfo.reset(*_hth);
    }

    const tlgnum_t& OutgoingTlg::tlgNum() const
    {
        return tlgNum_;
    }
    const std::string& OutgoingTlg::text() const
    {
        return text_;
    }

    std::string OutgoingTlg::type() const
    {
        LogTrace1<<"<\n"<<text_<<">";
        std::regex r("\\bMSG\\+\\d?:([^\"\']+)[\"\']");
        std::smatch what;
        if(std::regex_search(text_, what, r))
        {
            LogTrace1<<"     match";
            return what.str(1);
        }
        else
            return {};
    }
    bool OutgoingTlg::isAfter(const OutgoingTlg::Optional_t& afterWhat) const
    {
        return !afterWhat || seqNum_ > afterWhat->seqNum_;
    }
    
    /***************************************************************************
     * TlgOutbox
     **************************************************************************/

    OutgoingTlg::iterator::iterator(const OutgoingTlg* _p) : p(_p) { if(p) { z = p->text().find(delim); if(z != std::string::npos) z++; } }
    OutgoingTlg::iterator& OutgoingTlg::iterator::operator++()
    {
        if(z == std::string::npos)
            p = nullptr;
        if(p == nullptr)
            return *this;
        s.clear();
        auto& t = p->text();
        i += z;
        z = t.find(delim,i);
        if(z == t.npos)
            p = nullptr;
        z -= i;
        z ++;
        return *this;
    }
    OutgoingTlg::iterator OutgoingTlg::iterator::operator++(int) { auto x(*this); this->operator++(); return x; }
    const std::string& OutgoingTlg::iterator::operator*() const { if(s.empty()) s = p->text().substr(i,z); return s; }
    bool OutgoingTlg::iterator::operator==(const iterator& x) const { return (not p and not x.p) or (i==x.i and z==x.z and p==x.p); }
    bool OutgoingTlg::iterator::operator!=(const iterator& x) const { return not this->operator==(x); }

    TlgOutbox::TlgOutbox():
        seqNum_(0)
    {}

    /* static */
    TlgOutbox& TlgOutbox::getInstance()
    {
        static TlgOutbox instance;
        return instance;
    }

    OutgoingTlg::Optional_t TlgOutbox::last() const
    {
        return outbox_.empty() ? OutgoingTlg::Optional_t() : outbox_.back();
    }

    const std::list<OutgoingTlg>& TlgOutbox::all() const
    {
        return outbox_;
    }
        
    std::vector<OutgoingTlg> TlgOutbox::allAfter(const OutgoingTlg::Optional_t& afterWhat) const
    {
        std::vector<OutgoingTlg> result;
        for (std::list<OutgoingTlg>::const_iterator it = outbox_.begin(); it != outbox_.end(); ++it)
            if (it->isAfter(afterWhat))
                result.push_back(*it);
        return result;
    }

    void TlgOutbox::push(const tlgnum_t& tlgNum, const std::string& tlgText, const hth::HthInfo *hth)
    {
        outbox_.push_back(OutgoingTlg(tlgNum, tlgText, hth, seqNum_));
        ++seqNum_;
        
        if (outbox_.size() > MAX_OUTBOX_SIZE)
            outbox_.pop_front();
    }
    
    /***************************************************************************
     * TlgAccumulator
     **************************************************************************/
        
    TlgAccumulator::TlgAccumulator()
    {
        afterWhat_ = TlgOutbox::getInstance().last();
    }
    
    void TlgAccumulator::clear()
    {
        afterWhat_ = TlgOutbox::getInstance().last();
    }

    std::vector<OutgoingTlg> TlgAccumulator::list() const
    {
        return TlgOutbox::getInstance().allAfter(afterWhat_);
    }

    OutgoingTlg::Optional_t TlgAccumulator::last() const
    {
        std::vector<OutgoingTlg> tlgs = list();
        return tlgs.empty() ? OutgoingTlg::Optional_t() : tlgs.back();
    }
    
};

#endif /* XP_TESTING */

