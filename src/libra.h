#pragma once

#include <serverlib/oci8.h>
#include "jxtlib/JxtInterface.h"


class LibraInterface : public JxtInterface
{
public:
    LibraInterface() : JxtInterface("","libra")
    {
      AddEvent("request", JXT_HANDLER(LibraInterface, RequestHandler));
    }

    void RequestHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

};


namespace LIBRA
{

// может имеет смысл в существующий алгоритм поиска компоновки для назначения на рейс встроить поиск в БД Либры?
// если нашли в Либре, то поиск в наших аналога. Если нашли - назначили все недостоющие свойства, не нашли - пустая компоновка только на основе Либры.
// пусть вручную добавляют свойства. Поиск аналога у нас только по борту или еще по конфирурации и типу ВС?

bool LIBRA_ENABLED();

}
