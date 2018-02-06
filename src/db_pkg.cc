#include "db_pkg.h"
#include "oralib.h"
#include "stl_utils.h"
#include <fstream>
#include "boost/filesystem/operations.hpp"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace boost;
namespace fs = boost::filesystem;
using namespace EXCEPTIONS;

string file_name(const string &pkg_name)
{
    const map<string, int> _pkg =
    {
        {"ckin",    1},
        {"kassa",   2},
        {"salons",  3},
        {"report",  4},
        {"statist", 6},
        {"timer",   7},
        {"system",  10},
        {"adm",     12},
        {"arch",    13},
        {"hist",    14}
    };
    map<string, int>::const_iterator idx = _pkg.find(pkg_name);
    ostringstream result;
    if(idx != _pkg.end()) {
        result << idx->second << pkg_name;
    } else
        result << pkg_name;
    result << ".sql";
    return result.str();
}

void db_pkg_usage(string name, string what)
{
    cout 
        << "Error: " << what << endl
        << "Usage: " << name << " <dir> [<pkg name>]" << endl;
    cout
        << "Example:" << endl
        << "  " << name << " ./beta/ timer" << endl;

}

int db_pkg(int argc, char **argv)
{
    try {
        fs::path full_path;
        if(argc > 1)
            full_path = fs::system_complete(fs::path(argv[1]));
        else
            throw Exception("dir not specified");
        string pkg;
        if(argc > 2) {
            pkg = lowerc(argv[2]);
            if(pkg == "timer") pkg = "gtimer";
        }
        if ( !fs::exists( full_path ) )
            throw Exception("path not found: %s", full_path.string().c_str());
        if ( not fs::is_directory( full_path ) )
            throw Exception("path is not a directory: %s", full_path.string().c_str());

        TQuery Qry(&OraSession);
        string SQLText =
            "select type, name, text from user_source where "
            "   type in('PACKAGE', 'PACKAGE BODY') and "
            "   name <> 'UTILS' ";
        if(not pkg.empty()) {
            SQLText += " and lower(name) = :pkg ";
            Qry.CreateVariable("pkg", otString, pkg);
        }
        SQLText +=
            "order by "
            "   name, type, line";
        Qry.SQLText = SQLText;
        Qry.Execute();
        string name;
        string type;
        vector<ofstream> files;
        for(; not Qry.Eof; Qry.Next()) {
            string tmp_name = Qry.FieldAsString("name");
            if(tmp_name == "GTIMER") tmp_name = "TIMER";
            string tmp_type = Qry.FieldAsString("type");
            if(name != tmp_name) {
                name = tmp_name;
                if(not files.empty()) {
                    files.back()
                        << "/" << endl
                        << "show error" << endl;
                    files.back().close();
                    cout << "OK" << endl;
                }
                fs::path apath = full_path / file_name(lowerc(name));
                files.push_back(ofstream(apath.string().c_str(), std::ios::binary|std::ios::trunc));
                cout << "fetching " << name << "... ";
                cout.flush();
            }
            if(type != tmp_type) {
                type = tmp_type;
                if(type == "PACKAGE BODY")
                    files.back()
                        << "/" << endl
                        << "show error" << endl;
                files.back() << "create or replace ";
            }
            string cur_line = ConvertCodepage(Qry.FieldAsString("text"), "cp866", "utf-8");
            files.back() << RTrimString(cur_line) << endl;
        }
        if(not files.empty()) {
            files.back()
                << "/" << endl
                << "show error" << endl;
            files.back().close();
            cout << "OK" << endl;
        }
    } catch(Exception &E) {
        db_pkg_usage(argv[0], E.what());
        return 1;
    }
    return 1;
}
