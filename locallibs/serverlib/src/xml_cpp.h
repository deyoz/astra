#ifndef _XML_CPP_H__
#define _XML_CPP_H__

#include <string>
#include <memory>
#include <libxml/tree.h>
#include <libxml/xpath.h>

class XmlDoc
{
    std::shared_ptr<xmlDoc> doc;
  public:
    xmlDocPtr operator->() const {  return doc.operator->();  }
    xmlDocPtr get() const {  return doc.get();  }
    explicit operator bool() const {  return doc.get();  }
    static XmlDoc create();
    static XmlDoc impropriate(xmlDocPtr); // takes ownership
};

XmlDoc xml_parse_memory(const std::string& q);
XmlDoc xml_parse_memory(const char* q, size_t len);

std::string xml_dump(const XmlDoc& , bool indent = false);

xmlNodePtr find_node(const XmlDoc&, const char* path);
xmlNodePtr find_node(const xmlNodePtr n, const char* path);

inline xmlNodePtr find_node(const XmlDoc& doc, const std::string& path)
{
    return find_node(doc, path.c_str());
}

inline xmlNodePtr find_node(const xmlNodePtr n, const std::string& path)
{
    return find_node(n, path.c_str());
}

size_t num_nodes(const XmlDoc&, const char* path);
size_t num_nodes(const xmlNodePtr n, const char* path);

bool has_prop(const xmlNodePtr node, const char* prop_name, const char* prop_value);

//-----------------------------------------------------------------------

class XPathResult
{
  public:
    typedef std::shared_ptr<xmlXPathObject> smart_ptr;

    XPathResult(): ptr_() { }
    XPathResult(const smart_ptr& xmlXPathObjectPtr): ptr_(xmlXPathObjectPtr) { }

    smart_ptr operator->() const { return ptr_; }
    bool empty() const { return !ptr_ or xmlXPathNodeSetIsEmpty(ptr_->nodesetval); }
    size_t size() const { return this->empty() ? 0 : ptr_->nodesetval->nodeNr; }
    const xmlNodePtr* begin() const { return this->empty() ? nullptr : ptr_->nodesetval->nodeTab; }
    const xmlNodePtr* end() const { return this->empty() ? nullptr : ptr_->nodesetval->nodeTab+ptr_->nodesetval->nodeNr; }
  private:
    smart_ptr ptr_;
};

XPathResult find_xpath(const XmlDoc& d, const char* path);
XPathResult find_xpath(const xmlNodePtr n, const char* path);

template <typename X, class F> F for_each_node_if_xpath(X root, const char* xpath, F f)
{
    for(auto ptr : find_xpath(root,xpath))
        f(ptr);
    return f;
}

//-----------------------------------------------------------------------

template <class F> F for_each_node_if_name_and_prop(xmlNodePtr root, const char* node_name,
                                                    const char* prop_name, const char* prop_value, F f)
{
  for(xmlNodePtr node = root ? root->children : NULL, next_node = NULL; node; node = next_node)
  {
      next_node = xmlNextElementSibling(node);
      if(!node_name or xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>(node_name))==0)
          if(!prop_name or has_prop(node, prop_name, prop_value))
              f(node);
  }
  return f;
}

template <class F> F for_each_node_if_name(xmlNodePtr root, const char* node_name, const F& f)
{
  return for_each_node_if_name_and_prop(root, node_name, 0, 0, f);
}

template <class F> F for_each_node(xmlNodePtr root, const F& f)
{
  return for_each_node_if_name(root, 0, f);
}

template<typename FN>
xmlNodePtr findChildNodeIf(xmlNodePtr root, const char * node_name, FN fn)
{
    for(xmlNodePtr node = root ? root->children : nullptr, next_node = nullptr; node; node = next_node)
    {
        next_node = xmlNextElementSibling(node);
        if(!node_name or xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>(node_name))==0)
        {
            const bool match = fn(node);
            if(match)
                return node;
        }
    }
    return nullptr;
}

#endif // _XML_CPP_H__
