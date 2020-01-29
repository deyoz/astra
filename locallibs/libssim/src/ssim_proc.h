#pragma once

#include <nsi/nsi.h>

#include "ssim_data_types.h"
#include "ssm_data_types.h"
#include "dei_handlers.h"

namespace ssim {

namespace route_makers {

Expected<ssim::Route> createNew(const ssim::LegStuffList&, const ssim::SegmentInfoList&);

Expected<ssim::PeriodicRoutes> applyTim(
    const ssim::ScdPeriod&,
    const ssim::RoutingInfoList&,
    const ssim::SegmentInfoList&,
    const ProcContext&, bool
);

Expected<ssim::PeriodicRoutes> applyEqtCon(
    const ssim::ScdPeriod&,
    const ssim::EquipmentInfo&,
    const ssim::SegmentInfoList&,
    const boost::optional<nsi::Points>&,
    const ProcContext&, const boost::optional<PartnerInfo>&, bool
);

Expected<ssim::PeriodicRoutes> applyAdm(
    const ssim::ScdPeriod&,
    const ssim::SegmentInfoList&,
    const ProcContext&, bool
);

} //route_makers

using timeModeConvert = std::function< ssim::ScdPeriods (const ssim::ScdPeriod&) >;

Expected<ssim::ScdPeriods> makeScdPeriod(
    const ssim::FlightInfo&, const ssim::PeriodInfo&,
    const ssim::LegStuffList&, const ssim::SegmentInfoList&,
    const ProcContext&, const timeModeConvert&,
    bool check24
);

boost::optional<PartnerInfo> getPartnerInfo(AgreementType, const std::vector<const DeiInfo*>&);

std::vector<nsi::DepArrPoints> getAffectedLegs(const nsi::Points&, const ssim::Route&, bool is_ssm);

boost::gregorian::days getUtcOffset(const ssim::ScdPeriod&);
boost::gregorian::days getScdOffset(const ssim::ScdPeriod&, bool timeIsLocal);

namespace details {

ssim::ScdPeriods getSchedulesUsingTimeMode(const ct::Flight&, const Period&, const ProcContext&, bool timeIsLocal);

Expected<ssim::PeriodicCshs> getCshContextEqtCon(
    const ssim::ScdPeriod&, const ProcContext&,
    bool invertedScd, const boost::optional<PartnerInfo>&
);

} //details

Expected<ct::Flight> getActFlight(const ssim::ProcContext&, const ct::Flight&, const Period&, bool allowNotFound = false);

ssim::ScdPeriods toLocalTime(const ssim::ScdPeriod&);

Message correctLegOrders(ssim::PeriodicRoutes&, const boost::optional<ssim::details::CshContext>&);

} //ssim
