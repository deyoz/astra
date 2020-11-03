#ifndef CORETYPES_PREDPOINT_H
#define CORETYPES_PREDPOINT_H

#include <string>
#include <iosfwd>
#include <boost/optional.hpp>

namespace ct
{

class PredPoint
{
public:
    static boost::optional<PredPoint> create(const std::string&);

    const std::string& str() const;     // e.g: MOWU6, MOWBEK
    std::string airimpStr() const;      // e.g.: MOWU6, MOW/BEK (with oblique for 3-char airlines)
    std::string cityPortStr() const;
    std::string airlineStr() const;

    bool operator<(const PredPoint& rhp) const;
    bool operator==(const PredPoint& rhp) const;
    bool operator!=(const PredPoint& rhp) const { return !(*this == rhp); }
private:
    explicit PredPoint(const std::string&);
    std::string predPoint_;
};
std::ostream& operator<<(std::ostream&, const PredPoint&);


} // ct

#endif /* CORETYPES_PREDPOINT_H */

