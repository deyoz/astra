#include "html_pages.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "boost/filesystem.hpp"
#include "qrys.h"
#include "exceptions.h"
#include "astra_utils.h"
#include "xml_unit.h"
#include "md5_sum.h"
#include "stl_utils.h"
#include "PgOraConfig.h"
#include "db_tquery.h"
#include <boost/regex.hpp>

#include <serverlib/dbcpp_cursctl.h>
#include <serverlib/dates_io.h>
#include <serverlib/str_utils.h>

#define NICKNAME "KOSHKIN"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;
namespace fs = boost::filesystem;

const bool LOCAL_DEBUG = true;

void html_db_usage(string name, string what)
{
    cout    << "Error: " << what << endl
            << "Usage:" << endl
            << " " << name << " <html_dir>" << endl
            << "Example:" << endl
            << " " << name << " html" << endl;
}

vector<string> get_file_list(const string& init_path)
{
    //  https://gist.github.com/vivithemage/9517678
    //  http://stackoverflow.com/questions/67273/how-do-you-iterate-through-every-file-directory-recursively-in-standard-c
    vector<string> file_list;
    if (init_path.empty())
        throw Exception("Path is empty");
    fs::recursive_directory_iterator end;
    for (fs::recursive_directory_iterator i_entry(init_path); i_entry != end; ++i_entry)
        if (fs::is_regular_file(i_entry->path()))
            file_list.push_back( i_entry->path().string().substr(init_path.size()) );
    return file_list;
}

string file_to_string(const string& full_path)
{
    if (LOCAL_DEBUG) cout << __FUNCTION__ << " READ FILE: " << full_path << endl;
    ifstream in(full_path.c_str(), std::ios::binary);
    if (!in.good())
        throw Exception("Cannot open for read file %s", full_path.c_str());
    return StrUtils::b64_encode(string( std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() ));
}

void file_to_db(const string& init_path, const string& file_path)
{
    string file_data =  file_to_string(init_path + file_path);

    int existingId = 0;
    auto cur = make_db_curs(
"select ID from HTML_PAGES where name = :name",
                PgOra::getRWSession("HTML_PAGES"));

    cur
            .def(existingId)
            .bind(":name", file_path)
            .exec();

    while(!cur.fen()) {
        make_db_curs(
"delete from HTML_PAGES_TEXT where ID = :id",
                    PgOra::getRWSession("HTML_PAGES_TEXT"))
                .bind(":id", existingId)
                .exec();
    }

    make_db_curs(
"delete from HTML_PAGES where name = :name",
                PgOra::getRWSession("HTML_PAGES"))
            .bind(":name", file_path)
            .exec();

    int id = PgOra::getSeqNextVal_int("TID__SEQ");
    make_db_curs(
"insert into HTML_PAGES(id, name, etag, last_modified) values(:id, :name, :etag, :last_modified)",
                PgOra::getRWSession("HTML_PAGES"))
            .stb()
            .bind(":id",            id)
            .bind(":name",          file_path)
            .bind(":etag",          md5_sum(file_data))
            .bind(":last_modified", boost::posix_time::second_clock::universal_time())
            .exec();

    DB::TCachedQuery Qry(PgOra::getRWSession("HTML_PAGES_TEXT"),
 "insert into HTML_PAGES_TEXT(id, page_no, text) values(:id, :page_no, :text)",
        QParams()
            << QParam("id",      otInteger, id)
            << QParam("page_no", otInteger)
            << QParam("text",    otString),
        STDLOG
    );
    longToDB(Qry.get(), "text", file_data);
}

//  --------------------------------
int html_to_db(int argc, char **argv)
{
    try
    {
        if (argc != 2)
            throw Exception("Must provide path");
        string init_path(argv[1]);
        while (init_path.size() > 1 and *init_path.rbegin() == '/') init_path.erase(init_path.size() - 1);
        vector<string> file_list = get_file_list(init_path);
        for (vector<string>::iterator i_file_path = file_list.begin(); i_file_path != file_list.end(); ++i_file_path)
            file_to_db(init_path, *i_file_path);
    }
    catch(Exception &E)
    {
        html_db_usage(argv[0], E.what());
        return 1;
    }
    return 0;
}

//  --------------------------------
int html_from_db(int argc, char **argv)
{
    try
    {
        if (argc != 2)
            throw Exception("Must provide path");
        string init_path(argv[1]);
        while (init_path.size() > 1 and *init_path.rbegin() == '/') init_path.erase(init_path.size() - 1);
        DB::TCachedQuery Qry1(PgOra::getROSession("SP_PG_GROUP_HTML"),
            "select name, text from HTML_PAGES, HTML_PAGES_TEXT "
            "where "
            "   HTML_PAGES.id = HTML_PAGES_TEXT.id "
            "order by "
            "   name, page_no",
            STDLOG
        );
        Qry1.get().Execute();
        map<string, string> pages;
        for (; not Qry1.get().Eof; Qry1.get().Next())
        {
            string &text = pages[Qry1.get().FieldAsString("name")];
            text += Qry1.get().FieldAsString("text");
        }
        for (map<string, string>::iterator ip = pages.begin(); ip != pages.end(); ip++)
        {
            string file_path =  ip->first;
            string full_path = init_path + file_path;
            // path.filename() �����頥� ࠧ�� ⨯� � ࠧ��� ������ boost, ���⮬� c_str()
            string dir_path = full_path.substr( 0, full_path.size() - string( fs::path(full_path).filename().c_str() ).size() );
            fs::create_directories(dir_path);
            if (LOCAL_DEBUG) cout << __FUNCTION__ << " WRITE FILE: " << full_path << endl;
            ofstream out(full_path.c_str(), std::ios::binary|std::ios::trunc);
            if(!out.good())
                throw Exception("Cannot open for write file %s", full_path.c_str());
            out << StrUtils::b64_decode(ip->second);
        }
    }
    catch(Exception &E)
    {
        html_db_usage(argv[0], E.what());
        return 1;
    }
    return 0;
}

struct THTMLResource {
    string name;
    string etag;
    TDateTime last_modified;
    string data;
    void Clear();
    THTMLResource() { Clear(); }
    void get(const string &aname);
    string get_last_modified();
};

string THTMLResource::get_last_modified()
{
    //select to_char(:last_modified, 'Dy, dd Mon yyyy hh24:mi:ss', 'NLS_DATE_LANGUAGE = American')||' GMT' from dual
    auto lm = DateTimeToBoost(last_modified);
    std::ostringstream os;
    os << lm.date().day_of_week() << ", "
       << HelpCpp::string_cast(lm, "%d %b %Y %H:%M:%S GMT");
    return os.str();
}

void THTMLResource::Clear()
{
    name.clear();
    etag.clear();
    last_modified = ASTRA::NoExists;
    data.clear();
}

void THTMLResource::get(const string &aname)
{
    Clear();
    DB::TCachedQuery Qry(PgOra::getROSession("SP_PG_GROUP_HTML"),
            "select etag, last_modified, text from HTML_PAGES, HTML_PAGES_TEXT "
            "where "
            "   HTML_PAGES.name = :name and "
            "   HTML_PAGES.id = HTML_PAGES_TEXT.id "
            "order by "
            "   page_no",
            QParams() << QParam("name", otString, aname),
            STDLOG
            );
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        for (; not Qry.get().Eof; Qry.get().Next()) {
            if(name.empty()) {
                name = aname;
                etag = Qry.get().FieldAsString("etag");
                last_modified = Qry.get().FieldAsDateTime("last_modified");
            }
            data += Qry.get().FieldAsString("text");
        }
    }
}

string getResource(const string &file_path)
{
    THTMLResource html_resource;
    html_resource.get(file_path);
    return html_resource.data;
}

const string TResHTTPParams::NAME = "res_http_params";

void TResHTTPParams::Clear()
{
    status = ServerFramework::HTTP::reply::ok;
    hdrs.clear();
}

void TResHTTPParams::fromXML(string &data)
{
    LogTrace(TRACE5) << "TResHTTPParams::fromXML data: '" << data << "'";

    static const boost::regex e("^(.*)(<" + NAME + ">.*</" + NAME + ">)(.*)$");
    boost::match_results<std::string::const_iterator> results;
    if(boost::regex_match(data, results, e)) {
        data = results[1] + results[3]; // �모�뢠�� �� �⢥⭮�� ���⥭� ᥪ�� �� http params

        XMLDoc doc = XMLDoc(results[2]);
        if(not doc.docPtr()) throw Exception("TResHTTPParams::fromXML failed to parse xml");
        xmlNodePtr node = doc.docPtr()->children->children;
        xmlNodePtr curNode = NodeAsNodeFast("status", node);
        status = (ServerFramework::HTTP::reply::status_type)NodeAsInteger(curNode);
        curNode = NodeAsNodeFast("hdrs", node);
        curNode = curNode->children;
        for(; curNode; curNode = curNode->next)
            hdrs[(char *)curNode->name] = NodeAsString(curNode);
    }
}

void TResHTTPParams::toXML(xmlNodePtr node)
{
    xmlNodePtr paramsNode = NewTextChild(node, NAME.c_str());
    NewTextChild(paramsNode, "status", status);
    xmlNodePtr hdrsNode = NULL;
    for(map<string, string>::const_iterator i = hdrs.begin();
            i != hdrs.end(); i++) {
        if(not hdrsNode)
            hdrsNode = NewTextChild(paramsNode, "hdrs");
        NewTextChild(hdrsNode, i->first.c_str(), i->second);
    }
}

void HtmlInterface::get_resource(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string uri_path = NodeAsString("uri_path", reqNode);
    LogTrace(TRACE5) << "get_resource uri_path: " << uri_path;

    TResHTTPParams rhp;
    THTMLResource html_resource;
    html_resource.get(uri_path);
    xmlNodePtr contentNode = NewTextChild(resNode, "content");
    if(not html_resource.data.empty()) {

        string if_none_match = html_header_param(HTTP_HDR::IF_NONE_MATCH, reqNode);
        string if_modified_since_str = html_header_param(HTTP_HDR::IF_MODIFIED_SINCE, reqNode);

        LogTrace(TRACE5) << "if_none_match: '" << if_none_match << "'";
        LogTrace(TRACE5) << "if_modified_since_str: '" << if_modified_since_str << "'";

        TDateTime if_modified_since = ASTRA::NoExists;
        if(not if_modified_since_str.empty()) {
            // ����� ��������� ��ப� ���� 'Fri, 10 Feb 2017 09:33:30 GMT'
            // �⪨�뢠�� ���� ������ � GMT
            size_t idx = if_modified_since_str.find(',');
            if(idx == string::npos)
                throw Exception("unexpected format of %s", HTTP_HDR::IF_MODIFIED_SINCE.c_str());
            if_modified_since_str.erase(0, idx + 1);
            if_modified_since_str.erase(if_modified_since_str.size() - 4);
            if_modified_since_str = upperc(if_modified_since_str);
            LogTrace(TRACE5) << "if_modified_since_str after strip: '" << if_modified_since_str << "'";

            if(StrToDateTime(if_modified_since_str.c_str(), "dd mmm yyyy hh:nn:ss", if_modified_since, true) == EOF)
                throw Exception("get_resource: can't parse if_modified_since date: %s", if_modified_since_str.c_str());
            LogTrace(TRACE5) << "if_modified_since: " << DateTimeToStr(if_modified_since);
        }

        if(if_none_match.empty() or if_none_match != html_resource.etag) {
            NodeSetContent(contentNode, html_resource.data);
            SetProp(contentNode, "b64", true);
            rhp.hdrs[HTTP_HDR::LAST_MODIFIED] = html_resource.get_last_modified();
        } else {
            // not modified
            rhp.status = ServerFramework::HTTP::reply::not_modified;
        }
        rhp.hdrs[HTTP_HDR::ETAG] = html_resource.etag;
        rhp.toXML(resNode);
    }
}

string http_req_param(const string &tag_name, xmlNodePtr reqNode, bool hdr)
{
    string result;
    xmlNodePtr node = reqNode->children;
    if(hdr)
        node = NodeAsNodeFast("header", node);
    else
        node = NodeAsNodeFast("get_params", node);
    if(not node) throw Exception("html_get_param: get_params not found where expected");
    node = node->children;
    for(; node; node = node->next) {
        xmlNodePtr node2 = node->children;
        string name = NodeAsStringFast("name", node2);
        string value = NodeAsStringFast("value", node2);
        if(name == tag_name) {
            result = value;
            break;
        }
    }
    return result;
}

string html_header_param(const string &tag_name, xmlNodePtr reqNode)
{
    return http_req_param(tag_name, reqNode, true);
}

string html_get_param(const string &tag_name, xmlNodePtr reqNode)
{
    return http_req_param(tag_name, reqNode, false);
}
