#ifndef _SALON_PAX_LIST_H_
#define _SALON_PAX_LIST_H_

#include "salons.h"

namespace SalonPaxList {

    SALONS2::TGetPassFlags tranzit_flags();
    SALONS2::TGetPassFlags curr_point_flags();

    struct TCurrPosHandler {
        int point_dep;
        int point_arv;
        std::string cls;
        std::string grp_status;

        const SALONS2::TSalonPax *pax;

        virtual void process() = 0;

        virtual ~TCurrPosHandler() {}
    };


    struct TSalonPaxList {
        private:
            void iterate_arv(
                    SALONS2::TSalonPassengers::iterator iDep,
                    SALONS2::TIntArvSalonPassengers &arvMap
                    );
            TCurrPosHandler *handler;
        public:
            SALONS2::TSalonPassengers pax_list;
            void get(const SALONS2::TGetPassFlags &flags, int point_id);
            void set_handler(TCurrPosHandler *ahandler) { handler = ahandler; }
            void iterate();
            TSalonPaxList(): handler(NULL) {}
    };

}

#endif
