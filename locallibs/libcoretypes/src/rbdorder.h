#ifndef CORETYPES_RBDORDER_H
#define CORETYPES_RBDORDER_H

#include <map>
#include <vector>
#include <string>

#include <serverlib/lngv.h>
#include <serverlib/rip_validators.h>
#include <serverlib/rip.h>

#define MAX_RBD_VAL 27
#define MAX_CABIN_VAL 27

namespace ct
{

DECL_RIP_LESS(Rbd, unsigned, 27);
DECL_RIP_LESS(Cabin, unsigned, 27);

typedef std::vector<Rbd> Rbds;
typedef std::vector<Cabin> Cabins;
std::string rbdCode(const ct::Rbd&, Language l = ENGLISH);
boost::optional<ct::Rbd> rbdFromStr(const std::string&);
boost::optional<ct::Rbds> rbdsFromStr(const std::string&);
std::string rbdsCode(const ct::Rbds&, Language = ENGLISH);

std::string cabinCode(const ct::Cabin&, Language l = ENGLISH);
boost::optional<ct::Cabin> cabinFromStr(const std::string&);
std::string cabinsCode(const ct::Cabins&, Language = ENGLISH);
boost::optional<ct::Cabins> cabinsFromString(const std::string&);

ct::Rbds intersection(const ct::Rbds&, const ct::Rbds&);
ct::Cabins intersection(const ct::Cabins&, const ct::Cabins&);

typedef boost::optional<int> IntOpt;

class RbdOrder2
{
    ct::Rbds order;
    RbdOrder2(const ct::Rbds&);
public:
    RbdOrder2();

    const ct::Rbds& rbds() const;

    std::string toString(Language lang = ENGLISH) const;

    boost::optional<RbdOrder2> filter(const ct::Rbds&) const;

    static boost::optional<RbdOrder2> getRbdOrder(const std::string&);
    static boost::optional<RbdOrder2> create(const ct::Rbds&);
};
bool operator== (const ct::RbdOrder2&, const ct::RbdOrder2&);
boost::optional<RbdOrder2> intersection(const RbdOrder2&, const RbdOrder2&);

typedef std::vector< std::pair< ct::Cabin, IntOpt > > CabsConfig;
typedef std::pair<RbdOrder2, CabsConfig> RbdsConfig;

boost::optional<RbdsConfig> getRbdsConfig(const std::string&);
std::string toString(const CabsConfig&, Language lang = ENGLISH);
std::string toString(const RbdsConfig&, Language lang = ENGLISH);

typedef std::vector< std::pair< ct::Cabin, ct::Rbds> > rbd_layout;
typedef std::map< ct::Cabin, ct::Rbds > rbd_layout_map;

class RbdLayout
{
    RbdOrder2 order;          //полный набор и порядок подклассов
    CabsConfig cfg;          //конфигурация
    rbd_layout_map layout;   //раскладка по кабинам

    RbdLayout(const RbdsConfig&);
    RbdLayout(const rbd_layout_map&, const RbdsConfig&);
public:
    bool validLayout() const;

    // набор подклассов по кабине
    boost::optional<ct::RbdOrder2> rbdOrder(const ct::Cabin&) const;

    const RbdOrder2& rbdOrder() const;
    const CabsConfig& config() const;
    const rbd_layout_map& rbdLayout() const;

    ct::Rbds rbds() const;
    ct::Rbds rbds(const ct::Cabin&) const;

    bool contains(const ct::Rbd&) const;

    // кабина к которой относится подкласс
    boost::optional<ct::Cabin> cabin(const ct::Rbd&) const;
    // список кабин
    ct::Cabins cabins() const;

    // инициализация as-is + проверка на валидность
    static boost::optional<RbdLayout> create(const rbd_layout_map&, const Rbds&, const CabsConfig&);
    // чистый порядок, без раскладки
    static RbdLayout partialCreate(const RbdsConfig&);

    std::string toString(Language lang = ENGLISH) const;
    std::string toPlainString(Language lang = ENGLISH) const;
    static boost::optional<RbdLayout> fromString(const std::string&);

    bool operator== (const RbdLayout&) const;

    boost::optional<RbdLayout> filter(const ct::Rbds&) const;

    boost::optional<RbdLayout> operator& (const RbdLayout&) const;
};
std::ostream& operator<<(std::ostream& os, const ct::RbdLayout&);

} // ct

#endif /* CORETYPES_RBDORDER_H */

