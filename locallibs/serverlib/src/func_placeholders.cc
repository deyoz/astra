#include "func_placeholders.h"
#include <boost/algorithm/string/split.hpp>
#include <ctype.h>
#include "exception.h"

#define NICKNAME "DMITRYVM"
#include "slogger.h"

#define THROW_MSG(msg)\
    do {\
        std::ostringstream msgStream;\
        msgStream << msg;\
        throw comtech::Exception(STDLOG, __FUNCTION__, msgStream.str());\
    } while (0)

/*******************************************************************************
 * Таблица функций
 ******************************************************************************/

FuncPhMap_t& GetFuncPhMap()
{
    static FuncPhMap_t m;
    return m;
}

void RegisterFuncPh(const std::string& name, FuncPhProc proc)
{
    if (GetFuncPhMap().count(name))
        THROW_MSG("function \"" << name << "\" already registered");
    GetFuncPhMap().insert(FuncPhMap_t::value_type(name, proc));
}

namespace tok {

    std::string GetValue(
            const std::vector<Param>& params,
            const std::string& name,
            const std::string& defaultValue)
    {
        for (const Param& p:  params)
            if (p.name == name)
                return p.value;

        return defaultValue;
    }

    std::string GetValue_ThrowUndefined(
            const std::vector<Param>& params,
            const std::string& name)
    {
        for (const Param& p:  params)
            if (p.name == name)
                return p.value;

        THROW_MSG("named parameter \"" << name << "\" is required");
    }
    
    std::string Validate(const std::string& value, const std::string& validValues)
    {
        std::vector<std::string> validValuesList;
        boost::split(validValuesList, validValues, isspace);
        if (std::find(validValuesList.begin(), validValuesList.end(), value) == validValuesList.end())
            THROW_MSG("invalid value \"" << value << "\", valid values are: " << validValues);
        return value;
    }

    void ValidateParams(
            const std::vector<Param>& params,
            size_t minPositional,
            size_t maxPositional,
            const std::string& validNames)
    {
        const size_t posCount = PositionalValues(params).size();
        if (posCount < minPositional || posCount > maxPositional)
            THROW_MSG("invalid number of positional parameters ("
                    << posCount
                    << "), min = " << minPositional << ", max = " << maxPositional);

        std::vector<std::string> validNamesList;
        boost::split(validNamesList, validNames, isspace);
        for (const Param& p:  params)
            if (!p.name.empty() && std::find(validNamesList.begin(), validNamesList.end(), p.name) == validNamesList.end())
                THROW_MSG("unexpected named parameter \"" << p.name << "\", valid named parameters are: " << validNames);
    }

    std::vector<std::string> PositionalValues(const std::vector<Param>& params)
    {
        std::vector<std::string> result;
        for (const Param& p:  params)
            if (p.name.empty())
                result.push_back(p.value);
        return result;
    }
    
    std::vector<std::string> ForcePositionalValues(const std::vector<Param>& params)
    {
        std::vector<std::string> result;
        for (const Param& p:  params) {
            if (!p.name.empty())
                THROW_MSG("unexpected named parameter \"" << p.name << "\", only positional values are required");
            result.push_back(p.value);
        }
        return result;
    }

} /* namespace tok */

