#ifndef __XML_CONTEXT_H__
#define __XML_CONTEXT_H__

#include <libxml/tree.h>

#ifdef __cplusplus
#include <string>
#include <list>

class XMLContext
{
public:
    static XMLContext *Instance(bool reset = false);

    int donotencode2UTF8() { return do_not_encode2utf8; }
    void set_donotencode2UTF8(int enc) { do_not_encode2utf8=enc; }

private:
    XMLContext();
    int do_not_encode2utf8;
};

XMLContext *getXmlContext();

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __JXT_XML_CONT_H__ */
