#include "SalonPaxList.h"

using namespace std;
using namespace ASTRA;
using namespace SALONS2;
using namespace EXCEPTIONS;

namespace SalonPaxList {

    TGetPassFlags tranzit_flags()
    {
        TGetPassFlags flags;
        flags.setFlag( gpWaitList );
        flags.setFlag( gpTranzits );
        flags.setFlag( gpInfants );
        return flags;
    }

    TGetPassFlags curr_point_flags()
    {
        TGetPassFlags flags;
        flags.setFlag( gpPassenger );
        flags.setFlag( gpWaitList );
        flags.setFlag( gpInfants );
        return flags;
    }

    void TSalonPaxList::get(const TGetPassFlags &flags, int point_id)
    {
        TSalonList salonList;
        salonList.ReadFlight( TFilterRoutesSets( point_id, ASTRA::NoExists ),
                              "", NoExists );
        pax_list.clear();
        salonList.getPassengers( pax_list, flags );
    }

    void TSalonPaxList::iterate_arv(
            TSalonPassengers::iterator iDep,
            TIntArvSalonPassengers &arvMap
            )
    {
        for(TIntArvSalonPassengers::const_iterator
                iArv = arvMap.begin();
                iArv != arvMap.end();
                iArv++) {
            for(TIntClassSalonPassengers::const_iterator
                    iCls = iArv->second.begin();
                    iCls != iArv->second.end();
                    iCls++) {
                for(TIntStatusSalonPassengers::const_iterator
                        iStatus = iCls->second.begin();
                        iStatus != iCls->second.end();
                        iStatus++) {
                    for(set<TSalonPax,ComparePassenger>::const_iterator
                            iPax = iStatus->second.begin();
                            iPax != iStatus->second.end();
                            iPax++) {
                        if(not handler)
                            throw Exception("TCurrPosHandler is null");
                        handler->point_dep = iDep->first;
                        handler->point_arv = iArv->first;
                        handler->cls = iCls->first;
                        handler->grp_status = iStatus->first;
                        handler->pax = &(*iPax);
                        handler->process();
                        /*
                        TPaxMapCoord pax_map_coord;
                        pax_map_coord.point_dep = iDep->first;
                        pax_map_coord.point_arv = iArv->first;
                        pax_map_coord.cls = iCls->first;
                        pax_map_coord.grp_status = iStatus->first;
                        (*append_evt)(pax_map_coord, *this, *iPax, routeIdx, routeItem);
                        */
                    }
                }
            }
        }
    }

    void TSalonPaxList::iterate()
    {
        for(TSalonPassengers::iterator
                iDep = pax_list.begin();
                iDep != pax_list.end();
                iDep++) {
            iterate_arv(iDep, iDep->second);
            iterate_arv(iDep, iDep->second.infants);
        }
    }

}
