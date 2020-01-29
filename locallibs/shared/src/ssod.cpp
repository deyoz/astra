#include <algorithm>
#include <functional>
#include "ssod.h"

#include "date_time.h"

using namespace BASIC::date_time;

using namespace EXCEPTIONS;
using namespace std;

TSSODField::TSSODField(string name, SSODFieldType type, int len)
{
  if ((type==ssodDate&&len!=5&&len!=6&&len!=8)||
      (type==ssodTime&&len!=4))
    throw Exception("Wrong field length");
  this->name=name;
  this->type=type;
  this->len=len;
  this->value=(char*)malloc(len+1);
  if (this->value==NULL) throw EMemoryError("Out of memory");
  Clear();
};

TSSODField::~TSSODField()
{
  free(this->value);
};

string TSSODField::Name()
{
  return name;
};

void TSSODField::Clear()
{
  switch(type)
  {
    case ssodString:
      sprintf(value,"%-*s",len,"");
      break;
    case ssodInteger:
    case ssodDate:
    case ssodTime:
      sprintf(value,"%0*d",len,0);
      break;
  };
};

void TSSODField::Set(string value, bool truncate)
{
  if (type!=ssodString) throw Exception("Uncompatible data types: " + Name());
  if (value.size()>len)
  {
    if (truncate)
      value.erase(len);
    else
      throw Exception("Value too long: " + Name());
  };
  sprintf(this->value,"%-*s",len,value.c_str());
};

void TSSODField::Set(int value)
{
  char buf[20];
  if (type!=ssodInteger) throw Exception("Uncompatible data types: " + Name());
  sprintf(buf,"%d",value);
  if (strlen(buf)>len)
    throw Exception("Value too long: " + Name());
  sprintf(this->value,"%0*d",len,value);
};

void TSSODField::Set(double value)
{
  switch(type)
  {
    case ssodInteger:
      char buf[20];
      sprintf(buf,"%.0f",value);
      if (strlen(buf)>len) throw Exception("Value too long: " + Name());
      sprintf(this->value,"%0*.0f",len,value);
      break;
    case ssodDate:
      switch(len)
      {
        case 5:
          DateTimeToStr(value,"ddmmm",this->value);
          break;
        case 6:
          DateTimeToStr(value,"yymmdd",this->value);
          break;
        case 8:
          DateTimeToStr(value,"yyyymmdd",this->value);
          break;
      };
      break;
    case ssodTime:
      DateTimeToStr(value,"hhnn",this->value);
      break;
    default:
      throw Exception("Uncompatible data types: " + Name());
  };
};

void TSSODField::Print(ostream& os)
{
  os<<value;
};

ostream& operator << (ostream& os, TSSODField& field)
{
  field.Print(os);
  return os;
};

TSSODRec::TSSODRec(char id, TSSODFile& f)
{
  this->id=id;
  this->version=f.Version();
  f.GetRec(id,fields);
};

TSSODRec::~TSSODRec()
{
  vector<TSSODField*>::iterator i;
  for(i=fields.begin();i!=fields.end();i++) delete *i;
};

string TSSODRec::Version()
{
  return version;
};

bool operator != (TSSODField* field, string name)
{
  return field->Name()!=name;
};

bool operator == (TSSODField* field, string name)
{
  return field->Name()==name;
};

TSSODField& TSSODRec::FindField(string& name)
{
  vector<TSSODField*>::iterator i;
  i=find(fields.begin(),fields.end(),name);
  if (i==fields.end()) throw Exception("Field '%s' not found",name.c_str());
  return **i;
};

void TSSODRec::ClearField(string name)
{
  FindField(name).Clear();
};

void TSSODRec::SetField(string name, string value, bool truncate)
{
  FindField(name).Set(value,truncate);
};

void TSSODRec::SetField(string name, int value)
{
  FindField(name).Set(value);
};

void TSSODRec::SetField(string name, double value)
{
  FindField(name).Set(value);
};

void TSSODRec::Print(ostream& os)
{
  vector<TSSODField*>::iterator i;
  for(i=fields.begin();i!=fields.end();i++) os<<**i;
  os<<endl;

/* for_each(fields.begin(),fields.end(),
           bind2nd(mem_fun_ref(&TSSODField::Print),os));*/
};

ostream& operator << (ostream& os, TSSODRec& rec)
{
  rec.Print(os);
  return os;
};

TSSODFile::TSSODFile(string name, string version, TSession* session)
{
  if (session==NULL) throw Exception("Session undefined");
  this->version=version;
  this->session=session;
  this->rows=0;
  file.open(name.c_str(),ios_base::out);
};

TSSODFile::~TSSODFile()
{
  file.close();
};

string TSSODFile::Version()
{
  return version;
};

bool operator < (const TSSODCacheRec& rec1, const TSSODCacheRec& rec2)
{
  return rec1.id<rec2.id;
};

void TSSODFile::GetRec(char id, vector<TSSODField*>& fields)
{
  vector<TSSODCacheRec>::iterator i;
  TSSODCacheRec rec;
  rec.id=id;
  i=lower_bound(cache.begin(),cache.end(),rec);
  if (i==cache.end()||i->id!=id)
  {
    //считать из базы и вставить запись в кэш
    char rec_id[2]={id,0};
    TSSODCacheField field;
    int next_pos=1;
    TQuery Qry(session);
    Qry.SQLText=
      "SELECT name,type,length,position FROM ssod_fields\
       WHERE version=:version AND rec_id=:rec_id ORDER BY position";
    Qry.CreateVariable("version",otString,version);
    Qry.CreateVariable("rec_id",otString,rec_id);
    Qry.Execute();
    while(!Qry.Eof)
    {
      if (Qry.FieldAsInteger("position")!=next_pos||next_pos>=256)
        throw Exception("Wrong record format (id=%s)",rec_id);
      field.name=Qry.FieldAsString("name");
      switch(Qry.FieldAsString("type")[0])
      {
        case 'N': field.type=ssodInteger; break;
        case 'D': field.type=ssodDate; break;
        case 'T': field.type=ssodTime; break;
        default : field.type=ssodString; break;
      };
      field.len=Qry.FieldAsInteger("length");
      rec.push_back(field);
      next_pos+=field.len;
      Qry.Next();
    };
    if (next_pos!=256) throw Exception("Wrong record format (id=%s)",rec_id);
    i=cache.insert(i,rec);
  };
  fields.clear();
  TSSODCacheRec::iterator iField;
  TSSODField *field;
  for(iField=i->begin();iField!=i->end();iField++)
  {
    field= new TSSODField(iField->name,iField->type,iField->len);
    fields.push_back(field);
  };
};

void TSSODFile::PrintRec(TSSODRec& rec)
{
  if (version!=rec.Version()) throw Exception("Different SSOD versions");
  file<<rec;
  rows++;
};

TSSODFile& operator << (TSSODFile& f, TSSODRec& rec)
{
  f.PrintRec(rec);
  return f;
};

void TSSODFile::PrintTran(TSSODTran& tran)
{
  TSSODTran::iterator i;
  for(i=tran.begin();i!=tran.end();i++)
  {
    if (version!=(*i)->Version()) throw Exception("Different SSOD versions");
    file<<**i;
    rows++;
  };
};

TSSODFile& operator << (TSSODFile& f, TSSODTran& tran)
{
  f.PrintTran(tran);
  return f;
};

int TSSODFile::Rows()
{
  return rows;
};

