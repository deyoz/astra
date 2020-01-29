#include <iostream>
#include <string>
#include <list>
#include <fstream>
#include <cstring>
#include <ctime>
#include <cctype>
#include <dirent.h>
#include <errno.h>
#include "jxtlib_db_callbacks.h"

#include <serverlib/cursctl.h>
#include "xml_tools.h"
#include "xmllibcpp.h"
#include "xml_cpp.h"
#include "xml_utils.h"

#define NICKNAME "MIKHAIL"
#include <serverlib/slogger.h>

using namespace std;

struct ILinks
{
    string type;
    string id;
    string lang;
    ILinks(const string& type_, const string& id_, const string& lang_ = "")
        : type(type_), id(id_), lang(lang_)
    {}
};

static void findIparts(xmlNodePtr node, std::list<ILinks> &ilinks)
{
    for (; node; node = node->next) {
        if (xmlStrcmp(node->name, "ipart") == 0) {
            for (xmlAttrPtr pr = node->properties; pr; pr = pr->next) {
                if (xmlStrcmp(pr->name, "id") == 0) {
                    string ipart_id = pr->children->content ? (const char*)pr->children->content : "";
                    if (!ipart_id.empty()) {
                        ilinks.push_back(ILinks("ipart", ipart_id));
                        //ilinks.push_back(ILinks("ppart",ipart_id+"_en","en"));
                    }
                    break;
                }
            }
        }
        findIparts(node->children, ilinks);
    }
}

static int findIfaceIparts(XmlDoc iDoc, std::list<ILinks> &ilinks)
{
    ilinks.clear();
    xmlNodePtr mainNode = iDoc->children; // <interface>
    xmlNodePtr winNode = findNode(mainNode, "window");

    xmlNodePtr node = winNode->children;
    findIparts(node, ilinks);

    return 0;
}

static bool isp(char c)
{
    return std::isspace(c);
}

static bool doAll(std::string const& filename, bool force_update)
{
    ifstream in(filename.c_str(), ifstream::binary);
    if (not in) {
        cerr << "cannot open input file '" << filename << "'" << endl;
        return false;
    }

    std::string descr;

    while (in.good()) {
        std::string tmp;
        std::getline(in, tmp);
        if(not std::all_of(tmp.begin(), tmp.end(), isp))
            descr += tmp += "\n";
    }
    in.close();

    XmlDoc iDoc = xml_parse_memory(descr);
    if (not iDoc) {
        cerr << "  Xml content must contain errors! Correct them and try again." << endl;
        return false;
    }
    xmlSetProp(iDoc->children, "ver", "");

    string type = (const char*)iDoc->children->name;
    string id = getStrPropFromXml(iDoc->children, "id");

    std::string buff = xml_dump(iDoc);
    if (buff.empty()) {
        cerr << "  xmlDocDumpMemory failed!" << endl;
        return false;
    }

    std::list<ILinks> ilinks;
    if (type == "interface" and findIfaceIparts(iDoc, ilinks)) {
        cerr << "  failed to get IfaceIparts..." << endl;
        return false;
    }

    if (!force_update) {
        std::string page;
        // We won't update data if it's the same - for better version management
        long version = jxtlib::JxtlibDbCallbacks::instance()->getXmlDataVer(type, id, false);
        string old_descr = jxtlib::JxtlibDbCallbacks::instance()->getXmlData(type, id, version);
        if (old_descr == buff) {
            cerr << "  " << type << ":" << id << ": no changes" << endl;
            return true;
        }
    }

    jxtlib::JxtlibDbCallbacks::instance()->insertXmlStuff(type, id, buff);

    if (type == "interface") {
        jxtlib::JxtlibDbCallbacks::instance()->deleteIfaceLinks(id);

        std::vector<jxtlib::IfaceLinks> ilinksDb;
        for (const auto &ilink: ilinks) {
            if (ilink.type == "ipart") {
                ilinksDb.push_back(jxtlib::IfaceLinks(ilink.id, ilink.type, "", false));
            }
        }
        jxtlib::JxtlibDbCallbacks::instance()->insertIfaceLinks(id, ilinksDb, 0);
    }

    cerr << "  " << type << ":" << id << endl;
    return true;
}

static int ext(const struct dirent* f, const char* ext4)
{
    size_t f_d_namlen = strlen(f->d_name); // f->d_namlen
    return (f->d_type == DT_REG or f->d_type == DT_UNKNOWN) and
        f_d_namlen > 4 and strcmp(f->d_name + f_d_namlen - strlen(ext4), ext4) == 0;
}

static int xml(const struct dirent* f)
{
    return ext(f, ".xml");
}

static int sql(const struct dirent* f)
{
    return ext(f, ".sql");
}

static void list_files(const char* dir_name, std::list<std::string>& files, int(*filter)(const struct dirent*))
{
    struct dirent** eps = 0;
    int n = scandir(dir_name, &eps, filter, alphasort);
    if (n >= 0) {
        std::string slash = "/";
        for (int i = 0; i < n; ++i) {
            files.push_back(dir_name + slash + eps[i]->d_name);
            free(eps[i]);
        }
        free(eps);
    } else {
        switch (errno) {
        case 0:
            return;
        case EACCES:
            throw std::runtime_error("Read permission is denied for the directory specified");
        case ENAMETOOLONG:
            throw std::runtime_error("Name is too long");
        case ENOENT:
            throw std::runtime_error("Directory does not exist");
        case ENOTDIR:
            throw std::runtime_error("Is not a directory");
        case ELOOP:
            throw std::runtime_error("Too many symbolic links were resolved while trying to look up the directory");
        case EMFILE:
            throw std::runtime_error("The process has too many files open");
        case ENFILE:
            throw std::runtime_error("Cannot have any additional open files at the moment");
        case ENOMEM:
            throw std::runtime_error("Not enough memory available");
        default:
            throw std::runtime_error("Unknown error");
        }
    }
}

static bool parse_args_and_get_files(int argc, char* argv[], int(*filter)(const struct dirent*), std::list<std::string>& files)
{
    if (strcmp(argv[0], "-d") == 0 || strcmp(argv[0], "--dirname") == 0) { // list files in directories
        for (int i = 1; i < argc; ++i) {
            try {
                list_files(argv[i], files, filter);
            } catch (const std::exception& e) {
                cerr << "failed on directory '" << argv[i] << "' :: " << e.what() << endl;
                return false;
            }
        }
    } else {
        for (int i = 0; i < argc; ++i)
            files.push_back(argv[i]);
    }

    if (files.empty()) {
        cerr << "empty filelist" << endl;
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------

template <class F> class DoSqlThingsHelper
{
    F f;
    bool proceed;

public:
    DoSqlThingsHelper(F&& _f) : f(std::forward<F>(_f)), proceed(true) {}
    bool result() const {
        return proceed;
    }
    void operator()(const std::string& arg) {
        if (not proceed)
            return;
        try {
            if (f(arg)) {
                jxtlib::JxtlibDbCallbacks::instance()->commit();
                return;
            }
        } catch (const std::exception& e) {
            cerr << e.what() << endl;
        }

        cerr << "failed on file'" << arg << "'" << endl;
        jxtlib::JxtlibDbCallbacks::instance()->rollback();
        proceed = false;
    }
};

template <class F> DoSqlThingsHelper<F> DoSqlThings(F&& f)
{
    return DoSqlThingsHelper<F>(std::forward<F>(f));
}

static bool doAllSql(const std::string& filename)
{
    ifstream in(filename.c_str(), ifstream::binary);
    if (not in) {
        cerr << "cannot open input file '" << filename << "'" << endl;
        return false;
    }

    std::string descr;

    while (in.good()) {
        std::string tmp;
        std::getline(in, tmp);
        if(not std::all_of(tmp.begin(), tmp.end(), isp))
            descr += tmp += "\n";
    }
    in.close();

    OciCpp::CursCtl("begin\n" + descr + "end;").exec();
    return true;
}

extern "C" int nosir_make_ifaces(int argc, char** argv)
{
    if (argc < 2) {
        cerr << "usage: fin_pwd [-f] filename(s)" << endl;
        return 1;
    }

    bool force_update = false;

    int argnum = 1;
    if (strcmp(argv[argnum], "-f") == 0 || strcmp(argv[argnum], "--force") == 0) { // force update
        force_update = true;
        ++argnum;
    }

    std::list<std::string> files;

    if (not parse_args_and_get_files(argc - argnum, argv + argnum, xml, files))
        return 1;

    bool result = for_each(files.begin(), files.end(),
            DoSqlThings([force_update](auto& f){ return doAll(f, force_update); }))
        .result();
    return result ? 0 : 1;
}

extern "C" int nosir_make_ifaces_data(int argc, char** argv)
{
    if (argc < 2) {
        cerr << "usage: fin_pwd [-f] filename(s)" << endl;
        return 1;
    }

    std::list<std::string> files;

    if (not parse_args_and_get_files(argc - 1, argv + 1, sql, files))
        return 1;

    bool result = for_each(files.begin(), files.end(),
            DoSqlThings([](auto& f){ return doAllSql(f); }))
        .result();
    return result ? 0 : 1;
}

