#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <boost/algorithm/string.hpp>
#include <boost/optional/optional_io.hpp>

#include <serverlib/json_packer.h>
#include <serverlib/str_utils.h>
#include <serverlib/helpcpp.h>
#include <serverlib/logopt.h>
#include <serverlib/enum.h>

#include "packer.h"

#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

#ifdef TRACE5
#undef TRACE5
#endif // TRACE5
#define TRACE5  getRealTraceLev(99),STDLOG

static int get_subcls_num(unsigned char c)
{
    static const char subclasses[256] = {
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1,  3, 11,  5,  6, 23,  2, 21, 12, 25,  4, 13, 14, 15, 16, 24,
          1, 17,  0,  9, 18, 22, 19,  8, 20, 10,  7, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
          3,  5, 19, 21,  6, 23, 11, -1,  4, -1, 13, 14, 15, 16, 24,  2,
          0,  9, 18, 22,  1, 20, 12, -1,  7, -1, -1, 25, -1, 10,  8, 17,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };
    return subclasses[c];
}

static const char* rbdCodes[2] = {
    "RPFAJCDZWSYBHKLMNQTVXGUEOI",
    "êîèÄàÅÑòûëùÜñäãåçüíÇïÉìÖéõ"
};

template <typename It>
static bool is_unique(It beg, It end)
{
    unsigned int rs = 0L;
    for (It ri = beg; ri != end ; ++ri) {
        if ((rs & (1 << ri->get())) != 0) {
            return false;
        }
        rs = (rs | (1 << ri->get()));
    }
    return true;
}

static bool is_unique(const ct::Rbds& rbds)
{
    return is_unique(rbds.begin(), rbds.end());
}

namespace ct
{

std::string rbdCode(const ct::Rbd& rbd, Language l)
{
    return std::string(1, rbdCodes[l == ENGLISH ? 0 : 1][rbd.get()]);
}

boost::optional<ct::Rbd> rbdFromStr(const std::string& s)
{
    if (s.size() != 1)
        return boost::optional<ct::Rbd>();
    int ret = get_subcls_num(s[0]);
    return ret < 0 ? boost::optional<ct::Rbd>() : ct::Rbd(ret);
}

std::string cabinCode(const ct::Cabin& c, Language l)
{
    return rbdCode(ct::Rbd(c.get()), l);
}

boost::optional<ct::Cabin> cabinFromStr(const std::string& s)
{
    boost::optional<ct::Rbd> rbd(rbdFromStr(s));
    if (!rbd) {
        return boost::optional<ct::Cabin>();
    }
    return ct::Cabin(rbd->get());
}

template<typename C>
static boost::optional<C> rbds_from_string(const std::string& s)
{
    if (s.size() > size_t(MAX_RBD_VAL)) {
        return boost::none;
    }

    C out(s.size(), typename C::value_type(0));
    typename C::iterator i = out.begin();
    for(char c:  s) {
        const int r = get_subcls_num(c);
        if (r < 0) {
            return boost::none;
        }
        *i = typename C::value_type(r);
        ++i;
    }

    if (!is_unique(out.begin(), out.end())) {
        return boost::none;
    }
    return out;
}

template<typename C>
static std::string rbds_to_string(const C& cnt, Language l)
{
    char buf[64] = {0};
    ASSERT(cnt.size() < (sizeof(buf) / sizeof(buf[0])));

    unsigned int i = 0;
    for (typename C::const_iterator it = cnt.begin(), it_end = cnt.end(); it != it_end; ++it, ++i) {
        buf[i] = (rbdCodes[l == ENGLISH ? 0 : 1][it->get()]);
    }
    return std::string(buf, i);
}

boost::optional<ct::Rbds> rbdsFromStr(const std::string& s)
{
    return rbds_from_string<ct::Rbds>(s);
}

boost::optional<ct::Cabins> cabinsFromString(const std::string& s)
{
    return rbds_from_string<ct::Cabins>(s);
}

std::string cabinsCode(const ct::Cabins& cs, Language l)
{
    return rbds_to_string(cs, l);
}

std::string rbdsCode(const ct::Rbds& rs, Language l)
{
    return rbds_to_string(rs, l);
}

//TODO: to serverlib?
template<typename T>
T unified_intersection(const T& x, const T& y)
{
    T r;
    for (typename T::const_iterator xi = x.begin(), xe = x.end(); xi != xe; ++xi) {
        if (HelpCpp::contains(y, *xi)) {
            r.insert(r.end(), *xi);
        }
    }
    return r;
}
ct::Rbds intersection(const ct::Rbds& x, const ct::Rbds& y)
{
    return unified_intersection(x, y);
}

ct::Cabins intersection(const ct::Cabins& x, const ct::Cabins& y)
{
    return unified_intersection(x, y);
}
//#############################################################################
RbdOrder2::RbdOrder2()
{}

RbdOrder2::RbdOrder2(const ct::Rbds& rs)
    : order(rs)
{}

const ct::Rbds& RbdOrder2::rbds() const
{
    return order;
}

std::string RbdOrder2::toString(Language lang) const
{
    return rbdsCode(order, lang);
}

boost::optional<RbdOrder2> RbdOrder2::getRbdOrder(const std::string &str)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": [" << str << ']';
    // ‡†ß‡•Ë†•¨ Ø„·‚Æ© - ¢·• ‡†§® .ACV
    if (str.empty())
        return RbdOrder2();
    boost::optional<ct::Rbds> rbds = rbds_from_string<ct::Rbds>(str);
    if (!rbds) {
        LogTrace(TRACE1) << "invalid prbd " << str;
        return boost::optional<RbdOrder2>();
    }
    return RbdOrder2::create(*rbds);
}

boost::optional<RbdOrder2> RbdOrder2::create(const Rbds& rs)
{
    if (rs.empty()) {
        LogTrace(TRACE1) << "empty rbd set";
        return boost::optional<RbdOrder2>();
    }

    if (!is_unique(rs)) {
        LogTrace(TRACE1) << "duplication of rbds occured";
        return boost::optional<RbdOrder2>();
    }
    return RbdOrder2(rs);
}

boost::optional<RbdOrder2> RbdOrder2::filter(const ct::Rbds& filter) const
{
    const ct::Rbds rs = unified_intersection(order, filter);
    if (rs.empty()) {
        return boost::optional<RbdOrder2>();
    }
    return RbdOrder2(rs);
}

bool operator== (const ct::RbdOrder2& x, const ct::RbdOrder2& y)
{
    return x.rbds() == y.rbds();
}

boost::optional<RbdOrder2> intersection(const RbdOrder2& x, const RbdOrder2& y)
{
    return x.filter(y.rbds());
}
//#############################################################################
typedef std::pair<std::vector<char>, std::vector<IntOpt> > ConfigTemplate;

static ConfigTemplate extractBaseSubcls(const std::string& ord)
{
    LogTrace(TRACE5) << __FUNCTION__ << " " << ord;

    std::vector<char> bases(ord.begin(), ord.end());
    bases.erase(std::remove_if(bases.begin(), bases.end(), isdigit), bases.end());
    if (bases.size() == ord.size()) {
        return std::make_pair(bases, std::vector<IntOpt>(bases.size(), IntOpt()));
    }

    std::vector<std::string> numbers;
    boost::split(numbers, ord, boost::is_any_of(std::string(bases.begin(), bases.end())));
    numbers.erase(numbers.begin()); //it is empty string
    std::vector<IntOpt> sizes;
    for(const std::string& str : numbers) {
        if (str.empty()) {
            sizes.push_back(IntOpt());
            continue;
        }

        size_t pos = 0;
        const int num = std::stoi(str, &pos);
        if(pos != str.size()) {
            LogTrace(TRACE1) << "failed to extract number from airc string " << str;
            return std::make_pair(std::vector<char>(), std::vector<IntOpt>());
        }
        sizes.push_back(num);
    }
    ASSERT(bases.size() == sizes.size());
    return std::make_pair(bases, sizes);
}

static void cutPartsFromOrder(const std::string & order, std::string & scls, std::string & craft)
{
    //split PRBD[/PRBM][.ACV] into parts
    const size_t posDot = order.find('.');

    //extract PRBD, skip PRMB if any
    const std::string prbd = order.substr(0, posDot);
    scls = prbd.substr(0, prbd.find('/'));

    //cut number of seats from PRBD
    const std::vector<char> extr = extractBaseSubcls(scls).first;
    scls = std::string(extr.begin(), extr.end());

    //extract ACV
    if (posDot != std::string::npos) {
        craft = order.substr(posDot + 1);
        //cut VV and BB parts from ACV (not supported yet)
        craft = craft.substr(0, craft.find("VV"));
        craft = craft.substr(0, craft.find("BB"));
        //cut cargo codes
        craft = craft.substr(0, craft.find("LL"));
        craft = craft.substr(0, craft.find("PP"));
    }
}

static boost::optional<CabsConfig> get_config(const ConfigTemplate& t)
{
    const std::string cabs(t.first.begin(), t.first.end());
    const ConfigTemplate::second_type& sizes = t.second;

    if (cabs.size() != sizes.size()) {
        return boost::none;
    }

    boost::optional<ct::Cabins> cc = cabinsFromString(cabs);
    if (!cc || cc->size() != cabs.size()) {
        LogTrace(TRACE1) << "invalid cabins: " << cabs;
        return boost::none;
    }

    CabsConfig cs;
    for (size_t i = 0, n = cabs.size(); i < n; ++i) {
        cs.push_back(CabsConfig::value_type((*cc)[i], sizes[i]));
    }
    return cs;
}

boost::optional<RbdsConfig> getRbdsConfig(const std::string& str)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": [" << str << ']';

    if (str.empty()) {
        LogTrace(TRACE1) << "empty string";
        return boost::none;
    }

    std::string subclString, aircraftStr;
    cutPartsFromOrder(str, subclString, aircraftStr);

    std::vector<char> origBases;
    std::vector<IntOpt> sizes;

    if (aircraftStr.empty()) {
        if (subclString.empty()) {
            LogTrace(TRACE1) << "No one subclass found: [" << str << "]";
            return boost::none;
        }
    } else {
        //configuration found; get base subclasses from after-dot section
        ConfigTemplate pairs = extractBaseSubcls(aircraftStr);
        origBases = pairs.first;
        sizes = pairs.second;
        if (origBases.empty()) {
            LogTrace(TRACE1) << "No one subclass found: [" << str << "]";
            return boost::none;
        }
    }
    //-------------------------------------------------------------------------
    boost::optional<RbdOrder2> order = RbdOrder2::getRbdOrder(subclString);
    if (!order) {
        return boost::none;
    }

    boost::optional<CabsConfig> cs = get_config(std::make_pair(origBases, sizes));
    if (!cs) {
        return boost::none;
    }

    return RbdsConfig(*order, *cs);
}

std::string toString(const CabsConfig& cs, Language lang)
{
    std::string out;
    for(const CabsConfig::value_type& v:  cs) {
        out += cabinCode(v.first, lang);
        if (v.second) {
            out += HelpCpp::string_cast(*v.second);
        }
    }
    return out;
}
std::string toString(const RbdsConfig& rc, Language lang)
{
    return rc.second.empty() ? rc.first.toString(lang) : rc.first.toString(lang) + '.' + toString(rc.second, lang);
}
//#############################################################################
class RbdLayoutCache
{
    //TODO: unordered_map
    typedef std::map<std::string, RbdLayout> LayoutMap;

    static const size_t maxSize_ = 3000;

    LayoutMap data_;
public:
    static RbdLayoutCache& instance()
    {
        static RbdLayoutCache cache;
        return cache;
    }

    const RbdLayout* get(const std::string& s) const
    {
        LayoutMap::const_iterator i = data_.find(s);
        if (i != data_.end()) {
            return &i->second;
        }
        return NULL;
    }

    const RbdLayout& push(const std::string& s, const RbdLayout& r)
    {
        if (data_.size() >= maxSize_ && data_.find(s) == data_.end()) {
            //remove element with shortest string representation
            //LRU may be better but with additional overhead
            data_.erase(data_.begin());
        }
        data_.insert(LayoutMap::value_type(s, r));
        return r;
    }
};
//#############################################################################
template<typename C>
static boost::optional<ct::Cabin> find_cabin(const C& layout, const ct::Rbd& r)
{
    for(const typename C::value_type& v:  layout) {
        if (HelpCpp::contains(v.second, r)) {
            return v.first;
        }
    }
    return boost::optional<ct::Cabin>();
}

template<typename C>
static Rbds union_order(const C& rl)
{
    Rbds out;
    for(const typename C::value_type& v:  rl) {
        out.insert(out.end(), v.second.begin(), v.second.end());
    }
    return out;
}

RbdLayout::RbdLayout(const RbdsConfig& rc)
    : order(rc.first), cfg(rc.second)
{
}

RbdLayout::RbdLayout(const rbd_layout_map& rl, const RbdsConfig& rc)
    : order(rc.first), cfg(rc.second), layout(rl)
{
    ASSERT(!order.rbds().empty());
    ASSERT(!layout.empty());
    ASSERT(!cfg.empty());
}

bool RbdLayout::validLayout() const
{
    return !layout.empty();
}

const ct::RbdOrder2& RbdLayout::rbdOrder() const
{
    return order;
}

boost::optional<ct::RbdOrder2> RbdLayout::rbdOrder(const ct::Cabin& c) const
{
    return ct::RbdOrder2::create(rbds(c));
}

const CabsConfig& RbdLayout::config() const
{
    return cfg;
}

const rbd_layout_map& RbdLayout::rbdLayout() const
{
    return layout;
}

ct::Rbds RbdLayout::rbds() const
{
    return order.rbds();
}

ct::Rbds RbdLayout::rbds(const ct::Cabin& c) const
{
    ASSERT(validLayout());
    rbd_layout_map::const_iterator ri = layout.find(c);
    if (ri == layout.end()) {
        return ct::Rbds();
    }

    return ri->second;
}

bool RbdLayout::contains(const ct::Rbd& r) const
{
    return HelpCpp::contains(order.rbds(), r);
}

boost::optional<ct::Cabin> RbdLayout::cabin(const ct::Rbd& r) const
{
    ASSERT(validLayout());
    return find_cabin(layout, r);
}

ct::Cabins RbdLayout::cabins() const
{
    ct::Cabins cs;
    cs.reserve(cfg.size());
    std::transform(cfg.begin(), cfg.end(), std::back_inserter(cs), [](auto c){ return c.first; });
    return cs;
}

std::string RbdLayout::toString(Language lang) const
{
    if (!validLayout()) {
        return ct::toString(RbdsConfig(order, cfg), lang);
    }

    std::string out;
    for(const rbd_layout_map::value_type& v:  layout) {
        out += std::string(out.empty() ? 0 : 1, ' ') + cabinCode(v.first, lang) + ':' + rbdsCode(v.second, lang);
    }
    out += '.' + ct::toString(cfg, lang);
    return out;
}

std::string RbdLayout::toPlainString(Language lang) const
{
    return ct::toString(RbdsConfig(order, cfg), lang);
}

static boost::optional<rbd_layout::value_type> parseCab(const std::string& s)
{
    //expected string like Y:KLMN
    if (s.size() < 3) {
        LogTrace(TRACE1) << "Too short order part: [" << s << ']';
        return boost::none;
    }
    if (s[1] != ':') {
        LogTrace(TRACE1) << "Cannot find cabin/rbds delimiter in: [" << s << ']';
        return boost::none;
    }
    boost::optional<ct::Cabin> cab = ct::cabinFromStr(s.substr(0, 1));
    if (!cab) {
        LogTrace(TRACE1) << "Unknown cabin: " << s.substr(0, 1);
        return boost::none;
    }
    boost::optional<ct::Rbds> rbds = rbds_from_string<ct::Rbds>(s.substr(2));
    if (!rbds || rbds->empty()) {
        LogTrace(TRACE1) << "Invalid prbd: " << s.substr(2);
        return boost::none;
    }
    return rbd_layout::value_type(*cab, *rbds);
}

boost::optional<RbdLayout> RbdLayout::fromString(const std::string& s)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": [" << s << ']';

    if (s.empty()) {
        return boost::none;
    }

    if (const RbdLayout* r = RbdLayoutCache::instance().get(s)) {
        return *r;
    }

    if (s.find_first_of(" :") == std::string::npos) {
        //simple case: order + config
        if (boost::optional<RbdsConfig> rc = getRbdsConfig(s)) {
            return RbdLayoutCache::instance().push(s, RbdLayout(*rc));
        }
        return boost::none;
    }
    //-------------------------------------------------------------------------
    const size_t p = s.find('.');
    const std::string cabsOrd = s.substr(0, p);
    const std::string confs = (p == std::string::npos) ? std::string() : s.substr(p + 1);

    //read main part of order
    ct::Cabins cabs;
    rbd_layout_map layout;

    std::vector<std::string> strs;
    boost::split(strs, cabsOrd, boost::is_any_of(" "));
    for(const std::string& str:  strs) {
        boost::optional<rbd_layout::value_type> cab = parseCab(str);
        if (!cab) {
            return boost::none;
        }

        cabs.push_back(cab->first);
        ct::Rbds& rs = layout[cab->first];
        rs.insert(rs.end(), cab->second.begin(), cab->second.end());
    }

    //get configuration
    CabsConfig cc;
    if (!confs.empty()) {
        boost::optional<CabsConfig> cs = get_config(extractBaseSubcls(confs));
        if (!cs) {
            return boost::none;
        }
        cc = *cs;
    }
    //if no config found use cabin order as-is
    if (cc.empty()) {
        if (cabs.size() != layout.size()) {
            LogTrace(TRACE1) << "Duplicated cabin without configuration: " << s;
            return boost::none;
        }
        for(ct::Cabin c:  cabs) {
            cc.push_back(CabsConfig::value_type(c, IntOpt()));
        }
    }

    //create ordered rbds by ordered cabins
    ct::Rbds order;
    for(const CabsConfig::value_type& c:  cc) {
        const ct::Rbds& rs = layout[c.first];
        order.insert(order.end(), rs.begin(), rs.end());
    }

    if (boost::optional<RbdLayout> out = RbdLayout::create(layout, order, cc)) {
        return RbdLayoutCache::instance().push(s, *out);
    }
    return boost::none;
}

bool RbdLayout::operator== (const RbdLayout& x) const
{
    return this->order == x.order && this->layout == x.layout && this->cfg == x.cfg;
}

boost::optional<RbdLayout> RbdLayout::filter(const ct::Rbds& rbds) const
{
    LogTrace(TRACE5) << __FUNCTION__ << ": [" << this->toString() << "] by [" << rbdsCode(rbds) << ']';

    boost::optional<RbdOrder2> ord = order.filter(rbds);
    if (!ord) {
        return boost::optional<RbdLayout>();
    }
    if (layout.empty()) {
        return RbdLayout::partialCreate(RbdsConfig(*ord, cfg));
    }

    CabsConfig cc = cfg;
    rbd_layout_map rm;
    for(const rbd_layout_map::value_type& v:  layout) {
        ct::Rbds rs = unified_intersection(v.second, rbds);
        if (!rs.empty()) {
            rm.insert(rbd_layout_map::value_type(v.first, rs));
        } else {
            cc.erase(
                std::remove_if(cc.begin(), cc.end(), [&v](auto c){ return c.first == v.first; }),
                cc.end()
            );
        }
    }

    return RbdLayout::create(rm, ord->rbds(), cc);
}

boost::optional<RbdLayout> RbdLayout::operator& (const RbdLayout& r) const
{
    boost::optional<RbdOrder2> ord = intersection(this->order, r.order);
    if (!ord) {
        return boost::optional<RbdLayout>();
    }

    CabsConfig cc;
    for(const CabsConfig::value_type& v:  this->cfg) {
        auto ci = std::find_if(r.cfg.begin(), r.cfg.end(), [&v](auto c){ return c.first == v.first; });
        if (ci != r.cfg.end()) {
            //FIXME: Á‚Æ §•´†‚Ï · ‡†ß¨•‡Æ¨ ™†°®≠Î?
            cc.push_back(v);
        }
    }

    if (!this->validLayout() || !r.validLayout()) {
        return RbdLayout::partialCreate(RbdsConfig(*ord, cc));

    }

    rbd_layout_map rm;
    for(const rbd_layout_map::value_type& v:  this->layout) {
        ct::Rbds rs = unified_intersection(v.second, r.rbds(v.first));
        if (!rs.empty()) {
            rm.insert(rbd_layout_map::value_type(v.first, rs));
        } else {
            cc.erase(
                std::remove_if(cc.begin(), cc.end(), [&v](auto c){ return c.first == v.first; }),
                cc.end()
            );
        }
    }
    return RbdLayout::create(rm, ord->rbds(), cc);
}
//#############################################################################
boost::optional<RbdLayout> RbdLayout::create(const rbd_layout_map& rl, const Rbds& rs, const CabsConfig& cc)
{
    boost::optional<RbdOrder2> order = RbdOrder2::create(rs);
    if (!order) {
        return boost::optional<RbdLayout>();
    }

    if (rl.empty() || rl.size() != cc.size()) {
        LogTrace(TRACE1) << "Invalid cabs count: " << rl.size() << " vs " << cc.size();
        return boost::optional<RbdLayout>();
    }

    ct::Rbds rbds;
    for(const CabsConfig::value_type& v:  cc) {
        const rbd_layout_map::const_iterator ri = rl.find(v.first);
        if (ri == rl.end()) {
            LogTrace(TRACE1) << "Cabin not found in layout: " << cabinCode(v.first);
            return boost::optional<RbdLayout>();
        }
        rbds.insert(rbds.end(), ri->second.begin(), ri->second.end());
    }
    if (!is_unique(rbds)) {
        LogTrace(TRACE1) << "Duplicated rbd found";
        return boost::optional<RbdLayout>();
    }
    if (std::set<ct::Rbd>(rs.begin(), rs.end()) != std::set<ct::Rbd>(rbds.begin(), rbds.end())) {
        LogTrace(TRACE1) << "Layout misfits rbds";
        return boost::optional<RbdLayout>();
    }
    return RbdLayout(rl, RbdsConfig(*order, cc));
}

RbdLayout RbdLayout::partialCreate(const ct::RbdsConfig& r)
{
    return RbdLayout(r);
}

std::ostream& operator<<(std::ostream& os, const ct::RbdLayout& l)
{
    return os << l.toString();
}

} // ct

#ifdef XP_TESTING

void init_rbd_tests() {}

#include <serverlib/checkunit.h>

using namespace ct;

START_TEST(rbd_parse2)
{
    fail_if(static_cast<bool>(getRbdsConfig("FY.FCM")) == false, "order not parsed");
    fail_unless(static_cast<bool>(getRbdsConfig("GSFGY.FY")) == false, "wrong order parsed");

    boost::optional<RbdsConfig> ord;
    
    ord = getRbdsConfig("ABCD.C10");
    fail_unless(toString(*ord) == "ABCD.C10");

    ord = getRbdsConfig("CIZDRWXYSMQBHKNVJLTGOUE.C16Y162");
    fail_unless(toString(*ord) == "CIZDRWXYSMQBHKNVJLTGOUE.C16Y162");

    ord = getRbdsConfig("FIZDRWXYSMQBHKNVJLTGOUE/RFG.FWYVV3G");
    fail_unless(toString(*ord) == "FIZDRWXYSMQBHKNVJLTGOUE.FWY");

    ord = getRbdsConfig("CGHFABYQ.CFY");
    fail_unless(toString(*ord) == "CGHFABYQ.CFY");

    ord = getRbdsConfig("BCDFLMKUIVYP.B20L30K30V40");
    fail_unless(toString(*ord) == "BCDFLMKUIVYP.B20L30K30V40");

    fail_unless(!RbdLayout::fromString("C:C Y:Y.YSNK.CY"));
    fail_unless(RbdLayout::fromString("C:C Y:YSNK.CY")->toString() == "C:C Y:YSNK.CY");
}
END_TEST
#define SUITENAME "coretypes"
TCASEREGISTER(0, 0)
{
    ADD_TEST(rbd_parse2);
}
TCASEFINISH

#endif // XP_TESTING
