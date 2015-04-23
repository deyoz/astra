#include "dev_utils.h"
#include "basic.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "misc.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace ASTRA;
using namespace std;
using namespace EXCEPTIONS;

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
};

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
    default: return "";
  };
};



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

  if (IsLetter(str.back()))
  {
    result.second=string(1,str.back());
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
    if (IsLetter(str.back()))
    {
      result.second=string(1,str.back());
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

      if (sections.unique.conditional[0]!='>') throw EConvertError("invalid item 8 <Beginning of version number>");

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
    throw EConvertError("invalid size of bcbp");

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
  for(list<BCBPRepeatedSections>::const_iterator s=sections.repeated.begin(); s!=sections.repeated.end(); ++s, seg++)
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

