#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <fstream>
#include <iostream>
#include <regex>

#include <serverlib/query_runner.h>
#include <serverlib/a_ya_a_z_0_9.h>
#include <serverlib/lngv_user.h>

#include "types.h"
#include "telegrams.h"
#include "tlg_loader.h"

#define NICKNAME "DAG"
#include <serverlib/slogger.h>

namespace telegrams {

static Expected<tlgnum_t> pushToQueue(const std::string& tlg, int queue)
{
    if (tlg.empty()) {
        return Message(STDLOG, _("Telegram is empty."));
    }

    const int handlerQueue(queue == 0 ? telegrams::callbacks()->defaultHandler() : queue);

    INCOMING_INFO ii = {};
    ii.q_num = handlerQueue;
    ii.router = 0;
    ii.ttl = 0;
    ii.isEdifact = false;

    auto tlgResult = telegrams::callbacks()->writeTlg(&ii, tlg.c_str(), false);
    if (!tlgResult) {
        return Message(STDLOG, _("Put telegram to input queue failed: internal error."));
    }
    telegrams::callbacks()->registerHandlerHook(handlerQueue);
    return tlgResult->tlgNum;
}

std::vector<int> readQueueNumbers(int argc, char *argv[], int offset)
{
    std::vector<int> queues;
    for (int i = offset; i < argc; ++i) {
        const int n(atoi(argv[i]));
        if (n > 0) {
            queues.push_back(n);
        }
    }
    return queues;
}

class SsmFlightExtractor
{
    bool is_ssm_;
    std::string flight_;
public:
    SsmFlightExtractor() : is_ssm_(false) {}

    void processLine(const std::string& line)
    {
        if (!is_ssm_) {
            is_ssm_ = (line.substr(0, 3) == "SSM");
        } else if (flight_.empty()) {
            static std::regex rx(
                "^([" ZERO_NINE A_Z A_YA "]{2}[" A_Z A_YA "]?[" ZERO_NINE "]{3,4}[" A_Z A_YA "]?)"
            );

            std::smatch m;
            if (std::regex_search(line, m, rx, std::regex_constants::match_any)) {
                flight_ = m.str(1);
            }
        }
    }

    std::string getFlight() const {
        return flight_;
    }

    void reset() {
        is_ssm_ = false;
        flight_.clear();
    }
};

int tlgLoader(const std::string& fileName, const std::vector<int>& queues)
{
    std::ifstream in(fileName);
    if (!in.good()) {
        LogError(STDLOG) << "can't open input file " << fileName;
        std::cout << "Error, check nosir.log" << std::endl;
        return -1;
    }

    std::string line, currTlg;
    size_t msgCount = 0, errProcCount = 0;

    SsmFlightExtractor se;

    bool good = true;
    do {
        good = std::getline(in, line).good();
        if (good && !line.empty() && line.find_first_not_of("\r\n\t\v ") != std::string::npos) {
            currTlg += line + "\n";
            se.processLine(line);
            continue;
        }
        if (!currTlg.empty()) {
            int queue(0);
            if (!queues.empty()) {
                queue = queues[msgCount % queues.size()];
            }

            LogTrace(TRACE5) << "incoming tlg #" << msgCount << " to queue " << queue;
            LogTrace(TRACE5) << "[" << currTlg << "]";

            Expected<tlgnum_t> tlgNum = pushToQueue(currTlg, queue);
            if (!tlgNum) {
                LogError(STDLOG) << tlgNum.err().toString(UserLanguage::en_US());;
                ++errProcCount;
            }

            if (!se.getFlight().empty()) {
                LogTrace(TRACE0) << "Message inserted as " << tlgNum << "; flight: " << se.getFlight();
            }

            currTlg.erase();
            se.reset();
            ++msgCount;
            ServerFramework::applicationCallbacks()->commit_db();
        }
    } while (good);

    LogTrace(TRACE0) << "Finished, errCount=" << errProcCount;
    std::cout << "Loading finished, proc err = " << errProcCount << " / " << msgCount << " total" << std::endl;
    return 0;
}

} //telegrams
