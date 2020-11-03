#include "stl_utils.h"
#include <deque>
#include <algorithm>
#include <iomanip>
#include <math.h>
#include "misc.h"
#include "exceptions.h"
#include "misc.h"
#include <iconv.h>

using namespace std;
using namespace EXCEPTIONS;

string& RTrimString(string& value)
{
  string::iterator p;
  for(p=value.end();p!=value.begin()&&*(p-1)>=0&&*(p-1)<=' ';p--);
  value.erase(p,value.end());
  return value;
};

string& LTrimString(string& value)
{
  string::iterator p;
  for(p=value.begin();p!=value.end()&&*p>=0&&*p<=' ';p++);
  value.erase(value.begin(),p);
  return value;
};

string& TrimString(string& value)
{
  RTrimString(value);
  LTrimString(value);
  return value;
};

string& NormalizeString(string& str)
{
  string::iterator pcur,pnext;
  char prev=0,next=0;
  for(pcur=str.begin();pcur!=str.end();)
  {
    if (*pcur>0&&*pcur<=' ')
    {
      pnext=pcur+1;
      if (pnext!=str.end())
        next=*pnext;
      else
        next=0;
      if (next>0&&next<=' ') next=' ';
      if (next==0||prev==0||
          strchr("-/;,.:) ",next)!=NULL||
          strchr("-/(",prev)!=NULL)
      {
        pcur=str.erase(pcur);
        continue;
      }
      else *pcur=' ';
    };
    prev=*pcur;
    pcur++;
  };
  return str;
};

void SeparateString(const char* str, int len, vector<string>& strs)
{
  strs.clear();
  if (len<=0||str==NULL) return;
  string strh;
  char *p,*pcur,*pnext,*pend;
  p=(char*)str;
  pend=p+strlen(p);
  while ((pcur=p+len)<pend)
  {
    do
    {
      pnext=pcur;
      pcur--;
      if (*pnext>0&&*pnext<=' '&&*pcur!='.')
      {
        strs.push_back(strh.assign(p,pnext-p));
        p=pnext+1;
        break;
      };
      if (strchr(";,:",*pcur)!=NULL||
          *pcur==')'&&
          !IsDigitIsLetter(*pnext))
      {
        strs.push_back(strh.assign(p,pnext-p));
        p=pnext;
        break;
      };
    }
    while (pcur!=p);
    if (pcur==p)
    {
      strs.push_back(strh.assign(p,len));
      p+=len;
    };
  };
  if (p<pend) strs.push_back(p);  //записать остаток
};

void SeparateString(string str, char separator, vector<string>& strs)
{
  strs.clear();
  if (str.empty()) return;
  string::size_type pos;
  while((pos=str.find_first_of(separator))!=string::npos)
  {
    strs.push_back(str.substr(0,pos));
    str.erase(0,pos+1);
  };
  strs.push_back(str);
};

const char *HexChars="0123456789ABCDEF";

void StringToHex(const string& src, string& dest)
{

  unsigned char c;
  string::const_iterator i;
  dest="";
  for(i=src.begin();i!=src.end();i++)
  {
    c=*i;
    dest.append(1,HexChars[c/16]);
    dest.append(1,HexChars[c%16]);
  };
};

bool HexToString(const string& src, string& dest)
{

  unsigned char c;
  const char *p;
  string::const_iterator i;
  dest="";
  for(i=src.begin();i!=src.end();i++)
  {
    if ((p=strchr(HexChars,*i))==NULL) return false;
    c=(p-HexChars)*16;
    i++;
    if (i==src.end()) return false;
    if ((p=strchr(HexChars,*i))==NULL) return false;
    c+=(p-HexChars);
    dest.append(1,c);
  };
  return true;
};

string upperc(const string &value) {
    string result = value;
    transform(result.begin(), result.end(), result.begin(), ToUpper);
    return result;
}

string lowerc(const string &value) {
    string result = value;
    transform(result.begin(), result.end(), result.begin(), ToLower);
    return result;
}

bool IsAscii7(const string &value)
{
  for(string::const_iterator i=value.begin(); i!=value.end(); ++i)
    if (!IsAscii7(*i)) return false;
  return true;
};

string IntToString(int val)
{
    ostringstream buf;
    buf << val;
    return buf.str();
}

string FloatToString( double val, int precision )
{
   ostringstream buf;
   buf.setf(ios::fixed);
   if (precision>=0) buf << std::setprecision(precision);
   buf << val;
   return buf.str();
}


int ToInt(const string val)
{
  int i;
  if (StrToInt(val.c_str(),i)==EOF)
    throw EConvertError("ToInt: can't convert %s to int",val.c_str());
  return i;
};

string ConvertCodepage(const string& str,
                       const string& fromCp,
                       const string& toCp)
{
  static char* icv_buf=NULL;
  static size_t icv_buflen=0;

  char *inbuf,*outbuf,*ph;
  size_t inlen,outlen;
  iconv_t icv;

  string result;
  if (str.empty()) return result;
  icv = iconv_open(toCp.c_str(),fromCp.c_str());
  if (icv == (iconv_t)-1)
    throw EConvertError("ConvertCodepage: Coding handler from %s to %s is not specified",fromCp.c_str(),toCp.c_str());
  try
  {
    inbuf=(char*)str.c_str();
    inlen=str.size();
    outlen=inlen*2;
    if (icv_buf==NULL || icv_buflen<outlen+1)
    {
      if (icv_buflen==0)
        ph=(char*)malloc(outlen+1);
      else
        ph=(char*)realloc(icv_buf,outlen+1);
      if (ph!=NULL)
      {
        icv_buf=ph;
        icv_buflen=outlen+1;
      };
    };
    outbuf=icv_buf;
    outlen=icv_buflen-1;

    if (iconv(icv,&inbuf,&inlen,&outbuf,&outlen)==size_t(-1))
      throw EConvertError("ConvertCodepage: Can't convert form %s to %s",fromCp.c_str(),toCp.c_str());
    *outbuf=0;
    result.assign(icv_buf,outbuf-icv_buf);

    iconv_close(icv);
    return result;
  }
  catch(...)
  {
    iconv_close(icv);
    throw;
  };

};

string::size_type EditDistance(const std::string &s1, const std::string &s2)
{
  //LevenshteinDistance
  deque<string::size_type> D;
  D.push_back(0);
  bool z1=true;
  for(string::const_iterator i=s1.begin(); z1 || i!=s1.end(); z1=false)
  {
    bool z2=true;
    for(string::const_iterator j=s2.begin(); z2 || j!=s2.end(); z2=false)
    {
      //z1=true нет диагонального и следующего за диагональным (front + front)
      //z2=true нет диагонального и предыдущего перед вычисляемым (front + back)
      //z1=true && z2=true нет ничего
      if (z1)
      {
        if (z2)
          D.push_back(0);
        else
          D.push_back(D.back()+1);
      }
      else
      {
        if (z2)
        {
          D.pop_front();
          D.push_back(D.front()+1);
        }
        else
        {
          string::size_type diag=D.front();
          D.pop_front();
          D.push_back( min(min(diag+(*i==*j?0:1), D.front()+1), D.back()+1));
        };
      };

//      for(deque<string::size_type>::const_iterator d=D.begin(); d!=D.end(); ++d)
//        printf("%zu", *d);
//      printf("\n");

      if (!z2) ++j;
    }
    if (!z1) ++i;
  }

  return D.back();
}

int EditDistanceSimilarity(const std::string &s1, const std::string &s2)
{
  string::size_type max_size=max(s1.size(), s2.size());
  if (max_size==0) return 100;
  return (int)(round(100-EditDistance(s1, s2)*100.0/max_size));
}


