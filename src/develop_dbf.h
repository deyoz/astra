#ifndef _DEVELOP_DBF_H_
#define _DEVELOP_DBF_H_

#include <string>
#include <sstream>
#include <vector>

struct TField {
	std::string name;
	char type; //C, L, N, M или F
	int len; // общая длина поля включая запятую
	int precision; // кол-во знаков до запятой
};

struct DBFRow {
	bool pr_del;
	std::vector<std::string> data;
	DBFRow( ) {
		pr_del = 0;
  }
};

class Develop_dbf
{
private:
	std::ostringstream descrField, header, data;		
	std::vector<TField> fields;
	std::vector<DBFRow> rows;
	unsigned char version;
	unsigned long rowCount;
	int headerLen;
	int descriptorFieldsLen;
  int recLen;
	void BuildHeader();
	void BuildFields();
	void BuildData();
public:
	Develop_dbf( );
	void setVersion( unsigned char v );
	void AddField( std::string name, char type, int len, int precision );
	void AddField( std::string name, char type, int len );
	void AddRow( DBFRow &row );
	void Build( );
	int fieldCount( ) {
		return (int)fields.size();
	}
	bool isEmpty() {
		return !rowCount;
	}
	std::string Result( );
};
#endif /*_DEVELOP_DBF_H_*/
