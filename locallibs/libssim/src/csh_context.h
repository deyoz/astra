#pragma once

#include "csh_settings.h"

namespace ssim { namespace details {

struct CshContext
{
    bool inverse;   //apply opr -> mkt scd inversion
    nsi::CompanyId mkt;
    nsi::CompanyId opr;
    std::map<nsi::DepArrPoints, RbdMappings> mappings;

    CshContext(const ssim::CshSettings&, bool);
    RbdMappings getMapping(const nsi::DepArrPoints&) const;

    static boost::optional<CshContext> create(const boost::optional<ssim::CshSettings>&, bool);
};

ct::Rbds remapOprToMan(ct::Rbd, const RbdMappings&);
ct::Cabins remapOprToMan(ct::Cabin, const RbdMappings&);
boost::optional<ct::Rbd> remapManToOpr(ct::Rbd, const RbdMappings&);
boost::optional<ct::Cabin> remapManToOpr(ct::Cabin, const RbdMappings&);

} } //ssim::details
