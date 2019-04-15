#include "get_cls_upgrade.h"
#include "base_tables.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

TClsUpgradeType get_cls_upgrade(const std::string &cls, const std::string &compartment)
{
    TClsUpgradeType result = cutNone;
    if(cls != compartment) {
        TClasses &classes = (TClasses &)base_tables.get("classes");
        int cls_prior = ((const TClassesRow &)classes.get_row("code", cls)).priority;
        int compartment_prior = ((const TClassesRow &)classes.get_row("code", compartment)).priority;
        if(compartment_prior > cls_prior)
            result = cutUpgrade;
        else if(compartment_prior < cls_prior)
            result = cutDowngrade;
    }
    return result;
}
