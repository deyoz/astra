#include <serverlib/tscript.h>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "edilib/edi_func_cpp.h"
#include "exceptions.h"
#include "xp_testing.h"
#include "serverlib/EdiHelpManager.h"
#include <serverlib/func_placeholders.h>
#ifdef XP_TESTING
#include "tlg/edi_handler.h"
#include "tlg/tlg.h"

#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>


namespace xp_testing {
namespace tscript {

    void ExecuteTlg(const std::vector<tok::Param>& params)
    {
        tok::ValidateParams(params, 1, 1, "frot h2h");
        const std::string tlg = tok::PositionalValues(params).at(0);
        const std::string frot = tok::GetValue(params, "frot");
        const std::string h2h_str = boost::algorithm::replace_all_copy(
                tok::GetValue(params, "h2h"), "\\", "\r");

        if(IsEdifactText(tlg.c_str(), tlg.length())) {

            int tlg_num = saveTlg("LOOPB", "LOOPB", "OUTA", tlg);

            tlg_info tlgi = {};
            tlgi.id = tlg_num;
            tlgi.sender = "LOOPB";
            tlgi.text = edilib::ChangeEdiCharset(tlg, "IATA");
            handle_edi_tlg(tlgi);
        } else {
            throw EXCEPTIONS::Exception("Unsopported tlg type");
        }
    }

}} /* namespace xp_testing::tscript */

#endif /* #ifdef XP_TESTING */

