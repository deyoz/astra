#ifndef _SERVERLIB_COPYTABLE_H_
#define _SERVERLIB_COPYTABLE_H_
#include <boost/variant.hpp>
#include "cursctl.h"
#include <vector>
#include <string>
#include <list>

namespace OciCpp {
typedef  boost::variant<unsigned long long,std::string,boost::posix_time::ptime> threeTypes;


class threeTypesBinder : public boost::static_visitor<> {
    OciCpp::CursCtl & curs_;
    std::string bindVar_;
    public:
    threeTypesBinder( OciCpp::CursCtl & curs, std::string const & bindVar):curs_(curs),bindVar_(bindVar)
    {
    }
    void operator() (unsigned long long const &l)
    {
        curs_.bind(bindVar_,l);
    }
    void operator() (std::string  const &s)
    {
        curs_.bind(bindVar_,s);
    }
    void operator() (boost::posix_time::ptime  const &d)
    {
        curs_.bind(bindVar_,d);
    }


};


class CopyTable
{
public:
    struct FdescMore {
        std::string col_name;
        threeTypes data;
        FdescMore( std::string const &s,threeTypes const &d) : col_name(s),data(d){}
        std::string bindvar() const
        {
            return ":"+col_name;
        }
    };
    
    CopyTable(OciSession&, const std::string& table1, const std::string & table2, std::list<FdescMore> const &moreFields);
    void exec(int loglevel, const char* nick, const char* file, int line);
    std::string buildSelectList ();
    
private:
    struct Fdesc {
        std::string col_name;
        int data_type;
        int data_len;
        int alloc_len;
        char *ptr;
        sb2 *indp;
        std::string bindvar;
    };
    OciSession& sess_;
    std::string table1_;
    std::string table2_;
    mutable std::list<Fdesc> fields_;
    std::list<FdescMore>  const &moreFields_;
    std::vector<char> buf;
    std::vector<sb2> indbuf;
    std::string buildSelect ();
    std::string buildInsert ();
    mutable std::string select_query;
    mutable std::string insert_query;
};
}

#endif
