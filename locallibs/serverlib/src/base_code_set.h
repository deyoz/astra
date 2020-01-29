/* 2006 by Roman Kovalev */
/* roman@pike.dev.sirena2000.ru */
#pragma once

#include <string.h>
#include <map>
#include <memory>
#include <ostream>
#include <algorithm>

#include "string_cast.h"
#include "exception.h"
#include "lngv.h"

//typedef unsigned int CodeInt_t;

///@class NoSuchCode
///@brief Если не найден элемент по коду
struct NoSuchCode : public comtech::Exception
{
    NoSuchCode(const char* n, const char* f, int l, const std::string &wht);
};

///template <class Tp,class __BaseIntType = int>
///@class BaseTypeElem
///@brief Базовый тип элемента списка
template <class Tp,class __BaseIntType = int> class BaseTypeElem
{
    Tp Type;
    const char * RCode;
    const char * LCode;
    const char * RDescription;
    const char * LDescription;
    BaseTypeElem()
        :RCode(0),LCode(0),RDescription(0), LDescription(0)
    {
    }
public:
    typedef Tp BaseType;
    BaseTypeElem(Tp codeI,
                   const char *rcode,
                   const char *lcode,
                   const char *rdesc,
                   const char *ldesc)
        :
        Type(codeI),
        RCode(rcode),
        LCode(lcode),
        RDescription(rdesc),
        LDescription(ldesc)
    {
    }

    inline Tp type () const { return Type; }
    inline Tp codeInt () const { return type(); }

    /**
     * @brief Код элемента
     * @param l language - default ENGLISH (must be!)
     * @return const char *
     */
    virtual inline const char * code(Language l=ENGLISH) const
    {
        return ((l == RUSSIAN)?RCode:LCode);
    }
    virtual inline const char * description(Language l=ENGLISH) const
    {
        return ((l == RUSSIAN)?RDescription:LDescription);
    }
    virtual __BaseIntType toBaseType() const
    {
        return static_cast<__BaseIntType> (type());
    }
    virtual ~BaseTypeElem(){}
};

///@class BaseTypeElemHolder
///@brief Контейнер базовых элементов
template <typename ElemT> class BaseTypeElemHolder
{
protected:
    ElemT *entry()
    {
        if(!Elem)
        {
            throw comtech::Exception("Use of uninitialized element");
        }
        return Elem.get();
    }

public:
    typedef std::map<typename ElemT::BaseType,ElemT> TypesMap;

    BaseTypeElemHolder(){}
    BaseTypeElemHolder(const ElemT *elem)
    {
        Elem = std::make_shared<ElemT>(*elem);
    }
    BaseTypeElemHolder(typename ElemT::BaseType type)
    {
        typename TypesMap::const_iterator it = typesList().find(type);
        if(it == typesList().end())
        {
            // no data ...
            throw NoSuchCode("ROMAN",__FILE__,__LINE__,"Unknown " + std::string(ElemT::ElemName) + "code " + HelpCpp::string_cast(type));
        }
        Elem = std::make_shared<ElemT>(it->second);
    }

    BaseTypeElemHolder(const char *code, Language l)
    {
        *this = initByUserSearch(find_by_code(code,l));
    }
    BaseTypeElemHolder(const char *code)
    {
        *this = initByUserSearch(find_by_code(code));
    }

    BaseTypeElemHolder(const std::string &code,Language l)
    {
        *this = BaseTypeElemHolder (code.c_str(),l);
    }
    BaseTypeElemHolder(const std::string &code)
    {
        *this = BaseTypeElemHolder (code.c_str());
    }

    static const TypesMap &typesList()
    {
        static TypesMap VTypes;
        static bool initialized = false;
        if(!initialized){
            init(VTypes);
            initialized = true;
        }
        return VTypes;
    }

    ///@brief Доступ к полям элемента
    const ElemT *operator -> () const
    {
        if(!Elem)
        {
            throw comtech::Exception("Use of uninitialized element");
        }
        return Elem.get();
    }

    /**
     * @brief Проверка на инициализированность
     */
    explicit operator bool() const
    {
        return not not Elem;
    }

    bool operator == (const BaseTypeElemHolder &cmp) const
    {
        return Elem && cmp && Elem->type() == cmp->type();
    }
    bool operator != (const BaseTypeElemHolder &cmp) const
    {
        return ! operator == (cmp);
    }

    /**
     * Проверка на наличие кода
     * @param str value to check
     * @return true - если существует
     */
    static bool checkExistence(const std::string &str)
    {
        return checkExistence(str.c_str());
    }

    /**
     * Проверка на наличие кода
     * @param str value to check
     * @return true - если существует
     */
    static bool checkExistence(const char *str)
    {
        return std::any_of(typesList().begin(), typesList().end(), find_by_code(str));
    }

    virtual ~BaseTypeElemHolder(){}
protected:
    struct userFind : public std::unary_function<typename TypesMap::value_type, bool>
    {
        virtual std::string dataName() const = 0;
        virtual std::string dataStr() const = 0;
        virtual ~userFind(){}
    };

    struct find_by_code : public userFind
    {
        const char *code;
        Language lang;
        bool strict_lang; // Сравнивать точно по коду или нет
        find_by_code(const char *c, Language l):
                code(c),
                lang(l),
                strict_lang(true)
        {
        }
        find_by_code(const char *c):
                code(c),
                lang(ENGLISH),
                strict_lang(false)
        {
        }
        bool operator () (const typename TypesMap::value_type &e) const
        {
            if(strict_lang)
            {
                return !strcmp(e.second.code(lang), code);
            }
            else
            {
                return !strcmp(e.second.code(RUSSIAN), code) ||
                       !strcmp(e.second.code(ENGLISH), code);
            }
        }

        virtual std::string dataName() const { return "code"; }
        virtual std::string dataStr()  const { return code;   }
        virtual ~find_by_code(){}
    };
    template <class finder>static BaseTypeElemHolder initByUserSearch(const finder &uFind)
    {
        typename TypesMap::const_iterator it =
                std::find_if(typesList().begin(), typesList().end(), uFind);
        if(it == typesList().end())
        {
            // no data ...
            throw NoSuchCode("ROMAN",__FILE__,__LINE__, "Unknown " + std::string(ElemT::ElemName) + " " + uFind.dataName() + ":" + uFind.dataStr());
        }
        return BaseTypeElemHolder(& it->second);
    }

    friend std::ostream  &operator << (std::ostream &s, const BaseTypeElemHolder &holder)
    {
        if(holder)
            s << holder->code() << " (" << holder->description() << ")";
        else
            s << "Uninitialized " << ElemT::ElemName;
        return s;
    }

  private:

    std::shared_ptr<ElemT> Elem;
    static void init(TypesMap& VTypes);
    static void addElem(TypesMap& VTypes, const ElemT &elem)
    {
        VTypes.insert(std::make_pair(elem.type(), elem));
    }
};

/**
  * Базовый элемент
*/
#define DECLARE_CODE_SET_ELEM(ENAME) \
class ENAME : public BaseTypeElem<int>\
{\
    typedef BaseTypeElem<int> CodeListData_t;\
public:\
    static const char *ElemName;\
    ENAME(int codeI, const char *lcode, \
                 const char *rcode, \
                 const char *ldesc,\
                 const char *rdesc) throw()\
    :CodeListData_t(codeI, rcode, lcode, rdesc, ldesc)\
    {\
    }\
    virtual ~ENAME(){}


/**
  * Контейнер элементов, созданных с помощью DECLARE_CODE_SET_ELEM
*/
#define DECLARE_CODE_SET(CNAME,ENAME) \
class CNAME : public BaseTypeElemHolder<ENAME>\
{\
public:\
    typedef BaseTypeElemHolder<ENAME> TypeElemHolder;\
    typedef TypeElemHolder::TypesMap CodeSetMap;\
\
    CNAME():TypeElemHolder() {}\
    CNAME(const TypeElemHolder &base) :\
            TypeElemHolder(base)\
    {\
    }\
    CNAME(int t):TypeElemHolder(t) {}\

// need }; at the end of declaration

/**
  * Инициализация данными. Добавить в cpp
*/
#define DESCRIBE_CODE_SET(ENAME) \
template <> void BaseTypeElemHolder<ENAME>::init(TypesMap& VTypes)
//{ initialization here ... }
