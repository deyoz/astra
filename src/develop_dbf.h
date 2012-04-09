#ifndef _DEVELOP_DBF_H_
#define _DEVELOP_DBF_H_

#include <string>
#include <sstream>
#include <vector>

const std::string DBF_TYPE_CHAR = "CHAR";
const std::string DBF_TYPE_DATE = "DATE";
const std::string DBF_TYPE_NUMBER = "NUMBER";
const std::string DBF_TYPE_FLOAT = "FLOAT";

struct TField {
  int idx;
	std::string name;
	char type; //C, L, N, M или F
	int len; // общая длина поля включая запятую
	int precision; // кол-во знаков до запятой
	TField() {
    idx = -1;
	};
};

struct DBFRow {
	bool pr_del;
	bool modify;
	std::vector<std::string> newdata, olddata;
	DBFRow( ) {
		pr_del = 0;
		modify = false;
  };
};

class Develop_dbf
{
private:
  std::string endDBF;
	std::ostringstream descrField, header, data;		
	std::vector<TField> fields;
	std::vector<DBFRow> rows;
	unsigned char version;
	unsigned long rowCount;
	int headerLen;
	int descriptorFieldsLen;
  int recLen;
  void ParseHeader( std::string &indata );
	void BuildHeader();
	void ParseFields( std::string &indata );
	void BuildFields();
	void BuildData( const std::string &encoding );
	void ParseData( std::string &indata, const std::string &encoding );
	void AddRow( DBFRow &row );
	void GetRow( int idx, DBFRow &row );
	int GetFieldIndex( const std::string &FieldName );
public:
	Develop_dbf( );
	void setVersion( unsigned char v );
	void AddField( std::string name, char type, int len, int precision );
	void AddField( std::string name, char type, int len );
	void NewRow( );
	void DeleteRow( int idx, bool pr_del );
	void RollBackRow( int idx );
	void Build( const std::string &encoding );
	void Parse( const std::string &indata, const std::string &encoding );
	std::string GetFieldValue( int idx, const std::string &FieldName );
	void SetFieldValue( int idx, const std::string &FieldName, const std::string &Value );
	bool isModifyRow( int idx );
	int fieldCount( ) {
		return (int)fields.size();
	}
	bool isEmpty() {
		return !rowCount;
	}
	int getRowCount() {
    return rowCount;
	}
	std::string Result( );
};
#endif /*_DEVELOP_DBF_H_*/
