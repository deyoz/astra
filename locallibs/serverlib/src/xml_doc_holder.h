#ifndef _XML_DOC_HOLDER_H__
#define _XML_DOC_HOLDER_H__

#include <libxml/tree.h>
#include <serverlib/exception.h>

class XmlDocHolder
{
public:
    XmlDocHolder(xmlDocPtr doc)
        : m_doc(doc)
    {
        if (!m_doc)
            throw XmlDocHolder::HolderException("doc == NULL in XmlDocHolder constructor");
        m_root = xmlDocGetRootElement(m_doc);
        if (!m_root) {
            xmlFreeDoc(m_doc);
            throw XmlDocHolder::HolderException("no root node in XmlDocHolder constructor");
        }
    }
    ~XmlDocHolder()
    {
        xmlFreeDoc(m_doc);
    }
    const xmlDocPtr &getDoc() const {   return m_doc;   }
    const xmlNodePtr &getRoot() const {   return m_root;   }
    class HolderException : public comtech::Exception
    {
        public:
        HolderException(const std::string& str) : comtech::Exception(str) {};
        virtual ~HolderException() throw() {};
    };
private:
    xmlDocPtr m_doc;
    xmlNodePtr m_root;
};


#endif // _XML_DOC_HOLDER_H__
