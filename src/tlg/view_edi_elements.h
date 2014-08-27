#ifndef _VIEW_EDI_ELEMENTS_H_
#define _VIEW_EDI_ELEMENTS_H_

#include "edi_elements.h"

#include <edilib/edi_types.h>


namespace edifact
{

/**
 * @brief makes an UNB element
*/
void viewUnbElement( _EDI_REAL_MES_STRUCT_* pMes, const UnbElem& elem );

/**
 * @brief makes an UNG element
*/
void viewUngElement( _EDI_REAL_MES_STRUCT_* pMes, const UngElem& elem );

/**
 * @brief makes an UNH element
*/
void viewUnhElement( _EDI_REAL_MES_STRUCT_* pMes, const UnhElem& elem );

/**
 * @brief makes an UNE element
*/
void viewUneElement( _EDI_REAL_MES_STRUCT_* pMes, const UneElem& elem );

/**
 * @brief makes a BGM element
*/
void viewBgmElement( _EDI_REAL_MES_STRUCT_* pMes, const BgmElem& elem );

/**
 * @brief makes a NAD element
*/
void viewNadElement( _EDI_REAL_MES_STRUCT_* pMes, const NadElem& elem, int num = 0 );

/**
 * @brief makes a COM element
*/
void viewComElement( _EDI_REAL_MES_STRUCT_* pMes, const ComElem& elem, int num = 0 );

/**
 * @brief makes a TDT element
*/
void viewTdtElement( _EDI_REAL_MES_STRUCT_* pMes, const TdtElem& elem, int num = 0 );

/**
 * @brief makes a LOC element
*/
void viewLocElement( _EDI_REAL_MES_STRUCT_* pMes, const LocElem& elem, int num = 0 );

/**
 * @brief makes a DTM element
*/
void viewDtmElement( _EDI_REAL_MES_STRUCT_* pMes, const DtmElem& elem, int num = 0 );

/**
 * @brief makes an ATT element
*/
void viewAttElement( _EDI_REAL_MES_STRUCT_* pMes, const AttElem& elem, int num = 0 );

/**
 * @brief makes a NAT element
*/
void viewNatElement( _EDI_REAL_MES_STRUCT_* pMes, const NatElem& elem, int num = 0 );

/**
 * @brief makes a RFF element
*/
void viewRffElement( _EDI_REAL_MES_STRUCT_* pMes, const RffElem& elem, int num = 0 );

/**
 * @brief makes a DOC element
*/
void viewDocElement( _EDI_REAL_MES_STRUCT_* pMes, const DocElem& elem, int num = 0 );

/**
 * @brief makes a CNT element
*/
void viewCntElement( _EDI_REAL_MES_STRUCT_* pMes, const CntElem& elem, int num = 0 );

/**
 * @brief makes a TKT element
*/
void viewTktElement( _EDI_REAL_MES_STRUCT_* pMes, const TktElem& elem );

/**
 * @brief makes a CPN element
*/
void viewCpnElement( _EDI_REAL_MES_STRUCT_* pMes, const CpnElem& elem );

/**
 * @brief makes an EQN element
*/
void viewEqnElement( _EDI_REAL_MES_STRUCT_* pMes, const EqnElem& elem );
void viewEqnElement( _EDI_REAL_MES_STRUCT_* pMes, const std::list<EqnElem>& lElem );

/**
 * @brief makes an ORG element
*/
void viewOrgElement( _EDI_REAL_MES_STRUCT_* pMes, const Ticketing::OrigOfRequest& elem );


}//namespace edifact

#endif/*_VIEW_EDI_ELEMENTS_H_*/
