#include "timatic_service.h"
#include "timatic_exception.h"
#include "timatic_xml.h"

#include <serverlib/httpsrv.h>
#include <serverlib/query_runner.h>
#define NICKNAME "TIMATIC"
#include <serverlib/slogger.h>

#include <openssl/md5.h>

namespace Timatic {

static const std::string checkForecast =
        "HTTP/1.1 200 OK\r\n"
        "Date: Wed, 11 Dec 2019 09:54:02 GMT\r\n"
        "Content-Length: 681\r\n"
        "Content-Type: text/xml; charset=utf-8\r\n"
        "X-Powered-By: Servlet/2.5 JSP/2.1\r\n"
        "Set-Cookie: JSESSIONID=nRvndw8hhJlKhN73SNBJW1Nb0LhRThNwLl3LHbQybnvysR03m1C1!646012074; path=/; HttpOnly\r\n"
        "Via: 1.0 staging.timatic.aero\r\n"
        "Connection: close\r\n\r\n"
        "<?xml version='1.0' encoding='utf-8' standalone='yes'?><env:Envelope xmlns:soapenc='http://schemas.xmlsoap.org/soap/encoding/' xmlns:xsd='http://www.w3.org/2001/XMLSchema' xmlns:env='http://schemas.xmlsoap.org/soap/envelope/' xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'><env:Header/><env:Body><m:processLoginResponse xmlns:m='http://www.opentravel.org/OTA/2003/05/beta'><checkNameResponse xmlns='http://www.opentravel.org/OTA/2003/05/beta'><requestID>1</requestID><subRequestID>1576058038</subRequestID><randomNumber>-7817049584556491687</randomNumber><sessionID>1de7e3f8:16ee83df015:-384</sessionID></checkNameResponse></m:processLoginResponse></env:Body></env:Envelope>";

static const std::string loginForecast =
        "HTTP/1.1 200 OK\r\n"
        "Date: Wed, 11 Dec 2019 09:54:02 GMT\r\n"
        "Content-Length: 802\r\n"
        "Content-Type: text/xml; charset=utf-8\r\n"
        "X-Powered-By: Servlet/2.5 JSP/2.1\r\n"
        "Set-Cookie: JSESSIONID=ntJydw8hnTdfmkyXlSHVwZC610cbmh8p042nTzfH8Lh3JsvGhcHy!638673067; path=/; HttpOnly\r\n"
        "Via: 1.0 staging.timatic.aero\r\n"
        "Connection: close\r\n\r\n"
        "<?xml version='1.0' encoding='utf-8' standalone='yes'?><env:Envelope xmlns:soapenc='http://schemas.xmlsoap.org/soap/encoding/' xmlns:xsd='http://www.w3.org/2001/XMLSchema' xmlns:env='http://schemas.xmlsoap.org/soap/envelope/' xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'><env:Header><m:sessionID xmlns:m='http://www.opentravel.org/OTA/2003/05/beta' actor='http://schemas.xmlsoap.org/soap/actor/next' mustUnderstand='0'>1de7e3f8:16ee83df015:-384</m:sessionID></env:Header><env:Body><m:processLoginResponse xmlns:m='http://www.opentravel.org/OTA/2003/05/beta'><loginResponse xmlns='http://www.opentravel.org/OTA/2003/05/beta'><requestID>2</requestID><subRequestID>1576058042</subRequestID><successfulLogin>true</successfulLogin></loginResponse></m:processLoginResponse></env:Body></env:Envelope>";

static const std::string docForecast =
        "HTTP/1.1 200 OK\r\n"
        "Date: Wed, 11 Dec 2019 12:21:08 GMT\r\n"
        "Content-Length: 5004\r\n"
        "Content-Type: text/xml; charset=utf-8\r\n"
        "X-Powered-By: Servlet/2.5 JSP/2.1\r\n"
        "Via: 1.0 staging.timatic.aero\r\n"
        "Connection: close\r\n\r\n"
        "<?xml version='1.0' encoding='utf-8' standalone='yes'?><env:Envelope xmlns:soapenc='http://schemas.xmlsoap.org/soap/encoding/' xmlns:xsd='http://www.w3.org/2001/XMLSchema' xmlns:env='http://schemas.xmlsoap.org/soap/envelope/' xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'><env:Header><m:sessionID xmlns:m='http://www.opentravel.org/OTA/2003/05/beta' actor='http://schemas.xmlsoap.org/soap/actor/next' mustUnderstand='0'>1f00b326:16ee80d2fae:868</m:sessionID></env:Header><env:Body><m:processRequestResponse xmlns:m='http://www.opentravel.org/OTA/2003/05/beta'><documentResponse xmlns='http://www.opentravel.org/OTA/2003/05/beta'><requestID>3</requestID><subRequestID>ec81d2080f1ac94d14f9ba4d9fce</subRequestID><customerCode>25075</customerCode><documentCheckResponse><sufficientDocumentation>Conditional</sufficientDocumentation><documentCountryInformation><countryCode>GB</countryCode><countryName>United Kingdom</countryName><destinationCode>GB</destinationCode><sufficientDocumentation>Conditional</sufficientDocumentation><sectionInformation><sectionName>Passport</sectionName><documentParagraph paragraphId='17511'><paragraphType>Requirement</paragraphType><paragraphText>Passport required. </paragraphText></documentParagraph><subsectionInformation><subsectionName>Document Validity:</subsectionName><documentParagraph paragraphId='14070'><paragraphType>Requirement</paragraphType><paragraphText>Passports and other documents accepted for entry must be valid for the period of intended stay.</paragraphText></documentParagraph></subsectionInformation><subsectionInformation><subsectionName>Minors:</subsectionName><documentParagraph paragraphId='17270'><paragraphType>Information</paragraphType><paragraphText>When their names are registered in the passport of (one of) their parents or guardians, the (transit) visa - if required - must indicate that it is also valid for the child(ren) traveling on that passport. Children are not allowed to travel on a passport in which they are registered if they are not accompanied by the holder(s) of that passport. </paragraphText></documentParagraph></subsectionInformation></sectionInformation><sectionInformation><sectionName>Visa</sectionName><documentParagraph paragraphId='17537'><paragraphType>Requirement</paragraphType><paragraphText>Visa required.</paragraphText></documentParagraph><subsectionInformation><subsectionName>Additional Information:</subsectionName><documentParagraph paragraphId='68335'><paragraphType>Information</paragraphType><paragraphText>Valid visas in expired travel documents are accepted if accompanied by a new travel document.</paragraphText></documentParagraph><documentParagraph paragraphId='60013'><paragraphType>Information</paragraphType><paragraphText>Passengers with a Leave to Remain in the form of wet ink stamps issued by Guernsey, Isle of Man or Jersey can enter or transit through the United Kingdom.</paragraphText></documentParagraph><documentParagraph paragraphId='93445'><paragraphType>Information</paragraphType><paragraphText>Flights between the United Kingdom and the Channel Islands, Ireland (Rep.) and Isle of Man are treated as domestic flights, therefore are not subject to UK immigration control.</paragraphText></documentParagraph></subsectionInformation><subsectionInformation><subsectionName>Minors:</subsectionName><documentParagraph paragraphId='14075'><paragraphType>Information</paragraphType><paragraphText><strong>Child Visit Visas</strong> are issued to minors of visa required nationalities, and are endorsed 'Child Visitor'. There are two types of visa:<br/>1. '<strong>Accompanied</strong>', when the minor is traveling with one or two parents/legal guardians, or other adult. The name of that person, together with their passport number must be shown on the visa. In the case of two named adults, only the passport numbers will be entered. The minor will <strong>not</strong> be able to travel if the named person is not accompanying them; or<br/>2. '<strong>Unaccompanied</strong>', when the minor is permitted to travel unaccompanied. This type of visa also permits the minor to be accompanied by any adult. <br/>Note: this only applies to 'Child Visitor' visas and not any other category, (e.g. student). Once the holder of a 'Child Visitor' visa reaches the age of 18 years, the visa is treated as a normal 'Visit Visa'. </paragraphText></documentParagraph><documentParagraph paragraphId='102448'><paragraphType>Information</paragraphType><paragraphText>Warning:<br/>Some Child-Accompanied visit visas were issued without the details of the accompanying adult(s). In such cases, carriers must obtain travel authorization from the local Immigration Enforcement International (IEI). For more information please contact <a href='mailto:carriersliaisonsection@homeoffice.gov.uk'>carriersliaisonsection@homeoffice.gov.uk</a> . </paragraphText></documentParagraph></subsectionInformation></sectionInformation></documentCountryInformation></documentCheckResponse></documentResponse></m:processRequestResponse></env:Body></env:Envelope>";

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
#ifdef XP_TESTING
    if (expectedType == ResponseType::CheckName)
        httpsrv::xp_testing::add_forecast(checkForecast);
    else if (expectedType == ResponseType::Login)
        httpsrv::xp_testing::add_forecast(loginForecast);
#endif

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

Service::Service(const Config &config)
    : config_(config)
{
    reset();
}

bool Service::ready() const
{
#ifdef XP_TESTING
    return true;
#else
    if (time(nullptr) < session_.expires)
        return true;
    else
        return false;
#endif
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

void initTimaticTests() {}

static void testInitTimatic()
{
    initTest();
}

START_TEST(timatic_example)
{
    using namespace Timatic;

    // create service
    Service service(Config("1H", "SirenaXML", "Hg66SvP+wKKj2s", "staging.timatic.aero", 443));
    ck_assert_str_eq(service.session().sessionID, "1de7e3f8:16ee83df015:-384");
    ck_assert_str_eq(service.session().jsessionID, "ntJydw8hnTdfmkyXlSHVwZC610cbmh8p042nTzfH8Lh3JsvGhcHy!638673067");

    // create request
    DocumentReq req;
    req.requestID(3);
    req.subRequestID("ec81d2080f1ac94d14f9ba4d9fce");
    req.section(DataSection::PassportVisa);
    req.destinationCode("GB");
    req.nationalityCode("RU");
    req.documentType("Passport");
    req.documentGroup("N");

    // send request
    httpsrv::xp_testing::add_forecast(docForecast);
    const auto &msgid = ServerFramework::getQueryRunner().getEdiHelpManager().msgId();
    service.send(req, msgid);

    // fetch response
    const auto httpDatas = service.fetch(msgid);
    ck_assert_int_eq(httpDatas.size(), 1);

    // check response
    const HttpData &httpData = httpDatas[0];
    ck_assert_int_eq(httpData.httpCode(), 200);
    fail_unless(httpData.jsessionID().empty());
    fail_unless(httpData.response());
    fail_unless(httpData.response()->type() == ResponseType::Document);

    const DocumentResp resp = httpData.get<DocumentResp>();
    ck_assert_int_eq(resp.requestID(), 3);
    ck_assert_str_eq(*resp.subRequestID(), "ec81d2080f1ac94d14f9ba4d9fce");
    ck_assert_str_eq(*resp.customerCode(), "25075");
    fail_unless(resp.documentCheckResp());

    const DocumentCheckResp &dcr = *resp.documentCheckResp();
    fail_unless(dcr.sufficientDocumentation() == SufficientDocumentation::Conditional);
    ck_assert_int_eq(dcr.documentCountryInfo().size(), 1);

    const DocumentCountryInfo &dci = dcr.documentCountryInfo()[0];
    ck_assert_str_eq(dci.countryCode(), "GB");
    ck_assert_int_eq(dci.sectionInformation().size(), 2);
    ck_assert_str_eq(dci.sectionInformation()[0].sectionName(), "Passport");
    ck_assert_str_eq(dci.sectionInformation()[1].sectionName(), "Visa");
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
    DocumentResp resp(xFindNodeR(xmlDocGetRootElement(doc.get()), DocumentResp::cname()));
    ck_assert_int_eq(resp.requestID(), 3);
    ck_assert_str_eq(*resp.customerCode(), "25075");
    fail_unless(resp.documentCheckResp());

    const DocumentCheckResp &dcr = *resp.documentCheckResp();
    fail_unless(dcr.sufficientDocumentation() == SufficientDocumentation::Conditional);
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

    const std::string content =
            R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
            <env:Envelope xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:env="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
              <env:Header>
                <m:sessionID xmlns:m="http://www.opentravel.org/OTA/2003/05/beta" actor="http://schemas.xmlsoap.org/soap/actor/next" mustUnderstand="0">1f00b326:16ee80d2fae:3e7</m:sessionID>
              </env:Header>
              <env:Body>
                <m:processRequestResponse xmlns:m="http://www.opentravel.org/OTA/2003/05/beta">
                  <paramResponse xmlns="http://www.opentravel.org/OTA/2003/05/beta">
                    <requestID>3</requestID>
                    <customerCode>25075</customerCode>
                    <paramResults>
                      <countryCode>GB</countryCode>
                      <section>Passport,Visa</section>
                      <parameterList>
                        <parameter>
                          <parameterName>destinationCode</parameterName>
                          <mandatory>true</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>nationalityCode</parameterName>
                          <mandatory>true</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>documentType</parameterName>
                          <mandatory>true</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>issueCountryCode</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>issueDate</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>expiryDate</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>residentCountryCode</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>arrivalDate</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>departAirportCode</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>carrierCode</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>countriesVisited</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>stayDuration</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>stayType</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>birthCountryCode</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>birthDate</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>passportSeries</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>transitCountry</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>documentGroup</parameterName>
                          <mandatory>true</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>visa</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>residencyDocument</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>documentFeature</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>ticket</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>typeForVisa</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>issuedBy</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>calendarDay</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                        <parameter>
                          <parameterName>secondaryDocumentType</parameterName>
                          <mandatory>false</mandatory>
                        </parameter>
                      </parameterList>
                    </paramResults>
                  </paramResponse>
                </m:processRequestResponse>
              </env:Body>
            </env:Envelope>)";

    XmlDoc doc = XmlDoc::impropriate(xmlParseMemory(content.data(), content.size()));
    ParamResp resp(xFindNodeR(xmlDocGetRootElement(doc.get()), ParamResp::cname()));
    ck_assert_int_eq(resp.requestID(), 3);
    ck_assert_str_eq(*resp.customerCode(), "25075");
    ck_assert_int_eq(resp.paramResults().size(), 1);

    const ParamResults &pres = resp.paramResults()[0];
    ck_assert_str_eq(pres.countryCode(), "GB");
    fail_unless(pres.section() == DataSection::PassportVisa);

    const std::vector<ParameterType> &plist = pres.parameterList();
    ck_assert_int_eq(plist.size(), 26);

    ck_assert_str_eq(plist[0].parameterName, "destinationCode");
    fail_unless(plist[0].mandatory == true);

    ck_assert_str_eq(plist[17].parameterName, "documentGroup");
    fail_unless(plist[17].mandatory == true);

    ck_assert_str_eq(plist[25].parameterName, "secondaryDocumentType");
    fail_unless(plist[25].mandatory == false);
}
END_TEST

START_TEST(timatic_param_values_resp)
{
    using namespace Timatic;

    const std::string content =
            R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
            <env:Envelope xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:env="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
              <env:Header>
                <m:sessionID xmlns:m="http://www.opentravel.org/OTA/2003/05/beta" actor="http://schemas.xmlsoap.org/soap/actor/next" mustUnderstand="0">1f00b326:16ee80d2fae:fc7</m:sessionID>
              </env:Header>
              <env:Body>
                <m:processRequestResponse xmlns:m="http://www.opentravel.org/OTA/2003/05/beta">
                  <paramValuesResponse xmlns="http://www.opentravel.org/OTA/2003/05/beta">
                    <requestID>3</requestID>
                    <customerCode>25075</customerCode>
                    <values>
                      <value>
                        <name>NoTicket</name>
                        <displayName>No Ticket</displayName>
                      </value>
                      <value>
                        <name>Ticket</name>
                        <displayName>Return/Onward Ticket</displayName>
                      </value>
                    </values>
                  </paramValuesResponse>
                </m:processRequestResponse>
              </env:Body>
            </env:Envelope>)";

    XmlDoc doc = XmlDoc::impropriate(xmlParseMemory(content.data(), content.size()));
    ParamValuesResp resp(xFindNodeR(xmlDocGetRootElement(doc.get()), ParamValuesResp::cname()));

    ck_assert_int_eq(resp.requestID(), 3);
    ck_assert_str_eq(*resp.customerCode(), "25075");
    ck_assert_int_eq(resp.paramValues().size(), 2);

    const ParamValue &pv1 = resp.paramValues()[0];
    fail_unless(!pv1.code);
    ck_assert_str_eq(pv1.name, "NoTicket");
    ck_assert_str_eq(pv1.displayName, "No Ticket");

    const ParamValue &pv2 = resp.paramValues()[1];
    fail_unless(!pv2.code);
    ck_assert_str_eq(pv2.name, "Ticket");
    ck_assert_str_eq(pv2.displayName, "Return/Onward Ticket");
}
END_TEST

START_TEST(timatic_country_resp)
{
    using namespace Timatic;

    const std::string content =
            R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
            <env:Envelope xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:env="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
              <env:Header>
                <m:sessionID xmlns:m="http://www.opentravel.org/OTA/2003/05/beta" actor="http://schemas.xmlsoap.org/soap/actor/next" mustUnderstand="0">1de7e3f8:16ee83df015:108b</m:sessionID>
              </env:Header>
              <env:Body>
                <m:processRequestResponse xmlns:m="http://www.opentravel.org/OTA/2003/05/beta">
                  <countryResponse xmlns="http://www.opentravel.org/OTA/2003/05/beta">
                    <requestID>3</requestID>
                    <customerCode>25075</customerCode>
                    <groups>
                      <group>
                        <groupCode>YFAF</groupCode>
                        <groupName>African countries with risk of yellow fever transmission</groupName>
                        <groupDescription>African countries with risk of Yellow Fever transmission</groupDescription>
                      </group>
                      <group>
                        <groupCode>ALLC</groupCode>
                        <groupName>All Countries</groupName>
                        <groupDescription>List of all countries (Excluding PS)</groupDescription>
                      </group>
                      <group>
                        <groupCode>ARAB</groupCode>
                        <groupName>Arab League</groupName>
                        <groupDescription>Arab League</groupDescription>
                      </group>
                      <group>
                        <groupCode>APEC</groupCode>
                        <groupName>Asia Pacific Economic Cooperation</groupName>
                        <groupDescription>Asia Pacific Economic Cooperation</groupDescription>
                      </group>
                      <group>
                        <groupCode>ASEA</groupCode>
                        <groupName>Association of Southeast Asian Nations</groupName>
                        <groupDescription>Association of Southeast Asian Nations</groupDescription>
                      </group>
                      <group>
                        <groupCode>AUST</groupCode>
                        <groupName>Australia Territories Domestic</groupName>
                        <groupDescription>Australia Territories Domestic flights</groupDescription>
                      </group>
                      <group>
                        <groupCode>GBVA</groupCode>
                        <groupName>British Passports Validity for Selected Countries</groupName>
                        <groupDescription>ONLY for British Passports validity paragraph 54941 if you need to override parent</groupDescription>
                      </group>
                      <group>
                        <groupCode>CARI</groupCode>
                        <groupName>Caribbean Community</groupName>
                        <groupDescription>Caribbean Community</groupDescription>
                      </group>
                      <group>
                        <groupCode>CIS</groupCode>
                        <groupName>Commonwealth of Independent States</groupName>
                        <groupDescription>Commonwealth of Independent States</groupDescription>
                      </group>
                      <group>
                        <groupCode>COMM</groupCode>
                        <groupName>Commonwealth of Nations</groupName>
                        <groupDescription>Independent Commonwealth Countries</groupDescription>
                      </group>
                      <group>
                        <groupCode>CETP</groupCode>
                        <groupName>Countries Emergency and Temporary passports</groupName>
                        <groupDescription>Countries accepting all Emergency and Temporary passports</groupDescription>
                      </group>
                      <group>
                        <groupCode>ENID</groupCode>
                        <groupName>EEA Member States Issuing National ID Card to Own Nationals</groupName>
                        <groupDescription>EEA National Member States Issuing Nat ID Card to Own Nationals</groupDescription>
                      </group>
                      <group>
                        <groupCode>ETEP</groupCode>
                        <groupName>EEA member states issuing emergency / temporary passports</groupName>
                        <groupDescription>EEA member states issuing emergency / temporary passports</groupDescription>
                      </group>
                      <group>
                        <groupCode>EUVW</groupCode>
                        <groupName>EU visa waiver group (excluding IE, UK)</groupName>
                        <groupDescription>Countries with common visa policy for 3rd country nationals</groupDescription>
                      </group>
                      <group>
                        <groupCode>ECOW</groupCode>
                        <groupName>Economic Community of West African States</groupName>
                        <groupDescription>Economic Community of West African States</groupDescription>
                      </group>
                      <group>
                        <groupCode>EEA</groupCode>
                        <groupName>European Economic Area</groupName>
                        <groupDescription>European Economic Area</groupDescription>
                      </group>
                      <group>
                        <groupCode>EFTA</groupCode>
                        <groupName>European Free Trade Association</groupName>
                        <groupDescription>European Free Trade Association</groupDescription>
                      </group>
                      <group>
                        <groupCode>EEU</groupCode>
                        <groupName>European Union Member States</groupName>
                        <groupDescription>European Union Member States</groupDescription>
                      </group>
                      <group>
                        <groupCode>GCC</groupCode>
                        <groupName>Gulf Cooperation Council</groupName>
                        <groupDescription>Gulf Cooperation Council</groupDescription>
                      </group>
                      <group>
                        <groupCode>ILO1</groupCode>
                        <groupName>International Labour Organisation ILO 108</groupName>
                        <groupDescription>International Labour Organisation ILO 108</groupDescription>
                      </group>
                      <group>
                        <groupCode>ILO3</groupCode>
                        <groupName>International Labour Organisation ILO 185</groupName>
                        <groupDescription>International Labour Organisation ILO 185</groupDescription>
                      </group>
                      <group>
                        <groupCode>ILO2</groupCode>
                        <groupName>International Labour Organisation ILO 22 </groupName>
                        <groupDescription>International Labour Organisation ILO 22 </groupDescription>
                      </group>
                      <group>
                        <groupCode>IQSP</groupCode>
                        <groupName>Iraq S series passport invalidated</groupName>
                        <groupDescription>Iraq S series passport invalidated </groupDescription>
                      </group>
                      <group>
                        <groupCode>LEAP</groupCode>
                        <groupName>Latvian Alien's passports</groupName>
                        <groupDescription>Countries where information for Latvian Alien's passports is published</groupDescription>
                      </group>
                      <group>
                        <groupCode>MSUR</groupCode>
                        <groupName>MERCOSUR Member and Associated States</groupName>
                        <groupDescription>MERCOSUR Member and Associated States</groupDescription>
                      </group>
                      <group>
                        <groupCode>MSWN</groupCode>
                        <groupName>MS - warning</groupName>
                        <groupDescription>Letter of Guarantee/Employment</groupDescription>
                      </group>
                      <group>
                        <groupCode>MALA</groupCode>
                        <groupName>Malarious Areas </groupName>
                        <groupDescription>Malarious Areas </groupDescription>
                      </group>
                      <group>
                        <groupCode>NATO</groupCode>
                        <groupName>North Atlantic Treaty Organization</groupName>
                        <groupDescription>North Atlantic Treaty Organization</groupDescription>
                      </group>
                      <group>
                        <groupCode>OECD</groupCode>
                        <groupName>Organisation for Economic Cooperation and Development </groupName>
                        <groupDescription>Organisation for Economic Cooperation and Development </groupDescription>
                      </group>
                      <group>
                        <groupCode>OAS</groupCode>
                        <groupName>Organisation of American States</groupName>
                        <groupDescription>Organisation of American States</groupDescription>
                      </group>
                      <group>
                        <groupCode>PLAG</groupCode>
                        <groupName>Plague Infected Areas</groupName>
                        <groupDescription>Plague Infected Areas</groupDescription>
                      </group>
                      <group>
                        <groupCode>SEID</groupCode>
                        <groupName>SE/FI Nat ID (Editing Purpose only)</groupName>
                        <groupDescription>Non-Schengen/EU countries that accept SE/FI Nat ID</groupDescription>
                      </group>
                      <group>
                        <groupCode>SACD</groupCode>
                        <groupName>Schengen + BG, CY, HR, RO</groupName>
                        <groupDescription>Schengen + BG, CY, HR, RO</groupDescription>
                      </group>
                      <group>
                        <groupCode>SCHE</groupCode>
                        <groupName>Schengen Acquis Visa Exempt Nationals</groupName>
                        <groupDescription>List of nationals that are listed as visa exempt according to Schengen Acquis</groupDescription>
                      </group>
                      <group>
                        <groupCode>SCHN</groupCode>
                        <groupName>Schengen Member States</groupName>
                        <groupDescription>Schengen States</groupDescription>
                      </group>
                      <group>
                        <groupCode>SCCH</groupCode>
                        <groupName>Schengen Member States without CH</groupName>
                        <groupDescription>Schengen countries without CH</groupDescription>
                      </group>
                      <group>
                        <groupCode>BRIT</groupCode>
                        <groupName>Territories Dependent on the United Kingdom</groupName>
                        <groupDescription>Territories Dependent on the United Kingdom</groupDescription>
                      </group>
                      <group>
                        <groupCode>NZEA</groupCode>
                        <groupName>Territories associated with New Zealand</groupName>
                        <groupDescription>Territories associated with New Zealand</groupDescription>
                      </group>
                      <group>
                        <groupCode>AUST</groupCode>
                        <groupName>Territories dependent on Australia</groupName>
                        <groupDescription>Territories dependent on Australia</groupDescription>
                      </group>
                      <group>
                        <groupCode>FRAN</groupCode>
                        <groupName>Territories dependent on France</groupName>
                        <groupDescription>Territories dependent on France</groupDescription>
                      </group>
                      <group>
                        <groupCode>NETH</groupCode>
                        <groupName>Territories under administration of the Netherlands</groupName>
                        <groupDescription>Territories under administration of the Netherlands</groupDescription>
                      </group>
                      <group>
                        <groupCode>USAD</groupCode>
                        <groupName>Territories under administration of the U.S.A.</groupName>
                        <groupDescription>Territories under administration of the U.S.A.</groupDescription>
                      </group>
                      <group>
                        <groupCode>TMPP</groupCode>
                        <groupName>Turkmenistan: old-style passports</groupName>
                        <groupDescription>All countries except for Turkmenistan</groupDescription>
                      </group>
                      <group>
                        <groupCode>VWP</groupCode>
                        <groupName>US Visa Waiver Program</groupName>
                        <groupDescription>US Visa Waiver Program</groupDescription>
                      </group>
                      <group>
                        <groupCode>USAT</groupCode>
                        <groupName>USA Territories Domestic</groupName>
                        <groupDescription>USA Territories Domestic flights</groupDescription>
                      </group>
                      <group>
                        <groupCode>WHTI</groupCode>
                        <groupName>Western Hemisphere Travel Initiative</groupName>
                        <groupDescription>Western Hemisphere Travel Initiative</groupDescription>
                      </group>
                      <group>
                        <groupCode>YFVA</groupCode>
                        <groupName>YF 10 days after vaccination</groupName>
                        <groupDescription>10 days after vaccination</groupDescription>
                      </group>
                      <group>
                        <groupCode>YFIN</groupCode>
                        <groupName>Yellow Fever Infected Areas (countries with risk of yellow fever transmission)</groupName>
                        <groupDescription>Yellow Fever Infected Areas</groupDescription>
                      </group>
                    </groups>
                  </countryResponse>
                </m:processRequestResponse>
              </env:Body>
            </env:Envelope>)";

    XmlDoc doc = XmlDoc::impropriate(xmlParseMemory(content.data(), content.size()));
    CountryResp resp(xFindNodeR(xmlDocGetRootElement(doc.get()), CountryResp::cname()));
    ck_assert_int_eq(resp.requestID(), 3);
    ck_assert_str_eq(*resp.customerCode(), "25075");
    ck_assert_int_eq(resp.groups().size(), 48);
    ck_assert_int_eq(resp.countries().size(), 0);

    const GroupDetail &gd01 = resp.groups()[0];
    ck_assert_str_eq(gd01.groupCode(), "YFAF");
    ck_assert_str_eq(gd01.groupName(), "African countries with risk of yellow fever transmission");
    ck_assert_str_eq(gd01.groupDescription(), "African countries with risk of Yellow Fever transmission");

    const GroupDetail &gd48 = resp.groups()[47];
    ck_assert_str_eq(gd48.groupCode(), "YFIN");
    ck_assert_str_eq(gd48.groupName(), "Yellow Fever Infected Areas (countries with risk of yellow fever transmission)");
    ck_assert_str_eq(gd48.groupDescription(), "Yellow Fever Infected Areas");
}
END_TEST

START_TEST(timatic_visa_resp)
{
    using namespace Timatic;

    const std::string content =
            R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
            <env:Envelope xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:env="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
              <env:Header>
                <m:sessionID xmlns:m="http://www.opentravel.org/OTA/2003/05/beta" actor="http://schemas.xmlsoap.org/soap/actor/next" mustUnderstand="0">3e6b3883:16ee7facf74:104a</m:sessionID>
              </env:Header>
              <env:Body>
                <m:processRequestResponse xmlns:m="http://www.opentravel.org/OTA/2003/05/beta">
                  <visaResponse xmlns="http://www.opentravel.org/OTA/2003/05/beta">
                    <requestID>3</requestID>
                    <customerCode>25075</customerCode>
                    <visaResults>
                      <countryName>Russian Fed.</countryName>
                      <countryCode>RU</countryCode>
                      <countryVisaList>
                        <visaType>
                          <visaType>VVV</visaType>
                          <issuedBy>RU</issuedBy>
                          <applicableFor>Destination and Transit</applicableFor>
                        </visaType>
                        <visaType>
                          <visaType>VVI</visaType>
                          <issuedBy>RU</issuedBy>
                          <applicableFor>Destination</applicableFor>
                        </visaType>
                      </countryVisaList>
                    </visaResults>
                  </visaResponse>
                </m:processRequestResponse>
              </env:Body>
            </env:Envelope>)";

    XmlDoc doc = XmlDoc::impropriate(xmlParseMemory(content.data(), content.size()));
    VisaResp resp(xFindNodeR(xmlDocGetRootElement(doc.get()), VisaResp::cname()));
    ck_assert_int_eq(resp.requestID(), 3);
    ck_assert_str_eq(*resp.customerCode(), "25075");
    fail_unless(resp.visaResults());

    const VisaResults vres = *resp.visaResults();
    ck_assert_str_eq(vres.countryName(), "Russian Fed.");
    ck_assert_str_eq(vres.countryCode(), "RU");
    ck_assert_int_eq(vres.countryVisaList().size(), 2);

    const VisaType vt01 = vres.countryVisaList()[0];
    ck_assert_str_eq(vt01.visaType, "VVV");
    ck_assert_str_eq(vt01.issuedBy, "RU");
    ck_assert_str_eq(vt01.applicableFor, "Destination and Transit");
    fail_unless(!vt01.expiryDateGMT);

    const VisaType vt02 = vres.countryVisaList()[1];
    ck_assert_str_eq(vt02.visaType, "VVI");
    ck_assert_str_eq(vt02.issuedBy, "RU");
    ck_assert_str_eq(vt02.applicableFor, "Destination");
    fail_unless(!vt02.expiryDateGMT);
}
END_TEST

#define SUITENAME "timatic_connect"
TCASEREGISTER(testInitTimatic, testShutDBConnection)
    ADD_TEST(timatic_example)
    ADD_TEST(timatic_doc_resp)
    ADD_TEST(timatic_param_list_resp)
    ADD_TEST(timatic_param_values_resp)
    ADD_TEST(timatic_country_resp)
    ADD_TEST(timatic_visa_resp)
TCASEFINISH
#undef SUITENAME

#endif
