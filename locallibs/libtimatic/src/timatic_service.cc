#include "timatic_service.h"
#include "timatic_exception.h"
#include "timatic_xml.h"

#include <serverlib/httpsrv.h>
#include <serverlib/query_runner.h>
#define NICKNAME "TIMATIC"
#include <serverlib/slogger.h>

#include <openssl/md5.h>

namespace Timatic {

static const int EXPIRE_SECS = 60 * 27;

static void sendHttp(const Config &config, Session &session, const Request &request, const ServerFramework::InternalMsgId &msgid)
{
    const std::string content = request.content(session);
    if (content.empty())
        LogTrace(TRACE1) << __FUNCTION__ << ": content is empty, request.type=" << (int)request.type();

    const std::string header =
                "POST /timaticwebservices/timatic3services HTTP/1.0\r\n"
                "Host: " + config.host + ":" + std::to_string(config.port) + "\r\n"
                + (session.jsessionID.empty() ? "" : "Cookie: JSESSIONID=" + session.jsessionID + ";\r\n") +
                "Content-Type: text/xml; charset=utf-8\r\n"
                "Content-Length: " + std::to_string(content.size()) + "\r\n\r\n";

    httpsrv::DoHttpRequest(msgid,
                           httpsrv::Domain("TIMATIC"),
                           httpsrv::HostAndPort(config.host, config.port),
                           header + content)();

    session.expires = time(nullptr) + EXPIRE_SECS;
}

static std::vector<HttpData> fetchHttp(const ServerFramework::InternalMsgId &msgid)
{
    auto resps = httpsrv::FetchHttpResponses(msgid, httpsrv::Domain("TIMATIC"));
    if (resps.empty()) {
        LogTrace(TRACE1) << __FUNCTION__ << " has no responses";
        return {};
    }

    std::vector<HttpData> datas;
    datas.reserve(resps.size());
    for (const auto &resp : resps)
        datas.emplace_back(resp);

    return datas;
}

//-----------------------------------------------

static std::string md5(const std::string &str)
{
    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5_CTX md5;
    MD5_Init(&md5);
    MD5_Update(&md5, str.c_str(), str.size());
    MD5_Final(hash, &md5);
    char outputBuffer[MD5_DIGEST_LENGTH * 2 + 1] = {0};
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i)
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    return std::string(outputBuffer);
}

static Optional<HttpData> processLogin(const Config &config, Session &session, const Request &request, const ResponseType expectedType)
{
    const auto &msgid = ServerFramework::getQueryRunner().getEdiHelpManager().msgId();
    sendHttp(config, session, request, msgid);
    const auto httpDatas = fetchHttp(msgid);
    LogTrace(TRACE1) << __FUNCTION__ << ": httpDatas.size=" << httpDatas.size();

    if (!httpDatas.empty()) {
        const auto &httpData = httpDatas.back();

        if (httpData.response() == nullptr) {
            LogTrace(TRACE1) << __FUNCTION__ << ": response is null";
            return {};
        }

        if (httpData.response()->type() == ResponseType::Error) {
            for (const auto &err : httpData.get<ErrorResp>().errors())
                LogTrace(TRACE1) << __FUNCTION__ << ": message=" << err.message << " code=" << (err.code ? *err.code : "");
            return {};
        }

        if (httpData.response()->type() == expectedType)
            return httpData;
    }

    return {};
}

static Optional<HttpData> doCheckNameReq(const Config &config, Session &session)
{
    CheckNameReq request;
    request.requestID(1);
    request.subRequestID(std::to_string(time(nullptr)));
    request.username(config.username);
    request.subUsername(config.subUsername);

    return processLogin(config, session, request, ResponseType::CheckName);
}

static Optional<HttpData> doLoginReq(const Config &config, Session &session, const CheckNameResp &checkResp)
{
    LoginReq request;
    request.requestID(2);
    request.subRequestID(std::to_string(time(nullptr)));
    request.loginInfo(md5(md5(config.password) + checkResp.randomNumber() + checkResp.sessionID()));

    return processLogin(config, session, request, ResponseType::Login);
}

static Optional<Session> createSession(const Config &config)
{
    // send checkNameRequest
    // got checkNameResponse/errorResponse
    // calc md5 loginInfo
    // send loginRequest
    // got loginResponse/errorResponse
    // init session values

    Session tmp;
    tmp.expires = time(nullptr);

    //

    Optional<HttpData> checkHttpData = doCheckNameReq(config, tmp);
    if (!checkHttpData) {
        LogTrace(TRACE1) << __FUNCTION__ << ": no CheckNameResponse";
        return {};
    }

    CheckNameResp checkResp = checkHttpData->get<CheckNameResp>();
    tmp.sessionID = checkResp.sessionID();
    tmp.jsessionID = checkHttpData->jsessionID();
    tmp.expires = time(nullptr);

    //

    Optional<HttpData> loginHttpData = doLoginReq(config, tmp, checkResp);
    if (!loginHttpData) {
        LogTrace(TRACE1) << __FUNCTION__ << ": no LoginResponse";
        return {};
    }

    LoginResp loginResp = loginHttpData->get<LoginResp>();
    if (!loginHttpData->jsessionID().empty())
        tmp.jsessionID = loginHttpData->jsessionID();
    tmp.expires = time(nullptr);

    //

    LogTrace(TRACE1) << __FUNCTION__ << ": successfulLogin=" << loginResp.successfulLogin();
    if (loginResp.successfulLogin())
        return tmp;
    else
        return {};
}

//-----------------------------------------------

void initTestSuite() 
{
}

//-----------------------------------------------

Service::Service(const Config &config)
    : config_(config)
{
    reset();
}

bool Service::ready() const
{
    if (time(nullptr) < session_.expires)
        return true;
    else
        return false;
}

void Service::reset()
{
    Optional<Session> session = createSession(config_);
    if (session)
        session_ = *session;
    else
        throw SessionError(BIGLOG, "cann't create session");
}

void Service::send(const Request &request, const ServerFramework::InternalMsgId &msgid)
{
    if (!ready())
        reset();

    sendHttp(config_, session_, request, msgid);
}

std::vector<HttpData> Service::fetch(const ServerFramework::InternalMsgId &msgid)
{
    return fetchHttp(msgid);
}

} // Timatic

//===============================================

#ifdef XP_TESTING

#include <serverlib/checkunit.h>
#include <serverlib/xp_test_utils.h>

#include "timatic_exception.h"
#include <iostream>

void initTimaticTests() {}

static void testInitTimatic()
{
    initTest();
}

START_TEST(timatic_service_create)
{
    using namespace Timatic;
    httpsrv::xp_testing::set_need_real_http();
    Service service(Config("1H", "SirenaXML", "Hg66SvP+wKKj2s", "staging.timatic.aero", 443));
}
END_TEST

START_TEST(timatic_doc_resp)
{
    using namespace Timatic;

    const std::string content =
            R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
            <env:Envelope xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:env="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
              <env:Header>
                <m:sessionID xmlns:m="http://www.opentravel.org/OTA/2003/05/beta" actor="http://schemas.xmlsoap.org/soap/actor/next" mustUnderstand="0">166866e7:16eb6dcfef6:-6bf3</m:sessionID>
              </env:Header>
              <env:Body>
                <m:processRequestResponse xmlns:m="http://www.opentravel.org/OTA/2003/05/beta">
                  <documentResponse xmlns="http://www.opentravel.org/OTA/2003/05/beta">
                    <requestID>3</requestID>
                    <customerCode>25075</customerCode>
                    <documentCheckResponse>
                      <sufficientDocumentation>Conditional</sufficientDocumentation>
                      <documentCountryInformation>
                        <countryCode>RU</countryCode>
                        <countryName>Russian Fed.</countryName>
                        <destinationCode>DME</destinationCode>
                        <sufficientDocumentation>Conditional</sufficientDocumentation>
                        <sectionInformation>
                          <sectionName>Geographical Information</sectionName>
                          <documentParagraph paragraphId="11172">
                            <paragraphType>Information</paragraphType>
                            <paragraphText>Capital - Moscow (MOW). The Russian Fed. is a member state of the Commonwealth of Independent States.</paragraphText>
                          </documentParagraph>
                        </sectionInformation>
                        <sectionInformation>
                          <sectionName>Passport</sectionName>
                          <documentParagraph paragraphId="20698">
                            <paragraphType>Requirement</paragraphType>
                            <paragraphText>Passport required.</paragraphText>
                          </documentParagraph>
                          <subsectionInformation>
                            <subsectionName>Document Validity:</subsectionName>
                            <documentParagraph paragraphId="26080">
                              <paragraphType>Requirement</paragraphType>
                              <paragraphText>Passports and other documents accepted for entry must be valid on arrival. </paragraphText>
                            </documentParagraph>
                          </subsectionInformation>
                        </sectionInformation>
                        <sectionInformation>
                          <sectionName>Visa</sectionName>
                          <documentParagraph paragraphId="26077">
                            <paragraphType>Requirement</paragraphType>
                            <paragraphText>
                              <p>Visa required.</p>
                            </paragraphText>
                          </documentParagraph>
                          <subsectionInformation>
                            <subsectionName>Warning:</subsectionName>
                            <documentParagraph paragraphId="43229">
                              <paragraphType>Information</paragraphType>
                              <paragraphText>Visitors not holding return/onward tickets could be refused entry. <notesReference paragraphId="43399"/></paragraphText>
                              <documentChildParagraph noteType="notesRef" paragraphId="43399">
                                <paragraphType>Information</paragraphType>
                                <paragraphText>This does not apply to passengers with a valid visa. </paragraphText>
                              </documentChildParagraph>
                            </documentParagraph>
                            <documentParagraph paragraphId="74368">
                              <paragraphType>Information</paragraphType>
                              <paragraphText>Valid visas in full or expired travel documents are not accepted.</paragraphText>
                            </documentParagraph>
                          </subsectionInformation>
                        </sectionInformation>
                        <sectionInformation>
                          <sectionName>Health</sectionName>
                          <documentParagraph paragraphId="20652">
                            <paragraphType>Information</paragraphType>
                            <paragraphText>Vaccinations not required.</paragraphText>
                          </documentParagraph>
                        </sectionInformation>
                        <sectionInformation>
                          <sectionName>Airport Tax</sectionName>
                          <documentParagraph paragraphId="11181">
                            <paragraphType>Information</paragraphType>
                            <paragraphText>No airport tax is levied on passengers upon embarkation at the airport.</paragraphText>
                          </documentParagraph>
                        </sectionInformation>
                        <sectionInformation>
                          <sectionName>Customs</sectionName>
                          <subsectionInformation>
                            <subsectionName>Import:</subsectionName>
                            <documentParagraph paragraphId="30612">
                              <paragraphType>Information</paragraphType>
                              <paragraphText>Free import by persons of 18 years and older:<br/>1. 400 cigarettes or 200 cigarillos or 100 cigars or 500 grams of tobacco products if only one type of tobacco products is imported (otherwise only half of the quantities allowed);<br/>2. only for persons of 21 years and older: alcoholic beverages: 3 liters;<br/>3. a reasonable quantity of perfume for personal use;<br/>4. goods up to an amount of EUR 10,000.- for personal use only;<br/>5. caviar (factory packed) max. 250 grams per person. </paragraphText>
                            </documentParagraph>
                            <documentParagraph paragraphId="30613">
                              <paragraphType>Information</paragraphType>
                              <paragraphText>Prohibited:<br/>- Photographs and printed matter directed against Russian Fed. <br/>Acipenseridae family (sturgeons) fish and any product made thereof and live animals (subject to special permit). Military arms and ammunition, narcotics, fruit and vegetables.<br/>Dairy and meat products from Armenia and Georgia. Poultry and poultry products from China (People's Rep.). </paragraphText>
                            </documentParagraph>
                          </subsectionInformation>
                          <subsectionInformation>
                            <subsectionName>Arms and Ammunition:</subsectionName>
                            <documentParagraph paragraphId="20672">
                              <paragraphType>Information</paragraphType>
                              <paragraphText>Import and export permit required. Arms and ammunition can only be carried as checked baggage. Import and export of military arms and ammunition is prohibited. </paragraphText>
                            </documentParagraph>
                          </subsectionInformation>
                          <subsectionInformation>
                            <subsectionName>Additional Information:</subsectionName>
                            <documentParagraph paragraphId="11245">
                              <paragraphType>Information</paragraphType>
                              <paragraphText>The customs declaration, filled out on entry of the Russian Fed., must be handed in on departure.</paragraphText>
                            </documentParagraph>
                            <documentParagraph paragraphId="30777">
                              <paragraphType>Information</paragraphType>
                              <paragraphText>Dairy and meat products are permitted (except from Armenia and Georgia), if packed in original factory packing and in quantities that can be considered for personal use. Larger quantities must be accompanied by the appropriate documentation, including veterinary health documents. </paragraphText>
                            </documentParagraph>
                          </subsectionInformation>
                          <subsectionInformation>
                            <subsectionName>Export:</subsectionName>
                            <documentParagraph paragraphId="11183">
                              <paragraphType>Information</paragraphType>
                              <paragraphText>1. Archeological, historical and artistic objects require a written permission from the Ministry of Cultural Affairs and a photograph of the exported object.<br/>2. A permit from the Internal Affairs Ministry required for import of weapons.</paragraphText>
                            </documentParagraph>
                            <documentParagraph paragraphId="20680">
                              <paragraphType>Information</paragraphType>
                              <paragraphText>Prohibited: <br/>- Precious metals, stones, pearls and articles made thereof, except when belonging to the imported personal effects of the passenger (e.g. jewellery) and declared upon arrival.<br/>- Acipenseridae family (sturgeons) fish and any product made thereof (subject to special permit). <br/>- Antiquities (i.e. any article older than 50 years) and art objects (subject to duty and special permit from the Ministry of Culture), furs. </paragraphText>
                            </documentParagraph>
                          </subsectionInformation>
                          <subsectionInformation>
                            <subsectionName>Pets:</subsectionName>
                            <documentParagraph paragraphId="11182">
                              <paragraphType>Information</paragraphType>
                              <paragraphText>Cats, dogs and birds must be accompanied by an international veterinary passport or veterinarian health certificate (nationals of C.I.S. countries may present a veterinary license instead) with seal of local Board of Health and issued no more than five days prior to arrival. Pets must have been vaccinated against rabies within 12 months and 30 days prior to entry. Pigeons must be accompanied by a certificate which proves they are free from bird flu.<br/>Special import permission from Veterinary dept. Ministry of Agriculture is required for import of more than 2 animals of any type. </paragraphText>
                            </documentParagraph>
                          </subsectionInformation>
                          <subsectionInformation>
                            <subsectionName>Baggage Clearance:</subsectionName>
                            <documentParagraph paragraphId="11184">
                              <paragraphType>Information</paragraphType>
                              <paragraphText>Baggage must be cleared at the first airport of entry in the Russian Fed.</paragraphText>
                              <documentChildParagraph paragraphId="54088">
                                <paragraphType>Information</paragraphType>
                                <paragraphText><strong>Exempt:</strong><br/>1. baggage of passengers arriving on certain airlines. Contact your airline for further details;<br/>2. baggage of transit passengers to Sochi if airline conforms to Customs procedures. Contact your airline for further details;<br/>3. baggage of transit passengers continuing to a third country. The baggage must be checked through to its final destination irrespective of terminal of arrival/departure.<br/>If traveling onward to Armenia, Belarus, Kazakhstan and Kyrgyzstan, please contact your airline for further details. <br/></paragraphText>
                              </documentChildParagraph>
                            </documentParagraph>
                            <documentParagraph paragraphId="33377">
                              <paragraphType>Information</paragraphType>
                              <paragraphText>For travelers entering the Russian Fed., a limit of baggage allowance of 50 kilograms and/or a value of RUB 65,000.- exists. </paragraphText>
                              <documentChildParagraph paragraphId="33378">
                                <paragraphType>Information</paragraphType>
                                <paragraphText><strong>Exempt:</strong> nationals of the Russian Fed. whose stay abroad exceeds 6 months.</paragraphText>
                              </documentChildParagraph>
                            </documentParagraph>
                          </subsectionInformation>
                        </sectionInformation>
                        <sectionInformation>
                          <sectionName>Currency</sectionName>
                          <subsectionInformation>
                            <subsectionName>Import:</subsectionName>
                            <documentParagraph paragraphId="11249">
                              <paragraphType>Information</paragraphType>
                              <paragraphText>Import allowed.<br/>Local currency: (Russian Rouble - RUB): and foreign currencies: no restrictions. <br/>Written import declaration may be needed on export.</paragraphText>
                            </documentParagraph>
                          </subsectionInformation>
                          <subsectionInformation>
                            <subsectionName>Export:</subsectionName>
                            <documentParagraph paragraphId="11185">
                              <paragraphType>Information</paragraphType>
                              <paragraphText>Export allowed.<br/>Local currency (Russian Rouble - RUB) and foreign currencies: Written declaration required for amounts over 3,000.-. Import declaration / confirmation of import or transfer required for amounts over 10,000.-. Traveler's cheques are allowed up to amounts imported. Declaration required for traveler's cheques purchased in Russia.</paragraphText>
                            </documentParagraph>
                          </subsectionInformation>
                        </sectionInformation>
                      </documentCountryInformation>
                    </documentCheckResponse>
                  </documentResponse>
                </m:processRequestResponse>
              </env:Body>
            </env:Envelope>)";

    XmlDoc doc = XmlDoc::impropriate(xmlParseMemory(content.data(), content.size()));
    DocumentResp resp(xFindNodeR(xmlDocGetRootElement(doc.get()), "documentResponse"));

    ck_assert_int_eq(resp.requestID(), 3);
    ck_assert_str_eq(*resp.customerCode(), "25075");

    fail_unless(resp.documentCheckResp());
    const DocumentCheckResp &dcr = *resp.documentCheckResp();
    fail_unless(dcr.sufficientDocumentation() == Sufficient::Conditional);

    ck_assert_int_eq(dcr.documentCountryInfo().size(), 1);
    const DocumentCountryInfo &dci = dcr.documentCountryInfo().front();
    ck_assert_str_eq(dci.countryCode(), "RU");
    ck_assert_str_eq(dci.countryName(), "Russian Fed.");
    ck_assert_int_eq(dci.sectionInformation().size(), 7);

    const DocumentSection ds0 = dci.sectionInformation()[0];
    ck_assert_str_eq(ds0.sectionName(), "Geographical Information");
    ck_assert_int_eq(ds0.documentParagraph().size(), 1);

    const DocumentParagraphSection dp0 = ds0.documentParagraph()[0];
    ck_assert_int_eq(dp0.paragraphID(), 11172);
    fail_unless(dp0.paragraphType() == ParagraphType::Information);
    ck_assert_int_eq(dp0.paragraphText().size(), 1);
    ck_assert_str_eq(dp0.paragraphText()[0], "Capital - Moscow (MOW). The Russian Fed. is a member state of the Commonwealth of Independent States.");

    const DocumentSection ds1 = dci.sectionInformation()[1];
    ck_assert_str_eq(ds1.sectionName(), "Passport");

    const DocumentSection ds2 = dci.sectionInformation()[2];
    ck_assert_str_eq(ds2.sectionName(), "Visa");

    const DocumentSection ds3 = dci.sectionInformation()[3];
    ck_assert_str_eq(ds3.sectionName(), "Health");

    const DocumentSection ds4 = dci.sectionInformation()[4];
    ck_assert_str_eq(ds4.sectionName(), "Airport Tax");

    const DocumentSection ds5 = dci.sectionInformation()[5];
    ck_assert_str_eq(ds5.sectionName(), "Customs");

    const DocumentSection ds6 = dci.sectionInformation()[6];
    ck_assert_str_eq(ds6.sectionName(), "Currency");
    ck_assert_int_eq(ds6.documentParagraph().size(), 0);
    ck_assert_int_eq(ds6.subsectionInformation().size(), 2);

    const DocumentSubSection dss60 = ds6.subsectionInformation()[0];
    ck_assert_str_eq(dss60.subsectionName(), "Import:");
    ck_assert_int_eq(dss60.documentParagraph().size(), 1);

    const DocumentParagraphSection dp60 = dss60.documentParagraph()[0];
    ck_assert_int_eq(dp60.paragraphID(), 11249);
    fail_unless(dp60.paragraphType() == ParagraphType::Information);
    ck_assert_int_eq(dp60.paragraphText().size(), 3);
    ck_assert_str_eq(dp60.paragraphText()[0], "Import allowed.");
    ck_assert_str_eq(dp60.paragraphText()[1], "Local currency: (Russian Rouble - RUB): and foreign currencies: no restrictions.");
    ck_assert_str_eq(dp60.paragraphText()[2], "Written import declaration may be needed on export.");

    const DocumentSubSection dss61 = ds6.subsectionInformation()[1];
    ck_assert_str_eq(dss61.subsectionName(), "Export:");
    ck_assert_int_eq(dss61.documentParagraph().size(), 1);

    const DocumentParagraphSection dp61 = dss61.documentParagraph()[0];
    ck_assert_int_eq(dp61.paragraphID(), 11185);
    fail_unless(dp61.paragraphType() == ParagraphType::Information);
    ck_assert_int_eq(dp61.paragraphText().size(), 2);
    ck_assert_str_eq(dp61.paragraphText()[0], "Export allowed.");
    ck_assert_str_eq(dp61.paragraphText()[1], "Local currency (Russian Rouble - RUB) and foreign currencies: Written declaration required for amounts over 3,000.-. Import declaration / confirmation of import or transfer required for amounts over 10,000.-. Traveler's cheques are allowed up to amounts imported. Declaration required for traveler's cheques purchased in Russia.");
}
END_TEST

START_TEST(timatic_param_list_resp)
{
    using namespace Timatic;
    httpsrv::xp_testing::set_need_real_http();

    Service service(Config("1H", "SirenaXML", "Hg66SvP+wKKj2s", "staging.timatic.aero", 443));

    ParamReq req;
    req.requestID(3);
    req.countryCode("GB");
    req.section(DataSection::PassportVisa);

    const auto &msgid = ServerFramework::getQueryRunner().getEdiHelpManager().msgId();
    service.send(req, msgid);
    const auto httpDatas = service.fetch(msgid);

    std::cerr << __FUNCTION__ << ": httpDatas.size=" << httpDatas.size() << std::endl;
    for (const auto &hd : httpDatas)
        std::cerr << "\n*** header:\n" << hd.header()
                  << "\n*** content:\n" << xDumpDoc(XmlDoc::impropriate(xmlParseMemory(hd.content().data(), hd.content().size())), true)
                  << std::endl;

    if (httpDatas.back().response() && httpDatas.back().response()->type() == ResponseType::Param) {
        ParamResp resp = httpDatas.back().get<ParamResp>();
        std::cerr << "ParamResp:" << std::endl;
        std::cerr << "\trequestID=" << resp.requestID() << std::endl;
        std::cerr << "\tsubRequestID=" << (resp.subRequestID() ? *resp.subRequestID() : "") << std::endl;
        std::cerr << "\tcustomerCode=" << (resp.customerCode() ? *resp.customerCode() : "") << std::endl;
        for (const auto &result : resp.paramResults()) {
            std::cerr << "countryCode=" << result.countryCode() << " section=" << (int)result.section() << std::endl;
            for (const auto &param : result.parameterList())
                std::cerr << param.parameterName << " ... " << param.mandatory << std::endl;
        }
    }
}
END_TEST

START_TEST(timatic_param_values_resp)
{
    using namespace Timatic;
    httpsrv::xp_testing::set_need_real_http();

    Service service(Config("1H", "SirenaXML", "Hg66SvP+wKKj2s", "staging.timatic.aero", 443));

    ParamValuesReq req;
    req.requestID(3);
    req.parameterName(ParameterName::Ticket);

    const auto &msgid = ServerFramework::getQueryRunner().getEdiHelpManager().msgId();
    service.send(req, msgid);
    const auto httpDatas = service.fetch(msgid);

    std::cerr << __FUNCTION__ << ": httpDatas.size=" << httpDatas.size() << std::endl;
    for (const auto &hd : httpDatas)
        std::cerr << "\n*** header:\n" << hd.header()
                  << "\n*** content:\n" << xDumpDoc(XmlDoc::impropriate(xmlParseMemory(hd.content().data(), hd.content().size())), true)
                  << std::endl;

    if (httpDatas.back().response() && httpDatas.back().response()->type() == ResponseType::ParamValues) {
        ParamValuesResp resp = httpDatas.back().get<ParamValuesResp>();
        std::cerr << "ParamValuesResp:" << std::endl;
        std::cerr << "\trequestID=" << resp.requestID() << std::endl;
        std::cerr << "\tsubRequestID=" << (resp.subRequestID() ? *resp.subRequestID() : "") << std::endl;
        std::cerr << "\tcustomerCode=" << (resp.customerCode() ? *resp.customerCode() : "") << std::endl;
        for (const auto &value : resp.paramValues())
            std::cerr << value.name << " ... " << value.displayName << " ... " << (value.code ? *value.code : "") << std::endl;
    }
}
END_TEST

START_TEST(timatic_country_resp)
{
    using namespace Timatic;
    httpsrv::xp_testing::set_need_real_http();

    Service service(Config("1H", "SirenaXML", "Hg66SvP+wKKj2s", "staging.timatic.aero", 443));

    CountryReq req;
    req.requestID(3);
    //req.countryName("");
    req.groupCode("");

    const auto &msgid = ServerFramework::getQueryRunner().getEdiHelpManager().msgId();
    service.send(req, msgid);
    const auto httpDatas = service.fetch(msgid);

    std::cerr << __FUNCTION__ << ": httpDatas.size=" << httpDatas.size() << std::endl;
    for (const auto &hd : httpDatas)
        std::cerr << "\n*** header:\n" << hd.header()
                  << "\n*** content:\n" << xDumpDoc(XmlDoc::impropriate(xmlParseMemory(hd.content().data(), hd.content().size())), true)
                  << std::endl;

    if (httpDatas.back().response() && httpDatas.back().response()->type() == ResponseType::Country) {
        CountryResp resp = httpDatas.back().get<CountryResp>();
        std::cerr << "CountryResp:" << std::endl;
        std::cerr << "\trequestID=" << resp.requestID() << std::endl;
        std::cerr << "\tsubRequestID=" << (resp.subRequestID() ? *resp.subRequestID() : "") << std::endl;
        std::cerr << "\tcustomerCode=" << (resp.customerCode() ? *resp.customerCode() : "") << std::endl;
        for (const auto &group : resp.groups())
            std::cerr << group.groupName() << " ... " << group.groupCode() << std::endl;
        for (const auto &country : resp.countries())
            std::cerr << country.countryName() << " ... " << country.countryCode() << std::endl;

    }
}
END_TEST

START_TEST(timatic_visa_resp)
{
    using namespace Timatic;
    httpsrv::xp_testing::set_need_real_http();

    Service service(Config("1H", "SirenaXML", "Hg66SvP+wKKj2s", "staging.timatic.aero", 443));

    VisaReq req;
    req.requestID(3);
    req.countryCode("RU");

    const auto &msgid = ServerFramework::getQueryRunner().getEdiHelpManager().msgId();
    service.send(req, msgid);
    const auto httpDatas = service.fetch(msgid);

    std::cerr << __FUNCTION__ << ": httpDatas.size=" << httpDatas.size() << std::endl;
    for (const auto &hd : httpDatas)
        std::cerr << "\n*** header:\n" << hd.header()
                  << "\n*** content:\n" << xDumpDoc(XmlDoc::impropriate(xmlParseMemory(hd.content().data(), hd.content().size())), true)
                  << std::endl;

    if (httpDatas.back().response() && httpDatas.back().response()->type() == ResponseType::Visa) {
        VisaResp resp = httpDatas.back().get<VisaResp>();
        std::cerr << "VisaResp:" << std::endl;
        std::cerr << "\trequestID=" << resp.requestID() << std::endl;
        std::cerr << "\tsubRequestID=" << (resp.subRequestID() ? *resp.subRequestID() : "") << std::endl;
        std::cerr << "\tcustomerCode=" << (resp.customerCode() ? *resp.customerCode() : "") << std::endl;
        VisaResults res = resp.visaResults()[0];
        std::cerr << res.countryName() << " ... " << res.countryCode() << std::endl;
        for (const auto &vt : res.countryVisaList())
            std::cerr << vt.visaType << " ... " << vt.issuedBy << " ... " << vt.applicableFor << std::endl;
        for (const auto &vt : res.groupVisaList())
            std::cerr << vt.visaType << " ... " << vt.issuedBy << " ... " << vt.applicableFor << std::endl;
    }
}
END_TEST

#define SUITENAME "timatic_connect"
TCASEREGISTER(testInitTimatic, testShutDBConnection)
    ADD_TEST(timatic_doc_resp)
    //ADD_TEST(timatic_service_create)
    //ADD_TEST(timatic_param_list_resp)
    //ADD_TEST(timatic_param_values_resp)
    //ADD_TEST(timatic_country_resp)
    //ADD_TEST(timatic_visa_resp)
TCASEFINISH
#undef SUITENAME

#endif
