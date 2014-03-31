#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef XP_TESTING
#include <queue>
#include <fstream>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <serverlib/tscript.h>
#include <serverlib/exception.h>
#include <serverlib/func_placeholders.h>
#include <serverlib/cursctl.h>
#include <serverlib/str_utils.h>

#include <jxtlib/jxtlib.h>
#include "xml_unit.h"
#include "astra_main.h"
#include "tlg/tlg.h"

#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>

void tests_init() {}

using namespace xp_testing::tscript;

static std::string executeAstraRequest(const std::string &request,
    const std::string &ignore_err_code,
    bool ignore_err)
{
    ServerFramework::QueryRunner query_runner (ServerFramework::TextQueryRunner());

    const std::string answer =
        jxtlib::JXTLib::Instance()->GetCallbacks()->Main(std::string(),
                            request, GetTestContext()->vars["JXT_PULT"], "");

    XMLDoc doc(answer);
    if(!ignore_err && GetNode("/term/answer/command/error", doc.docPtr())) {
        const std::string error = NodeAsString( "/term/answer/command/error", doc.docPtr() );
        if(ignore_err_code.empty())
            throw EXCEPTIONS::Exception("failed with code: " + error);
        else if(ignore_err_code != error) {
            throw EXCEPTIONS::Exception("expected error: " + ignore_err_code +
                            " but failed with: " + error);
        }
    }
    return answer;
}

static std::string pretty_xml(const std::string &xml)
{
    return StrUtils::replaceSubstrCopy(XMLTreeToText( TextToXMLTree(xml) ), "\"", "'");
}

static void executeRequest(
            const std::vector<tok::Param>& params,
            const std::string& req,
            std::string& reply, /* io */
            std::queue<std::string>& outq /* io */)
{
    const std::string capture = tok::Validate(tok::GetValue(params, "capture", "off"), "noformat on off");
    const std::string errStr = tok::GetValue(params, "err");
    reply = executeAstraRequest(req, errStr, errStr == "ignore");
    if (capture == "on")
        outq.push(pretty_xml(reply));
}

static std::string FP_astra_hello(const std::vector<std::string>& args)
{
    return "Hello world!";
}

static std::string LastRedisplay;

void setLastRedisplay(const std::string &redisplay)
{
    LastRedisplay = redisplay;
}
const std::string &getLastRedisplay()
{
    return LastRedisplay;
}

static std::string FP_lastRedisplay(const std::vector<std::string> &args)
{
    return getLastRedisplay();
}

static std::string FP_req(const std::vector<tok::Param>& params)
{
    std::queue<std::string>& outq = GetTestContext()->outq;
    tok::ValidateParams(params, 1, 1, "err ignore pages capture ws");
    const std::string text = tok::PositionalValues(params).at(0);

    if (text.empty()) {
        LogTrace(TRACE5) << __FUNCTION__ << ": skipping empty request";
        return std::string();
    }

    LogTrace(TRACE5) << __FUNCTION__ << ": top";
    CheckEmpty(outq);

    executeRequest(params, text, GetTestContext()->reply, outq);
    return GetTestContext()->reply;
}

static std::string FP_init_jxt_pult(const std::vector<std::string> &args)
{
    if(init_locale() < 0) {
        puts("Error retrieving site information");
        return "";
    }
    init_edifact();

    ASSERT(args.size() == 1 && args[0].length() == 6)
    GetTestContext()->vars["JXT_PULT"] = args[0];
    return "";
}

static std::vector<std::string> edifactOurrefVector()
{
    std::vector<std::string> refs;
    std::string ref;
    OciCpp::CursCtl cur = make_curs("select ourref from edisession order by last_access desc, ida desc");

    cur.
        def(ref).
        exec();
    while(!cur.fen()) {
        refs.push_back(ref);
    }
    return refs;
}

static std::string lastEdifactOurref(size_t pos)
{
    std::vector<std::string> res = edifactOurrefVector();
    return (res.empty() || (res.size() - 1) < pos) ? "" : res.at(pos);
}


/* $(last_edifact_ref) */
static std::string FP_last_edifact_ref(const std::vector<std::string>& p)
{
    if (p.size() > 0) {
        return lastEdifactOurref(atoi(p.at(0).c_str()));
    } else {
        return lastEdifactOurref(0);
    }
}

namespace xp_testing {
    namespace tscript {
        void ExecuteTlg(const std::vector<tok::Param>& params);
    }
};

static std::string FP_tlg_in(const std::vector<tok::Param>& params)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": top";
    std::queue<std::string>& outq = GetTestContext()->outq;
    CheckEmpty(outq);

    ExecuteTlg(params); /* Подаём на вход */
    return std::string();
}

FP_REGISTER("<<", FP_tlg_in);


FP_REGISTER("!!", FP_req);
FP_REGISTER("astra_hello", FP_astra_hello);
FP_REGISTER("last_edifact_ref", FP_last_edifact_ref);
FP_REGISTER("init_jxt_pult", FP_init_jxt_pult);
FP_REGISTER("lastRedisplay", FP_lastRedisplay);


#endif /* XP_TESTING */
