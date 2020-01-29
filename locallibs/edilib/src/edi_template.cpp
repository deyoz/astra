#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "edi_tables.h"
#include "edi_all.h"
#include "edi_func.h"
#include "edi_user_func.h"
#include "edi_malloc.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE

#include "edi_test.h"

//-----------------------------------------------------------------------

void EDI_MESSAGES_STRUCT_Deleter::operator()(EDI_MESSAGES_STRUCT* p) const
{
    delete[] p->char_set_arr;
    p->char_set_arr = nullptr;

    auto pp = p;
    DeleteAllEdiMessages(&pp);
    delete p;
}

typedef std::unique_ptr<EDI_MESSAGES_STRUCT, EDI_MESSAGES_STRUCT_Deleter> pEDI_MESSAGES_STRUCT;
static pEDI_MESSAGES_STRUCT pTempMes;

static void EDI_TABLES_STRUCT_Deleter(EDI_TABLES_STRUCT* p)
{
    auto pp = p;
    DeleteAllEdiTabMessages(&pp, 1);
    edi_free(p);
}

typedef std::shared_ptr<EDI_TABLES_STRUCT> pEDI_TABLES_STRUCT;

//-----------------------------------------------------------------------

static pEDI_MESSAGES_STRUCT CreateTemplateMessagesFt(const pEDI_TABLES_STRUCT& pEdiTab);
static pEDI_MESSAGES_STRUCT CreateTemplateMes_(const pEDI_TABLES_STRUCT& pTab);



//-----------------------------------------------------------------------

EDI_TEMP_MESSAGE *GetEdiTemplateByType(EDI_MESSAGES_STRUCT *pTMes, edi_msg_types_t msg_type)
{
    EDI_TEMP_MESSAGE *pMes=pTMes->pTemps;
    for(;(pMes && (pMes->Type.type != msg_type)); EdiTrace(TRACE3,"type=%d",pMes->Type.type),pMes = pMes->Next);
    return pMes;
}

EDI_TEMP_MESSAGE *GetEdiTemplateByName(EDI_MESSAGES_STRUCT *pTMes ,const char *MesName)
{
    EDI_TEMP_MESSAGE *pMes = pTMes->pTemps;
    while ( pMes )
    {
        if(memcmp(pMes->Type.code, MesName, MESSAGE_LEN) == 0) {
            break;
        }
        pMes = pMes->Next;
    }/*end while*/
    return pMes;
}

static int(*init_edilib_fp)(void) = NULL;

void RegEdilibInit(int(*init_edilib_)(void))
{
    init_edilib_fp = init_edilib_;
}

void init_edilib()
{
    static bool init = false;
    if(!init && init_edilib_fp) {
        init = true;
        if(init_edilib_fp())
            init = false;
    }
}

/*
 * Возвращает указатель на шаблон сообщений
 */
EDI_MESSAGES_STRUCT *GetEdiTemplateMessages(void)
{
    init_edilib();
    return pTempMes.get();
}

static pEDI_TABLES_STRUCT ReadTablesFf(const char *directory)
{
    EDI_TABLES_STRUCT   *pEdiTab = nullptr;
    if( ReadTablesFf(directory,&pEdiTab) != EDI_READ_BASE_OK )
        return pEDI_TABLES_STRUCT();
    return pEDI_TABLES_STRUCT(pEdiTab, EDI_TABLES_STRUCT_Deleter);
}

/*
 * Составляет шаблон сообщений из файлов
 * На вход передается директория с файлами сообщений
 * return 0 - Ok
 * return -1 - error
 */
int CreateTemplateMessagesFf(const char *directory)
{
    if(not pTempMes)
    {
        if(auto pEdiTab = ReadTablesFf(directory))
        {
            pTempMes = CreateTemplateMessagesFt(pEdiTab);
            return pTempMes ? 0 : -1;
        }
        EdiError(EDILOG,"ERROR READ EDIFACT DIRECTORY !!!\n\n");
        return -1; //-2;
    }
    return 0;
}


static pEDI_TABLES_STRUCT ReadTablesOra()
{
    EDI_TABLES_STRUCT   *pEdiTab = nullptr;
    if( ReadTablesOra(&pEdiTab) != EDI_READ_BASE_OK )
        return pEDI_TABLES_STRUCT();
    return pEDI_TABLES_STRUCT(pEdiTab, EDI_TABLES_STRUCT_Deleter);
}

pEDI_MESSAGES_STRUCT CreateTemplateMessages_()
{
    return CreateTemplateMessagesFt( ReadTablesOra() );
}

/*
 * Составляет шаблон сообщений
 * return 0 - Ok
 * return -1 - error
 */
int CreateTemplateMessages()
{
    if(not pTempMes)
    {
        pTempMes = CreateTemplateMessages_();
        return pTempMes ? 0 : -1;
    }
    return 0;
}

static pEDI_MESSAGES_STRUCT CreateTemplateMessagesFt(const pEDI_TABLES_STRUCT& pEdiTab)
{
    int Err = TestEdiTbl(pEdiTab.get()); /*Проверка считанной информации*/
    if(Err != EDI_READ_BASE_OK){
        EdiError(EDILOG,"TESTING TABLES ERROR !!!\n\n");
        return pEDI_MESSAGES_STRUCT();
    }
    return CreateTemplateMes_(pEdiTab);
}

/*****************************************************************************/
/* Выстраивает в памяти шаблоны сообщений;                                   */
/* Удаляет из ОП считанные таблицы - они более не нужны.                     */
/* return EDI_READ_BASE_OK  - все щл                                         */
/* return EDI_READ_BASE_ERR - ошибка                                         */
/*****************************************************************************/
static pEDI_MESSAGES_STRUCT CreateTemplateMes_(const pEDI_TABLES_STRUCT& pTab)
{
    int Ret;
    EDI_TEMP_MESSAGE *pMesTmpPrev=NULL;

    if( (pTab.get()        == NULL) || (pTab->MesTable    == NULL) ||
        (pTab->MesStrTable == NULL) || (pTab->SegStrTable == NULL) ||
        (pTab->CompStrTable== NULL) || (pTab->DataTable   == NULL)  ) {
        return pEDI_MESSAGES_STRUCT();
        //return EDI_READ_BASE_ERR ;
    }

    auto pMesTabTmp = pTab->MesTable; /*Таблица сообщений*/

    pEDI_MESSAGES_STRUCT ppEdiMes(new EDI_MESSAGES_STRUCT);
    memset(ppEdiMes.get(), 0, sizeof(EDI_MESSAGES_STRUCT));

    while( pMesTabTmp ) {
        auto pMesTmp=(EDI_TEMP_MESSAGE *)edi_calloc(1,sizeof(EDI_TEMP_MESSAGE));
        if(pMesTmp ==NULL ) {
            EdiError(EDILOG,"Memory error");
            return pEDI_MESSAGES_STRUCT();
        }

        strcpy(pMesTmp->Type.code, pMesTabTmp->Message);
        pMesTmp->Type.type = 0;
        /*Переприсваиваем информацию  */
        strcpy(pMesTmp->Text, pMesTabTmp->Text);

        Ret = CreateTempSeg(pMesTmp, pMesTabTmp,pTab.get(),pMesTmp);/*Создаем шаблон для */
        if( Ret != EDI_READ_BASE_OK ){               /*каждого сообщения  */
            EdiError(EDILOG,"Create template of the message %s ... Error!\n",
                     pMesTmp->Type.code);
            edi_free(pMesTmp);
            return pEDI_MESSAGES_STRUCT();
        }

        EdiTrace(TRACE3,"Create template of the message %s ... Ok!\n",
                 pMesTmp->Type.code);

        if(pMesTmpPrev==NULL){
            pMesTmpPrev = pMesTmp;
            ppEdiMes->pTemps=pMesTmp;
        }else {
            pMesTmpPrev->Next = pMesTmp;
            pMesTmpPrev = pMesTmp;
        }

        pMesTabTmp = pMesTabTmp->Next;
    }

    EdiTrace(TRACE3,"Create template of the messages... Ok!\n");
    return ppEdiMes;
}
/************************************************************/
/************************************************************/
int CreateTempSeg(EDI_TEMP_MESSAGE *wholeTemplate, MESSAGE_TABLE_STRUCT *pMesTab,
                  EDI_TABLES_STRUCT *pTab, EDI_TEMP_MESSAGE *pMes)
{
    int Ret;
    MES_STRUCT_TABLE_STRUCT *MesStrTab;
    TAG_STRUCT              *TagStr;
    if((pMes->Tag = (TAG_STRUCT *)calloc(1, sizeof(TAG_STRUCT)))==NULL)
        return EDI_READ_BASE_ERR;
    TagStr = pMes->Tag;
    MesStrTab = pMesTab->First; /*Адрес первого сегм. данного сообщения*/
    while ( 1 )
    {
        EdiTrace(TRACE3, "%s MaxPos:%d, Pos:%d, R:%d, S:%s, Tag:%s, Text:%s",
            MesStrTab->Message, MesStrTab->MaxPos,
            MesStrTab->Pos, MesStrTab->R, MesStrTab->S,
            MesStrTab->Tag, MesStrTab->Text);

        if( MesStrTab->MaxPos != 0 ) { /*Сегментная группа*/
            Ret = InsertSegGroup(wholeTemplate, &MesStrTab, TagStr);
            if(Ret != EDI_READ_BASE_OK)
                return EDI_READ_BASE_ERR; /*Перемещаемся к концу данной сег. группы*/
        } else { /*Сегмент*/
            if(TestSegUnique(pMes->Tag, MesStrTab->Tag)){
                EdiError(EDILOG,"Tag %s is not unique in root of %s",
                         TagStr->TagName,
                         pMes->Type.code);
                return EDI_READ_BASE_ERR;
            }
            strcpy(TagStr->TagName, MesStrTab->Tag);
            strcpy(TagStr->Text, MesStrTab->Text);
            TagStr->Pos     =  MesStrTab->Pos;
            TagStr->S       =  MesStrTab->S[0];
            TagStr->MaxR    =  MesStrTab->R;
            Ret = CreateTempComp(TagStr,MesStrTab);
            if(Ret != EDI_READ_BASE_OK)
                return EDI_READ_BASE_ERR;
        }
        if (MesStrTab->Next == NULL) {
            break;
        }
        MesStrTab = MesStrTab->Next;
        if (memcmp(MesStrTab->Message,pMesTab->Message,MESSAGE_LEN) != 0) {
            break;
        }
        if((TagStr->NextTag = (TAG_STRUCT *)calloc(1, sizeof(TAG_STRUCT)))==NULL)
            return EDI_READ_BASE_ERR;
        TagStr = TagStr->NextTag;
    }
    TagStr->NextTag = NULL;
    return EDI_READ_BASE_OK;
}
/***********************/
int InsertSegGroup(EDI_TEMP_MESSAGE *wholeTemplate,
                   MES_STRUCT_TABLE_STRUCT **ppMesStrTab,TAG_STRUCT *pTagStr)
{
    int Ret,MaxPos;
    char TmpText[50];
    MES_STRUCT_TABLE_STRUCT *pMesStrTab;
    TAG_STRUCT *pTagStrTmp;

    MaxPos     = (*ppMesStrTab)->MaxPos;
    pMesStrTab = (*ppMesStrTab);

    if (TestSegGrUnique(wholeTemplate->Tag, pMesStrTab->GrpNum)) {
        EdiError(EDILOG,"Segment group %d is not unique in whole message %s",
                 pMesStrTab->GrpNum,
                 wholeTemplate->Type.code);
        return EDI_READ_BASE_ERR;
    }

    sprintf(TmpText,"Segment group %d", pMesStrTab->GrpNum);
    strcpy(pTagStr->Text, TmpText);
    pTagStr->Num     = pMesStrTab->GrpNum;
    pTagStr->Pos     = pMesStrTab->Pos;
    pTagStr->S       = pMesStrTab->S[0];
    pTagStr->MaxR    = pMesStrTab->R;
    pTagStr->Comp    = NULL;
    /******/
    if(pMesStrTab->Next == NULL){
        EdiError(EDILOG,"Error in body of segment group %s", TmpText);
        return EDI_READ_BASE_ERR;
    }
    pMesStrTab = pMesStrTab->Next;
    if( (pMesStrTab->S[0] != MANDATORY) || (pMesStrTab->R != 1) ){
        EdiError(EDILOG,"A segment group must begin whith segment M 1. Error in "
                 "TagName : %s, Pos : %d SegmentGroup : %d",
                 pMesStrTab->Tag, pMesStrTab->Pos, pMesStrTab->GrpNum);
        return EDI_READ_BASE_ERR;
    }
    if((pTagStr->GrStr = (TAG_STRUCT *)calloc(1, sizeof(TAG_STRUCT)))==NULL)
        return EDI_READ_BASE_ERR;
    pTagStrTmp = pTagStr->GrStr;
    /******/
    while ( 1 )
    {
        EdiTrace(TRACE3, "%s MaxPos:%d, Pos:%d, R:%d, S:%s, Tag:%s, Text:%s",
            pMesStrTab->Message, pMesStrTab->MaxPos,
            pMesStrTab->Pos, pMesStrTab->R, pMesStrTab->S,
            pMesStrTab->Tag, pMesStrTab->Text);

        if( pMesStrTab->MaxPos != 0 ) { /*Сегментная группа*/
            Ret = InsertSegGroup(wholeTemplate, &pMesStrTab,pTagStrTmp);
            if(Ret != EDI_READ_BASE_OK){
                return EDI_READ_BASE_ERR; /*Перемещаемся к концу данной сег. группы*/
            }
        } else { /*Сегмент*/
            if(TestSegUnique(pTagStr->GrStr, pMesStrTab->Tag)){
                EdiError(EDILOG,"Tag %s is not unique in SegGr #%d (%s)",
                         pMesStrTab->Tag, pMesStrTab->GrpNum, pTagStr->GrStr->TagName);
                return EDI_READ_BASE_ERR;
            }
            strcpy(pTagStrTmp->TagName, pMesStrTab->Tag);
            strcpy(pTagStrTmp->Text, pMesStrTab->Text);
            pTagStrTmp->Pos     =  pMesStrTab->Pos;
            pTagStrTmp->S       =  pMesStrTab->S[0];
            pTagStrTmp->MaxR    =  pMesStrTab->R;
            Ret = CreateTempComp(pTagStrTmp,pMesStrTab);
            if(Ret != EDI_READ_BASE_OK)
                return EDI_READ_BASE_ERR;

        }
        if(pMesStrTab->Pos == MaxPos) {
            break;
        }
        if(pMesStrTab->Next == NULL) {
            EdiError(EDILOG,"Error in body of segment group %s", TmpText);
            return EDI_READ_BASE_ERR;
        }
        pMesStrTab = pMesStrTab->Next;

        if((pTagStrTmp->NextTag = (TAG_STRUCT *)calloc(1, sizeof(TAG_STRUCT)))==NULL)
            return EDI_READ_BASE_ERR;
        pTagStrTmp = pTagStrTmp->NextTag;
    }
    (*ppMesStrTab)= pMesStrTab;
    return EDI_READ_BASE_OK;
}
/*******************************/
int CreateTempComp(TAG_STRUCT *pTagStr, MES_STRUCT_TABLE_STRUCT *pMesStrTab)
{
    int Ret;
    COMPOSITE_STRUCT *pCompStr;
    SEG_STRUCT_TABLE_STRUCT *pSegTabStr;

    if((pTagStr->Comp = (COMPOSITE_STRUCT *)
        calloc(1, sizeof(COMPOSITE_STRUCT))) == NULL)
        return EDI_READ_BASE_ERR;
    pCompStr = pTagStr->Comp;
    pSegTabStr = pMesStrTab->First;

    while( 1 ){

        if( pSegTabStr->DataElem != 0){
            Ret = CreateTempDataFromSeg(pSegTabStr,pCompStr, pTagStr->Comp);
            if(Ret == EDI_READ_BASE_ERR)
                return Ret;
        }
        else {
            pCompStr->Num = CountEdiComp(pTagStr->Comp, pSegTabStr->Composite);
            strcpy(pCompStr->Composite, pSegTabStr->Composite);
            strcpy(pCompStr->Text, pSegTabStr->Text);
            Ret = CreateTempData(pSegTabStr,pCompStr);
            if(Ret == EDI_READ_BASE_ERR) return Ret;
            pCompStr->Pos   = pSegTabStr->Pos;
            pCompStr->S     = pSegTabStr->S[0];
            pCompStr->MaxR  = pSegTabStr->R;
        }

        if (pSegTabStr->Next == NULL) {
            break;
        }
        pSegTabStr = pSegTabStr->Next;

        if(memcmp(pSegTabStr->Tag,pMesStrTab->Tag,TAG_LEN+1) != 0){
            break;
        }

        if((pCompStr->Next = (COMPOSITE_STRUCT *)
            calloc(1, sizeof(COMPOSITE_STRUCT))) == NULL)
            return EDI_READ_BASE_ERR;

        pCompStr = pCompStr->Next;
    }/*end while*/
    pCompStr->Next = NULL;
    return EDI_READ_BASE_OK;
}
/***************************************************************************/
int CreateTempDataFromSeg(SEG_STRUCT_TABLE_STRUCT *pSegTabStr,
                          COMPOSITE_STRUCT        *pCompStr,
                          COMPOSITE_STRUCT        *pCompRoot)
{
    /* int Ret;*/
    DATA_ELEM_TABLE_STRUCT   *pDataElemTab;
    DATA_ELEM_STRUCT         *pDataElem;

    pDataElemTab = pSegTabStr->FirstElem;
    if((pCompStr->DataElem = (DATA_ELEM_STRUCT *)
        calloc(1, sizeof(DATA_ELEM_STRUCT))) == NULL)
        return EDI_READ_BASE_ERR;

    pDataElem           = pCompStr->DataElem;
    pDataElem->DataElem = pDataElemTab->DataElem;
    strcpy(pDataElem->Text, pDataElemTab->Text);
    pDataElem->Format   = pDataElemTab->Format[0];
    pDataElem->MaxField = pDataElemTab->MaxField;
    pDataElem->MinField = pDataElemTab->MinField;
    pDataElem->MaxR     = pSegTabStr->R;
    pDataElem->Pos      = pSegTabStr->Pos;
    pDataElem->S        = pSegTabStr->S[0];
    pDataElem->Num 	= CountEdiSingleData(pCompRoot, pSegTabStr->DataElem)-pDataElem->MaxR;

    /* pDataElem->Next     = NULL;
     printf("-DataNum: %d    Pos: %d  MaxR: %d\n",pDataElem->DataElem,
     pDataElem->Pos,pDataElem->MaxR);*/

    return EDI_READ_BASE_OK;
}
/************************/
int CreateTempData (SEG_STRUCT_TABLE_STRUCT *pSegTabStr,
                    COMPOSITE_STRUCT        *pCompStr  )
{
    /* int Ret;*/
    int sch = 0;/**/
    DATA_ELEM_TABLE_STRUCT   *pDataElemTab;
    DATA_ELEM_STRUCT         *pDataElem;
    COMP_STRUCT_TABLE_STRUCT *pCompStrTab;

    pCompStrTab = pSegTabStr->FirstComp;
    if((pCompStr->DataElem = (DATA_ELEM_STRUCT *)
        calloc(1, sizeof(DATA_ELEM_STRUCT))) == NULL)
        return EDI_READ_BASE_ERR;
    pDataElem    = pCompStr->DataElem;
    pDataElemTab = pCompStrTab->First;
    while ( 1 ) {
        pDataElem->Num      = CountEdiData(pCompStr->DataElem, pDataElemTab->DataElem);
        pDataElem->DataElem = pDataElemTab->DataElem;
        strcpy(pDataElem->Text, pDataElemTab->Text);
        pDataElem->Format   = pDataElemTab->Format[0];
        pDataElem->MaxField = pDataElemTab->MaxField;
        pDataElem->MinField = pDataElemTab->MinField;
        pDataElem->S        = pCompStrTab->S[0];
        pDataElem->Pos      = pCompStrTab->Pos;
        pDataElem->MaxR     = pCompStrTab->R;
        sch += pDataElem->MaxR;

        if(pCompStrTab->Next == NULL){
            break;
        }
        pCompStrTab  = pCompStrTab->Next;
        pDataElemTab = pCompStrTab->First;

        if(memcmp(pCompStrTab->Composite,pSegTabStr->Composite,COMP_LEN+1) != 0) {
            break;
        }

        if((pDataElem->Next = (DATA_ELEM_STRUCT *)
            calloc(1, sizeof(DATA_ELEM_STRUCT))) == NULL)
            return EDI_READ_BASE_ERR;
        pDataElem = pDataElem->Next;
    }/*end while*/
    /* pDataElem->Next = NULL;  */
    if(sch < 2){
        EdiError(EDILOG,"A data element must be containing \n"
                 "two or more component data element!!!");
        return EDI_READ_BASE_ERR;
    }
    return EDI_READ_BASE_OK;
}

int TestSegGrUnique(TAG_STRUCT *pTag, int GrpNum)
{
    while(pTag) {
        if(pTag->Num == GrpNum)
            return -1;
        if(pTag->GrStr && TestSegGrUnique(pTag->GrStr, GrpNum) < 0)
            return -1;

        pTag = pTag->NextTag;
    }
    return 0;
}

int TestSegUnique(TAG_STRUCT *pTag, const char *tag)
{
    while(pTag){
        if(!memcmp(tag, pTag->TagName, TAG_LEN)){
            return -1;
        }
        pTag = pTag->NextTag;
    }
    return 0;
}

int CountEdiComp(COMPOSITE_STRUCT *pComp, const char *comp)
{
    int count = 0;
    while(pComp){
        if(*pComp->Composite && !memcmp(pComp->Composite, comp, COMP_LEN)){
            count += pComp->MaxR;
        }
        pComp = pComp->Next;
    }
    return count;
}

int CountEdiData(DATA_ELEM_STRUCT *pData, int data)
{
    int count = 0;
    while(pData)
    {
        if(pData->DataElem == data){
            count += pData->MaxR;
        }
        pData = pData->Next;
    }
    return count;
}

int CountEdiSingleData(COMPOSITE_STRUCT *pComp, int data)
{
    int count = 0;
    while(pComp){
        if(!(*pComp->Composite) && pComp->DataElem->DataElem == data){
            count += pComp->DataElem->MaxR;
        }
        pComp = pComp->Next;
    }
    return count;
}

