#pragma once

#include "astra_ticket.h"

#include <etick/tick_doctype.h>
#include <etick/emd_data.h>

#include <boost/optional.hpp>


namespace Ticketing {

class RfiscDescription
{
    std::string rfisc_;
    std::string description_;
public:
    RfiscDescription(const std::string& rfisc, const std::string& desc);
    const std::string& rfisc() const { return rfisc_; }
    const std::string& description() const { return description_; }
};

//-----------------------------------------------------------------------------

class EmdCoupon: public Coupon
{
    DocType                            docType_;
    TicketNum_t                        tickNum_;
    CouponNum_t                        associatedNum_;
    TicketNum_t                        associatedTickNum_;
    bool                               associated_;
    boost::optional<RfiscDescription>  rfisc_;
    boost::optional<TaxAmount::Amount> amount_;
    unsigned                           quantity_;
    bool                               consumed_;
    unsigned                           version_;

public:
    const DocType&               docType() const { return docType_;                        }
    CouponNum_t                      num() const { return CouponNum_t(couponInfo().num()); }
    const TicketNum_t&           tickNum() const { return tickNum_;                        }
    CouponNum_t            associatedNum() const { return associatedNum_;                  }
    const TicketNum_t& associatedTickNum() const { return associatedTickNum_;              }
    bool                      associated() const { return associated_;                     }

    void setRfisc(const RfiscDescription& val) { rfisc_ = val; }
    const boost::optional<RfiscDescription>& rfisc() const { return rfisc_; }

    void setAmount(const TaxAmount::Amount& am) { amount_ = am; }
    const TaxAmount::Amount* amount() const { return amount_.get_ptr(); }

    void setConsumed(bool val = true) { consumed_ = val; }
    bool consumed() const { return consumed_; }

    void setQuantity(int q) { quantity_ = q; }
    int quantity() const { return quantity_; }

    unsigned version() const { return version_; }

    static EmdCoupon makeEmdSCoupon(const CouponNum_t& num, const CouponStatus& status,
                                    const TicketNum_t& tnum, unsigned version = 1);

    static EmdCoupon makeEmdACoupon(const CouponNum_t& num, const CouponStatus& status,
                                    const TicketNum_t& tnum,
                                    const CouponNum_t& numAssoc, const TicketNum_t& tnumAssoc,
                                    bool associated, unsigned version = 1);

private:
    EmdCoupon(DocType docType, const CouponNum_t& num, const CouponStatus& status,
              const TicketNum_t& tnum, unsigned version);
};

//-----------------------------------------------------------------------------

class EmdTicket
{
    DocType                          docType_;
    TicketNum_t                      tickNum_;
    boost::optional<TicketNum_t>     tickNumConnect_;
    TickStatAction::TickStatAction_t tickActCode_;
    unsigned                         num_;
    std::list<EmdCoupon>             lCpn_;

public:
    static EmdTicket makeEmdSTicket(const TicketNum_t& tickNum, const TicketNum_t& tickNumConnect,
                                    TickStatAction::TickStatAction_t tac = TickStatAction::newtick);
    static EmdTicket makeEmdATicket(const TicketNum_t& tickNum,
                                    TickStatAction::TickStatAction_t tac = TickStatAction::newtick);

    const DocType&                      docType() const { return docType_;        }
    const TicketNum_t&                  tickNum() const { return tickNum_;        }
    boost::optional<TicketNum_t> tickNumConnect() const { return tickNumConnect_; }
    unsigned                                num() const { return num_;            }
    const std::list<EmdCoupon>&            lCpn() const { return lCpn_;           }

    void setLCpn(const std::list<EmdCoupon> &l);
    void addCpn(const EmdCoupon &cpn);

private:
    EmdTicket(DocType docType, const TicketNum_t& tickNum, TickStatAction::TickStatAction_t tac);
};

//-----------------------------------------------------------------------------

class Emd
{
    DocType                   docType_;    // Emd-A or Emd-S
    RficType                  rficType_;   // Reason for Issuance
    OrigOfRequest             org_;        // Orig of request
    ResContrInfo              rci_;        // Reclocs
    Passenger                 pass_;       // Passenger

    std::list<TaxDetails>     lTax_;       // Taxes
    std::list<MonetaryInfo>   lMon_;       // Tariffs
    std::list<FormOfPayment>  lFop_;       // Forms of payment
    std::list<FreeTextInfo>   lIft_;       // Free text
    std::list<FormOfId>       lFoid_;      // Form of id
    std::list<EmdTicket>      lTicket_;    // Tickets

    boost::optional<TourCode_t>           tourCode_;     // Tour code
    boost::optional<TicketingAgentInfo_t> agentInfo_;    // Agent info

public:
    const DocType&                  type()      const { return docType_;  }
    const RficType&                 rfic()      const { return rficType_; }
    const OrigOfRequest&            org()       const { return org_;      }
    const ResContrInfo&             rci()       const { return rci_;      }
    const Passenger&                pass()      const { return pass_;     }
    const std::list<TaxDetails>&    lTax()      const { return lTax_;     }
    const std::list<MonetaryInfo>&  lMon()      const { return lMon_;     }
    const std::list<FormOfPayment>& lFop()      const { return lFop_;     }
    const std::list<FreeTextInfo>&  lIft()      const { return lIft_;     }
    const std::list<FormOfId>&      lFoid()     const { return lFoid_;    }
    const std::list<EmdTicket>&     lTicket()   const { return lTicket_;  }
    const TicketingAgentInfo_t*     agentInfo() const { return agentInfo_.get_ptr(); }
    const TourCode_t*               tourCode()  const { return tourCode_.get_ptr();  }

    void setTourCode(const TourCode_t& tc)                { tourCode_ = tc; }
    void setAgentInfo(const TicketingAgentInfo_t& agInfo) { agentInfo_ = agInfo; }

    Emd(const DocType& docType,
        const RficType& rfic,
        const OrigOfRequest& org,
        const ResContrInfo& rci,
        const Passenger& pass,
        const std::list<TaxDetails>& lTax,
        const std::list<MonetaryInfo>& lMon,
        const std::list<FormOfPayment>& lFop,
        const std::list<FreeTextInfo> & lIft,
        const std::list<FormOfId>& lFoid,
        const std::list<EmdTicket>& lTicket);
};

//-----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, const RfiscDescription &rfisc);
std::ostream& operator<<(std::ostream& os, const EmdCoupon &cpn);
std::ostream& operator<<(std::ostream& os, const EmdTicket &tkt);
std::ostream& operator<<(std::ostream& os, const Emd &emd);

}//namespace Ticketing
