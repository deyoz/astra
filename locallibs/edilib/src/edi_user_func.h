#ifndef _EDI_USER_FUNC_H_
#define _EDI_USER_FUNC_H_
#include <stdarg.h>
#include <stdio.h>
#include <memory>

#include "edi_types.h"
#include "edi_logger.h"

#define EDI_MES_OK             0 /*Все Щл*/
#define EDI_MES_NOT_FND       -1 /*Данное сообщение отсутствует в базе или просто чего-то нет*/
#define EDI_MES_ERR           -2 /*Общая ошибка (програмная)*/
#define EDI_MES_STRUCT_ERR    -3 /*Ошибка в структуре сообщения*/
#define EDI_MES_TYPES_ERR     -4 /*Ошибка работы с типами*/

enum EDI_MES_LANG {
	EDI_ENGLISH=0,
	EDI_RUSSIAN=1
};

/**
 * init edilib call back register. Called on first access to GetEdiTemplateMessages
 */
void RegEdilibInit(int(*init_edilib_)(void));

/******************************************************************/
/* Ассоциирует программные типы сообщений со считанными шаблонами */
/* return 0 - Ok                                                  */
/* return -1- Error                                               */
/******************************************************************/
int InitEdiTypes(EDI_MSG_TYPE *types, size_t num);
int InitEdiTypes_(EDI_MESSAGES_STRUCT *pMesTemp,
		  EDI_MSG_TYPE *types, size_t num);


/*******************************************************************/
/* Загружает набор управляющих символов,                           */
/* привязывая их к конкретному шаблону.                            */
/* Если функция не выполнена, будут использов. символы по умолчан. */
/*******************************************************************/
int InitEdiCharSet(const edi_loaded_char_sets *char_set, size_t num);
int InitEdiCharSet_(EDI_MESSAGES_STRUCT *pMesTemp,
                    const edi_loaded_char_sets *char_set, size_t num);

/*******************************************************************/
/* Возвращает структуру с набором управл. символов                 */
/* return NULL - ничего нет					   */
/*******************************************************************/
Edi_CharSet *GetEdiCurCharSet(void);
Edi_CharSet *GetEdiCurCharSet_(EDI_REAL_MES_STRUCT *pMes);

/********************************************************************/
/* Возвращает структуру с набором управл. символов из текста EDIFACT*/
/* return NULL - ничего нет					    */
/********************************************************************/
Edi_CharSet *GetEdiCharSetFromText(const char *text, int len);
Edi_CharSet *GetEdiCharSetFromText_(EDI_REAL_MES_STRUCT *pMes,
				    EDI_MESSAGES_STRUCT *pMesTemp,
				    const char *text, int len);

/*
 * по названию сообщения достаем его тип в числах
 */
edi_msg_types_t GetEdiMsgTypeByName(const char *mname);
edi_msg_types_t GetEdiMsgTypeByName_(EDI_MESSAGES_STRUCT *pMesTemp,
				     const char *mname);

/*
 return 1 = EDIFACT
 return 0 = скорее всего нет
 */
int IsEdifactText(const char *text, int len);
int IsEdifactText_(EDI_MESSAGES_STRUCT *pMes, const char *text, int len);

struct EDI_MESSAGES_STRUCT_Deleter {  void operator()(EDI_MESSAGES_STRUCT* p) const;  };
typedef std::unique_ptr<EDI_MESSAGES_STRUCT, EDI_MESSAGES_STRUCT_Deleter> pEDI_MESSAGES_STRUCT;
/*
 * Составляет шаблон сообщений
 * return 0 - Ok
 * return -1 - error
 */
pEDI_MESSAGES_STRUCT CreateTemplateMessages_();

/******************************************************************/
/* 1. Считывает из ORACLE шаблоны сообщений                       */
/* 2. Тестирование считанных данных                               */
/* 3. Преобразование считанных данных в удобный для обработки вид */
/* 4. Удаляет старые данные из ОП                                 */
/* return -1 програмная ошибка                                    */
/* return -2 ошибка чтения ORACLE                                 */
/* return  0 все Ok!                                              */
/******************************************************************/
int CreateTemplateMessages();

/*
 * Возвращает указатель на шаблон сообщений
 */
EDI_MESSAGES_STRUCT *GetEdiTemplateMessages(void);

/*
 * Составляет шаблон сообщений из файлов
 * На вход передается директория с файлами сообщений
 * return 0 - Ok
 * return -1 - error
 */
int CreateTemplateMessagesFf(const char *directory);

int CreateEdiLocalMesStruct(void);

/*****************************************************************/
/*Удаляет старое сообщение, обнуляет стек, считывает новое сообщ.*/
/*input : *Mes - сообщение в формате EDIFACT                     */
/*return -1 - програмная ошибка!!!                               */
/*return -2 - ошибка синтаксиса!!!                               */
/*return -3 - неизвестное сообщение                              */
/*return  1 - все Ok!                                            */
/*****************************************************************/
int ReadEdiMessage(const char *Mes);
int ReadEdiMessage_(EDI_MESSAGES_STRUCT *pTempMes,
		            EDI_REAL_MES_STRUCT *pstMes, const char *Mes);

/*
 Удаляет из памяти структуру считанного сообщения
 */
void DeleteMesIncoming(void);
/*
 Удаляет из памяти структуру собранного сообщения
 */
void DeleteMesOutgoing(void);

/******************************************************************/
/*return 1 - если библиотека корректно 				  */
/*	     обработала очередное сообщение			  */
/*return 0 - ошибка библиотеки					  */
/******************************************************************/
int IsEdilib(void);

int SetEdilib(int Set);

/*************************************************************/
/*Возвращает указатель на идентификатор сообщения.	     */
/*************************************************************/
const char *GetEdiMes_(EDI_REAL_MES_STRUCT *pMes);
const char *GetEdiMes(void);
const char *GetEdiMesW(void);
edi_msg_types_t GetEdiMesType_(EDI_REAL_MES_STRUCT *pMes);
edi_msg_types_t GetEdiMesType(void);
edi_msg_types_t GetEdiMesTypeW(void);


/*Возвращает указатель на вн. статич стректуру со считанным сообщением*/
EDI_REAL_MES_STRUCT *GetEdiMesStruct(void);
/*Возвращает указатель на вн. статич стректуру с созданным сообщением*/
EDI_REAL_MES_STRUCT *GetEdiMesStructW(void);

/********************************************************************/
/*Устанавливает обл. видим. на целиковое сообщение(обнуление pPoint)*/
/*Инициализирует pPoint                                             */
/*return -1 - программная ошибка                                    */
/*return  0 - все Ok                                                */
/********************************************************************/
int ResetEdiPoint_(EDI_REAL_MES_STRUCT *pM);
int ResetEdiPoint(void);
int ResetEdiPointW_(EDI_REAL_MES_STRUCT *pMes);
int ResetEdiPointW();

/********************************************************************/
/*Точка записи последного элемента внутри области видимости	    */
/*Устанавливается автоматом при добавлении элемента в сообщение	    */
/*Функция сбрасывает WrPoint устанавливаясь на первый эл. внутри    */
/*текущей области видимости					    */
/*return 0 - ok			                                    */
/********************************************************************/
int ResetEdiWrPoint();
int ResetEdiWrPoint_(EDI_REAL_MES_STRUCT *pMes);


/********************************************************************/
/*Сохраняет обл. видим. в стек                                      */
/*return  -1 - программная ошибка                                    */
/*return   0 - все Ok                                                */
/********************************************************************/
int PushEdiPoint_(EDI_REAL_MES_STRUCT *pM);
int PushEdiPoint(void);
int PushEdiPointW_(EDI_REAL_MES_STRUCT *pMes);
int PushEdiPointW(void);

/********************************************************************/
/*Извлекает обл. видим. из стека                                    */
/*return -1 - нечего извлекать                                      */
/*return  0 - все Ok                                                */
/********************************************************************/
int PopEdiPoint_(EDI_REAL_MES_STRUCT *pM);
int PopEdiPoint(void);
int PopEdiPointW_(EDI_REAL_MES_STRUCT *pMes);
int PopEdiPointW(void);

/********************************************************************/
/*Извлекает обл. видим. из стека не удаляя эл. стека                */
/*return -1 - нечего извлекать                                      */
/*return  0 - все Ok                                                */
/********************************************************************/
int PopEdiPoint_wd_(EDI_REAL_MES_STRUCT *pM);
int PopEdiPoint_wd(void);
int PopEdiPoint_wdW_(EDI_REAL_MES_STRUCT *pMes);
int PopEdiPoint_wdW(void);

/********************************************************************/
/*Обнуляет  стек                                                    */
/*return  0 - все Ok                                                */
/********************************************************************/
int ResetStackPoint_(EDI_REAL_MES_STRUCT *pMes);
int ResetStackPoint(void);
int ResetStackPointW_(EDI_REAL_MES_STRUCT *pMes);
int ResetStackPointW(void);

/********************************************************************/
/*Инициализирует pFind, или обнуляет , если инициализация уже была  */
/*return -1 - программная ошибка                                    */
/*return  0 - все Ok                                                */
/********************************************************************/
int ResetEdiFind_(EDI_REAL_MES_STRUCT *pMes);
int ResetEdiFind(void);
int ResetEdiFindW(void);

/********************************************************************/
/*Сохраняет найденные последним поиском данные под индексом,        */
/*указанным в качестве входного параметра (ind < MAX_SAVE_FOUND)    */
/*return -1  - неудача                                              */
/*return  0  - Ok                                                   */
/********************************************************************/
int SaveEdiFound(int ind);
int SaveEdiFoundW(int ind);
int SaveEdiFound_(EDI_REAL_MES_STRUCT *pMes, int ind);

/********************************************************************/
/*Восстанавливает сохраненные данные.                               */
/*Внимание: учитывается обл. видимости,                             */
/*для которой эти данные сохранялись                                */
/*return -1   - неудача                                             */
/*return  0   - все Ok                                              */
/********************************************************************/
int LoadEdiFound_(EDI_REAL_MES_STRUCT *pMes, int ind);
int LoadEdiFound(int ind);
int LoadEdiFoundW(int ind);

/********************************************************************/
/*Удаляет сохраненные в SaveEdiFound данные                         */
/********************************************************************/
void FreeEdiFound_(EDI_REAL_MES_STRUCT *pM, int ind);
void FreeEdiFound(int ind);
void FreeEdiFoundW(int ind);

/********************************************************************/
/*Удаляет все сохраненные в SaveEdiFound данные                     */
/********************************************************************/
void ResetEdiFound_(EDI_REAL_MES_STRUCT  *pM);
void ResetEdiFound(void);
void ResetEdiFoundW(void);

/************************************************************/
/*Находит элемент данных по его имени                       */
/*Num - если поиск успешный - кол-во найденных эл. данных   */
/*return -1 - программная ошибка                            */
/*return  0 - ничего не нашли (Num = 0)                     */
/*return  1 - нашли Num эл. данных с заданным именем        */
/************************************************************/
int GetDataByName_(EDI_REAL_MES_STRUCT *pMes, int Data, int *Num);
int GetDataByName(int Data, int *Num);

int GetDataByNum_(EDI_REAL_MES_STRUCT *pMes, int Data, int Seq, int *Num);
int GetDataByNum(int Data, int Seq, int *Num);

/**********************************************************/
/*Возвращает указатель на найденные данные по номеру      */
/*return NULL - неудача                                   */
/*return char* - указатель на данные                      */
/**********************************************************/
const char *GetFoundDataByNum(int Num);
const char *GetFoundDataByNum_(EDI_REAL_MES_STRUCT *pMes, int Num);

/**********************************************************/
/*Производит поиск элемента данных (если его еще не было) */
/*Возвращает указатель на элемент данных                  */
/*или указатель на пустую строку в случае неоднозначности */
/*Input : data - эл. данных                               */
/**********************************************************/
const char *GetDBNz(int data);
const char *GetDBNz_(EDI_REAL_MES_STRUCT *pMes, int data);

/**********************************************************/
/*Производит поиск элемента данных (если его еще не было) */
/*Возвращает указатель на элемент данных                  */
/*или указатель на NULL в случае неоднозначности          */
/*Input : data - эл. данных                               */
/**********************************************************/
const char *GetDBN(int data);
const char *GetDBN_(EDI_REAL_MES_STRUCT *pMes, int data);

/**********************************************************/
/*Производит поиск элемента данных (если его еще не было) */
/*Возвращает указатель на элемент данных по имени и номеру*/
/*или указатель на NULL в случае неудачи                  */
/*Input : data - эл. данных                               */
/**********************************************************/
const char *GetDBNum(int data, int Num_d);
const char *GetDBNum_(EDI_REAL_MES_STRUCT *pMes, int data, int Num_d);
const char *GetDBNumSeq_(EDI_REAL_MES_STRUCT *pMes, int data, int Num_d, int NumSeq);

/**********************************************************/
/*Производит поиск элемента данных (если его еще не было) */
/*Возвращает указатель на элемент данных по имени и номеру*/
/*или указатель на пустую строку в случае неудачи         */
/*Input : data - эл. данных                               */
/**********************************************************/
const char *GetDBNumz(int data, int Num_d);
const char *GetDBNumz_(EDI_REAL_MES_STRUCT *pMes, int data, int Num_d);

/************************************************************/
/*Находит элемент данных по его полному имени               */
/*Num - если поиск успешный - кол-во найденных эл. данных   */
/*программная ошибка или неодназначность:		    */
/*if flag return  NULL					    */
/*     else return  '/0'				    */
/*return  char * - Указатель на эл. данных с заданным именем*/
/************************************************************/
const char *GetDBFN(short SegGr , int SegGrNum,
			const char *Segm, int SegmNum,
			const char *Comp, int CompNum,
			int Data  , int DataNum);

const char *GetDBFN_(EDI_REAL_MES_STRUCT *pMes,
		     short SegGr , int SegGrNum,
		     const char *Segm, int SegmNum,
		     const char *Comp, int CompNum,
		     int Data  , int DataNum);

const char *GetDBFNSeq_(EDI_REAL_MES_STRUCT *pMes,
		     short SegGr , int SegGrNum,
		     const char *Segm, int SegmNum,
		     const char *Comp, int CompNum,
		     int Data  , int DataNum, int NumSeq);

const char *GetDBFNz(short SegGr , int SegGrNum,
                     const char *Segm, int SegmNum,
                     const char *Comp, int CompNum,
		     int Data  , int DataNum );

const char *GetDBFNz_(EDI_REAL_MES_STRUCT *pMes,
                      short SegGr , int SegGrNum,
                      const char *Segm, int SegmNum,
                      const char *Comp, int CompNum,
		      int Data  , int DataNum);

const char *GetDBFN__(EDI_REAL_MES_STRUCT *pMes,
                      short SegGr , int SegGrNum,
                      const char *Segm, int SegmNum,
                      const char *Comp, int CompNum,
                      int Data  , int DataNum , int NumSeq, int flag);
/************************************************************/
/*Находит элемент данных по его полному имени без указания  */
/*номера сегментной группы				    */
/*Num - если поиск успешный - кол-во найденных эл. данных   */
/*программная ошибка или неодназначность:		    */
/* return  '/0'						    */
/*return  char * - Указатель на эл. данных с заданным именем*/
/************************************************************/
#define GetDBFNSegGrZ(Segm,SegmNum,Comp,CompNum,Data,DataNum) \
	GetDBFNz(0,0,(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum))

/************************************************************/
/*Находит элемент данных по его полному имени без указания  */
/*номера сегментной группы				    */
/*Num - если поиск успешный - кол-во найденных эл. данных   */
/*программная ошибка или неодназначность:		    */
/* return  NULL					    	    */
/*return  char * - Указатель на эл. данных с заданным именем*/
/************************************************************/
#define GetDBFNSegGr(Segm,SegmNum,Comp,CompNum,Data,DataNum) \
	GetDBFN(0,0,(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum))

/************************************************************/
/*Находит элемент данных по его полному имени от тек. сегм  */
/*Num - если поиск успешный - кол-во найденных эл. данных   */
/*программная ошибка или неодназначность:		    */
/* return  '/0'						    */
/*return  char * - Указатель на эл. данных с заданным именем*/
/************************************************************/
#define GetDBFNSegZ(Comp,CompNum,Data,DataNum) \
	GetDBFNz(0,0,NULL,0,(Comp),(CompNum),(Data),(DataNum))

/************************************************************/
/*Находит элемент данных по его полному имени от тек. сегм  */
/*Num - если поиск успешный - кол-во найденных эл. данных   */
/*программная ошибка или неодназначность:		    */
/* return  NULL						    */
/*return  char * - Указатель на эл. данных с заданным именем*/
/************************************************************/
#define GetDBFNSeg(Comp,CompNum,Data,DataNum) \
	GetDBFN(0,0,NULL,0,(Comp),(CompNum),(Data),(DataNum))

/************************************************************/
/*Находит элемент данных по его полному имени от тек. комп  */
/*Num - если поиск успешный - кол-во найденных эл. данных   */
/*программная ошибка или неодназначность:		    */
/* return  '/0'						    */
/*return  char * - Указатель на эл. данных с заданным именем*/
/************************************************************/
#define GetDBFNCompZ(Data,DataNum) \
	GetDBFNz(0,0,NULL,0,NULL,0,(Data),(DataNum))

/************************************************************/
/*Находит элемент данных по его полному имени от тек. комп  */
/*Num - если поиск успешный - кол-во найденных эл. данных   */
/*программная ошибка или неодназначность:		    */
/* return  NULL						    */
/*return  char * - Указатель на эл. данных с заданным именем*/
/************************************************************/
#define GetDBFNComp(Data,DataNum) \
	GetDBFN(0,0,NULL,0,NULL,0,(Data),(DataNum))

/*******тоже самое со структурой******/
#define GetDBFNSegGrZ_(pMes, Segm,SegmNum,Comp,CompNum,Data,DataNum) \
	GetDBFNz_((pMes),0,0,(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum))

#define GetDBFNSegGr_(pMes,Segm,SegmNum,Comp,CompNum,Data,DataNum) \
	GetDBFN_((pMes),0,0,(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum))

#define GetDBFNSegZ_(pMes,Comp,CompNum,Data,DataNum) \
	GetDBFNz_((pMes),0,0,NULL,0,(Comp),(CompNum),(Data),(DataNum))

#define GetDBFNSeg_(pMes,Comp,CompNum,Data,DataNum) \
	GetDBFN_((pMes),0,0,NULL,0,(Comp),(CompNum),(Data),(DataNum))

#define GetDBFNCompZ_(pMes,Data,DataNum) \
	GetDBFNz_((pMes),0,0,NULL,0,NULL,0,(Data),(DataNum))

#define GetDBFNComp_(pMes,Data,DataNum) \
	GetDBFN_((pMes),0,0,NULL,0,NULL,0,(Data),(DataNum))


/********************************************************************/
/*      a) Если поиск по заданному  комп.  уже производился,        */
/*         ограничиваем им обл. видимости                           */
/*         return 1 - Ok                                            */
/*      b) Поиска еще не было.                                      */
/*         Производим поиск;                                        */
/*         выполняем пункт а);                                      */
/* return -1 - программная ошибка                                   */
/* return  0 - нет  сегм. с заданным порядковым номером             */
/* return  1 - все Ok                                               */
/********************************************************************/
int SetEdiPointToComposite(const char *Comp, int Num);
int SetEdiPointToCompositeW(const char *Comp, int Num);
int SetEdiPointToComposite_(EDI_REAL_MES_STRUCT *pMes, const char *Comp, int Num);
int SetEdiPointToCompositeW_(EDI_REAL_MES_STRUCT *pMes, const char *Comp, int Num);

/********************************************************************/
/*      a) Если поиск по заданному  сегм.  уже производился,        */
/*         ограничиваем им обл. видимости                           */
/*         return 1 - Ok                                            */
/*      b) Поиска еще не было.                                      */
/*         Производим поиск;                                        */
/*         выполняем пункт а);                                      */
/* return -1 - программная ошибка                                   */
/* return  0 - нет  сегм. с заданным порядковым номером             */
/* return  1 - все Ok                                               */
/********************************************************************/
int SetEdiPointToSegment(const char *Segm, int Num);
int SetEdiPointToSegmentW(const char *Segm, int Num);
int SetEdiPointToSegment_(EDI_REAL_MES_STRUCT *pMes, const char *Segm, int Num);
int SetEdiPointToSegmentW_(EDI_REAL_MES_STRUCT *pMes, const char *Segm, int Num);

/********************************************************************/
/*      a) Если поиск по заданной гр. сегм.  уже производился,      */
/*          ограничиваем обл. видимости группой.с заданным пор. ном.*/
/*         return 1 - Ok                                            */
/*      b) Поиска еще не было.                                      */
/*         Производим поиск;                                        */
/*         выполняем пункт а);                                      */
/* return -1 - программная ошибка                                   */
/* return  0 - нет гр. сегм. с заданным порядковым номером          */
/* return  1 - все Ok                                               */
/********************************************************************/
int SetEdiPointToSegGr(short SegGr, int Num);
int SetEdiPointToSegGr_(EDI_REAL_MES_STRUCT *pMes, short SegGr, int Num);
int SetEdiPointToSegGrW(short SegGr, int Num);
int SetEdiPointToSegGrW_(EDI_REAL_MES_STRUCT *pMes, short SegGr, int Num);

/********************************************************************/
/* Производим поиск, если поиска еще небыло                         */
/* Возвращает кол-во сегментных групп в сообщении                   */
/* return -1 - программная ошибка                                   */
/* return  N - кол-во                                               */
/********************************************************************/
int GetNumSegGr(short SegGr);
int GetNumSegGr_(EDI_REAL_MES_STRUCT *pMes, short SegGr);
int GetNumSegGrW(short SegGr);

/********************************************************************/
/* Производит поиск, если поиска еще небыло                         */
/* Возвращает кол-во сегментов в сообщении учитывая обл. видимости  */
/* return -1 - программная ошибка                                   */
/* return  N - кол-во                                               */
/********************************************************************/
int GetNumSegment(const char *Segm);
int GetNumSegmentW(const char *Segm);
int GetNumSegment_(EDI_REAL_MES_STRUCT *pMes, const char *Segm);


/********************************************************************/
/* Производит поиск, если поиска еще небыло                         */
/* Возвращает кол-во сегментов в сообщении учитывая обл. видимости  */
/* return -1 - программная ошибка                                   */
/* return  N - кол-во                                               */
/********************************************************************/
int GetNumComposite(const char *Comp);
int GetNumCompositeW(const char *Comp);
int GetNumComposite_(EDI_REAL_MES_STRUCT *pMes, const char *Comp);

/*
 * по типу сообщения достаем его тип (запрос / ответ)
 */
edi_msg_req GetEdiMsgQTypeByType(edi_msg_types_t type);

/* Создание нового EDIFACT сообщения */
int CreateNewEdiMes(edi_msg_types_t type, const char *chset);
/* Создание нового EDIFACT сообщения */
int CreateNewEdiMes_(EDI_MESSAGES_STRUCT *pTempMes,
		     EDI_REAL_MES_STRUCT *pMes,
		     edi_msg_types_t type, const char *chset);


/*
 Преобразует ст-ру сообщения в текст EDIFACT
 Выделяет память
 return len >= 0 -Ok
 return < 0 - Error
 */
int WriteEdiMessage_(EDI_REAL_MES_STRUCT *pMes, char **buff);
int WriteEdiMessage(char **buff);

/*
 Преобразует ст-ру сообщения в текст EDIFACT
 Использует переданный буфер
 return len >= 0 -Ok
 return < 0 - Error или маленький буфер
 */
int WriteEdiMessageStat_(EDI_REAL_MES_STRUCT *pMes, char *buff, int bsize);
int WriteEdiMessageStat(char *buff, int bsize);

/*
 Ввод сегмента из строки по формату
 d1+d2:d3+++d4++...
 */
int SetEdiFullSegmentF(const char *segm, int num, const char *format, ...);
/*
 Ввод сегмента из строки
 d1+d2:d3+++d4++...
 */
int SetEdiFullSegment(const char *segm, int num, const char *seg_str);
int SetEdiFullSegment_(EDI_REAL_MES_STRUCT *pM, const char *segm,
		       int num, const char *seg_str);
int SetEdiFullCurSegment(EDI_REAL_MES_STRUCT *pM, const char *seg_str);

/*
 Ввод композита из строки по формату
 d1::d2:d3::...
 */
int SetEdiFullCompositeF(const char *comp, int num, const char *format, ...);
/*
 Ввод композита из строки
 d1::d2:d3::...
 */
int SetEdiFullComposite(const char *comp, int num, const char *comp_str);
int SetEdiFullComposite_(EDI_REAL_MES_STRUCT *pM, const char *comp,
			 int num, const char *comp_str);
int SetEdiFullCurComposite(EDI_REAL_MES_STRUCT *pM, const char *comp_str);


int SetEdiSegGr_(EDI_REAL_MES_STRUCT *pM, int SegGr, int num);
int SetEdiSegGr(int SegGr, int num);

int SetEdiSegment_(EDI_REAL_MES_STRUCT *pM, const char *segm, int num);
int SetEdiSegment(const char *segm, int num);

int SetEdiComposite_(EDI_REAL_MES_STRUCT *pM, const char *comp, int num);
int SetEdiComposite(const char *comp, int num);

int SetEdiDataElem(int dataNum, const char *data);
int SetEdiDataElemArr(int dataNum, int num, const char *data);
int SetEdiDataElemLen(int dataNum, const char *data, int len);
int SetEdiDataElemArrLen(int dataNum, int num, const char *data, int len);
int SetEdiDataElemF(int dataNum, const char *format, ...);
int SetEdiDataElemArrF(int dataNum, int num, const char *format, ...);
int SetEdiDataElem_(EDI_REAL_MES_STRUCT *pM, int data, int num,
		    const char *dataStr, int len);
int SetEdiDataElem__(EDI_REAL_MES_STRUCT *pM, int data, int numseq, int num,
            const char *dataStr, int len);

/*
*Создает элемент данных по его полному имени
*set != 0 - создавать путь
*программная ошибка или неодназначность:
*return -1 - error
*return  0 - Ok
*/
int SetDBFNF(short SegGr , int SegGrNum,
	     const char *Segm, int SegmNum,
	     const char *Comp, int CompNum,
	     int Data  , int DataNum, int set,
	     const char *format, ...);

/*
*Создает элемент данных по его полному имени
*set != 0 - создавать путь
*программная ошибка или неодназначность:
*return -1 - error
*return  0 - Ok
*/
int SetDBFN_(short SegGr , int SegGrNum,
	     const char *Segm, int SegmNum,
	     const char *Comp, int CompNum,
	     int Data  , int DataNum, int set,
	     const char *dataStr, int dataLen);

/*Не создавать путь*/
#define SetDBFN(SegGr, SegGrNum, Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((SegGr),(SegGrNum),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 0, (dataStr), strlen(dataStr))
/*Создавать путь*/
#define SetDBFNs(SegGr, SegGrNum, Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((SegGr),(SegGrNum),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 1, (dataStr), strlen(dataStr))

/*Не создавать путь, передавать длину данных*/
#define SetDBFNl(SegGr, SegGrNum, Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr, dataLen) \
    SetDBFN_((SegGr),(SegGrNum),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 0, (dataStr), (dataLen))
/*Создавать путь, передавать длину данных*/
#define SetDBFNls(SegGr, SegGrNum, Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((SegGr),(SegGrNum),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 1, (dataStr), (dataLen))


/*внутри сегментной группы */
#define SetDBFNSegGr(Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((0),(0),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 0, (dataStr), strlen(dataStr))

#define SetDBFNSegGrs(Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((0),(0),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 1, (dataStr), strlen(dataStr))

#define SetDBFNSegGrl(Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr, dataLen) \
    SetDBFN_((0),(0),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 0, (dataStr), (dataLen))

#define SetDBFNSegGrls(Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((0),(0),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 1, (dataStr), (dataLen))

/*Внутри сегмента */
#define SetDBFNSeg(Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((0),(0),(NULL),(0),(Comp),(CompNum),(Data),(DataNum), 0, (dataStr), strlen(dataStr))

#define SetDBFNSegs(Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((0),(0),(NULL),(0),(Comp),(CompNum),(Data),(DataNum), 1, (dataStr), strlen(dataStr))

#define SetDBFNSegl(Comp, CompNum, Data, DataNum, dataStr, dataLen) \
    SetDBFN_((0),(0),(NULL),(0),(Comp),(CompNum),(Data),(DataNum), 0, (dataStr), (dataLen))

#define SetDBFNSegls(Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((0),(0),(NULL),(0),(Comp),(CompNum),(Data),(DataNum), 1, (dataStr), (dataLen))


void PrintRealEdiMesW(FILE *fl);
void PrintRealEdiMes (FILE *fl);
void PrintRealEdiMes_(EDI_REAL_MES_STRUCT *pIdeMes, FILE *fl);

const char *EdiErrGetString();
const char *EdiErrGetString_(EDI_REAL_MES_STRUCT *pMes);

void SetEdiErrLang(int lang);
void SetEdiErrLang_(EDI_REAL_MES_STRUCT *pMes, int lang);

const char *EdiErrGetSegName(EDI_REAL_MES_STRUCT *pMes);
int EdiErrGetSegNum(void);
int EdiErrGetSegNum_(EDI_REAL_MES_STRUCT *pMes);
int EdiErrGetCompNum(void);
int EdiErrGetCompNum_(EDI_REAL_MES_STRUCT *pMes);
int EdiErrGetDataNum(void);
int EdiErrGetDataNum_(EDI_REAL_MES_STRUCT *pMes);

int isEdiError(void);
int isEdiError_(EDI_REAL_MES_STRUCT *pMes);

int GetEdiErrNum(void);
int GetEdiErrNum_(EDI_REAL_MES_STRUCT *pMes);

/*const char * get_edi_msg_by_sir(int data_elem, int sir_msg);*/


/*Устанавливает спец. символы (по умолчанию для составляемого сообщения)*/
int SetEdiCharSet(CharSetType type);
int SetEdiCharSet_(EDI_REAL_MES_STRUCT *pMes, CharSetType type);
int SetCustomCharSet(const char *s);
/*
 For our char set s = ":+,? \"\n"
 comma or full stop
 */
int SetCustomCharSet_(EDI_REAL_MES_STRUCT *pMes, const char *s);


#if 0
/*
 * по типу сообщения достаем его тип (запрос / ответ)
 */
edi_msg_req GetEdiMsgQTypeByType(edi_msg_types_t type);

#endif
/*
 * по названию сообщения достаем его тип в числах
 */
int GetEdiMsgTypeByType(edi_msg_types_t type, edi_mes_head *pHead);
int GetEdiMsgTypeByType_(EDI_MESSAGES_STRUCT *pMesTemp,
			 edi_msg_types_t type, edi_mes_head *pHead);

int GetEdiMsgTypeByCode(const char *code, edi_mes_head *pHead);
int GetEdiMsgTypeByCode_(EDI_MESSAGES_STRUCT *pMesTemp,
			 const char *code, edi_mes_head *pHead);

/*
 Заносит сообщения в базу из файлов директории
 return EDI_MES_OK - ok
 return other - error
 */
int insert_to_ora_from_dir(const char *dir, const char *, const char *);

int insert_to_ora_from_dir_cur(const char *dir);

/*
 Чистит шаблоны
 */
void DeleteAllEdiMessages( EDI_MESSAGES_STRUCT **ppEdiMes );

int CreateEdiMesStruct(EDI_REAL_MES_STRUCT **pMes);

/****************************************************************************/
/*Считывает сообщение в ОП                                                  */
/*Mes -указатель на строку, содержащую данное сообщение                     */
/*return EDI_MES_NOT_FND    -неизвестное сообщение                          */
/*return EDI_MES_ERR        -ошибка                                         */
/*return EDI_MES_OK         -считывание прошло успешно                      */
/*return EDI_MES_STRUCT_ERR -ошибка в структуре сообщения                   */
/****************************************************************************/
int ReadEdiMes(const char *Mes, EDI_MESSAGES_STRUCT *pMes,
                          EDI_REAL_MES_STRUCT *pCurMes);

/*
 Не считывая сообщения целиком, берем только UNB,UNH
 return EDI_MES_OK
 return EDI_MES_STRUCT_ERR
 */
int FillEdiHeadStr(EDI_REAL_MES_STRUCT *pCurMes, EDI_MESSAGES_STRUCT *pMes,
		    const char *Mes);

int create_edi_empty_files(const char *dir);

void PrintAllEdiTemplateMessages(FILE *fl);

void PrintAllEdiMessages( EDI_MESSAGES_STRUCT *pIdeMes, FILE *fl);

/******************************************************/
/*        Очищает базу данных сообщений               */
/* return 0 - SQL ERROR                               */
/* return 1 - Ok                                      */
/******************************************************/
int DeleteDBMesseges();


/*
 Перекодирует сообщение EDIFACT из исходного набора упр сиволов в заданный
 Параметры:
 edi_text - исходный текст
 len - длинна
 for_coding - код набора упр сиволов (напр IATA)
 dest - адрес получателя
 dest_size - размер буфера получателя
 */
int ChangeEdiCharSetStat(const char *edi_text, int len,
			 const char *for_coding, char *dest, size_t dest_size);

/*
 ВЫДЕЛЯЕТ ПАМЯТЬ!
 Перекодирует сообщение EDIFACT из исходного набора упр сиволов в заданный
 Параметры:
 edi_text - исходный текст
 len - длинна
 for_coding - код набора упр сиволов (напр IATA)
 dest - адрес получателя
 dest_len - длина получ строки
 */
int ChangeEdiCharSet(const char *edi_text, int len,
		     const char *for_coding, char **dest, size_t *dest_len);

int maskSpecialChars_capp(const Edi_CharSet *Chars, const char *instr, char **out);

#endif /*_EDI_USER_FUNC_H_*/
