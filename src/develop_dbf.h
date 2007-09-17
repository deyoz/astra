#ifndef _DEVELOP_DBF_H_
#define _DEVELOP_DBF_H_

#include <string>
#include <sstream>
#include <vector>

const std::string PARAM_CANON_NAME = "CANON_NAME";
const std::string PARAM_WORK_DIR = "WORKDIR";
const std::string PARAM_FILE_NAME = "FileName";
const std::string PARAM_IN_ORDER = "IN_ORDER";	
const std::string PARAM_TYPE = "type";
const std::string PARAM_FILE_TYPE = "FILE_TYPE";	
const std::string VALUE_TYPE_FILE = "FILE";
const std::string VALUE_TYPE_BSM = "BSM";
const std::string NS_PARAM_AIRP = "AIRP";
const std::string NS_PARAM_AIRLINE = "AIRLINE";
const std::string NS_PARAM_FLT_NO = "FLT_NO";	
const std::string PARAM_FILE_REC_NO = "rec_no";


struct TField {
	std::string name;
	char type; //C, L, N, M ��� F
	int len; // ���� ����� ���� ������ �������
	int precision; // ���-�� ������ �� ����⮩
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
