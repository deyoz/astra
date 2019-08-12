#ifndef _PRN_FORMS_LAYOUT_H
#define _PRN_FORMS_LAYOUT_H

#include <map>
#include <string>
#include "dev_consts.h"

class TPrnFormsLayout {
    private:
        typedef std::map<std::string, std::string> TParamMap;
        typedef std::map<ASTRA::TDevOper::Enum, TParamMap> TOpTypeMap;
        boost::optional<TOpTypeMap> items;
        const std::string &get(ASTRA::TDevOper::Enum op_type, const std::string &param_name);
    public:
        const std::string &msg_unavail_for_device(ASTRA::TDevOper::Enum op_type);
        const std::string &msg_not_avail_for_crew(ASTRA::TDevOper::Enum op_type);
        const std::string &msg_not_avail_for_unacc(ASTRA::TDevOper::Enum op_type);
        const std::string &msg_type_not_assigned(ASTRA::TDevOper::Enum op_type);
};

extern TPrnFormsLayout prn_forms_layout;

#endif
