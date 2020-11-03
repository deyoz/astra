#ifndef _BUILDER_H_
#define _BUILDER_H_

#include <string>

enum TPutStyle {psNormal=0x00,psInverse=0x01,psBright=0x02,psBlink=0x03,psProtected=0x80};

class TBuilder
{
  private:
  protected:
    int lenX,lenY,len,pos;
  public:
    TBuilder(int sx, int sy);
    ~TBuilder();
    void Clear();
    int SizeX();
    int SizeY();
    void MoveXY(int x, int y);
    void MoveX(int x);
    void MoveY(int y);
    int GetX();
    int GetY();
    void NextLine();
};

class TScreenBuilder:public TBuilder
{
  private:
    bool IsNormalBlank(int i);
  protected:
    char *c;  //массив символов
    char *f;  //массив состояний
  public:
    TScreenBuilder(int sx, int sy);
    ~TScreenBuilder();
    void Clear();    
    void PutXY(int x, int y, char* s, int l=-1);
    void PutXY(int x, int y, char* s, TPutStyle st, int l=-1);
    void PutX(int x, char* s, int l=-1);
    void PutX(int x, char* s, TPutStyle st, int l=-1);
    void PutY(int y, char* s, int l=-1);
    void PutY(int y, char* s, TPutStyle st, int l=-1);
    void Put(char* s, int l=-1);
    void Put(char* s, TPutStyle st, int l=-1);
    void ClearAllLines();
    void ClearLines(int from=-1, int to=-1);
    void ClearLine();
    std::string Get();
};

class TPrintBuilder:public TBuilder
{
  private:
    bool IsNormalBlank(int i);
  protected:
    char *c;  //массив символов
  public:
    std::string InitStr;
    std::string StrBuf;
    bool AutoPrint;
    bool Optimize;
    bool AutoFormFeed;
    TPrintBuilder(int sx, int sy);
    ~TPrintBuilder();
    void Clear();
    void PutXY(int x, int y, char* s, int l=-1);
    void PutX(int x, char* s, int l=-1);
    void PutY(int y, char* s, int l=-1);
    void Put(char* s, int l=-1);
    void ClearAllLines();
    void ClearLines(int from=-1, int to=-1);
    void ClearLine();
    std::string Get();
};

extern TScreenBuilder Builder;
extern TPrintBuilder PrBuilder;

#endif

