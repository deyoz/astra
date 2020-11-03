#pragma once

#include <serverlib/message.h>
#include <serverlib/period.h>

#include "ssim_data_types.h"
#include "dei_default_values.h"
#include "csh_context.h"

namespace ssim { namespace details {

struct DefaultRequisites
{
    ct::Flight flt;
    Period prd;
};

class SegmentInfoCleaner
{
public:
    SegmentInfoCleaner();
    ~SegmentInfoCleaner();

    void getUnhandled(ssim::ExtraSegInfos&,  const ssim::SegmentInfoList&) const;

    static void markAsHandled(ct::DeiCode);
};

// dei handlers (only for dei stated in segment information)
class DeiHandler
{
protected:
    ct::DeiCode dei;
    const ssim::SegmentInfoList& slist;

    virtual Message applyInner(ssim::Route&, const ssim::SegmentInfo&, ssim::DefValueSetters&) const = 0;
    virtual Message resetValue(ssim::Route&, const ssim::SegmentInfo&) const;
    virtual ssim::DefValueSetters initialState(const ssim::Route&, const ProcContext&) const;
public:
    DeiHandler(int, const ssim::SegmentInfoList&);
    virtual ~DeiHandler();

    virtual bool fits(ct::DeiCode) const;
    virtual Message apply(PeriodicRoutes&, const ProcContext&, const boost::optional<DefaultRequisites>& withDefaults = boost::none) const;
};

// dei 8
class DeiHandlerTrafRestr : public DeiHandler
{
    virtual Message applyInner(ssim::Route&, const ssim::SegmentInfo&, ssim::DefValueSetters&) const override;
    virtual Message resetValue(ssim::Route&, const ssim::SegmentInfo&) const override;
public:
    DeiHandlerTrafRestr(const ssim::SegmentInfoList&);
};

// dei 10
class DeiHandlerMktLegDup : public DeiHandler
{
    virtual Message applyInner(ssim::Route&, const ssim::SegmentInfo&, ssim::DefValueSetters&) const override;
public:
    DeiHandlerMktLegDup(const ssim::SegmentInfoList&);
};

// dei 50
class DeiHandlerOprLegDup : public DeiHandler
{
    virtual Message applyInner(ssim::Route&, const ssim::SegmentInfo&, ssim::DefValueSetters&) const override;
public:
    DeiHandlerOprLegDup(const ssim::SegmentInfoList&);
};

// dei 99
class DeiHandlerDepTerm : public DeiHandler
{
    virtual Message applyInner(ssim::Route&, const ssim::SegmentInfo&, ssim::DefValueSetters&) const override;
    virtual Message resetValue(ssim::Route&, const ssim::SegmentInfo&) const override;
    virtual ssim::DefValueSetters initialState(const ssim::Route&, const ProcContext&) const override;
public:
    DeiHandlerDepTerm(const ssim::SegmentInfoList&);
};

// dei 98
class DeiHandlerArrTerm : public DeiHandler
{
    virtual Message applyInner(ssim::Route&, const ssim::SegmentInfo&, ssim::DefValueSetters&) const override;
    virtual Message resetValue(ssim::Route&, const ssim::SegmentInfo&) const override;
    virtual ssim::DefValueSetters initialState(const ssim::Route&, const ProcContext&) const override;
public:
    DeiHandlerArrTerm(const ssim::SegmentInfoList&);
};

// dei 101
class DeiHandlerSegorder : public DeiHandler
{
    const boost::optional<CshContext> cs;

    virtual Message applyInner(ssim::Route&, const ssim::SegmentInfo&, ssim::DefValueSetters&) const;
    virtual ssim::DefValueSetters initialState(const ssim::Route&, const ProcContext&) const;
    virtual Message resetValue(ssim::Route&, const ssim::SegmentInfo&) const;
public:
    DeiHandlerSegorder(const ssim::SegmentInfoList&, const boost::optional<CshContext>&);
};

// dei 503
class DeiHandlerInflightServices : public DeiHandler
{
    virtual Message applyInner(ssim::Route&, const ssim::SegmentInfo&, ssim::DefValueSetters&) const;
public:
    DeiHandlerInflightServices(const ssim::SegmentInfoList&);
};

// dei 504
class DeiHandlerSf : public DeiHandler
{
    virtual Message applyInner(ssim::Route&, const ssim::SegmentInfo&, ssim::DefValueSetters&) const;
    virtual ssim::DefValueSetters initialState(const ssim::Route&, const ProcContext&) const;
public:
    DeiHandlerSf(const ssim::SegmentInfoList&);
};

// dei 505
class DeiHandlerEt : public DeiHandler
{
    virtual Message applyInner(ssim::Route&, const ssim::SegmentInfo&, ssim::DefValueSetters&) const;
    virtual ssim::DefValueSetters initialState(const ssim::Route&, const ProcContext&) const;
public:
    DeiHandlerEt(const ssim::SegmentInfoList&);
};

// dei 109
class DeiHandlerLegMeals : public DeiHandler
{
    boost::optional<CshContext> cs;

    virtual Message applyInner(ssim::Route&, const ssim::SegmentInfo&, ssim::DefValueSetters&) const;
    virtual Message resetValue(ssim::Route&, const ssim::SegmentInfo&) const;
public:
    DeiHandlerLegMeals(const ssim::SegmentInfoList&, const boost::optional<CshContext>&);
};

// dei 111
class DeiHandlerSegMeals : public DeiHandler
{
    boost::optional<CshContext> cs;

    virtual Message applyInner(ssim::Route&, const ssim::SegmentInfo&, ssim::DefValueSetters&) const;
    virtual Message resetValue(ssim::Route&, const ssim::SegmentInfo&) const;
    virtual ssim::DefValueSetters initialState(const ssim::Route&, const ProcContext&) const;
public:
    DeiHandlerSegMeals(const ssim::SegmentInfoList&, const boost::optional<CshContext>&);
};

} } //ssim::details
