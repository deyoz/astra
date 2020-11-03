/*
*  C++ Implementation: test_true_typeb
*
* Description:
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
*
*/
#ifdef XP_TESTING

#include "test_true_typeb.h"
#include "FreqTrElem.h"
#include <serverlib/str_utils.h>
#include <boost/regex.hpp>
#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>
#include <serverlib/xp_test_utils.h>
#include <serverlib/checkunit.h>

void initTests()
{
}

namespace
{
const char * test_msg =
"MAWRM3R\n\
.MOWRMSQ 211444\n\
MOWSQ 5ZM41P/24åéÇ/åéÄÖ29/íäè24åéÇ0224\n\
MAW3R 2V9T8G\n\
1GASIMOVA/HAGIGAT 1GASIMOVA/ZULFIYA 1GASIMOV/NIYAZI\n\
3R0711L02JUN08 DMEKVD HK3\n\
SSR OTHS 3R HK1 DMEKVD0711L02JUN-1GASIMOVA/HAGIGAT.FARE LOWPD\n\
SSR OTHS 3R///NUC203.45 RUB4925.00 TAX F  RUB370.00 EUR10.00 ZZ\n\
SSR OTHS 3R///RUB90.00 RUB90.00 SA RUB740.00 EUR20.00\n\
SSR TKNA 3R HK1 DMEKVD0711L02JUN-1GASIMOVA/HAGIGAT.66100475304\n\
SSR OTHS 3R HK1 DMEKVD0711L02JUN-1GASIMOVA/ZULFIYA.FARE -CH-35\n\
SSR OTHS 3R///NUC133.08 RUB3220.00 TAX F  RUB370.00 EUR10.00 ZZ\n\
SSR OTHS 3R///RUB90.00 RUB90.00 SA RUB740.00 EUR20.00\n\
SSR TKNA 3R HK1 DMEKVD0711L02JUN-1GASIMOVA/ZULFIYA.66100475305\n\
SSR OTHS 3R HK1 DMEKVD0711L02JUN-1GASIMOV/NIYAZI.FARE -CH-35\n\
SSR OTHS 3R///NUC133.08 RUB3220.00 TAX SA RUB740.00 EUR20.00 F\n\
SSR OTHS 3R///RUB370.00 EUR10.00 ZZ RUB90.00 RUB90.00\n\
SSR TKNA 3R HK1 DMEKVD0711L02JUN-1GASIMOV/NIYAZI.66100475306\n";
}

namespace typeb_parser
{
    using namespace boost;
bool ReclocTemplate::isItYours(const std::string &s, std::string::size_type *till) const
{
    LogTrace(TRACE3) << __FUNCTION__ <<  ": " << s;
    if(regex_match(s,
       regex("^[A-ZÄ-ü]{3}[A-ZÄ-ü0-9]{2}\\s*[A-ZÄ-ü0-9]{5,}.*$"), match_any))
    {
        return true;
    }
    return false;
}

TbElement * ReclocTemplate::parse(const std::string & text) const
{
    return new ReclocElem(text);
}

bool Name2Template::isItYours(const std::string &s, std::string::size_type *till) const
{
    LogTrace(TRACE3) << __FUNCTION__ <<  ": " << s;
    smatch what;
    if(regex_match(s,what,
       regex("^([0-9]{1,2}[A-ZÄ-ü]{2,48}/[A-ZÄ-ü ]{1,48})\\s*.*")))
    {
        LogTrace(TRACE3) << "what[1]=" << what[1].str() << "; size=" << what[1].str().size();
        *till = what[1].str().size();
        return true;
    }
    return false;
}

TbElement * Name2Template::parse(const std::string & text) const
{
    return new NameElem(StrUtils::rtrim(text));
}

bool SegmTemplate::isItYours(const std::string &s, std::string::size_type *till) const
{
    //3R0711L02JUN08 DMEKVD HK3
    LogTrace(TRACE3) << __FUNCTION__ <<  ": " << s;
    smatch what;
    if(regex_match(s,what,
       regex("^[A-ZÄ-ü0-9]{2,2}\\s*[0-9]{1,4}[A-ZÄ-ü]{1}" //3R0711L
               "[0-9]{1,2}[A-ZÄ-ü]{3}[0-9]{0,2}\\s*" //02JUN08
               "[A-ZÄ-ü0-9]{3}\\s*[A-ZÄ-ü0-9]{3}\\s*"//DMEKVD
               "[A-Z]{2}[0-9]{1,3}" //HK3
            ), match_any))
    {
        LogTrace(TRACE3) << "what[0]=" << what[0].str() << "; size=" << what[0].str().size();
        return true;
    }
    return false;
}

TbElement * typeb_parser::SegmTemplate::parse(const std::string & text) const
{
    return new SegmElem(text);
}

TbElement * SsrTemplate::parse(const std::string & text) const
{
    return new SsrElem(text);
}

}

typeb_parser::AIRIMP_template::AIRIMP_template()
    :typeb_template("", "AIRIMP")
{
    addElement(new ReclocTemplate)->setNecessary()->setMaxNumOfRepetition(2);
    addElement(new Name2Template)->
            setMayBeInline()->
            setMaxNumOfRepetition(99)->
            setNecessary();
    addElement(new SegmTemplate)->
            setNecessary()->
            setMaxNumOfRepetition(99);
    addElement(new SsrTemplate)->
            setMaxNumOfRepetition(99);
}

// #include "serverlib/xp_testing.h"
#include "typeb_message.h"
using namespace typeb_parser;
START_TEST(chk_airimp_parse1)
{
//     1GASIMOVA/HAGIGAT 1GASIMOVA/ZULFIYA 1GASIMOV/NIYAZI
    TypeBMessage AiMsg = TypeBMessage::parse(test_msg);
    LogTrace(TRACE3) << "total = " << AiMsg.total();
    fail_unless(AiMsg.total() == 4, "inv size");

    const TbListElement *list = AiMsg.find_listp("name");
    LogTrace(TRACE3) << "total names:  " << list->total();
    fail_unless(list->total() == 3, "inv total names!");

    const NameElem * name = TbElem_cast<const NameElem>(STDLOG,list->byNump(0));
    LogTrace(TRACE3) << "name element: " << name->source();
    fail_unless(name->source() == "1GASIMOVA/HAGIGAT", "inv name 1");

    name = TbElem_cast<const NameElem>(STDLOG,list->byNump(1));
    LogTrace(TRACE3) << "name element: " << name->source();
    fail_unless(name->source() == "1GASIMOVA/ZULFIYA", "inv name 2");

    name = TbElem_cast<const NameElem>(STDLOG,list->byNump(2));
    LogTrace(TRACE3) << "name element: " << name->source();
    fail_unless(name->source() == "1GASIMOV/NIYAZI", "inv name 3");
}
END_TEST;

START_TEST( chk_freq_travel )
{
    /*FreqTrElem *ft =*/ FreqTrElem::parse("FV 1232434534");
    
}
END_TEST;

namespace
{
    void init()
    {
        typeb_template::addTemplate(new AIRIMP_template());
    }

    void tear_down()
    {
    }
}


#define SUITENAME "tbparser"
TCASEREGISTER(init, tear_down)
{
    ADD_TEST( chk_airimp_parse1 );
    ADD_TEST( chk_freq_travel );
}
TCASEFINISH;
#endif /*XP_TESTING*/
