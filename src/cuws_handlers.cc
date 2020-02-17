#include "cuws_handlers.h"
#include "xml_unit.h"
#include "web_search.h"
#include "serverlib/str_utils.h"
#include <boost/regex.hpp>

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;

namespace CUWS {

void to_envelope(xmlNodePtr resNode, const string &data)
{
    to_content(resNode, "/cuws_envelope.xml", "BODY", data);
}

void wrap_empty(const string &tag, ostringstream &body)
{
    body << "<" << tag << "/>";
}

void wrap_begin(const string &tag, ostringstream &body)
{
    body << "<" << tag << ">";
}

void wrap_end(const string &tag, ostringstream &body)
{
    body << "</" << tag << ">";
}

void wrap(const string &tag, const string &data, ostringstream &body)
{
    if(data.empty())
        wrap_empty(tag, body);
    else {
        wrap_begin(tag, body);
        body << data;
        wrap_end(tag, body);
    }
}

void Search_Bags_By_BCBP(xmlNodePtr actionNode, xmlNodePtr resNode)
{
    static const string response_tag        = "ns:Search_Bags_By_BCBPResponse";
    static const string summary_bags_tag    = "SummaryBags";
    static const string tag_no_tag          = "BagTagNumber";
    static const string bag_id_tag          = "BaggageId";
    static const string pnr_tag             = "PNR";

    int point_id, reg_no, pax_id;
    bool isBoardingPass;
    SearchPaxByScanData(NodeAsString("BCBP", actionNode), point_id, reg_no, pax_id, isBoardingPass);
    ostringstream body, data;
    if(isBoardingPass) {
        CheckIn::TSimplePaxItem pax;
        pax.getByPaxId(pax_id);
        multiset<TBagTagNumber> tags;
        GetTagsByPool(pax.grp_id, pax.bag_pool_num, tags, true);
        if(tags.empty())
            wrap_empty(response_tag, body);
        else {
            wrap_begin(response_tag, body);

            TPnrAddrs pnrs;
            pnrs.getByPaxId(pax_id);
            string pnr_addr;
            if(not pnrs.empty()) pnr_addr = pnrs.begin()->addr;
            for(const auto &i: tags) {
                wrap_begin(summary_bags_tag, body);

                wrap(tag_no_tag, i.str(), body);
                wrap_empty(bag_id_tag, body);
                wrap(pnr_tag, pnr_addr, body);

                wrap_end(summary_bags_tag, body);
            }

            wrap_end(response_tag, body);
        }
    }
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

} //end namespace CUWS

