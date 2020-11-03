#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <ctype.h> /*for ISDIGIT ...*/

#include "edi_err.h"
#include "edi_tables.h"
#include "edi_all.h"
#include "edi_func.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include "edi_test.h"
#include "edi_user_func.h"



static Edi_CharSet CharSet[]=
{
    {END_SEG,  sizeof(END_SEG)-1 , END_COMP, END_EL_D, EDI_RELEASE},
    /*{END_SEG1, sizeof(END_SEG1)-1, END_COMP, END_EL_D, EDI_RELEASE},*/
    {"", 0, '\0','\0','\0'}, /*One of the preset charsets*/
    {"", 0, '\0','\0','\0'}, /*Loaded from message*/
};

Edi_CharSet *GetCharSetByType(CharSetType type)
{
    return &CharSet[type];
}

/*******************************************************************/
/* Возвращает структуру с набором управл. символов                 */
/* return NULL - ничего нет					   */
/*******************************************************************/
Edi_CharSet *GetEdiCurCharSet(void)
{
    return GetEdiCurCharSet_(GetEdiMesStruct());
}
Edi_CharSet *GetEdiCurCharSet_(EDI_REAL_MES_STRUCT *pMes)
{
    return pMes->pCharSet;
}

/********************************************************************/
/* Возвращает структуру с набором управл. символов из текста EDIFACT*/
/* return NULL - ничего нет					    */
/********************************************************************/
Edi_CharSet *GetEdiCharSetFromText(const char *text, int len)
{
    return GetEdiCharSetFromText_(GetEdiMesStruct(), GetEdiTemplateMessages(),
                                  text, len);
}

Edi_CharSet *GetEdiCharSetFromText_(EDI_REAL_MES_STRUCT *pMes,
									EDI_MESSAGES_STRUCT *pMesTemp,
									const char *text, int len)
{
    char mes_name[MESSAGE_LEN+1];
    if(GetMesNameCharSet(pMes, pMesTemp, text, len, mes_name, 0)==EDI_MES_OK)
        return pMes->pCharSet;
    else
        return NULL;
}

const char *GetEdiCharSetByName(EDI_MESSAGES_STRUCT *pMesTemp,
                                const char *chname, short check)
{
    int i;
    for(i=0;i<pMesTemp->chsnum;i++)
    {
        if(!strncmp(pMesTemp->char_set_arr[i].name, chname, sizeof(pMesTemp->char_set_arr[i].name)-1))
        {
            EdiTrace(TRACE2,"Found character set by %s",
                     pMesTemp->char_set_arr[i].name);
            return pMesTemp->char_set_arr[i].chset_str;
        }
    }

    EdiTrace(check,EDILOG,"Preset is not found for CharSet=%.*s",
             (int)(sizeof(pMesTemp->char_set_arr[i].name)-1), chname);

    return NULL;
}

/*
 return 1 = EDIFACT
 return 0 = скорее всего нет
 */
int IsEdifactText(const char *text, int len)
{
    return IsEdifactText_(GetEdiTemplateMessages(), text, len);
}

int IsEdifactText_(EDI_MESSAGES_STRUCT *pMes, const char *text, int len)
{
    char mes_name[MESSAGE_LEN + 1];
    int ret = EDI_MES_OK;
    EDI_REAL_MES_STRUCT *pCurMes=NULL;

    if(CreateEdiMesStruct(&pCurMes)) {
        ret = EDI_MES_ERR;
    }

    if (ret == EDI_MES_OK){
        ret = GetMesNameCharSet(pCurMes, pMes, text, len, mes_name,CHECK_LOG_LEVEL);
    }

    DeleteRealMesStruct(pCurMes);

    if(ret==EDI_MES_OK) {
        EdiTrace(TRACE2,"It is EDIFACT! (%s)", mes_name);
        return 1;
    } else {
        EdiTrace(TRACE2,"It is not EDIFACT!");
        return 0;
    }
}

/****************************************************************************/
/*Считывает сообщение в ОП                                                  */
/*Mes -указатель на строку, содержащую данное сообщение                     */
/*return EDI_MES_NOT_FND    -неизвестное сообщение                          */
/*return EDI_MES_ERR        -ошибка                                         */
/*return EDI_MES_OK         -считывание прошло успешно                      */
/*return EDI_MES_STRUCT_ERR -ошибка в структуре сообщения                   */
/****************************************************************************/
int ReadEdiMes(const char *Mes, EDI_MESSAGES_STRUCT *pMes,
               EDI_REAL_MES_STRUCT *pCurMes)
{
    int Ret;
    char MesName[MESSAGE_LEN+1];
    int len = strlen(Mes);

    Ret = GetMesNameCharSet(pCurMes, pMes, Mes, len, MesName,0);
    if(Ret != EDI_MES_OK) {
        return Ret;
    }
    Ret = InsertCurMes(MesName, pMes, Mes, pCurMes);
    if(Ret != EDI_MES_OK){
        if(Ret == EDI_MES_NOT_FND){
            WriteEdiLog(EDILOG,"The template for the message is not found ! Mes: %s\n",MesName);
            SetEdiErrNum_(pCurMes, EDI_INCORRECT);
        }
        return Ret;
    }
    return EDI_MES_OK;
}

/*
 Не считывая сообщения целиком, берем только UNB,UNH
 return EDI_MES_OK
 return EDI_MES_STRUCT_ERR
 */
int FillEdiHeadStr(EDI_REAL_MES_STRUCT *pCurMes, EDI_MESSAGES_STRUCT *pMes,
                   const char *Mes)
{
    char mes_name[MESSAGE_LEN+1];
    char *Seg,*MesStr;
    int Ret, i;
    TAG_REAL_STRUCT *pCurTag;
    TAG_STRUCT *pTag=pMes->pTemps->Tag;
    int len = strlen(Mes);

    EdiTrace(TRACE5, "%s : Message is '%s'", __FUNCTION__, Mes);

    if((Ret=GetMesNameCharSet(pCurMes, pMes, Mes, len, mes_name,0))!=EDI_MES_OK){
        return Ret;
    }

    /*
    pCurMes->pTempMes=pMes
    */

    if((Ret = FillEdiMesStruct(mes_name, pMes, Mes, len, pCurMes))!=EDI_MES_OK){
        return Ret;
    }

    MesStr=pCurMes->MesTxt.mes;

    for(i=0;i<2;i++){
        Ret = GetNextSeg(pCurMes, &MesStr, &Seg);
        if (Ret != EDI_MES_OK)
            return Ret;
        // 	EdiTrace(TRACE3,"Segment <%s>",Seg);

        if(i){
            if(!CreateNewTagReal(&(pCurTag->NextTag)))
                return EDI_MES_ERR;
            pCurTag = pCurTag->NextTag;
            pTag = pTag->NextTag;
        }else {
            if(!CreateNewTagReal(&pCurTag))
                return EDI_MES_ERR;
            pCurMes->Tag = pCurTag;/*Первый сегмент реального сообщения*/
            pTag = pMes->pTemps->Tag;      /*Первый сегмент шаблона сообщений*/
        }

        if(pTag->GrStr != NULL  || pTag->MaxR > 1 ||
                pTag->S != MANDATORY || memcmp(Seg, pTag->TagName, TAG_LEN)){
            WriteEdiLog(EDILOG,"Bad parameters in template");
            return EDI_MES_STRUCT_ERR;
        }

        pCurTag->R = 1;
        Ret = InsertSeg(pCurMes, Seg, pTag, pCurTag);
        if(Ret != EDI_MES_OK){
            SetEdiErrSegm_(pCurMes, pTag->TagName);
            return Ret;
        }
    }

    tst();
    return EDI_MES_OK;
}

const char *GetCharSetNameFromText(const char *text, int len, short check)
{
    /*Считается что UNA тут нет*/
    if(TAG_LEN+1+EDI_CHSET_LEN<len){
        return text+TAG_LEN+1;
    }

    EdiTrace(check,EDILOG,"Message is too short (%d)", len);
    return NULL;
}


int GetMesNameCharSet(EDI_REAL_MES_STRUCT *pCurMes, EDI_MESSAGES_STRUCT *pMes,
                      const char *Mes, int len, char *MesName, short check)
{
    const char *MesTmp = Mes;
    const char *part;
    int pos=0;
#if _DUAL_CHAR_SET_SUPPORT_
    int save_pos, save_len;
    const char *SaveMesTmp;
    int num_try=0;
#endif

    if(len>TAG_LEN && !memcmp(MesTmp, CUSTOM_CHAR_SET_TAG, TAG_LEN))
    {
        char CharSet[CUSTOM_CHAR_SET_LEN+1];

        EdiTrace(TRACE2,"Found custom character set in UNA segment");

        memcpy(CharSet, MesTmp+TAG_LEN, CUSTOM_CHAR_SET_LEN);
        CharSet[CUSTOM_CHAR_SET_LEN]='\0';
        if(SetCustomCharSet_(pCurMes, CharSet))
        {
            return EDI_MES_ERR;
        }
        pos=TAG_LEN+CUSTOM_CHAR_SET_LEN;
        MesTmp+=pos;
        len-=pos;
    }

    if(pCurMes->pCharSet == NULL)
    {
        /*Берем из предустановок*/
        const char *chsname=GetCharSetNameFromText(MesTmp,len,check);
        const char *chs_str;
        if(chsname && (chs_str=GetEdiCharSetByName(pMes,chsname,check)) &&
                !SetCharSetOnType(pCurMes, chs_str, PresetEdiCharSet))
        {
            /*pCurMes->pCharSet=GetCharSetByType(PresetEdiCharSet);*/
            tst();
        }
        else
        {
            EdiTrace(check,EDILOG,
                     "Unknown character set in the message");
            SetEdiErrNum_(pCurMes, EDI_UNKNOWN_SEPARAT);
            return EDI_MES_STRUCT_ERR;
        }
    }


#if _DUAL_CHAR_SET_SUPPORT_
    save_pos=pos;
    save_len=len;
    SaveMesTmp=MesTmp;
    *MesName='\0';

    while(!*MesName)
    {
        /* Временное явление!
        Пока есть необходимость в совместимости с нашим charset'ом*/
        if(num_try)
        {
            pos=save_pos;
            len=save_len;
            MesTmp=SaveMesTmp;
            ResetEdiErr_(pCurMes);
            if(num_try==1){
                SetCharSetOnType(pCurMes, "\x3A\x2B,\x3F \"\n", PresetEdiCharSet);
            }else {
                SetCharSetOnType(pCurMes, "\x3A\x2B,\x3F \x27", PresetEdiCharSet);
            }
        }
//         EdiTrace(TRACE4,"Num_try=%d",num_try);
#endif /* _DUAL_CHAR_SET_SUPPORT_ */

    EdiTrace(TRACE5,"Pos is %.10s, len=%d",MesTmp+TAG_LEN+1,len);
    if(part=GetNextSegRO(pCurMes, MesTmp+TAG_LEN+1, check)) {
        if(!memcmp(part, "UNG", TAG_LEN)) {
            MesTmp = part;
        }
    }

    if((part=GetNextSegRO(pCurMes, MesTmp+TAG_LEN+1, check)) &&
            ((len-=(part-MesTmp), pos+=(part-MesTmp),
              EdiTrace(TRACE5,"Pos is %.10s,len=%d",Mes+pos,len),len>TAG_LEN+1)) &&
            (MesTmp=GetNextCompRO(pCurMes, part+TAG_LEN+1)) &&
            ((len-=(MesTmp-part), pos+=(MesTmp-part),
              EdiTrace(TRACE5,"Pos is %.10s,len=%d",Mes+pos,len),len>0)) &&
            (part=GetNextDataRO(pCurMes, MesTmp)))
    {
        int mes_len=part-MesTmp-1;
        EdiTrace(TRACE2,"Message name is %.*s", mes_len, MesTmp);
        if(mes_len!=MESSAGE_LEN)
        {
            EdiTrace(check,EDILOG,"Bad message length %d", mes_len);
#if _DUAL_CHAR_SET_SUPPORT_
                if(num_try>=2)
                {
#endif
            SetEdiErrNum_(pCurMes, EDI_MES_NAME_ERR);
            return EDI_MES_STRUCT_ERR;
#if _DUAL_CHAR_SET_SUPPORT_
                } else {
                    num_try++;
                    continue;
        }
#endif
            }
        else
        {
            memcpy(MesName, MesTmp, MESSAGE_LEN);
            MesName[MESSAGE_LEN] = '\0';
        }
    }
    else
    {
#if _DUAL_CHAR_SET_SUPPORT_
            if(num_try>=2)
            {
#endif
        EdiTrace(check,EDILOG,"Cannot get message name");
        SetEdiErrNum_(pCurMes, EDI_MES_NAME_ERR);
        return EDI_MES_STRUCT_ERR;
#if _DUAL_CHAR_SET_SUPPORT_
            } else {
                num_try++;
                continue;
    }
#endif
        }
#if _DUAL_CHAR_SET_SUPPORT_
    }
#endif /* _DUAL_CHAR_SET_SUPPORT_ */

    ResetEdiErr_(pCurMes);

    return EDI_MES_OK;
}

int FillEdiMesStruct(const char *MesName, EDI_MESSAGES_STRUCT *pTMes,
                     const char *Mes, int len, EDI_REAL_MES_STRUCT *pCurMes)
{
    int offset=0;

    if(!memcmp(Mes, CUSTOM_CHAR_SET_TAG, TAG_LEN)){
        offset=TAG_LEN+CUSTOM_CHAR_SET_LEN;
        len-=offset;
    }

    if(realloc_edi_buff(&pCurMes->MesTxt, len+1)){
        EdiError(EDILOG,"Can't allocate memory\n");
        return EDI_MES_ERR;
    }
    memcpy(pCurMes->MesTxt.mes, Mes+offset, len+1); /*Запоминаем выделенный выше участок памяти, чтобы затем его м.б. удалить*/
    memcpy(pCurMes->Message,MesName,MESSAGE_LEN+1);
    pCurMes->MesTxt.len = len;
    pCurMes->MesTxt.size= len+1;
    /* pCurMes->pTempMes = pMes; */

    return EDI_MES_OK;
}

int InsertCurMes(const char *MesName, EDI_MESSAGES_STRUCT *pMes,
                 const char *Mes    , EDI_REAL_MES_STRUCT *pCurMes)
{
    int len=strlen(Mes);
    int Ret;

    if((pCurMes->pTempMes = GetEdiTemplateByName(pMes, MesName))==NULL){
        return EDI_MES_NOT_FND;
    }

    Ret=FillEdiMesStruct(MesName, pMes, Mes, len, pCurMes);
    if (Ret!=EDI_MES_OK){
        return Ret;
    }
    strcpy(pCurMes->Text, pCurMes->pTempMes->Text);

    Ret = GetCurMes(pCurMes->MesTxt.mes,pCurMes->pTempMes,pCurMes);
    if (Ret == EDI_MES_NOT_FND){
        Ret = EDI_MES_STRUCT_ERR;
    }

    return Ret;
}

int GetCurMes(char *Mes,                /* Строка с сообщением */
			  EDI_TEMP_MESSAGE *pMes,/* pMes-шаблон для данного сообщения */
			  EDI_REAL_MES_STRUCT *pCurMes)/* ppCurMes-куда считывать */
{
    int Ret,flag=0;
    TAG_REAL_STRUCT *pCurTag=NULL;
    TAG_REAL_STRUCT *pPrevTag=NULL;
    TAG_STRUCT *pTag = pMes->Tag;
    char *MesTmp = NULL;

    while( 1 ) {
        tst();
        Ret = GetNextSeg(pCurMes, &Mes, &MesTmp);
        if (Ret == EDI_MES_STRUCT_ERR)
            return Ret;
        //   if (Ret == EDI_MES_OK)
        //       EdiTrace(TRACE3,"Testing segment <%s>",MesTmp);
        if (Ret != EDI_MES_OK){ /*EDI_MES_NOT_FND*/
            Ret = CheckTagsGet(pCurMes, pTag);
            return Ret;
        } else  if(pTag == NULL) {
            WriteEdiLog(EDILOG,"Tag %.*s is not found in the template",TAG_LEN, MesTmp);
#ifdef EDI_DEBUG
            return EDI_MES_OK;
#endif
            SetEdiErrNum_(pCurMes, EDI_SEGM_UNKNOWN);
            SetEdiErrSegm_(pCurMes, Mes);
            return EDI_MES_STRUCT_ERR;
        }

        if(flag){
            if(!CreateNewTagReal(&(pCurTag->NextTag)))
                return EDI_MES_ERR;
            pPrevTag = pCurTag;
            pCurTag = pCurTag->NextTag;
        }else {
            if(!CreateNewTagReal(&pCurTag))
                return EDI_MES_ERR;
            pCurMes->Tag = pCurTag;/*Первый сегмент реального сообщения*/
            pTag = pMes->Tag;      /*Первый сегмент шаблона сообщений*/
            flag=1;
        }
        pCurTag->ParTag = NULL;
        while( 1 ){
            if(pTag->GrStr == NULL){
                if(!memcmp(MesTmp,pTag->TagName,TAG_LEN)){
                    if(pTag->MaxR > 1){
                        Ret = InsertSegArr(pCurMes, &Mes, &MesTmp, pTag, pCurTag);
                        if(Ret != EDI_MES_OK){
                            if(Ret == EDI_MES_STRUCT_ERR){
                                WriteEdiLog(EDILOG,"Error of the message structure in InsertSegArr()");
                            }
                            return Ret;
                        }
                    }
                    else {
                        pCurTag->R = 1;
                        Ret = InsertSeg(pCurMes, MesTmp, pTag, pCurTag);
                        if(Ret != EDI_MES_OK){
                            SetEdiErrSegm_(pCurMes, pTag->TagName);
                            return Ret;
                        }
                    }
                    pTag = pTag->NextTag;
                    break;
                }
            }
            else  /*pTag->GrStr != NULL*/
                if(!memcmp(MesTmp,(pTag->GrStr)->TagName,TAG_LEN)){
                    EdiTrace(TRACE3,"Testing segment group %d <%s>",pTag->Num,(pTag->GrStr)->TagName);
                    Ret = InsertSegGr(pCurMes, &Mes, &MesTmp, pTag, pCurTag, 0);
                    pTag = pTag->NextTag;
                    if((Ret == EDI_MES_NOT_FND) && (pTag == NULL))
                        return EDI_MES_OK;
                    if(Ret == EDI_MES_NOT_FND)
                        break;
                    if(Ret != EDI_MES_OK)
                        return Ret;
                    /*************************/
                    if(!CreateNewTagReal(&(pCurTag->NextTag)))
                        return EDI_MES_ERR;
                    pCurTag = pCurTag->NextTag;
                    /*************************/
                    continue;
                }

            if ( (pTag->NextTag == NULL      ) ||
                 (pTag->S       == MANDATORY )  ){
                if(pTag->S == MANDATORY && TemplateTagByName(MesTmp, pTag)){
                    if(pTag->GrStr==NULL){
                        WriteEdiLog(EDILOG, "Tag %s mandatory but not found in the message",
                                    pTag->TagName);
#ifndef EDI_DEBUG
						SetEdiErrNum_(pCurMes, EDI_SEGM_MANDATORY);
						SetEdiErrSegm_(pCurMes, pTag->TagName);
#endif
					}else {
						WriteEdiLog(EDILOG, "Segment group %d mandatory but not found in the message",
									pTag->Num);
#ifndef EDI_DEBUG
						SetEdiErrNum_(pCurMes, EDI_SEGGR_MANDATORY);
						SetEdiErrSegGr_(pCurMes, pTag->Num, 1);
#endif
					}
				}else {
					WriteEdiLog(EDILOG,
								"Tag %.*s is not found in the template",TAG_LEN, MesTmp);
#ifndef EDI_DEBUG
					SetEdiErrNum_(pCurMes, EDI_SEGM_UNKNOWN);
					SetEdiErrSegm_(pCurMes, MesTmp);
#endif
				}
#ifdef EDI_DEBUG
				if(pTag->NextTag == NULL)
					return EDI_MES_OK;
				else {
					pTag = pTag->NextTag;
					break;
				}
#else
				if(pPrevTag){
					pPrevTag->NextTag = NULL;
				}else {
					pCurMes->Tag = NULL;
				}
				free(pCurTag);
				return EDI_MES_STRUCT_ERR;
#endif
            }
            pTag = pTag->NextTag;
        }/*end while*/
    }/*end while*/
    return EDI_MES_OK;
}

int InsertSegGr (EDI_REAL_MES_STRUCT *pCurMes, char **Mes, char **MesTmp,
                 TAG_STRUCT *pTag, TAG_REAL_STRUCT *pCurTag, int array)
{
    int Ret;
    TAG_REAL_STRUCT     *pCurTagTmp;/*, *pCurTagTmp2;*/
    TAG_STRUCT *pTagTmp;
    char SegGrName[TAG_LEN+1];

    pCurTag->TemplateTag = pTag;

    if (array>=1){ /*След. элемент в массиве сегментных групп*/
        array ++;
        if ( array > pTag->MaxR){
            WriteEdiLog(EDILOG,"Too many segment groups %d (%s)\n",pTag->Num,(pTag->GrStr)->TagName);
#ifdef EDI_DEBUG
            return EDI_MES_OK;
#endif
            SetEdiErrNum_(pCurMes, EDI_SEGGR_MUCH);
            SetEdiErrSegGr_(pCurMes, pTag->Num, array);
            return EDI_MES_STRUCT_ERR;
        }
        pCurTag->R = array;/*Счетчик в массиве сегм групп*/
    }
    else   pCurTag->R = 1;

    if( !CreateNewTagReal(&pCurTagTmp))
        return EDI_MES_ERR;
    pCurTag->GrStr = pCurTagTmp;/*Составляющие группы*/
    pCurTagTmp->ParTag = pCurTag;/*обратная ссылка на родителя*/

    pTagTmp = pTag;
    pTag = pTag->GrStr; /*Составляющие сегм гр. в шаблоне сообщения*/
    memcpy(SegGrName,pTag->TagName,TAG_LEN+1);/*Имя данной сегментной группы*/

    /*******************************************/
    Ret = InsertSeg(pCurMes, *MesTmp, pTag, pCurTagTmp);
    if(Ret != EDI_MES_OK){
        SetEdiErrSegm_(pCurMes, pTag->TagName);
        SetEdiErrSegGr_(pCurMes, pTagTmp->Num, array);
        return Ret;
    }
    pTag = pTag->NextTag;
    /*Подразумевается, что шаблон сообщения    */
    /*выполнен по всем правилам. Первый сегмент*/
    /*сегментной группы имеет параметры M 1    */
    /*Проверка происходит в edi_template.c     */
    /*******************************************/
    while( 1 ) {
        tst();
        Ret = GetNextSeg(pCurMes, Mes,MesTmp);
        if (Ret == EDI_MES_STRUCT_ERR){
            SetEdiErrSegGr_(pCurMes, pTagTmp->Num, array);
            return Ret;
        }
        //     if (Ret == EDI_MES_OK)
        // 	EdiTrace(TRACE3,"Testing segment <%s>",*MesTmp);
        if (Ret != EDI_MES_OK){ /*EDI_MES_NOT_FND*/
            Ret = CheckTagsGet(pCurMes, pTag);
            if (Ret != EDI_MES_OK) {
                SetEdiErrSegGr_(pCurMes, pTagTmp->Num, array);
                return Ret;
            }
            return EDI_MES_NOT_FND;
        }
        if(pTag == NULL) {
            Ret = ProbeCurentSegGr(pCurMes, SegGrName,Mes,MesTmp,pCurTag,pTagTmp);
            return Ret;
        }
        while( 1 ){
            //      if(pTag->GrStr != NULL)
            // 	     EdiTrace(TRACE3,"Current template TAG - GrStr #%d<%s>",pTag->Num,(pTag->GrStr)->TagName);
            //      else
            // 	     EdiTrace(TRACE3,"Current template TAG <%s>",pTag->TagName);

            if(pTag->GrStr == NULL){/*По шаблону на очереди не сегментная группа*/
                if(!memcmp(*MesTmp,pTag->TagName,TAG_LEN)){
                    if( !CreateNewTagReal( &(pCurTagTmp->NextTag) ) )
                        return EDI_MES_ERR;
                    pCurTagTmp = pCurTagTmp->NextTag;
                    pCurTagTmp->ParTag = pCurTag;
                    if(pTag->MaxR > 1){
                        Ret = InsertSegArr(pCurMes, Mes, MesTmp, pTag, pCurTagTmp);
                        if( Ret != EDI_MES_OK ) {
                            if(Ret == EDI_MES_STRUCT_ERR){
                                WriteEdiLog(EDILOG,"Error of the message structure");
                                SetEdiErrSegGr_(pCurMes, pTagTmp->Num, array);
                                return Ret;
                            }
                        }
                    }
                    else {
                        Ret = InsertSeg(pCurMes, *MesTmp, pTag, pCurTagTmp);
                        if(Ret != EDI_MES_OK){
                            SetEdiErrSegm_(pCurMes, pTag->TagName);
                            SetEdiErrSegGr_(pCurMes, pTagTmp->Num, array);
                            return Ret;
                        }
                        pCurTagTmp->R=1;
                    }
                    pTag = pTag->NextTag;
                    break ;
                }
            }
            else  /*pTag->GrStr != NULL*/
                if(!memcmp(*MesTmp,(pTag->GrStr)->TagName,TAG_LEN)){
                    //             EdiTrace(TRACE3,"Testing segment group %d <%s>",pTag->Num,(pTag->GrStr)->TagName);
                    if( !CreateNewTagReal( &(pCurTagTmp->NextTag) ) )
                        return EDI_MES_ERR;
                    pCurTagTmp = pCurTagTmp->NextTag;
                    pCurTagTmp->ParTag = pCurTag;
                    Ret = InsertSegGr(pCurMes, Mes, MesTmp, pTag, pCurTagTmp, 0);
                    if(Ret != EDI_MES_OK){
                        return Ret;
                    }
                    pTag = pTag->NextTag;
                    if( pTag == NULL ) {
                        Ret = ProbeCurentSegGr(pCurMes, SegGrName,Mes,MesTmp,pCurTag,pTagTmp);
                        return Ret;
                    }
                    else continue;
                }
            if  ( pTag->S == MANDATORY ){
                if(pTag->GrStr==NULL){
                    WriteEdiLog(EDILOG, "Tag %s mandatory but not found in the message",
                                pTag->TagName);
                    SetEdiErrNum_(pCurMes, EDI_SEGM_MANDATORY);
                    SetEdiErrSegm_(pCurMes, pTag->TagName);
                }else {
                    WriteEdiLog(EDILOG, "Segment group %d mandatory but not found in the message",
                                pTag->Num);
                    SetEdiErrNum_(pCurMes, EDI_SEGGR_MANDATORY);
                    SetEdiErrSegGr_(pCurMes, pTag->Num, 1);
                }
#ifndef EDI_DEBUG
                return EDI_MES_STRUCT_ERR;
#endif
            }
            //      EdiTrace(TRACE3,"Skiping <%s>",pTag->TagName);
            pTag = pTag->NextTag;
            if( pTag == NULL ) {
                Ret = ProbeCurentSegGr(pCurMes, SegGrName,Mes,MesTmp,pCurTag,pTagTmp);
                return Ret;
            }
        }/*end while*/
    }/*end while*/
    return EDI_MES_OK;
}

int InsertSegArr(EDI_REAL_MES_STRUCT *pCurMes, char **Mes, char **MesTmp,
                 TAG_STRUCT *pTag, TAG_REAL_STRUCT *pCurTag)
{
    int Ret;
    int MaxR = pTag->MaxR, sch=0;
    char *Mes2, *MesTmp2;
    TAG_REAL_STRUCT *pPrevTag = NULL;

    while( 1 ){
        sch ++;
        if(sch > MaxR){
            WriteEdiLog(EDILOG,"Too many segments: %s\n",pTag->TagName);
            if(pPrevTag){
                pPrevTag->NextTag = NULL;
                free(pCurTag);
            }
#ifdef EDI_DEBUG
            return EDI_MES_OK;
#endif
            SetEdiErrNum_(pCurMes, EDI_SEGM_MUCH);
            SetEdiErrSegmArr_(pCurMes, pTag->TagName, sch);

            return EDI_MES_STRUCT_ERR;
        }
        pCurTag->R = sch;
        Ret = InsertSeg(pCurMes, *MesTmp,pTag,pCurTag);
        if(Ret != EDI_MES_OK){
            SetEdiErrSegmArr_(pCurMes, pTag->TagName, sch);
            return Ret;
        }
        Mes2    = (*Mes)   ;
        MesTmp2 = (*MesTmp);
        tst();
        Ret = GetNextSeg(pCurMes, Mes, MesTmp);
        if (Ret == EDI_MES_STRUCT_ERR){
            SetEdiErrSegmArr_(pCurMes, pTag->TagName, sch);
            return Ret;
        }
        if (Ret == EDI_MES_NOT_FND)
            return EDI_MES_OK;
        if(memcmp(*MesTmp,MesTmp2,TAG_LEN) != 0)  {
            (*Mes)    = Mes2;
            (*MesTmp) = MesTmp2;
            break;
        }
        if(!CreateNewTagReal(&(pCurTag->NextArr)))
            return EDI_MES_ERR;
        pPrevTag = pCurTag;
        pCurTag = pCurTag->NextArr;
        pCurTag->ParTag = pPrevTag->ParTag;
    }/*end while*/
    return EDI_MES_OK;
}

int InsertSeg(EDI_REAL_MES_STRUCT *pCurMes, char *Mes, TAG_STRUCT *pTag, TAG_REAL_STRUCT *pCurTag)
{
    int Ret, flag=0;
    char *CompMes;
    COMPOSITE_STRUCT *pComp;
    COMP_REAL_STRUCT *pCurComp=NULL;

    pCurTag->TemplateTag = pTag;
    if(Mes[TAG_LEN] == '\0'){ /*TAG" - пустой сегмент*/
        Ret = CheckCompGet(pCurMes, pTag->Comp);
        return Ret;
    }

    pComp = pTag->Comp; /*Первый композит данного сегмента(шаблон)*/
    Mes += TAG_LEN + 1;/*Пропускаем TAG+ */
    while( 1 ){
        Ret = GetNextComp_Data(pCurMes, &Mes, &CompMes, pCurMes->pCharSet->EndComp, pCurMes->pCharSet->Release);
        if(Ret == EDI_MES_NOT_FND) {
            return CheckCompGet(pCurMes, pComp);
        }
        if(pComp == NULL)  {
            WriteEdiLog(EDILOG,"Too much composites! <%s> Cannot be found here\n",CompMes);
#ifdef EDI_DEBUG
            return EDI_MES_OK;
#else
            SetEdiErrNum_(pCurMes, EDI_COMP_MUCH_AFTER);
            if(pCurComp){
                if(pCurComp->SingleData){
                    SetEdiErrDataElem_(pCurMes, pCurComp->DataElem->DataElem,-1);
                }else {
                    SetEdiErrComp_(pCurMes, pCurComp->TemplateComp->Composite);
                }
            }
            return EDI_MES_STRUCT_ERR;
#endif
        }
        if(Ret == EDI_MES_OK){/*Нет пропущенного композита!*/
            if(flag){
                if( (pCurComp->Next = (COMP_REAL_STRUCT *)
                     calloc(1, sizeof(COMP_REAL_STRUCT))) == NULL){
                    EdiError(EDILOG,"Can't allocate %zd bytes\n",sizeof(COMP_REAL_STRUCT));
                    return EDI_MES_ERR;
                }
                pCurComp = pCurComp->Next;
            }else {
                if( (pCurComp = (COMP_REAL_STRUCT *)
                     calloc(1,sizeof(COMP_REAL_STRUCT))) == NULL)
                {
                    EdiError(EDILOG,"Can't allocate %zd bytes\n",sizeof(COMP_REAL_STRUCT));
                    return EDI_MES_ERR;
                }
                pCurTag->Comp = pCurComp;
            }
            flag=1;

            pCurComp->TemplateComp = pComp;
            pCurComp->R=1;
            pCurComp->ParTag = pCurTag;

            if(pComp->Composite[0] == '\0'){
                pCurComp->SingleData = 1;
            }

            if(pComp->MaxR > 1) /*Mассив композитов*/
            {
                Ret = InsertCompArr(pCurMes, pCurComp, pComp, &Mes, &CompMes);
                if(Ret == EDI_MES_NOT_FND)
                    return EDI_MES_OK;
                if(Ret != EDI_MES_OK)    {
                    return Ret;
                }
            }
            else { /*MaxR == 1*/
                if(pCurComp->SingleData && pComp->DataElem->MaxR>1) {
                    int len_tmp = strlen(CompMes);
                    pCurComp->SingleData = 1;
                    // 		EdiTrace(TRACE3,"Inserting single array of data element <%d>!",pComp->DataElem->DataElem);
                    if(len_tmp!=0 && strlen(Mes)!=0)
                        CompMes[len_tmp]=pCurMes->pCharSet->EndComp;
                }

                Ret = InsertComp(pCurMes, pCurComp, pComp, &CompMes);

                if(pCurComp->SingleData && pComp->DataElem->MaxR>1)
                    Mes=CompMes;

                if(Ret != EDI_MES_OK){
                    if(!pCurComp->SingleData)
                        SetEdiErrCompArr_(pCurMes, pComp->Composite, 1);
                    return Ret;
                }
            }
        }
        else { /*пропущен композит*/
            Ret = CheckOneCompGet(pCurMes, pComp);
            if(Ret != EDI_MES_OK){
                return Ret;
            }
            if(pComp->MaxR>1){
                if((Ret = SkipCompArr(pCurMes, pComp, &Mes, &CompMes, 1))!=EDI_MES_OK){
                    return (Ret==EDI_MES_NOT_FND)?EDI_MES_OK:Ret;
                }
                if(pComp->Next == NULL)  {
                    WriteEdiLog(EDILOG,"Too much composites! <%s> Cannot be found here\n",CompMes);
#ifdef EDI_DEBUG
                    return EDI_MES_OK;
#else
					SetEdiErrNum_(pCurMes, EDI_COMP_MUCH_AFTER);
					if(pCurComp){
						if(pCurComp->SingleData){
							SetEdiErrDataElem_(pCurMes, pCurComp->DataElem->DataElem,-1);
						}else {
							SetEdiErrComp_(pCurMes, pCurComp->TemplateComp->Composite);
						}
					}
					return EDI_MES_STRUCT_ERR;
#endif
				}
			} else if(pComp->Composite[0] == '\0' && pComp->DataElem->MaxR>1){
				if((Ret = SkipDataArr(pCurMes, pComp->DataElem, &Mes, &CompMes, 1,1))!= EDI_MES_OK){
					return (Ret==EDI_MES_NOT_FND)?EDI_MES_OK:Ret;
				}
				/*EDI_MES_OK*/
				if(pComp->Next == NULL)  {
					WriteEdiLog(EDILOG,"Too much composites! <%s> cannot be found here!\n",CompMes );
#ifdef EDI_DEBUG
					return EDI_MES_OK;
#else
					SetEdiErrNum_(pCurMes, EDI_COMP_MUCH_AFTER);
					if(pCurComp){
						if(pCurComp->SingleData){
							SetEdiErrDataElem_(pCurMes, pCurComp->DataElem->DataElem,-1);
						}else {
							SetEdiErrComp_(pCurMes, pCurComp->TemplateComp->Composite);
						}
					}
					return EDI_MES_STRUCT_ERR;
#endif
				}
			}
		}
		pComp = pComp->Next;
	} /*end while*/
	if(!flag){
		free(pCurComp);
		pCurTag->Comp = NULL;
	}
	return EDI_MES_OK;
}

int SkipCompArr(EDI_REAL_MES_STRUCT *pCurMes, COMPOSITE_STRUCT *pComp,
                char **Mes, char **MesTmp, int sch)
{
    int Ret;
    int i;
    int MaxR = pComp->Composite[0] == '\0'?pComp->DataElem->MaxR:pComp->MaxR;

    if(sch==1){
        return EDI_MES_OK;
    } else if(sch<1){
        EdiError(EDILOG,"SkipCompArr(), sch=%d!!! Should be >1", sch);
        return EDI_MES_ERR;
    }
    for(i=sch; i<MaxR; i++ ){
        Ret = GetNextComp_Data(pCurMes, Mes, MesTmp, pCurMes->pCharSet->EndComp, pCurMes->pCharSet->Release);
        if(Ret == EDI_MES_NOT_FND)
            return Ret;
        if(Ret == EDI_MES_OK){/*Нет пропуска композита*/
            WriteEdiLog(EDILOG,"Composite <%s> should be missed!\n",*MesTmp);
#ifdef EDI_DEBUG
            return EDI_MES_OK;
#else
			SetEdiErrNum_(pCurMes, EDI_COMP_MISSED);
			SetEdiErrCompArr_(pCurMes,pComp->Composite, i);
			return EDI_MES_STRUCT_ERR;
#endif
        }
    }/*end for(...)*/
    return EDI_MES_OK;
}

int InsertCompArr(EDI_REAL_MES_STRUCT *pCurMes, COMP_REAL_STRUCT *pCurComp, COMPOSITE_STRUCT *pComp,
                  char **Mes, char **MesTmp)
{
    int sch=1,MaxR;
    int Ret;
    TAG_REAL_STRUCT *pParTag = pCurComp->ParTag;

    Ret = InsertComp(pCurMes, pCurComp, pComp, MesTmp);
    if(Ret != EDI_MES_OK){
        SetEdiErrCompArr_(pCurMes,pComp->Composite, sch);
        return Ret;
    }
    pCurComp->R = sch;
    MaxR = pComp->MaxR;/*Максимальное кол-во повт. композита*/
    while( 1 ){
        if(sch == MaxR)/*Закончили формирование массива!*/
            return EDI_MES_OK;
        Ret = GetNextComp_Data(pCurMes, Mes, MesTmp, pCurMes->pCharSet->EndComp, pCurMes->pCharSet->Release);
        if(Ret == EDI_MES_NOT_FND)
            return Ret;
        sch++;/*Счетчик композитов*/
        if(Ret == EDI_MES_OK)/*Нет пропуска!*/{
            if( (pCurComp->NextArr = (COMP_REAL_STRUCT *)
                 calloc(1, sizeof(COMP_REAL_STRUCT))) == NULL){
                EdiError(EDILOG,"Can't allocate %zd bytes\n",sizeof(COMP_REAL_STRUCT));
                return EDI_MES_ERR;
            }
            pCurComp = pCurComp->NextArr;
            pCurComp->TemplateComp = pComp;
            pCurComp->ParTag = pParTag;
            Ret = InsertComp(pCurMes, pCurComp, pComp, MesTmp);
            if(Ret != EDI_MES_OK){
                SetEdiErrCompArr_(pCurMes,pComp->Composite, sch);
                return Ret;
            }
            pCurComp->R = sch;
        } else {
            return SkipCompArr(pCurMes, pComp, Mes, MesTmp, sch);
        }
    }/*end while*/
}

int InsertComp(EDI_REAL_MES_STRUCT *pCurMes, COMP_REAL_STRUCT *pCurComp, COMPOSITE_STRUCT *pComp,
               char **Mes)
{
    int Ret, sch=0, flag=0;
    char *MesTmp;
    DATA_ELEM_STRUCT *pData;
    DATA_REAL_STRUCT *pCurData;
    COMP_REAL_STRUCT *pParComp=pCurComp;

    if( (pCurData = (DATA_REAL_STRUCT *)
         calloc(1, sizeof(DATA_REAL_STRUCT))) == NULL)
    {
        EdiError(EDILOG,"Can't allocate %zd bytes\n",sizeof(DATA_REAL_STRUCT));
        return EDI_MES_ERR;
    }

    pCurComp->DataElem = pCurData;
    pData = pComp->DataElem; /*Первый эл данного композита (шаблон)*/
    pCurData->TemplateData = pData;
    pCurData->ParComp = pParComp;
    while( 1 ){
        sch++;
        Ret = GetNextComp_Data(pCurMes, Mes, &MesTmp, pCurComp->SingleData?pCurMes->pCharSet->EndComp:pCurMes->pCharSet->EndData, pCurMes->pCharSet->Release);

        if(Ret == EDI_MES_NOT_FND) {
            Ret = CheckDataGet(pCurMes, pData);
            if(!flag){
                free(pCurData);
                pCurComp->DataElem = NULL;
            }
            return Ret;
        }
        if(pData == NULL)  {
            WriteEdiLog(EDILOG,"Too much data_elements! <%s> cannot be found here!\n",MesTmp );
#ifdef EDI_DEBUG
            return EDI_MES_OK;
#else
            SetEdiErrNum_(pCurMes, EDI_DATA_MUCH_AFTER);
            if(pCurData)
                SetEdiErrDataElem_(pCurMes, pCurData->TemplateData->DataElem,-1);
            return EDI_MES_STRUCT_ERR;
#endif
        }
        if(Ret == EDI_MES_OK){/*Нет пропущенного композита!*/
            if(pCurComp->SingleData && !CheckForBadChars(MesTmp, pCurMes->pCharSet->Release, pCurMes->pCharSet->EndData)){
                WriteEdiLog(EDILOG,"Invalid separator in the single data element !");
#ifdef EDI_DEBUG
                return EDI_MES_OK;
#else
				SetEdiErrNum_(pCurMes, EDI_DATA_BAD_SIPARAT);
				SetEdiErrDataElem_(pCurMes, pData->DataElem,-1);
				return EDI_MES_STRUCT_ERR;
#endif
			}

			if(flag){
				if( (pCurData->Next = (DATA_REAL_STRUCT *)
					 calloc(1, sizeof(DATA_REAL_STRUCT))) == NULL){
					EdiError(EDILOG,"Can't allocate %zd bytes\n",sizeof(DATA_REAL_STRUCT));
					return EDI_MES_ERR;
				}
				pCurData = pCurData->Next;
				pCurData->TemplateData = pData;
				pCurData->R=1;
				pCurData->ParComp = pParComp;
			}
			flag=1;
			if(pData->MaxR > 1) /*Mассив элем. данных*/
			{
				Ret = InsertDataArr(pCurMes, pCurData , pData, Mes, &MesTmp, pCurComp->SingleData);
				if(Ret == EDI_MES_NOT_FND)
					return EDI_MES_OK;
				if(Ret != EDI_MES_OK)
					return Ret;
				if(pCurComp->SingleData) {
					// 	  	EdiTrace(TRACE3,"Вводили массив одиноких эл. данных!");
					return EDI_MES_OK;
				}
			}
			else { /*MaxR == 1*/
				pCurData->len = strlen(MesTmp);
				pCurData->Data = MesTmp;
				pCurData->TemplateData = pData;
				pCurData->DataElem = pData->DataElem;
				pCurData->R=1;

				Ret = CheckDataByRuleGet(pCurMes, pData, pCurData);
				/*Проверка данных согласно правилам*/
				if (Ret != EDI_MES_OK){
					SetEdiErrDataElem_(pCurMes, pData->DataElem, 1);
					return Ret;
				}
			}
		}
		else { /*пропущен композит*/
			Ret = CheckOneDataGet(pCurMes, pData);
			if(Ret != EDI_MES_OK)
				return Ret;
			if(pData->MaxR>1){
				if((Ret = SkipDataArr(pCurMes, pData, Mes, &MesTmp, 1,pCurComp->SingleData))!=EDI_MES_OK){
					return (Ret==EDI_MES_NOT_FND)?EDI_MES_OK:Ret;
				}
				if(pData->Next == NULL && pCurComp->SingleData==0)  {
					WriteEdiLog(EDILOG,"Too much data_elements! <%s> cannot be found here!\n",MesTmp );
#ifdef EDI_DEBUG
					return EDI_MES_OK;
#else
					SetEdiErrNum_(pCurMes, EDI_DATA_MUCH_AFTER);
					if(pCurData)
						SetEdiErrDataElem_(pCurMes, pCurData->TemplateData->DataElem,-1);
					return EDI_MES_STRUCT_ERR;
#endif
				}
			}

		}
		pData = pData->Next;
	} /*end while*/
	if(!flag){
		free(pCurData);
		pCurComp->DataElem = NULL;
	}
	return EDI_MES_OK;
}

int InsertDataArr(EDI_REAL_MES_STRUCT *pCurMes, DATA_REAL_STRUCT *pCurData, DATA_ELEM_STRUCT *pData,
                  char **Mes, char **MesTmp, int dataflag)
{
    int sch=1,MaxR;
    int Ret;
    COMP_REAL_STRUCT *pParComp = pCurData->ParComp;

    pCurData->len = strlen(*MesTmp);
    pCurData->Data = *MesTmp;
    pCurData->DataElem = pData->DataElem;
    pCurData->R = sch;
    Ret = CheckDataByRuleGet(pCurMes, pData, pCurData);
    /*Проверка данных согласно правилам*/
    if (Ret != EDI_MES_OK){
        SetEdiErrDataElem_(pCurMes, pData->DataElem, sch);
        return Ret;
    }

    MaxR = pData->MaxR;/*Максимальное кол-во повт. эл. данных*/
    while( 1 ){
        if(sch == MaxR)/*Закончили формирование массива!*/
            return EDI_MES_OK;
        Ret = GetNextComp_Data(pCurMes, Mes, MesTmp,dataflag?pCurMes->pCharSet->EndComp:pCurMes->pCharSet->EndData, pCurMes->pCharSet->Release);
        if(Ret == EDI_MES_NOT_FND)
            return Ret;
        if(Ret == EDI_MES_OK)/*Нет пропуска!*/{
            if( (pCurData->NextArr = (DATA_REAL_STRUCT *)
                 calloc(1, sizeof(DATA_REAL_STRUCT))) == NULL){
                EdiError(EDILOG,"Can't allocate %zd bytes\n",sizeof(DATA_REAL_STRUCT));
                return EDI_MES_ERR;
            }
            if(dataflag && !CheckForBadChars(*MesTmp, pCurMes->pCharSet->Release, pCurMes->pCharSet->EndData)){
                WriteEdiLog(EDILOG,"Invalid separator in the array of the single data elements !");
#ifdef EDI_DEBUG
                return EDI_MES_OK;
#else
				SetEdiErrNum_(pCurMes, EDI_DATA_BAD_ARR_SIPARAT);
				SetEdiErrDataElem_(pCurMes, pData->DataElem, ++sch);
				return EDI_MES_STRUCT_ERR;
#endif
			}
			pCurData = pCurData->NextArr;
			pCurData->TemplateData = pData;

			pCurData->Data = *MesTmp;
			pCurData->len  = strlen(*MesTmp);
			pCurData->DataElem = pData->DataElem;
			pCurData->R = ++sch; /*Счетчик эл. данных*/
			pCurData->ParComp=pParComp;
			Ret = CheckDataByRuleGet(pCurMes, pData, pCurData);
			/*Проверка данных согласно правилам*/
			if (Ret != EDI_MES_OK){
				SetEdiErrDataElem_(pCurMes, pData->DataElem, sch);
				return Ret;
			}

        } else {
            return SkipDataArr(pCurMes, pData, Mes, MesTmp, sch, dataflag);
        }
    }/*end while*/
}

int SkipDataArr(EDI_REAL_MES_STRUCT *pCurMes, DATA_ELEM_STRUCT *pData,
                char **Mes, char **MesTmp, int sch, int dataflag)
{
    int MaxR = pData->MaxR;
    int Ret;
    int i;

    if(sch==1){
        return EDI_MES_OK;
    } else if(sch<1){
        EdiError(EDILOG,"SkipDataArr(), sch=%d!!! Should be >1", sch);
        return EDI_MES_ERR;
    }
    for( i=sch;i<MaxR;i++ ){
        Ret = GetNextComp_Data(pCurMes, Mes, MesTmp, dataflag?pCurMes->pCharSet->EndComp:pCurMes->pCharSet->EndData, pCurMes->pCharSet->Release);
        if(Ret == EDI_MES_NOT_FND) /*Нет ничего*/
            return Ret;
        else  if(Ret == EDI_MES_OK){/*Нет пропуска эл. данных*/
            WriteEdiLog(EDILOG,"Data element <%s> should be missed\n",*MesTmp);
#ifdef EDI_DEBUG
            return EDI_MES_OK;
#else
			SetEdiErrNum_(pCurMes, EDI_DATA_MISSED);
			SetEdiErrDataElem_(pCurMes, pData->DataElem, i);
			return EDI_MES_STRUCT_ERR;
#endif
        }
    }/*end while*/
    return EDI_MES_OK;
}

int ProbeCurentSegGr(EDI_REAL_MES_STRUCT *pCurMes, char *SegGrName, char **Mes, char **MesTmp,
                     TAG_REAL_STRUCT *pCurTag, TAG_STRUCT *pTagTmp)
/*Проверяет, не началaсь ли данная сегм. группа сначала*/
{
    int Ret;
    TAG_REAL_STRUCT *pCurTagTmp;

    if ( memcmp(SegGrName,*MesTmp,TAG_LEN) == 0 ){
        if( !CreateNewTagReal( &pCurTagTmp ) )
            return EDI_MES_ERR;
        pCurTag->NextArr = pCurTagTmp;
        //         EdiTrace(TRACE3,"Testing segment group %d:%li <%s>",pTagTmp->Num,pCurTag->R+1,(pTagTmp->GrStr)->TagName);
     	Ret = InsertSegGr(pCurMes, Mes, MesTmp, pTagTmp, pCurTagTmp, pCurTag->R);
        if(Ret != EDI_MES_OK){
     	    return Ret;
        }
    }
    return EDI_MES_OK;
}

int GetNextSeg_(EDI_REAL_MES_STRUCT *pCurMes, char **Mes, char **MesTmp, int ro)
{
    static char *Last=NULL; /*Т.к м.б вызвана 2-ды для одного сегмента*/
    int ended=0;

    if((*Mes)[0] == '\0')
        return EDI_MES_NOT_FND;

    *MesTmp = *Mes;
    while( (*Mes)[0] != '\0' ){
        int len = 0;
        if( ( (*Mes)[0] == pCurMes->pCharSet->Release ) && /*'??' || '?+'*/
                (( len = 1,(*Mes)[1] == pCurMes->pCharSet->Release ) ||
                 (len=pCurMes->pCharSet->EndSegLen,
                  !memcmp( &((*Mes)[1]), pCurMes->pCharSet->EndSegStr, len)) )){
            (*Mes) += len;
        } else if(!memcmp( &((*Mes)[0]), pCurMes->pCharSet->EndSegStr, pCurMes->pCharSet->EndSegLen) ){  /*'+'*/
            if (!ro)
                (*Mes)[0] = '\0';
            ended++;
            Last = *MesTmp;
            IncEdiSegment(pCurMes);
            break;
        }
        (*Mes) ++;
    }
    (*Mes)+=pCurMes->pCharSet->EndSegLen;

    //     EdiTrace(TRACE5, "ended=%d, Last(static) = %p, MesTmp = %p, MesTmp='%s'",
    //              ended, Last, *MesTmp, *MesTmp);

    if(!ended && Last!=(*MesTmp)){
        EdiTrace((ro==2)?CHECK_LOG_LEVEL:CHECK_LOG_LEVEL,EDILOG,
                 "Segment %s is not ended", *MesTmp);
#ifdef EDI_DEBUG
        return EDI_MES_OK;
#else
        SetEdiErrNum_(pCurMes, EDI_SEGM_NOTENDED);
        SetEdiErrSegm_(pCurMes, *MesTmp);
        return EDI_MES_STRUCT_ERR;
#endif
    }

    return EDI_MES_OK;
}

int GetNextSeg(EDI_REAL_MES_STRUCT *pCurMes, char **Mes, char **MesTmp)
{
    return GetNextSeg_(pCurMes,Mes,MesTmp,0);
}

/*Read only*/
const char * GetNextSegRO(EDI_REAL_MES_STRUCT *pCurMes, const char *Mes, short check)
{
    char *MesTmp2, *Mes2=(char *)Mes;

    if(GetNextSeg_(pCurMes, &Mes2, &MesTmp2, (check)?2:1)==EDI_MES_OK){
        return Mes2;
    }

    tst();
    return NULL;
}

int GetNextComp_Data_(EDI_REAL_MES_STRUCT *pCurMes, char **Mes,
					  char **MesTmp, const char Separator,
					  const char Release, int ro)
{
    int flag=0;
    if((*Mes)[0] == '\0') return EDI_MES_NOT_FND;
    if((*Mes)[0] == Separator){
        if(Separator == pCurMes->pCharSet->EndComp){
            IncEdiComposite(pCurMes);
        }else {
            IncEdiDataElem(pCurMes);
        }
        *MesTmp = ++(*Mes);
        return EDI_MES_ERR; /*Не ошибка! */
        /*Просто наткнулись на пропущенный композит(++)/элемент данных(::)*/
    }
    *MesTmp = *Mes;

    while( (*Mes)[0] != '\0' ){
        if( ( (*Mes)[0] == Release ) && /*'??' || '?+'*/
                (( (*Mes)[1] == Release ) ||
                 ( (*Mes)[1] == Separator) )){
            (*Mes) ++;
        } else if( (*Mes)[0] == Separator ){  /*'+'*/
            if(!ro)
                (*Mes)[0] = '\0';
            flag=1;
            if(Separator == pCurMes->pCharSet->EndComp){
                IncEdiComposite(pCurMes);
            }else {
                IncEdiDataElem(pCurMes);
            }
            break;
        }
        (*Mes) ++;
    }

    if(flag)
        (*Mes) ++;
    return EDI_MES_OK;
}

int GetNextComp_Data(EDI_REAL_MES_STRUCT *pCurMes, char **Mes,
					 char **MesTmp, const char Separator,
					 const char Release)
{
    return GetNextComp_Data_(pCurMes, Mes, MesTmp, Separator,Release,0);
}

const char * GetNextCompRO(EDI_REAL_MES_STRUCT *pCurMes, const char *Mes)
{
    char *MesTmp2, *Mes2=(char *)Mes;

    if(GetNextComp_Data_(pCurMes, &Mes2, &MesTmp2,
                         pCurMes->pCharSet->EndComp,
                         pCurMes->pCharSet->Release,1)==EDI_MES_OK){
        return Mes2;
    }

    tst();
    return NULL;
}

const char * GetNextDataRO(EDI_REAL_MES_STRUCT *pCurMes, const char *Mes)
{
    char *MesTmp2, *Mes2=(char *)Mes;

    if(GetNextComp_Data_(pCurMes, &Mes2, &MesTmp2,
                         pCurMes->pCharSet->EndData,
                         pCurMes->pCharSet->Release,1)==EDI_MES_OK){
        return Mes2;
    }

    tst();
    return NULL;
}


int CheckTagsGet(EDI_REAL_MES_STRUCT *pCurMes, TAG_STRUCT *pTag)
{
    return CheckTags_(pCurMes, pTag, 1);
}

int CheckTagsInsert(EDI_REAL_MES_STRUCT *pCurMes, TAG_STRUCT *pTag)
{
    return CheckTags_(pCurMes, pTag, 0);
}

int CheckTags_(EDI_REAL_MES_STRUCT *pCurMes, TAG_STRUCT *pTag, int get)
{
    while(pTag != NULL){
        if(pTag->S == MANDATORY) {
            if(pTag->GrStr){
                WriteEdiLog(EDILOG,"Segment group <%d:%s> mandatory but not found!\n",
                            pTag->Num, pTag->GrStr->TagName);
#ifdef EDI_DEBUG
                return EDI_MES_OK;
#endif
				if(get){
					SetEdiErrNum_(pCurMes, EDI_SEGGR_MANDATORY);
					SetEdiErrSegGr_(pCurMes, pTag->Num, 1);
				}
			}else {
				WriteEdiLog(EDILOG,"Tag <%s> mandatory but not found!\n",
							pTag->TagName);
#ifdef EDI_DEBUG
				return EDI_MES_OK;
#endif
				if(get){
					SetEdiErrNum_(pCurMes, EDI_SEGM_MANDATORY);
					SetEdiErrSegm_(pCurMes, pTag->TagName);
				}
			}
			return EDI_MES_STRUCT_ERR;
		}
		pTag = pTag->NextTag;
	}
	return EDI_MES_OK;
}

int CheckOneCompGet(EDI_REAL_MES_STRUCT *pCurMes, COMPOSITE_STRUCT *pComp)
{
    return CheckOneComp_(pCurMes, pComp, 1);
}

int CheckOneCompInsert(EDI_REAL_MES_STRUCT *pCurMes, COMPOSITE_STRUCT *pComp)
{
    return CheckOneComp_(pCurMes, pComp, 0);
}

int CheckOneComp_(EDI_REAL_MES_STRUCT *pCurMes, COMPOSITE_STRUCT *pComp, int get)
{
    if((pComp->S == MANDATORY) ||
            (pComp->Composite[0]=='\0' && pComp->DataElem->S==MANDATORY)) {
        if(pComp->Composite[0]!='\0'){
            WriteEdiLog(EDILOG,"Composite <%s> mandatory but not found!\n",
                        pComp->Composite);
#ifdef EDI_DEBUG
            return EDI_MES_OK;
#endif
			if(get){
				SetEdiErrNum_(pCurMes, EDI_COMP_MANDATORY);
				SetEdiErrComp_(pCurMes, pComp->Composite);
			}

        }else {
            WriteEdiLog(EDILOG,"Data element <%d> mandatory but not found!\n",
                        pComp->DataElem->DataElem);
#ifdef EDI_DEBUG
            return EDI_MES_OK;
#endif
			if(get){
				SetEdiErrNum_(pCurMes, EDI_DATA_MANDATORY);
				SetEdiErrDataElem_(pCurMes, pComp->DataElem->DataElem,-1);
			}
		}
		return EDI_MES_STRUCT_ERR;
    }
    return EDI_MES_OK;
}

int CheckCompGet(EDI_REAL_MES_STRUCT *pCurMes, COMPOSITE_STRUCT *pComp)
{
    return CheckComp_(pCurMes, pComp, 1);
}

int CheckCompInsert(EDI_REAL_MES_STRUCT *pCurMes, COMPOSITE_STRUCT *pComp)
{
    return CheckComp_(pCurMes, pComp, 0);
}

int CheckComp_(EDI_REAL_MES_STRUCT *pCurMes, COMPOSITE_STRUCT *pComp, int get)
{
    while(pComp){
        if(pComp->S == MANDATORY) {
            WriteEdiLog(EDILOG,"Composite <%s> mandatory but not found!",
                        pComp->Composite);
#ifndef EDI_DEBUG
			if(get){
				SetEdiErrNum_(pCurMes, EDI_COMP_MANDATORY);
				SetEdiErrComp_(pCurMes, pComp->Composite);
			}
			return EDI_MES_STRUCT_ERR;
#endif
        }else  if(pComp->Composite[0]=='\0' && pComp->DataElem->S == MANDATORY){
            WriteEdiLog(EDILOG,"Single data element <%d> mandatory but not found!",
                        pComp->DataElem->DataElem);
#ifndef EDI_DEBUG
			if(get){
				SetEdiErrNum_(pCurMes, EDI_DATA_MANDATORY);
				SetEdiErrDataElem_(pCurMes, pComp->DataElem->DataElem,-1);
			}
			return EDI_MES_STRUCT_ERR;
#endif
        }
        pComp = pComp->Next;
    }
    return EDI_MES_OK;
}

int CheckOneDataGet(EDI_REAL_MES_STRUCT *pCurMes, DATA_ELEM_STRUCT *pData)
{
    return CheckOneData_(pCurMes, pData, 1);
}

int CheckOneDataInsert(EDI_REAL_MES_STRUCT *pCurMes, DATA_ELEM_STRUCT *pData)
{
    return CheckOneData_(pCurMes, pData, 0);
}

int CheckOneData_(EDI_REAL_MES_STRUCT *pCurMes, DATA_ELEM_STRUCT *pData, int get)
{
    if(pData->S == MANDATORY) {
        WriteEdiLog(EDILOG,"Data element <%d> mandatory but not found!\n",
                    pData->DataElem);
#ifndef EDI_DEBUG
		if(get){
			SetEdiErrNum_(pCurMes, EDI_DATA_MANDATORY);
			SetEdiErrDataElem_(pCurMes, pData->DataElem,-1);
		}
		return EDI_MES_STRUCT_ERR;
#endif
    }
    return EDI_MES_OK;
}

int CheckDataGet(EDI_REAL_MES_STRUCT *pCurMes, DATA_ELEM_STRUCT *pData)
{
    return CheckData_(pCurMes, pData, 1);
}

int CheckDataInsert(EDI_REAL_MES_STRUCT *pCurMes, DATA_ELEM_STRUCT *pData)
{
    return CheckData_(pCurMes, pData, 0);
}

int CheckData_(EDI_REAL_MES_STRUCT *pCurMes, DATA_ELEM_STRUCT *pData, int get)
{
    while(pData !=NULL){
        if(pData->S == MANDATORY) {
            WriteEdiLog(EDILOG,"Data element <%d> mandatory but not found!\n",
                        pData->DataElem);
#ifndef EDI_DEBUG
			if(get){
				SetEdiErrNum_(pCurMes, EDI_DATA_MANDATORY);
				SetEdiErrDataElem_(pCurMes, pData->DataElem,-1);
			}
			return EDI_MES_STRUCT_ERR;
#endif
		}
		pData = pData->Next;
    }
    return EDI_MES_OK;
}

int CreateNewTagReal(TAG_REAL_STRUCT **pCurTag)
{
	if( (*pCurTag = (TAG_REAL_STRUCT *)
		 calloc(1,sizeof(TAG_REAL_STRUCT))) == NULL){
	    EdiError(EDILOG,"Can't allocate %zd bytes\n",sizeof(TAG_REAL_STRUCT));
	    return 0;
	}
	return 1;
}

int CreateNewCompReal(COMP_REAL_STRUCT **pComp)
{
	if( (*pComp = (COMP_REAL_STRUCT *)
		 calloc(1,sizeof(COMP_REAL_STRUCT))) == NULL){
	    EdiError(EDILOG,"Can't allocate %zd bytes\n",sizeof(COMP_REAL_STRUCT));
	    return 0;
	}
	return 1;
}

int CreateNewDataReal(DATA_REAL_STRUCT **pData)
{
    if( (*pData = (DATA_REAL_STRUCT *)
         calloc(1,sizeof(DATA_REAL_STRUCT))) == NULL){
        EdiError(EDILOG,"Can't allocate %zd bytes\n",sizeof(DATA_REAL_STRUCT));
        return 0;
    }
    return 1;
}

int CheckForBadChars(const char *Mes, char Release, char EndData)
{
    while ( Mes[0] != '\0'){
        if(Mes[0] == Release &&
                Mes[1] == Release){
            Mes ++;
        }else if (Mes[0]!=Release &&
                  Mes[1]==EndData){
            return 0;
        }
        Mes ++;
    }
    return 1;
}

int CheckDataByRuleGet(EDI_REAL_MES_STRUCT *pMes, DATA_ELEM_STRUCT *pTData, DATA_REAL_STRUCT *pRData)
{
    return CheckDataByRule_(pMes, pTData, pRData, 1);
}

int CheckDataByRuleInsert(EDI_REAL_MES_STRUCT *pMes, DATA_ELEM_STRUCT *pTData, DATA_REAL_STRUCT *pRData)
{
    return CheckDataByRule_(pMes, pTData, pRData, 0);
}

/*Записывает длину*/
int CheckDataByRule_(EDI_REAL_MES_STRUCT *pMes, DATA_ELEM_STRUCT *pTData,
                     DATA_REAL_STRUCT *pRData, int get)
{
    /* int Ret;*/
    if(get){
        DeleteEdiMaskChars2(pMes, pRData);
    }

    switch(pTData->Format){
    case DATA_FORMAT1:/*Alpha-Numeric*/
        /*Любые символы!*/
        break;
    case DATA_FORMAT2: /*Numeric*/
        /*Все, кроме alphabetic*/
        if ( !CheckNumeric(pRData->Data, pRData->len) ){
            WriteEdiLog(EDILOG,"Error in %d data element \n",pTData->DataElem);
#ifndef EDI_DEBUG
            if(get) SetEdiErrNum_(pMes, EDI_DATA_ALPHA);
            return EDI_MES_STRUCT_ERR;
#endif
		}
		break;
    case DATA_FORMAT3: /*Alphabetic*/
        /*Все, кроме numeric*/
        if( !CheckAlphabetic(pRData->Data, pRData->len) ){
            WriteEdiLog(EDILOG,"Error in %d data element \n",pTData->DataElem);
#ifndef EDI_DEBUG
            if(get) SetEdiErrNum_(pMes, EDI_DATA_DIGIT);
            return EDI_MES_STRUCT_ERR;
#endif
		}
		break;
    default :
        EdiError(EDILOG,"Unknown %c - format of the data element!\n",pTData->Format);
#ifndef EDI_DEBUG
        return EDI_MES_ERR;
#endif
        break;
    }

    int data_length = pRData->len;
    if(data_length > 1 && pTData->Format == DATA_FORMAT2 && *pRData->Data == '-') {
        data_length -= 1;
    }

    if((data_length > pTData->MaxField) || (data_length < pTData->MinField)) {
        WriteEdiLog(EDILOG,"DataElem: %d. Min = %d, Max = %d, Data = %s",
                    pTData->DataElem, pTData->MinField, pTData->MaxField, pRData->Data);
#ifndef EDI_DEBUG
        if(get) {
            if(data_length > pTData->MaxField) {
                SetEdiErrNum_(pMes, EDI_DATA_TOO_LONG);
            } else {
                SetEdiErrNum_(pMes, EDI_DATA_TOO_SHORT);
            }
            SetEdiErrDataElem_(pMes, pTData->DataElem, -1);
        }
        return EDI_MES_STRUCT_ERR;
#endif
    }

    return EDI_MES_OK;
}

int CheckAlphabetic(const char *Mes, int len)
{
    int i;
    for ( i=0; i<len; i++){
        if (ISDIGIT((int)Mes[i])){
            WriteEdiLog(EDILOG,"%s - whith digit characters, "
                        "but should be alphabetic! \n",Mes);
            return 0;
        }
    }
    return 1;
}

int CheckNumeric(const char *Mes, int len)
{
    int i;
    int have_comma = 0;
    /*******************
     В некоторых сообщениях мешаются пробелы
     не учитываем их... (TVL IFT segments)
     *******************/
    for (i = 0; i < len; i++) {
        if (Mes[i] == '-' && i == 0 && len > 1) {
            // minus sign
        } else if (!have_comma && len > 1 && (Mes[i] == EDI_FULL_STOP || Mes[i] == EDI_COMMA)) {
            have_comma ++;
        } else if (ISDIGIT((int)Mes[i])) {
            // OK
        } else {
            WriteEdiLog(EDILOG,"%s - whith alpha characters, but should be numeric! \n", Mes);
            return 0;
        }
    }
    return 1;
}

int CountEdiSegments_(EDI_REAL_MES_STRUCT *pMes)
{
    int count=0;

    if(!pMes || !pMes->Tag){
        EdiError(EDILOG,"CountEdiSegments: NULL pointer");
        return -1;
    }

    CountEdiTags(pMes->Tag, &count);
    return count;
}

int CountEdiTags(TAG_REAL_STRUCT *pTag, int *count)
{
    while(pTag) {
        if(pTag->GrStr){
            CountEdiTags(pTag->GrStr, count);
        } else if (!strcmp(pTag->TemplateTag->TagName, "UNE") ||
                   !strcmp(pTag->TemplateTag->TagName, "UNG")) {
            EdiTrace(TRACE2,"ignore tag %s", pTag->TemplateTag->TagName);
            // Ignore system segments
        } else {
            (*count)++;
        }
        CountEdiTags(pTag->NextArr, count);
        pTag = pTag->NextTag;
    }
    return 0;
}

int TemplateTagByName(const char *Mes, TAG_STRUCT *pTag)
{
    while(pTag){
        if(!memcmp(Mes, pTag->TagName, TAG_LEN)){
            return 1;
        }
        pTag  = pTag->NextTag;
    }
    return 0;
}
