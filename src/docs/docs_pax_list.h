#ifndef _PAX_LIST_H_
#define _PAX_LIST_H_

#include "passenger.h"
#include "telegram.h"
#include "docs_consts.h"
#include "pers_weights.h"

namespace REPORTS {

    // ����� ���� ���⠢��� �� ����, ��।������ ᫥���騬� ��ࠬ��ࠬ�
    //
    // pax_list.options.lang - �� �뢮�� �������� ������ (���, � ����...)
    // pax_list.options.mkt_flt - �롮ઠ �� ��થ⨭������ ३��
    // pax_list.options.rem_event_type = retRPT_PM - ⨯� ६�ப. �᫨ �� �����, ६�ન �� ������
    // pax_list.options.flags.setFlag(REPORTS::oeBagAmount) - �������� ���� �� ���-�� ������, ���ਬ��

    enum TOptionsEnum {
        oeBagAmount,
        oeBagWeight,
        oeRkAmount,
        oeRkWeight,
        oeExcess,
        oeSeatNo,
        oeTags
    };

    class TPaxListFlags: public BitSet<TOptionsEnum> {};

    struct TBrdVal {
        enum Enum {bvNULL, bvNOT_NULL, bvTRUE, bvFALSE};
        Enum val;
        TBrdVal(Enum _val): val(_val) {}
    };

    struct TOptions {
        std::string lang;
        boost::optional<TSimpleMktFlight> mkt_flt;
        boost::optional<TRemEventType> rem_event_type;
        boost::optional<TBrdVal> pr_brd;
        boost::optional<bool> wait_list;
        bool not_refused;
        TSortType sort;
        TPaxListFlags flags;
        void clear()
        {
            lang = AstraLocale::LANG_RU;
            mkt_flt = boost::none;
            rem_event_type = boost::none;
            pr_brd = boost::none;
            wait_list = boost::none;
            not_refused = false;
            sort = stRegNo;
            flags.clearFlags();
        }
        TOptions() { clear(); }
    };

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
            rk_amount = 0;
            rk_weight = 0;
            amount = 0;
            weight = 0;
            excess_wt = 0;
            excess_pc = 0;
        }
        TBaggage():
            excess_wt(0),
            excess_pc(0)
        {
            clear();
        }
        bool empty()
        {
            return
                rk_amount == 0 or
                rk_weight == 0 or
                amount == 0 or
                weight == 0 or
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

        std::string full_name_view() const;

        std::string user_descr;
        TBaggage baggage;

        const std::string &cl() const;
        int rk_amount() const;
        int rk_weight(bool cbbg_weight = false) const;
        int bag_amount() const;
        int bag_weight() const;
        TBagKilos excess_wt() const;
        TBagPieces excess_pc() const;

        TTlgSeatList seat_list;
        int seats() const;
        std::string seat_no() const;
        std::string tkn_str() const;
        std::string _seat_no; // # ���� � �஡����� � ��砫�, ��� ���஢��

        std::multiset<TBagTagNumber> _tags;
        std::string get_tags() const;

        std::multiset<CheckIn::TPaxRemItem> _rems;
        virtual std::string rems() const;

        virtual void clear()
        {
            simple.clear();
            cbbg_list.clear();
            seat_list.Clear();
            _seat_no.clear();
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
        const CheckIn::TSimplePaxGrpItem &grp() const;
    };



    struct TUnboundCBBGList: public std::list<TPaxPtr> {
        // �饬 � ᯨ᪥ pax_list ��७� cbbg_pax
        // �᫨ ������, �ய��뢠�� ��७�� ��뫪� �� cbbg_pax
        // � ��⨢��� ��砥 ������塞 � ����� ᯨ᮪
        void add_cbbg(TPaxPtr _cbbg_pax);

        // �஡�� �� ������� ᯨ��
        // � pax �ய��뢠�� ��뫪� �� �������� 砩���
        // ����塞 ��������� �� ������� ᯨ᪠
        void bind_cbbg(TPaxPtr _pax);
        void trace(TRACE_SIGNATURE);
    };

    struct TPaxList: public std::list<TPaxPtr> {
        TOptions options;
        int point_id;
        TUnboundCBBGList unbound_cbbg_list; // ᯨ᮪ ���ਢ易���� CBBG ��� point_id

        boost::optional<TTlgCompLayerList> complayers;
        boost::optional<TRemGrp> rem_grp;
        boost::optional<PersWeightRules> pwr;

        std::map<int, boost::optional<CheckIn::TSimplePaxGrpItem>> grps;

        void clear();
        void trace(TRACE_SIGNATURE);
        // ���譨� �����
        void fromDB(TQuery &Qry);
        // ������� ᢮�, ����७���
        void fromDB();
        virtual TPaxPtr getPaxPtr();

        TPaxList(int _point_id);
        virtual ~TPaxList() {};
    };

    bool pax_cmp(TPaxPtr pax1, TPaxPtr pax2);
}

#endif
