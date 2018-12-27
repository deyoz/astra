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

    struct TBaggage {
        int rk_amount;
        int rk_weight;
        int amount;
        int weight;
        TBagKilos excess_wt;
        TBagPieces excess_pc;
        void clear()
        {
            rk_amount = ASTRA::NoExists;
            rk_weight = ASTRA::NoExists;
            amount = ASTRA::NoExists;
            weight = ASTRA::NoExists;
            excess_wt = ASTRA::NoExists;
            excess_pc = ASTRA::NoExists;
        }
        TBaggage():
            excess_wt(ASTRA::NoExists),
            excess_pc(ASTRA::NoExists)
        {
            clear();
        }
        bool empty()
        {
            return
                rk_amount == ASTRA::NoExists or
                rk_weight == ASTRA::NoExists or
                amount == ASTRA::NoExists or
                weight == ASTRA::NoExists or
                excess_wt.empty() or
                excess_pc.empty();
        }
        void fromDB(TQuery &Qry);
    };

    struct TPaxList;
    struct TPax {
        TPaxList &pax_list;
        CheckIn::TSimplePaxItem simple;
        TCrsSeatsBlockingList cbbg_list;

        std::string user_descr;
        TBaggage baggage;

        int rk_amount() const;
        int rk_weight() const;
        int bag_amount() const;
        int bag_weight() const;
        TBagKilos excess_wt() const;
        TBagPieces excess_pc() const;

        TTlgSeatList seat_list;
        int seats() const;
        std::string seat_no() const;
        std::string tkn_str() const;

        std::multiset<TBagTagNumber> _tags;
        std::string get_tags() const;

        std::multiset<CheckIn::TPaxRemItem> _rems;
        virtual std::string rems() const;

        virtual void clear()
        {
            simple.clear();
            cbbg_list.clear();
            seat_list.Clear();
            user_descr.clear();
            baggage.clear();
            _tags.clear();
            _rems.clear();
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
        std::string lang;
        TUnboundCBBGList unbound_cbbg_list; // список непривязанных CBBG для point_id

        boost::optional<TSimpleMktFlight> mkt_flt;
        boost::optional<TRemEventType> rem_event_type;
        boost::optional<std::vector<TTlgCompLayer>> complayers;
        boost::optional<TRemGrp> rem_grp;

        void clear();
        void trace(TRACE_SIGNATURE);
        void fromDB(TQuery &Qry);
        virtual TPaxPtr getPaxPtr();

        TPaxList(int _point_id);
        virtual ~TPaxList() {};
    };
}

#endif
