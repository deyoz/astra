#include "csh_context.h"

namespace ssim { namespace details {

CshContext::CshContext(const ssim::CshSettings& cs, bool inv)
    : inverse(inv), mkt(cs.mkt), opr(cs.opr), mappings(cs.mappings)

{}

boost::optional<CshContext> CshContext::create(const boost::optional<ssim::CshSettings>& cs, bool inv)
{
    if (cs) {
        return CshContext(*cs, inv);
    }
    return boost::none;
}

RbdMappings CshContext::getMapping(const nsi::DepArrPoints& dap) const
{
    auto i = mappings.find(dap);
    if (i != mappings.end()) {
        return i->second;
    }
    return {};
}

ct::Rbds remapOprToMan(ct::Rbd r, const RbdMappings& rms)
{
    ct::Rbds out;
    for (const RbdMapping& rm :rms) {
        if (rm.opRbd == r) {
            out.push_back(rm.manRbd);
        }
    }
    return out;
}

ct::Cabins remapOprToMan(ct::Cabin c, const RbdMappings& rms)
{
    std::set<ct::Cabin> ocs;
    for (const RbdMapping& rm : rms) {
        if (rm.opCabin == c) {
            ocs.insert(rm.manCabin);
        }
    }
    return ct::Cabins(ocs.begin(), ocs.end());
}

boost::optional<ct::Rbd> remapManToOpr(ct::Rbd r, const RbdMappings& rms)
{
    for (const RbdMapping& rm : rms) {
        if (rm.manRbd == r) {
            return rm.opRbd;
        }
    }
    return boost::none;
}

boost::optional<ct::Cabin> remapManToOpr(ct::Cabin c, const RbdMappings& rms)
{
    for (const RbdMapping& rm : rms) {
        if (rm.manCabin == c) {
            return rm.opCabin;
        }
    }
    return boost::none;
}

} } //ssim::details
