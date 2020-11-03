#pragma once

#include <list>
#include <vector>

#include <serverlib/message.h>
#include <serverlib/period.h>
#include <coretypes/flight.h>
#include <coretypes/route.h>

namespace ssim {

struct Leg;
struct Route;

typedef std::pair<Period, Route> PeriodicRoute;
typedef std::vector<PeriodicRoute> PeriodicRoutes;

class DefaultValueHandler
{
protected:
    nsi::DepArrPoints target;
    bool byLeg;

    virtual Message setValue(const ct::Flight&, const Period&, ssim::PeriodicRoutes&, ct::LegNum) const;
    virtual Message setValue(const ct::Flight&, const Period&, ssim::PeriodicRoutes&, ct::SegNum) const;

    explicit DefaultValueHandler(const nsi::DepArrPoints&, bool);
public:
    virtual ~DefaultValueHandler();
    virtual Message setValue(const ct::Flight&, const Period&, ssim::PeriodicRoutes&) const;
    const nsi::DepArrPoints& getTarget() const;
};

using DefValueSetter = std::shared_ptr<DefaultValueHandler>;
using DefValueSetters = std::list<DefValueSetter>;

} //ssim
