#include "utils.h"
#include "stat_utils.h"
#include "astra_utils.h"

using namespace std;
using namespace AstraLocale;

string get_test_str(int page_width, string lang)
{
    string result;
    for(int i=0;i<page_width/6;i++) result += " " + ( STAT::bad_client_img_version() and not get_test_server() ? " " : getLocaleText("CAP.TEST", lang)) + " ";
    return result;
}

