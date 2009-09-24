#include "dev_utils.h"
#include <string.h>

using namespace ASTRA;
using namespace std;

TDevOperType DecodeDevOperType(const char *s)
{
  unsigned int i;
  for(i=0;i<sizeof(TDevOperTypeS)/sizeof(TDevOperTypeS[0]);i+=1) if (strcmp(s,TDevOperTypeS[i])==0) break;
  if (i<sizeof(TDevOperTypeS)/sizeof(TDevOperTypeS[0]))
    return (TDevOperType)i;
  else
    return dotUnknown;
}

char* EncodeDevOperType(TDevOperType s)
{
  return (char*)TDevOperTypeS[s];
};

TDevFmtType DecodeDevFmtType(const char *s)
{
  unsigned int i;
  for(i=0;i<sizeof(TDevFmtTypeS)/sizeof(TDevFmtTypeS[0]);i+=1) if (strcmp(s,TDevFmtTypeS[i])==0) break;
  if (i<sizeof(TDevFmtTypeS)/sizeof(TDevFmtTypeS[0]))
    return (TDevFmtType)i;
  else
    return dftUnknown;
}

char* EncodeDevFmtType(TDevFmtType s)
{
  return (char*)TDevFmtTypeS[s];
};
