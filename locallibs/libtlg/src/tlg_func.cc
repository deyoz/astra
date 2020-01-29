#include <string.h>
#include "tlg_func.h"
#include "telegrams.h"
#include "gateway.h"

std::ostream& operator<< (std::ostream& os, const tlgnum_t& tlgNum)
{
    return os << (tlgNum.express ? "XPR" : "") << tlgNum.num;
}

extern"C" int write_tlg(tlgnum_t& num, INCOMING_INFO *ii, const char *body)
{
    Expected<telegrams::TlgResult, int> result = telegrams::callbacks()->writeTlg(ii, body);
    if (result) {
        num = result->tlgNum;
        return 0;
    }
    else {
        return result.err();
    }
}

namespace telegrams
{
extern "C" size_t tlgLength(const AIRSRV_MSG& t)
{
    return (sizeof(AIRSRV_MSG) - (sizeof(t.body) - strnlen(t.body, sizeof(t.body))));
}
extern "C" char* tlgTime()
{
    static char str[64];
    time_t tt = time(NULL);
    struct tm *tm_p = localtime(&tt);

    strftime(str, 32, "%d%b%T", tm_p);

    return(str);
}


}


