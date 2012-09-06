#include "config.h"
#ifdef XP_TESTING
#include <stddef.h>
#include <list>
#include <string>

#include <boost/foreach.hpp>

#include <serverlib/xp_test_utils.h>
#include <serverlib/checkunit.h>
#include <serverlib/str_utils.h>

#include "xp_testing.h"

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>

namespace xp_testing
{

class TestStringsImpl
{
    friend class TestStrings;
private:
    std::list<std::string> match_list;

};

TestStrings::TestStrings(TestStrings const &s)
{
    pl = new TestStringsImpl;
    *pl = *s.pl;
}

TestStrings::TestStrings(const std::string& tst)
{
    pl = new TestStringsImpl;

    std::string line;
    std::istringstream strs(tst);
    while( std::getline(strs, line) ) {
        this->add(line);
    }
}

TestStrings& TestStrings::operator=(TestStrings const &s)
{   
    TestStrings tmp(s);
//    std::swap(tmp.pl->l,this->pl->l);
    return *this;
}

TestStrings::TestStrings()
{
    pl = new TestStringsImpl;
}

TestStrings::~TestStrings()
{
    delete pl;
}

TestStrings& TestStrings::add(std::string const &s)
{
    pl->match_list.push_back(s);
    return *this;
}

std::string TestStrings::show_mismatch(std::string const &answer) const
{
    size_t pos = 0;
    size_t pos_prev = 0;
    BOOST_FOREACH(std::string const &s, pl->match_list) {
        pos_prev = pos;
        pos = answer.find(s, pos);
        if (pos == std::string::npos) {
            while(pos_prev < answer.size() && (answer[pos_prev] == ' ' || answer[pos_prev] == '\n'))
                pos_prev ++;
            std::string match_substring = answer.substr(pos_prev, answer.find("\n", pos_prev) - pos_prev);
            match_substring = StrUtils::rtrim(match_substring);
            return "Substring mismatch:"
                    "\n expected  >>>" + s + "<<<"
                    "\n we've got >>>" + match_substring + "<<<";
        }
        pos += s.length();
    }
    return "";
}

std::string TestStrings::check(std::string const & answer) const
{
    size_t pos = 0;
    BOOST_FOREACH(std::string const &s, pl->match_list) {
        pos = answer.find(s, pos);
        if (pos == std::string::npos) {
            return s;
        }
        pos += s.length();
    }
    return "";
}

std::string TestStrings::find_all(std::string const & answer) const
{
    std::map<std::string,size_t> find_pos; 
    BOOST_FOREACH(std::string const &s, pl->match_list) {
        std::map<std::string,size_t>::iterator i_find_pos=find_pos.find(s);
        size_t pos = (i_find_pos==find_pos.end()?0:i_find_pos->second);
        pos = answer.find(s, pos);
        if (pos == std::string::npos) {
            return s;
        }
        if (i_find_pos==find_pos.end())
          find_pos.insert(std::make_pair(s,pos+s.length()));
        else
          i_find_pos->second=pos+s.length();
    }
    return "";
}

}//namespace xp_testing

#endif//XP_TESTING
