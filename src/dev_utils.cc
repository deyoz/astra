#include "dev_utils.h"

using namespace ASTRA;
using namespace std;

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
