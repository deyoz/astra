#include <string.h>
#include <libxml/xmlerror.h>
#include <libxml/tree.h>
#include "xmllibcpp.h"
#include "dates.h"

namespace libxmlNoEncCheck
{

static xmlAttrPtr
xmlGetPropNodeInternal(xmlNodePtr node, const xmlChar *name,
		       const xmlChar *nsName, int useDTD)
{
    xmlAttrPtr prop;

    if ((node == NULL) || (node->type != XML_ELEMENT_NODE) || (name == NULL))
	return(NULL);

    if (node->properties != NULL) {
	prop = node->properties;
	if (nsName == NULL) {
	    /*
	    * We want the attr to be in no namespace.
	    */
	    do {
		if ((prop->ns == NULL) && xmlStrEqual(prop->name, name)) {
		    return(prop);
		}
		prop = prop->next;
	    } while (prop != NULL);
	} else {
	    /*
	    * We want the attr to be in the specified namespace.
	    */
	    do {
		if ((prop->ns != NULL) && xmlStrEqual(prop->name, name) &&
		    ((prop->ns->href == nsName) ||
		     xmlStrEqual(prop->ns->href, nsName)))
		{
		    return(prop);
		}
		prop = prop->next;
	    } while (prop != NULL);
	}
    }

#ifdef LIBXML_TREE_ENABLED
    if (! useDTD)
	return(NULL);
    /*
     * Check if there is a default/fixed attribute declaration in
     * the internal or external subset.
     */
    if ((node->doc != NULL) && (node->doc->intSubset != NULL)) {
	xmlDocPtr doc = node->doc;
	xmlAttributePtr attrDecl = NULL;
	const xmlChar *elemQName = NULL;
    xmlChar *tmpstr = NULL;

	/*
	* We need the QName of the element for the DTD-lookup.
	*/
	if ((node->ns != NULL) && (node->ns->prefix != NULL)) {
	    tmpstr = xmlStrdup(node->ns->prefix);
	    tmpstr = xmlStrcat(tmpstr, reinterpret_cast<const xmlChar*>(":"));
	    tmpstr = xmlStrcat(tmpstr, node->name);
	    if (tmpstr == NULL)
		return(NULL);
	    elemQName = tmpstr;
	} else
	    elemQName = node->name;
	if (nsName == NULL) {
	    /*
	    * The common and nice case: Attr in no namespace.
	    */
	    attrDecl = xmlGetDtdQAttrDesc(doc->intSubset,
		elemQName, name, NULL);
	    if ((attrDecl == NULL) && (doc->extSubset != NULL)) {
		attrDecl = xmlGetDtdQAttrDesc(doc->extSubset,
		    elemQName, name, NULL);
	    }
	} else {
	    xmlNsPtr *nsList, *cur;

	    /*
	    * The ugly case: Search using the prefixes of in-scope
	    * ns-decls corresponding to @nsName.
	    */
	    nsList = xmlGetNsList(node->doc, node);
	    if (nsList == NULL) {
		if (tmpstr != NULL)
		    xmlFree(tmpstr);
		return(NULL);
	    }
	    cur = nsList;
	    while (*cur != NULL) {
		if (xmlStrEqual((*cur)->href, nsName)) {
		    attrDecl = xmlGetDtdQAttrDesc(doc->intSubset, elemQName,
			name, (*cur)->prefix);
		    if (attrDecl)
			break;
		    if (doc->extSubset != NULL) {
			attrDecl = xmlGetDtdQAttrDesc(doc->extSubset, elemQName,
			    name, (*cur)->prefix);
			if (attrDecl)
			    break;
		    }
		}
		cur++;
	    }
	    xmlFree(nsList);
	}
	if (tmpstr != NULL)
	    xmlFree(tmpstr);
	/*
	* Only default/fixed attrs are relevant.
	*/
	if ((attrDecl != NULL) && (attrDecl->defaultValue != NULL))
	    return((xmlAttrPtr) attrDecl);
    }
#endif /* LIBXML_TREE_ENABLED */
    return(NULL);
}

static xmlAttrPtr xmlNewPropInternal(xmlNodePtr node, xmlNsPtr ns,
                                     const xmlChar * name, const xmlChar * value,
                                     int eatname)
{
  xmlAttrPtr cur;
  xmlDocPtr doc = NULL;

  if ((node != NULL) && (node->type != XML_ELEMENT_NODE)) {
      if ((eatname == 1) &&
    ((node->doc == NULL) ||
     (!(xmlDictOwns(node->doc->dict, name)))))
          xmlFree((xmlChar *) name);
      return (NULL);
  }

  /*
   * Allocate a new property and fill the fields.
   */
  cur = (xmlAttrPtr)xmlMalloc(sizeof(xmlAttr));
  if (cur == NULL)
  {
    if ((eatname == 1) &&
        ((node == NULL) || (node->doc == NULL) ||
         (!(xmlDictOwns(node->doc->dict, name)))))
    {
      xmlFree((xmlChar *) name);
    }
    //__xmlSimpleError(XML_FROM_TREE, XML_ERR_NO_MEMORY, NULL, NULL, "building attribute");

    return (NULL);
  }
  memset(cur, 0, sizeof(xmlAttr));
  cur->type = XML_ATTRIBUTE_NODE;

  cur->parent = node;
  if (node != NULL) {
      doc = node->doc;
      cur->doc = doc;
  }
  cur->ns = ns;

  if (eatname == 0) {
      if ((doc != NULL) && (doc->dict != NULL))
          cur->name = (xmlChar *) xmlDictLookup(doc->dict, name, -1);
      else
          cur->name = xmlStrdup(name);
  } else
      cur->name = name;

  if (value != NULL)
  {
    xmlNodePtr tmp;

    cur->children = xmlNewDocText(doc, value);
    cur->last = NULL;
    tmp = cur->children;
    while (tmp != NULL) {
        tmp->parent = (xmlNodePtr) cur;
        if (tmp->next == NULL)
            cur->last = tmp;
        tmp = tmp->next;
    }
  }

  /*
   * Add it at the end to preserve parsing order ...
   */
  if (node != NULL) {
      if (node->properties == NULL) {
          node->properties = cur;
      } else {
          xmlAttrPtr prev = node->properties;

          while (prev->next != NULL)
              prev = prev->next;
          prev->next = cur;
          cur->prev = prev;
      }
  }

  if ((value != NULL) && (node != NULL) &&
      (xmlIsID(node->doc, node, cur) == 1))
      xmlAddID(NULL, node->doc, value, cur);

  /*
  if ((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
    xmlRegisterNodeDefaultValue((xmlNodePtr) cur);
  */
  return (cur);
}



xmlAttrPtr xmlSetNsProp(xmlNodePtr node, xmlNsPtr ns, const xmlChar *name, const xmlChar *value)
{
  if (ns && (ns->href == NULL))
    return(NULL);
  auto prop = xmlGetPropNodeInternal(node, name, (ns != NULL) ? ns->href : NULL, 0);
  if (prop != NULL)
  {
    /*
    * Modify the attribute's value.
    */
    if (prop->atype == XML_ATTRIBUTE_ID)
    {
      xmlRemoveID(node->doc, prop);
      prop->atype = XML_ATTRIBUTE_ID;
    }
    if (prop->children != NULL)
      xmlFreeNodeList(prop->children);
    prop->children = NULL;
    prop->last = NULL;
    prop->ns = ns;
    if (value != NULL)
    {
      xmlNodePtr tmp;

      prop->children = xmlNewDocText(node->doc, value);
      prop->last = NULL;
      tmp = prop->children;
      while (tmp != NULL)
      {
        tmp->parent = (xmlNodePtr) prop;
        if (tmp->next == NULL)
          prop->last = tmp;
        tmp = tmp->next;
      }
    }
    if (prop->atype == XML_ATTRIBUTE_ID)
      xmlAddID(NULL, node->doc, value, prop);
    return(prop);
  }
  /*
  * No equal attr found; create a new one.
  */
  return(xmlNewPropInternal(node, ns, name, value, 0));
}

} // namespace libxmlNoEncCheck

xmlAttrPtr xmlSetPropNoEncCheck(xmlNodePtr node, const char *name, const char *value)
{
  int len;
  const xmlChar *nqname;

  if ((node == NULL) || (name == NULL) || (node->type != XML_ELEMENT_NODE))
    return(NULL);

  /*
   * handle QNames
   */
  nqname = xmlSplitQName3(reinterpret_cast<const xmlChar*>(name), &len);
  if (nqname != NULL)
  {
    xmlNsPtr ns;
    xmlChar *prefix = xmlStrndup(reinterpret_cast<const xmlChar*>(name), len);
    ns = xmlSearchNs(node->doc, node, prefix);
    if (prefix != NULL)
      xmlFree(prefix);
    if (ns != NULL)
      return(libxmlNoEncCheck::xmlSetNsProp(node, ns, nqname, reinterpret_cast<const xmlChar*>(value)));
  }
  return(libxmlNoEncCheck::xmlSetNsProp(node, NULL, reinterpret_cast<const xmlChar*>(name), reinterpret_cast<const xmlChar*>(value)));
}

xmlNodePtr xmlNewTextChild_ddmmyyyy(xmlNodePtr node, xmlNsPtr ns, const char *name, const boost::gregorian::date& d)
{
    return xmlNewTextChild(node, ns, name, Dates::ddmmyyyy(d,true));
}

xmlNodePtr xmlNewTextChild_hh24mi_ddmmyyyy(xmlNodePtr node, xmlNsPtr ns, const char *name, const boost::posix_time::ptime& t)
{
    return xmlNewTextChild(node, ns, name, Dates::hh24mi_ddmmyyyy(t));
}

xmlAttrPtr xmlSetProp_ddmmyyyy(xmlNodePtr node, const char *name, const boost::gregorian::date& d)
{
    return xmlSetProp(node, name, Dates::ddmmyyyy(d,true));
}

xmlAttrPtr xmlSetProp_hh24mi_ddmmyyyy(xmlNodePtr node, const char *name, const boost::posix_time::ptime& t)
{
    return xmlSetProp(node, name, Dates::hh24mi_ddmmyyyy(t));
}
