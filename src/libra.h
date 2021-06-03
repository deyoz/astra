#pragma once

#include "xml_unit.h"

#include <serverlib/oci8.h>
#include <serverlib/str_utils.h>
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
bool LIBRA_ENABLED();

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

void synchronousHttpGetRequest(const std::string& req, const std::string& path);
void synchronousHttpPostRequest(xmlDocPtr reqDoc, const std::string& path);

void synchronousHttpGetRequest(xmlDocPtr reqDoc, const std::string& path);
void synchronousHttpPostRequest(const std::string& req, const std::string& path);

std::string synchronousHttpGetExchange(const std::string& req, const std::string& path);
std::string synchronousHttpPostExchange(const std::string& req, const std::string& path);

void asynchronousHttpGetRequest(const std::string& req, const std::string& path);
void asynchronousHttpPostRequest(const std::string& req, const std::string& path);

std::string receiveHttpResponse();

struct FieldData
{
public:
    enum class Type
    {
        Integer,
        String,
        DateTime
    };

    FieldData(int row_number, xmlNodePtr col_node)
        : row_number(row_number)
    {
        data_type = getDataType(getStrPropFromXml(col_node, "type"));
        value = getStrFromXml(col_node);
        is_null = getIsNull(StrUtils::ToLower(getStrPropFromXml(col_node, "null")));
        date_format = getStrPropFromXml(col_node, "format");
    }

    int fieldAsInteger() const;
    const std::string& fieldAsString() const;
    BASIC::date_time::TDateTime fieldAsDateTime() const;

private:
    int row_number;
    Type data_type;
    std::string value;
    bool is_null;
    std::string date_format;

    Type getDataType(const std::string& date_type);
    bool getIsNull(const std::string& is_null);
};

typedef std::string FieldName_t;
typedef std::map<FieldName_t, FieldData> RowData;

std::string makeHttpQueryString(const std::map<std::string,std::string>& dict);
std::vector<RowData> getHttpRequestDataRows(const std::string& request,
                                            const std::string& params);
RowData getHttpRequestDataRow(const std::string& request,
                              const std::string& params);

}//namespace LIBRA
