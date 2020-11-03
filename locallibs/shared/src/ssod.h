#ifndef ssodH
#define ssodH

#include <vector>
#include <fstream>
#include "exceptions.h"
#include "oralib.h"

enum SSODFieldType { ssodString, ssodInteger, ssodDate, ssodTime };

class TSSODFile;

class TSSODCacheField
{
  public:
    std::string name;
    SSODFieldType type;
    unsigned int len;
};

class TSSODField:private TSSODCacheField
{
  private:
    char* value;
  public:
    TSSODField(std::string name, SSODFieldType type, int len);
    ~TSSODField();
    std::string Name();
    void Clear();
    void Set(std::string value, bool truncate=false);
    void Set(int value);
    void Set(double value);
    void Print(std::ostream& os);
};

class TSSODRec
{
  private:
    char id;
    std::string version;
    std::vector<TSSODField*> fields;
    TSSODField& FindField(std::string& name);
  public:
    TSSODRec(char id, TSSODFile& f);
    ~TSSODRec();
    std::string Version();
    void ClearField(std::string name);
    void SetField(std::string name, std::string value, bool truncate=false);
    void SetField(std::string name, int value);
    void SetField(std::string name, double value);
    void Print(std::ostream& os);
};

class TSSODTran:public std::vector<TSSODRec*>
{
};

class TSSODCacheRec:public std::vector<TSSODCacheField>
{
  public:
    char id;
};

class TSSODFile
{
  private:
    std::fstream file;
    std::string version;
    TSession* session;
    std::vector<TSSODCacheRec> cache;
    int rows;
  public:
    TSSODFile(std::string name, std::string version, TSession* session);
    ~TSSODFile();
    std::string Version();
    void GetRec(char id, std::vector<TSSODField*>& fields);
    void PrintRec(TSSODRec& rec);
    void PrintTran(TSSODTran& tran);
    int Rows();
};

std::ostream& operator << (std::ostream& os, TSSODField& field);
std::ostream& operator << (std::ostream& os, TSSODRec& rec);
TSSODFile& operator << (TSSODFile& f, TSSODRec& rec);
TSSODFile& operator << (TSSODFile& f, TSSODTran& tran);

#endif
