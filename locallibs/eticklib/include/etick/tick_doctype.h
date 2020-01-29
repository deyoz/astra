#ifndef TICK_DOCTYPE_H
#define TICK_DOCTYPE_H

#include <serverlib/base_code_set.h>

namespace Ticketing {

class DocTypeElem : public BaseTypeElem<int>
{
public:
    static const char *ElemName;
    typedef BaseTypeElem<int> CodeListData_t;
    DocTypeElem(int codeI, const char *code, const char *desc) throw();
};

/// @class DocType
/// @brief Doc Types.
class DocType : public BaseTypeElemHolder<DocTypeElem>
{
public:
    typedef BaseTypeElemHolder<DocTypeElem> TypeElemHolder;

    ///@enum Type_t
    ///@brief Types.
    enum Type_t
    {
        /// Emd - A
        EmdA = 1,
        EmdS,
        Mco,
        Ticket
    };

    /**
     * @brief Пустой конструктор
     * if(*this) returns false
     */
    DocType();
    /**
     * @brief Конструктор от строки
     * @param typestr
     */
    DocType(const std::string &typestr);
    /**
     * @brief Конструктор от типа
     * @param typ
     */
    DocType(Type_t typ);
    /**
     * @brief Is it Emd?
     * @return true - emd, false - else
    */
    bool isEmd() const;
    /**
     * @brief Is it Eticket?
     * @return true - emd, false - else
    */
    bool isEticket() const;
};

} // namespace Ticketing

#endif // TICK_DOCTYPE_H
