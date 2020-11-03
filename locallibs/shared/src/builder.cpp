#include <stdlib.h>
#include <string>
#include <algorithm>
#include "builder.h"
#include "misc.h"
#include "exceptions.h"

using namespace EXCEPTIONS;
using namespace std;

TBuilder::TBuilder(int sx, int sy)
{
  lenX=sx;
  lenY=sy;
  len=lenX*lenY;
  if (len<=0) throw Exception("Wrong parameters");
  Clear();
};

TBuilder::~TBuilder()
{
};

void TBuilder::Clear()
{
  pos=0;
};

void TBuilder::MoveXY(int x, int y)
{
  if (x<0||x>=lenX) throw Exception("Wrong X coordinate");
  if (y<0||y>=lenY) throw Exception("Wrong Y coordinate");
  pos=y*lenX+x;
};

void TBuilder::MoveX(int x)
{
  if (x<0||x>=lenX) throw Exception("Wrong X coordinate");
  pos=(pos/lenX)*lenX+x;
};

void TBuilder::MoveY(int y)
{
  if (y<0||y>=lenY) throw Exception("Wrong Y coordinate");
  pos=y*lenX+(pos%lenX);
};

void TBuilder::NextLine()
{
  pos=(pos/lenX+1)*lenX;
  if (pos>=len) pos=0;
};

int TBuilder::SizeX()
{
  return lenX;
};

int TBuilder::SizeY()
{
  return lenY;
};

int TBuilder::GetX()
{
  return pos%lenX;
};

int TBuilder::GetY()
{
  return pos/lenX;
};

TScreenBuilder::TScreenBuilder(int sx, int sy):TBuilder(sx,sy)
{
  c=NULL;
  f=NULL;
  c=(char*)malloc(len);
  f=(char*)malloc(len);
  if (c==NULL||f==NULL) throw EMemoryError("");
  Clear();
};

TScreenBuilder::~TScreenBuilder()
{
  free(c);
  free(f);
};

void TScreenBuilder::Clear()
{
  TBuilder::Clear();
  memset(c,0,len);
  memset(f,0,len); 
};

void TScreenBuilder::PutXY(int x, int y, char* s, int l)
{
  PutXY(x,y,s,psNormal,l);
};

void TScreenBuilder::PutXY(int x, int y, char* s, TPutStyle st, int l)
{
  MoveXY(x,y);
  Put(s,st,l);
};

void TScreenBuilder::PutX(int x, char* s, int l)
{
  PutX(x,s,psNormal,l);
};

void TScreenBuilder::PutX(int x, char* s, TPutStyle st, int l)
{
  MoveX(x);
  Put(s,st,l);
};

void TScreenBuilder::PutY(int y, char* s, int l)
{
  PutY(y,s,psNormal,l);
};

void TScreenBuilder::PutY(int y, char* s, TPutStyle st, int l)
{
  MoveY(y);
  Put(s,st,l);
};

void TScreenBuilder::Put(char* s, int l)
{
  Put(s,psNormal,l);
};

void TScreenBuilder::Put(char* s, TPutStyle st, int l)
{
  TPutStyle cs=st;
  if (s==NULL) s="";
  for(;!(*s==0&&l<0||l==0);)
  {
    if (*s==0&&l>0||*s!=0)
    {
      if (*s==0||*s==0x0D||*s==0x0A)
      {
        c[pos]=' ';
        if (*s==0x0D||*s==0x0A)
        {
          if ((pos+1)%lenX==0)
          {
            if (*s==0x0D&&*(s+1)==0x0A) s+=2; else s++;
          };
        };
      }
      else
      {
      	if(*s=='\x1b')
        {
          switch(*(s+1))
          {
            case '\x36':
              cs=psInverse;
              break;
            case '\x37':
              cs=st;
              break;
            case '\x38':
              cs=psBright;
              break;
            case '\x39':
              cs=st;
              break;
            case '\x3A':
              cs=psBlink;
              break;
            case '\x3B':
              cs=st;
              break;
            case '\x3E':
              cs=psProtected;
              break;
            case '\x3F':
              cs=st;
              break;
          }
          s+=2;
          continue;
        }
        c[pos]=*s;
        s++;
      };
      f[pos]=/*f[pos]&0xFC||*/cs;      
      pos++;
      if (pos>=len) pos=0;
      if (l>0) l--;
    };
  };
};

void TScreenBuilder::ClearAllLines()
{
  memset(c,' ',len);
  memset(f,psNormal,len);
};

void TScreenBuilder::ClearLine()
{
  memset(c+(pos/lenX)*lenX,' ',lenX);
  memset(f+(pos/lenX)*lenX,psNormal,lenX);
};

void TScreenBuilder::ClearLines(int from, int to)
{
  if (from>=lenY||to>=lenY) throw Exception("Wrong line number");
  if (from<0) from=(pos/lenX)*lenX; else from=from*lenX;
  if (to<0) to=(pos/lenX+1)*lenX; else to=(to+1)*lenX;
  if (to>from)
  {
    memset(c+from,' ',to-from);
    memset(f+from,psNormal,to-from);
  };
};

bool TScreenBuilder::IsNormalBlank(int i)
{
  int j;
  bool pr_bprot,pr_eprot;
  if (i==0) j=len-1; else j=i-1;
  pr_bprot=(f[j]&0x80)!=(f[i]&0x80)&&(f[i]&0x80)!=psNormal;
  if (i==len-1) j=0; else j=i+1;
  pr_eprot=(f[j]&0x80)!=(f[i]&0x80)&&(f[i]&0x80)!=psNormal;
  return c[i]==' '&&(f[i]&0x03)==psNormal&&(pr_bprot&&pr_eprot||!pr_bprot&&!pr_eprot);
};

string TScreenBuilder::Get()
{
  int x,y,i,j;
  char ch;
  string buf="",blanks="",move="";
  bool pr_screen_zero=false,pr_line_zero;
  bool pr_screen_blanks=false,pr_line_blanks;
  bool pr_protect=false,pr_bprot,pr_eprot;
  buf+=(char)(pos%lenX+0x20);
  buf+=(char)(pos/lenX+0x20);
  buf+="Y\033";
  for(y=lenY-1;y>=0;y--)
  {
    pr_line_zero=false;
    pr_line_blanks=false;
    for(x=lenX-1;x>=0;x--)
    {
      i=y*lenX+x;
      if ((i==len-1||(f[i+1]&0x03)!=(f[i]&0x03))&&(f[i]&0x03)!=psNormal)
      {
        switch (f[i]&0x03)
        {
          case psInverse: buf+= "7\033"; break;
          case psBright:  buf+= "9\033"; break;
          case psBlink:   buf+= ";\033"; break;
        };
      };
      //начало/конец защищенной зоны
      if (i==0) j=len-1; else j=i-1;
      pr_bprot=(f[j]&0x80)!=(f[i]&0x80)&&(f[i]&0x80)!=psNormal;
      if (i==len-1) j=0; else j=i+1;
      pr_eprot=(f[j]&0x80)!=(f[i]&0x80)&&(f[i]&0x80)!=psNormal;

      if (c[i]!=0)
      {
        if (IsNormalBlank(i))
        {
          blanks+=' ';
          if (i==0||pr_screen_zero&&x==0||!IsNormalBlank(i-1))
          {
            if (!(pr_screen_zero&&pr_line_zero)&&blanks.length()>move.length())
            {
              buf+=move;
              move="";
              pr_screen_blanks=true;
              pr_line_blanks=true;
            }
            else
              buf+=blanks;
            blanks="";
          };
        }
        else
        {
          if (pr_bprot&&pr_eprot||!pr_bprot&&!pr_eprot)
          {
            if ((unsigned char)c[i]>=0x20&&(unsigned char)c[i]<0x80||IsAlpha(c[i]))
            {
              ch=ToUpper(c[i]);
              if (ch=='Ё') ch='Е';
#ifdef __WIN32__
              ch=AsciiToKoi7(ch);
#endif
              buf+=ch;
            }
            else
            {
              switch (c[i])
              {
                case 0x10: buf+="=\033"; break;
                case 0x11: buf+="<\033"; break;
                default:   buf+=' ';
              };
            };
          }
          else
          {
            pr_protect=true;
            if (pr_bprot)
              buf+=">\033";
            else
              buf+="?\033";
          };
        };
        if (i==0||c[i-1]==0)
        {
          if (!pr_screen_zero)
          {
            if (pr_screen_blanks) buf+="J\033";
            pr_screen_zero=true;
          };
        };
        if (i==0||pr_screen_zero&&x==0||c[i-1]==0)
        {
          if (!pr_line_zero)
          {
            if (pr_line_blanks) buf+="K\033";
            pr_line_zero=true;
          };
        };
        if (i==0||pr_screen_zero&&x==0||c[i-1]==0)
        {
          buf+=(char)(x+0x20);
          buf+=(char)(y+0x20);
          buf+="Y\033";
          move="";
        }
        else
        {
          if (!IsNormalBlank(i)&&IsNormalBlank(i-1))
          {
            move=(char)(x+0x20);
            move+=(char)(y+0x20);
            move+="Y\033";
          };
        };
      };
      if ((i==0||(f[i-1]&0x03)!=(f[i]&0x03))&&(f[i]&0x03)!=psNormal)
      {
        switch (f[i]&0x03)
        {
          case psInverse: buf+= "6\033"; break;
          case psBright:  buf+= "8\033"; break;
          case psBlink:   buf+= ":\033"; break;
        };
      };
    };
  };
  reverse(buf.begin(),buf.end());
  if (pr_protect) buf+="\033W";
  return buf;
};

TPrintBuilder::TPrintBuilder(int sx, int sy):TBuilder(sx,sy)
{
  c=NULL;
  c=(char*)malloc(len);
  if (c==NULL) throw EMemoryError("");
  Clear();
};

TPrintBuilder::~TPrintBuilder()
{
  free(c);
};

void TPrintBuilder::Clear()
{
  TBuilder::Clear();
  memset(c,' ',len);
  InitStr="";
  StrBuf="";
  AutoPrint=true;
  Optimize=true;
  AutoFormFeed=true;
};

void TPrintBuilder::PutXY(int x, int y, char* s, int l)
{
  MoveXY(x,y);
  Put(s,l);
};

void TPrintBuilder::PutX(int x, char* s, int l)
{
  MoveX(x);
  Put(s,l);
};

void TPrintBuilder::PutY(int y, char* s, int l)
{
  MoveY(y);
  Put(s,l);
};

void TPrintBuilder::Put(char* s, int l)
{
  if (s==NULL) s="";
  for(;!(*s==0&&l<0||l==0);)
  {
    if (*s==0&&l>0||*s!=0)
    {
      if (*s==0||*s==0x0D||*s==0x0A)
      {
        c[pos]=' ';
        if (*s==0x0D||*s==0x0A)
        {
          if ((pos+1)%lenX==0)
          {
            if (*s==0x0D&&*(s+1)==0x0A) s+=2; else s++;
          };
        };
      }
      else
      {
//        if ((unsigned char)(*s)>=0x20)
          c[pos]=*s;
//        else
//          c[pos]=' ';
        s++;
      };
      pos++;
      if (pos>=len) pos=0;
      if (l>0) l--;
    };
  };
};

void TPrintBuilder::ClearAllLines()
{
  memset(c,' ',len);
};

void TPrintBuilder::ClearLines(int from, int to)
{
  if (from>=lenY||to>=lenY) throw Exception("Wrong line number");
  if (from<0) from=(pos/lenX)*lenX; else from=from*lenX;
  if (to<0) to=(pos/lenX+1)*lenX; else to=(to+1)*lenX;
  if (to>from)
  {
    memset(c+from,' ',to-from);
  };
};

void TPrintBuilder::ClearLine()
{
  memset(c+(pos/lenX)*lenX,' ',lenX);
};

bool TPrintBuilder::IsNormalBlank(int i)
{
  return !((unsigned char)c[i]>0x20&&(unsigned char)c[i]<0x80||IsAlpha(c[i]));
};

string TPrintBuilder::Get()
{
  int x,y,i;
  char ch=0;
  string buf="",blanks="";
  for(i=len-1;i>=0;i--)
    if (!IsNormalBlank(i)) break;
  if (i>=0)
  {
    for(y=i/lenX;y>=0;y--)
    {
      if (ch=='\t') buf+="w\033";
      ch='\n';
      buf+=ch;
      for(x=lenX-1;x>=0;x--)
        if (!IsNormalBlank(y*lenX+x)) break;
      for(;x>=0;x--)
      {
        i=y*lenX+x;
        if (!IsNormalBlank(i))
        {
          if (ch=='\t') buf+="w\033";
          ch=ToUpper(c[i]);
          if (ch=='Ё') ch='Е';
#ifdef __WIN32__
//          ch=AsciiToKoi7(ch);
#endif
          buf+=ch;
        }
        else
        {
          if (Optimize)
          {
            blanks+=' ';
            if (x%8==0||!IsNormalBlank(i-1))
            {
              if ((x+blanks.length())%8==0)
              {
                if (ch!='\t') buf+="x\033";
                ch='\t';
                buf+=ch;
              }
              else
              {
                if (ch=='\t') buf+="w\033";
                ch=' ';
                buf+=blanks;
              };
              blanks="";
            };
          }
          else
          {
            if (ch=='\t') buf+="w\033";
            ch=' ';
            buf+=ch;
          };  
        };
      };
    };
    if (ch=='\t') buf+="w\033";
    reverse(buf.begin(),buf.end());
    if (AutoPrint)
      buf="\033E\033]\033w"+InitStr+"\033x"+StrBuf+buf;
    else
      buf="\033E\033w"+InitStr+"\033x"+StrBuf+buf;
    if (AutoFormFeed)
      buf+="\033w\014\033x\033=";
    else
      buf+="\033=";
  }
  else
  {
    if (StrBuf!="")
    {
      buf="\033]"+StrBuf;
      if (AutoFormFeed)
        buf+="\033w\014\033x\033=";
      else
        buf+="\033=";
    };
  };

  //переведем buf в koi7
  bool pr_protected=false,pr_turn_off=false;
  string::iterator p;
  for(p=buf.begin();p!=buf.end();p++)
  {
    if (*p=='\033')
    {
      if (pr_protected&&pr_turn_off)
      {
        //включить связной контроллер
        p=buf.insert(p,'\033'); p++;
        p=buf.insert(p,'w'); p++;
        pr_turn_off=false;
      }
      p++;
      if (p!=buf.end())
      {
        if (*p=='w') pr_protected=true;
        if (*p=='x') pr_protected=false;
        if (*p=='Y')
        {
          //пропускаем 2 символа
          p++;
          if (p!=buf.end()) p++;
        };
      };
    }
    else
    {
      if (pr_protected)
      {
/*#ifndef __WIN32__
        ch=AsciiToKoi7(*p);
        if (ch!=*p)
        {
          //будет перекодирование на УИСе
          if (Koi7ToAscii(ch)==*p)
          {
            //выключить связной контроллер
//#ifdef __WIN32__
//            p=buf.erase(p);
//#endif
            if (!pr_turn_off)
            {
              p=buf.insert(p,'\033'); p++;
              p=buf.insert(p,'x'); p++;
              pr_turn_off=true;
            };
//#ifdef __WIN32__
//            p=buf.insert(p,ch);
//#endif

          }
          else
          {
//#ifdef __WIN32__
//            p=buf.erase(p);
//            p=buf.insert(p,ch);
//#endif
          };
        }
        else
        {
          //не будет перекодирование на УИСе
          if (pr_turn_off)
          {
            //включить связной контроллер
            p=buf.insert(p,'\033'); p++;
            p=buf.insert(p,'w'); p++;
            pr_turn_off=false;
          };
        };
#endif*/
      }
      else
      {
        if ((unsigned char)*p<0x80||IsAlpha(*p))
        {
          ch=ToUpper(*p);
          if (ch=='Ё') ch='Е';
#ifdef __WIN32__
          ch=AsciiToKoi7(ch);
#endif
          if (*p!=ch)
          {
            p=buf.erase(p);
            p=buf.insert(p,ch);
          };
        }
        else
        {
          p=buf.erase(p);
          p=buf.insert(p,' ');
        };
      };
    };
  };
  return buf;
};


TScreenBuilder Builder(80,24);
TPrintBuilder PrBuilder(80,24);





