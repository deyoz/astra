#if HAVE_CONFIG_H
#endif

#ifdef SERVERLIB_ADDR2LINE

#include <bfd.h>
#include <libiberty.h>

#include <string>
#include <sstream>

#define NICKNAME "NONSTOP"
#include "slogger.h"

#include "exception.h"

namespace
{
static std::string Addr2lineFilename;

struct Addr2lineHelper
{
    asymbol **syms;
    bfd_vma pc;
    const char *filename;
    const char *functionname;
    unsigned int line;
    int found;
};

static void find_address_in_section(bfd* abfd, asection* section, PTR data)
{
    if (!data) {
        return;
    }
    Addr2lineHelper* a2lh = reinterpret_cast<Addr2lineHelper*>(data);

    if (a2lh->found) {
        return;
    }

    if ((bfd_get_section_flags(abfd, section) & SEC_ALLOC) == 0) {
        return;
    }

    bfd_vma vma = bfd_get_section_vma(abfd, section);
    if (a2lh->pc < vma) {
        return;
    }
    
    a2lh->found = bfd_find_nearest_line(abfd, section, a2lh->syms, a2lh->pc - vma, &(a2lh->filename), &(a2lh->functionname), &(a2lh->line));
}

class Addr2line
{
public:
    class Exception
        : public comtech::Exception
    {
    public:
        Exception(const std::string& msg) : comtech::Exception(msg) {}
        virtual ~Exception() throw() {}
    };
public:
    static Addr2line& instance()
    {
        static Addr2line* ptr = NULL;
        if (!ptr)
            ptr = new Addr2line(Addr2lineFilename);
        return *ptr;
    }
    std::string translateAddress(const std::string& addr)
    {
        Addr2lineHelper a2lh = {0};
        a2lh.pc = bfd_scan_vma(addr.c_str(), NULL, 16);
        a2lh.syms = syms_;
        bfd_map_over_sections(abfd_, find_address_in_section, (PTR)(&a2lh));
        
        if (!a2lh.found)
            return "";
        std::string file = (a2lh.filename) ? a2lh.filename : "";;
        if (file == "")
            return "";
        std::stringstream str;
        str << file << ":" << a2lh.line;
        return str.str();
    }
private:
    Addr2line(const std::string& filename)
        : filename_(filename)
    {
        bfd_init();

        abfd_ = bfd_openr(filename_.c_str(), NULL);
        if (abfd_ == NULL)
            throw Exception("bfd_openr failed for " + filename_);

        if (bfd_check_format(abfd_, bfd_archive))
            throw Exception("bfd_check_format failed");

        char **matching = NULL;
        if (! bfd_check_format_matches(abfd_, bfd_object, &matching)) {
            if (bfd_get_error() == bfd_error_file_ambiguously_recognized) {
                free(matching);
            }
            throw Exception("bfd_check_format_matches failed");
        }

        slurpSymtab(abfd_);
    }
    ~Addr2line()
    {
        bfd_close(abfd_);
    }
    void slurpSymtab(bfd* abfd)
    {
        if ((bfd_get_file_flags(abfd) & HAS_SYMS) == 0)
            throw Exception("bfd_get_file_flags(abfd) & HAS_SYMS != 0");

        const long storage = bfd_get_symtab_upper_bound(abfd);
        if (storage < 0) {
            std::stringstream ss("bfd_get_symtab_upper_bound failed: ");
            ss << storage;
            throw Exception(ss.str());
        }

        syms_ = (asymbol**)malloc(storage);

        const long symcount = bfd_canonicalize_symtab(abfd, syms_);
        if (symcount < 0)
            throw Exception("bfd_canonicalize_symtab failed");
    }
    
    std::string filename_;
    bfd* abfd_;
    asymbol **syms_;
};

}


void init_addr2line(const std::string& filename)
{
    Addr2lineFilename = filename;
    Addr2line::instance();
}

std::string addr2line(const std::string& addr)
{
    std::string res = Addr2line::instance().translateAddress(addr);
    return (res != "") ? res : addr;
}


#endif // SERVERLIB_ADDR2LINE

