#include "dev_utils.h"
#include "exceptions.h"
#include "stl_utils.h"

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

//bcbp_begin_idx - позиция первого байта штрих-кода
//airline_use_begin_idx - позиция первого байта <For individual airline use> первого сегмента
//airline_use_end_idx - позиция, следующая за последним байтом <For individual airline use> первого сегмента
void checkBCBP_M(const string bcbp,
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
