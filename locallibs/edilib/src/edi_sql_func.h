#ifndef _EDI_SQL_FUNC_H_
#define _EDI_SQL_FUNC_H_
#include "edi_tables.h"
#include "edi_sql_insert.h"

int insert_to_ora_from_files(FILE *,FILE *,FILE *,FILE *);
int insert_to_from_file(FILE *fpoint, edi_insert_func ifunc, Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab);

int insert_to_tab_from_mes_file(EDI_TABLES_STRUCT *pTab,FILE *fpoint);
int insert_to_tab_from_seg_file(EDI_TABLES_STRUCT *pTab,FILE *fpoint);
int insert_to_tab_from_comp_file(EDI_TABLES_STRUCT *pTab,FILE *fpoint);
int insert_to_tab_from_data_file(EDI_TABLES_STRUCT *pTab,FILE *fpoint);

/*********************************************************************/
/* �ய�᪠�� ᨬ���� �஡��� � ⠡��樨                            */
/* return 1 - �� Ok                                                 */
/*********************************************************************/
int skiptabs(char **buffer);

/*********************************************************************/
/*       �����頥� � word ᫥��饥 ᫮��                            */
/* return -1 - ���������஢����� ��� �ய�� ��ப�                  */
/* return  0 - ����� ��ப�                                          */
/* return  1 - �� Ok                                                */
/*********************************************************************/
int get_next_word(char **buffer, char *word, int flag);

/*********************************************************************/
/*       �����頥� � text ᨬ���� � ����窠�                        */
/* return 0 - ��㤠�                                                */
/* return 1 - �� Ok                                                 */
/*********************************************************************/
int get_text(char **buffer, char *text);

/************************************************************************/
/* �����뢠�� ��⠭��� ��ப� � ���-��                                 */
/* �������� ��।��� � �㭪�� insert_to_sql()                         */
/* return 0 - ��㤠�                                                   */
/* return 1 - Ok                                                        */
/************************************************************************/
int insert_comm_str_message(Command_Struct *pCommStr, char **bufftmp);

/************************************************************************/
/* �����뢠�� ��⠭��� ��ப� � ���-��                                 */
/* �������� ��।��� � �㭪�� insert_to_sql()                         */
/* return 0 - ��㤠�                                                   */
/* return 1 - Ok                                                        */
/************************************************************************/
int insert_comm_str_segment(Command_Struct *pCommStr, char **bufftmp);

/************************************************************************/
/* �����뢠�� ��⠭��� ��ப� � ���-��                                 */
/* �������� ��।��� � �㭪�� insert_to_sql()                         */
/* return 0 - ��㤠�                                                   */
/* return 1 - Ok                                                        */
/************************************************************************/
int insert_comm_str_composite(Command_Struct *pCommStr, char **bufftmp);

int insert_comm_str_data_element(Command_Struct *pCommStr, char **bufftmp);

void error(const char *filename, const char *errorline);

int CreateFiles();

int insert_help(int num);

int insert_to_tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab);

int insert_to_sql(Command_Struct *pCommStr);
int InsertMess(Command_Struct *pCommStr);
int InsertMesStr(Command_Struct *pCommStr);
int InsertSegment(Command_Struct *pCommStr);
int InsertSegmentStr(Command_Struct *pCommStr);
int InsertComposite(Command_Struct *pCommStr);
int InsertCompositeStr(Command_Struct *pCommStr);
int InsertDataElement(Command_Struct *pCommStr);


int InsertMess2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab);
int InsertMesStr2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab);
int InsertSegment2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab);
int InsertSegmentStr2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab);

int InsertComposite2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab);
int InsertCompositeStr2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab);
int InsertDataElement2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab);

int TestForDigit(char *Str);

int CreateFiles();

int insert_help(int num);

int create_file_name(const char *dir, const char *file, char *res, size_t size);

int open_edi_files(const char *dir, EdiFilePoints *pFp);

void close_edi_files(EdiFilePoints *pFp);

#endif /*_EDI_SQL_FUNC_H_*/
