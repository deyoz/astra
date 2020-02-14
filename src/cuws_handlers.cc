#include "cuws_handlers.h"
#include "xml_unit.h"
#include "web_search.h"
#include "serverlib/str_utils.h"
#include <boost/regex.hpp>

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;

void to_envelope(xmlNodePtr resNode, const string &data)
{
    to_content(resNode, "/cuws_envelope.xml", "BODY", data);
}

void Search_Bags_By_BCBP(xmlNodePtr actionNode, xmlNodePtr resNode)
{
    static const string response_tag = "ns:Search_Bags_By_BCBPResponse";

    int point_id, reg_no, pax_id;
    bool isBoardingPass;
    SearchPaxByScanData(NodeAsString("BCBP", actionNode), point_id, reg_no, pax_id, isBoardingPass);
    ostringstream body, data;
    if(isBoardingPass) {
        CheckIn::TSimplePaxItem pax;
        pax.getByPaxId(pax_id);
        multiset<TBagTagNumber> tags;
        GetTagsByPool(pax.grp_id, pax.bag_pool_num, tags, true);
        TPnrAddrs pnrs;
        pnrs.getByPaxId(pax_id);
        string pnr_addr;
        if(not pnrs.empty()) pnr_addr = pnrs.begin()->addr;
        for(const auto &i: tags) {
            data
                << "<SummaryBags BagTagNumber=\"" << i.str() << "\" BaggageId=\"\" PNR=\"" << pnr_addr << "\"/>";
        }
    }
    if(data.str().empty())
        body << "<" << response_tag << "/>";
    else
        body << "<" << response_tag << ">" << data.str() << "</" << response_tag << ">";

    to_envelope(resNode, body.str());
}

string getResource(string file_path)
{
    try
    {
        TCachedQuery Qry1(
            "select text from HTML_PAGES, HTML_PAGES_TEXT "
            "where "
            "   HTML_PAGES.name = :name and "
            "   HTML_PAGES.id = HTML_PAGES_TEXT.id "
            "order by "
            "   page_no",
            QParams()
            << QParam("name", otString, file_path)
        );
        Qry1.get().Execute();
        string result;
        for (; not Qry1.get().Eof; Qry1.get().Next())
            result += Qry1.get().FieldAsString("text");
        return result;
    }
    catch(Exception &E)
    {
        cout << __FUNCTION__ << " " << E.what() << endl;
        return "";
    }
}

void to_content(xmlNodePtr resNode, const string &resource, const string &tag, const string &tag_data)
{
    string data = getResource(resource);
    if(not tag.empty())
        data = StrUtils::b64_encode(
                boost::regex_replace(
                    StrUtils::b64_decode(data),
                    boost::regex(tag), tag_data));
    SetProp( NewTextChild(resNode, "content",  data), "b64", true);
}


