#pragma once

#include <serverlib/expected.h>
#include <serverlib/period.h>

#include "ssim_enums.h"
#include "ssim_data_types.h"

namespace ssim {

struct ProcContext;
struct CachedScdPeriods;
class ParseRequisitesCollector;

struct PeriodInfo : public DeiInfo
{
    Period period;

    PeriodInfo(const Period&, const RawDeiMap& = RawDeiMap());
};
std::ostream& operator<< (std::ostream&, const PeriodInfo&);
bool operator< (const PeriodInfo&, const PeriodInfo&);
bool operator== (const PeriodInfo&, const PeriodInfo&);

using PeriodInfoList = std::vector<PeriodInfo>;

struct LegChangeInfo : public DeiInfo
{
    nsi::Points points;

    LegChangeInfo(const nsi::Points&, const RawDeiMap& = RawDeiMap());
};
std::ostream& operator << (std::ostream&, const LegChangeInfo&);

struct SkdPeriodInfo
{
    boost::gregorian::date start;
    boost::gregorian::date end;
};
std::ostream& operator << (std::ostream&, const SkdPeriodInfo&);

struct RevFlightInfo : public FlightInfo
{
    PeriodInfo pi;

    RevFlightInfo(const ct::Flight&, const PeriodInfo&, const RawDeiMap& = RawDeiMap());
};

struct SsmProtoNewStuff
{
    ssim::PeriodInfo pi;
    ssim::LegStuffList legs;
};

struct ProtoEqtStuff
{
    ssim::PeriodInfo pi;
    ssim::EquipmentInfo eqi;
    boost::optional<ssim::LegChangeInfo> legs;
};
std::ostream& operator << (std::ostream&, const ProtoEqtStuff&);

struct SsmTimStuff
{
    ssim::PeriodInfo pi;
    ssim::RoutingInfoList legs;
};
std::ostream& operator << (std::ostream&, const SsmTimStuff&);

//#############################################################################
enum Action {
    CLEAN,      // just loaded
    DELETE,     // period for deletion (SKD/CNL only)
    INSERT,     // new period for save (only NEW)
    UPDATE
};

struct MarkedScdPeriod
{
    Action act;
    ssim::ScdPeriod scd;
};

using MarkedScdPeriods = std::vector<MarkedScdPeriod>;

struct CachedScdPeriods
{
    // existing schedule periods for deletion
    std::map<ct::Flight, std::set<ssim::ScdPeriod> > forDeletion;

    MarkedScdPeriods changes;

    //periods of CNL (even if it "covered" by NEW)
    std::map<ct::Flight, Periods> cancelled;

    std::map<ct::Flight, ssim::ScdPeriods> forSaving() const;
};
std::ostream& operator<< (std::ostream&, const CachedScdPeriods&);
//#############################################################################
class SsmSubmsg
{
public:
    ssim::FlightInfo fi;

    virtual ~SsmSubmsg();

    virtual ssim::SsmActionType type() const = 0;
    virtual std::string toString() const = 0;
    virtual Periods getPeriods() const = 0;

    bool isInverted(const ssim::ProcContext&) const;
    virtual Message modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const = 0;

protected:
    SsmSubmsg(const ssim::FlightInfo&);
};

using SsmSubmsgPtr = std::shared_ptr<SsmSubmsg>;

struct SsmProtoNewSubmsg : public SsmSubmsg
{
    bool xasm;
    std::list<ssim::SsmProtoNewStuff> stuff;
    ssim::SegmentInfoList segs;

    virtual std::string toString() const override;
    virtual Periods getPeriods() const override;
protected:
    SsmProtoNewSubmsg(bool x, const ssim::FlightInfo&, const std::list<ssim::SsmProtoNewStuff>&, const ssim::SegmentInfoList&);
};

struct SsmNewSubmsg : public SsmProtoNewSubmsg
{
    virtual ssim::SsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Message modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const override;

    SsmNewSubmsg(bool x, const ssim::FlightInfo&, const std::list<ssim::SsmProtoNewStuff>&, const ssim::SegmentInfoList&);

    static SsmSubmsgPtr create(const ssim::ScdPeriod&, const Periods&, const ssim::PubOptions&, bool xasm);
};

struct SsmRplSubmsg : public SsmProtoNewSubmsg
{
    virtual ssim::SsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Message modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const override;

    SsmRplSubmsg(bool x, const ssim::FlightInfo&, const std::list<ssim::SsmProtoNewStuff>&, const ssim::SegmentInfoList&);

    static SsmSubmsgPtr create(const ssim::ScdPeriod&, const Periods&, const ssim::PubOptions&, bool xasm);
};

struct SsmSkdSubmsg : public SsmSubmsg
{
    bool xasm;
    ssim::SkdPeriodInfo period;

    virtual ssim::SsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Periods getPeriods() const override;
    virtual Message modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const override;

    SsmSkdSubmsg(bool b, const ssim::FlightInfo&, const ssim::SkdPeriodInfo&);
};

struct SsmRsdSubmsg : public SsmSubmsg
{
    ssim::SkdPeriodInfo period;

    virtual ssim::SsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Periods getPeriods() const override;
    virtual Message modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const override;

    SsmRsdSubmsg(const ssim::FlightInfo&, const ssim::SkdPeriodInfo&);
};

struct SsmCnlSubmsg : public SsmSubmsg
{
    bool xasm;
    ssim::PeriodInfoList periods;

    virtual ssim::SsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Periods getPeriods() const override;
    virtual Message modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const override;

    SsmCnlSubmsg(bool x, const ssim::FlightInfo&, const ssim::PeriodInfoList&);
};

struct SsmEqtConSubmsg : public SsmSubmsg
{
    std::list<ssim::ProtoEqtStuff> stuff;
    ssim::SegmentInfoList segs;

    virtual std::string toString() const override;
    virtual Periods getPeriods() const override;
    virtual Message modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const override;
protected:
    SsmEqtConSubmsg(const ssim::FlightInfo&, const std::list<ssim::ProtoEqtStuff>&, const ssim::SegmentInfoList&);
};

struct SsmEqtSubmsg : public SsmEqtConSubmsg
{
    virtual ssim::SsmActionType type() const override;

    SsmEqtSubmsg(const ssim::FlightInfo&, const std::list<ssim::ProtoEqtStuff>&, const ssim::SegmentInfoList&);

    static SsmSubmsgPtr create(const ssim::ScdPeriod&, const Periods&, const ssim::PubOptions&);
};

struct SsmConSubmsg : public SsmEqtConSubmsg
{
    virtual ssim::SsmActionType type() const override;

    SsmConSubmsg(const ssim::FlightInfo&, const std::list<ssim::ProtoEqtStuff>&, const ssim::SegmentInfoList&);

    static SsmSubmsgPtr create(const ssim::ScdPeriod&, const Periods&, const ssim::PubOptions&);
};

struct SsmTimSubmsg : public SsmSubmsg
{
    std::list<ssim::SsmTimStuff> stuff;
    ssim::SegmentInfoList segs;

    virtual ssim::SsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Periods getPeriods() const override;
    virtual Message modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const override;

    SsmTimSubmsg(const ssim::FlightInfo&, const std::list<ssim::SsmTimStuff>&, const ssim::SegmentInfoList&);

    static SsmSubmsgPtr create(const ssim::ScdPeriod&, const ssim::PubOptions&);
};

struct SsmRevSubmsg : public SsmSubmsg
{
    ssim::PeriodInfo flightPeriod; //period from flight string ('exist_per' in old parser)
    ssim::PeriodInfoList periods;

    virtual ssim::SsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Periods getPeriods() const override;
    virtual Message modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const override;

    SsmRevSubmsg(const ssim::RevFlightInfo&, const ssim::PeriodInfoList&);
};

struct SsmAdmSubmsg : public SsmSubmsg
{
    ssim::PeriodInfoList periods;
    ssim::SegmentInfoList segs;

    virtual ssim::SsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Periods getPeriods() const override;
    virtual Message modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const override;

    SsmAdmSubmsg(const ssim::FlightInfo&, const ssim::PeriodInfoList&, const ssim::SegmentInfoList&);
};

struct SsmFltSubmsg : public SsmSubmsg
{
    ssim::FlightInfo newFi;
    ssim::PeriodInfoList periods;
    ssim::SegmentInfoList segs;

    virtual ssim::SsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Periods getPeriods() const override;
    virtual Message modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const override;

    SsmFltSubmsg(const ssim::FlightInfo& f1, const ssim::FlightInfo& f2, const ssim::PeriodInfoList&, const ssim::SegmentInfoList&);
};

struct SsmNacSubmsg : public SsmSubmsg
{
    std::string rejectInfo;
    ssim::PeriodInfoList periods;

    virtual ssim::SsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Periods getPeriods() const override;
    virtual Message modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const override;

    SsmNacSubmsg(const ssim::FlightInfo&, const std::string&, const ssim::PeriodInfoList&);
};

struct SsmAckSubmsg : public SsmSubmsg
{
    virtual ssim::SsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Periods getPeriods() const override;
    virtual Message modify(ssim::CachedScdPeriods&, const ssim::ProcContext&) const override;

    SsmAckSubmsg();
};

struct SsmStruct
{
    ssim::MsgHead head;
    std::vector<SsmSubmsgPtr> specials; //NAC, ACK
    std::map< ct::Flight, std::vector<SsmSubmsgPtr> > submsgs;
    std::string toString() const;
};

Expected<SsmStruct> parseSsm(const std::string&, ParseRequisitesCollector*);

} //ssim
