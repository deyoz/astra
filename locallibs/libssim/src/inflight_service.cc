#include <serverlib/str_utils.h>

#include "inflight_service.h"

#define NICKNAME "ASH"
#include <serverlib/slogger.h>

namespace ssim {

Expected<InflightServices> getInflightServices(const std::string& in)
{
    if (in.empty()) {
        return InflightServices();
    }

    InflightServices out;
    for (const std::string& s : StrUtils::split_string<std::vector< std::string> >(in, '/', StrUtils::KeepEmptyTokens::True)) {
        const EncString srv = EncString::from866(s);

        if (!nsi::InflService::find(srv)) {
            return Message(STDLOG, _("Unrecognized inflight service %1%")).bind(srv);
        }

        out.emplace_back(nsi::InflService(srv).id());
    }
    return out;
}

std::string toString(const InflightServices& iss)
{
    return StrUtils::join("/", iss, [] (const nsi::InflServiceId& id) {
        return nsi::InflService(id).code(ENGLISH);
    });
}

bool shouldBeTruncated(const std::set<nsi::InflServiceId>& srvs)
{
    //Users must send no more than fourteen (14) value codes
    //or no more than one (1) DEI 503 line in order to ensure data is processed,
    //stored and displayed in other computer systems.
    return srvs.size() > 14;
}

} //ssim
