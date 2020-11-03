#ifndef _PIECESDATATEMPLATE_H_
#define _PIECESDATATEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{
    class PiecesDataTemplate : public tb_descr_element {
        public:
            virtual const char * name() const { return "Pieces/Weight data element"; }
            virtual const char * lexeme() const { return ".W/"; }
            virtual const char * accessName() const { return "Pieces"; }
            virtual TbElement * parse(const std::string &text) const {return 0;}
    };
} // namespace typeb_parser
#endif /*_PIECESDATATEMPLATE_H_*/
