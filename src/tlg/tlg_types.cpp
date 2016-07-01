/*
*  C++ Implementation: tlg_types
*
* Description: ��直� ⨯� ������, ����騥 �⭮襭�� � ⥫��ࠬ���
*
*/
#include "tlg_types.h"

using namespace TlgHandling;

DESCRIBE_CODE_SET(TlgTypesList)
{
    addElem(VTypes, TlgTypesList(TlgType::unknown,   "",    "Unknown", "���������"));
    addElem(VTypes, TlgTypesList(TlgType::edifact,   "TPA", "Edifact", "Edifact"));
    addElem(VTypes, TlgTypesList(TlgType::airimp,    "TPB", "Type B" , "Type B"));
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
