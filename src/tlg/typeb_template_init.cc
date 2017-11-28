/*
*  C++ Implementation: typeb_template_init
*
* Description: ���樠������ 蠡����� type B ᮮ�饭��
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#include "typeb_template_init.h"
#include "ifm_parser.h"

namespace typeb_parser
{
void typeb_template_init()
{
    typeb_template::addTemplate(new IFM_template());
}
} // namespace typeb_parser
