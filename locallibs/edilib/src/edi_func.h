#ifndef _EDI_FUNC_H_
#define _EDI_FUNC_H_

#include <stdio.h>
#include <stdarg.h>
#include "edi_sql_insert.h"
#include "edi_tables.h"

int realloc_edi_buff(MesEdiStruct *MesTxt, int need_len);
void free_edi_buff(MesEdiStruct *MesTxt);

/***********************************************************/
/*Считывает в ОП таблицы, описывающие форматы edi-сообщений*/
/*return EDI_READ_BASE_OK - считывание прошло успешно          */
/*return EDI_READ_BASE_ERR - считывание завершено с ошибкой    */
/***********************************************************/
int ReadTablesOra(EDI_TABLES_STRUCT **ppTab);

/***********************************************************/
/*Считывает в ОП файлы, описывающие форматы edi-сообщений*/
/*return EDI_READ_BASE_OK - считывание прошло успешно          */
/*return EDI_READ_BASE_ERR - считывание завершено с ошибкой    */
/***********************************************************/
int ReadTablesFf(const char *directory, EDI_TABLES_STRUCT **ppTab);

int ReadTablesType(EDI_TABLES_STRUCT **ppTab, EdiFilePoints *pFp);

int ReadMesFileTable(EDI_TABLES_STRUCT *pTab,FILE *fp_mes);
int ReadMesTable(EDI_TABLES_STRUCT *pTab);

int ReadMesStrTable(EDI_TABLES_STRUCT *pTab);

int ReadSegFileTable(EDI_TABLES_STRUCT *pTab,FILE *fp_seg);
int ReadSegStrTable(EDI_TABLES_STRUCT *pTab);

int ReadCompFileTable(EDI_TABLES_STRUCT *pTab,FILE *fp_comp);
int ReadCompStrTable(EDI_TABLES_STRUCT *pTab);

int ReadDataFileTable(EDI_TABLES_STRUCT *pTab,FILE *fp_data);
int ReadDataTable(EDI_TABLES_STRUCT *pTab);

int ReadSegTable(EDI_TABLES_STRUCT *pTab);

int ReadCompTable(EDI_TABLES_STRUCT *pTab);

/****************************************************************************/
/* Проверяет правильность ссылок в считанных таблицах                       */
/* Устанавливает ссылки в структурах, содержащих инфу из таблиц             */
/* return EDI_READ_BASE_OK  - все щл                                        */
/* return EDI_READ_BASE_ERR - ошибка в ссылках                              */
/****************************************************************************/
int TestEdiTbl(EDI_TABLES_STRUCT *pTab);

int TestMesStrTbl(MESSAGE_TABLE_STRUCT *MesTab, EDI_TABLES_STRUCT *pTab);

int TestSegStrTbl(MES_STRUCT_TABLE_STRUCT *MesStrTab, EDI_TABLES_STRUCT *pTab);

int TestCompStrTbl(SEG_STRUCT_TABLE_STRUCT *SegStrTab,
                   EDI_TABLES_STRUCT *pTab );

int TestDataStrTbl(DATA_ELEM_TABLE_STRUCT **Point,int DataElem,
                   EDI_TABLES_STRUCT *pTab );

MES_STRUCT_TABLE_STRUCT *GetMesStrByMes(const char *Mes, EDI_TABLES_STRUCT *pTab);

SEG_STRUCT_TABLE_STRUCT *GetSegStrByTag(const char *Tag, EDI_TABLES_STRUCT *pTab);

COMP_STRUCT_TABLE_STRUCT *GetCompStrByComp(const char *Composite,
                                           EDI_TABLES_STRUCT *pTab);

DATA_ELEM_TABLE_STRUCT *GetDataElem(int DataElem, EDI_TABLES_STRUCT *pTab);

const char *GetSegText(const char *Tag, const EDI_TABLES_STRUCT *pTab);

const char *GetCompText(const char *Comp, EDI_TABLES_STRUCT *pTab);

int CreateTemplateMessagesFt(EDI_TABLES_STRUCT   *pEdiTab,
                             EDI_MESSAGES_STRUCT **pTempMes);

EDI_TEMP_MESSAGE *GetEdiTemplateByName(EDI_MESSAGES_STRUCT *pTMes ,const char *MesName);

EDI_TEMP_MESSAGE *GetEdiTemplateByType(EDI_MESSAGES_STRUCT *pTMes, edi_msg_types_t msg_type);

/*****************************************************************************/
/* Выстраивает в памяти шаблоны сообщений;                                   */
/* Удаляет из ОП считанные таблицы - они более не нужны.                     */
/* return EDI_READ_BASE_OK  - все щл                                         */
/* return EDI_READ_BASE_ERR - ошибка                                         */
/*****************************************************************************/
int CreateTemplateMes_( EDI_TABLES_STRUCT *pTab,
                        EDI_MESSAGES_STRUCT **ppIdeMes);

int CreateTempSeg(EDI_TEMP_MESSAGE *wholeTemplate, MESSAGE_TABLE_STRUCT *pMesTab, EDI_TABLES_STRUCT *pTab,
                  EDI_TEMP_MESSAGE *pMes);

int InsertSegGroup(EDI_TEMP_MESSAGE *wholeTemplate,
                   MES_STRUCT_TABLE_STRUCT **ppMesStrTab, TAG_STRUCT *pTagStr);

int CreateTempComp(TAG_STRUCT *pTagStr, MES_STRUCT_TABLE_STRUCT *pMesStrTab);

int CreateTempDataFromSeg(SEG_STRUCT_TABLE_STRUCT *pSegTabStr,
						  COMPOSITE_STRUCT        *pCompStr,
						  COMPOSITE_STRUCT        *pCompRoot  );

int CreateTempData (SEG_STRUCT_TABLE_STRUCT *pSegTabStr,
					COMPOSITE_STRUCT        *pCompStr  );

int TestSegUnique(TAG_STRUCT *pTag, const char *tag);
int TestSegGrUnique(TAG_STRUCT *pTag, int GrpNum);

/****************************************************************************/
/******************Delete functions******************************************/
/****************************************************************************/

void DeleteTags(TAG_STRUCT *pTag);

void DeleteComp(COMPOSITE_STRUCT *pComp);

void DeleteDataElem(DATA_ELEM_STRUCT *pDataElem);

/****************************************************************************/

void DeleteAllEdiTabMessages(EDI_TABLES_STRUCT **ppTab, int delflag);

void DeleteMesTable(EDI_TABLES_STRUCT *pTab, int delflag);

void DeleteMesStrTable(EDI_TABLES_STRUCT *pTab, int delflag);

void DeleteSegStrTable(EDI_TABLES_STRUCT *pTab, int delflag);

void DeleteCompStrTable(EDI_TABLES_STRUCT *pTab, int delflag);

void DeleteDataTable(EDI_TABLES_STRUCT *pTab, int delflag);

void DeleteSegTable ( EDI_TABLES_STRUCT *pTab, int delflag);

void DeleteCompTable( EDI_TABLES_STRUCT *pTab, int delflag);

/***************************************************************************/
/*Удаляет стр-ру целиком*/
void DeleteRealMesStruct(EDI_REAL_MES_STRUCT *pEdiMes);

void DeleteRealMes( EDI_REAL_MES_STRUCT *pEdiMes);

void DelRealTags(TAG_REAL_STRUCT *pTag);

void DelRealComp(COMP_REAL_STRUCT *pComp);

void DelRealDataElem(DATA_REAL_STRUCT *pDataElem);


/****************************************************************************/
/******************Print functions*******************************************/
/****************************************************************************/
void PrintTags(TAG_STRUCT *pTag, int *Vloj, FILE *fl);

void PrintComp(COMPOSITE_STRUCT *pComp, FILE *fl);

void PrintDataElem(DATA_ELEM_STRUCT *pDataElem, FILE *fl);

/****************************************************************************/

void PrintRealTags(TAG_REAL_STRUCT *pTag, int *Vloj, FILE *fl);

void PrintRealComp(COMP_REAL_STRUCT *pComp, FILE *fl);

void PrintRealDataElem(DATA_REAL_STRUCT *pData, FILE *fl);

void PrintMesStrTabMes(MES_STRUCT_TABLE_STRUCT *MesStrTab, FILE *fl);

/************************************************************************/
/***********************SQL FUNCTION*************************************/
/************************************************************************/
int InsertMesTable(MESSAGE_TABLE_STRUCT *pMesTable, int *flag, short ind[]);
int InsertMesStrTable(MES_STRUCT_TABLE_STRUCT *pMesStrTable, int *flag, short ind[]);
int InsertSegStrTable(SEG_STRUCT_TABLE_STRUCT *pSegStrTable, int *flag, short ind[]);
int InsertCompStrTable(COMP_STRUCT_TABLE_STRUCT *pCompStrTable, int *flag, short ind[]);
int InsertDataElemTable(DATA_ELEM_TABLE_STRUCT *pDataTable, int *flag, short ind[]);
int InsertSegTable(SEGMENT_TABLE_STRUCT *pSegTable, int *flag, short ind[]);
int InsertCompTable(COMPOSITE_TABLE_STRUCT *pCompTable, int *flag, short ind[]);
int TestInd(short ind[], short n, const char *NameTable);

/****************************************************************************/
/****** Функции считывания в память сообщений *******************************/
/****************************************************************************/
int GetMesNameCharSet(EDI_REAL_MES_STRUCT *ppCurMes,EDI_MESSAGES_STRUCT *pMes,
					  const char *Mes, int len, char *MesName, short check);

int FillEdiMesStruct(const char *MesName, EDI_MESSAGES_STRUCT *pTMes,
					 const char *Mes, int len, EDI_REAL_MES_STRUCT *pCurMes);

int InsertCurMes(const char *MesName, EDI_MESSAGES_STRUCT *pMes,
                 const char *Mes    , EDI_REAL_MES_STRUCT *pCurMes);

int GetCurMes(char *Mes, EDI_TEMP_MESSAGE *pMes,
              EDI_REAL_MES_STRUCT *pCurMes);

int InsertSegGr (EDI_REAL_MES_STRUCT *pCurMes, char **Mes, char **MesTmp,
                 TAG_STRUCT *pTag, TAG_REAL_STRUCT *pCurTag, int array);

int InsertSegArr(EDI_REAL_MES_STRUCT *pCurMes, char **Mes, char **MesTmp,
                 TAG_STRUCT *pTag, TAG_REAL_STRUCT *pCurTag);

int InsertSeg(EDI_REAL_MES_STRUCT *pCurMes, char *Mes,
              TAG_STRUCT *pTag, TAG_REAL_STRUCT *pCurTag);

int InsertCompArr(EDI_REAL_MES_STRUCT *pCurMes, COMP_REAL_STRUCT *pCurComp,
                  COMPOSITE_STRUCT *pComp, char **Mes, char **MesTmp);

int InsertComp(EDI_REAL_MES_STRUCT *pCurMes, COMP_REAL_STRUCT *pCurComp,
               COMPOSITE_STRUCT *pComp, char **Mes);

int InsertDataArr(EDI_REAL_MES_STRUCT *pCurMes, DATA_REAL_STRUCT *pCurData,
                  DATA_ELEM_STRUCT *pData, char **Mes, char **MesTmp, int dataflag);

Edi_CharSet *GetCharSetByType(CharSetType type);
/*Проверяет, не началaсь ли данная сегм. группа сначала*/
int ProbeCurentSegGr(EDI_REAL_MES_STRUCT *pCurMes, char *SegGrName, char **Mes, char **MesTmp,
                     TAG_REAL_STRUCT *pCurTag, TAG_STRUCT *pTagTmp);

int SkipCompArr(EDI_REAL_MES_STRUCT *pCurMes, COMPOSITE_STRUCT *pComp,
                char **Mes, char **MesTmp, int sch);

int SkipDataArr(EDI_REAL_MES_STRUCT *pCurMes, DATA_ELEM_STRUCT *pData,
                char **Mes, char **MesTmp, int sch, int dataflag);

int CountEdiComp(COMPOSITE_STRUCT *pComp, const char *comp);
int CountEdiData(DATA_ELEM_STRUCT *pData, int data);
int CountEdiSingleData(COMPOSITE_STRUCT *pComp, int data);


int GetNextSeg(EDI_REAL_MES_STRUCT *pCurMes, char **Mes, char **MesTmp);
const char * GetNextSegRO(EDI_REAL_MES_STRUCT *pCurMes, const char *Mes, short check);
const char * GetNextCompRO(EDI_REAL_MES_STRUCT *pCurMes, const char *Mes);
const char * GetNextDataRO(EDI_REAL_MES_STRUCT *pCurMes, const char *Mes);


int GetNextComp_Data(EDI_REAL_MES_STRUCT *pCurMes, char **Mes,
                     char **MesTmp, const char Separator, const char Release);

int CheckTagsGet(EDI_REAL_MES_STRUCT *pCurMes, TAG_STRUCT *pTag);
int CheckTagsInsert(EDI_REAL_MES_STRUCT *pCurMes, TAG_STRUCT *pTag);
int CheckTags_(EDI_REAL_MES_STRUCT *pCurMes, TAG_STRUCT *pTag, int get);

int CheckOneCompGet(EDI_REAL_MES_STRUCT *pCurMes, COMPOSITE_STRUCT *pComp);
int CheckOneCompInsert(EDI_REAL_MES_STRUCT *pCurMes, COMPOSITE_STRUCT *pComp);
int CheckOneComp_(EDI_REAL_MES_STRUCT *pCurMes, COMPOSITE_STRUCT *pComp, int get);

int CheckCompInsert(EDI_REAL_MES_STRUCT *pCurMes, COMPOSITE_STRUCT *pComp);
int CheckCompGet(EDI_REAL_MES_STRUCT *pCurMes, COMPOSITE_STRUCT *pComp);
int CheckComp_(EDI_REAL_MES_STRUCT *pCurMes, COMPOSITE_STRUCT *pComp, int get);

int CheckOneDataGet(EDI_REAL_MES_STRUCT *pCurMes, DATA_ELEM_STRUCT *pData);
int CheckOneDataInsert(EDI_REAL_MES_STRUCT *pCurMes, DATA_ELEM_STRUCT *pData);
int CheckOneData_(EDI_REAL_MES_STRUCT *pCurMes, DATA_ELEM_STRUCT *pData, int get);

int CheckDataGet(EDI_REAL_MES_STRUCT *pCurMes, DATA_ELEM_STRUCT *pData);
int CheckDataInsert(EDI_REAL_MES_STRUCT *pCurMes, DATA_ELEM_STRUCT *pData);
int CheckData_(EDI_REAL_MES_STRUCT *pCurMes, DATA_ELEM_STRUCT *pData, int get);

int CreateNewTagReal(TAG_REAL_STRUCT **pCurTag);
int CreateNewCompReal(COMP_REAL_STRUCT **pComp);
int CreateNewDataReal(DATA_REAL_STRUCT **pData);

int CheckDataByRuleGet(EDI_REAL_MES_STRUCT *pMes, DATA_ELEM_STRUCT *pData, DATA_REAL_STRUCT *pRData);
int CheckDataByRuleInsert(EDI_REAL_MES_STRUCT *pMes, DATA_ELEM_STRUCT *pData, DATA_REAL_STRUCT *pRData);
int CheckDataByRule_(EDI_REAL_MES_STRUCT *pMes, DATA_ELEM_STRUCT *pData,
                     DATA_REAL_STRUCT *pRData, int get);

int CheckAlphabetic(const char *Mes, int len);
int CheckNumeric(const char *Mes, int len);
int CheckForBadChars(const char *Mes, char Release, char EndData);

int TemplateTagByName(const char *Mes, TAG_STRUCT *pTag);
/****************************************************************************/
int FindInTag(Find_Data *pFind, TAG_REAL_STRUCT *pTag);

int FindIn1Tag(Find_Data *pFind, TAG_REAL_STRUCT *pTag);

int FindInComp(Find_Data *pFind, COMP_REAL_STRUCT *pComp);

int FindIn1Comp(Find_Data *pFind, COMP_REAL_STRUCT *pComp);

int FindInData(Find_Data *pFind, DATA_REAL_STRUCT *pData);

/*****************************************************/
/*Сохраняет найденный эл данных в pFind              */
/*return 1 - Ok                                      */
/*return 0 - error allocated memory                  */
/*****************************************************/
int SaveData(Find_Data *pFind, void *pData);

int Resize_pFind(Find_Data *pFind);

/************************************************************/
/*                 Поиск композита                          */
/* return -1 - програмная ошибка                            */
/* return  0 - ничего не нашли                              */
/* return  1 - OK                                           */
/************************************************************/
int FindComp_(EDI_REAL_MES_STRUCT *pMes, const char *Comp, int FindReset);

int FindCompInTag(Find_Data *pFind, TAG_REAL_STRUCT *pTag);

int FindCompIn1Tag(Find_Data *pFind, TAG_REAL_STRUCT *pTag);

int FindCompInComp(Find_Data *pFind, COMP_REAL_STRUCT *pComp);

/**********************************************************/
/*Возвращает указатель на найденный элемент (группу       */
/* сегментов, сегмент, композит, эл. данных ) по номеру   */
/*return void * - указатель на элемент                    */
/**********************************************************/
void *GetFoundByNum(EDI_REAL_MES_STRUCT *pMes, int Num);

const char *GetDBN__(EDI_REAL_MES_STRUCT *pMes, int data, int NumSeq, int zero);

const char *GetDBNum__(EDI_REAL_MES_STRUCT *pMes, int data, int Num_d, int NumSeq, int zero);

/************************************************************/
/*                 Поиск сегмента                           */
/* return -1 - програмная ошибка                            */
/* return  0 - ничего не нашли                              */
/* return  1 - OK                                           */
/************************************************************/
int FindSegm_(EDI_REAL_MES_STRUCT *pMes, const char *Segm, int FindReset);

int FindSegmInTag(Find_Data *pFind, TAG_REAL_STRUCT *pTag);

/************************************************************/
/*             Поиск сегментной группы                      */
/* return -1 - програмная ошибка                            */
/* return  0 - ничего не нашли                              */
/* return  1 - OK                                           */
/************************************************************/
int FindSegGr(EDI_REAL_MES_STRUCT *pMes, short SegGr, int FindReset);

int FindSegGrInTag(Find_Data *pFind, TAG_REAL_STRUCT *pTag);

int GetMesType();



/**************** Составление сообщений *******************/
int SetSpacesToEdiMessage_(EDI_REAL_MES_STRUCT *pMes);

int CheckEdiTags(EDI_REAL_MES_STRUCT *pCurMes, TAG_REAL_STRUCT *Tag ,TAG_STRUCT *TagTemp, int set, int arr);
int CheckEdiComposites(EDI_REAL_MES_STRUCT *pCurMes, COMP_REAL_STRUCT **ppComp, COMPOSITE_STRUCT *CompTemp, int set, int arr);
int CheckEdiDataElems(EDI_REAL_MES_STRUCT *pCurMes, DATA_REAL_STRUCT **ppDataElem, DATA_ELEM_STRUCT *DataTemp, int set, int arr);

int WriteEdiTags(EDI_REAL_MES_STRUCT *pCurMes, TAG_REAL_STRUCT *pTags, MesEdiStruct *MesTxt);
int WriteEdiComp(EDI_REAL_MES_STRUCT *pCurMes, COMP_REAL_STRUCT *pComp, MesEdiStruct *MesTxt);
int WriteEdiData(EDI_REAL_MES_STRUCT *pCurMes, DATA_REAL_STRUCT *pData, MesEdiStruct *MesTxt, int first, int single);
const char *GetEdiDataStr(EDI_REAL_MES_STRUCT *pM, const char *format, va_list *ap);

int CountEdiSegments_(EDI_REAL_MES_STRUCT *pMes);
int CountEdiTags(TAG_REAL_STRUCT *pTag, int *count);
int WriteEdiMessage__(EDI_REAL_MES_STRUCT *pMes, MesEdiStruct *MesTxt);

/*************** edi_err.c *********************/
void ResetEdiErr();
void ResetEdiErr_(EDI_REAL_MES_STRUCT *pMes);

void SetEdiErrDataElemArr(int DataElem, int num);
void SetEdiErrDataElem(int DataElem);
void SetEdiErrDataElem_(EDI_REAL_MES_STRUCT *pMes, int DataElem, int num);

void SetEdiErrCompArr(const char *Comp, int num);
void SetEdiErrCompArr_(EDI_REAL_MES_STRUCT *pMes, const char *Comp, int num);
void SetEdiErrComp(const char *Comp);
void SetEdiErrComp_(EDI_REAL_MES_STRUCT *pMes, const char *Comp);
void SetEdiErrComp__(EDI_REAL_MES_STRUCT *pMes, const char *Comp, int num);


void SetEdiErrSegmArr(const char *Segm, int num);
void SetEdiErrSegmArr_(EDI_REAL_MES_STRUCT *pMes, const char *Segm, int num);
void SetEdiErrSegm(const char *Segm);
void SetEdiErrSegm_(EDI_REAL_MES_STRUCT *pMes, const char *Segm);
void SetEdiErrSegm__(EDI_REAL_MES_STRUCT *pMes, const char *Segm, int num);

void SetEdiErrSegGr(int SegGr, int num);
void SetEdiErrSegGr_(EDI_REAL_MES_STRUCT *pMes, int SegGr, int num);

void SetEdiErrNum(unsigned num);
void SetEdiErrNum_(EDI_REAL_MES_STRUCT *pMes, unsigned num);

void SetEdiErrNum(unsigned num);
void SetEdiErrNum_(EDI_REAL_MES_STRUCT *pMes, unsigned num);

/* Counter functions */
void IncEdiSegment(EDI_REAL_MES_STRUCT *pMes);
void IncEdiComposite(EDI_REAL_MES_STRUCT *pMes);
void IncEdiDataElem(EDI_REAL_MES_STRUCT *pMes);

/**********************************
Удаляет спец символы edifact из текста
**********************************/
int DeleteEdiMaskChars2(EDI_REAL_MES_STRUCT *pMes, DATA_REAL_STRUCT *pData);
int DeleteEdiMaskChars(const Edi_CharSet *Chars, char *str, int *len_);

/**********************************
Расставляет спец символы
**********************************/
int InsertEdiMaskChars2(EDI_REAL_MES_STRUCT *pMes, DATA_REAL_STRUCT *pData);

int SetCharSetOnType(EDI_REAL_MES_STRUCT *pMes, const char *s, CharSetType chstype);

const char *GetEdiCharSetByName(EDI_MESSAGES_STRUCT *pMesTemp, const char *chname, short check);

const char *GetCharSetNameFromText(const char *text, int len, short check);

int ChangeEdiCharSet_(const char *src, size_t srclen,
                      char **dest, size_t *destlen,
                      Edi_CharSet *CharSrc, Edi_CharSet *CharDest);

/* Печатает текущую область видимости */
void PrintEdiPoint(EDI_REAL_MES_STRUCT *pMes, char *str, size_t size);

#endif /*_EDI_FUNC_H_*/
