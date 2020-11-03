//
// C++ Interface: CategoryElement
//
// Description: Category NOSHO, GOSHO....
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2009
//
//
#ifndef _CATEGORYELEMENT_H_
#define _CATEGORYELEMENT_H_
#include <string>
#include "typeb_message.h"
#include "tb_elements.h"

namespace typeb_parser
{
    class CategoryElement : public TbElement
    {
        std::string CatName;
        unsigned Num;
        char SubClsCode;

        CategoryElement(const std::string &CName,
                        unsigned n, char SclsCode)
            :CatName(CName), Num(n), SubClsCode(SclsCode)
        {
        }
        public:
            static CategoryElement * parse (const std::string &text);
            const std::string &catName() const { return  CatName; }
            unsigned num() const { return Num; }
            char subClsCode() const { return SubClsCode; }
    };



    std::ostream & operator << (std::ostream& os, const CategoryElement &nums);
}
#endif /*_CATEGORYELEMENT_H_*/
