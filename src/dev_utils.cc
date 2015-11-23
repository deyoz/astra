#include "astra_utils.h"
#include "dev_utils.h"
#include "basic.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "misc.h"
#include <utility>
#include <vector>
#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace ASTRA;
using namespace std;
using namespace EXCEPTIONS;


const std::string BCBPInternalWork::meet_null_term_symb = "meet unexpected null (\\0) symbol in str";
static const string mandatory_str = "mandatory";
static const string conditional_str = "conditional";
static const string alfa_in_numeric_field = "got non-number in numeric field";
static const string non_decimal_str = "non-decimal symbols before last";
static const int max_supported_size_of_repeated_field = 9;

using namespace BCBPPosAndSizes;

static const TConstPos pos_format_code(0,1);
static const TConstPos pos_number_of_legs_encoded(pos_format_code,1);
static const TConstPos pos_passenger_name(pos_number_of_legs_encoded,20);
static const TConstPos pos_electronic_ticket_indicator(pos_passenger_name,1);


static const TConstPos pos_operating_pnr_code(0,7);
static const TConstPos pos_from_city_airport_code(pos_operating_pnr_code,3);
static const TConstPos pos_to_city_airport_code(pos_from_city_airport_code,3);
static const TConstPos pos_operating_carrier_designator(pos_to_city_airport_code,3);
static const TConstPos pos_flight_number(pos_operating_carrier_designator,5);
static const TConstPos pos_date_of_flight(pos_flight_number,3);
static const TConstPos pos_compartment_code(pos_date_of_flight,1);
static const TConstPos pos_seat_number(pos_compartment_code,4);
static const TConstPos pos_checkin_seq_num(pos_seat_number,5);
static const TConstPos pos_passenger_status(pos_checkin_seq_num,1);
static const TConstPos pos_field_of_var_sz_field(pos_passenger_status,2);



static const TConstPos pos_begining_of_version_num(0,1);
static const TConstPos pos_version(pos_begining_of_version_num,1);
static const TConstPos pos_size_following_struct(pos_version,2);
static const TConstPos pos_pass_descr(pos_size_following_struct,1);
static const TConstPos pos_checkin_source(pos_pass_descr,1);
static const TConstPos pos_pass_issuance_source(pos_checkin_source,1);
static const TConstPos pos_pass_issuance_year(pos_pass_issuance_source,1);
static const TConstPos pos_pass_issuance_date(pos_pass_issuance_year,3);
static const TConstPos pos_doc_type(pos_pass_issuance_date,1);
static const TConstPos pos_airline_designator(pos_doc_type,3);
static const TConstPos pos_baggage_plate_nums[3] = {TConstPos(pos_airline_designator,13), TConstPos(pos_baggage_plate_nums[0],13), TConstPos(pos_baggage_plate_nums[1],13)};


static const TConstPos pos_size_rep_section(0,2);
static const TConstPos pos_airline_num_code(pos_size_rep_section,3);
static const TConstPos pos_doc_sn(pos_airline_num_code,10);
static const TConstPos pos_selectee_ind(pos_doc_sn,1);
static const TConstPos pos_int_doc_verification(pos_selectee_ind,1);
static const TConstPos pos_marketing_designator(pos_int_doc_verification,3);
static const TConstPos pos_freq_flyer_designator(pos_marketing_designator,3);
static const TConstPos pos_freq_flyer_number(pos_freq_flyer_designator,16);
static const TConstPos pos_id_ad_ind(pos_freq_flyer_number,1);
static const TConstPos pos_free_baggage_allowance(pos_id_ad_ind,3);
static const TConstPos pos_fast_track(pos_free_baggage_allowance,1);


static const TConstPos pos_beg_security_data(0,1);
static const TConstPos pos_typeof_sec_data(pos_beg_security_data,1);
static const TConstPos pos_length_sec_data(pos_typeof_sec_data,2);
static const TConstPos pos_sec_data(pos_length_sec_data,1);


int TConstPos::size() const
{ return end - begin;
}

static std::string show_pos(int i)
{   return std::string(", pos[") + std::to_string(i) + "]";
}
const std::string BCBPInternalWork::small_data_size(int i)
{   return std::string("too small size of section (") + std::to_string(i) + ")";
}

TDevOperType DecodeDevOperType(string s)
{
  unsigned int i;
  for(i=0;i<sizeof(TDevOperTypeS)/sizeof(TDevOperTypeS[0]);i+=1) if (s == TDevOperTypeS[i]) break;
  if (i<sizeof(TDevOperTypeS)/sizeof(TDevOperTypeS[0]))
    return (TDevOperType)i;
  else
    return dotUnknown;
}

string EncodeDevOperType(TDevOperType s)
{
  return TDevOperTypeS[s];
}

TDevFmtType DecodeDevFmtType(string s)
{
  unsigned int i;
  for(i=0;i<sizeof(TDevFmtTypeS)/sizeof(TDevFmtTypeS[0]);i+=1) if (s == TDevFmtTypeS[i]) break;
  if (i<sizeof(TDevFmtTypeS)/sizeof(TDevFmtTypeS[0]))
    return (TDevFmtType)i;
  else
    return dftUnknown;
}

string EncodeDevFmtType(TDevFmtType s)
{
  return TDevFmtTypeS[s];
};

ASTRA::TDevClassType getDevClass(const TOperMode desk_mode,
                                 const std::string &env_name)
{
  switch(desk_mode)
  {
    case omCUSE: if (env_name == "ATB") return dctATB;
                 if (env_name == "BTP") return dctBTP;
                 if (env_name == "BGR") return dctBGR;
                 if (env_name == "DCP") return dctDCP;
                 if (env_name == "SCN") return dctSCN;
                 if (env_name == "OCR") return dctOCR;
                 if (env_name == "MSR") return dctMSR;
                 if (env_name == "WGE") return dctWGE;
                 return dctUnknown;
    case omMUSE: if (env_name == "ATB") return dctATB;
                 if (env_name == "BTP") return dctBTP;
                 if (env_name == "BGR") return dctBGR;
                 if (env_name == "DCP") return dctDCP;
                 if (env_name == "BCR") return dctSCN;
                 if (env_name == "OCR") return dctOCR;
                 if (env_name == "MSR") return dctMSR;
                 return dctUnknown;
    case omRESA: if (env_name == "ATB") return dctATB;
                 if (env_name == "BPP") return dctATB;
                 if (env_name == "BTP") return dctBTP;
                 if (env_name == "BGR") return dctBGR;
                 if (env_name == "BCD") return dctBGR;
                 if (env_name == "DCP") return dctDCP;
                 if (env_name == "MSG") return dctDCP;
                 if (env_name == "RTE") return dctWGE;
                 return dctUnknown;
        default: return dctUnknown;
  };
};

std::string getDefaultDevModel(const TOperMode desk_mode,
                               const ASTRA::TDevClassType dev_class)
{
  switch(desk_mode)
  {
    case omCUSE:
      switch(dev_class)
      {
        case dctATB: return "ATB CUSE";
        case dctBTP: return "BTP CUSE";
        case dctBGR: return "BCR CUSE";
        case dctDCP: return "DCP CUSE";
        case dctSCN: return "SCN CUSE";
        case dctOCR: return "OCR CUSE";
        case dctMSR: return "MSR CUSE";
        case dctWGE: return "WGE CUSE";
        default: return "";
      };
    case omMUSE:
       switch(dev_class)
       {
        case dctATB: return "ATB MUSE";
        case dctBTP: return "BTP MUSE";
        case dctBGR: return "BCR MUSE";
        case dctDCP: return "DCP MUSE";
        case dctSCN: return "SCN MUSE";
        case dctOCR: return "OCR MUSE";
        case dctMSR: return "MSR MUSE";
            default: return "";
      };
    case omRESA:
      switch(dev_class)
      {
        case dctATB: return "ATB RESA";
        case dctBTP: return "BTP RESA";
        case dctBGR: return "BCR RESA";
        case dctDCP: return "DCP RESA";
        case dctWGE: return "WGE RESA";
        default: return "";
      };
    default: return "";
  };
};


template<bool allow_non_found>
unsigned int BCBPInternalWork::get_int(const std::string& x, TConstPos pos, std::string& err, unsigned int allow_min, unsigned int allow_max)
{
    return get_int<allow_non_found>(x, pos.begin, pos.end, err, allow_min, allow_max);
}

template<class T, bool allow_nums, bool allow_non_found> boost::optional<T>  BCBPInternalWork::get_enum_opt(const std::string& x, TConstPos pos, std::string& err, const std::string& test)
{
    return get_enum_opt<T, allow_nums, allow_non_found>(x, pos.begin, err, test);
}

template<bool allow_nums, bool allow_non_found>
std::string BCBPInternalWork::get_alfa_chars_str(const std::string& x, TConstPos pos, std::string& err, const std::string special_symbols_allowed)
{
    return get_alfa_chars_str<allow_nums>(x, pos.begin, pos.end, err, special_symbols_allowed);
}

template<bool allow_non_num_alfa, bool allow_non_found>
char BCBPInternalWork::get_char(const std::string& x, TConstPos pos, std::string& err){
    return get_char<allow_non_num_alfa>(x, pos.begin, err);
}



int BCBPUniqueSections::numberOfLigs() const
{
  if (mandatory.size()!=23) throw EConvertError("invalid size of unique mandatory section");
  int result=NoExists;
  if (BASIC::StrToInt(mandatory.substr(1,1).c_str(), result)==EOF ||
      result<=0 || result>9) throw EConvertError("invalid item 5 <Number of Legs Encoded>");
  return result;
}

pair<string, string> BCBPUniqueSections::passengerName() const
{
  if (mandatory.size()!=23) throw EConvertError("invalid size of unique mandatory section");
  pair<string, string> result;
  result.first=mandatory.substr(2,20);
  TrimString(result.first);
  if (result.first.empty()) throw EConvertError("empty item 11 <Passenger Name>");
  string& res = result.first;
  for(unsigned int i = 0; i<res.size(); i++)
  {
      if(!IsLetter(res[i]) && res[i] != ' ' && res[i] != '/' && res[i] != '-')
          throw EConvertError(string("invalid item 11 <Passenger Name> at ") + std::to_string(i) + "of pos in field");
  }
  string::size_type pos=result.first.find('/');
  if (pos != string::npos)
  {
    result.second=result.first.substr(pos+1);
    result.first=result.first.substr(0,pos);
    TrimString(result.first);
    TrimString(result.second);
    if (result.first.empty()) throw EConvertError("invalid item 11 <Passenger Name>");
  };

  return make_pair(upperc(result.first), upperc(result.second));
}

std::string BCBPRepeatedSections::operatingCarrierPNRCode() const
{
  if (mandatory.size()!=37) throw EConvertError("invalid size of repeated mandatory section");
  string result=mandatory.substr(0,7);
  TrimString(result);

  return upperc(result);
}

string BCBPRepeatedSections::fromCityAirpCode() const
{
  if (mandatory.size()!=37) throw EConvertError("invalid size of repeated mandatory section");
  string result=mandatory.substr(7,3);
  TrimString(result);
  if (result.empty()) throw EConvertError("empty item 26 <From City Airport Code>");
  for(string::const_iterator c=result.begin(); c!=result.end(); ++c)
    if (!IsLetter(*c)) throw EConvertError("invalid item 26 <From City Airport Code>");

  return upperc(result);
}

string BCBPRepeatedSections::toCityAirpCode() const
{
  if (mandatory.size()!=37) throw EConvertError("invalid size of repeated mandatory section");
  string result=mandatory.substr(10,3);
  TrimString(result);
  if (result.empty()) throw EConvertError("empty item 38 <To City Airport Code>");
  for(string::const_iterator c=result.begin(); c!=result.end(); ++c)
    if (!IsLetter(*c)) throw EConvertError("invalid item 38 <To City Airport Code>");

  return upperc(result);
}

string BCBPRepeatedSections::operatingCarrierDesignator() const
{
  if (mandatory.size()!=37) throw EConvertError("invalid size of repeated mandatory section");
  string result=mandatory.substr(13,3);
  TrimString(result);
  if (result.empty()) throw EConvertError("empty item 42 <Operating carrier Designator>");
  for(string::const_iterator c=result.begin(); c!=result.end(); ++c)
    if (!IsDigitIsLetter(*c)) throw EConvertError("invalid item 42 <Operating carrier Designator>");

  return upperc(result);
}

std::pair<int, std::string> BCBPRepeatedSections::flightNumber() const
{
  if (mandatory.size()!=37) throw EConvertError("invalid size of repeated mandatory section");
  pair<int, string> result(NoExists, "");
  string str=mandatory.substr(16,5);
  TrimString(str);
  if (str.empty()) throw EConvertError("empty item 43 <Flight Number>");

  if (IsLetter(*str.rbegin()))
  {
    result.second=string(1,*str.rbegin());
    str.erase(str.size()-1);
    TrimString(str);
    if (str.empty()) throw EConvertError("invalid item 43 <Flight Number>");
  };

  if ( BASIC::StrToInt( str.c_str(), result.first ) == EOF ||
       result.first > 99999 || result.first <= 0 )
    throw EConvertError("invalid item 43 <Flight Number>");

  return make_pair(result.first, upperc(result.second));
}

int BCBPRepeatedSections::dateOfFlight() const
{
  if (mandatory.size()!=37) throw EConvertError("invalid size of repeated mandatory section");
  int result=NoExists;
  if (BASIC::StrToInt(mandatory.substr(21,3).c_str(), result)==EOF ||
      result<=0 || result>366) throw EConvertError("invalid item 46 <Date of Flight (Julian Date)>");
  return result;
}

std::pair<int, std::string> BCBPRepeatedSections::checkinSeqNumber() const
{
  if (mandatory.size()!=37) throw EConvertError("invalid size of repeated mandatory section");
  pair<int, string> result(NoExists, "");
  string str=mandatory.substr(29,5);
  TrimString(str);
  if (!str.empty())
  {
    if (IsLetter(*str.rbegin()))
    {
      result.second=string(1,*str.rbegin());
      str.erase(str.size()-1);
      TrimString(str);
      if (str.empty()) throw EConvertError("invalid item 107 <Check-In Sequence Number>");
    };

    if ( BASIC::StrToInt( str.c_str(), result.first ) == EOF ||
         result.first > 99999 || result.first <= 0 )
      throw EConvertError("invalid item 107 <Check-In Sequence Number>");
  };

  return make_pair(result.first, upperc(result.second));
}

int BCBPUniqueSections::conditionalSize() const //conditional
{
  if (conditional.empty()) throw Exception("%s: empty unique conditional section", __FUNCTION__);

  if (conditional.size()<4) throw EConvertError("invalid size of unique conditional section");
  return BCBPSections::fieldSize(conditional.substr(2,2),
                                 EConvertError("invalid item 10 <Field size of following structured message - unique>"));
}

int BCBPUniqueSections::securitySize() const //security
{
  if (security.empty()) throw Exception("%s: empty security section", __FUNCTION__);

  if (security.size()<4) throw EConvertError("invalid size of security section");
  return BCBPSections::fieldSize(security.substr(2,2),
                                 EConvertError("invalid item 29 <Length of Security Data>"));
}

int BCBPRepeatedSections::variableSize(int seg) const //conditional+individual
{
  if (mandatory.size()!=37) throw EConvertError("invalid size of repeated mandatory section (Flight Segment #%d)", seg);
  return BCBPSections::fieldSize(mandatory.substr(35,2),
                                 EConvertError("invalid item 6 <Field size of variable size field> (Flight Segment #%d)", seg));
}

int BCBPRepeatedSections::conditionalSize(int seg) const //conditional
{
  if (conditional.empty()) throw Exception("%s: empty repeated conditional section (Flight Segment #%d)", __FUNCTION__, seg);

  if (conditional.size()<2) throw EConvertError("invalid size of repeated conditional section (Flight Segment #%d)", seg);
  return BCBPSections::fieldSize(conditional.substr(0,2),
                                 EConvertError("invalid item 17 <Field size of following structured message - repeated> (Flight Segment #%d)", seg));
}

int BCBPSections::fieldSize(const std::string &s,
                            const EXCEPTIONS::EConvertError &e)
{
  string c;
  if (!HexToString(s, c) || c.size()!=1) throw e;
  int result=c[0];
  if (result<0x00 || result>0xFF)  throw e;
  return result;
}

string BCBPSections::substr_plus(const std::string &bcbp,
                                 string::size_type &idx,
                                 const string::size_type &len,
                                 const EXCEPTIONS::EConvertError &e)
{
  string result=substr(bcbp, idx, len, e);
  idx+=len;
  return result;
}

string BCBPSections::substr(const std::string &bcbp,
                            const string::size_type &idx,
                            const string::size_type &len,
                            const EXCEPTIONS::EConvertError &e)
{
  if (idx+len>bcbp.size()) throw e;
  return bcbp.substr(idx, len);
}

void BCBPSections::get(const std::string &bcbp,
                       const string::size_type bcbp_begin_idx,
                       const string::size_type bcbp_end_idx,
                       BCBPSections &sections,
                       bool only_mandatory)
{
  sections.clear();
  if (bcbp.empty()) throw Exception("%s: empty bcbp", __FUNCTION__);

  if (bcbp_begin_idx==string::npos || bcbp.size()-bcbp_begin_idx<1)
    throw Exception("%s: wrong bcbp_begin_idx", __FUNCTION__);
  if (bcbp[bcbp_begin_idx]!='M')
    throw EConvertError("unknown item 1 <Format Code>");

  if (bcbp_end_idx!=string::npos && bcbp.size()<bcbp_end_idx)
    throw Exception("%s: wrong bcbp_end_idx", __FUNCTION__);

  sections.unique.mandatory=substr(bcbp, bcbp_begin_idx, 23,
                                   EConvertError("invalid size of mandatory unique section"));
  string::size_type mandatory_repeated_idx=bcbp_begin_idx+23;
  string err;
  for(int seg=1; seg<=sections.unique.numberOfLigs(); seg++)
  {
    sections.repeated.push_back(BCBPRepeatedSections());
    BCBPRepeatedSections &repeatedSection=sections.repeated.back();
    repeatedSection.mandatory=substr(bcbp, mandatory_repeated_idx, 37,
                                     EConvertError("invalid size of mandatory repeated section (Flight Segment #%d)", seg));
    if (repeatedSection.mandatory[0]=='^') throw EConvertError("invalid item 5 <Number of Legs Encoded> (Flight Segment #%d)", seg);

    string::size_type conditional_idx=mandatory_repeated_idx+37;
    mandatory_repeated_idx=conditional_idx+repeatedSection.variableSize(seg);
    if (mandatory_repeated_idx==conditional_idx || only_mandatory) continue;

    //conditional section
    string conditional=substr(bcbp, conditional_idx, repeatedSection.variableSize(seg),
                              EConvertError("invalid size of conditional section (Flight Segment #%d)", seg));
    conditional_idx=0;
    if (seg==1)
    {
      sections.unique.conditional=substr_plus(conditional, conditional_idx, 4,
                                              EConvertError("invalid size of conditional unique section"));

      if (sections.unique.conditional[0]!='>') {
          err = "invalid item 8 <Beginning of version number>, found data: (";
          err.push_back(sections.unique.conditional[0]);
          err.push_back(')');
          throw EConvertError(err);
      }

      sections.unique.conditional+=substr_plus(conditional,
                                               conditional_idx,
                                               sections.unique.conditionalSize(),
                                               EConvertError("invalid size of conditional unique section"));
    };
    repeatedSection.conditional=substr_plus(conditional, conditional_idx, 2,
                                            EConvertError("invalid size of conditional repeated section (Flight Segment #%d)", seg));

    repeatedSection.conditional+=substr_plus(conditional,
                                             conditional_idx,
                                             repeatedSection.conditionalSize(seg),
                                             EConvertError("invalid size of conditional repeated section (Flight Segment #%d)", seg));
    repeatedSection.individual=conditional.substr(conditional_idx);
  };
  if (only_mandatory) return;

  string::size_type security_idx=mandatory_repeated_idx;
  if (bcbp.size()-security_idx>=1 && bcbp[security_idx]=='^')
  {
    //есть секция secrity
    sections.unique.security=substr_plus(bcbp, security_idx, 4,
                                         EConvertError("invalid size of security section"));
    sections.unique.security+=substr_plus(bcbp,
                                          security_idx,
                                          sections.unique.securitySize(),
                                          EConvertError("invalid size of security section"));
  };
  if (bcbp_end_idx!=string::npos && security_idx!=bcbp_end_idx)
  { err = string("invalid size of bcbp, expected ") + std::to_string(bcbp_end_idx) + " got " + std::to_string(security_idx);
    throw EConvertError(err);
  }

}

std::ostream& operator<<(std::ostream& os, const BCBPUniqueSections& section)
{
  os << "  mandatory:   |" << section.mandatory << "|" << endl
     << "  conditional: |" << section.conditional << "|" << endl
     << "  security:    |" << section.security << "|";
  return os;
}

std::ostream& operator<<(std::ostream& os, const BCBPRepeatedSections& section)
{
  os << "  mandatory:   |" << section.mandatory << "|" << endl
     << "  conditional: |" << section.conditional << "|" << endl
     << "  individual:  |" << section.individual << "|";
  return os;
}

std::ostream& operator<<(std::ostream& os, const BCBPSections& sections)
{
  os << "uniqie:" << endl
     << sections.unique << endl
     << "repeated: ";
  int seg=1;
  for(vector<BCBPRepeatedSections>::const_iterator s=sections.repeated.begin(); s!=sections.repeated.end(); ++s, seg++)
  {
    os << endl
       << "seg #" << seg << endl
       << *s;
  };
  return os;
}

//bcbp_begin_idx - позиция первого байта штрих-кода
//airline_use_begin_idx - позиция первого байта <For individual airline use> первого сегмента
//airline_use_end_idx - позиция, следующая за последним байтом <For individual airline use> первого сегмента
void checkBCBP_M(const string &bcbp,
                 const string::size_type bcbp_begin_idx,
                 string::size_type &airline_use_begin_idx,
                 string::size_type &airline_use_end_idx)
{
  airline_use_begin_idx=string::npos;
  airline_use_end_idx=string::npos;
  if (bcbp.empty()) throw Exception("checkBCBP_M: empty bcbp");
  string::size_type bcbp_size=bcbp.size();
  if (/*bcbp_begin_idx<0 ||*/ bcbp_begin_idx>=bcbp_size) throw Exception("checkBCBP_M: wrong bcbp_begin_idx");

  string::size_type p=bcbp_begin_idx;
  int item6, len_u, len_r;
  string c;
  if (bcbp[p]!='M') throw EConvertError("unknown item 1 <Format Code>");
  if (bcbp_size<p+60) throw EConvertError("invalid size of mandatory items");
  if (!HexToString(bcbp.substr(p+58,2),c) || c.size()<1)
    throw EConvertError("invalid item 6 <Field size of variable size field>");
  item6=(int)c[0]; //item 6
  p+=60;
  if (bcbp_size<p+item6) throw EConvertError("invalid size of conditional items + item 4 <For individual airline use>");
  airline_use_end_idx=p+item6;
  if (item6>0)
  {
    if (bcbp[p]=='>')
    {
      //есть conditional items
      if (bcbp_size<p+4) throw EConvertError("invalid size of conditional items");
      if (!HexToString(bcbp.substr(p+2,2),c) || c.size()<1)
        throw EConvertError("invalid item 10 <Field size of following structured message - unique>");
      len_u=(int)c[0]; //item 10
      //ProgTrace(TRACE5,"checkBCBP_M: len_u=%d",len_u);
      p+=4;
      if (bcbp_size<p+len_u+2) throw EConvertError("invalid size of following structured message - unique");
      if (!HexToString(bcbp.substr(p+len_u,2),c) || c.size()<1)
        throw EConvertError("invalid item 17 <Field size of following structured message - repeated>");
      len_r=(int)c[0]; //item 17
      //ProgTrace(TRACE5,"checkBCBP_M: len_r=%d",len_r);
      p+=len_u+2;
      if (bcbp_size<p+len_r) throw EConvertError("invalid size of following structured message - repeated");
      p+=len_r;
    };
  };
  //p указывает на начало item 4 <For individual airline use>
  //airline_use_end_idx указывает на item 25 <Beginning of Security Data> (следующий байт после item 4)
  if (airline_use_end_idx<p) throw EConvertError("6,10,17 items do not match");
  airline_use_begin_idx=p;
};

#include <fstream>

int bcbp_test(int argc,char **argv)
{
    ifstream ifs("bcbp");
    std::string bcbp( (std::istreambuf_iterator<char>(ifs) ),
            (std::istreambuf_iterator<char>()    ) );

    BCBPSections sections;
    try
    {
        BCBPSections::get(bcbp, 0, bcbp.size(), sections, true);
    }
    catch(EXCEPTIONS::EConvertError &e)
    {
    }
    return 1;
}



void BCBPUniqueSections::mandatory_size_check(int i)
{   if (mandatory.size()!=23)
    {
     if(i) throw EConvertError(string("invalid size of unique mandatory section ") + std::to_string(i));
     throw EConvertError("invalid size of unique mandatory section");
    }

}


string BCBPInternalWork::invalid_format(const string& x)
{
    return std::string("invalid format of bcbp field: \"") + x + std::string("\" ");
}
string BCBPInternalWork::invalid_format(const string& field, const string& what)
{
    return std::string("invalid format of bcbp field: \"") + field + std::string(" \" , ") + what;
}
const bool is_allow_nums = false;
template<class T, bool allow_nums, bool allow_non_found> boost::optional<T>  BCBPInternalWork::get_enum_opt(const string& x, int start, string& err, const string& test)
{   char ret = get_char<true, allow_non_found>(x, start, err);
    if(ret == ' ') return boost::none;
    int  i;
    for(i = 0; i<test.size(); i++)
        if(test[i] == ret) goto Label_far;
    err = "unexpected data in enum field \" \"";
    err[err.size()-2] = ret;
    return boost::none;
    Label_far:
    return boost::optional<T>(static_cast<T>(i));
}

bool BCBPInternalWork::bad_symbol(char x, string& err)
{
    if(!IsDigitIsLetter(x) && !isspace(x))
    {
        err = "bad symbol";
        return true;
    }
    return false;
}

std::string BCBPInternalWork::write_section_str(const string& field, const string& field_type, const string& descr, int section)
{       string str_section;
        if(section > max_supported_size_of_repeated_field || section < -1)
        {   if(field.empty() && field_type.empty())
                throw(EConvertError(descr));
            else
                throw(EConvertError(invalid_format(field) + descr + " also attempt to show section with unsupported number"));
        }
        if(section != -1)
        {   str_section = "rep. section N ";
            str_section.push_back((char)(section + '0'));
        }
        else str_section = "unique section";
        return str_section;
}

void BCBPInternalWork::test_on_warning(char x)
{   //функция IsLetter вызывается до этой
    if(x >= 'A' && x<= 'Z') return;
    if(x >= 'a' && x<= 'z')
    {   found_lower_case = true;
        return;
    }
    found_cyrilic = true;
    return;
}

void BCBPInternalWork::process_err(const string& field, const string& field_type, const string& descr, int section)
{   if(descr.empty() && !found_cyrilic &&  !found_lower_case) return;
    string str_section = write_section_str(field, field_type, descr, section);
    if(found_cyrilic)
    {   warnings.push_back(string("warning in field: ") + field + " in " + str_section  + " found cyrilic symbols");
        found_cyrilic = false;
    }
    if(found_lower_case)
    {   warnings.push_back(string("warning in field: ") + field + " in " + str_section + " found lower case symbols");
        found_lower_case = false;
    }
    if(descr.empty()) return;
    throw(EConvertError(invalid_format(field) + "in " + str_section + ", " + field_type + ": " + descr));
}



template<bool allow_nums, bool allow_non_found>
std::string BCBPInternalWork::get_alfa_chars_str(const string& x, int start, int end, string& err, const std::string special_symbols_allowed)
{
    if(x.size() < end)
    {   if(!allow_non_found) err = small_data_size(x.size());
        return "";
    }
    string ret;
    ret.reserve(end - start);
    char byte;
    const int sp_s_str_sz = special_symbols_allowed.size();
    string parsed_substr = string(" (") + x.substr(start, end - start) + ")";
    for(int i = start; i < end; i++)
    {
        byte = x[i];
        if(byte == '\0')
        {   err = string("meet null symbol") + show_pos(i) + parsed_substr;
            return "";
        }
        else
        if(isdigit(byte))
        {   if(!allow_nums)
            {err = string("numbers are not allowed")+ show_pos(i) + parsed_substr;
            return "";
            }
        }
        else if(byte != ' ' && !IsLetter(byte))
        { if(sp_s_str_sz)
          {  bool symbol_allowed = false;
             for(int i = 0; i<sp_s_str_sz; i++)
                if(byte == special_symbols_allowed[i])
                {   symbol_allowed = true;
                    break;
                }

             if(!symbol_allowed)
             {   err = string("special symbols are not allowed")+ show_pos(i) + parsed_substr;
                return "";
             }
          }
          else
          {  err = string("non-alfanumeric are not allowed")+ show_pos(i) + parsed_substr ;
             return "";
          }
        }
        else
            test_on_warning(byte);
        ret.push_back(byte);
    }
    return ret;
}

void BCBPSections::check_i(int i)
{   const int unsupported_size = max_supported_size_of_repeated_field + 1;
    if(i < 0)
        process_err("", "", "new added field parsing: attemt to access to vector<> repeated[] with negative number " + std::to_string(i), unsupported_size);
     if(i >= (int)repeated.size())
        process_err("", "", "new added field parsing: attemt to access to vector<> repeated[] with " + std::to_string(i) + " >= repeated.size()", unsupported_size);
}


bool alfa_num_check(char x, std::string& err)
{
    if(x != ' ' && !IsDigitIsLetter(x)) {
        err = "non alfa or num symbol \" \" not allowed";
        err[24] = x;
        return false;
    }
    return true;
}

template<bool allow_non_num_alfa, bool allow_non_found>
char BCBPInternalWork::get_char(const string& x, int pos, string& err)
{
    if(pos >= x.size())
    {   if(!allow_non_found)
        {    err = small_data_size(x.size());
             return -1;
        }
        else return ' ';
    }
    char byte = x[pos];
    //if(bad_symbol(byte, err)) return '\0';
    if(byte == '\0')
    {   err = string("meet null symbol") + show_pos(pos);
        return -1;
    }
    if(!allow_non_num_alfa && !alfa_num_check(byte, err))
        return -1;
    return byte;
}


template<bool allow_non_found>
unsigned int BCBPInternalWork::get_int(const string& x, unsigned int start, unsigned int end, string& err, unsigned int allow_min, unsigned int allow_max)
{   if(x.size() < end)
    {   err = small_data_size(x.size());
        return 0;
    }
    if(end - start > 9)
    {    err = "wrong opts in BCBPAnySection::get_int func: start - end params not allowed to exceed 9 ";
        return 0;
    }
    string parsed_substr = string(" (") + x.substr(start, end - start) + ")";
    unsigned int ret = 0;
    bool got_num = false, got_ending_space = false;
    for(unsigned int i = start; i < end; i++)
    {  if(x[i] == ' ')
       {    if(got_num) got_ending_space = true;
            continue;
       }
       if(x[i] < '0' || x[i] > '9')\
       {   if(x[i] != '\0') err = alfa_in_numeric_field;
           else err = meet_null_term_symb + show_pos(i) + parsed_substr;
           return 0;
       }
       if(got_ending_space)
       {   err = string("2 or more nums in numeric field") + parsed_substr;
           return 0;
       }
       ret*=10;
       ret+=x[i] - '0';
       got_num = true;
    }
    if(ret < allow_min)
    {   err  = "got number (" + std::to_string(ret) +  ") less then minimum allowed for this field";
    }
    if(ret > allow_max)
    {   err  = "got number (" + std::to_string(ret) + ") exceed maximum allowed for this field";
        return 0;
    }
    return ret;
}

template<bool allow_non_found>
int BCBPInternalWork::get_hex(const string& x, unsigned int start, string& err)
{
    if(x.size() <= start + 1)
    {   if(!allow_non_found){
            err = small_data_size(x.size());
            return -1;
        }
        else return 0;
    }
    char a = get_xdigit(x[start]), b = get_xdigit(x[start + 1]);
    if(a == -1 || b == -1)
    {   err = "wrong format of hex num ";
        a = x[start], b = x[start+1];
        if(a != '\0' && b != '\0')
        { err = string("got \"");
          err.push_back(a);
          err.push_back(b);
          err.push_back('\"');
        }
        else
           err = meet_null_term_symb + show_pos(start + (a == -1 ? 0 : 1));
        return -1;
    }
    return a<<4 && b;
}

std::string BCBPInternalWork::delete_ending_blanks(const std::string& x)
{   for(int i = x.size() - 1; i >= 0; i--)
        if(x[i] != ' ') return x.substr(0, i + 1);
    return "";
}

std::string BCBPInternalWork::delete_all_blanks(std::string& x)
{   return TrimString(x);
}


bool BCBPSections::electronic_ticket_indicator()
{   unique.mandatory_size_check(0);
    std::string err;
    char e_t_indicator = get_char<false>(unique.mandatory, pos_electronic_ticket_indicator, err);
    std::string field_name = "electronic ticket indicator";
    process_err(field_name, mandatory_str, err);
    if(e_t_indicator == 'E') return true;
    if(e_t_indicator == ' ') return false;
    err = "got \" \" instead E or space";
    err[5] = e_t_indicator;
    process_err(field_name, mandatory_str, err);
    return false; //никогда не выполнится, просто для подавления warning
}

std::string BCBPSections::operatingCarrierPNR(int i)
{   check_i(i);
    return repeated[i].operatingCarrierPNRCode();
}

std::string BCBPSections::from_city_airport(int i)
{   check_i(i);
    std::string err;
    string ret = get_alfa_chars_str<true>(repeated[i].mandatory, pos_from_city_airport_code, err);
    process_err("from city airport", mandatory_str, err, i);
    return delete_ending_blanks(ret);
}

std::string BCBPSections::to_city_airport(int i)
{   check_i(i);
    std::string err;
    string ret = get_alfa_chars_str<true>(repeated[i].mandatory, pos_to_city_airport_code, err);
    process_err("to city airport", mandatory_str, err, i);
    return delete_ending_blanks(ret);
}


std::string BCBPSections::operating_carrier_designator(int i)
{   check_i(i);
    std::string err;
    string ret = get_alfa_chars_str<true>(repeated[i].mandatory, pos_operating_carrier_designator, err);
    process_err("operating carrier designator", mandatory_str, err, i);
    return delete_ending_blanks(ret);
}



std::string BCBPSections::flight_number(int i)
{   check_i(i);
    std::string err;
    string ret = get_alfa_chars_str<true>(repeated[i].mandatory, pos_flight_number, err);
    string field_name = "flight number";
    process_err(field_name, mandatory_str, err, i);
    const TConstPos pos(pos_flight_number.begin, pos_flight_number.size() - 1);
    get_int(repeated[i].mandatory, pos, err);
    if(!err.empty()) err = non_decimal_str + " " + err;
    process_err(field_name, mandatory_str, err, i);
    return delete_ending_blanks(ret);

}

int BCBPSections::date_of_flight(int i)
{   check_i(i);
    std::string err;
    int ret = get_int(repeated[i].mandatory, pos_date_of_flight, err, 1, 366);
    process_err("date of flight", mandatory_str, err, i);
    return ret;
}


boost::optional<BCBPSectionsEnums::PassengerClass> BCBPSections::compartment_code(int i)
{    check_i(i);
     std::string err;
     static string  allowed_symbols = "FJY";
     boost::optional<BCBPSectionsEnums::PassengerClass> ret = get_enum_opt<BCBPSectionsEnums::PassengerClass>(repeated[i].mandatory, pos_compartment_code, err, allowed_symbols);\
     std::string field_name = "compartment code";
     process_err(field_name, mandatory_str, err, i);
     return ret;
}
std::string BCBPSections::seat_number(int i)
{   check_i(i);
    std::string err, field_name = "seat number";
    std::string ret = get_alfa_chars_str<true>(repeated[i].mandatory, pos_seat_number, err);
    process_err(field_name, mandatory_str, err, i);
    const TConstPos pos(pos_seat_number.begin, pos_seat_number.size() - 1);
    get_int(repeated[i].mandatory, pos, err);
    if(!err.empty()) err = non_decimal_str + " " + err;
    process_err(field_name, mandatory_str, err, i);
    return delete_ending_blanks(ret);

}
std::string BCBPSections::check_in_seq_number(int i)
{   check_i(i);
    std::string err, field_name = "check in seq number";
    string ret = get_alfa_chars_str<true>(repeated[i].mandatory, pos_checkin_seq_num, err);
    process_err(field_name, mandatory_str, err, i);
    const TConstPos pos(pos_checkin_seq_num.begin, pos_checkin_seq_num.size() - 1);
    get_int(repeated[i].mandatory, pos, err);
    if(!err.empty()) err = non_decimal_str + " " + err;
    process_err(field_name, mandatory_str, err, i);
    return delete_ending_blanks(ret);

}
char BCBPSections::passenger_status(int i)
{   check_i(i);
    std::string err;
    char ret = get_char<true>(repeated[i].mandatory, pos_passenger_status, err);
    process_err("passenger status", mandatory_str, err, i);
    return ret;
}


boost::optional<BCBPSectionsEnums::PassengerDescr> BCBPSections::passenger_description()
{   std::string err;
    char ret = get_char<true>(unique.conditional, pos_pass_descr, err);
    if(ret == ' ') return boost::none;
    process_err("passenger description", conditional_str, err);
    if(!(ret >= '0' && ret <= '7')) return BCBPSectionsEnums::PassengerDescr::future_industry_use;
    return boost::optional<BCBPSectionsEnums::PassengerDescr>(static_cast<BCBPSectionsEnums::PassengerDescr>(ret-'0'));
}

template<bool allow_non_found = true>
static inline bool check_none_in_cond_int(const string& str, TConstPos pos)
{
    if(pos.end > str.size()) return allow_non_found;
    for(unsigned int i = pos.begin; i<pos.end; i++)
        if(str[i] != ' ')
            return false;
    return true;
}

boost::optional<int> BCBPSections::version()
{   std::string err;
    if(check_none_in_cond_int(unique.conditional, pos_version)) return boost::none;
    unsigned int ret = get_int(unique.conditional, pos_version, err);
    process_err("version", conditional_str, err);
    return ret;
}

boost::optional<BCBPSectionsEnums::SourceOfIssuance> BCBPSections::source_of_checkin()
{   static const std::string allowed_chars = "WKXRMOTV";
    string err, field_name = "source of check in";
    boost::optional<BCBPSectionsEnums::SourceOfIssuance> ret = get_enum_opt<BCBPSectionsEnums::SourceOfIssuance>(unique.conditional, pos_checkin_source, err, allowed_chars);
    process_err(field_name, conditional_str, err);
    if(ret == BCBPSectionsEnums::SourceOfIssuance::transfer_kiosk)
        process_err(field_name, conditional_str, "X symbol not allowed here");
    return ret;
}
boost::optional<BCBPSectionsEnums::SourceOfIssuance> BCBPSections::source_of_boarding_pass_issuance()
{   static const std::string allowed_chars = "WKXRMOTV";
    string err;
    boost::optional<BCBPSectionsEnums::SourceOfIssuance> ret = get_enum_opt<BCBPSectionsEnums::SourceOfIssuance>(unique.conditional, pos_pass_issuance_source, err, allowed_chars);
    process_err("source of boarding pass issuance", conditional_str, err);
    return ret;
}

boost::optional<BASIC::TDateTime> BCBPSections::date_of_boarding_pass_issuance()
{   using namespace BASIC;
    string err;
    TConstPos pos(pos_pass_issuance_year.begin, pos_pass_issuance_date.end);
    if(check_none_in_cond_int(unique.conditional, pos)) return boost::none;
    unsigned int ret_year = get_int(unique.conditional, pos_pass_issuance_year, err);
    process_err("date of boarding pass issuance (year part)", conditional_str, err);
    unsigned int ret_day = get_int(unique.conditional, pos_pass_issuance_date, err, 1, 366);
    process_err("date of boarding pass issuance (day part)", conditional_str, err);\
    int curr_year_in_decade, now_year, skip1, skip2;
    BASIC::TDateTime ret = NowUTC();
    DecodeDate(ret, now_year, skip1, skip2);
    curr_year_in_decade = now_year%10;
    if(curr_year_in_decade > (int)ret_year) ret_year += now_year - curr_year_in_decade; //try to deduce real date of ticket (format of ticket code dont allow enough precission of year keeping)
    else ret_year += now_year - curr_year_in_decade - 10;
    try {ret = JulianDateToDateTime(ret_day, ret_year);}
    catch(Exception e){process_err("date of boarding pass issuance", conditional_str, e.what());}
    return boost::optional<TDateTime>(ret);
}

boost::optional<BCBPSectionsEnums::DocType> BCBPSections::doc_type()
{   std::string err;
    static const string  allowed_values = "BI";
    boost::optional<BCBPSectionsEnums::DocType> ret = get_enum_opt<BCBPSectionsEnums::DocType>(unique.conditional, pos_doc_type, err, allowed_values);
    process_err("selectee", conditional_str, err);
    return ret;
}

std::string BCBPSections::airline_of_boarding_pass_issuance()
{   std::string err;
    string ret = get_alfa_chars_str<true>(unique.conditional, pos_airline_designator, err);
    process_err("airline of boarding pass issuance", conditional_str, err);
    return delete_ending_blanks(ret);
}

BCBPSections::Baggage_plate_nums::Baggage_plate_nums(int64_t _basic_number, unsigned short _consequense)
{ basic_number = _basic_number, consequense = _consequense;
}

std::vector<BCBPSections::Baggage_plate_nums> BCBPSections::baggage_plate_nums()
{   std::vector<std::string> x = baggage_plate_nums_as_str();
    std::vector<BCBPSections::Baggage_plate_nums> ret;
    int64_t temp_basic_number;
    unsigned short temp_consequense;
    for(unsigned int i = 0; i < x.size(); i++)
    {   temp_basic_number = std::atoi(x[i].substr(0,9).c_str());
        temp_consequense = std::atoi(x[i].substr(9,3).c_str());
        ret.push_back(Baggage_plate_nums(temp_basic_number, temp_consequense));
    }
    return ret;
}

std::vector<std::string> BCBPSections::baggage_plate_nums_as_str()
{   std::string err;
    vector<string> ret;
    if(pos_baggage_plate_nums[0].begin < unique.conditional.size())
    for(unsigned int i = 0; i < sizeof(pos_baggage_plate_nums) / sizeof(pos_baggage_plate_nums[0]); i++)
    {   if(i) if(pos_baggage_plate_nums[i-1].end == unique.conditional.size()) break;
        ret.push_back(get_alfa_chars_str<true>(unique.conditional, pos_baggage_plate_nums[i], err));
        process_err(string("baggage_plate_nums[") + std::to_string(i) + "]", conditional_str, err);
        for(unsigned int j = 0; j<ret[i].size(); j++)
            if(!isdigit(ret[i][j]))
                process_err(string("baggage_plate_nums [") + std::to_string(i) + "]", conditional_str, alfa_in_numeric_field + show_pos(j + pos_baggage_plate_nums[i].begin));
    }
    return ret;
}


boost::optional<int> BCBPSections::airline_num_code(int i)
{   check_i(i);
    std::string err;
    if(check_none_in_cond_int(repeated[i].conditional, pos_airline_num_code)) return boost::none;
    int ret = get_int(repeated[i].conditional, pos_airline_num_code, err);
    process_err("airline num code", conditional_str, err);
    return ret;
}

std::string BCBPSections::doc_serial_num(int i)
{   check_i(i);
    std::string err;
    string ret = get_alfa_chars_str<true>(repeated[i].conditional, pos_doc_sn, err);
    process_err("doc serial num", conditional_str, err, i);
    return ret;

}

boost::optional<bool> BCBPSections::selectee(int i)
{   check_i(i);
    std::string err;
    static const string  allowed_values = "01";
    boost::optional<bool> ret = get_enum_opt<bool>(repeated[i].conditional, pos_selectee_ind, err, allowed_values);
    process_err("selectee", conditional_str, err, i);
    return ret;

}

char BCBPSections::international_doc_verification(int i)
{   check_i(i);
    std::string err;
    char ret = get_char<true>(repeated[i].conditional, pos_int_doc_verification, err);
    process_err("international doc verification", conditional_str, err, i);
    return ret;
}

std::string BCBPSections::marketing_carrier_designator(int i)
{   check_i(i);
    std::string err;
    string ret = get_alfa_chars_str<true>(repeated[i].conditional, pos_marketing_designator, err);
    process_err("marketing carrier designator",conditional_str, err, i);
    return delete_ending_blanks(ret);

}
std::string BCBPSections::frequent_flyer_airline_designator(int i)
{   check_i(i);
    std::string err;
    string ret = get_alfa_chars_str<true>(repeated[i].conditional, pos_freq_flyer_designator, err);
    process_err("frequent flyer designator", conditional_str, err, i);
    return delete_ending_blanks(ret);
}
std::string BCBPSections::frequent_flyer_num(int i)
{   check_i(i);
    std::string err;
    string ret = get_alfa_chars_str<true>(repeated[i].conditional, pos_freq_flyer_number, err);
    process_err("frequent flyer number", conditional_str, err, i);
    return delete_ending_blanks(ret);
}


char BCBPSections::id_ad(int i)
{   std::string err;
    char ret = get_char<true>(repeated[i].conditional, pos_id_ad_ind, err);
    process_err("ID/AD indicator", conditional_str, err, i);
    return ret;
}




boost::optional<bool> BCBPSections::fast_track(int i)
{   check_i(i);
    static string  allowed_values = "NY";
    std::string err;
    boost::optional<bool> ret = get_enum_opt<bool>(repeated[i].conditional, pos_fast_track, err, allowed_values);
    process_err("fast track", conditional_str, err, i);
    return ret;
}

boost::optional<std::pair<int, BCBPSectionsEnums::FreeBaggage>> BCBPSections::free_baggage_allowance(int i)
{  check_i(i);
   std::string err;
   char got = ' ';
   string& str = repeated[i].conditional;
   unsigned int j, ret = 0;
   bool found_num = false;
   for(j = pos_free_baggage_allowance.begin; j < pos_free_baggage_allowance.end; j++)
       if(repeated[i].conditional[j]!=' ') break;
   for(; j < pos_free_baggage_allowance.end; j++)
   { got =  get_char<true>(str, j, err);
     process_err("free baggage allowance", conditional_str, err, i);
     if(!isdigit(got)) break;
     ret *= 10;
     ret += got - '0';
     found_num = true;
   }
   string str_type = get_alfa_chars_str<false>(str, j, pos_free_baggage_allowance.end, err);
   process_err("free baggage allowance", conditional_str, err, i);
   TrimString(str_type);
   if(!found_num && !str_type.empty()) process_err("free baggage allowance", conditional_str, "num not found", i);
   if(str_type == "K") return boost::optional<std::pair<int, BCBPSectionsEnums::FreeBaggage>>(std::make_pair(ret, BCBPSectionsEnums::FreeBaggage::kg));
   if(str_type == "PC") return boost::optional<std::pair<int, BCBPSectionsEnums::FreeBaggage>>(std::make_pair(ret, BCBPSectionsEnums::FreeBaggage::pc));
   if(str_type == "L") return boost::optional<std::pair<int, BCBPSectionsEnums::FreeBaggage>>(std::make_pair(ret, BCBPSectionsEnums::FreeBaggage::pound));
   if(got == ' ' && str_type.empty()) return boost::none; //here
   process_err("free baggage allowance", conditional_str, "wrong type", i);
   return boost::none;
}

std::string BCBPSections::airline_specific()
{   return "";
}

int BCBPSections::num_repeated_sections()
{
    return repeated.size();
}


char BCBPSections::type_of_security_data()
{   std::string err;
    static const string security_str = "security";
    char ret = get_char<true>(unique.security, pos_typeof_sec_data, err);
    process_err("type of security data", security_str, err);
    return ret;
}

std::string BCBPSections::security()
{   std::string err;
    static const string security_str = "security";
    if(pos_sec_data.begin >= unique.security.size()) return ""; //get_alfa_chars_str предполагает, что begin всегда < end
    std::string ret = get_alfa_chars_str<true>(unique.security, pos_sec_data.begin, unique.security.size(), err);
    process_err("security data", security_str, err);
    return ret;
}
