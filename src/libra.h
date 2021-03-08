#pragma once

#include "xml_unit.h"

#include <serverlib/oci8.h>
#include <jxtlib/JxtInterface.h>

#include <boost/optional.hpp>


class LibraInterface : public JxtInterface
{
public:
    LibraInterface() : JxtInterface("","libra")
    {
        AddEvent("request", JXT_HANDLER(LibraInterface, RequestHandler));
        AddEvent("kick",    JXT_HANDLER(LibraInterface, KickHandler));
    }

    void RequestHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};


namespace LIBRA
{

// может имеет смысл в существующий алгоритм поиска компоновки для назначения на рейс встроить поиск в БД Либры?
// если нашли в Либре, то поиск в наших аналога. Если нашли - назначили все недостоющие свойства, не нашли - пустая компоновка только на основе Либры.
// пусть вручную добавляют свойства. Поиск аналога у нас только по борту или еще по конфирурации и типу ВС?

class TCompon
{
private:
  void ComponFromBort( const std::string& bort );
  void ReadCompon( int comp_id );
  void ReadSections( int comp_id );
  void ConfigList( int comp_id );
public:
};

//---------------------------------------------------------------------------------------

struct LibraHttpResponse
{
    struct Status
    {
        static const std::string Ok;
        static const std::string Err;
    };

public:
    static boost::optional<LibraHttpResponse> read();

    xmlNodePtr resultNode() const;

protected:
    explicit LibraHttpResponse(const std::string& resp);

private:
    std::string             m_resp;
    boost::optional<XMLDoc> m_respDoc;
};

//---------------------------------------------------------------------------------------

bool needSendHttpRequest();
void synchronousHttpRequest(const std::string& req, const std::string& path);
void synchronousHttpRequest(xmlDocPtr reqDoc, const std::string& path);
std::string synchronousHttpExchange(const std::string& req, const std::string& path);
void asynchronousHttpRequest(const std::string& req, const std::string& path);
std::string receiveHttpResponse();

}//namespace LIBRA
