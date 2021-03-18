#include "load_fr.h"
#include <iostream>
#include <vector>
#include <fstream>
#define NICKNAME "DENIS"
#include "serverlib/test.h"
#include <boost/shared_array.hpp>
#include "boost/filesystem/operations.hpp"
#include "stl_utils.h"
#include "oralib.h"
#include "exceptions.h"
#include "PgOraConfig.h"

#include <serverlib/dbcpp_cursctl.h>


using namespace std;
using namespace boost;
namespace fs = boost::filesystem;
using namespace EXCEPTIONS;

const string FALSE_LOCALE = "0";

// тест

static void save_fr_ora(const std::string& name,
                        const std::string& version,
                        const std::string& form,
                        int locale)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "update fr_forms2 set form = :form where name = :name and version = :version and pr_locale = :pr_locale";
    Qry.CreateVariable("name", otString, name);
    Qry.CreateVariable("version", otString, version);
    Qry.CreateLongVariable("form", otLong, (void *)form.c_str(), form.size());
    Qry.CreateVariable("pr_locale", otInteger, locale);
    Qry.Execute();
    if(!Qry.RowsProcessed()) {
        Qry.SQLText = "insert into fr_forms2(name, version, form, pr_locale) values(:name, :version, :form, :pr_locale)";
        Qry.Execute();
    }
}

static int upd_fr_pg(const std::string& name,
                     const std::string& version,
                     const std::string& form,
                     int locale)
{
    auto cur = make_db_curs(
"update FR_FORMS2 set FORM = :form "
"where NAME = :name and VERSION = :version and PR_LOCALE = :pr_locale",
                PgOra::getRWSession("FR_FORMS2"));
    cur
            .bind(":name",      name)
            .bind(":version",   version)
            .bind(":form",      form)
            .bind(":pr_locale", locale)
            .exec();
    return cur.rowcount();
}

static int ins_fr_pg(const std::string& name,
                     const std::string& version,
                     const std::string& form,
                     int locale)
{
    auto cur = make_db_curs(
"insert into FR_FORMS2(NAME, VERSION, FORM, PR_LOCALE) values(:name, :version, :form, :pr_locale)",
                PgOra::getRWSession("FR_FORMS2"));
    cur
            .bind(":name",      name)
            .bind(":version",   version)
            .bind(":form",      form)
            .bind(":pr_locale", locale)
            .exec();
    return cur.rowcount();
}

static void save_fr_pg(const std::string& name,
                       const std::string& version,
                       const std::string& form,
                       int locale)
{


    if(!upd_fr_pg(name, version, form, locale)) {
        ins_fr_pg(name, version, form, locale);
    }
}

static void save_fr(const std::string& name,
                    const std::string& version,
                    const std::string& form,
                    int locale)
{
    if(PgOra::supportsPg("FR_FORMS2")) {
        save_fr_pg(name, version, form, locale);
    } else {
        save_fr_ora(name, version, form, locale);
    }
}

void my(const fs::path &apath)
{
    // в boost 1.43 filename возвращает string,
    // а в boost 1.49 - объект
    // чтобы работало с обеими версиями, использован ostringstream
    //
    // Причем, если передать в ostringstream посто объект filename (без c_str()),
    // в boost 1.49 сформируется название файла с кавычками, напр. "crs.fr3",
    // чтобы было без кавычек для обеих версий boost (crs.fr3), передаем в поток c_str()
    ostringstream fname;
    fname << apath.filename().c_str();
    if(
            not fs::is_regular_file(apath) or
            apath.extension() != ".fr3"
      )
        return;

    ifstream in(apath.string().c_str());
    if(!in.good())
        throw Exception("Cannot open file %s", fname.str().c_str());
    char c;
    streambuf *sb;
    sb = in.rdbuf();
    ostringstream buf;
    while(sb->sgetc() != EOF) {
        c = sb->sbumpc();
        buf << c;
    }
    string form = buf.str();

    vector<string> tokens;
    size_t begin_pos = 0;
    size_t dot_pos = fname.str().find(".");
    while(dot_pos != string::npos) {
        tokens.push_back(fname.str().substr(begin_pos, dot_pos - begin_pos));
        begin_pos = dot_pos + 1;
        dot_pos = fname.str().find(".", begin_pos);
    }

    if(tokens.size() > 3)
        throw Exception("wrong file name: %s", fname.str().c_str());

    string name, version;
    int locale = 0;
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

    save_fr(name, version, form, locale);
    cout << fname.str() << "  ok." << endl;
}

void load_fr_help(const char *name)
{
  printf("  %-15.15s ", name);
  puts("<path to .fr3 files>");
};

void usage(string name, string what)
{
    cout 
        << "Error: " << what << endl
        << "Usage:" << endl;
    load_fr_help(name.c_str());
    cout
        << "Example:" << endl
        << "  " << name << " ./fr_reports/" << endl;

}

namespace {
    struct Fr
    {
        std::string name;
        std::string version;
        std::string form;
        int         pr_locale;
    };

    std::vector<Fr> load_fr_ora()
    {
        std::vector<Fr> vFr;
        TQuery Qry(&OraSession);
        Qry.SQLText = "select name, version, form, pr_locale from fr_forms2";
        Qry.Execute();
        for(; not Qry.Eof; Qry.Next()) {
            string name = Qry.FieldAsString("name");
            string version = Qry.FieldAsString("version");
            int pr_locale = Qry.FieldAsInteger("pr_locale");
            int len = Qry.GetSizeLongField("form");
            shared_array<char> data (new char[len]);
            Qry.FieldAsLong("form", data.get());
            string form;
            form.append(data.get(), len);

            vFr.emplace_back(Fr{name, version, form, pr_locale});
        }
        return vFr;
    }

    std::vector<Fr> load_fr_pg()
    {
        std::vector<Fr> vFr;
        auto cur = make_db_curs(
"select NAME, VERSION, FORM, PR_LOCALE from FR_FORMS2",
                    PgOra::getROSession("FR_FORMS2"));
        Fr fr = {};
        cur
                .def(fr.name)
                .def(fr.version)
                .def(fr.form)
                .def(fr.pr_locale)
                .exec();
        while(!cur.fen()) {
            vFr.emplace_back(fr);
        }
        return vFr;
    }

    std::vector<Fr> load_fr()
    {
        if(PgOra::supportsPg("FR_FORMS2")) {
            return load_fr_pg();
        } else {
            return load_fr_ora();
        }
    }
}//namespace

int get_fr(int argc,char **argv)
{
    try {
        fs::path full_path;
        if(argc > 1)
            full_path = fs::system_complete(fs::path(argv[1]));
        else
            throw Exception("dir not specified");
        if ( !fs::exists( full_path ) )
            throw Exception("path not found: %s", full_path.string().c_str());
        if ( not fs::is_directory( full_path ) )
            throw Exception("path is not a directory: %s", full_path.string().c_str());

        auto vFr = load_fr();
        for(auto fr: vFr) {
            std::string fname = fr.name;
            std::string version = fr.version;
            std::string pr_locale = std::to_string(fr.pr_locale);
            if(version != DEF_VERS)
                fname += "." + version;
            if(pr_locale == FALSE_LOCALE)
                fname += "." + pr_locale;
            fname += ".fr3";

            cout << "getting " << fname.c_str() << endl;
            fs::path apath = full_path / fname;
            std::ofstream out(apath.string().c_str());
            if(!out.good())
                throw Exception("Cannot open file %s", apath.string().c_str());
            out << fr.form;
        }
        cout << "The templates were fetched successfully" << endl;
    } catch(Exception &E) {
        usage(argv[0], E.what());
        return 1;
    }
    return 0;
}

int load_fr(int argc,char **argv)
{
    try {
        fs::path full_path;
        if(argc > 1)
            full_path = fs::system_complete(fs::path(argv[1]));
        else
            throw Exception("dir not specified");
        if ( !fs::exists( full_path ) )
            throw Exception("path not found: %s", full_path.string().c_str());

        if ( fs::is_directory( full_path ) )
        {
            fs::directory_iterator end_iter;
            for ( fs::directory_iterator dir_itr( full_path ); dir_itr != end_iter; ++dir_itr )
                my(dir_itr->path());
        } else {
            my(full_path);
        }
    } catch(Exception &E) {
        usage(argv[0], E.what());
        return 1;
    }
    return 0;
}

