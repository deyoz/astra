//
// C++ Interface: tlg_types
//
// Description: ��直� ⨯� ������, ����騥 �⭮襭�� � ⥫��ࠬ���
//
#pragma once

#include <list>
#include <libtlg/tlgnum.h>
#include <serverlib/base_code_set.h>

namespace TlgHandling {
typedef unsigned routerid_t;
typedef unsigned tlg_type_t;
typedef unsigned tlg_subtype_t;

class TlgTypesList : public BaseTypeElem<int>
{
    typedef BaseTypeElem<int> CodeListData_t;
public:
    static const char *ElemName;
    TlgTypesList(int codeI, const char *code,
                 const char *ldesc,
                 const char *rdesc) throw()
    :CodeListData_t(codeI, code, code, rdesc, ldesc)
    {
    }
    virtual ~TlgTypesList(){}
};

class TlgType : public BaseTypeElemHolder<TlgTypesList>
{
public:
    typedef BaseTypeElemHolder<TlgTypesList> TypeElemHolder;
    typedef TypeElemHolder::TypesMap CouponStatMap;
    typedef std::list< TlgTypesList > listOfTlgTypes_t;

    TlgType():TypeElemHolder() {}
    TlgType(const TypeElemHolder &base) :
            TypeElemHolder(base)
    {
    }
    TlgType(int t):TypeElemHolder(t) {}

    enum {
        unknown=0,
        edifact,
        airimp,
    };
private:
};

/// @class TlgSubtype
/// ������ ����� ��� ���⨯�� ������� ⨯�� ⥫��ࠬ�
class TlgSubtype
{
public:
    /**
     * ������ ⨯ ⥫��ࠬ��
     * @return TlgType
     */
    virtual const TlgType tlgType() const = 0;

    /**
     * ��� ���⨯� - ETL/ETH ... etc
     * @return const char
     */
    virtual const char *code() const = 0;

    virtual ~TlgSubtype(){}
};

std::ostream  &operator << (std::ostream &s,
                            const TlgType &status);


}//namespace TlgHandling
