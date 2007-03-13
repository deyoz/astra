#ifndef _DEVELOP_DBF_H_
#define _DEVELOP_DBF_H_

#include <string>
#include <sstream>
#include <vector>

struct TField {
	std::string name;
	char type; //C, L, N, M ��� F
	int len; // ���� ����� ���� ������ �������
	int precision; // ���-�� ������ �� ����⮩
};

struct TRow {
	bool init;
	bool pr_del;
	std::vector<std::string> data;
};

class Develop_dbf
{
private:
	std::ostringstream descrField, header, data;		
	std::vector<TField> fields;
	std::vector<TRow> rows;
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
	void AddRow(  TRow &row );
	void AddData( std::string name, std::string value );
	void Build( );
	std::string Result( );
};
#endif /*_DEVELOP_DBF_H_*/
