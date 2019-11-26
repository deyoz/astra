#include "prn_forms_layout.h"
#include "oralib.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace std;
using namespace EXCEPTIONS;

TPrnFormsLayout prn_forms_layout;

const std::string &TPrnFormsLayout::msg_unavail_for_device(TDevOper::Enum op_type)
{
    return get(op_type, "msg_unavail_for_device");
}

const std::string &TPrnFormsLayout::msg_type_not_assigned(TDevOper::Enum op_type)
{
    return get(op_type, "msg_type_not_assigned");
}

const std::string &TPrnFormsLayout::msg_not_avail_for_unacc(TDevOper::Enum op_type)
{
    return get(op_type, "msg_not_avail_for_unacc");
}

const std::string &TPrnFormsLayout::msg_not_avail_for_crew(TDevOper::Enum op_type)
{
    return get(op_type, "msg_not_avail_for_crew");
}

const std::string &TPrnFormsLayout::get(TDevOper::Enum op_type, const std::string &param_name)
{
    LogTrace(TRACE5)
        << "TPrnFormsLayout::get("
        << DevOperTypes().encode(op_type)
        << ", "
        << param_name
        << ")";
    if(not items) {
        items = boost::in_place();
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "select op_type, param_name, param_value from prn_forms_layout where "
            "  param_name in ( "
            "   'msg_not_avail_for_crew', "
            "   'msg_type_not_assigned', "
            "   'msg_unavail_for_device', "
            "   'msg_not_avail_for_unacc' "
            ") ";
        Qry.Execute();
        for(; not Qry.Eof; Qry.Next())
            items.get()[DevOperTypes().decode(Qry.FieldAsString("op_type"))][Qry.FieldAsString("param_name")] = Qry.FieldAsString("param_value");
    }
    if((*items)[op_type][param_name].empty())
        throw Exception("prn_forms_layout: op_type '%s', param_name '%s' not found", DevOperTypes().encode(op_type).c_str(), param_name.c_str());
    return (*items)[op_type][param_name];
}
