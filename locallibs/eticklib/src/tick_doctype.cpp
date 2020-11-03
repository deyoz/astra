#include <etick/tick_doctype.h>

namespace Ticketing {


const char *DocTypeElem::ElemName = "Document type";

DocType::DocType()
{
}

DocType::DocType(const std::string &typestr)
    :TypeElemHolder(typestr)
{
}

DocType::DocType(DocType::Type_t type)
    :TypeElemHolder(type)
{
}

bool DocType::isEmd() const
{
    return (*this == Ticketing::DocType::EmdA ||
            *this == Ticketing::DocType::EmdS);
}

bool DocType::isEticket() const
{
    return (*this == Ticketing::DocType::Ticket);
}

DocTypeElem::DocTypeElem(int codeI, const char *code, const char *desc) throw()
    :CodeListData_t(codeI, code, code, desc, desc)
{
}

} // namespace Ticketing

DESCRIBE_CODE_SET(Ticketing::DocTypeElem)
{
#define ADD_ONE(a, code, desc) \
    addElem( VTypes, Ticketing::DocTypeElem(Ticketing::DocType::a, code, desc))

    ADD_ONE(EmdA,   "J", "EMD-A");
    ADD_ONE(Mco,    "M", "Miscellaneous charge order");
    ADD_ONE(EmdS,   "Y", "EMD-S");
    ADD_ONE(Ticket, "T", "Ticket");
}
