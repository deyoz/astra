//
// C++ Interface: tick_reader
//
// Description:
//
//
// Author: Roman Kovalev <rom@sirena2000.ru>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _ETICK_TICK_READER_H_
#define _ETICK_TICK_READER_H_

#include <list>
#include "etick/ticket.h"

namespace Ticketing{

void nologtrace(int lv, const char* n, const char* f, int l, const char* s);

namespace TickReader{

/// @class ReaderData - ������ ����� �⥭�� �����
class ReaderData{
public:
    ReaderData()
    {
    }
    /**
     * ����� ⥪�饣� �����
     * ����� � ���
     * @return unsigned
     */
    virtual unsigned currentItem() const
    {
        return 0;
    }

    /**
     * ��⠥� ����
     * @return true or false
     */
    virtual bool isPackTickRead() const
    {
        return false;
    }

    virtual ~ReaderData() {}
};

/// @class ReaderDataItem - ������ ����� �⥭�� ��������� ��ࠧ� ����� (��� ᯨ᪠)
class ReaderDataItem : public ReaderData{
public:
    virtual bool isPackTickRead() const { return true; }
    virtual ~ReaderDataItem() {}
};

/// @class ListReaderData - ������ ����� �⥭�� ᯨ᪠ ����⮢ (����. ������� ��᪮�� ����ᥩ)
class ListReaderData : public ReaderData
{
public:
    virtual unsigned numberOfItems() const = 0;
    //virtual unsigned currentItem() const = 0;
    /**
     * ��⠥� ����
     * @return true or false
     */
    virtual bool isPackTickRead() const
    {
        return true;
    }
    virtual ReaderDataItem & operator [] (unsigned i) = 0;
    virtual ~ListReaderData(){}
};

/// @class ReaderDataPack - ������ ����� �⥭�� ᯨ᪠ ������ ��ࠧ�� �����
/// ����. �믨᪠ ����⮢ �� ��᪮�쪮 ஦ � ����� �����
class ReaderDataPack : public ReaderData
{
    unsigned CurrentItem;
    bool CommonLevel;
public:
    ReaderDataPack():
        ReaderData(),
        CurrentItem(0),
        CommonLevel(true)
    {
    }

    /**
     * virtual
     * �������� �।���������� ���-�� ��⠭��� ����⮢
     * @return count
     */
    virtual unsigned countExpectedTickets() const = 0;

    /**
     * ����� ⥪�饣� �����
     * ����� � ���
     * @return unsigned
     */
    virtual unsigned currentItem() const { return CurrentItem; }

    /**
     * ���� ����� ⥪ �����
     */
    virtual void incCurrentItem() { CurrentItem ++; }

    /**
     * ��⠥� ����
     * @return true or false
     */
    virtual bool isPackTickRead() const
    {
        return true;
    }

    /**
     * ��⠭����� �ਧ��� ��饣� �஢�� � "false"
     */
    virtual void unsetCommonLevel()
    {
        CommonLevel = false;
    }

    /**
     * ��⠥� ��騩 �஢��� ��� � ��⠫���樥� �� ���ᠦ��?
     * @return true if yes and false otherwise
     */
    virtual bool commonLevel() const
    {
        return CommonLevel;
    };

    virtual ~ReaderDataPack(){}
};

template<class OrigOfRequestT>
class BaseOrigOfRequestReader {
public:
    virtual OrigOfRequestT operator () (ReaderData &actData) const = 0;
    virtual ~BaseOrigOfRequestReader(){}
};

template<class ResContrInfoT>
class BaseResContrInfoReader {
public:
    virtual ResContrInfoT operator () (ReaderData &actData) const = 0;
    virtual ~BaseResContrInfoReader(){}
};

template<class PassengerT>
class BasePassengerReader {
public:
    virtual typename PassengerT::SharedPtr operator () (ReaderData &actData) const = 0;
    virtual ~BasePassengerReader(){}
};

template<class FormOfIdT>
class BaseFormOfIdReader
{
public:
    virtual void operator () (ReaderData &actData, std::list<FormOfIdT> &) const = 0;
    virtual ~BaseFormOfIdReader(){}
};

class BaseCommonTicketDataReader
{
public:
    virtual CommonTicketData operator () (ReaderData &actData) const = 0;
    virtual ~BaseCommonTicketDataReader(){}
};

class DummyCommonTicketDataReader : public BaseCommonTicketDataReader
{
public:
    virtual CommonTicketData operator () (ReaderData &) const
    {
        nologtrace(1,"ROMAN",__FILE__,__LINE__,"DummyCommonTicketDataReader::operator ()");
        return CommonTicketData();
    };
    virtual ~DummyCommonTicketDataReader(){}
};

template<class FrequentPassT>
class BaseFrequentPassReader {
public:
    virtual void operator () (ReaderData &actData, std::list<FrequentPassT> &) const = 0;
    virtual ~BaseFrequentPassReader(){}
};

template<class FreeTextInfoT>
class BaseFreeTextInfoReader {
public:
    virtual void operator () (ReaderData &actData, std::list<FreeTextInfoT> &) const = 0;
    virtual ~BaseFreeTextInfoReader(){}
};

template<class TaxDetailsT>
class BaseTaxDetailsReader {
public:
    virtual void operator () (ReaderData &actData, std::list<TaxDetailsT> &) const = 0;
    virtual ~BaseTaxDetailsReader () {}
};

template<class MonetaryInfoT>
class BaseMonetaryInfoReader {
public:
    virtual void operator () (ReaderData &actData, std::list<MonetaryInfoT> &) const = 0;
    virtual ~BaseMonetaryInfoReader () {}
};

template<class FormOfPaymentT>
class BaseFormOfPaymentReader
{
public:
    virtual void operator () (ReaderData &actData, std::list<FormOfPaymentT> &) const = 0;
    virtual ~BaseFormOfPaymentReader () {}
};

template<
        class CouponT,
        class FrequentPassT
                >
class BaseCouponReader
{
public:
    virtual const BaseFrequentPassReader<FrequentPassT> &frequentPassRead () const = 0;
    virtual void operator () (ReaderData &actData, std::list<CouponT> &) const = 0;
    virtual ~BaseCouponReader() {}
};

template<class TicketT,
         class CouponReaderT>
class BaseTicketReader {
public:
    virtual void operator () (ReaderData &actData, std::list<TicketT> &,
                                const CouponReaderT &) const = 0;
    virtual const CouponReaderT &couponReader() const = 0;
    virtual ~BaseTicketReader () {}
};


template <
        class OrigOfRequestT,
        class ResContrInfoT,
        class FormOfPaymentT,
        class TicketT,
        class CouponT,
        class CouponReaderT,
        class PassengerT,
        class FrequentPassT,
        class MonetaryInfoT
        >
class CommonPnrReader
{
public:
    virtual const BaseOrigOfRequestReader<OrigOfRequestT> &origOfReqRead () const = 0;
    virtual const BaseResContrInfoReader<ResContrInfoT> &resContrInfoRead () const = 0;
    virtual const BaseFormOfPaymentReader<FormOfPaymentT> &formOfPaymentRead () const = 0;
    virtual const BaseTicketReader<TicketT, CouponReaderT> &ticketRead() const = 0;
    virtual const BaseCouponReader<CouponT, FrequentPassT> &couponRead() const = 0;
    virtual const BasePassengerReader<PassengerT> &passengerRead () const = 0;
    virtual ~CommonPnrReader(){}
};

template <
        class OrigOfRequestT,
        class ResContrInfoT,
        class FormOfPaymentT,
        class TicketT,
        class CouponT,
        class CouponReaderT,
        class PassengerT,
        class FormOfIdT,
        class FrequentPassT,
        class FreeTextInfoT,
        class TaxDetailsT,
        class MonetaryInfoT,
        class PnrT
                >
class BasePnrReader : public CommonPnrReader<
                                OrigOfRequestT,
                                ResContrInfoT,
                                FormOfPaymentT,
                                TicketT,
                                CouponT,
                                CouponReaderT,
                                PassengerT,
                                FrequentPassT,
                                MonetaryInfoT>
{
    DummyCommonTicketDataReader DummyCTicketDataReader;
public:
    virtual const BaseTaxDetailsReader<TaxDetailsT> &taxDetailsRead () const = 0;
    virtual const BaseMonetaryInfoReader<MonetaryInfoT> &monetaryInfoRead () const = 0;
    virtual const BaseFreeTextInfoReader<FreeTextInfoT> &freeTextInfoRead () const = 0;
    virtual const BaseFormOfIdReader<FormOfIdT> &formOfIdRead () const = 0;
    virtual const BaseCommonTicketDataReader &commonTicketDataRead () const { return DummyCTicketDataReader; };
    virtual ReaderData &readData() const = 0;
    virtual void checkData(PnrT &) const{}
    virtual void checkData(std::list<PnrT> &) const{}

//  virtual void operator () (const Pnr &pnr) = 0;
    virtual ~BasePnrReader(){};
};

template <
        class OrigOfRequestT,
        class ResContrInfoT,
        class FormOfPaymentT,
        class TicketT,
        class CouponT,
        class CouponReaderT,
        class PassengerT,
        class FrequentPassT,
        class MonetaryInfoT
                >
class BasePnrListReader : public CommonPnrReader<
                                OrigOfRequestT,
                                ResContrInfoT,
                                FormOfPaymentT,
                                TicketT,
                                CouponT,
                                CouponReaderT,
                                PassengerT,
                                FrequentPassT,
                                MonetaryInfoT>
{
public:
    virtual ReaderData &readData(unsigned) const = 0;
    virtual ListReaderData &readData() const = 0;
    virtual ~BasePnrListReader() {}
};

}
}
#endif //_ETICK_TICK_READER_H_
