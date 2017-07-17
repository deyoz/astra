#include "kiosk_alias.h"
#include <iostream>
#include <fstream>
#include <boost/shared_array.hpp>
#include "boost/filesystem/operations.hpp"
#include "exceptions.h"
#define NICKNAME "DENIS"
#include "serverlib/slogger.h"
#include "serverlib/str_utils.h"
#include "oralib.h"
#include "xml_unit.h"

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <boost/regex.hpp>

using namespace std;
using namespace boost;
namespace fs = boost::filesystem;
using namespace EXCEPTIONS;

namespace KIOSK {

    string exec(const char* cmd) {
        std::array<char, 128> buffer;
        string result;
        std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
        if (!pipe) throw runtime_error("popen() failed!");
        while (!feof(pipe.get())) {
            if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
                result += buffer.data();
        }
        return result;
    }


    void alias_to_db_help(const char *name)
    {
        printf("  %-15.15s ", name);
        puts("<path to alias xml file>");
    };

    void usage(string name, string what)
    {
        cout 
            << "Error: " << what << endl
            << "Usage:" << endl;
        alias_to_db_help(name.c_str());
        cout
            << "Example:" << endl
            << "  " << name << " aliases_kiosk" << endl;

    }

    int alias_to_db1(int argc, char **argv)
    {
        try {
            fs::path full_path;
            if(argc > 1)
                full_path = fs::system_complete(fs::path(argv[1]));
            else
                throw Exception("file not specified");

            if(not fs::is_regular_file(full_path))
                throw Exception("%s not a regular file", full_path.string().c_str());

            ostringstream fname;
            fname << full_path.filename().c_str();

            ifstream in(full_path.string().c_str());
            if(!in.good())
                throw Exception("Cannot open file %s", fname.str().c_str());
            string content = string( std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() );
            XMLDoc doc(content);
            if(not doc.docPtr())
                throw Exception("cannot parse XML content");
            xmlNodePtr returnNode = doc.docPtr()->children;
            for(; returnNode; returnNode = returnNode->children)
                if(not strcmp((const char*)returnNode->name, "return")) break;
            if(not returnNode)
                throw Exception("return node not found");
            xmlNodePtr curNode = returnNode->children;
            int count = 0;
            set<string> found;
            for(; curNode; curNode = curNode->next, count++) {
                string descr = NodeAsString("@description", curNode);
                string name = NodeAsString("@name", curNode);
                string cmd = "grep -n '" + name + "' -R ~/kiosk/ --exclude=*.svn-base";
                cout << setw(5) << count << " processing: " << name << endl;
                string src = exec(cmd.c_str());
                if(not src.empty()) found.insert(name);
                LogTrace(TRACE5) << "descr: '" << descr << "'";
                LogTrace(TRACE5) << "name: '" << name << "'";
                LogTrace(TRACE5) << "src: '" << src << "'";
                xmlNodePtr valueNode = curNode->children;
                for(; valueNode; valueNode = valueNode->next) {
                    string lang = NodeAsString("@lang", valueNode);
                    string value = NodeAsString(valueNode);
                    LogTrace(TRACE5) << lang << ": " << value;
                }
                LogTrace(TRACE5) << "---------------------";
            }
            LogTrace(TRACE5) << count << " aliases processed";
            LogTrace(TRACE5) << found.size() << " aliases found";
            for(set<string>::iterator i = found.begin(); i != found.end(); i++) {
                LogTrace(TRACE5) << *i;
            }
        } catch(Exception &E) {
            usage(argv[0], E.what());
            return 1;
        }
        return 0;
    }

    struct TAlias {
        string name;
        string descr;
        map<string, string> locale;
        bool operator < (const TAlias &item) const
        {
            return name < item.name;
        }
    };

    struct TAliasList {
        string src;
        set<TAlias> alias_list;
    };

    typedef map<string, TAliasList> TAliasBaseList;

    int alias_to_db(int argc, char **argv)
    {
        try {
            fs::path full_path;
            if(argc > 1)
                full_path = fs::system_complete(fs::path(argv[1]));
            else
                throw Exception("file not specified");

            if(not fs::is_regular_file(full_path))
                throw Exception("%s not a regular file", full_path.string().c_str());

            ostringstream fname;
            fname << full_path.filename().c_str();

            ifstream in(full_path.string().c_str());
            if(!in.good())
                throw Exception("Cannot open file %s", fname.str().c_str());
            string content = string( std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() );
            XMLDoc doc(content);
            if(not doc.docPtr())
                throw Exception("cannot parse XML content");
            xmlNodePtr returnNode = doc.docPtr()->children;
            for(; returnNode; returnNode = returnNode->children)
                if(not strcmp((const char*)returnNode->name, "return")) break;
            if(not returnNode)
                throw Exception("return node not found");
            xmlNodePtr curNode = returnNode->children;
            int count = 0;
            TAliasBaseList alias_base_list;
            for(; curNode; curNode = curNode->next, count++) {
                TAlias alias;
                alias.descr = NodeAsString("@description", curNode);
                alias.name = NodeAsString("@name", curNode);

                xmlNodePtr valueNode = curNode->children;
                for(; valueNode; valueNode = valueNode->next) {
                    alias.locale.insert(make_pair(
                                NodeAsString("@lang", valueNode),
                                NodeAsString(valueNode)));
                }

                if(alias.name == "DEN") {
                    LogTrace(TRACE5) << "DEN: " << alias.locale["de"];


                    xmlDocPtr doc = CreateXMLDoc( "tst" );
                    try {
                        xmlNodePtr queryNode = NewTextChild(doc->children, "query", "&" + alias.locale["de"]);
                        LogTrace(TRACE5) << GetXMLDocText(doc);
                    }
                    catch ( ... ) {
                        xmlFreeDoc( doc );
                        throw;
                    }




                    string encoded = StrUtils::b64_encode(alias.locale["de"]);
                    TQuery Qry(&OraSession);
                    Qry.SQLText = "insert into kiosk_alias_list values(:encoded, :encoded)";
                    Qry.CreateVariable("encoded", otString, encoded);
                    Qry.Execute();
                    return 0;
                }

                boost::match_results<std::string::const_iterator> results;
                static const boost::regex e("^\\S+$");
                static const boost::regex digits("^\\d+$"); // to exclude 123 alias
                if(
                        not boost::regex_match(alias.name, results, e) or
                        boost::regex_match(alias.name, results, digits))
                    continue;
                /*
                else {
                    LogTrace(TRACE5) << "wrong alias: '" << alias.name << "'";
                    LogTrace(TRACE5) << "value: '" << alias.locale["ru"] << "'";
                }
                */

                vector<string> alias_parts;
                boost::split(alias_parts, alias.name, boost::is_any_of("."));
                string alias_base;
                static const size_t alias_base_size = 2;
                if(alias_parts.size() > alias_base_size) {
                    size_t curr_part = 1;
                    for(vector<string>::iterator i = alias_parts.begin(); i != alias_parts.end() and curr_part <= alias_base_size; i++, curr_part++) {
                        alias_base += *i + ".";
                    }
                } else
                    alias_base = alias.name;
                alias_base_list[alias_base].alias_list.insert(alias);
            }

            LogTrace(TRACE5) << count << " aliases processed";
            LogTrace(TRACE5) << "alias_base_list.size(): " << alias_base_list.size();
            count = 0;
            TAliasBaseList found;
            for(TAliasBaseList::iterator i = alias_base_list.begin();
                    i != alias_base_list.end(); i++, count++) {
                string search_pattern = i->first;
                boost::replace_all(search_pattern, ".", "\\.");
                string cmd = "grep -n '" + search_pattern + "' -R ~/kiosk/ --exclude=*.svn-base";
                cout << setw(5) << count << " processing: " << i->first << endl;
                i->second.src = exec(cmd.c_str());
                if(not i->second.src.empty()) found.insert(*i);
            }
            LogTrace(TRACE5) << count << " alias bases processed";
            LogTrace(TRACE5) << found.size() << " alias bases found";
            count = 0;
            size_t alias_len = 0;
            size_t descr_len = 0;
            LogTrace(TRACE5) << "----dump results----";
            for(TAliasBaseList::iterator i = found.begin(); i != found.end(); i++) {
                count += i->second.alias_list.size();
                LogTrace(TRACE5) << "alias_base: " << i->first;
                LogTrace(TRACE5) << "src: " << i->second.src;
                for(set<TAlias>::iterator j = i->second.alias_list.begin();
                        j != i->second.alias_list.end(); j++) {
                    LogTrace(TRACE5) << j->name;

                    boost::match_results<std::string::const_iterator> results;
                    static const boost::regex e("^[A-Za-z.\\d_]+$");
                    if(not boost::regex_match(j->name, results, e))
                        LogTrace(TRACE5) << "NOT MATCH: " << j->name;

                    if(alias_len < j->name.size()) alias_len = j->name.size();
                    if(descr_len < j->descr.size()) descr_len = j->descr.size();
                }
            }
            LogTrace(TRACE5) << count << " aliases found by base";
            LogTrace(TRACE5) << "alias_len: " << alias_len;
            LogTrace(TRACE5) << "descr_len: " << descr_len;
        } catch(Exception &E) {
            usage(argv[0], E.what());
            return 1;
        }
        return 0;
    }

}
