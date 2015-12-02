#include "img.h"
#include "boost/filesystem/operations.hpp"
#include <fstream>
#include "exceptions.h"
#include "oralib.h"

namespace fs = boost::filesystem;
using namespace std;
using namespace EXCEPTIONS;

const int PAGE_SIZE = 4000;

namespace img {

    // для cp866
    void process(const fs::path &apath)
    {
        if(not fs::is_regular_file(apath)) return;

        ostringstream fname;
        fname << apath.filename().c_str();
        ifstream in(apath.string().c_str(), std::ios::binary);
        if(!in.good())
            throw Exception("Cannot open file %s", fname.str().c_str());
        string buffer((
                    std::istreambuf_iterator<char>(in)), 
                (std::istreambuf_iterator<char>()));
        cout << "buffer.size(): " << buffer.size() << endl;
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "begin "
            "   delete from images_data where id = (select id from images where name = :name); "
            "   delete from images where name = :name; "
            "   insert into images values(tid__seq.nextval, :name) returning id into :id; "
            "end; ";
        Qry.CreateVariable("name", otString, fname.str());
        Qry.DeclareVariable("id", otInteger);
        Qry.Execute();
        int id = Qry.GetVariableAsInteger("id");
        cout << "id: " << id << endl;
        Qry.Clear();
        Qry.SQLText = "insert into images_data(id, page_no, data) values(:id, :page_no, :data)";
        Qry.CreateVariable("id", otInteger, id);
        Qry.DeclareVariable("page_no", otInteger);
        Qry.DeclareVariable("data", otString);
        int page = 0;
        while(true) {
            size_t first = page * PAGE_SIZE;
            size_t last = (page + 1) * PAGE_SIZE;
            Qry.SetVariable("page_no", page);
            Qry.SetVariable("data", buffer.substr(first, last));
            Qry.Execute();
            if(last >= buffer.size()) break;
            page++;
        }
    }

    void load_img_help(const char *name)
    {
        printf("  %-15.15s ", name);
        puts("<path to img files>");
    };

    void usage(string name, string what)
    {
        cout 
            << "Error: " << what << endl
            << "Usage:" << endl;
        load_img_help(name.c_str());
        cout
            << "Example:" << endl
            << "  " << name << " ./img/" << endl;

    }

    int get_img(int argc,char **argv)
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
            throw Exception("path is not directory: %s", full_path.string().c_str());

        TQuery Qry(&OraSession);
        Qry.SQLText =
            "select data from images, images_data "
            "   where images.name = :name and "
            "   images.id = images_data.id "
            "order by "
            "   page_no";
        Qry.CreateVariable("name", otString, full_path.string());
        Qry.Execute();
        /*
        for(; not Qry.Eof; Qry.Next()) {
            string fname = Qry.FieldAsString("name");
            string version = Qry.FieldAsString("version");
            string pr_locale = Qry.FieldAsString("pr_locale");
            if(version != DEF_VERS)
                fname += "." + version;
            if(pr_locale == FALSE_LOCALE)
                fname += "." + pr_locale;
            fname += ".fr3";

            int len = Qry.GetSizeLongField("form");
            shared_array<char> data (new char[len]);
            Qry.FieldAsLong("form", data.get());
            string form;
            form.append(data.get(), len);

            cout << "getting " << fname.c_str() << endl;
            fs::path apath = full_path / fname;
            ofstream out(apath.string().c_str());
            if(!out.good())
                throw Exception("Cannot open file %s", apath.string().c_str());
            out << form;
        }
        cout << "The templates were fetched successfully" << endl;
    */
    } catch(Exception &E) {
        usage(argv[0], E.what());
        return 1;
    }
    return 0;



    /*

        TQuery Qry(&OraSession);
        Qry.SQLText =
            "select data from images, images_data "
            "   where images.name = :name and "
            "   images.id = images_data.id "
            "order by "
            "   page_no";
        Qry.CreateVariable("name", otString, 
        return 0;
        */
    }

    int load_img(int argc,char **argv)
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
                    process(dir_itr->path());
            } else {
                process(full_path);
            }
        } catch(Exception &E) {
            usage(argv[0], E.what());
            return 1;
        }
        return 0;
    }
}
