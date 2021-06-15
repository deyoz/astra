#pragma once
#include "xml_unit.h"
#include <string>
#include "astra_utils.h"
#include "seats_utils.h"
#include "astra_consts.h"

namespace RESEAT
{

enum EnumSitDown { stSeat, stReseat, stDropseat };
void SitDownPlease( xmlNodePtr reqNode,
                    xmlNodePtr externalSysResNode,
                    xmlNodePtr resNode,
                    EnumSitDown seat_type, //высадка, пересадка, назначение места
                    const std::string& whence,
                    const std::optional<int>& time_limit = std::nullopt );

void ChangeLayer( int point_id, int pax_id, int& tid,
                  const TSeat& seat,
                  const std::optional<ASTRA::TCompLayerType>& layer_type,
                  const std::string& whence,
                  const std::optional<int>& time_limit = std::nullopt );

void FreeUpSeatsSyncCabinClass( int point_id, int pax_id, int& tid,
                                const std::string& whence ); //высадить
void Reseat( int point_id, int pax_id, int& tid,
             const TSeat& seat,
             const std::string& whence );
void AfterRefreshEMD(xmlNodePtr reqNode, xmlNodePtr resNode);
bool isSitDownPlease( int point_id, const std::string& whence );
} //end namespace RESEAT
