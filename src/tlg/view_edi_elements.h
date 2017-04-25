#ifndef _VIEW_EDI_ELEMENTS_H_
#define _VIEW_EDI_ELEMENTS_H_

#include "edi_elements.h"
#include "iatci_types.h"

#include <edilib/edi_types.h>


namespace edifact
{

/**
 * @brief makes an UNB element
*/
void viewUnbElement(_EDI_REAL_MES_STRUCT_* pMes, const UnbElem& elem);

/**
 * @brief makes an UNG element
*/
void viewUngElement(_EDI_REAL_MES_STRUCT_* pMes, const UngElem& elem);

/**
 * @brief makes an UNH element
*/
void viewUnhElement(_EDI_REAL_MES_STRUCT_* pMes, const UnhElem& elem);

/**
 * @brief makes an UNE element
*/
void viewUneElement(_EDI_REAL_MES_STRUCT_* pMes, const UneElem& elem);

/**
 * @brief makes a BGM element
*/
void viewBgmElement(_EDI_REAL_MES_STRUCT_* pMes, const BgmElem& elem);

/**
 * @brief makes a NAD element
*/
void viewNadElement(_EDI_REAL_MES_STRUCT_* pMes, const NadElem& elem, int num = 0);

/**
 * @brief makes a COM element
*/
void viewComElement(_EDI_REAL_MES_STRUCT_* pMes, const ComElem& elem, int num = 0);

/**
 * @brief makes a TDT element
*/
void viewTdtElement(_EDI_REAL_MES_STRUCT_* pMes, const TdtElem& elem, int num = 0);

/**
 * @brief makes a LOC element
*/
void viewLocElement(_EDI_REAL_MES_STRUCT_* pMes, const LocElem& elem, int num = 0);

/**
 * @brief makes a DTM element
*/
void viewDtmElement(_EDI_REAL_MES_STRUCT_* pMes, const DtmElem& elem, int num = 0);

/**
 * @brief makes an ATT element
*/
void viewAttElement(_EDI_REAL_MES_STRUCT_* pMes, const AttElem& elem, int num = 0);

/**
 * @brief makes a MEA element
*/
void viewMeaElement(_EDI_REAL_MES_STRUCT_* pMes, const MeaElem& elem, int num = 0);

/**
 * @brief makes a NAT element
*/
void viewNatElement(_EDI_REAL_MES_STRUCT_* pMes, const NatElem& elem, int num = 0);

/**
 * @brief makes a RFF element
*/
void viewRffElement(_EDI_REAL_MES_STRUCT_* pMes, const RffElem& elem, int num = 0);

/**
 * @brief makes a DOC element
*/
void viewDocElement(_EDI_REAL_MES_STRUCT_* pMes, const DocElem& elem, int num = 0);

/**
 * @brief makes a CNT element
*/
void viewCntElement(_EDI_REAL_MES_STRUCT_* pMes, const CntElem& elem, int num = 0);

/**
 * @brief makes a TVL element
*/
void viewTvlElement(_EDI_REAL_MES_STRUCT_* pMes, const TvlElem& elem);
void viewItin(EDI_REAL_MES_STRUCT *pMes, const Ticketing::Itin &itin, int num = 0);

/**
 * @brief makes a TKT element
*/
void viewTktElement(_EDI_REAL_MES_STRUCT_* pMes, const TktElem& elem);

/**
 * @brief makes a CPN element
*/
void viewCpnElement(_EDI_REAL_MES_STRUCT_* pMes, const CpnElem& elem);

/**
 * @brief makes an EQN element
*/
void viewEqnElement(_EDI_REAL_MES_STRUCT_* pMes, const EqnElem& elem);
void viewEqnElement(_EDI_REAL_MES_STRUCT_* pMes, const std::list<EqnElem>& lElem);

/**
 * @brief makes an ORG element
*/
void viewOrgElement(_EDI_REAL_MES_STRUCT_* pMes, const Ticketing::OrigOfRequest& elem);


//-------------------------------------IACTI---------------------------------------------
/**
 * @brief makes a LOR element
*/
void viewLorElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::OriginatorDetails& origin);

/**
 * @brief makes a FDQ element
*/
void viewFdqElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::FlightDetails& nextFlight,
                    boost::optional<iatci::FlightDetails> currFlight = boost::none);

/**
 * @brief makes a PPD element
*/
void viewPpdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::PaxDetails& pax);
void viewPpdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::PaxDetails& pax,
                    const iatci::PaxDetails& infant);

/**
 * @brief makes a PSD element
*/
void viewSpdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::SelectPersonalDetails& pax);

/**
 * @brief makes a PSI element
*/
void viewPsiElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::ServiceDetails& service);

/**
 * @brief makes a PRD element
*/
void viewPrdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::ReservationDetails& reserv);

/**
 * @brief makes a PSD element
*/
void viewPsdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::SeatDetails& seat);

/**
 * @brief makes a PBD element
*/
void viewPbdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::BaggageDetails& baggage);

/**
 * @brief makes a FDR element
*/
void viewFdrElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::FlightDetails& flight,
                    const std::string& fcIndicator = "T");

/**
 * @brief makes a CHD element
*/
void viewChdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::CascadeHostDetails& cascadeDetails);

/**
 * @brief makes a RAD element
*/
void viewRadElement(_EDI_REAL_MES_STRUCT_* pMes, const std::string& respType,
                                                 const std::string& respStatus);

/**
 * @brief makes a FSD element
*/
void viewFsdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::FlightDetails& flight);
void viewFsdElement(_EDI_REAL_MES_STRUCT_* pMes, const FsdElem& elem);

/**
 * @brief makes a PFD element
*/
void viewPfdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::FlightSeatDetails& seat,
                    const boost::optional<iatci::FlightSeatDetails>& infantSeat = boost::none);
void viewPfdElement(_EDI_REAL_MES_STRUCT_* pMes, const PfdElem& elem);

/**
 * @brief makes an ERD element
*/
void viewErdElement(_EDI_REAL_MES_STRUCT_* pMes, const std::string& errLevel,
                                                 const std::string& errCode,
                                                 const std::string& errText);

/**
 * @brief makes an UPD element
*/
void viewUpdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::UpdatePaxDetails& updPax);

/**
 * @brief makes an USD element
*/
void viewUsdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::UpdateSeatDetails& updSeat);

/**
 * @brief makes an UBD element
*/
void viewUbdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::UpdateBaggageDetails& updBaggage);

/**
 * @brief make an UAP element
*/
void viewUapElement(_EDI_REAL_MES_STRUCT_* pMes,
                    const iatci::UpdateDocDetails& updDoc, const iatci::PaxDetails& pax);

/**
 * @brief make an USI element
*/
void viewUsiElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::UpdateServiceDetails& updService);

/**
 * @brief makes a SRP element
*/
void viewSrpElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::SeatRequestDetails& seatReqDetails);

/**
 * @brief makes a CBD element
*/
void viewCbdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::CabinDetails& cabinDetails, int num = 0);

/**
 * @brief makes a ROD element
*/
void viewRodElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::RowDetails& rowDetails, int num = 0);

/**
 * @brief makes a PAP element
*/
void viewPapElement(_EDI_REAL_MES_STRUCT_* pMes,
                    const iatci::DocDetails& doc, const iatci::PaxDetails& pax);


}//namespace edifact

#endif/*_VIEW_EDI_ELEMENTS_H_*/
