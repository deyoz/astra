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

// ����� ����� ��� � �������騩 ������ ���᪠ ���������� ��� �����祭�� �� ३� ���ந�� ���� � �� �����?
// �᫨ ��諨 � ����, � ���� � ���� �������. �᫨ ��諨 - �����稫� �� �������騥 ᢮��⢠, �� ��諨 - ����� ���������� ⮫쪮 �� �᭮�� �����.
// ����� ������ ��������� ᢮��⢠. ���� ������� � ��� ⮫쪮 �� ����� ��� �� �� �������樨 � ⨯� ��?

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
