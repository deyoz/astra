#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "edi_tables.h"
#include "edi_func.h"
#include "edi_user_func.h"
#include "edi_logger.h"
#include "edi_sess.h"

#include "edi_err.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include "edi_test.h"

/*
 Читает заголовок EDIFACT сообщения
 UNB/UNH
 return EDI_MES_OK  - Ok
 return EDI_MES_ERR - Error
 */
int ReadEdiContext(const char *Mes, edi_mes_head *pHead)
{
    EDI_REAL_MES_STRUCT *pCurMes=NULL;
    EDI_MESSAGES_STRUCT *pTempMes=NULL;

    if((pTempMes=GetEdiTemplateMessages())==NULL){
        EdiError(EDILOG,"GetTemplateMessages()==NULL");
        return EDI_MES_ERR;
    }

    if(CreateEdiLocalMesStruct() || (pCurMes=GetEdiMesStruct())==NULL){
        EdiError(EDILOG,"Error in CreateEdiLocalMesStruct()");
        return EDI_MES_ERR;
    }

    return ReadEdiContext_(Mes,pHead, pTempMes, pCurMes, 1);
}

/*
 Читает заголовок EDIFACT сообщения
 UNB/UNH
 return EDI_MES_OK  - Ok
 return EDI_MES_ERR - Error
 */
int ReadEdiContextNoTypes(const char *Mes, edi_mes_head *pHead)
{
    EDI_REAL_MES_STRUCT *pCurMes=NULL;
    EDI_MESSAGES_STRUCT *pTempMes=NULL;

    if((pTempMes=GetEdiTemplateMessages())==NULL){
        EdiError(EDILOG,"GetTemplateMessages()==NULL");
        return EDI_MES_ERR;
    }

    if(CreateEdiLocalMesStruct() || (pCurMes=GetEdiMesStruct())==NULL){
        EdiError(EDILOG,"Error in CreateEdiLocalMesStruct()");
        return EDI_MES_ERR;
    }

    return ReadEdiContext_(Mes,pHead, pTempMes, pCurMes, 0);
}

int ReadEdiContext_(const char *Mes, edi_mes_head *pHead,
                    EDI_MESSAGES_STRUCT *pTempMes,
                    EDI_REAL_MES_STRUCT *pCurMes,
                    short read_types)
{
    int ret;

    // Удаляем, если там что-то осталось. Глобальная переменная!
    DeleteRealMes(pCurMes);

    /*Считываем строку с сообщением во внутр представление edilib*/
    if((ret=FillEdiHeadStr(pCurMes, pTempMes, Mes))!=EDI_MES_OK){
        WriteEdiLog(EDILOG,"FillEdiHeadStr() returned %d", ret);
        DeleteRealMes(pCurMes);
        return ret;
    }

    /*из edilib достаем в pAttr*/
    if(SetMesHead_(pHead, pCurMes, read_types))
    {
        DeleteRealMes(pCurMes);
        return EDI_MES_ERR;
    }

    // Больше считанное сообщение не нужно
    DeleteRealMes(pCurMes);
    return EDI_MES_OK;
}

/******************************************************************/
/*Заполняет аттрибуты сообщения 				  */
/*	return  0 - Ok 						  */
/*	return -1 - программная ошибка				  */
/*      return  1 - нельзя читать                                 */
/******************************************************************/
int SetMesHead(edi_mes_head *pHead, short read_types)
{
    return SetMesHead_(pHead, GetEdiMesStruct(), read_types);
}

int SetMesHead_(edi_mes_head *pHead, EDI_REAL_MES_STRUCT *pMes, short read_types)
{
    if(isEdiError_(pMes)) {
        const char * segErr = EdiErrGetSegName(pMes);
        if(segErr && *segErr && (!strcmp("UNB", segErr) || !strcmp("UNH", segErr))){
            EdiTrace(TRACE1, "Syntax error in %s segment. No attributes", segErr);
            return 1;
        }
    }

    PushEdiPoint_(pMes);
    ResetEdiPoint_(pMes);
    if(SetEdiPointToSegment_(pMes, "UNB", 0)!=1){
        PopEdiPoint_(pMes);
        return EDI_MES_ERR;
    }

    pHead->syntax_ver = (int)strtol(GetDBFNSegZ_(pMes, "S001",0, 2,0), NULL, 10);
    strcpy(pHead->chset,     GetDBFNSegZ_(pMes, "S001",0, 1,0));
    strcpy(pHead->from,      GetDBFNSegZ_(pMes, "S002",0, 4,0));
    strcpy(pHead->fromAddVer,GetDBFNSegZ_(pMes, "S002",0, 7,0));
    strcpy(pHead->fromAddExt,GetDBFNSegZ_(pMes, "S002",0, 8,0));
    strcpy(pHead->to,        GetDBFNSegZ_(pMes, "S003",0, 10,0));
    strcpy(pHead->toAddVer,  GetDBFNSegZ_(pMes, "S003",0, 7,0));
    strcpy(pHead->toAddExt,  GetDBFNSegZ_(pMes, "S003",0, 14,0));
    strcpy(pHead->date,      GetDBFNSegZ_(pMes, "S004",0, 17,0));
    strcpy(pHead->time,      GetDBFNSegZ_(pMes, "S004",0, 19,0));
    strcpy(pHead->other_ref, GetDBNz_(pMes, 20));
    strcpy(pHead->our_ref,   GetDBFNSegZ_(pMes, "S005",0, 22,0));
    strcpy(pHead->FseId,     GetDBNz_(pMes, 26));
    strcpy(pHead->assoc_code,GetDBNz_(pMes, 29));

    PopEdiPoint_wd_(pMes);
    if(SetEdiPointToSegment_(pMes, "UNH", 0)!=1){
        PopEdiPoint_(pMes);
        return EDI_MES_ERR;
    }

    strcpy(pHead->unh_number,GetDBNz_(pMes, 62));

    if(PushEdiPoint_(pMes) || SetEdiPointToComposite_(pMes, "S009", 0)!=1){
        PopEdiPoint_(pMes);
        return EDI_MES_ERR;
    }
    strcpy(pHead->code      ,GetDBFNCompZ_(pMes, 65,0));
    strcpy(pHead->ver_num   ,GetDBFNCompZ_(pMes, 52,0));
    strcpy(pHead->rel_num   ,GetDBFNCompZ_(pMes, 54,0));
    strcpy(pHead->cntrl_agn ,GetDBFNCompZ_(pMes, 51,0));
    PopEdiPoint_(pMes);
    strcpy(pHead->acc_ref   ,GetDBNz_(pMes, 68));

    {
        char *p=NULL;
        pHead->mes_num = (unsigned)strtol(pHead->unh_number, &p, 10);
        if(*p != '\0') {
            EdiTrace(TRACE0,"UNH message number usually is digit in our interchange (%s)",
                     pHead->unh_number);
            //PopEdiPoint_(pMes);
            //return EDI_MES_ERR;
        } else {
            strcpy(pHead->unh_number, "\0");
        }
    }

    {
        Edi_CharSet *pChSet = GetEdiCurCharSet_(pMes);
        if(!pChSet){
            EdiError(EDILOG,"Char set is not found");
            return EDI_MES_ERR;
        }
        memcpy(&pHead->CharSet, pChSet, sizeof(pHead->CharSet));
    }

    if(read_types && pMes->pTempMes) {
        pHead->msg_type  	= GetEdiMesType_(pMes);
        if(GetEdiMsgTypeByType(pHead->msg_type, pHead)){
            EdiError(EDILOG,"No types defined for message %d", pHead->msg_type);
            return EDI_MES_ERR;
        }
    } else if(read_types){
        if(GetEdiMsgTypeByCode(pHead->code, pHead)){
            EdiError(EDILOG,"No types defined for message %s", pHead->code);
            return EDI_MES_ERR;
        }
    }

    EdiTrace(TRACE3,
             "UNB :\n"
             "\tfrom=<%s::%s>, to=<%s::%s>, date=<%s>, time=<%s>\n"
             "\tother_ref=<%s>, our_ref=<%s>, assoc_code=<%s>\n"
             "UNH :\n"
             "\tcode=<%s>, acc_ref=<%s>",
             pHead->from,pHead->fromAddExt,
             pHead->to,  pHead->toAddExt,
             pHead->date,pHead->time,
             pHead->other_ref,pHead->our_ref,pHead->assoc_code,
             pHead->code,pHead->acc_ref);
    PopEdiPoint_(pMes);
    return EDI_MES_OK;
}

/*
 Создает структуру нового сообщения mes_type
 Заполняет первонач. данными из pHead(тэги UNH/UNB)
 return  0 - Ok
 return  <0 - error
 */
int CreateMesByHead(edi_mes_head *pHead)
{
    int err = 0;

    if(pHead == NULL){
        EdiError(EDILOG,"pHead pointer is NULL");
        return -1;
    }

    if(initEdiTime(pHead)){
        return -2;
    }

    if((err = CreateNewEdiMes(pHead->msg_type, pHead->chset))){
        EdiError(EDILOG,"Create new edifact message %d: err = %d",
                 pHead->msg_type, err);
        return -3;
    }

    EdiTrace(TRACE3,"UNB :\n"
             "\tfrom=<%s::%s>, to=<%s::%s>, date=<%s>, time=<%s>\n"
             "\tother_ref=<%s>, our_ref=<%s>, assoc_code=<%s>\n"
             "UNH :\n"
             "\tcode=<%s>, acc_ref=<%s>",
             pHead->from,pHead->fromAddExt,
             pHead->to,pHead->toAddExt,
             pHead->date,pHead->time,
             pHead->other_ref,pHead->our_ref,pHead->assoc_code,
             pHead->code,pHead->acc_ref);

    /*Собственно внутренности*/
    if(SetEdiSegment("UNB", 0) ||
            SetEdiPointToSegmentW("UNB", 0) !=1 ||
            SetDBFNSegs("S001", 0, 1,  0, pHead->chset) ||
            SetDBFNF (0,0,NULL,0,"S001", 0, 0002, 0, 0, "%d", pHead->syntax_ver) ||
            SetDBFNSegs("S002", 0, 4,  0, pHead->from) ||
            SetDBFNSegs("S002", 0, 7,  0, pHead->fromAddVer) ||
            SetDBFNSegs("S002", 0, 8,  0, pHead->fromAddExt) ||
            SetDBFNSegs("S003", 0, 10, 0, pHead->to) ||
            SetDBFNSegs("S003", 0, 7, 0,  pHead->toAddVer) ||
            SetDBFNSegs("S003", 0, 14, 0,  pHead->toAddExt) ||
            SetDBFNSegs("S004", 0, 17, 0, pHead->date) ||
            SetDBFNSeg ("S004", 0, 19, 0, pHead->time) ||
            SetEdiDataElem(20, pHead->our_ref) ||
            SetDBFNSegs("S005", 0, 22, 0, pHead->other_ref) ||
            SetEdiDataElem(26, pHead->FseId) ||
            SetEdiDataElem(29, pHead->assoc_code) ||

            ResetEdiPointW() ||

            SetEdiSegment("UNH", 0) ||
            SetEdiPointToSegmentW("UNH", 0) !=1 ||
            (strlen(pHead->unh_number) ? SetEdiDataElem(62, pHead->unh_number)
                                       : SetEdiDataElemF(62, "%d", pHead->mes_num)) ||
            SetDBFNSegs("S009", 0, 65, 0, GetEdiMesW()) ||
            PushEdiPointW() ||
            SetEdiPointToCompositeW("S009", 0) !=1 ||
            SetEdiDataElem(52, pHead->ver_num) ||
            SetEdiDataElem(54, pHead->rel_num) ||
            SetEdiDataElem(51, pHead->cntrl_agn) ||
            PopEdiPointW() ||
            SetEdiDataElem(68, pHead->acc_ref) ||
            ResetEdiPointW()   ){
        EdiError(EDILOG,"Collect edifact message struct: error");
        return -4;
    }

    EdiTrace(TRACE3,"Collect edifact message struct: ok");

    return 0;
}

int CreateAnswerByHead(edi_mes_head *pHead)
{
    if(pHead == NULL){
        EdiError(EDILOG,"pHead pointer is NULL");
        return -1;
    }

    if(pHead->msg_type_req!=QUERY){
        EdiError(EDILOG, "Type of the current message is not query");
        return -1;
    }

    if(pHead->answer_type == EDINOMES){
        EdiError(EDILOG, "No answer type specify");
        return -1;
    }

    pHead->msg_type = pHead->answer_type;

    return 0;
}

int initEdiTime(edi_mes_head *pHead)
{
    int ret;
    struct tm *ptm;
    time_t now = time(NULL);
    if(now==((time_t) -1)){
        EdiError(EDILOG, "time: %s", strerror(errno));
        return -1;
    }
    ptm=gmtime(&now);
    ret = snprintf(pHead->date, sizeof(pHead->date), "%02d%02d%02d",
                   ptm->tm_year%100, ptm->tm_mon+1,ptm->tm_mday);

    if(ret < 0 || size_t(ret) > sizeof(pHead->date)) {
        EdiError(EDILOG,"Too small buffer for date");
        return -2;
    }

    ret = snprintf(pHead->time, sizeof(pHead->time), "%02d%02d",
                   ptm->tm_hour,ptm->tm_min);
    if(ret<0 || size_t(ret) > sizeof(pHead->time)) {
        EdiError(EDILOG,"Too small buffer for time");
        return -3;
    }

    EdiTrace(TRACE4,"date:%s, time:%s", pHead->date, pHead->time);
    return 0;
}

int InitEdiAfrerErrProcFunc()
{
    return 0;
}
