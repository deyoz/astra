#ifndef _PAX_LIST_H_
#define _PAX_LIST_H_

#include "passenger.h"
#include "telegram.h"

namespace REPORTS {

    struct TPax;
    typedef std::shared_ptr<TPax> TPaxPtr;

    struct TCrsSeatsBlockingItem {
        int seat_id;
        std::string surname;
        std::string name;
        int pax_id;
        TPaxPtr pax_info;

        void clear()
        {
            seat_id = ASTRA::NoExists;
            surname.clear();
            name.clear();
            pax_id = ASTRA::NoExists;
            pax_info.reset();
        }
        void fromDB(TQuery &Qry);

        TCrsSeatsBlockingItem() { clear(); }
    };

    struct TCrsSeatsBlockingList: public std::list<TCrsSeatsBlockingItem> {
        int pax_id;

        void clear()
        {
            std::list<TCrsSeatsBlockingItem>::clear();
            pax_id = ASTRA::NoExists;
        }

        TCrsSeatsBlockingList() { clear(); }

        void fromDB(int _pax_id);

    };

    struct TPaxList;
    struct TPax {
        TPaxList &pax_list;
        CheckIn::TSimplePaxItem simple;
        TCrsSeatsBlockingList cbbg_list;

        TTlgSeatList seat_list;

        virtual void clear()
        {
            simple.clear();
            cbbg_list.clear();
            seat_list.Clear();
        }

        TPax(TPaxList &_pax_list): pax_list(_pax_list)
        {
            clear();
        }

        virtual void trace(TRACE_SIGNATURE);
        virtual void fromDB(TQuery &Qry);
        virtual ~TPax() {}
    };



    struct TUnboundCBBGList: public std::list<TPaxPtr> {
        // Ищем в списке pax_list парента cbbg_pax
        // Если найден, прописываем паренту ссылку на cbbg_pax
        // в противном случае добавляем в данный список
        void add_cbbg(TPaxPtr _cbbg_pax);

        // Пробег по данному списку
        // В pax прописываем ссылки на найденные чайлды
        // Удаляем найденных из данного списка
        void bind_cbbg(TPaxPtr _pax);
        void trace(TRACE_SIGNATURE);
    };

    struct TPaxList: public std::list<TPaxPtr> {
        int point_id;
        TUnboundCBBGList unbound_cbbg_list; // список непривязанных CBBG для point_id

        boost::optional<TRemEventType> rem_event_type;
        boost::optional<std::vector<TTlgCompLayer>> complayers;
        boost::optional<TRemGrp> rem_grp;

        void clear();
        void trace(TRACE_SIGNATURE);
        void fromDB(TQuery &Qry);
        virtual TPaxPtr getPaxPtr();

        TPaxList(int _point_id, boost::optional<TRemEventType> _rem_event_type = boost::none);
        virtual ~TPaxList() {};
    };
}

#endif
