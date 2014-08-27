#ifndef __READ_EDI_ELEMENTS_H_
#define __READ_EDI_ELEMENTS_H_

#include <boost/optional.hpp>
#include "edi_elements.h"

struct _EDI_REAL_MES_STRUCT_;

namespace Ticketing{
namespace TickReader{

boost::optional<edifact::TktElem> readEdiTkt(_EDI_REAL_MES_STRUCT_ *pMes);
boost::optional<edifact::CpnElem> readEdiCpn(_EDI_REAL_MES_STRUCT_ *pMes, int numCpn);

/**
  * @brief read ATI from current and zero level of the message
*/
boost::optional<TourCode_t> readEdiTourCodeCurrOr0(_EDI_REAL_MES_STRUCT_ *pMes);

/**
  * @brief read ATI from current level of the message
*/
boost::optional<TourCode_t> readEdiTourCode(_EDI_REAL_MES_STRUCT_ *pMes);


/**
  * @brief read TAI from current and zero level of the message
*/
boost::optional<TicketingAgentInfo_t> readEdiTicketAgnInfoCurrOr0(_EDI_REAL_MES_STRUCT_ *pMes);

/**
  * @brief read TAI from current level of the message
*/
boost::optional<TicketingAgentInfo_t> readEdiTicketAgnInfo(_EDI_REAL_MES_STRUCT_ *pMes);


/**
  * @brief read RCI elements.
*/
edifact::RciElements readEdiResControlInfo(_EDI_REAL_MES_STRUCT_ *pMes);

/**
  * @brief с текущей области видимости и с уровня выше
*/
edifact::RciElements readEdiResControlInfoCurrAnd0(_EDI_REAL_MES_STRUCT_ *pMes);


boost::optional<Passenger> readEdiPassenger(_EDI_REAL_MES_STRUCT_ *pMes);
Passenger readEdiPassengerStrict(_EDI_REAL_MES_STRUCT_ *pMes);

/**
  * read MON elements.
*/
edifact::MonElements readEdiMon(_EDI_REAL_MES_STRUCT_ *pMes);
edifact::MonElements readEdiMonCurrAnd0(_EDI_REAL_MES_STRUCT_ *pMes);
void readEdiMonetaryInfo(const std::list<edifact::MonElem> &mons, std::list<MonetaryInfo> &lmon);

edifact::FopElements readEdiFop(_EDI_REAL_MES_STRUCT_ *pMes);
edifact::FopElements readEdiFopCurrAnd0(_EDI_REAL_MES_STRUCT_ *pMes);

edifact::TxdElements readEdiTxd(_EDI_REAL_MES_STRUCT_ *pMes);
edifact::TxdElements readEdiTxdCurrAnd0(_EDI_REAL_MES_STRUCT_ *pMes);

edifact::IftElements readEdiIft(_EDI_REAL_MES_STRUCT_ *pMes, unsigned level
                                /*,TicketNum_t currTicket, CouponNum_t currCoupon*/);

edifact::IftElements readEdiIftCurrAnd0(_EDI_REAL_MES_STRUCT_ *pMes, unsigned level
                                        /*,TicketNum_t currTicket = TicketNum_t(),
                                        CouponNum_t currCoupon = CouponNum_t()*/);

//void readEdiTkts(std::list<edifact::TktEdifact>& tkts, _EDI_REAL_MES_STRUCT_ *pMes);
//void readEdiCpns(edifact::TktEdifact& tkt, _EDI_REAL_MES_STRUCT_ *pMes);

boost::optional<edifact::EbdElem> readEdiEbd(_EDI_REAL_MES_STRUCT_ *pMes);
boost::optional<edifact::PtkElem> readEdiPtk(_EDI_REAL_MES_STRUCT_ *pMes);
boost::optional<edifact::PtkElem> readEdiPtkCurrOr0(_EDI_REAL_MES_STRUCT_ *pMes);
boost::optional<edifact::PtsElem> readEdiPts(_EDI_REAL_MES_STRUCT_ *pMes);
boost::optional<edifact::TktElem> readEdiTkt(_EDI_REAL_MES_STRUCT_ *pMes);
boost::optional<edifact::CpnElem> readEdiCpn(_EDI_REAL_MES_STRUCT_ *pMes, int n = 0);
boost::optional<edifact::TvlElem> readEdiTvl(_EDI_REAL_MES_STRUCT_ *pMes);


} // namespace Ticketing
} // namespace TickReader

#endif /* __READ_EDI_ELEMENTS_H_ */
