/*
*  C++ Implementation: tlg_types
*
* Description: ��直� ⨯� ������, ����騥 �⭮襭�� � ⥫��ࠬ���
*
*/
#include "tlg_types.h"

template <> BaseTypeElemHolder< TlgHandling::TlgTypesList >::TypesMap
        BaseTypeElemHolder< TlgHandling::TlgTypesList >::VTypes =
        BaseTypeElemHolder< TlgHandling::TlgTypesList >::TypesMap();
template <> bool BaseTypeElemHolder<TlgHandling::TlgTypesList>::initialized = false;
template <> void BaseTypeElemHolder<TlgHandling::TlgTypesList>::init()
{
    using namespace TlgHandling;
    addElem( TlgTypesList(TlgType::unknown,   "",    "Unknown", "���������"));
    addElem( TlgTypesList(TlgType::edifact,   "TPA", "Edifact", "Edifact"));
    addElem( TlgTypesList(TlgType::airimp,    "TPB", "Type B" , "Type B"));
}

namespace TlgHandling
{
const char *TlgTypesList::ElemName = "Tlg type";

std::ostream & operator <<(std::ostream & s, const TlgType & type)
{
    s << type->code() << " (" << type->description() << ")";
    return s;
}
}
