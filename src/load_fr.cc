#include "load_fr.h"
#include <iostream>
#include <vector>
#include <fstream>
#define NICKNAME "DENIS"
#include "serverlib/test.h"
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "stl_utils.h"
#include "oralib.h"
#include "exceptions.h"


using namespace std;
using namespace boost;
namespace fs = boost::filesystem;
using namespace EXCEPTIONS;

const string FALSE_LOCALE = "0";
const string DEF_VERS = "000000-0000000";

// тест

void my(fs::path &full_path, string fname)
{
    fs::path apath = full_path / fname;
    ifstream in(apath.native_file_string().c_str());
    if(!in.good())
        throw Exception("Cannot open file %s", fname.c_str());
    char c;
    streambuf *sb;
    sb = in.rdbuf();
    ostringstream form;
    while(sb->sgetc() != EOF) {
        c = sb->sbumpc();
        form << c;
    }

    cout << "saving " << (apath.native_file_string() + ".out") << endl;
    ofstream out((apath.native_file_string() + ".out").c_str());
    out << form.str();
    out.close();

    vector<string> tokens;
    size_t begin_pos = 0;
    size_t dot_pos = fname.find(".");
    while(dot_pos != string::npos) {
        tokens.push_back(fname.substr(begin_pos, dot_pos - begin_pos));
        begin_pos = dot_pos + 1;
        dot_pos = fname.find(".", begin_pos);
    }

    if(tokens.size() > 3)
        throw Exception("wrong file name: %s", fname.c_str());

    string name, version;
    int locale;
    name = tokens[0];
    if(tokens.size() == 3) {
        version = tokens[1];
        locale = ToInt(tokens[2]);
    }
    if(tokens.size() == 2) {
        if(tokens[1] == FALSE_LOCALE) {
            version = DEF_VERS;
            locale = 0;
        } else {
            version = tokens[1];
            locale = 1;
        }
    }
    if(tokens.size() == 1) {
        version = DEF_VERS;
        locale = 1;
    }

    TQuery Qry(&OraSession);
    Qry.SQLText =
        "begin "
        "   update fr_forms2 set "
        "       form = :form "
        "   where "
        "       name = :name and "
        "       version = :version and "
        "       pr_locale = :pr_locale; "
        "   if sql%notfound then "
        "       insert into fr_forms2 ( "
        "           name, "
        "           version, "
        "           form, "
        "           pr_locale "
        "       ) values ( "
        "           :name, "
        "           :version, "
        "           :form, "
        "           :pr_locale "
        "       ); "
        "   end if; "
        "   commit; "
        "end; ";
    Qry.CreateVariable("name", otString, name);
    Qry.CreateVariable("version", otString, version);
    Qry.CreateLongVariable("form", otLong, (void *)form.str().c_str(), form.str().size());
    Qry.CreateVariable("pr_locale", otInteger, locale);
    ProgTrace(TRACE5, "before exec");
    Qry.Execute();
    ProgTrace(TRACE5, "after exec");
}

int load_fr(int argc,char **argv)
{
    fs::path full_path;
    if(argc > 1)
        full_path = fs::system_complete(fs::path(argv[1]));
    else
        throw Exception("dir not specified");
    if ( !fs::exists( full_path ) )
        throw Exception("path not found: %s", full_path.native_file_string().c_str());

    if ( fs::is_directory( full_path ) )
    {
        fs::directory_iterator end_iter;
        for ( fs::directory_iterator dir_itr( full_path ); dir_itr != end_iter; ++dir_itr )
        {
            if ( not fs::is_directory( *dir_itr ) )
            {
                if(dir_itr->leaf().substr(dir_itr->leaf().size() - 4, 4) == ".fr3") {
                    ProgTrace(TRACE5, "processing %s", dir_itr->leaf().c_str());
                    my(full_path, dir_itr->leaf());
                }
            }
        }
    }

    ProgTrace(TRACE5, "Den was here");


    return 0;
}
