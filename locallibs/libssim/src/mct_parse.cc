#include <serverlib/str_utils.h>
#include <serverlib/expected.h>
#include <serverlib/dates.h>
#include <serverlib/lngv_user.h>
#include <serverlib/query_runner.h>

#include "mct_parse.h"

#define NICKNAME "DAG"
#include <serverlib/slogger.h>

namespace ssim { namespace mct {

bool isEquivalent(const Record&, const Record&);

namespace {

enum RecordType
{
    Header = 1,
    MctData = 2,
    Trailer = 3
};

struct Context
{
    ContentIndicator contentType;
    unsigned int serialNumber;
};

using field_desc = std::pair<unsigned int, unsigned int>;

static std::string toString(ParseError::Type t)
{
    switch (t) {
    case ParseError::Syntax:    return "Syntax";
    case ParseError::Nsi:       return "Nsi";
    case ParseError::Logic:     return "Logic";
    }
    ASSERT(0 && "unknown error type");
    return std::string();
}

static std::string getField(const std::string& in, const field_desc& fd, bool trim)
{
    const std::string s = in.substr(fd.first - 1, fd.second - fd.first + 1);
    return trim ? StrUtils::rtrim(s) : s;
}

static Message makeErrorMsg(ParseError::Type type, const std::pair<field_desc, std::string>& dts)
{
    const std::string val = getField(dts.second, dts.first, false);

    return Message(STDLOG, _("%1% error on field [%2%, %3%]: '%4%'"))
        .bind(toString(type)).bind(dts.first.first).bind(dts.first.second).bind(val);
}

static Message makeErrorMsg(ParseError::Type type, const std::string& details)
{
    return Message(STDLOG, _("%1% error: %2%")).bind(toString(type)).bind(details);
}

static Message addLineNumber(const Message& msg, size_t lineNo)
{
    return Message(STDLOG, _("#%1% %2%")).bind(lineNo).bind(msg);
}

template <typename T>
static ParseError makeParseError(ParseError::Type type, const T& details)
{
    return ParseError { type, makeErrorMsg(type, details) };
}

static Expected<unsigned int, ParseError> parseNumericField(const std::string& in, const field_desc& fd)
{
    unsigned int out = 0;
    for (size_t i = fd.first - 1; i < fd.second; ++i) {
        const char c = in[i];
        if (c >= '0' && c <= '9') {
            out = (out * 10) + (c - '0');
        } else {
            return makeParseError(ParseError::Syntax, std::make_pair(fd, in));
        }
    }
    return out;
}

static char getField(const std::string& in, size_t pos)
{
    return in[pos - 1];
}


template <typename T>
static Expected<boost::optional<typename T::id_type>, ParseError> parseOptionalNsiValue(
        const std::string& in, const field_desc& fd
    )
{
    const auto code = getField(in, fd, true);
    if (code.empty()) {
        return boost::optional<typename T::id_type>();
    }

    const EncString s = EncString::from866(code);
    if (!T::find(s)) {
        return makeParseError(ParseError::Nsi, std::make_pair(fd, in));
    }
    return boost::make_optional(T(s).id());
}

static Expected< boost::optional<nsi::RegionId>, ParseError > parseOptionalRegion(
        const std::string& in, const field_desc& cfd, const field_desc& sfd
    )
{
    const auto country = getField(in, cfd, true);
    const auto state = getField(in, sfd, true);

    if (country.empty() || state.empty()) {
        return boost::optional<nsi::RegionId>();
    }

    const EncString s = EncString::from866(country + state);
    if (!nsi::Region::find(s)) {
        return makeParseError(ParseError::Nsi, std::make_pair(sfd, in));
    }
    return boost::make_optional(nsi::Region(s).id());
}

static Expected<boost::gregorian::date, ParseError> parseDate(
        const std::string& in, const field_desc& fd,
        const boost::gregorian::date& ifEmpty
    )
{
    const auto s = getField(in, fd, true);
    if (s.empty()) {
        return ifEmpty;
    }

    const auto dt = Dates::DateFromDDMONYY(s, Dates::YY2YYYY_Wraparound50YearsFutureDate, Dates::currentDate());
    if (dt.is_not_a_date()) {
        return makeParseError(ParseError::Syntax, std::make_pair(fd, in));
    }
    return dt;
}

static Expected<boost::posix_time::time_duration, ParseError> parseConxTime(
        const std::string& in, const field_desc& fd
    )
{
    boost::posix_time::time_duration out(boost::posix_time::pos_infin);

    const std::string s = getField(in, fd, true);
    if (!s.empty()) {
        out = Dates::hh24mi(s);
        if (out.is_not_a_date_time()) {
            return makeParseError(ParseError::Syntax, std::make_pair(fd, in));
        }
    }
    return out;
}

static Expected<Status, ParseError> getStatus(const std::string& in, const field_desc& fd)
{
    Status v;
    if (enumFromStr(v, getField(in, fd, false))) {
        return v;
    }
    return makeParseError(ParseError::Syntax, std::make_pair(fd, in));
}

static Expected<AircraftBody, ParseError> getAircraftBody(const std::string& in, const field_desc& fd)
{
    const std::string s = getField(in, fd, true);
    if (s.empty()) {
        return AircraftBody::Any;
    }

    const auto v = enumFromStr2(s, AircraftBody::Any);
    if (v != AircraftBody::Any) {
        return v;
    }

    return makeParseError(ParseError::Syntax, std::make_pair(fd, in));
}

static Expected<bool, ParseError> getSuppressionIndicator(const std::string& in, const field_desc& fd)
{
    const std::string s = getField(in, fd, true);
    if (s.empty() || s == "N") {
        return false;
    }
    if (s == "Y") {
        return true;
    }
    return makeParseError(ParseError::Syntax, std::make_pair(fd, in));
}

static Expected<boost::optional<ActionIndicator>, ParseError> getActionIndicator(
        const std::string& in, const field_desc& fd, ContentIndicator cnt
    )
{
    const std::string s = getField(in, fd, true);
    if (s.empty()) {
        if (cnt == ContentIndicator::Updates) {
            return makeParseError(ParseError::Syntax, std::make_pair(fd, in));
        }
        return boost::optional<ActionIndicator>();
    }
    if (s == "A") {
        return boost::make_optional(ActionIndicator::Add);
    }
    if (s == "D") {
        return boost::make_optional(ActionIndicator::Delete);
    }
    return makeParseError(ParseError::Syntax, std::make_pair(fd, in));
}
//#############################################################################
static Expected<SegDesc, ParseError> parseSegment(const std::string& rec, bool arrival)
{
    enum SegmentRelatedField
    {
        ConxStation,
        Terminal,
        Status,
        Carrier,
        CshOprCarrier,
        CshIndicator,
        FltRangeStart,
        FltRangeEnd,
        AircraftType,
        AircraftBodyType,
        OppGeozone,
        OppCountry,
        OppState,
        OppStation
    };

    using SegFieldLayout = std::map<SegmentRelatedField, field_desc>;

    static const SegFieldLayout arrSfl = {
        { ConxStation,      {  2,  4 } }, { Terminal,         { 32, 33 } },
        { Status,           {  9,  9 } }, { Carrier,          { 14, 15 } },
        { CshOprCarrier,    { 17, 18 } }, { CshIndicator,     { 16, 16 } },
        { FltRangeStart,    { 46, 49 } }, { FltRangeEnd,      { 50, 53 } },
        { AircraftType,     { 24, 26 } }, { AircraftBodyType, { 27, 27 } },
        { OppGeozone,       { 66, 68 } }, { OppCountry,       { 36, 37 } },
        { OppState,         { 62, 63 } }, { OppStation,       { 38, 40 } }
    };

    static const SegFieldLayout depSfl = {
        { ConxStation,      { 11, 13 } }, { Terminal,         { 34, 35 } },
        { Status,           { 10, 10 } }, { Carrier,          { 19, 20 } },
        { CshOprCarrier,    { 22, 23 } }, { CshIndicator,     { 21, 21 } },
        { FltRangeStart,    { 54, 57 } }, { FltRangeEnd,      { 58, 61 } },
        { AircraftType,     { 28, 30 } }, { AircraftBodyType, { 31, 31 } },
        { OppGeozone,       { 69, 71 } }, { OppCountry,       { 41, 42 } },
        { OppState,         { 64, 65 } }, { OppStation,       { 43, 45 } }
    };

    const SegFieldLayout& sfl = (arrival ? arrSfl : depSfl);


    SegDesc out = { };

    //-------------------------------------------------------------------------
    {
        CALL_EXP_RET(v, getStatus(rec, sfl.at(Status)));
        out.status = *v;
    }
    //-------------------------------------------------------------------------
    {
        CALL_EXP_RET(v, parseOptionalNsiValue<nsi::Point>(rec, sfl.at(ConxStation)));
        out.conxStation = *v;
    }
    //-------------------------------------------------------------------------
    out.conxTerminal = nsi::TermId::create(getField(rec, sfl.at(Terminal), true));
    //-------------------------------------------------------------------------
    {

        CALL_EXP_RET(v, parseOptionalNsiValue<nsi::Company>(rec, sfl.at(Carrier)));
        out.carrier = *v;
    }
    //-------------------------------------------------------------------------
    {
        const char c = getField(rec, sfl.at(CshIndicator).first);
        out.cshIndicator = (c == 'Y');
        if (!out.cshIndicator && c != ' ') {
            return makeParseError(ParseError::Syntax, std::make_pair(sfl.at(CshIndicator), rec));
        }
    }
    //-------------------------------------------------------------------------
    {
        CALL_EXP_RET(v, parseOptionalNsiValue<nsi::Company>(rec, sfl.at(CshOprCarrier)));
        out.cshOprCarrier = *v;
    }
    //-------------------------------------------------------------------------
    {
        const std::string rngStart = getField(rec, sfl.at(FltRangeStart), true);
        const std::string rngEnd = getField(rec, sfl.at(FltRangeEnd), true);

        if (rngStart.empty() != rngEnd.empty()) {
            return makeParseError(ParseError::Syntax, std::make_pair(sfl.at(FltRangeStart), rec));
        }

        if (!rngStart.empty()) {
            CALL_EXP_RET(x, parseNumericField(rec, sfl.at(FltRangeStart)));
            CALL_EXP_RET(y, parseNumericField(rec, sfl.at(FltRangeEnd)));
            out.fltRange = FlightRange { *x, *y };
        }
    }
    //-------------------------------------------------------------------------
    {
        CALL_EXP_RET(v, parseOptionalNsiValue<nsi::AircraftType>(rec, sfl.at(AircraftType)));
        out.aircraftType = *v;
    }
    //-------------------------------------------------------------------------
    {
        CALL_EXP_RET(v, getAircraftBody(rec, sfl.at(AircraftBodyType)));
        out.aircraftBodyType = *v;
    }
    //-------------------------------------------------------------------------
    {
        CALL_EXP_RET(v, parseOptionalNsiValue<nsi::Geozone>(rec, sfl.at(OppGeozone)));
        out.oppGeozone = *v;
    }
    //-------------------------------------------------------------------------
    {
        CALL_EXP_RET(v, parseOptionalRegion(rec, sfl.at(OppCountry), sfl.at(OppState)));
        out.oppRegion = *v;
    }
    if (!out.oppRegion) {
        CALL_EXP_RET(v, parseOptionalNsiValue<nsi::Country>(rec, sfl.at(OppCountry)));
        out.oppCountry = *v;
    }
    //-------------------------------------------------------------------------
    {
        CALL_EXP_RET(v, parseOptionalNsiValue<nsi::Point>(rec, sfl.at(OppStation)));
        out.oppStation = *v;
    }
    return out;
}

static Expected<Context, ParseError> parseHeader(const std::string& rec)
{
    const field_desc fdTitle = { 2, 31 };
    const field_desc fdCnt = { 78, 78 };

    if (getField(rec, fdTitle, true) != "MINIMUM CONNECT TIME DATA SET") {
        return makeParseError(ParseError::Syntax, std::make_pair(fdTitle, rec));
    }

    const std::string cntType = getField(rec, fdCnt, true);
    if (cntType == "F") {
        return Context { ContentIndicator::FullReplacement, 1 };
    }
    if (cntType == "U") {
        return Context { ContentIndicator::Updates, 1 };
    }
    return makeParseError(ParseError::Syntax, std::make_pair(fdCnt, rec));
}

using RecordAndAction = std::pair<Record, boost::optional<ActionIndicator>>;

static Expected<RecordAndAction, ParseError> parseMctDataRecord(const std::string& rec, ContentIndicator cnt)
{
    CALL_EXP_RET(arrSg, parseSegment(rec, true));
    CALL_EXP_RET(depSg, parseSegment(rec, false));

    CALL_EXP_RET(begDt, parseDate(rec, { 72, 78 }, boost::gregorian::date(boost::gregorian::neg_infin)));
    CALL_EXP_RET(endDt, parseDate(rec, { 79, 85 }, boost::gregorian::date(boost::gregorian::pos_infin)));

    CALL_EXP_RET(suppressionIndicator, getSuppressionIndicator(rec, { 87, 87 }));
    //-------------------------------------------------------------------------

    Record out = {
        *arrSg,
        *depSg,
        *begDt,
        *endDt,
        *suppressionIndicator
    };

    //-------------------------------------------------------------------------
    {
        CALL_EXP_RET(v, parseOptionalNsiValue<nsi::Geozone>(rec, { 88, 90 }));
        out.suppressionGeozone = *v;
    }
    //-------------------------------------------------------------------------
    {
        CALL_EXP_RET(v, parseOptionalRegion(rec, { 91, 92 }, { 93, 94 }));
        out.suppressionRegion = *v;
    }
    if (!out.suppressionRegion) {
        CALL_EXP_RET(v, parseOptionalNsiValue<nsi::Country>(rec, { 91, 92 }));
        out.suppressionCountry = *v;
    }
    //-------------------------------------------------------------------------
    {
        CALL_EXP_RET(v, parseConxTime(rec, { 5, 8 }));
        out.time = *v;
    }
    //-------------------------------------------------------------------------
    CALL_EXP_RET(act, getActionIndicator(rec, { 104, 104 }, cnt));

    if (const Message m = validate(out)) {
        return makeParseError(ParseError::Logic, m.toString(UserLanguage::en_US()));
    }

    return RecordAndAction { out, *act };
}

static boost::optional<ParseError> parseRecord(
        const std::string& rec,
        std::unique_ptr<Context>& ctx,
        ImportHandler& mng
    )
{
    constexpr size_t RecordSize = 200;

    if (rec.size() < RecordSize) {
        return makeParseError(ParseError::Syntax, "record length");
    }

    const int rt = rec[0] - '0';
    if (rt != Header && rt != MctData && rt != Trailer) {
        return makeParseError(ParseError::Syntax, "record type");
    }

    if ((rt == Header && ctx) || (rt != Header && !ctx)) {
        return makeParseError(ParseError::Syntax, "record sequence");
    }

    CALL_EXP_RET(serNo, parseNumericField(rec, { 195, 200 }));
    if ((ctx && ctx->serialNumber != *serNo - (rt == Trailer ? 0 : 1)) || (!ctx && *serNo != 1)) {
        return makeParseError(ParseError::Syntax, "record sequence");
    }

    if (rt == Header) {
        CALL_EXP_RET(v, parseHeader(rec));
        ctx = std::make_unique<Context>(*v);
        mng.setProcessPolicy(ctx->contentType);
    } else if (rt == MctData) {
        ctx->serialNumber = *serNo;
        CALL_EXP_RET(v, parseMctDataRecord(rec, ctx->contentType));
        mng.stash(v->first, v->second);
    }
    return boost::none;
}


} //anonymous

Message parseMctDataSet(std::istream& in, ImportHandler& mng)
{
    size_t ln = 0;
    std::string line;

    std::unique_ptr<Context> ctx;

    while (!std::getline(in, line).eof()) {
        ++ln;
        if (auto pe = parseRecord(line, ctx, mng)) {
            const ParseError err = { pe->type, addLineNumber(pe->description, ln) };
            if (!mng.handleError(err) || err.type == ParseError::Syntax) {
                return err.description;
            }
        }
    }
    return Message();
}
//#############################################################################


ImportHandler::~ImportHandler() {}

void ImportHandler::setProcessPolicy(ContentIndicator ci)
{
    policy_ = ci;
}

void ImportHandler::stash(const Record& rec, const boost::optional<ActionIndicator>& act)
{
    data_[rec.arrival.conxStation.get_value_or(nsi::PointId(0))].emplace_back(rec, act);
}

void ImportHandler::applyChanges()
{
    ASSERT(policy_ && "undefined import policy");

    const bool replacement = (policy_ == ssim::mct::ContentIndicator::FullReplacement);

    std::set<nsi::PointId> imps;

    for (const auto& v : data_) {
        preProcess(v.first);

        imps.emplace(v.first);

        std::vector<Record> to_remove, to_insert;
        for (const auto& r : v.second) {
            std::vector<Record>& recs = (
                replacement || r.second == ssim::mct::ActionIndicator::Add
                    ? to_insert
                    : to_remove
            );
            recs.push_back(r.first);
        }

        std::vector<std::pair<std::string, Record>> exst;

        if (replacement) {
            removeRecords(v.first);
        } else if (!to_remove.empty()) {
            getRecords(v.first).swap(exst);
            for (const Record& rec : to_remove) {
                auto ir = std::find_if(exst.begin(), exst.end(), [&rec] (const auto& x) {
                    return isEquivalent(x.second, rec);
                });
                if (ir != exst.end()) {
                    removeRecord(ir->first);
                    exst.erase(ir);
                }
            }
        }

        for (const Record& rec : to_insert) {
            auto ir = std::find_if(exst.begin(), exst.end(), [&rec] (const auto& x) {
                return isDuplicated(x.second, rec);
            });
            if (ir != exst.end()) {
                LogTrace(TRACE1) << "Duplications: " << ir->second << " VS " << rec;
                removeRecord(ir->first);
                exst.erase(ir);
                continue;
            }
            const auto id = insertRecord(rec);
            exst.emplace_back(id, rec);
        }

        postProcess(v.first);
        ServerFramework::applicationCallbacks()->commit_db();
    }

    //cleanup missing points
    if (replacement) {
        const auto exst = getMctPoints();

        std::set<nsi::PointId> to_remove;
        std::set_difference(
            exst.begin(), exst.end(), imps.begin(), imps.end(),
            std::inserter(to_remove, to_remove.begin())
        );

        for (nsi::PointId pt : to_remove) {
            LogTrace(TRACE1) << "Remove records for " << pt << " (no data imported)";

            removeRecords(pt);
            ServerFramework::applicationCallbacks()->commit_db();
        }
    }
}

} } //ssim::mct
