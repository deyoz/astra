#ifndef _ASTRA_TICKET_H_
#define _ASTRA_TICKET_H_
#include <utility>
#include <string>
#include "etick/ticket.h"
#include "etick/tick_data.h"
#include "etick/tickmng.h"
#include "etick/ticket.h"

namespace Ticketing{
// NOTE: See eticklib for realization

typedef BaseCoupon_info Coupon_info;


// NOTE: See eticklib for realization
typedef BaseLuggage Luggage;

// NOTE: See eticklib for realization
typedef BaseFrequentPass FrequentPass;

// NOTE: See eticklib for realization
typedef BaseItin<Luggage> Itin;

// NOTE: See eticklib for realization
typedef BaseFreeTextInfo FreeTextInfo;

// NOTE: See eticklib for realization
typedef BaseCoupon<Coupon_info, Itin, FrequentPass, FreeTextInfo> Coupon;

// NOTE: See eticklib for realization
typedef BaseTicket<Coupon> Ticket;

// NOTE: See eticklib for realization
typedef BaseTaxDetails TaxDetails;

typedef BasePassenger Passenger;

typedef BaseMonetaryInfo MonetaryInfo;

typedef BaseFormOfPayment FormOfPayment;

typedef BaseResContrInfo ResContrInfo;

typedef BaseOrigOfRequest OrigOfRequest;

typedef BasePnr< OrigOfRequest,
                 ResContrInfo,
                 Passenger,
                 Ticket,
                 TaxDetails,
                 MonetaryInfo,
                 FormOfPayment,
                 FreeTextInfo>  Pnr;

typedef BasePnrListItem<OrigOfRequest,
                        ResContrInfo,
                        Passenger,
                        Ticket,
                        FormOfPayment> PnrListItem;

typedef BasePnrList <PnrListItem> PnrList;
}
#endif /*_ASTRA_TICKET_H_*/
