#include "xml_tools.h"
#include "xml_cpp.h"
#include <cstring>

XmlDoc XmlDoc::create()
{
    return impropriate(xmlNewDoc(reinterpret_cast<const xmlChar*>("1.0")));
}

XmlDoc XmlDoc::impropriate(xmlDocPtr doc)
{
    XmlDoc d;
    d.doc = std::shared_ptr<xmlDoc>(doc, xmlFreeDoc);
    return d;
}

XmlDoc xml_parse_memory(const char* q, size_t len)
{
    xmlKeepBlanksDefault(0);
    xmlSubstituteEntitiesDefault(1);
    return XmlDoc::impropriate(xmlParseMemory(q,len));
}

XmlDoc xml_parse_memory(const std::string& q)
{
  return xml_parse_memory(q.data(), q.size());
}

std::string xml_dump(const XmlDoc& d, bool indent)
{
  int size = 0;
  xmlChar* buf = nullptr;
  xmlDocDumpFormatMemoryEnc(d.get(), &buf, &size, "UTF-8", indent ? 1 : 0);
  std::unique_ptr<xmlChar, decltype(xmlFree)> xbuf(buf, xmlFree);
  return buf and size>0 ? std::string(reinterpret_cast<const char*>(buf),size) : std::string();
}

size_t num_nodes(const XmlDoc& d, const char* path)
{
  if(const xmlNodePtr root = xmlDocGetRootElement(d.get()))
      return num_nodes(root, path);
  return 0;
}

xmlNodePtr find_node(const XmlDoc& d, const char* path)
{
  if(const xmlNodePtr root = xmlDocGetRootElement(d.get()))
      return find_node(root, path);
  return 0;
}

XPathResult find_xpath(const XmlDoc& d, const char* path)
{
    if(const xmlNodePtr root = xmlDocGetRootElement(d.get()))
        return find_xpath(root, path);
    return XPathResult();
}

XPathResult find_xpath(const xmlNodePtr n, const char* path)
{
    typedef std::shared_ptr<xmlXPathContext> xmlXPathContext_ptr;

    if(auto xpathCtxt = xmlXPathContext_ptr(xmlXPathNewContext(n->doc), xmlXPathFreeContext))
    {
        xpathCtxt->node = n;
        if(auto x = xmlXPathEvalExpression(reinterpret_cast<const xmlChar*>(path), xpathCtxt.get()))
            return XPathResult::smart_ptr(x,xmlXPathFreeObject);
    }
    return XPathResult();
}

xmlNodePtr find_node(const xmlNodePtr n, const char* path)
{
    if(!n)
        return nullptr;

    const size_t not_a_trivial_char_i = std::strspn(path, "0123456789_abcdefghijklmnopqrstuvwxyz-ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    if(path[not_a_trivial_char_i] == '\0')
    {
        for(auto x = n->children; x; x = x->next)
            if(xmlStrcmp(x->name, reinterpret_cast<const xmlChar*>(path)) == 0)
                return x;
    }
    else
    {
        XPathResult result = find_xpath(n, path);
        if(not result.empty())
            return result->nodesetval->nodeTab[0];
    }
    return nullptr;
}

bool has_prop(const xmlNodePtr node, const char* prop_name, const char* prop_value)
{
  for(xmlAttrPtr a = node ? node->properties : 0; a; a = a->next)
      if(xmlStrcmp(a->name, reinterpret_cast<const xmlChar*>(prop_name))==0)
          return !prop_value or xmlStrcmp(a->children->content, reinterpret_cast<const xmlChar*>(prop_value))==0;
  return false;
}

size_t num_nodes(const xmlNodePtr node, const char* path) 
{
  XPathResult result = find_xpath(node, path);

  if(not result.empty())
    return result->nodesetval->nodeNr;

  return 0;
}
