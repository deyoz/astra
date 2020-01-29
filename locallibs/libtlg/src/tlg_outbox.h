#ifndef __SERVERLIB_TLG_OUTBOX_H
#define __SERVERLIB_TLG_OUTBOX_H

#include <string>
#include <vector>
#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include "tlgnum.h"
#include "hth.h"
#ifdef XP_TESTING

namespace aim { class Message; }

namespace xp_testing {

    class TlgOutbox;

    /*
     * ���ଠ�� �� ��室�饩 ⥫��ࠬ��
     */
    struct OutgoingTlg {
            enum { delim = '\'' };
    public:
        typedef boost::optional<OutgoingTlg> Optional_t;

        OutgoingTlg(const tlgnum_t& tlgNum, const std::string& text, const hth::HthInfo *hth, unsigned seqNum);

        const tlgnum_t& tlgNum() const;
        const std::string& text() const;
        
        const hth::HthInfo *hth() const { return HthInfo ? &HthInfo.get() : 0; }

        std::string type() const;
        /*
         * �᫨ afterWhat == none, �ᥣ�� �����頥� true.
         * � ��⨢��� ��砥, �����頥� true, �᫨ ⥫��ࠬ�� ��ࠢ���� ��᫥ afterWhat.
         */
        bool isAfter(const OutgoingTlg::Optional_t& afterWhat) const;

        class iterator {
            mutable std::string s;
            size_t i = 0, z = 0;
            const OutgoingTlg* p;
          public:
            iterator(const OutgoingTlg* _p);
            iterator& operator++();
            iterator operator++(int);
            const std::string& operator*() const;
            bool operator==(const iterator& x) const;
            bool operator!=(const iterator& x) const;
        };
        iterator begin() const { return this->empty() ? this->end() : iterator(this); }
        iterator end() const { return iterator(nullptr); }
        bool empty() const { return this->text().empty(); }
        unsigned size() const { return std::count(text().begin(), text().end(), delim); }
        typedef std::string value_type;
    private:
        tlgnum_t tlgNum_;
        std::string text_;
        boost::optional<hth::HthInfo> HthInfo;
        unsigned seqNum_;
    };

    /*
     * ������⥫� ��室��� ⥫��ࠬ�.
     *
     * =========================================
     * + �� ���� ������ ������� ������� ������ +
     * + �������� ����ᮬ TlgAccumulator    +
     * =========================================
     */
    class TlgOutbox {
    public:
        static TlgOutbox& getInstance();

        /* �����頥� ��᫥���� (�� �६���) ⥫��ࠬ��, �᫨ ⠪���� ���� */
        OutgoingTlg::Optional_t last() const;

        /* �� ���������� ⥫��ࠬ�� */
        const std::list<OutgoingTlg>& all() const;
        
        /*
         * �� ⥫��ࠬ�� ��᫥ ��������.
         * �᫨ afterWhat == none, १���� �������祭 १����� all(). 
         */
        std::vector<OutgoingTlg> allAfter(const OutgoingTlg::Optional_t& afterWhat) const;

        /*
         * ��������� ����� ⥫��ࠬ��.
         * �� �㭪�� ��뢠���� ����� �����, �⢥��饣� �� ࠡ��� � ��।ﬨ.
         *
         * �᫨ १������饥 ������⢮ ⥫��ࠬ� � ������⥫� �ॢ�蠥�
         * MAX_OUTBOX_SIZE, ��ࢠ� ⥫��ࠬ�� 㤠�����.
         */
        void push(const tlgnum_t&, const std::string&, const hth::HthInfo *hth);
    private:
        static const size_t MAX_OUTBOX_SIZE = 1024;

        TlgOutbox();

        std::list<OutgoingTlg> outbox_;
        unsigned seqNum_;
    };

    /*
     * ������⥫� ��室��� ⥫��ࠬ�, ᮧ������ ��᫥ ᮧ����� �������
     * ������� �����.
     */
    class TlgAccumulator {
    public:
        TlgAccumulator();

        /* ���������⭮ *this = TlgAccumulator() */
        void clear();

        std::vector<OutgoingTlg> list() const;
        
        /* �����頥� ��᫥���� (�� �६���) ⥫��ࠬ��, �᫨ ⠪���� ���� */
        OutgoingTlg::Optional_t last() const;
    private:
        OutgoingTlg::Optional_t afterWhat_;
    };

};
#endif /* #ifdef XP_TESTING */

#endif /* #ifndef __SERVERLIB_TLG_OUTBOX_H */

