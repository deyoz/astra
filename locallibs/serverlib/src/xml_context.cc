#include <string.h>

#include "xml_context.h"

#include <memory>

using namespace std;

XMLContext *XMLContext::Instance(bool reset)
{
    static std::shared_ptr<XMLContext> instance_(new XMLContext);
    if (reset) {
        instance_.reset();
    }
    return instance_.get();
}


XMLContext::XMLContext()
    : do_not_encode2utf8(0)
{ }

XMLContext *getXmlContext()
{
    return XMLContext::Instance();
}

