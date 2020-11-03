#pragma once

#include <libssim/ssim_proc.h>
#include <libssim/ssim_schedule.h>

#include "ssim_utils.h"
#include "ssm_data_types.h"

namespace ssim { namespace details {

void updateCachedPeriods(
    CachedScdPeriods& ms, const ProcContext&, const bool timeIsLocal,
    const ct::Flight& flt, const Period& pr, bool xasm = false
);

class DiffHandler
{
protected:
    const ct::Flight flt;
    const Period per;
    const ProcContext& attr;

    // что делать, если не нашли расписание, к которому применяются изменения
    // для всех, кроме NEW - это ошибка, так что дефолтное поведение такое
    virtual Message handleEmpty(MarkedScdPeriods&);
    // что делать с кусками старого расписания, не накрываемыми периодом
    // общее место - оставить как есть, поэтому такое дефолтное поведение
    virtual Message handleOld(MarkedScdPeriods&, const MarkedScdPeriods&);
    // что делать с кусками, которые пересекаются с периодом
    virtual Message handleInter(MarkedScdPeriods&, const MarkedScdPeriods&) = 0;
    // что делать с кусками периода, не покрывающими найденное расписание
    // по умолчанию - ничего (для NEW: сохранить, основная задача)
    virtual Message handleNew(MarkedScdPeriods&);

public :
    virtual ~DiffHandler();
    DiffHandler(const ct::Flight&, const Period&, const ProcContext&);

    Message handle(CachedScdPeriods&, bool useTimeMode = false);
};

class DiffHandlerNew : public DiffHandler
{
    const ssim::ScdPeriod& tlgScd;

    virtual Message handleEmpty(MarkedScdPeriods&) override;
    virtual Message handleInter(MarkedScdPeriods&, const MarkedScdPeriods&) override;
    virtual Message handleNew(MarkedScdPeriods&) override;
public:
    DiffHandlerNew(const ssim::ScdPeriod&, const ProcContext&);
};

class DiffHandlerRpl : public DiffHandler
{
    const ssim::ScdPeriod& tlgScd;

    virtual Message handleEmpty(MarkedScdPeriods&) override;
    virtual Message handleInter(MarkedScdPeriods&, const MarkedScdPeriods&) override;
    virtual Message handleNew(MarkedScdPeriods&) override;
public:
    DiffHandlerRpl(const ssim::ScdPeriod&, const ProcContext&);
};

class DiffHandlerCnl : public DiffHandler
{
    virtual Message handleEmpty(MarkedScdPeriods&) override;
    virtual Message handleInter(MarkedScdPeriods&, const MarkedScdPeriods&) override;
    virtual Message handleNew(MarkedScdPeriods&) override;
public:
    bool xasm;
    DiffHandlerCnl(const ct::Flight&, const Period& p, bool x, const ProcContext&);
};

class DiffHandlerTim : public DiffHandler
{
    const ssim::SsmTimStuff stuff;
    const ssim::SegmentInfoList slist;
    bool invertedScd;

    Message handleInterInner(MarkedScdPeriods&, const MarkedScdPeriod&);
    virtual Message handleInter(MarkedScdPeriods&, const MarkedScdPeriods&) override;
public:
    DiffHandlerTim(bool inv, const ssim::SsmTimStuff&, const ssim::SegmentInfoList&, const ct::Flight&, const ProcContext&);
};

class DiffHandlerEqtCon : public DiffHandler
{
    const ssim::ProtoEqtStuff& pes;
    const ssim::SegmentInfoList& slist;
    const boost::optional<PartnerInfo> oprDis;
    bool  invertedScd;

    Message handleInterInner(MarkedScdPeriods&, const MarkedScdPeriod&);
    virtual Message handleInter(MarkedScdPeriods&, const MarkedScdPeriods&) override;
public:
    DiffHandlerEqtCon(
        bool inv, const ssim::ProtoEqtStuff&, const ssim::SegmentInfoList&,
        const boost::optional<PartnerInfo>&,
        const ct::Flight&, const ProcContext&
    );
};

class DiffHandlerFlt : public DiffHandler
{
    const ct::Flight newFlt;
    const ct::Flight oldFlt;
    const ssim::SegmentInfoList& slist;
    bool invertedScd;

    Message handleInterInner(MarkedScdPeriods&, const MarkedScdPeriod&);
    virtual Message handleInter(MarkedScdPeriods&, const MarkedScdPeriods&) override;
public:
    DiffHandlerFlt(
        bool inv,
        const ct::Flight& nf, const ct::Flight& of, const ssim::SegmentInfoList&,
        const ct::Flight& f, const Period&, const ProcContext&
    );
};

class DiffHandlerAdm : public DiffHandler
{
    bool invertedScd;
    const ssim::SegmentInfoList slist;

    Message handleInterInner(MarkedScdPeriods&, const MarkedScdPeriod&);
    virtual Message handleInter(MarkedScdPeriods&, const MarkedScdPeriods&) override;
public:
    DiffHandlerAdm(bool inv, const ssim::SegmentInfoList&, const ct::Flight&, const Period&, const ProcContext&);
};

class DiffHandlerRev : public DiffHandler
{
    boost::optional<MarkedScdPeriod> base;
    const ssim::PeriodInfoList &plist;

    virtual Message handleInter(MarkedScdPeriods&, const MarkedScdPeriods&) override;
    virtual Message handleNew(MarkedScdPeriods&) override;
public:
    DiffHandlerRev(const ssim::PeriodInfoList&, const ct::Flight&, const Period&, const ProcContext&);
};

} } //ssim::details
