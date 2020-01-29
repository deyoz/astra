#pragma once

#include <serverlib/expected.h>
#include <typeb/tb_elements.h>

#include "ssim_data_types.h"

namespace typeb_parser {
class TypeBMessage;
}

namespace ssim {

class ParseRequisitesCollector;

namespace details {

Message fillHead(const typeb_parser::TypeBMessage&, ssim::MsgHead&);

Expected<ssim::FlightInfo> makeInfoFromFlight(const typeb_parser::TbElement&);
Expected<ssim::SegmentInfo> makeInfoFromSeg(const typeb_parser::TbElement&, bool nil_value_allowed);
Expected<ssim::RoutingInfo> makeInfoFromSsmRouting(const typeb_parser::TbElement&);
Expected<ssim::RoutingInfo> makeInfoFromAsmRouting(const typeb_parser::TbElement&);
Expected<ssim::EquipmentInfo> makeInfoFromEquip(const typeb_parser::TbElement&);

bool prepareRawDeiMap(ssim::RawDeiMap&, const std::vector<std::string>&);
Expected<nsi::Points> parseLegChange(const std::string&, unsigned line);

using supported_dei_func = bool (*)(ct::DeiCode);
bool isSegDeiAllowed_EqtCon(ct::DeiCode);
bool isSegDeiAllowed_Flt(ct::DeiCode);
bool isSegDeiAllowed_Tim(ct::DeiCode);

Message separateSegsAndSuppl(
    const typeb_parser::TbGroupElement&, size_t, ssim::SegmentInfoList&,
    bool nil_value_allowed = false, supported_dei_func = nullptr
);

template <typename I>
Expected< std::vector<I> > transformList(const typeb_parser::TbListElement& lst, Expected<I>(*f)(const typeb_parser::TbElement&))
{
    std::vector<I> ret;
    for (auto it = lst.begin(); it != lst.end(); ++it) {
        const Expected<I> info = f(*(*it));
        if (!info.valid()) {
            return info.err();
        }
        ret.push_back(*info);
    }
    return ret;
}

void reportParsedFlight(const ct::Flight&, ssim::ParseRequisitesCollector*);
void reportParsedPeriod(const ct::Flight&, const Period&, ssim::ParseRequisitesCollector*);

} } //ssim::details
