#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <libxml/tree.h>
#include <string.h>
#include <dirent.h>
#include <serverlib/helpcpp.h>
#include <serverlib/str_utils.h>
#include <serverlib/perfom.h>

#include "jxtlib.h"
#include "xml_cpp.h"
#define NICKNAME "ILYA"
#include <serverlib/slogger.h>
#include "jxt_xml_cont.h"
#include "jxt_cont.h"
#include "jxt_handle.h"
#include "xml_tools.h"
#include "xml_stuff.h"
#include "jxt_tools.h"
#include "jxt_sys_reqs.h"
#include "jxt_stuff.h"
#include "xmllibcpp.h"
#include "jxtlib_db_callbacks.h"

using namespace std;
using namespace jxtlib;
using namespace JxtContext;

xmlDocPtr newDoc()
{
  xmlDocPtr resDoc=xmlNewDoc(reinterpret_cast<const xmlChar*>("1.0"));
  xmlNodePtr resNode;

  if(resDoc)
  {
    xmlFree(const_cast<xmlChar *>(resDoc->encoding));
    resDoc->encoding=xmlStrdup("UTF-8");
    resDoc->children=xmlNewDocNode(resDoc,NULL,reinterpret_cast<const xmlChar*>("term"),NULL);
    resNode=xmlNewChild(resDoc->children,NULL,"answer",NULL);
  }
  if(resDoc && !resNode)
  {
    ProgError(STDLOG,"xmlNewChild failed!");
    xmlFreeDoc(resDoc);
    return NULL;
  }
  return resDoc;
}

xmlNodePtr setElemProp(xmlNodePtr resNode, const char *tag, const char *prop,
                       const char *value)
{
  xmlNodePtr propNode=getNode(resNode,"properties");

  // Введем усовершенствование - посмотрим, не указан ли в параметре tag
  // список параметров (через запятую). Если указан список, зададим свойства
  // через имя свойства, если нет - через idref.

  const char *comma_ptr=strchr(tag,',');
  if(comma_ptr) // указано свойство для списка элементов
  {
    xmlSetProp(xmlNewTextChild(propNode,NULL,prop,value),"name",tag);
  }
  else // указано свойство для одного элемента
  {
    xmlNodePtr idrefNode=findNodeByProp(propNode,"name",tag);
    if(!idrefNode || xmlStrcmp(idrefNode->name,"idref")!=0)
    {
      idrefNode=newChild(propNode,"idref");
      xmlSetProp(idrefNode,"name",tag);
    }
    xmlSetProp(idrefNode,prop,value);
    return idrefNode;
  }
  return NULL;
}

xmlNodePtr insertEmptyRow(xmlNodePtr tabNode, int col_count, int row_index)
{
  if(!tabNode)
    return NULL;
  xmlNodePtr rowNode=newChild(tabNode,"row");
  xmlSetProp(rowNode,"index",row_index);
  for(int i=0;i<col_count;i++)
    xmlSetProp(newChild(rowNode,"col"),"index",i);
  return rowNode;
}

/* Основная функция, добавляющая к документу все необходимые тэги для */
/* отрисовки интерфейса. */
extern "C" xmlNodePtr iface_C(xmlNodePtr resNode, const char *iface_id)
{
  if(!iface_id)
  {
    ProgError(STDLOG,"iface(): iface_id is NULL");
    throw jxtlib_exception(STDLOG,"Ошибка программы!");
  }
  return iface(resNode,iface_id,
               getJxtContHandler()->currContext()->getHandle(),false);
}

xmlNodePtr iface_C_nocheck(xmlNodePtr resNode, const char *iface_id)
{
  if(!iface_id)
  {
    ProgError(STDLOG,"iface(): iface_id is NULL");
    throw jxtlib_exception(STDLOG,"Ошибка программы!");
  }
  return iface(resNode,iface_id,
               getJxtContHandler()->currContext()->getHandle(),true);
}


/* Основная функция, добавляющая к документу все необходимые тэги для */
/* отрисовки интерфейса. */
xmlNodePtr iface(xmlNodePtr resNode, const std::string &iface_id)
{
  return iface(resNode,iface_id,
               getJxtContHandler()->currContext()->getHandle());
}

string getCachedIfaceWoIparts(const std::string &id, long answer_ver)
{
  return JxtlibDbCallbacks::instance()->getCachedIfaceWoIparts(id, answer_ver);
}

void setCachedIfaceWoIparts(xmlNodePtr resNode, const std::string &id,
                            long ver)
{
  xmlDocPtr ifaceDoc=resNode->doc;
  char *str=0;
  int str_len;
  xmlDocDumpMemory(ifaceDoc,(xmlChar **)&str,&str_len);
  std::string iface_str = str;
  xmlFree(str);

  JxtlibDbCallbacks::instance()->setCachedIfaceWoIparts(iface_str, id, ver);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

using namespace jxtlib;
void parseProps(xmlNodePtr propNode, map<string,map<string,string> > &props);

class ParsedIface
{
  private:
    std::string iface_id;
    std::string lang;
    XmlDoc ifaceDoc;
    long iface_version;
    long counted_version;
    xmlNodePtr iMainNode;

    std::vector<IfaceLinks> ilinks;
    typedef std::map<std::string,std::map<std::string,std::string> > PropsMap;
    PropsMap props;

    struct TypeDesc // Описание типа
    {
      XmlDoc typeDoc; // Адрес документа с описанием типа
      std::vector<std::string> ids; // IDы элементов типа
    };

    typedef std::map<std::string, TypeDesc *> TElemsMap;
    TElemsMap telems;

    void getIparts(xmlNodePtr windowNode);
    void getTypes();
    void getPparts(xmlNodePtr propNode);
    void mergeIpart(xmlNodePtr ifNode, XmlDoc ipartDoc);
  public:
    ParsedIface(const std::string &iface_id_, const std::string &lang_) : 
      iface_id(iface_id_), lang(lang_)
    {
      iface_version=0;
      counted_version=0;
      iMainNode=0;
    }
    ~ParsedIface();
    void process();
    std::string getDescr();
    const std::vector<IfaceLinks> &getILinks()
    {
      return ilinks;
    }
};

ParsedIface::~ParsedIface()
{
  // Бежим по заполненному списку типов и освобождаем объекты
  for(TElemsMap::iterator i=telems.begin();i!=telems.end();++i)
    delete i->second;
}

XmlDoc IfaceLinks_getDoc(const std::string &id, const std::string &type, 
                         long &ver)
{
  if(type=="data")
  {
    ProgTrace(TRACE1,"IfaceLinks::getDoc called for data id='%s'",id.c_str());
    throw jxtlib_exception(STDLOG,"Wrong call of IfaceLinks::getDoc()");
  }
  if(ver>0)
  {
    auto descr = GetXmlData(type.c_str(),id.c_str(),ver);
    getXmlCtxt()->set_donotencode2UTF8(1);
    return xml_parse_memory(descr);
  }
    if(ver==0)
    {
      ProgTrace(TRACE1,"IfaceLinks::getDoc() called, but ver is 0");
      throw jxtlib_exception(STDLOG,"Wrong call of IfaceLinks::getDoc()");
  }
  return {};
}

int iLinksCmp(const IfaceLinks &i1, const IfaceLinks &i2)
{
  if(i1.type=="ppart" && i2.type!="ppart")
    return -1;
  return 0;
}

void ParsedIface::process()
{
  ProgTrace(TRACE1,"ParsedIface::process: (%s,'%s')",iface_id.c_str(),lang.c_str());

  // Получим версию описания интерфейса из таблицы XML_STUFF
  // воспользуемся "внутренней" функцией, чтобы получить точную версию
  iface_version=getXmlDataVer_inner("interface",iface_id.c_str(),false);
  if (iface_version < 0) {
    ProgTrace(TRACE1, "getXmlDataVer failed: iface='%s'", iface_id.c_str());
    throw jxtlib_exception(STDLOG, "getXmlDataVer failed");
  }

  // Описание интерфейса из XML_STUFF
  string iface_descr(GetXmlData("interface",iface_id.c_str(),iface_version));
  // Версия интерфейса у нас уже и так в UTF-8, запретим перекодировку
  getXmlCtxt()->set_donotencode2UTF8(1);
  ifaceDoc = xml_parse_memory(iface_descr);
  if(!ifaceDoc)
    throw jxtlib_exception(STDLOG,"Unable to parse interface description");
  xmlNodePtr iMainNode=findNodeR(ifaceDoc->children,"interface");
  if(!iMainNode)
    throw jxtlib_exception(STDLOG,"Unable to find <interface> in "
                                  "interface description");

  IfaceLinks link(iface_id+"_en","ppart","en");
  if(link.ver != long(-1))
    ilinks.push_back(link);

  // Алгоритм:
  // 1. Найти все ipart и вставить в документ. Свойства интерфейса сильнее свойств ipart
  // 2. Найти все типы и вставить в документ. Свойства интерфейса сильнее свойств ipart
  // 3. Вставляя типы, сохранить все пары elementID-type, чтобы потом правильно применять ppart
  // 4. Взять ppart для всех ipart, type, самого интерфейса. Применить их. Свойства из ppart сильнее свойств интерфейса.
  // 5. Достать из свойств все ссылки на data. Это будет основа наших iface_links.
  // 6. Собрать все свойства вместе
  // 7. Достать заданные в спецсекции интерфейса iface_links и добавить их к уже полученным.
  // 8. Отсортируем ilinks - pparts должны идти впереди

  // Шаг 0.
  // Разберем свойства интерфейса
  xmlNodePtr iPropNode=findNode(iMainNode,"properties");
  if(!iPropNode)
    throw jxtlib_exception(STDLOG,"Unable to locate <properties> in "
                                  "interface description");
  parseProps(iPropNode,props);
  // Удалим тэги свойств интерфейса
  xmlUnlinkNode(iPropNode);
  xmlFreeNode(iPropNode);

  // Шаг 1.
  // Ищем все ipart и вставляем их в документ
  getIparts(findNode(iMainNode,"window"));

  // Шаги 2 и 3.
  // Ищем типы, заполняем iface_links и telems, объединяем свойства
  getTypes();

  // Шаг 4.
  // Разбираемся с ppart
  getPparts(findNode(iMainNode=findNodeR(ifaceDoc->children,"interface"),
                     "properties"));

  // Шаги 5 и 6.
  // Бежим по свойствам, записываем их в интерфейс и заодно дополняем links
  // ссылками на data
  xmlNodePtr propNode=newChild(iMainNode,"properties");
  for(auto& i : props)
  {
      xmlNodePtr idrefNode=newChild(propNode,"idref");
      for(auto& j : i.second)
      {
          xmlSetProp(idrefNode, j.first, j.second);
          if(j.first == "data")
              ilinks.emplace_back(j.second, "data", "");
      }
  }

  // Шаг 7.
  // Найдем в описании интерфейса спецсекцию ilinks и прочтем данные оттуда
  xmlNodePtr linksNode=findNode(iMainNode,"ilinks");
  if(linksNode)
    linksNode=linksNode->children;
  while(linksNode)
  {
    ilinks.emplace_back(getStrPropFromXml(linksNode,"name"), (char *)linksNode->name, getStrPropFromXml(linksNode,"lang"));
    linksNode=linksNode->next;
  }

  // Шаг 8
  std::sort(ilinks.begin(),ilinks.end(),iLinksCmp);
}

void ParsedIface::mergeIpart(xmlNodePtr ifNode, XmlDoc ipDoc)
{
  xmlNodePtr ipartMainNode=findNodeR(ipDoc->children,"ipart");
  if(!ipartMainNode)
    throw jxtlib_exception(STDLOG,"Invalid ipart description");
  xmlNodePtr winNode=findNode(ipartMainNode,"window");
  if(!winNode)
    throw jxtlib_exception(STDLOG,"Invalid ipart description");

  xmlNodeSetName(ifNode,"group"); // переименуем тэг в описании интерфейса
  // Скопируем все части нашего ipart в интерфейс
  xmlNodePtr n=winNode->children;
  while(n)
  {
    xmlNodePtr copyNode=xmlCopyNode(n,1);
    xmlAddChild(ifNode,copyNode);
    n=n->next;
  }

  // Добавим к коллекции свойств props свойства ipart
  xmlNodePtr propNode=findNode(ipartMainNode,"properties");
  if(!propNode)
    throw jxtlib_exception(STDLOG,"Unable to locate <properties> in ipart data");
  parseProps(propNode,props);

  // разберемся со static
  xmlNodePtr ipartStatic=findNode(iMainNode,"static");
  xmlNodePtr ifaceStatic=NULL;
  if(ipartStatic)
    ipartStatic=ipartStatic->children;
  while(ipartStatic)
  {
    xmlNodePtr tmpNode=xmlCopyNode(ipartStatic,1);
    if(!ifaceStatic)
      ifaceStatic=getNode(iMainNode,"static");
    xmlAddChild(ifaceStatic,tmpNode);
    ipartStatic=ipartStatic->next;
  }
}

void ParsedIface::getIparts(xmlNodePtr pNode)
{
  if(!pNode || !pNode->children)
    return;
  xmlNodePtr node=pNode->children;
  while(node)
  {
    if(!isempty(node))
    {
      getIparts(node);
      node=node->next;
      continue;
    }
    if(xmlStrcmp(node->name,"ipart")==0)
    {
      string ipName=getStrPropFromXml(node,"id");
      ProgTrace(TRACE1,"ipart found: '%s'",ipName.c_str());
      ilinks.push_back(IfaceLinks(ipName+"_en","ppart","en"));
      IfaceLinks il(ipName,"ipart","");
      ilinks.push_back(il);
      mergeIpart(node, IfaceLinks_getDoc(il.id, il.type, il.ver));
      continue;
    }
    node=node->next;
  }
}

void mergeTypeWindow(xmlNodePtr ifaceNode, const std::string &elem_id,
                     xmlNodePtr typeMenuNode)
{
  if(!ifaceNode || !ifaceNode->children || !typeMenuNode)
    return;
  xmlNodePtr node=ifaceNode->children;
  while(node)
  {
    if(!isempty(node))
    {
      mergeTypeWindow(node,elem_id,typeMenuNode);
      node=node->next;
      continue;
    }
    if(getStrPropFromXml(node,"id")==elem_id)
    {
      xmlNodePtr aaaNode=typeMenuNode->children;
      while(aaaNode)
      {
        // добавляем все menuitem в интерфейс
        xmlNodePtr tmpNode=xmlCopyNode(aaaNode,1);
        xmlAddChild(node,tmpNode);
        aaaNode=aaaNode->next;
      }
    }
    node=node->next;
  }
}

void parseTypePropsIntoInterface(const std::string &elem_id, xmlNodePtr propNode,
                                 std::map<std::string,std::map<std::string,std::string> > &props)
{
  typedef std::map<std::string,std::map<std::string,std::string> >::iterator propsIt;
  xmlNodePtr node=propNode->children;

  // Удалим из свойств элемента elem_id свойство type
  propsIt pos=props.find(elem_id);
  if(pos!=props.end())
  {
    map<string,string>::iterator ipos=pos->second.find("type");
    if(ipos!=pos->second.end())
      pos->second.erase(ipos);
  }

  // Достанем свойства из xml-описания type
  while(node)
  {
    if(node->type==XML_COMMENT_NODE)
    {
      node=node->next;
      continue;
    }
    string name=getStrPropFromXml(node,"name");
    if(xmlStrcmp(node->name,"idref")==0) // обычные свойства
    {
      propsIt pos=props.find(name);
      if(pos==props.end()) // таких еще нет
      {
        map<string,string> iprops;
        xmlAttrPtr pr=node->properties;
        while(pr)
        {
          iprops[reinterpret_cast<const char*>(pr->name)]=(pr->children->content?reinterpret_cast<const char*>(pr->children->content):"");
          pr=pr->next;
        }
        props[name]=iprops;
      }
      else
      {
        xmlAttrPtr pr=node->properties;
        while(pr)
        {
          map<string,string>::iterator ipos=pos->second.find(reinterpret_cast<const char*>(pr->name));
          // Сохраняем свойство, только если оно еще не задано
          if(ipos==pos->second.end()) // такого еще нет
          {
            (pos->second)[reinterpret_cast<const char*>(pr->name)]=(pr->children->content?reinterpret_cast<const char*>(pr->children->content):"");
          }
          pr=pr->next;
        }
      }
    }
    else // "мульти-свойства", возможно, без атрибута name - тогда берем elem_id
    {
      vector<string> names;
      if(!name.empty()) // атрибут name задан
        StrUtils::ParseStringToVecStr(names,name,',');
      else
        names.push_back(elem_id); // name не задан - подставляем elem_id
      string value=getStrFromXml(node);
      for(vector<string>::iterator it=names.begin();it!=names.end();++it)
      {
        propsIt pos=props.find(*it);
        if(pos==props.end())
        {
          map<string,string> iprops;
          iprops["name"]=(*it);
          iprops[reinterpret_cast<const char*>(node->name)]=value;
          props[*it]=iprops;
        }
        else
        {
          map<string,string>::iterator ipos=pos->second.find(reinterpret_cast<const char*>(node->name));
          // Сохраняем свойство, только если оно еще не задано
          if(ipos==pos->second.end()) // такого еще нет
          {
              (pos->second)[reinterpret_cast<const char*>(node->name)]=value;
          }
        }
      }
    }
    node=node->next;
  }
}

void ParsedIface::getTypes()
{
  // Пробежимся по свойствам, найдем все используемые типы и запомним ID
  // элементов, где эти типы используются
  for(PropsMap::iterator i=props.begin();i!=props.end();++i)
  {
    for(map<string,string>::iterator j=i->second.begin();j!=i->second.end();++j)
    {
      if(j->first=="type")
      {
        string type=j->second;

        if(type=="string" || type=="alpha" || type=="number" ||
           type=="integer"|| type=="date" || type=="boolean" ||
           type=="weekdays" || type=="double")
          continue;
        ProgTrace(TRACE1,"type='%s'",type.c_str());
        ilinks.push_back(IfaceLinks(type+"_en","ppart","en"));
        ilinks.push_back(IfaceLinks(type,"type",""));
        TElemsMap::iterator pos=telems.find(type);
        if(pos==telems.end()) // no such type
        {
          TypeDesc *td=new TypeDesc();
          td->typeDoc = IfaceLinks_getDoc(ilinks.rbegin()->id, ilinks.rbegin()->type, 
                                          ilinks.rbegin()->ver); // Сохраним адрес документа
          if(!td->typeDoc)
          {
            ProgError(STDLOG,"td.typeDoc is NULL!");
            throw jxtlib_exception(STDLOG,"Invalid type description "+type);
          }
          td->ids.push_back(i->first);
          telems[type]=td;
        }
        else
        {
          if(find(pos->second->ids.begin(),pos->second->ids.end(),i->first)==pos->second->ids.end())
            pos->second->ids.push_back(i->first); // добавим ID элемента в список
        }
      }
    }
  }

  // Теперь бежим по заполненному списку типов и преобразуем интерфейс
  for(TElemsMap::iterator i=telems.begin();i!=telems.end();++i)
  {
    // Перебираем все элементы данного типа
    for(vector<string>::iterator j=i->second->ids.begin();j!=i->second->ids.end();++j)
    {
      xmlNodePtr typeMenuNode=findNodeR(i->second->typeDoc->children,"menu");
      if(typeMenuNode && !isempty(typeMenuNode))
      {
        mergeTypeWindow(findNode(iMainNode=findNodeR(ifaceDoc->children,"interface"),"window"),*j,typeMenuNode);
      }
      else
        ProgTrace(TRACE1,"i->first='%s', <%s>",i->first.c_str(),
                  i->second->typeDoc->children->name);

      // Добавим к коллекции свойств props свойства из type
      xmlNodePtr typePropNode=findNodeR(i->second->typeDoc->children,"properties");
      if(!typePropNode)
        throw jxtlib_exception(STDLOG,"Invalid type description - "
                                      "unable to locate <properties>");
      parseTypePropsIntoInterface(*j,typePropNode,props);
    }
  }
}

void applyPpartToInterface(xmlNodePtr propNode,
                           std::map<std::string,std::map<std::string,std::string> > &props,
                           const std::string &elem_id="")
{
  typedef std::map<std::string,std::map<std::string,std::string> >::iterator propsIt;

  xmlNodePtr node=propNode->children;

  // Достанем свойства из xml-описания ppart
  while(node)
  {
    if(node->type==XML_COMMENT_NODE)
    {
      node=node->next;
      continue;
    }
    string name=getStrPropFromXml(node,"name");
    if(xmlStrcmp(node->name,"idref")==0) // обычные свойства
    {
      propsIt pos=props.find(name);
      if(pos==props.end()) // таких еще нет
      {
        // Если свойств для элемента еще нет - незачем с ним возиться,
        // элемент уже не появится
        node=node->next;
        continue;
      }
      else
      {
        xmlAttrPtr pr=node->properties;
        while(pr)
        {
          // Сохраняем свойство, даже если оно уже задано
          (pos->second)[reinterpret_cast<const char*>(pr->name)]= (pr->children->content?reinterpret_cast<const char*>(pr->children->content):"");
          pr=pr->next;
        }
      }
    }
    else // "мульти-свойства", возможно, без атрибута name - тогда берем elem_id
    {
      vector<string> names;
      if(!name.empty()) // атрибут name задан
        StrUtils::ParseStringToVecStr(names,name,',');
      else
      {
        if(!elem_id.empty())
          names.push_back(elem_id); // name не задан - подставляем elem_id
      }
      string value=getStrFromXml(node);
      for(vector<string>::iterator it=names.begin();it!=names.end();++it)
      {
        propsIt pos=props.find(*it);
        if(pos==props.end())
        {
          // Элемент не найден - пропускаем, он уже не появится
          continue;
        }
        else
        {
          // Сохраняем свойство, даже если оно уже задано
          (pos->second)[reinterpret_cast<const char*>(node->name)]=value;
        }
      }
    }
    node=node->next;
  }
}

void ParsedIface::getPparts(xmlNodePtr propNode)
{
  // перебираем все полученные к этому моменту линки
  // ppart'ы надо применять в следующем порядке: типы, ипарты, интерфейс
  // перебираем линки с конца, тогда ппарты будут как раз в нужном порядке
  for(vector<IfaceLinks>::reverse_iterator i=ilinks.rbegin();i!=ilinks.rend();++i)
  {
    if(i->type=="ppart" && this->lang==i->lang) // Надо применять
    {
      xmlNodePtr ppartNode=findNodeR(IfaceLinks_getDoc(i->id, i->type, i->ver)->children,"ppart");
      if(!ppartNode)
        throw jxtlib_exception(STDLOG,"Invalid ppart description - no <ppart> node");

      xmlNodePtr propNode=findNode(ppartNode,"properties");
      if(!propNode)
        throw jxtlib_exception(STDLOG,"Invalid ppart description - no <properties> node");

      string mType=getStrPropFromXml(ppartNode,"main_type");
      // С ppart для типов разбираемся отдельно - их надо применить к каждому
      // элементу нужного типа
      if(mType=="type")
      {
        string mId=getStrPropFromXml(ppartNode,"main_id");
        TElemsMap::iterator type_descr=telems.find(mId);
        if(type_descr==telems.end())
        {
          ProgTrace(TRACE1,"ppart for type '%s' which is not listed",mId.c_str());
          continue;
        }
        for(vector<string>::iterator j=type_descr->second->ids.begin();
            j!=type_descr->second->ids.end();++j)
        {
          applyPpartToInterface(propNode,props,*j);
        }
      }
      else
      {
        applyPpartToInterface(propNode,props);
      }
      // Теперь перенесем action
      xmlNodePtr node=ppartNode->children;
      while(node)
      {
        if(xmlStrcmp(node->name,"action")==0)
        {
          string a_name=getStrPropFromXml(node,"name"); // id action'а
          xmlNodePtr iActNode=findNamedNodeByProp(iMainNode,"action","name",
                                                  a_name.c_str());
          if(iActNode) // action с таким именем уже есть в интерфейсе
          {
            xmlUnlinkNode(iActNode);
            xmlFreeNode(iActNode);
          }
          iActNode=xmlCopyNode(node,1);
          xmlAddChild(iMainNode,iActNode);
        }
        node=node->next;
      }
    }
  }
}

std::string ParsedIface::getDescr()
{
  xmlFree(const_cast<xmlChar*>(ifaceDoc->encoding));
  ifaceDoc->encoding=xmlStrdup("UTF-8");
  return xml_dump(ifaceDoc);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

void createIfaceLinksForIface(XMLRequestCtxt *ctxt, xmlNodePtr resNode,
                              const std::string &iface_id, long ver)
{
  static ILanguage iLanguage=ILanguage::getILanguage();

  ParsedIface pi(iface_id,"");
  PerfomTest(__LINE__);
  int should_encode=ctxt->donotencode2UTF8();
  pi.process();
  ctxt->set_donotencode2UTF8(should_encode);
  PerfomTest(__LINE__);
  const vector<IfaceLinks> &ilinks=pi.getILinks();

  xmlNodePtr linkNode=NULL;
  vector<IfaceLinks> ilinks_to_insert;
  for(const auto ilink: ilinks)
  {
    ilinks_to_insert.push_back(ilink);
    if(ilink.type=="ipart")
      continue;
    if(!ilink.lang.empty() && ilink.lang!=iLanguage.getCode(ctxt->getLang()))
      continue;
    if(!linkNode)
      linkNode=getNode(resNode,"links");
    xmlNodePtr node=newChild(linkNode, ilink.type.c_str());
    xmlSetProp(node,"id",ilink.id);
    xmlSetProp(node,"ver",ilink.ver);
  }
  JxtlibDbCallbacks::instance()->insertIfaceLinks(iface_id, ilinks_to_insert, ver);
}

void getIfaceLinksFromDB(xmlNodePtr resNode, const std::string &iface_id,
                         long ver)
{
  static ILanguage iLanguage=ILanguage::getILanguage();
  XMLRequestCtxt *ctxt=getXmlCtxt();

  string link_name,link_type;

  if(not JxtlibDbCallbacks::instance()->ifaceLinkExists(iface_id, ver)) {
    ProgTrace(TRACE1,"re-creating iface_links");
    JxtlibDbCallbacks::instance()->deleteIfaceLinks(iface_id);
    createIfaceLinksForIface(ctxt,resNode,iface_id,ver);
    return;
  }

  auto iface_links = JxtlibDbCallbacks::instance()->getIfaceLinks(iface_id, iLanguage.getCode(ctxt->getLang()), ver);
  xmlNodePtr linkNode=0;

  for(const auto &iface: iface_links) {
    LogTrace(TRACE3) << __func__ << " iface.type = " << iface.type 
                     << " id = " << iface.id;
    if(iface.type=="ipart")
      continue;
    long ver = getXmlDataVer(iface.type, iface.id);
    if (ver < 0) {
        ProgTrace(TRACE1, "cannot get version for '%s':'%s'", iface.type.c_str(), iface.id.c_str());
        return;
    }
    string tmp = HelpCpp::string_cast(ver);
    if(!linkNode)
      linkNode=getNode(resNode,"links");
    xmlNodePtr node=newChild(linkNode,iface.type.c_str());
    xmlSetProp(node,"id",iface.id);
    xmlSetProp(node,"ver",tmp);

  }
}

/* Основная функция, добавляющая к документу все необходимые тэги для */
/* отрисовки интерфейса. */
xmlNodePtr iface(xmlNodePtr resNode, const std::string &iface_id, int handle,
                 bool no_check)
{
  if(!resNode)
  {
    ProgError(STDLOG,"iface(): resNode's NULL");
    throw jxtlib_exception(STDLOG,"Ошибка программы!");
  }

  ProgTrace(TRACE2,"iface: resNode->name=<%s>, iface_id='%s'",resNode->name,
            iface_id.c_str());

  JxtCont *jc_curr=getJxtContHandler()->currContext();
  if(!no_check)
  {
    if(handle!=jc_curr->getHandle())
    {
      ProgTrace(TRACE1,"create new handle");
      JxtHandles::createNewJxtHandle(handle);
      jc_curr=getJxtContHandler()->currContext();
    }
    if(!handle)
      ProgError(STDLOG,"iface() called for handle 0");
    xmlSetProp(resNode,"handle",handle);

    int current_is_modal=jc_curr->readInt("IS_MODAL");
    if(current_is_modal)
    {
      ProgTrace(TRACE4,"iface(): mark current window as modal");
      markCurrWindowAsModal(resNode);
    }
  }

  // получим версию интерфейса с учетом версий всех его iparts
  long ver=getXmlDataVer("interface",iface_id.c_str());

  xmlNodePtr ifaceNode=getNode(resNode,"interface");
  xmlSetProp(ifaceNode,"id",iface_id);
  xmlSetProp(ifaceNode,"ver",ver);

  /* Для того, чтобы все элементы интерфейса отобразились корректно, нужно */
  /* добавить к ответу секцию <links>, в которой необходимо перечислить    */
  /* версии всех типов, данных, iparts и pparts                            */
  getIfaceLinksFromDB(resNode,iface_id,ver);

  jc_curr->write("IFACE",iface_id);
  return ifaceNode;
}

int gotoIface(xmlNodePtr resNode, const char *iface_id, int do_not_draw)
{
  ProgTrace(TRACE2,"gotoIface: '%s'",iface_id);
  HandleParams hp(iface_id);
  int ft_handle=findHandleByParams(&hp);
  if(ft_handle>0) // окно с нужным интерфейсом найдено
  {
    closeWindow(resNode); // прикажем закрыть текущее окно
    xmlSetProp(resNode,"handle",ft_handle); // сделаем текущим найденное
    getJxtContHandler()->setCurrentContext(ft_handle);
    return 1;
  }
  if(!do_not_draw)
    iface(resNode,iface_id);
  return 0;
}

long getXmlDataVer_inner(const std::string &type, const std::string &id,
                         bool no_iparts)
{
  return JxtlibDbCallbacks::instance()->getXmlDataVer(type, id, no_iparts);
}

void parseProps(xmlNodePtr propNode, map<string,map<string,string> > &props)
{
  typedef map<string,map<string,string> >::iterator propsIt;
  xmlNodePtr node=propNode->children;
  while(node)
  {
    if(node->type==XML_COMMENT_NODE)
    {
      node=node->next;
      continue;
    }
    string name=getStrPropFromXml(node,"name");
    if(xmlStrcmp(node->name,"idref")==0) // обычные свойства
    {
      propsIt pos=props.find(name);
      if(pos==props.end()) // таких еще нет
      {
        map<string,string> iprops;
        xmlAttrPtr pr=node->properties;
        while(pr)
        {
          iprops[reinterpret_cast<const char*>(pr->name)] = pr->children->content ? reinterpret_cast<const char*>(pr->children->content) : "";
          pr=pr->next;
        }
        props[name]=iprops;
      }
      else
      {
        xmlAttrPtr pr=node->properties;
        while(pr)
        {
          map<string,string>::iterator ipos=pos->second.find(reinterpret_cast<const char*>(pr->name));
          // Сохраняем свойство, только если оно еще не задано
          if(ipos==pos->second.end()) // такого еще нет
          {
            (pos->second)[reinterpret_cast<const char*>(pr->name)]= pr->children->content ? reinterpret_cast<const char*>(pr->children->content) : "";
          }
          pr=pr->next;
        }
      }
    }
    else // "мульти-свойства"
    {
      vector<string> names;
      StrUtils::ParseStringToVecStr(names,name,',');
      string value=getStrFromXml(node);
      for(vector<string>::iterator it=names.begin();it!=names.end();++it)
      {
        propsIt pos=props.find(*it);
        if(pos==props.end())
        {
          map<string,string> iprops;
          iprops["name"]=(*it);
          iprops[reinterpret_cast<const char*>(node->name)]=value;
          props[*it]=iprops;
        }
        else
        {
          map<string,string>::iterator ipos=pos->second.find(reinterpret_cast<const char*>(node->name));
          // Сохраняем свойство, только если оно еще не задано
          if(ipos==pos->second.end()) // такого еще нет
          {
              (pos->second)[reinterpret_cast<const char*>(node->name)]=value;
          }
        }
      }
    }
    node=node->next;
  }
}

void mergeIpartIntoIface(xmlNodePtr ansNode, const std::string &ipart_name)
{
  xmlNodePtr resNode=findNodeR(ansNode,"interface");

  ProgTrace(TRACE1,"mergeIpartIntoIface (ipart '%s')",ipart_name.c_str());
  const char *ipart_type="ipart";
  long server_ver=getXmlDataVer(ipart_type,ipart_name.c_str());
  string ipart_descr(GetXmlData(ipart_type,ipart_name.c_str(),server_ver));
  getXmlCtxt()->set_donotencode2UTF8(1);
  auto ipartDoc = xml_parse_memory(ipart_descr);
  if(!ipartDoc)
    throw jxtlib_exception(STDLOG,"Unable to parse ipart data");
  xmlNodePtr iMainNode=findNodeR(ipartDoc->children,"ipart");

  // Заменим тэг ipart в интерфейсе на группу с данными из секции window ipart'а
  xmlNodePtr winNode=findNode(iMainNode,"window");
  if(!winNode)
    throw jxtlib_exception(STDLOG,"Unable to locate <window> in ipart data");
  xmlNodePtr ipartNode;
  while((ipartNode=findNamedNodeByPropR(findNode(resNode,"window"),"ipart",
                                        "id",ipart_name.c_str()))!=0)
  {
    xmlNodeSetName(ipartNode,"group");
    xmlNodePtr node=winNode->children;
    while(node)
    {
      xmlNodePtr copyNode=xmlCopyNode(node,1);
      xmlAddChild(ipartNode,copyNode);
      node=node->next;
    }
  }

  map<string,map<string,string> > props; // здесь будем хранить свойства

  // разберем свойства интерфейса
  xmlNodePtr propNode=findNode(resNode,"properties");
  if(!propNode)
    throw jxtlib_exception(STDLOG,"Unable to locate <properties> in iface data");
  parseProps(propNode,props);

  // Удалим тэги свойств интерфейса
  xmlUnlinkNode(propNode);
  xmlFreeNode(propNode);

  // Добавим к коллекции свойств props свойства ipart
  propNode=findNode(iMainNode,"properties");
  if(!propNode)
    throw jxtlib_exception(STDLOG,"Unable to locate <properties> in ipart data");
  parseProps(propNode,props);

  // Соберем все полученные свойства в интерфейс
  propNode=newChild(resNode,"properties");
  for(map<string,map<string,string> >::iterator i=props.begin();i!=props.end();++i)
  {
    xmlNodePtr idrefNode=newChild(propNode,"idref");
    for(map<string,string>::iterator j=i->second.begin();j!=i->second.end();++j)
      xmlSetProp(idrefNode,j->first.c_str(),j->second);
  }

  // разберемся со static
  xmlNodePtr ipartStatic=findNode(iMainNode,"static");
  xmlNodePtr ifaceStatic=NULL;
  if(ipartStatic)
    ipartStatic=ipartStatic->children;
  while(ipartStatic)
  {
    xmlNodePtr tmpNode=xmlCopyNode(ipartStatic,1);
    if(!ifaceStatic)
      ifaceStatic=getNode(resNode,"static");
    xmlAddChild(ifaceStatic,tmpNode);
    ipartStatic=ipartStatic->next;
  }
}

long lib_getXmlDataVer(const std::string &type, const std::string &id)
{
  if(type=="data")
    return getDataVer(id.c_str());

  return getXmlDataVer_inner(type,id,true);
}

long getXmlDataVer(const std::string &type, const std::string &id)
{
  ProgTrace(TRACE5,"getXmlDataVer for type='%s', id='%s'",type.c_str(),id.c_str());
  if(type=="data")
    return getDataVer(id.c_str());
  return lib_getXmlDataVer(type,id);
}

static long lib_getDataVer(const std::string &data_id)
{
  return JxtlibDbCallbacks::instance()->getDataVer(data_id);
}

long getDataVer(const char *data_id)
{
  ProgTrace(TRACE2,"getDataVer for data_id='%s'",data_id);
  if (!data_id)
    throw jxtlib_exception(STDLOG,"getDataVer(NULL)!");

  // Проверим не надо ли обработать этот тип данных отдельно
  const XmlDataDesc *xdd=JXTLib::Instance()->getDataImpl(data_id);
  if (xdd)
    return xdd->getDataVersion(data_id);

  return lib_getDataVer(data_id);
}

void updateJxtData(const char *id, long term_ver, xmlNodePtr resNode)
{
  ProgTrace(TRACE2,"updateJxtData ('%s', %li)",id,term_ver);
  if(!id)
    throw jxtlib_exception("updateJxtData: id is NULL!");

  xmlNodePtr updateNode=0, deleteNode=0, rootNode=0;
  /* имена тегов имена полей таблицы */
  int i,col_num,upd_rec_num,key_ind=0;
  int reset_ind=0;

  /* узнаем версию данных на сервере */
  long serv_ver = getDataVer(id);
  if (serv_ver < 0) {
    ProgTrace(TRACE1,"cannot get current version for data '%s'",id);
    throw jxtlib_exception(STDLOG,"Ошибка программы!");
  }

  //catch(...)
  //{
    //ProgTrace(TRACE1,"Unknown exception catched!");
    //throw;
  //}

  ProgTrace(TRACE5,"serv_ver=%li",serv_ver);

  // Проверим не надо ли обработать этот тип данных отдельно
  try
  {
    const XmlDataDesc *xdd=JXTLib::Instance()->getDataImpl(id);
    if(xdd)
      return xdd->getData(id,resNode,term_ver,serv_ver);
  }
  catch(jxtlib_exception &e)
  {
    ProgTrace(TRACE1,"updateJxtData() catched e: '%s'",e.what());
    throw;
  }

  /* добавили "data" */
  /* resNode пусть теперь все время указывает на тег <data> */
  xmlNodePtr dataNode=getNode(resNode,"data");
  xmlSetProp(dataNode,"id",id);

  const auto relid = JxtlibDbCallbacks::instance()->getRelIdTab(id);

  ProgTrace(TRACE5,"relid.root='%s', relid.tabname='%s'",relid.root.c_str(), relid.tabname.c_str());

  xmlSetProp(dataNode,"ver",serv_ver);
  xmlSetProp(dataNode,"root",relid.root);
  if(serv_ver<term_ver || term_ver==0 || !relid.undeletable)
    reset_ind=1;

  if(serv_ver!=term_ver)
  {
    /* узнаем имена необходимых полей и имена тегов, в которых */
    /* возвращать их значения */
    const auto reltabList = JxtlibDbCallbacks::instance()->getRelTabcolTab(id);
    /* счетчик кол-ва полей */
    col_num=0;
    for(const auto reltab: reltabList)
    {
      if(reltab.key == 1)
      {
        xmlSetProp(dataNode,"name",reltab.tagname);
        key_ind=col_num;
      }
      col_num++;
    } // end of while

    // узнаем, есть ли поле close
    // если нет, то у нас - вечный reset и не достаем close

    const auto tab_values = JxtlibDbCallbacks::instance()->getTabValues(relid, reltabList, reset_ind, term_ver);
    upd_rec_num = 0;
    for(const auto &tabval: tab_values)
    {
      if(tabval.close)
      {
        /* эта запись удалена в этой версии */
        if(!reset_ind)
        {
          /* если не устанавливали reset    */
          /* надо добавить вершину <delete> */
          deleteNode=newChild(dataNode,"delete");
          xmlSetProp(deleteNode,"value",tabval.tabcols[key_ind]);
        }
      }
      else
      {
        if((reset_ind && upd_rec_num==0) || !reset_ind)
          updateNode=newChild(dataNode,"update");
        upd_rec_num++;

        if(!reset_ind)
          xmlSetProp(updateNode,"value",tabval.tabcols[key_ind]);
        rootNode=newChild(updateNode,relid.root.c_str());
        for(i=0;i<col_num;i++)
        {
          /* добавляем теги с данными */
          xmlNewTextChild(rootNode,NULL,reltabList[i].tagname,(tabval.ind[i]<0)?" ":tabval.tabcols[i]);
        }
      }
    }
  }
  if(reset_ind)
    xmlSetProp(dataNode,"reset","true");
}

long getNewestPluginVer(const char *id, const char *ext)
{
  long termver=0;
  struct dirent *dir;
/*  struct dirent **namelist;
  int i,n;*/
  int idlen;
  char *ptr;
  long ver;
  char vers[20];
  char end[50];
  int extlen;
  DIR *plugdir=NULL;

  ProgTrace(TRACE2,"getNewestPlugin(%s.%s)",id,ext);

  if(!id || !ext)
    return -1;
  idlen=strlen(id);
  sprintf(end,".%s",ext);
  extlen=strlen(end);
  if((plugdir=opendir("../plugins"))==NULL)
  {
    ProgError(STDLOG,"Cannot opendir(\"../plugins\") !");
    return -1;
  }
  /*n=scandir("../plugins",&namelist,0,alphasort);*/
  while((dir=readdir(plugdir))!=NULL)
  {
/*  for(i=0;i<n;i++)*/
  /*{*/
    int len=strlen(dir->d_name);
    if(len<idlen+6 || strcmp(dir->d_name+len-extlen,end)!=0 ||
       strncmp(dir->d_name,id,idlen)!=0 ||
       dir->d_name[idlen]!='-' || len-idlen-5>19)
    {
      continue;
    }
    strncpy(vers,dir->d_name+idlen+1,len-idlen-5);
    vers[len-idlen-5]='\0';
    ver=strtol(vers,&ptr,10);
    if(ptr[0]!='\0')
    {
      continue;
    }
    if(ver>termver)
      termver=ver;
  }
  closedir(plugdir);
  return termver;
}

std::vector<char> readBinary(const char *id, const char *ext, long ver, int *err)
{
  char name[100];
  FILE *pl=NULL;

  if(*err==0 && (!id || strlen(id)>50 || !ext))
  {
    ProgTrace(TRACE1,"id='%s', ext='%s'",id,ext);
    *err=-1;
  }
  if(*err==0)
  {
    sprintf(name,"../plugins/%s-%li.%s",id,ver,ext);
    if((pl=fopen(name,"r"))==NULL)
    {
      ProgError(STDLOG,"Cannot open file '%s' for reading",name);
      *err=-1;
    }
  }

  std::vector<char> res;
  while(*err==0 && feof(pl)==0)
  {
    char buf[5000] = {};
    size_t bytes_read = fread(buf, sizeof(char), 5000, pl);
    res.insert(res.end(), buf, buf+bytes_read);
  }
  if(*err==0 && ferror(pl)!=0)
  {
    ProgError(STDLOG,"reading file '%s' failed with code %i!!!",name,ferror(pl));
    *err=-1;
  }
  fclose(pl);
  if(*err!=0)
      res.clear();
  ProgTrace(TRACE5,"Read %zu bytes",res.size());
  return res;
}

void findIparts(xmlNodePtr node, vector<IfaceLinks> &ilinks)
{
  while(node)
  {
    if(xmlStrcmp(node->name,reinterpret_cast<const xmlChar*>("ipart"))==0)
    {
      xmlAttrPtr pr=node->properties;
      while(pr)
      {
        if(xmlStrcmp(pr->name,reinterpret_cast<const xmlChar*>("id"))==0)
        {
          string ipart_id=pr->children->content?(const char *)pr->children->content:"";
          if(!ipart_id.empty())
          {
            ilinks.push_back(IfaceLinks("ipart",ipart_id,"",false));
          }
          break;
        }
        pr=pr->next;
      }
    }
    findIparts(node->children,ilinks);
    node=node->next;
  }
}

void insertXmlData(const std::string &type, const std::string &id,
                   const std::string &data)
{
  JxtlibDbCallbacks::instance()->insertXmlStuff(type, id, data);
  if(type=="interface")
  {
    // Удалим все iface_links и добавим их для ipart'ов
    xmlDocPtr iDoc=0;
    xmlKeepBlanksDefault(0);
    if((iDoc=xmlParseMemory(data.c_str(),data.size()))==NULL)
    {
      ProgError(STDLOG,"Xml for %s %s must contain errors!",type.c_str(),
                id.c_str());
      throw jxtlib_exception(STDLOG,"Ошибка программы!");
    }
    xmlNodePtr winNode=findNodeR(iDoc->children,"window");
    std::vector<IfaceLinks> ilinks;
    findIparts(winNode->children,ilinks);

    JxtlibDbCallbacks::instance()->deleteIfaceLinks(id);

     std::vector<IfaceLinks> ilinks_to_insert;
    for(const auto &ilink: ilinks)
      if(ilink.type=="ipart")
        ilinks_to_insert.push_back(ilink);

    JxtlibDbCallbacks::instance()->insertIfaceLinks(id, ilinks, 0);
  }
}

void InsertXmlData(const char *type, const char *id, const std::string &data)
{
  ProgTrace(TRACE2,"InsertXmlData('%s','%s')",type,id);
  if(!type || !id)
    throw jxtlib_exception(STDLOG,"InsertXmlData() got invalid params!");
  return insertXmlData(type,id,data);
}

std::string getXmlData(const std::string &type,
                       const std::string &id, long ver)
{
  return JxtlibDbCallbacks::instance()->getXmlData(type, id, ver);
}

string GetXmlData2(const char *type, const char *id, long query_ver)
{
  if(!type || !id)
  {
    ProgTrace(TRACE1,"GetXmlData2() got invalid params!");
    throw jxtlib_exception(STDLOG,"GetXmlData2() got invalid params!");
  }
  return getXmlData(type,id,query_ver);
}

string GetXmlData(const char *type, const char *id, long query_ver)
{
  ProgTrace(TRACE2,"GetXmlData: '%s', '%s', %li",type,id,query_ver);
  string str=GetXmlData2(type,id,query_ver);
  if(str.find("?>")==string::npos)  return string();
  StrUtils::replaceSubstr(str,"?>","?><term><answer handle=\"0\">");
  StrUtils::replaceSubstr(str,"ver=\"\"","ver=\""+HelpCpp::string_cast(query_ver)+"\"");
  str+="</answer></term>";
  return str;
}

xmlDocPtr getXmlStuffDoc(const string &id, const string &type)
{
  string xmltext=GetXmlData(type.c_str(),id.c_str(),
                            getXmlDataVer(type.c_str(),id.c_str()));
  xmlKeepBlanksDefault(0);
  return xmlParseMemory(xmltext.c_str(),xmltext.size());
}

namespace
{
int YYYYfromYY(int yy)
{
  if(yy<0 || yy>99)
  {
    ProgTrace(TRACE1,"InvalidYY: (%i)",yy);
    throw jxtlib::jxtlib_exception("InvalidYY");
  }
  return (yy>50)?1900+yy:2000+yy;
}
int YYYYfromYY(const char *yy)
{
  if(!yy || strlen(yy)!=2)
  {
    ProgTrace(TRACE1,"InvalidYY: '%s'",yy);
    throw jxtlib::jxtlib_exception("InvalidYY");
  }
  return YYYYfromYY(atoiNVL(yy,-1));
}
} // namespace

extern "C" void addPeriodConstraintDDMMRR(const char *date1, const char *date2,
                                          const char *tag, xmlNodePtr resNode)
// date1 and date2 in DDMMRR format
{
  char per[30];
  sprintf(per,"%.2s/%.2s/%i-%.2s/%.2s/%i",date1,date1+2,
          YYYYfromYY(date1+4),date2,date2+2,YYYYfromYY(date2+4));
  if(!resNode)
    resNode=getResDoc()->children->children;
  setElemProp(resNode,tag,"period",per);
}

extern "C" void addPeriodConstraintRRMMDD(const char *date1, const char *date2,
                                          const char *tag, xmlNodePtr resNode)
// date1 and date2 in RRMMDD format
{
  char per[30];
  int y1,y2;
  sprintf(per,"%.2s",date1);
  y1=YYYYfromYY(per);
  sprintf(per,"%.2s",date2);
  y2=YYYYfromYY(per);
  sprintf(per,"%.2s/%.2s/%i-%.2s/%.2s/%i",date1+4,date1+2,y1,
          date2+4,date2+2,y2);
  if(!resNode)
    resNode=getResDoc()->children->children;
  setElemProp(resNode,tag,"period",per);
}

extern "C" void addFrequencyConstraint(const char *freq)
{
  const char *days[]={"mon,mon_l","tue,tue_l","wed,wed_l","thu,thu_l","fri,fri_l","sat,sat_l","sun,sun_l"};
  xmlNodePtr propNode=getNode(getResNode(),"properties");
  for(int i=0; i<7; i++)
  {
      if(freq[i]=='.')
          xmlSetProp(xmlNewTextChild(propNode,NULL,"enable","false"),"name",days[i]);
  }
  xmlSetProp(xmlNewTextChild(propNode,NULL,"visible","false"),"name","ex,ex_l");
}

extern "C" void closeUserSession(const char *pult, xmlNodePtr resNode)
{
  ProgTrace(TRACE2,"closeUserSession");
  xmlNodePtr comNode=NULL;
  xmlNodePtr killNode;

  JxtContHandler *jch=getJxtContHandler();
  vector<int> contexts_vec;
  jch->getNumberOfContexts(contexts_vec);
  for(int i=jch->getMaxContextsNum();i>=0;--i)
  {
    if(contexts_vec[i]==0) // no such handle opened
      continue;
    jch->deleteContext(i);
    if(i>0)
    {
      if(!comNode)
        comNode=getNode(resNode,"command");
      killNode=newChild(comNode,"kill");
      xmlSetProp(killNode,"handle",i);
    }
  }
}

void markCurrWindowAsModal(xmlNodePtr resNode)
{
  xmlSetProp(resNode, reinterpret_cast<const xmlChar*>("modal"), reinterpret_cast<const xmlChar*>("true"));
  getCurrContext()->write("IS_MODAL","1");
}

