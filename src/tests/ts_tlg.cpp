#include "edilib/edi_func_cpp.h"
#include "exceptions.h"
#include "astra_utils.h"
#include "xp_testing.h"
#include "apps_interaction.h"

#include <serverlib/tscript.h>
#include <serverlib/EdiHelpManager.h>
#include <serverlib/func_placeholders.h>
#include <libtlg/telegrams.h>

#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string/replace.hpp>

#ifdef XP_TESTING
#include "tlg/edi_handler.h"
#include "tlg/typeb_handler.h"
#include "tlg/apps_handler.h"
#include "tlg/tlg.h"

#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>


namespace xp_testing {
namespace tscript {

    void ExecuteTlg(const std::vector<tok::Param>& params)
    {
        tok::ValidateParams(params, 1, 1, "frot charset h2h");
        const std::string tlg = tok::PositionalValues(params).at(0);
        const std::string frot = tok::GetValue(params, "frot");
        const std::string chset = tok::GetValue(params, "charset");
        const std::string h2h_str = boost::algorithm::replace_all_copy(
                tok::GetValue(params, "h2h"), "\\", "\r");

        if(IsEdifactText(tlg.c_str(), tlg.length()))
        {
            // edifact
            tlg_info tlgi = {};
            tlgi.id       = saveTlg("LOOPB", "LOOPB", "INA", tlg);
            tlgi.sender   = "LOOPB";
            tlgi.text     = edilib::ChangeEdiCharset(tlg, chset.empty() ? "IATA" : chset);

            if(!h2h_str.empty()) {
                hth::HthInfo hth = {};
                if(!hth::fromString(h2h_str, hth)) {
                    throw EXCEPTIONS::Exception("Invalid HtH info: " + h2h_str);
                }
                if (telegrams::callbacks()->writeHthInfo(ASTRA::make_tlgnum(tlgi.id), hth)) {
                    LogError(STDLOG) << "writeHthInfo failed: " << tlgi.id;
                }
            }
            handle_edi_tlg(tlgi);
        }
        else if(IsAPPSAnswText(tlg))
        {
            tlg_info tlgi = {};
            tlgi.id       = saveTlg("LOOPB", "LOOPB", "IAPP", tlg);
            tlgi.sender   = "LOOPB";
            tlgi.text     = "\x02" + StrUtils::replaceSubstrCopy(tlg, "\n", "\x03");

            handle_apps_tlg(tlgi);
        }
        else
        {
            // airimp
            tlg_info tlgi = {};
            tlgi.id       = loadTlg(tlg);
            tlgi.sender   = "LOOPB";
            tlgi.text     = tlg;
            parse_and_handle_tpb_tlg(tlgi);
        }
    }
} /* namespace tscript */
} /* namespace xp_testing */

#endif /* #ifdef XP_TESTING */

