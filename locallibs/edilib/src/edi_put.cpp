#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "edi_tables.h"
#include "edi_func.h"
#include "edi_malloc.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include "edi_test.h"
#include "edi_user_func.h"
/*#define _DEBUG_DATA_*/

static EDI_REAL_MES_STRUCT *pMesGlobalWrite=NULL;

/*Возвращает указатель на вн. статич стректуру с созданным сообщением*/
EDI_REAL_MES_STRUCT *GetEdiMesStructW(void)
{
    if (!pMesGlobalWrite)
    {
        CreateEdiMesStruct(&pMesGlobalWrite);
    }
    return pMesGlobalWrite;
}

int CreateEdiMesStructWrite()
{
    return CreateEdiMesStruct(&pMesGlobalWrite);
}

int SetEdiCharSet(CharSetType type)
{
    return SetEdiCharSet_(GetEdiMesStructW(), type);
}

int SetEdiCharSet_(EDI_REAL_MES_STRUCT *pMes, CharSetType type)
{
    pMes->pCharSet = GetCharSetByType(type);
    return 0;
}

int SetCustomCharSet(const char *s)
{
    return SetCustomCharSet_(GetEdiMesStructW(), s);
}

/*
 For our char set s = ":+,? \"\n"
 comma or full stop
 */
int SetCustomCharSet_(EDI_REAL_MES_STRUCT *pMes, const char *s)
{
    return SetCharSetOnType(pMes, s, LoadedEdiCharSet);
}

int SetCharSetOnType(EDI_REAL_MES_STRUCT *pMes, const char *s, CharSetType chstype)
{
    int len = strlen(s);

    if((len<CUSTOM_CHAR_SET_LEN) || len>(CUSTOM_CHAR_SET_LEN+MAX_END_SEG_LEN-1)){
        EdiError(EDILOG,"Bad string of character set:<%s>",s);
        return -1;
    }

    pMes->pCharSet = GetCharSetByType(chstype);
    if(!pMes->pCharSet)
    {
        EdiError(EDILOG,"Error in loading EdiCharSet");
        return -1;
    }

    pMes->pCharSet->EndData  =  s[0];
    pMes->pCharSet->EndComp  =  s[1];
    /*s[2] - reserved*/
    pMes->pCharSet->Release  =  s[3];
    /*s[4] - reserved*/
    pMes->pCharSet->EndSegLen = strlen(&s[5]);
    memcpy(pMes->pCharSet->EndSegStr, &s[5], pMes->pCharSet->EndSegLen);
    pMes->pCharSet->EndSegStr[pMes->pCharSet->EndSegLen]='\0';

    return EDI_MES_OK;
}

/* Создание нового EDIFACT сообщения */
int CreateNewEdiMes(edi_msg_types_t type, const char *chset)
{
    EDI_REAL_MES_STRUCT *pCurMes  = NULL;
    EDI_MESSAGES_STRUCT *pTempMes = NULL;

    if((pTempMes = GetEdiTemplateMessages())==NULL){
        EdiError(EDILOG,"GetTemplateMessages()==NULL");
        return EDI_MES_ERR;
    }

    if(CreateEdiMesStructWrite() || (pCurMes = GetEdiMesStructW()) == NULL) {
        EdiError(EDILOG,"Error in CreateEdiLocalMesStruct()");
        return EDI_MES_ERR;
    }

    return CreateNewEdiMes_(pTempMes, pCurMes, type, chset);
}

int CreateNewEdiMes_(EDI_MESSAGES_STRUCT *pTempMes,
                     EDI_REAL_MES_STRUCT *pNewMes,
                     edi_msg_types_t type, const char *chset)
{
    CharSetType chstype=LoadedEdiCharSet;

    if(pNewMes->Tag != NULL)
    {   /*Если ранее уже собирали сообщение и не убрали за собой*/
        WriteEdiLog(EDILOG,"The EDIFACT message structure has not been removed");
        DeleteRealMes(pNewMes);  /*Удаляем ранее собранное сообщение*/
    }

    if(chset)
    {
        const char *chset_str=GetEdiCharSetByName(pTempMes, chset,0);
        if(chset_str){
            if(SetCharSetOnType(pNewMes,chset_str,PresetEdiCharSet)){
                return EDI_MES_ERR;
            }
        } else {
            EdiError(EDILOG,"Unknown character set: %s", chset);
            return EDI_MES_ERR;
        }
        chstype=PresetEdiCharSet;
    }
    pNewMes->pCharSet = GetCharSetByType(chstype);
    if(pNewMes->pCharSet == NULL)
    {
        EdiError(EDILOG,"Cannot get char_set by type %d", chstype);
        return EDI_MES_ERR;
    }

    pNewMes->pTempMes = GetEdiTemplateByType(pTempMes, type);
    if(pNewMes->pTempMes == NULL)
    {
        EdiError(EDILOG,"Can't find template message by type=%d", type);
        return EDI_MES_NOT_FND;
    }

    strcpy(pNewMes->Text, pNewMes->pTempMes->Text);
    memcpy(pNewMes->Message,pNewMes->pTempMes->Type.code,
           MESSAGE_LEN);
    pNewMes->Message[MESSAGE_LEN]='\0';

    EdiTrace(TRACE4,"Create new edifact message <%s> - Ok", pNewMes->Message);

    return EDI_MES_OK;
}

const char *GetEdiMesW(void)
{
    return GetEdiMes_(GetEdiMesStructW());
}

edi_msg_types_t GetEdiMesTypeW(void)
{
    return GetEdiMesType_(GetEdiMesStructW());
}

int ResetEdiPointW(void)
{
    return ResetEdiPointW_(GetEdiMesStructW());
}

int ResetEdiPointW_(EDI_REAL_MES_STRUCT *pMes)
{
    int Ret = ResetEdiPoint_(pMes);
    ResetEdiWrPoint_(pMes);
    return Ret;
}

int PushEdiPointW(void)
{
    return PushEdiPointW_(GetEdiMesStructW());
}

int PushEdiPointW_(EDI_REAL_MES_STRUCT *pMes)
{
    int Ret = PushEdiPoint_(pMes);
    ResetEdiWrPoint_(pMes);
    return Ret;
}

int PopEdiPointW(void)
{
    return PopEdiPointW_(GetEdiMesStructW());
}

int PopEdiPointW_(EDI_REAL_MES_STRUCT *pMes)
{
    int Ret = PopEdiPoint_(pMes);
    ResetEdiWrPoint_(pMes);
    return Ret;
}

int PopEdiPoint_wdW(void)
{
    return PopEdiPoint_wdW_(GetEdiMesStructW());
}

int PopEdiPoint_wdW_(EDI_REAL_MES_STRUCT *pMes)
{
    int Ret = PopEdiPoint_wd_(pMes);
    ResetEdiWrPoint_(pMes);
    return Ret;
}

int ResetStackPointW(void)
{
    return ResetStackPointW_(GetEdiMesStructW());
}

int ResetStackPointW_(EDI_REAL_MES_STRUCT *pMes)
{
    int Ret = ResetStackPoint_(pMes);
    ResetEdiWrPoint_(pMes);
    return Ret;
}

int ResetEdiFindW(void)
{
    return ResetEdiFind_(GetEdiMesStructW());
}

int SaveEdiFoundW(int ind)
{
    return SaveEdiFound_(GetEdiMesStructW(), ind);
}

int LoadEdiFoundW(int ind)
{
    return LoadEdiFound_(GetEdiMesStructW(), ind);
}

void FreeEdiFoundW(int ind)
{
    return FreeEdiFound_(GetEdiMesStructW(), ind);
}

void ResetEdiFoundW(void)
{
    return ResetEdiFound_(GetEdiMesStructW());
}

int SetEdiPointToSegGrW(short SegGr, int Num)
{
    return SetEdiPointToSegGrW_(GetEdiMesStructW(), SegGr, Num);
}

int SetEdiPointToSegGrW_(EDI_REAL_MES_STRUCT *pMes, short SegGr, int Num)
{
    int Ret = SetEdiPointToSegGr_(pMes, SegGr, Num);
    ResetEdiWrPoint_(pMes);
    return Ret;
}

int GetNumSegGrW(short SegGr)
{
    return GetNumSegGr_(GetEdiMesStructW() ,SegGr);
}

int GetNumSegmentW(const char *Segm)
{
    return GetNumSegment_(GetEdiMesStructW(), Segm);
}

int SetEdiPointToSegmentW(const char *Segm, int Num)
{
    return SetEdiPointToSegmentW_(GetEdiMesStructW(), Segm, Num);
}

int SetEdiPointToSegmentW_(EDI_REAL_MES_STRUCT *pMes, const char *Segm, int Num)
{
    int Ret = SetEdiPointToSegment_(pMes, Segm, Num);
    ResetEdiWrPoint_(pMes);
    return Ret;
}

int GetNumCompositeW(const char *Comp)
{
    return GetNumComposite_(GetEdiMesStructW(), Comp);
}

int SetEdiPointToCompositeW(const char *Comp, int Num)
{
    return SetEdiPointToCompositeW_(GetEdiMesStructW(), Comp, Num);
}

int SetEdiPointToCompositeW_(EDI_REAL_MES_STRUCT *pMes, const char *Comp, int Num)
{
    int Ret = SetEdiPointToComposite_(pMes, Comp, Num);
    ResetEdiWrPoint_(pMes);
    return Ret;
}

int ResetEdiWrPoint()
{
    return ResetEdiWrPoint_(GetEdiMesStructW());
}

int ResetEdiWrPoint_(EDI_REAL_MES_STRUCT *pMes)
{
    memset(&pMes->WrPoint, 0, sizeof(pMes->WrPoint));
/*    if(pMes->pPoint){
    if(pMes->pPoint->pGrStr)
        pMes->WrPoint.pTag   = pMes->pPoint->pGrStr->GrStr;
    else if(pMes->pPoint->pTag)
        pMes->WrPoint.pComp  = pMes->pPoint->pTag->Comp;
    else if(pMes->pPoint->pComp)
        pMes->WrPoint.pData  = pMes->pPoint->pComp->DataElem;
    }*/
    return 0;
}

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
             const char *format, ...)
{
    va_list ap;
    MesEdiStruct MesTxt;
    int Ret;

    if(!format){
        EdiError(EDILOG,"NULL format pointer");
        return EDI_MES_ERR;
    }

    memset(&MesTxt, 0, sizeof(MesEdiStruct));
    if(realloc_edi_buff(&MesTxt, EDI_CHNG_SIZE))
    {
        return EDI_MES_ERR;
    }

    while(1)
    {
        int need_size = 0;

        va_start(ap, format);
        MesTxt.len = vsnprintf(MesTxt.mes, MesTxt.size , format, ap);
        va_end(ap);

        if(MesTxt.len>-1 && MesTxt.len<MesTxt.size)
        {
            tst();
            break;
        }

        if(MesTxt.len>0)
        {/*glibc >=2.1*/
            need_size = MesTxt.len + 1;
        }
        else
        {
            need_size = EDI_CHNG_SIZE;
        }

        if(realloc_edi_buff(&MesTxt, need_size))
        {
            return EDI_MES_ERR;
        }
    }

    if(MesTxt.len == 0)
    {
        WriteEdiLog(EDILOG,"Real data length == 0");
        free_edi_buff(&MesTxt);
        return EDI_MES_OK;
    }

    Ret = SetDBFN_(SegGr, SegGrNum,
                   Segm,  SegmNum,
                   Comp,  CompNum,
                   Data,  DataNum, set,
                   MesTxt.mes, MesTxt.len);

    free_edi_buff(&MesTxt);

    return Ret;
}

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
             const char *dataStr, int dataLen)
{
    int err = -1;

    if(*dataStr == '\0'){
        WriteEdiLog(EDILOG,"Data length == 0");
        return 0;
    }

    do{
        if(PushEdiPointW()){
            break;
        }

        if(SegGr){
            if(set && (err = SetEdiSegGr(SegGr, SegGrNum))){
                break;
            }

            if(SetEdiPointToSegGrW(SegGr, SegGrNum)!=1){
                err=-1;
                break;
            }
        }

        if(Segm){
            if(set && (err = SetEdiSegment(Segm, SegmNum))){
                break;
            }

            if(SetEdiPointToSegmentW(Segm, SegmNum)!=1){
                err=-1;
                break;
            }
        }

        if(Comp){
            if(set && (err = SetEdiComposite(Comp, CompNum))){
                break;
            }

            if(SetEdiPointToCompositeW(Comp, CompNum)!=1){
                err=-1;
                break;
            }
        }

        if((err = SetEdiDataElemArrLen(Data, DataNum, dataStr, dataLen))){
            break;
        }

        if(PopEdiPointW()){
            err=-1;
            break;
        }
        err = 0;
    }while(0);

    return err;
}

/*
 Ввод сегмента из строки по формату
 d1+d2:d3+++d4++...
 */
int SetEdiFullSegmentF(const char *segm, int num, const char *format, ...)
{
    va_list ap;
    MesEdiStruct MesTxt;
    int Ret;

    if(!format){
        EdiError(EDILOG,"NULL format pointer");
        return EDI_MES_ERR;
    }

    memset(&MesTxt, 0, sizeof(MesEdiStruct));
    if(realloc_edi_buff(&MesTxt, EDI_CHNG_SIZE)){
        return EDI_MES_ERR;
    }

    while(1)
    {
        int need_size = 0;

        va_start(ap, format);
        MesTxt.len = vsnprintf(MesTxt.mes, MesTxt.size , format, ap);
        va_end(ap);

        if(MesTxt.len>-1 && MesTxt.len<MesTxt.size){
            tst();
            break;
        }

        if(MesTxt.len>0){/*glibc >=2.1*/
            need_size = MesTxt.len + 1;
        }else {
            need_size = EDI_CHNG_SIZE;
        }

        if(realloc_edi_buff(&MesTxt, need_size)){
            return EDI_MES_ERR;
        }
    }
#ifdef _DEBUG_DATA_
    EdiTrace(TRACE4,"Inserting segment %s:%d, body = <%s>, len = %d", segm, num, MesTxt.mes, MesTxt.len);
#endif /*_DEBUG_DATA_*/
    Ret = SetEdiFullSegment(segm, num, MesTxt.mes);

    free_edi_buff(&MesTxt);

    return Ret;
}

/*
 Ввод сегмента из строки
 d1+d2:d3+++d4++...
 */
int SetEdiFullSegment(const char *segm, int num, const char *seg_str)
{
    return SetEdiFullSegment_(GetEdiMesStructW(), segm, num, seg_str);
}

int SetEdiFullSegment_(EDI_REAL_MES_STRUCT *pM, const char *segm,
                       int num, const char *seg_str)
{
    int err = -1;

    do {
        if(PushEdiPointW_(pM)){
            break;
        }

        if((err = SetEdiSegment_(pM, segm, num))){
            break;
        }

        if(SetEdiPointToSegmentW_(pM, segm, num)!=1){
            err=-1;
            break;
        }

        if((err=SetEdiFullCurSegment(pM, seg_str))){
            break;
        }

        if(PopEdiPointW_(pM)){
            err=-1;
            break;
        }
        err = 0;
    }while(0);

    return err;
}

static void DeleteSpecChars(char *data, int *len)
{
    Edi_CharSet Chars;
    memset(&Chars, 0, sizeof(Edi_CharSet));

    Chars.EndComp = '+';
    Chars.EndData = ':';
    Chars.Release = '?';

    DeleteEdiMaskChars(&Chars, data, len);
}

int SetEdiFullCurSegment(EDI_REAL_MES_STRUCT *pM, const char *seg_str)
{
    COMPOSITE_STRUCT *CompTemp=NULL;
    int Num = 0;
    int err=EDI_MES_OK;
    char *psave, *seg;
    seg = psave = strdup(seg_str);

    if(seg == NULL){
        EdiError(EDILOG,"str_dup return NULL");
        return EDI_MES_ERR;
    }

    if (pM->pPoint && pM->pPoint->pTag){
        /*Относительно области видимости*/
        if(pM->WrPoint.pComp){
            CompTemp = pM->WrPoint.pComp->TemplateComp;
        }else {
            CompTemp = pM->pPoint->pTag->TemplateTag->Comp;
        }
    }else {
        /*Нет ограничения обл. видимости*/
        EdiError(EDILOG, "Set composite: EdiPoint must be set to segment");
        free(psave);
        return EDI_MES_ERR;
    }

    while(1){
        char *comp;
        int Ret = GetNextComp_Data(pM, &seg, &comp, '+', '?');

        if(Ret == EDI_MES_NOT_FND){
            break;
        }else {
            if(CompTemp == NULL){
                EdiError(EDILOG, "Too many composites! <%s> cannot be found here!\n", *comp?comp:seg);
                err = EDI_MES_STRUCT_ERR;
                break;
            }
            if(Ret != EDI_MES_OK){
                Num = 0;
                CompTemp = CompTemp->Next;
                continue;
            }
        }

        if(*(CompTemp->Composite)){
            if((Ret = SetEdiFullComposite_(pM, CompTemp->Composite, Num+CompTemp->Num, comp))){
                EdiError(EDILOG,"SetEdiFullComposite_ return %d", Ret);
                err = Ret;
                break;
            }
            if(Num+1 < CompTemp->MaxR){
                Num ++;
                continue;
            }
        }else {
            if(CheckForBadChars(comp, '?', ':')==0){
                EdiError(EDILOG,"Single data element has data element separator <%s>!", comp);
                err = EDI_MES_STRUCT_ERR;
                break;
            }
            /* Remove IATA mask chars here! */
            int datalen = strlen(comp);
            DeleteSpecChars(comp, &datalen);
            if((Ret = SetEdiDataElem__(pM, CompTemp->DataElem->DataElem,
                CompTemp->DataElem->Num, Num, comp, datalen))){
                    EdiError(EDILOG,"SetEdiDataElem return %d", Ret);
                    err = Ret;
                    break;
                }
                if(Num+1 < CompTemp->DataElem->MaxR){
                    Num ++;
                    continue;
                }
        }

        Num = 0;
        CompTemp = CompTemp->Next;
    }

    free(psave);
    return err;
}

/*
 Ввод композита из строки по формату
 d1::d2:d3::...
 */
int SetEdiFullCompositeF(const char *comp, int num, const char *format, ...)
{
    va_list ap;
    MesEdiStruct MesTxt;
    int Ret;

    if(!format){
        EdiError(EDILOG,"NULL format pointer");
        return EDI_MES_ERR;
    }

    memset(&MesTxt, 0, sizeof(MesEdiStruct));
    if(realloc_edi_buff(&MesTxt, EDI_CHNG_SIZE)){
        return EDI_MES_ERR;
    }

    while(1)
    {
        int need_size = 0;

        va_start(ap, format);
        MesTxt.len = vsnprintf(MesTxt.mes, MesTxt.size , format, ap);
        va_end(ap);

        if(MesTxt.len>-1 && MesTxt.len<MesTxt.size){
            tst();
            break;
        }

        if(MesTxt.len>0){/*glibc >=2.1*/
            need_size = MesTxt.len + 1;
        }else {
            need_size = EDI_CHNG_SIZE;
        }

        if(realloc_edi_buff(&MesTxt, need_size)){
            return EDI_MES_ERR;
        }
    }

    EdiTrace(TRACE4,"Inserting composite %s:%d, body = <%s>, len = %d", comp, num, MesTxt.mes, MesTxt.len);

    Ret = SetEdiFullComposite(comp, num, MesTxt.mes);

    free_edi_buff(&MesTxt);

    return Ret;
}

/*
 Ввод композита из строки
 d1::d2:d3::...
 */
int SetEdiFullComposite(const char *comp, int num, const char *comp_str)
{
    return SetEdiFullComposite_(GetEdiMesStructW(), comp, num, comp_str);
}

int SetEdiFullComposite_(EDI_REAL_MES_STRUCT *pM, const char *comp,
                         int num, const char *comp_str)
{
    int err = -1;

    do {
        if(PushEdiPointW_(pM)){
            break;
        }

        if((err = SetEdiComposite_(pM, comp, num))){
            break;
        }

        if(SetEdiPointToCompositeW_(pM, comp, num)!=1){
            err=-1;
            break;
        }

        if((err=SetEdiFullCurComposite(pM, comp_str))){
            break;
        }

        if(PopEdiPointW_(pM)){
            err=-1;
            break;
        }
        err = 0;
    }while(0);

    return err;
}

int SetEdiFullCurComposite(EDI_REAL_MES_STRUCT *pM, const char *comp_str)
{
    DATA_ELEM_STRUCT *DataTemp=NULL;
    int Num = 0;
    int err=EDI_MES_OK;
    char *data;
    char *psave, *comp = psave = strdup(comp_str);

    if(comp == NULL){
        EdiError(EDILOG,"str_dup return NULL");
        return EDI_MES_ERR;
    }

    if (pM->pPoint && pM->pPoint->pComp)
    {
        /*Относительно области видимости*/
        if(pM->WrPoint.pData){
            DataTemp = pM->WrPoint.pData->TemplateData;
        }else {
            DataTemp = pM->pPoint->pComp->TemplateComp->DataElem;
        }
        if(!DataTemp)
        {
            EdiError(EDILOG, "EdiPoint is null");
            return EDI_MES_STRUCT_ERR;
        }
/*        else
        {
            EdiTrace(TRACE4,"Current template data element is %d",
                     DataTemp->DataElem);
        }*/
    }
    else
    {
        /*Нет ограничения обл. видимости*/
        EdiError(EDILOG, "Set composite: EdiPoint must be set to composite");
        free(psave);
        return EDI_MES_ERR;
    }

    while(1)
    {
        int Ret = GetNextComp_Data(pM, &comp, &data, ':', '?');

        if(Ret == EDI_MES_NOT_FND)
        {
            break;
        }
        else
        {
            if(DataTemp == NULL)
            {
                EdiError(EDILOG, "Too many data_elements! <%s> cannot be found here!\n", *data?data:comp);
                err = EDI_MES_STRUCT_ERR;
                break;
            }
            if(Ret != EDI_MES_OK)
            {
                Num = 0;
                DataTemp = DataTemp->Next;
                continue;
            }
        }

        /* Remove IATA chars here */
        int len = strlen(data);
        DeleteSpecChars(data, &len);
        if((Ret = SetEdiDataElem__(pM, DataTemp->DataElem,
            DataTemp->Num, Num, data, len)))
        {
            EdiError(EDILOG,"SetEdiDataElem return %d", Ret);
            err = Ret;
            break;
        }

        if(Num+1 < DataTemp->MaxR)
        {
            Num ++;
            EdiTrace(TRACE3,"Next Num = %d in data elements array",Num);
            continue;
        }

        Num = 0;
        DataTemp = DataTemp->Next;
    }

    free(psave);
    return err;
}


int SetEdiSegGr(int  SegGr, int num)
{
    return SetEdiSegGr_(GetEdiMesStructW(), SegGr, num);
}

int SetEdiSegGr_(EDI_REAL_MES_STRUCT *pM, int SegGr, int num)
{
    TAG_REAL_STRUCT *Tag = pM->Tag;
    TAG_REAL_STRUCT *TagPrev=NULL;
    TAG_STRUCT *TagTemp = pM->pTempMes->Tag;
    int err=EDI_MES_OK;

    if(!SegGr)
        return EDI_MES_OK;

    if(Tag)    { /*Если что-то уже есть ...*/
        if(pM->WrPoint.pTag){
            Tag = pM->WrPoint.pTag;
            TagTemp = Tag->TemplateTag;
        } else if (pM->pPoint && pM->pPoint->pGrStr){
            /*Относительно области видимости*/
            Tag = pM->pPoint->pGrStr;
            TagTemp = Tag->TemplateTag;
        }else if(pM->pPoint && (pM->pPoint->pTag || pM->pPoint->pComp)){
            EdiError(EDILOG, "Set segment group: EdiPoint must be set to segment group or empty");
            return EDI_MES_ERR;
        }
    }

    while(TagTemp)
    {
        int equal_temp=0;

        if(Tag && TagTemp &&
           ((Tag->GrStr && TagTemp->GrStr && Tag->TemplateTag->Num == TagTemp->Num) ||
           (!Tag->GrStr && !TagTemp->GrStr &&
            !memcmp(Tag->TemplateTag->TagName, TagTemp->TagName, TAG_LEN))))
        {
            equal_temp=1;
        }

        if(TagTemp->GrStr && TagTemp->Num == SegGr){
            /*Найдена по шаблону, создаем*/
            TAG_REAL_STRUCT *TagNew=NULL, *TagNewSeg=NULL;
            TAG_REAL_STRUCT *pTagRoot=Tag;

            if(num!=0 && !equal_temp){
                EdiError(EDILOG,"No groups %d found, but num=%d", SegGr, num);
                err = EDI_MES_STRUCT_ERR;
                break;
            }

            if(num >= TagTemp->MaxR){
                EdiError(EDILOG,"Too many segments groups SegGr%d:%d", SegGr, num+1);
                err = EDI_MES_STRUCT_ERR;
                break;
            }

            if(equal_temp){
                int sch=1;
                /*Если реальное сообщение уже сожержит данную группу*/
                if(pM->WrPoint.pGrStrArr
                   && pM->WrPoint.pGrStrArr->TemplateTag->Num==SegGr)
                {
                    Tag = pM->WrPoint.pGrStrArr;
                    sch = pM->WrPoint.pGrStrArr->R;
                }
                while(Tag->NextArr){
                    Tag = Tag->NextArr;
                    sch++;
                }
                TagPrev = Tag;
                if(sch > num) { /*Нашли = уже создан*/
                    WriteEdiLog(EDILOG,"Segment group SegGr%d:%d already exist (%d exists)",
                                SegGr, num, sch);
                    break;
                } else if (num > sch) {
                    EdiError(EDILOG,"Found SegGr%d:%d, but num=%d", SegGr, sch, num);
                    err = EDI_MES_STRUCT_ERR;
                    break;
                }
            }

            if(!CreateNewTagReal(&TagNew) || !CreateNewTagReal(&TagNewSeg)){
                /*Создание новой ячейки для сегм. группы*/
                free(TagNew);
                free(TagNewSeg);
                err = EDI_MES_ERR;
                break;
            }


            if(TagPrev){
                if(equal_temp){
                    TagPrev->NextArr = TagNew;
                    pM->WrPoint.pTag = pTagRoot;
                }else {
                    TagPrev->NextTag = TagNew;
                    TagNew->NextTag = Tag;
                    pM->WrPoint.pTag = TagNew;
                }
            }else {
                pM->Tag = TagNew;
                TagNew->NextTag = Tag;
            }
            pM->WrPoint.pGrStrArr = TagNew;

            TagNew->TemplateTag = TagTemp;
            TagNew->GrStr = TagNewSeg;
            TagNew->R = num+1;

            TagNewSeg->TemplateTag = TagTemp->GrStr;
            break;
        }

        TagTemp = TagTemp->NextTag;
        if(equal_temp)
        {
            TagPrev = Tag;
            Tag = Tag->NextTag;
        }
    }

    if(!TagTemp){
        EdiError(EDILOG,"Segment group %d not found in template", SegGr);
        err = EDI_MES_STRUCT_ERR;
    }

    return err;
}

int SetEdiSegment(const char *segm, int num)
{
    return SetEdiSegment_(GetEdiMesStructW(), segm, num);
}

int SetEdiSegment_(EDI_REAL_MES_STRUCT *pM, const char *segm, int num)
{
    TAG_REAL_STRUCT *Tag = pM->Tag;
    TAG_REAL_STRUCT *TagPrev=NULL;
    TAG_STRUCT *TagTemp = pM->pTempMes->Tag;
    int err=EDI_MES_OK;

    if(!segm || !*segm)
        return EDI_MES_OK;

    if(strlen ( segm ) != TAG_LEN){
        EdiError(EDILOG,"Bad segment name %s", segm);
        return  EDI_MES_STRUCT_ERR;
    }

    if(Tag)    { /*Если что-то уже есть ...*/
        if(pM->WrPoint.pTag){
            Tag = pM->WrPoint.pTag;
            TagTemp = Tag->TemplateTag;
        } else if (pM->pPoint && pM->pPoint->pGrStr){
            /*Относительно области видимости*/
            Tag = pM->pPoint->pGrStr;
            TagTemp = Tag->TemplateTag;
        }else if(pM->pPoint && (pM->pPoint->pTag || pM->pPoint->pComp)){
            EdiError(EDILOG, "Set segment: EdiPoint must be set to segment group or empty");
            return EDI_MES_ERR;
        }
    }

    while(TagTemp){
        int equal_temp=0;

        if(Tag && TagTemp &&
           ((Tag->GrStr && TagTemp->GrStr && Tag->TemplateTag->Num == TagTemp->Num) ||
           (!Tag->GrStr && !TagTemp->GrStr &&
            !memcmp(Tag->TemplateTag->TagName, TagTemp->TagName, TAG_LEN))))
        {
            equal_temp=1;
        }


        if(!TagTemp->GrStr && !memcmp(TagTemp->TagName, segm, TAG_LEN)){
            /*Найден по шаблону, создаем*/
            TAG_REAL_STRUCT *TagNewSeg=NULL;
            TAG_REAL_STRUCT *pTagRoot = Tag;

            if(num!=0 && !equal_temp){
                EdiError(EDILOG,"No segments %s found, but num=%d", segm, num);
                err = EDI_MES_STRUCT_ERR;
                break;
            }

            if(num >= TagTemp->MaxR){
                EdiError(EDILOG,"Too many segments %s:%d", segm, num+1);
                err = EDI_MES_STRUCT_ERR;
                break;
            }

            if(equal_temp)
            {
                int sch=1;
                /*Если реальное сообщение уже сожержит данный сегмент*/

                if(pM->WrPoint.pTagArr
                   && !strcmp(pM->WrPoint.pTagArr->TemplateTag->TagName, segm))
                {
                    Tag = pM->WrPoint.pTagArr;
                    sch = pM->WrPoint.pTagArr->R;
                }

                while(Tag->NextArr){
                    Tag = Tag->NextArr;
                    sch++;
                }
                TagPrev = Tag;
                if(sch > num){ /*Нашли = уже создан*/
                    WriteEdiLog(EDILOG,"Segment %s:%d already exist (%d exists)",
                                segm, num, sch);
                    break;
                }else if (num > sch){
                    EdiError(EDILOG,"Found %s:%d, but num=%d", segm, sch, num);
                    err = EDI_MES_STRUCT_ERR;
                    break;
                }
            }

            if( !CreateNewTagReal(&TagNewSeg) ){
                err = EDI_MES_ERR;
                break;
            }

            if(TagPrev){
                if(equal_temp){
                    TagPrev->NextArr = TagNewSeg;
                    pM->WrPoint.pTag = pTagRoot;
                }else {
                    TagPrev->NextTag = TagNewSeg;
                    TagNewSeg->NextTag = Tag;
                    pM->WrPoint.pTag = TagNewSeg;
                }
            }else {
                pM->Tag = TagNewSeg;
                TagNewSeg->NextTag = Tag;
            }
            pM->WrPoint.pTagArr=TagNewSeg;

            TagNewSeg->TemplateTag = TagTemp;
            TagNewSeg->R = num+1;
#ifdef _DEBUG_DATA_
            EdiTrace(TRACE3,"Add new tag %s in the message", TagNewSeg->TemplateTag->TagName);
#endif /*_DEBUG_DATA_*/
            break;
        }

        TagTemp = TagTemp->NextTag;
        if(equal_temp){
            TagPrev = Tag;
            Tag = Tag->NextTag;
        }
    }

    if(!TagTemp){
        EdiError(EDILOG,"Segment %s not found in template", segm);
        err = EDI_MES_STRUCT_ERR;
    }

    if(err==EDI_MES_OK){
        ResetEdiFind_(pM);
    }

    return err;
}

int SetEdiComposite(const char *comp, int num)
{
    return SetEdiComposite_(GetEdiMesStructW(), comp, num);
}

int SetEdiComposite_(EDI_REAL_MES_STRUCT *pM, const char *comp, int num)
{
    COMP_REAL_STRUCT *Comp    =NULL;
    COMP_REAL_STRUCT *CompPrev=NULL;
    TAG_REAL_STRUCT  *TagRoot =NULL;
    COMPOSITE_STRUCT *CompTemp=NULL;
    int err=EDI_MES_OK;

    if(!comp || !*comp)
        return EDI_MES_OK;

    if(strlen ( comp ) != COMP_LEN){
        EdiError(EDILOG,"Bad composite name %s", comp);
        return  EDI_MES_STRUCT_ERR;
    }

    if (pM->pPoint && pM->pPoint->pTag){
        /*Относительно области видимости*/
        TagRoot = pM->pPoint->pTag;
        if(pM->WrPoint.pComp){
            Comp = pM->WrPoint.pComp;
            CompTemp = pM->WrPoint.pComp->TemplateComp;
        }else {
            Comp = TagRoot->Comp;
            CompTemp = TagRoot->TemplateTag->Comp;
        }
    }else {
        /*Нет ограничения обл. видимости*/
        EdiError(EDILOG, "Set composite: EdiPoint must be set to segment");
        return EDI_MES_ERR;
    }

    while(CompTemp){
        int equal_temp=0;

        if(Comp && CompTemp &&
           ((!Comp->SingleData && !strcmp(Comp->TemplateComp->Composite, CompTemp->Composite)) ||
           (Comp->SingleData && (Comp->DataElem->DataElem == CompTemp->DataElem->DataElem))))
        {
            equal_temp=1;
        }


        if(*(CompTemp->Composite) && !memcmp(CompTemp->Composite, comp, COMP_LEN) &&
             !((num!=CompTemp->Num && !equal_temp) || num >= CompTemp->MaxR+CompTemp->Num))
        {
            /*Найден по шаблону, создаем*/
            COMP_REAL_STRUCT *CompNew=NULL;
            COMP_REAL_STRUCT *CompRoot=Comp;

        /*
         Может быть такой вариант:
         TVL
         C328 C1 | C328 C(X)
         C328 C1 | C328 C(Y)
         вместо
         C328 C2 | C328 C(X+Y)

         Выход такой:
         В памяти выгл. будет все в след виде
         C328 R1 | C328 R1 ->(NextArr) C328 R2 -> ... ->C328 R(X)
         |(Next) | |(Next)
             \/      | \/
         C328 R2 | C328 R(X+1) ->(NextArr) C328 R(X+2) -> ... ->C328 R(X+Y)
         Во как ...
             27.09.2004

         Не на фиг..
         Фсе будет объединяца вместе !!!!
         Т.е C328 C1
         C328 C1 ... ----> C328 C2 Еще на этапе загрузки шаблона
             01.10.2004

         Не на фиг...
         Фсе будет по-старому... -(
         Минусы - нет возможности обратиться к конкретному эл. данных,
         композиту в перечисленных выше случаях.
         Оказывается что бывает так, что композиты, эл. данных в составе
         композита/сегмента могут микшироваться в произвольном порядке -(
         Т.е. м.б. так
         !C328 C1!         !1234 C1!
          C333 C1    или    4321 C1
             !C328 C4!         !1234 C1!

         Доступ к ним будет осуществяться пока по-старому: по номеру из
         общей кучи.
             04.10.2004
         */

            if(equal_temp){
                /*Если реальное сообщение уже сожержит данный композит*/

                if(pM->WrPoint.pCompArr
                   && !strcmp(pM->WrPoint.pCompArr->TemplateComp->Composite, comp)){
                    Comp = pM->WrPoint.pCompArr;
                   }

                   while(Comp->NextArr){
                       Comp = Comp->NextArr;
                   }
                   CompPrev = Comp;
                   if(Comp->R+CompTemp->Num > num){ /*Нашли = уже создан*/
                       WriteEdiLog(EDILOG,"Composite %s:%d already exist (%d exists)",
                                   comp, num, Comp->R+CompTemp->Num);
                       break;
                   }else if (num > Comp->R+CompTemp->Num){
                       EdiError(EDILOG,"Found %s:%d, but num=%d", comp, Comp->R+CompTemp->Num, num);
                       err = EDI_MES_STRUCT_ERR;
                       break;
                   }
            }

            if( !CreateNewCompReal(&CompNew) ){
                err = EDI_MES_ERR;
                break;
            }

            if(CompPrev){
                if(equal_temp){
                    CompPrev->NextArr = CompNew;
                    pM->WrPoint.pComp = CompRoot;
                }else {
                    CompPrev->Next = CompNew;
                    CompNew->Next = Comp;
                    pM->WrPoint.pComp = CompNew;
                }
            }else {
                TagRoot->Comp = CompNew;
                CompNew->Next = Comp;
            }
            pM->WrPoint.pCompArr = CompNew;

            CompNew->TemplateComp = CompTemp;
            CompNew->R = num+1-CompTemp->Num;

#ifdef _DEBUG_DATA_
            EdiTrace(TRACE3,"Add new composite %s:%d in the message", CompNew->TemplateComp->Composite, CompNew->R-1+CompTemp->Num);
#endif /*_DEBUG_DATA_*/
            break;
        }

        CompTemp = CompTemp->Next;
        if(equal_temp){
            CompPrev = Comp;
            Comp = Comp->Next;
        }
    }

    if(!CompTemp){
        EdiError(EDILOG,"Composite %s:%d not found in template", comp, num);
        err = EDI_MES_STRUCT_ERR;
    }

    if(err==EDI_MES_OK){
        ResetEdiFind_(pM);
    }

    return err;
}

int SetEdiDataElemArrF(int dataNum, int num, const char *format, ...)
{
    int len = 0;
    va_list ap;
    MesEdiStruct MesTxt;
    int Ret;

    if(!format){
        EdiError(EDILOG,"NULL format pointer");
        return EDI_MES_ERR;
    }

    memset(&MesTxt, 0, sizeof(MesEdiStruct));
    if(realloc_edi_buff(&MesTxt, EDI_CHNG_SIZE)){
        return EDI_MES_ERR;
    }

    while(1)
    {
        int need_size = 0;

        va_start(ap, format);
        len = vsnprintf(MesTxt.mes, MesTxt.size , format, ap);
        va_end(ap);

        if(len>-1 && len<MesTxt.size){
            tst();
            break;
        }

        if(len>0){/*glibc >=2.1*/
            need_size = len + 1;
        }else {
            need_size = EDI_CHNG_SIZE;
        }

        if(realloc_edi_buff(&MesTxt, need_size)){
            return EDI_MES_ERR;
        }
    }

    if(len == 0){
        EdiTrace(TRACE5,"Real data length == 0");
        free_edi_buff(&MesTxt);
        return EDI_MES_OK;
    }

    Ret = SetEdiDataElem_(GetEdiMesStructW(), dataNum, num, MesTxt.mes, len);

    free_edi_buff(&MesTxt);

    return Ret;
}


int SetEdiDataElemF(int dataNum, const char *format, ...)
{
    int len = 0;
    va_list ap;
    MesEdiStruct MesTxt;
    int Ret;

    if(!format){
        EdiError(EDILOG,"NULL format pointer");
        return EDI_MES_ERR;
    }

    memset(&MesTxt, 0, sizeof(MesEdiStruct));
    if(realloc_edi_buff(&MesTxt, EDI_CHNG_SIZE)){
        return EDI_MES_ERR;
    }

    while(1)
    {
        int need_size = 0;

        va_start(ap, format);
        len = vsnprintf(MesTxt.mes, MesTxt.size , format, ap);
        va_end(ap);

        if(len>-1 && len<MesTxt.size){
            tst();
            break;
        }

        if(len>0){/*glibc >=2.1*/
            need_size = len + 1;
        }else {
            need_size = EDI_CHNG_SIZE;
        }

        if(realloc_edi_buff(&MesTxt, need_size)){
            return EDI_MES_ERR;
        }
    }

    if(len == 0){
        EdiTrace(TRACE5,"Real data length == 0");
        free_edi_buff(&MesTxt);
        return EDI_MES_OK;
    }

    Ret = SetEdiDataElem_(GetEdiMesStructW(), dataNum, 0, MesTxt.mes, len);

    free_edi_buff(&MesTxt);

    return Ret;
}

int SetEdiDataElemArrLen(int dataNum, int num, const char *data, int len)
{
    return SetEdiDataElem_(GetEdiMesStructW(), dataNum, num, data, len);
}

int SetEdiDataElemLen(int dataNum, const char *data, int len)
{
    return SetEdiDataElem_(GetEdiMesStructW(), dataNum, 0, data, len);
}

int SetEdiDataElem(int dataNum, const char *data)
{
    return SetEdiDataElem_(GetEdiMesStructW(), dataNum, 0, data, -1);
}

int SetEdiDataElemArr(int dataNum, int num, const char *data)
{
    return SetEdiDataElem_(GetEdiMesStructW(), dataNum, num, data, -1);
}

int SetEdiDataElem_(EDI_REAL_MES_STRUCT *pM, int data, int num, const char *dataStr, int data_len)
{
    return SetEdiDataElem__(pM, data, -1, num, dataStr, data_len);
}

int SetEdiDataElem__(EDI_REAL_MES_STRUCT *pM, int data, int numseq, int num,
                     const char *dataStr, int data_len)
{
    DATA_REAL_STRUCT *Data    =NULL;
    DATA_REAL_STRUCT *DataPrev=NULL;

    COMP_REAL_STRUCT *Comp    =NULL;
    COMP_REAL_STRUCT *CompPrev=NULL;

    TAG_REAL_STRUCT  *TagRoot =NULL;
    COMP_REAL_STRUCT *CompRoot=NULL;

    DATA_ELEM_STRUCT *DataTemp=NULL;
    COMPOSITE_STRUCT *CompTemp=NULL;

    int err=EDI_MES_OK;

    if(!data)
        return EDI_MES_OK;
    if(numseq < 0)
        numseq = 0;

    if( data<0 )
    {
        EdiError(EDILOG,"Bad data element number %d", data);
        return  EDI_MES_STRUCT_ERR;
    }

    if(!dataStr || !*dataStr)
    {
        EdiTrace(TRACE5,"Data string for the data element %d is null or has length=0",
                 data);
        return EDI_MES_OK;
    }

    if (pM->pPoint && (pM->pPoint->pTag || pM->pPoint->pComp))
    {
        /*Относительно области видимости*/
        if(pM->pPoint->pTag)
        {
            TagRoot = pM->pPoint->pTag;
            if(pM->WrPoint.pComp)
            {
                Comp = pM->WrPoint.pComp;
                CompTemp = Comp->TemplateComp;
            }
            else
            {
                Comp = TagRoot->Comp;
                CompTemp = TagRoot->TemplateTag->Comp;
            }
        }
        else
        {
            CompRoot = pM->pPoint->pComp;
            if(pM->WrPoint.pData)
            {
                Data = pM->WrPoint.pData;
                DataTemp = Data->TemplateData;
            }
            else
            {
                Data     = CompRoot->DataElem;
                DataTemp = CompRoot->TemplateComp->DataElem;
            }
        }
    }
    else
    {
        /*Нет ограничения обл. видимости*/
        EdiError(EDILOG, "Set data element: EdiPoint must be set to segment/composite");
        return EDI_MES_STRUCT_ERR;
    }

    while(DataTemp || CompTemp)
    {
        int equal_temp = 0;
        int found_by_num = 0;

        if((Data && DataTemp && (Data->TemplateData->DataElem == DataTemp->DataElem) &&
            Data->TemplateData->Num == DataTemp->Num) ||
            (Comp && CompTemp &&
            ((Comp->SingleData && (Comp->DataElem->DataElem == CompTemp->DataElem->DataElem)) ||
            (!Comp->SingleData && !strcmp(Comp->TemplateComp->Composite, CompTemp->Composite))))
          )
        {
            equal_temp=1;
        }


        if((CompTemp && !*(CompTemp->Composite) &&
            (CompTemp->DataElem->DataElem == data)
            && !((numseq!=CompTemp->DataElem->Num && !equal_temp) ||
            (numseq >= CompTemp->DataElem->Num + CompTemp->DataElem->MaxR))) ||

            (DataTemp && (DataTemp->DataElem == data)
            && !((numseq != DataTemp->Num && !equal_temp) ||
                  numseq >= DataTemp->Num + DataTemp->MaxR) ))
        {
            found_by_num = 1;
        }

#ifdef _DEBUG_DATA_
        if(DataTemp) {
            EdiTrace(TRACE5,"data = %d, DataTemp->DataElem = %d, "
                            "num = %d, numseq = %d, DataTemp->Num=%d, DataTemp->MaxR=%d,"
                            "equal_temp = %d, found_by_num = %d",
                 data, DataTemp->DataElem, num, numseq, DataTemp->Num, DataTemp->MaxR,
                 equal_temp, found_by_num);
        } else {
            EdiTrace(TRACE5,"data = %d, CompTemp->DataElem->DataElem = %d, "
                            "num = %d, numseq = %d, CompTemp->DataElemDataTemp->Num=%d, DataTemp->MaxR=%d,"
                            "equal_temp = %d, found_by_num = %d",
                 data, CompTemp->DataElem->DataElem, num, numseq, CompTemp->DataElem->Num, CompTemp->DataElem->MaxR,
                 equal_temp, found_by_num);
        }
#endif /* _DEBUG_DATA_ */

        if(found_by_num)
        {
            /*Найден по шаблону, создаем*/
            COMP_REAL_STRUCT *CompNew=NULL, *CompRoot2 = Comp;
            DATA_REAL_STRUCT *DataNew=NULL, *DataRoot2 = Data;

            if(equal_temp){
                /*Если реальное сообщение уже сожержит этот эл.данных*/
                if(pM->WrPoint.pDataArr
                   && pM->WrPoint.pDataArr->DataElem == data)
                {
                    Data = pM->WrPoint.pDataArr;
                }
                else if(Comp)
                {
                    Data = Comp->DataElem;
                }

                if(Data == NULL)
                {
                    EdiError(EDILOG,"Comp->DataElem == NULL");
                    return EDI_MES_ERR;
                }

                while(Data->NextArr)
                {
                    Data = Data->NextArr;
                }
                DataPrev = Data;

                if(Data->R > num)
                { /*Нашли = уже создан*/
                    EdiTrace(TRACE2, "DataElement %d:%d:%d already exist (%d exists)",
                                data, num, Data->R, Data->Num);
                    break;
                }
                else if (num > Data->R)
                {
                    EdiError(EDILOG,"Found %d:%d:%d, but num=%d",
                             data, Data->R, Data->Num, num);
                    err = EDI_MES_STRUCT_ERR;
                    break;
                }
            }

            if( CompTemp && !equal_temp && !CreateNewCompReal(&CompNew) )
            {
                err = EDI_MES_ERR;
                break;
            }

            if(!CreateNewDataReal(&DataNew))
            {
                free(CompNew);
                err = EDI_MES_ERR;
                break;
            }

            if(CompNew)
            {
                CompNew->SingleData = 1;
                CompNew->DataElem = DataNew;
                CompNew->TemplateComp = CompTemp;
                if(CompPrev)
                {
                    CompPrev->Next = CompNew;
                }
                else
                {
                    TagRoot->Comp = CompNew;
                }
                CompNew->Next = Comp;

                pM->WrPoint.pComp = CompNew;
            }
            else
            {
                /*Если елемент дан. с таким именем есть(но не номером)*/
                /*или вставляем эл.д. в составе композита*/
                if(DataPrev)
                {
                    if(equal_temp)
                    { /*След. в массиве*/
                        DataPrev->NextArr = DataNew;
                        if(CompTemp)
                        {
                            pM->WrPoint.pComp = CompRoot2;
                        }
                        else
                        {
                            pM->WrPoint.pData = DataRoot2;
                        }
                    }
                    else
                    {
                        DataPrev->Next = DataNew;
                        DataNew->Next = Data;
                        pM->WrPoint.pData = DataNew;
                    }
                }
                else
                {
                    CompRoot->DataElem = DataNew;
                    DataNew->Next = Data;
                    pM->WrPoint.pData = DataNew;
                }

            }

            pM->WrPoint.pDataArr = DataNew;

            if(DataTemp)
            {
                DataNew->TemplateData = DataTemp;
            }
            else
            {
                DataNew->TemplateData = CompTemp->DataElem;
            }

            DataNew->R = num + 1;
            DataNew->Num = numseq;
            DataNew->len  = (data_len < 0) ? strlen(dataStr) : data_len;
            DataNew->DataElem = data;
            DataNew->malloc = 1;
            DataNew->Data = static_cast<char *>(edi_malloc(DataNew->len + 1));
            if(DataNew->Data == NULL)
            {
                EdiError(EDILOG, "Error in edi_malloc (%d+1)", DataNew->len);
                return EDI_MES_ERR;
            }
            memcpy((char *)DataNew->Data, dataStr, DataNew->len + 1);

            if(CheckDataByRuleInsert(pM, DataNew->TemplateData, DataNew)!=EDI_MES_OK)
            {
                EdiError(EDILOG,"Error in data format");
                return EDI_MES_ERR;
            }

#ifdef _DEBUG_DATA_
            EdiTrace(TRACE3,"Add new data element %d:%d:%d <%s> in the message",
                     DataNew->DataElem,
                     DataNew->R - 1, DataNew->Num,
                     DataNew->Data);
#endif /*_DEBUG_DATA_*/
            break;
        }

        if(CompTemp)
        {
            CompTemp = CompTemp->Next;
        }
        else
        {
            DataTemp = DataTemp->Next;
        }
        if(equal_temp)
        {
            if(Comp)
            {
                CompPrev = Comp;
                Comp = Comp->Next;
            }
            else
            {
                DataPrev = Data;
                Data = Data->Next;
            }
        }
    }

    if(!DataTemp && !CompTemp)
    {
        EdiError(EDILOG,"Data element %d:%d:%d not found in template", data, num, numseq);
        err = EDI_MES_STRUCT_ERR;
    }

    if(err==EDI_MES_OK)
    {
        ResetEdiFind_(pM);
    }

    return err;
}


/*
 * Проверяет правильность структуры сообщения (согласно соотв. шаблону)
 * Расставляет пропуски эл. данных, композитов
 * return EDI_MES_STRUCT_ERR - ош. в структуре
 * return EDI_MES_ERR - ошибка программы
 * return EDI_MES_OK - Ok
 */
int SetSpacesToEdiMessage(void)
{
    return SetSpacesToEdiMessage_(GetEdiMesStructW());
}

int SetSpacesToEdiMessage_(EDI_REAL_MES_STRUCT *pMes)
{
    int ret;

    if(pMes == NULL || pMes->pTempMes == NULL || pMes->Tag == NULL){
        EdiError(EDILOG,"Pointer is NULL");
        return EDI_MES_ERR;
    }

    ret = CheckEdiTags(pMes, pMes->Tag, pMes->pTempMes->Tag, 1, 0);
    return ret;
}

int CheckEdiTags(EDI_REAL_MES_STRUCT *pCurMes, TAG_REAL_STRUCT *Tag ,TAG_STRUCT *TagTemp, int set, int arr)
{
    int Ret;

    tst();

    while( 1 ) {
        tst();

        if(Tag == NULL){
            /*Проверяем остатки шаблона*/
            return CheckTagsInsert(pCurMes, TagTemp);
        }

        if(TagTemp == NULL) {
            EdiError(EDILOG,
                     "Tag %.*s is not found in the template",
                     TAG_LEN, Tag->TemplateTag->TagName);
#ifdef EDI_DEBUG
            return EDI_MES_OK;
#else
            return EDI_MES_STRUCT_ERR;
#endif
        }

        while( 1 ){
            if(TagTemp->GrStr == NULL && Tag->GrStr == NULL)
            {
                if(!memcmp(Tag->TemplateTag->TagName,TagTemp->TagName,TAG_LEN))
                {

                    if(Tag->R > TagTemp->MaxR)
                    {
                        EdiError(EDILOG,"Too many segments %s (%d max)", TagTemp->TagName, TagTemp->MaxR);
#ifdef EDI_DEBUG
                        return EDI_MES_OK;
#else
                        return EDI_MES_STRUCT_ERR;
#endif
                    }

                    Ret = CheckEdiComposites(pCurMes, &Tag->Comp, TagTemp->Comp, set, 0);
                    if( Ret != EDI_MES_OK ){
                        if(Ret == EDI_MES_STRUCT_ERR){
                            EdiError(EDILOG,"Error in %s segment", TagTemp->TagName);
                        }
                        return Ret;
                    }

                    if(Tag->NextArr)
                    {
                        Ret = CheckEdiTags(pCurMes, Tag->NextArr, TagTemp, set, 1);
                        if( Ret != EDI_MES_OK ){
                            return Ret;
                        }
                    }

                    if(arr){
                        /*Проверялся массив сегментов/сем. групп*/
                        return EDI_MES_OK;
                    }

                    TagTemp = TagTemp->NextTag;
                    break;
                }else if(arr){
                    EdiError(EDILOG,"Bad segment %s in array of segments %s",
                             Tag->TemplateTag->TagName,TagTemp->TagName);
                    return EDI_MES_STRUCT_ERR;
                }

            }
            else if(TagTemp->GrStr != NULL && Tag->GrStr != NULL)
            {
                if(!memcmp(Tag->GrStr->TemplateTag->TagName,(TagTemp->GrStr)->TagName,TAG_LEN))
                {
                    /*TagTemp->GrStr != NULL*/

                    if(Tag->R > TagTemp->MaxR){
                        EdiError(EDILOG,"Too many segment groups %s (%d max)",
                                 TagTemp->GrStr->TagName, TagTemp->MaxR);
#ifdef EDI_DEBUG
                        return EDI_MES_OK;
#else
                        return EDI_MES_STRUCT_ERR;
#endif
                    }

                    if((Ret = CheckEdiTags(pCurMes, Tag->GrStr, TagTemp->GrStr, set, 0))){
                        return Ret;
                    }

                    if(Tag->NextArr){
                        Ret = CheckEdiTags(pCurMes, Tag->NextArr, TagTemp, set, 1);
                        if(Ret != EDI_MES_OK)
                            return Ret;
                    }

                    if(arr){
                        /*Проверялся массив сегментов/сем. групп*/
                        return EDI_MES_OK;
                    }

                    TagTemp = TagTemp->NextTag;
                    break;
                }
                else if(arr)
                {
                    EdiError(EDILOG,"Bad segment group %s:%d in array of segments groups %s:%d",
                             Tag->GrStr->TemplateTag->TagName, Tag->TemplateTag->Num,
                             (TagTemp->GrStr)->TagName, TagTemp->Num);
                    return EDI_MES_STRUCT_ERR;
                }
            }

            if ( (TagTemp->NextTag == NULL      ) ||
                 (TagTemp->S       == MANDATORY )  )
            {
                if(TagTemp->NextTag == NULL){
                    if(Tag->GrStr){ /*Сегм Гр*/
                        EdiError(EDILOG,
                                 "SegGr %d (%s) is not found in the template",
                                 Tag->TemplateTag->Num, Tag->GrStr->TemplateTag->TagName);
                    }else {
                        EdiError(EDILOG,
                                 "Tag %.*s is not found in the template",
                                 TAG_LEN, Tag->TemplateTag->TagName);
                    }
                }else {
                    if(TagTemp->Num){ /*Сегм Гр*/
                        EdiError(EDILOG,
                                 "SegGr %d (%s) mandatory but not found\n",
                                 TagTemp->Num, (TagTemp->GrStr)->TagName);
                    }else {
                        EdiError(EDILOG,
                                 "Tag %s mandatory but not found\n",
                                 TagTemp->TagName);
                    }
                }
#ifdef EDI_DEBUG
                if(TagTemp->NextTag == NULL)
                    return EDI_MES_OK;
                else {
                    TagTemp = TagTemp->NextTag;
                    break;
                }
#else
                return EDI_MES_STRUCT_ERR;
#endif
            }
            TagTemp = TagTemp->NextTag;
        }/*end while*/
        Tag = Tag->NextTag;
    }/*end while*/

}

int CheckEdiComposites(EDI_REAL_MES_STRUCT *pCurMes, COMP_REAL_STRUCT **ppComp,
                       COMPOSITE_STRUCT *CompTemp, int set, int arr)
{
    int Ret;
    COMP_REAL_STRUCT *Comp = *ppComp, *CompPrev = *ppComp;

    while ( 1 ) {

        if(Comp == NULL){
            return CheckCompInsert(pCurMes, CompTemp);
        }

        if(CompTemp == NULL) {
            if(Comp->TemplateComp->Composite[0]){
                EdiError(EDILOG,
                         "Composite %s is not found in the template",
                         Comp->TemplateComp->Composite);
            }else {
                EdiError(EDILOG,
                         "Single data element %d is not found in the template",
                         Comp->DataElem->TemplateData->DataElem);
            }
#ifdef EDI_DEBUG
            return EDI_MES_OK;
#else
            return EDI_MES_STRUCT_ERR;
#endif
        }

        while( 1 ){

            if(!Comp->SingleData){

                if(!memcmp(Comp->TemplateComp->Composite,CompTemp->Composite,COMP_LEN) &&
                    Comp->TemplateComp->Num == CompTemp->Num)
                {
                    if(Comp->R > CompTemp->MaxR)
                    {
                        EdiError(EDILOG,"Too many composites %s (%d max)",
                                 CompTemp->Composite, CompTemp->MaxR);
#ifdef EDI_DEBUG
                        return EDI_MES_OK;
#else
                        return EDI_MES_STRUCT_ERR;
#endif
                    }

                    Ret = CheckEdiDataElems(pCurMes, &Comp->DataElem, CompTemp->DataElem, set, 0);
                    if( Ret != EDI_MES_OK ){
                        if(Ret == EDI_MES_STRUCT_ERR){
                            EdiError(EDILOG,"Error in %s composite", CompTemp->Composite);
                        }
                        return Ret;
                    }

                    if(Comp->NextArr){
                        Ret = CheckEdiComposites(pCurMes, &Comp->NextArr, CompTemp, set, 1);
                        if( Ret != EDI_MES_OK ){
                            return Ret;
                        }
                    }

                    if(arr){
                        /*Проверялся массив композитов*/
                        return EDI_MES_OK;
                    }

                    CompTemp = CompTemp->Next;
                    break;
                    }
            }
            else
            { /*Data element whithout composite*/
                if(Comp->DataElem->TemplateData->DataElem == CompTemp->DataElem->DataElem &&
                   Comp->DataElem->TemplateData->Num == CompTemp->DataElem->Num)
                {
                    Ret = CheckEdiDataElems(pCurMes, &Comp->DataElem, CompTemp->DataElem, set, 0);
                    if( Ret != EDI_MES_OK )
                    {
                        return Ret;
                    }

                    CompTemp = CompTemp->Next;
                    break;
                }
            }
            if ( (CompTemp->Next == NULL      ) ||
                  (CompTemp->S    == MANDATORY )  )
            {
                if(CompTemp->Next == NULL){
                    if(Comp->SingleData){
                        EdiError(EDILOG,
                                 "DataElement %d is not found in the template",
                                 Comp->DataElem->DataElem);
                    }else {
                        EdiError(EDILOG,
                                 "Composite %s is not found in the template",
                                 Comp->TemplateComp->Composite);
                    }
                }else {
                    if(*CompTemp->Composite){
                        EdiError(EDILOG,
                                 "Composite %s mandatory but not found",
                                 CompTemp->Composite);
                    }else {
                        EdiError(EDILOG,
                                 "DataElement %d mandatory but not found",
                                 CompTemp->DataElem->DataElem);
                    }
                }
#ifdef EDI_DEBUG
                if(CompTemp->NextTag == NULL)
                    return EDI_MES_OK;
                else {
                    CompTemp = CompTemp->NextTag;
                    break;
                }
#else
                return EDI_MES_STRUCT_ERR;
#endif
            }

            if(set)
            {
                COMP_REAL_STRUCT *CompTmp;

                if( (CompTmp = (COMP_REAL_STRUCT *)
                     calloc(1, sizeof(COMP_REAL_STRUCT))) == NULL)
                {
                    EdiError(EDILOG,"Can't allocate %zu bytes\n", sizeof(COMP_REAL_STRUCT));
                    return EDI_MES_ERR;
                }

                if(*ppComp == Comp)
                {
                    *ppComp = CompTmp;
                }
                else
                {
                    CompPrev->Next = CompTmp;
                }

                CompTmp->TemplateComp = CompTemp;
                CompTmp->Next = Comp;
                CompPrev = CompTmp;
            }

            CompTemp = CompTemp->Next;
        }/*end while*/
        CompPrev = Comp;
        Comp = Comp->Next;
    }
}

int CheckEdiDataElems(EDI_REAL_MES_STRUCT *pCurMes, DATA_REAL_STRUCT **ppDataElem, DATA_ELEM_STRUCT *DataTemp, int set, int arr)
{
    DATA_REAL_STRUCT *DataElem = *ppDataElem;
    DATA_REAL_STRUCT *DataPrev = *ppDataElem;

    while(1){
        if(DataElem == NULL)
        {
            return CheckDataInsert(pCurMes, DataTemp);
        }

        if(DataTemp == NULL)
        {
            EdiError(EDILOG,"Data element %d is not found in the template",
                     DataElem->TemplateData->DataElem);
#ifdef EDI_DEBUG
            return EDI_MES_OK;
#else
            return EDI_MES_STRUCT_ERR;
#endif
        }

        while( 1 )
        {
            if(DataElem->TemplateData->DataElem == DataTemp->DataElem
              && DataElem->TemplateData->Num == DataTemp->Num)
            {
                if(DataElem->R > DataTemp->MaxR)
                {
                    EdiError(EDILOG,"Too many data elements %d:%d (%d max)",
                             DataTemp->DataElem, DataElem->R, DataTemp->MaxR);
#ifdef EDI_DEBUG
                    return EDI_MES_OK;
#else
                    return EDI_MES_STRUCT_ERR;
#endif
                }

                if(DataElem->NextArr){
                    int Ret = CheckEdiDataElems(pCurMes, &DataElem->NextArr, DataTemp, set, 1);
                    if(Ret != EDI_MES_OK)
                        return Ret;
                }

                if(arr){
                    /*Проверялся массив эл. данных*/
                    return EDI_MES_OK;
                }

                DataTemp = DataTemp->Next;
                break;
            }
            else if(arr)
            {
                EdiError(EDILOG,"Bad data element %d in the array of data %d",
                         DataElem->TemplateData->DataElem, DataTemp->DataElem);
            }

            if ( (DataTemp->Next == NULL      ) ||
                  (DataTemp->S    == MANDATORY )  )
            {
                if(DataTemp->Next == NULL)
                {
                    EdiError(EDILOG,
                             "Data element %d is not found in the template",
                             DataElem->TemplateData->DataElem);
                }else {
                    EdiError(EDILOG,
                             "Data element %d mandatory but not found\n",
                             DataTemp->DataElem);
                }
#ifdef EDI_DEBUG
                if(DataTemp->Next == NULL)
                    return EDI_MES_OK;
                else {
                    DataTemp = DataTemp->Next;
                    break;
                }
#else
                return EDI_MES_STRUCT_ERR;
#endif
            }

            if(set)
            {
                DATA_REAL_STRUCT *DataTmp;

                if( (DataTmp = (DATA_REAL_STRUCT *)
                     calloc(1, sizeof(DATA_REAL_STRUCT))) == NULL)
                {
                    EdiError(EDILOG,"Can't allocate %zu bytes\n",sizeof(DATA_REAL_STRUCT));
                    return EDI_MES_ERR;
                }

                if(*ppDataElem == DataElem)
                {
                    *ppDataElem = DataTmp;
                }
                else
                {
                    DataPrev->Next = DataTmp;
                }

                DataTmp->TemplateData = DataTemp;
                DataTmp->Next = DataElem;
                DataPrev=DataTmp;
            }

            DataTemp = DataTemp->Next;
        }/*end while*/

        DataPrev = DataElem;
        DataElem = DataElem->Next;
    }
}

int MakeUntUnzInEdiMes_(EDI_REAL_MES_STRUCT *pMes)
{
    int err = EDI_MES_STRUCT_ERR;

    if(ResetEdiPointW_(pMes))
    {
        return  EDI_MES_ERR;
    }

    do {
        /*counter = количество сегм в сообщении, исключая UNB и UNZ*/
        int seg_count = CountEdiSegments_(pMes);
        const char *our_ref     = GetDBFN_(pMes, 0,0, "UNB",0, NULL, 0, 20, 0);
        const char *mes_ref_num = GetDBFN_(pMes, 0,0, "UNH",0, NULL, 0, 62, 0);

        if(seg_count<=0)
        {
            EdiError(EDILOG,"Counter of the edifact segments = %d", seg_count);
            err = EDI_MES_ERR;
            break;
        }

        if(GetNumSegment_(pMes, "UNT")!=0 ||
           GetNumSegment_(pMes, "UNZ")!=0)
        {
            EdiError(EDILOG, "Tags UNT/UNZ are already exists");
            break;
        }

        if(our_ref == NULL || mes_ref_num == NULL)
        {
            EdiError(EDILOG, "UNB/UNH or our_ref/mes_ref_num not found");
            break;
        }

        if(SetDBFNF(0,0, "UNT",0, NULL,0, 74,0, 1, "%d", seg_count) ||
           SetDBFN (0,0, "UNT",0, NULL,0, 62,0, mes_ref_num) ||
           /*Только одно сообщение в рамках одного обмена мб у нас*/
           SetDBFNs(0,0, "UNZ",0, NULL,0, 36,0, "1") ||
           SetDBFN (0,0, "UNZ",0, NULL,0, 20,0, our_ref) )
        {
            EdiError(EDILOG,"Collect UNT/UNZ segments failed");
            err = EDI_MES_ERR;
            break;
        }

        err = EDI_MES_OK;
    } while(0);

    return err;
}

/*
 Преобразует ст-ру сообщения в текст EDIFACT
 Выделяет память
 return len >= 0 -Ok
 return < 0 - Error
 */
int WriteEdiMessage(char **buff)
{
    return WriteEdiMessage_(GetEdiMesStructW(), buff);
}

/*
 Преобразует ст-ру сообщения в текст EDIFACT
 Использует переданный буфер
 return len >= 0 -Ok
 return < 0 - Error или маленький буфер
 */
int WriteEdiMessageStat(char *buff, int bsize)
{
    return WriteEdiMessageStat_(GetEdiMesStructW(), buff, bsize);
}

int WriteEdiMessageStat_(EDI_REAL_MES_STRUCT *pMes, char *buff, int bsize)
{
    MesEdiStruct MesTxt;
    int Ret;

    memset(&MesTxt, 0, sizeof(MesEdiStruct));

    if(WriteEdiMessage__(pMes, &MesTxt) != EDI_MES_OK)
    {
        free_edi_buff(&MesTxt);
        return -1;
    }

    if(MesTxt.len >= bsize)
    {
        EdiError(EDILOG,"Buffer is too small");
        Ret = -1;
    }
    else
    {
        memcpy(buff, MesTxt.mes, MesTxt.len+1);
        Ret = MesTxt.len;
    }

    free_edi_buff(&MesTxt);
    return Ret;
}

int WriteEdiMessage_(EDI_REAL_MES_STRUCT *pMes, char **buff)
{
    MesEdiStruct MesTxt;
    int Ret;

    memset(&MesTxt, 0, sizeof(MesEdiStruct));

    if(WriteEdiMessage__(pMes, &MesTxt) != EDI_MES_OK)
    {
        free_edi_buff(&MesTxt);
        return -1;
    }

#ifdef strndup
    *buff = strndup(MesTxt.mes, MesTxt.len);
#else
    *buff = strdup(MesTxt.mes);
#endif

    if(*buff == NULL)
    {
        EdiError(EDILOG,"strdup return NULL");
        Ret = -1;
    }
    else
    {
        Ret = MesTxt.len;
    }

    free_edi_buff(&MesTxt);
    return Ret;
}

int WriteEdiMessage__(EDI_REAL_MES_STRUCT *pMes, MesEdiStruct *MesTxt)
{
    int Ret;

    if (pMes  == NULL || pMes->Tag == NULL)
    {
        EdiError(EDILOG,"Pointer is NULL");
        return EDI_MES_ERR;
    }
    tst();

    if((Ret = MakeUntUnzInEdiMes_(pMes))!=EDI_MES_OK)
    {
        tst();
        return Ret;
    }

    if((Ret = SetSpacesToEdiMessage_(pMes))!=EDI_MES_OK)
    {
        tst();
        return Ret;
    }

    return WriteEdiTags(pMes, pMes->Tag, MesTxt);
}

int WriteEdiTags(EDI_REAL_MES_STRUCT *pCurMes, TAG_REAL_STRUCT *pTags, MesEdiStruct *MesTxt)
{
    while (pTags)
    {
        tst();
        if(pTags->GrStr)
        {
            if(WriteEdiTags(pCurMes, pTags->GrStr, MesTxt)!=EDI_MES_OK)
                return EDI_MES_ERR;
        }
        else
        {
            if(realloc_edi_buff(MesTxt, TAG_LEN)){
                return EDI_MES_ERR;
            }
            MesTxt->len += sprintf(MesTxt->mes+MesTxt->len, "%.*s", TAG_LEN, pTags->TemplateTag->TagName);

            if(WriteEdiComp(pCurMes, pTags->Comp, MesTxt)!=EDI_MES_OK){
                return EDI_MES_ERR;
            }

            if(realloc_edi_buff(MesTxt, pCurMes->pCharSet->EndSegLen)){
                return EDI_MES_ERR;
            }
            MesTxt->len += sprintf(MesTxt->mes+MesTxt->len, "%.*s",
                                   pCurMes->pCharSet->EndSegLen,
                                   pCurMes->pCharSet->EndSegStr);
        }

        if(WriteEdiTags(pCurMes, pTags->NextArr, MesTxt)!=EDI_MES_OK){
            return EDI_MES_ERR;
        }

        pTags = pTags->NextTag;
    }
    return EDI_MES_OK;
}

int WriteEdiComp(EDI_REAL_MES_STRUCT *pCurMes, COMP_REAL_STRUCT *pComp, MesEdiStruct *MesTxt)
{
    while(pComp){
        if(realloc_edi_buff(MesTxt, sizeof(char))){
            return EDI_MES_ERR;
        }
        MesTxt->len+=sprintf(MesTxt->mes+MesTxt->len, "%c", pCurMes->pCharSet->EndComp);

        if(WriteEdiData(pCurMes, pComp->DataElem, MesTxt, 1, pComp->SingleData)!=EDI_MES_OK){
            return EDI_MES_ERR;
        }

        if(WriteEdiComp(pCurMes, pComp->NextArr, MesTxt)!=EDI_MES_OK){
            return EDI_MES_ERR;
        }
        pComp = pComp->Next;
    }
    return EDI_MES_OK;
}

int WriteEdiData(EDI_REAL_MES_STRUCT *pCurMes, DATA_REAL_STRUCT *pData,
                 MesEdiStruct *MesTxt, int first, int Single)
{
    while(pData)
    {
        char Separat = Single ? pCurMes->pCharSet->EndComp : pCurMes->pCharSet->EndData;

        tst();

        if(pData->TemplateData->Format!=DATA_FORMAT2/*numeric*/
           && InsertEdiMaskChars2(pCurMes, pData))
        {
            EdiError(EDILOG,"Error in InsertEdiMaskChars2");
            return EDI_MES_ERR;
        }

        if(realloc_edi_buff(MesTxt, (first?0:(sizeof(char))) + pData->len))
        {
            return EDI_MES_ERR;
        }

        if(!first)
        {
            MesTxt->len+=sprintf(MesTxt->mes+MesTxt->len, "%c", Separat);
        }

        MesTxt->len += sprintf(MesTxt->mes+MesTxt->len, "%.*s", pData->len, pData->Data);

        if(WriteEdiData(pCurMes, pData->NextArr, MesTxt, 0, Single)!=EDI_MES_OK){
            return EDI_MES_ERR;
        }

        pData = pData->Next;
        first=0;
    }

    return EDI_MES_OK;
}

void PrintRealEdiMesW(FILE *fl)
{
    return PrintRealEdiMes_(GetEdiMesStructW(), fl);
}

int realloc_edi_buff(MesEdiStruct *MesTxt, int need_len)
{
    if((MesTxt->len == 0 && need_len) ||
        ((MesTxt->size-MesTxt->len)<=need_len))
    {

        int n_ch = (need_len == EDI_CHNG_SIZE)?1:(need_len/EDI_CHNG_SIZE + 1);
        char *new_ptr = (char *)edi_realloc(MesTxt->mes, MesTxt->size+n_ch*EDI_CHNG_SIZE);

        if(!new_ptr){
            EdiError(EDILOG,"Realloc error");
            edi_free(MesTxt->mes);
            MesTxt->mes = NULL;
            return -1;
        }else {
            MesTxt->mes = new_ptr;
            MesTxt->size += n_ch*EDI_CHNG_SIZE;
        }
    }
    return 0;
}

void free_edi_buff(MesEdiStruct *MesTxt)
{
    edi_free(MesTxt->mes);
    memset(MesTxt, 0, sizeof(MesEdiStruct));
}
