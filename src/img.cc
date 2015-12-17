#include "img.h"
#include "boost/filesystem/operations.hpp"
#include <fstream>
#include "exceptions.h"
#include "oralib.h"
#include "serverlib/str_utils.h"

#define NICKNAME "DEN"
#include "serverlib/slogger.h"

namespace fs = boost::filesystem;
using namespace std;
using namespace EXCEPTIONS;

const int PAGE_SIZE = 4000; // last sym reserved for null terminator

namespace img {

    void test(string buffer)
    {
        vector<string> img;
        int page = 0;
        while(true) {
            size_t first = page * PAGE_SIZE;
            size_t last = (page + 1) * PAGE_SIZE;
            img.push_back(buffer.substr(first, last));
            if(last >= buffer.size()) break;
            page++;
        }

        cout << "img.size(): " << img.size() << endl;

        buffer.clear();
        for(vector<string>::iterator iv = img.begin(); iv != img.end(); iv++)
            buffer += *iv;

        ofstream out("utair_dup.bmp");
        if(out.good()) {
            out << buffer;
        }
    }

    // для cp866
    void process(const fs::path &apath)
    {
        if(not fs::is_regular_file(apath)) return;

        ostringstream fname;
        fname << apath.filename().c_str();
        ifstream in(apath.string().c_str(), std::ios::binary);
        if(!in.good())
            throw Exception("Cannot open file %s", fname.str().c_str());

        string buffer = StrUtils::b64_encode(
                string ((
                        std::istreambuf_iterator<char>(in)), 
                    (std::istreambuf_iterator<char>()))
                );


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
            size_t idx = page * PAGE_SIZE;
            Qry.SetVariable("page_no", page);
            Qry.SetVariable("data", buffer.substr(idx, PAGE_SIZE));
            Qry.Execute();
            if(idx + PAGE_SIZE >= buffer.size()) break;
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
                throw Exception("path is not a directory: %s", full_path.string().c_str());

            cout << full_path.filename().c_str() << endl;

            TQuery Qry(&OraSession);
            Qry.SQLText =
                "select name, data from images, images_data "
                "where "
                "   images.id = images_data.id "
                "order by "
                "   name, page_no";
            Qry.Execute();

            map<string, string> images;
            for(; not Qry.Eof; Qry.Next()) {
                string &data = images[Qry.FieldAsString("name")];
                data += Qry.FieldAsString("data");
            }

            for(map<string, string>::iterator im = images.begin(); im != images.end(); im++) {
                cout << "getting " << im->first << endl;
                fs::path apath = full_path / im->first;
                ofstream out(apath.string().c_str());
                if(!out.good())
                    throw Exception("Cannot open file %s", apath.string().c_str());
                out << StrUtils::b64_decode(im->second);
            }

            cout << "The images were fetched successfully" << endl;
        } catch(Exception &E) {
            usage(argv[0], E.what());
            return 1;
        }
        return 0;
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
