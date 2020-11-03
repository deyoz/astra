#pragma once

#include <serverlib/expected.h>
#include <serverlib/period.h>

#include <libssim/ssim_enums.h>
#include <libssim/ssim_data_types.h>

namespace ssim {

struct ProcContext;
class ParseRequisitesCollector;

struct FlightIdInfo : public ssim::DeiInfo
{
    ct::FlightDate fld;
    boost::optional<ct::FlightDate> newFld; //in FLT only
    boost::optional<nsi::Points> points;

    FlightIdInfo(const ct::FlightDate&, const boost::optional<ct::FlightDate>&, const ssim::RawDeiMap& = ssim::RawDeiMap());
};

std::ostream& operator<< (std::ostream&, const FlightIdInfo&);


//#############################################################################
class AsmSubmsg
{
public:
    FlightIdInfo fid;

    virtual ~AsmSubmsg();

    Period period() const;
    bool isInverted(const ProcContext&) const;

    virtual ssim::AsmActionType type() const = 0;
    virtual std::string toString() const = 0;

    //TODO: getAffectedScd ?
    virtual Expected<ssim::ScdPeriods> getOldScd(const ProcContext&) const;
    virtual Message modifyScd(ssim::ScdPeriods&, const ProcContext&) const = 0;

protected:
    explicit AsmSubmsg(const FlightIdInfo&);
    explicit AsmSubmsg(const ct::Flight&);   //ugly one for NAC/ACK
};

using AsmSubmsgPtr = std::shared_ptr<AsmSubmsg>;

struct AsmProtoNewSubmsg : public AsmSubmsg
{
    ssim::LegStuffList stuff;
    ssim::SegmentInfoList segs;

    virtual std::string toString() const override;
protected:
    AsmProtoNewSubmsg(const FlightIdInfo&, const ssim::LegStuffList&, const ssim::SegmentInfoList&);
};

struct AsmNewSubmsg : public AsmProtoNewSubmsg
{
    virtual ssim::AsmActionType type() const override;
    virtual Expected<ssim::ScdPeriods> getOldScd(const ProcContext&) const override;
    virtual Message modifyScd(ssim::ScdPeriods&, const ProcContext&) const override;

    AsmNewSubmsg(const FlightIdInfo&, const ssim::LegStuffList&, const ssim::SegmentInfoList&);

    static AsmSubmsgPtr create(const ssim::ScdPeriod&, const ssim::PubOptions&);
};

struct AsmRplSubmsg : public AsmProtoNewSubmsg
{
    virtual ssim::AsmActionType type() const override;
    virtual Expected<ssim::ScdPeriods> getOldScd(const ProcContext&) const override;
    virtual Message modifyScd(ssim::ScdPeriods&, const ProcContext&) const override;

    AsmRplSubmsg(const FlightIdInfo&, const ssim::LegStuffList&, const ssim::SegmentInfoList&);

    static AsmSubmsgPtr create(const ssim::ScdPeriod&, const ssim::PubOptions&);
};

struct AsmCnlSubmsg : public AsmSubmsg
{
    virtual ssim::AsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Expected<ssim::ScdPeriods> getOldScd(const ProcContext&) const override;
    virtual Message modifyScd(ssim::ScdPeriods&, const ProcContext&) const override;

    AsmCnlSubmsg(const FlightIdInfo&);
};

struct AsmRinSubmsg : public AsmSubmsg
{
    virtual ssim::AsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Message modifyScd(ssim::ScdPeriods&, const ProcContext&) const override;

    AsmRinSubmsg(const FlightIdInfo&);
};

struct AsmFltSubmsg : public AsmSubmsg
{
    ssim::SegmentInfoList segs;

    virtual ssim::AsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Message modifyScd(ssim::ScdPeriods&, const ProcContext&) const override;

    AsmFltSubmsg(const FlightIdInfo&, const ssim::SegmentInfoList&);
};

struct AsmAdmSubmsg : public AsmSubmsg
{
    ssim::SegmentInfoList segs;

    virtual ssim::AsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Message modifyScd(ssim::ScdPeriods&, const ProcContext&) const override;

    AsmAdmSubmsg(const FlightIdInfo&, const ssim::SegmentInfoList&);
};

struct AsmTimSubmsg : public AsmSubmsg
{
    ssim::RoutingInfoList legs;
    ssim::SegmentInfoList segs;

    virtual ssim::AsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Message modifyScd(ssim::ScdPeriods&, const ProcContext&) const override;

    AsmTimSubmsg(const FlightIdInfo&, const ssim::RoutingInfoList&, const ssim::SegmentInfoList&);

    static AsmSubmsgPtr create(const ssim::ScdPeriod&, const ssim::PubOptions&);
};

struct AsmRrtSubmsg : public AsmSubmsg
{
    boost::optional<ssim::EquipmentInfo> equip;
    ssim::RoutingInfoList legs;
    ssim::SegmentInfoList segs;

    virtual ssim::AsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Message modifyScd(ssim::ScdPeriods&, const ProcContext&) const override;

    AsmRrtSubmsg(const FlightIdInfo&, const boost::optional<ssim::EquipmentInfo>&, const ssim::RoutingInfoList&, const ssim::SegmentInfoList&);
};

struct AsmProtoEqtSubmsg : public AsmSubmsg
{
    ssim::EquipmentInfo equip;
    ssim::SegmentInfoList segs;

    virtual std::string toString() const override;
    virtual Message modifyScd(ssim::ScdPeriods&, const ProcContext&) const override;

protected:
    AsmProtoEqtSubmsg(const FlightIdInfo&, const ssim::EquipmentInfo&, const ssim::SegmentInfoList&);
};

struct AsmEqtSubmsg : public AsmProtoEqtSubmsg
{
    virtual ssim::AsmActionType type() const override;

    AsmEqtSubmsg(const FlightIdInfo&, const ssim::EquipmentInfo&, const ssim::SegmentInfoList&);

    static std::vector<AsmSubmsgPtr> create(const ssim::ScdPeriod&, const ssim::PubOptions&);
};

struct AsmConSubmsg : public AsmProtoEqtSubmsg
{
    virtual ssim::AsmActionType type() const override;

    AsmConSubmsg(const FlightIdInfo&, const ssim::EquipmentInfo&, const ssim::SegmentInfoList&);

    static std::vector<AsmSubmsgPtr> create(const ssim::ScdPeriod&, const ssim::PubOptions&);
};

struct AsmAckSubmsg : public AsmSubmsg
{
    virtual ssim::AsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Expected<ssim::ScdPeriods> getOldScd(const ProcContext&) const override;
    virtual Message modifyScd(ssim::ScdPeriods&, const ProcContext&) const override;

    AsmAckSubmsg();
};

struct AsmNacSubmsg : public AsmSubmsg
{
    std::string rejectInfo;

    virtual ssim::AsmActionType type() const override;
    virtual std::string toString() const override;
    virtual Expected<ssim::ScdPeriods> getOldScd(const ProcContext&) const override;
    virtual Message modifyScd(ssim::ScdPeriods&, const ProcContext&) const override;

    AsmNacSubmsg(const FlightIdInfo&, const std::string&);
};

struct AsmStruct
{
    ssim::MsgHead head;
    std::vector<AsmSubmsgPtr> specials; //NAC, ACK
    std::vector<AsmSubmsgPtr> submsgs;
    std::string toString() const;
};

Expected<AsmStruct> parseAsm(const std::string&, ssim::ParseRequisitesCollector*);

} //ssim
