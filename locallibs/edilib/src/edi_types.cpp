#include <stdlib.h>
#include <cstring>
#include <algorithm>

#include "edi_types.h"
#include "edi_user_func.h"
#include "edi_tables.h"
#include "edi_func.h"
#include "edi_malloc.h"
#include "edi_err.h"
#include "edi_sess.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE

#include "edi_test.h"

int cmp_edi_types(const EDI_MSG_TYPE *t1, const EDI_MSG_TYPE *t2);
int cmp_edi_names(const EDI_MSG_TYPE *t1, const EDI_MSG_TYPE *t2);


/******************************************************************/
/* Ассоциирует программные типы сообщений со считанными шаблонами */
/* return 0 - Ok                                                  */
/* return -1- Error                                               */
/******************************************************************/
int InitEdiTypes(EDI_MSG_TYPE *types, size_t num)
{
    return InitEdiTypes_(GetEdiTemplateMessages(), types, num);
}

int InitEdiTypes_(EDI_MESSAGES_STRUCT *pMesTemp,
                  EDI_MSG_TYPE *types, size_t num)
{
    for(size_t i=0; i<num; i++)
    {
        types[i].pTemp=GetEdiTemplateByName(pMesTemp,types[i].code);
        if(types[i].pTemp == NULL){
            EdiError(EDILOG,"InitTypes: %s: No such code in the templates",types[i].code);
            return EDI_MES_NOT_FND;
        }
        EdiTrace(TRACE3,"Init type %d for %s", types[i].type, types[i].code);
        types[i].pTemp->Type.type = types[i].type;
    }

    /*Проинициализировали*/
    /*теперь хешируем по имени и типу*/

    pMesTemp->HBNTypes = static_cast<EDI_MSG_TYPE *> (edi_calloc(num, sizeof(EDI_MSG_TYPE)));
    pMesTemp->HBTTypes = static_cast<EDI_MSG_TYPE *> (edi_calloc(num, sizeof(EDI_MSG_TYPE)));

    if(!pMesTemp->HBNTypes || !pMesTemp->HBTTypes){
        EdiError(EDILOG,"Calloc(%zd,%zd) failed", num, sizeof(EDI_MSG_TYPE));
        edi_free(pMesTemp->HBNTypes);
        edi_free(pMesTemp->HBTTypes);
        return EDI_MES_ERR;
    }

    memcpy(pMesTemp->HBNTypes, types, sizeof(EDI_MSG_TYPE)*num);
    memcpy(pMesTemp->HBTTypes, types, sizeof(EDI_MSG_TYPE)*num);
    pMesTemp->tnum=num;

    qsort(pMesTemp->HBNTypes, pMesTemp->tnum, sizeof(EDI_MSG_TYPE), (int(*)(const void *, const void *))cmp_edi_names);
    qsort(pMesTemp->HBTTypes, pMesTemp->tnum, sizeof(EDI_MSG_TYPE), (int(*)(const void *, const void *))cmp_edi_types);

    return EDI_MES_OK;
}

/*******************************************************************/
/* Загружает набор управляющих символов,                           */
/* привязывая их к конкретному шаблону.                            */
/* Если функция не выполнена, будут использов. символы по умолчан. */
/*******************************************************************/
int InitEdiCharSet(const edi_loaded_char_sets *char_set, size_t num)
{
    return InitEdiCharSet_(GetEdiTemplateMessages(), char_set, num);
}

int InitEdiCharSet_(EDI_MESSAGES_STRUCT *pMesTemp,
                    const edi_loaded_char_sets *char_set, size_t num)
{
    if(pMesTemp->char_set_arr != nullptr and pMesTemp->chsnum != num)
    {
        delete[] pMesTemp->char_set_arr;
        pMesTemp->char_set_arr = nullptr;
    }
    if(pMesTemp->char_set_arr == nullptr)
        pMesTemp->char_set_arr = new edi_loaded_char_sets[num];

    std::copy_n(char_set, num, pMesTemp->char_set_arr);
    pMesTemp->chsnum = num;

    return EDI_MES_OK;
}

edi_msg_types_t GetEdiMsgTypeByName(const char *mname)
{
    return GetEdiMsgTypeByName_(GetEdiTemplateMessages(), mname);
}

edi_msg_types_t GetEdiMsgTypeByName_(EDI_MESSAGES_STRUCT *pMesTemp,
                                     const char *mname)
{
    EDI_MSG_TYPE *pEType= GetEdiMsgTypeStrByName_(pMesTemp, mname);

    if(pEType){
        return pEType->type;
    }else {
        EdiTrace(TRACE3,"No such edifact message: %s", mname);
        return EDINOMES;
    }
}

edi_msg_types_t GetEdiAnswerByType(edi_msg_types_t type)
{
    return GetEdiAnswerByType_(GetEdiTemplateMessages(), type);
}

edi_msg_types_t GetEdiAnswerByType_(EDI_MESSAGES_STRUCT * pMesTemp, edi_msg_types_t type)
{
    EDI_MSG_TYPE *pEType= GetEdiMsgTypeStrByType_(pMesTemp, type);

    if(pEType)
    {
        return pEType->answer_type;
    }
    else
    {
        EdiTrace(TRACE3,"No such edifact message type: %d", type);
        return EDINOMES;
    }
}

const char * GetEdiMsgNameByType(edi_msg_types_t type)
{
    return GetEdiMsgNameByType_(GetEdiTemplateMessages(), type);
}

const char * GetEdiMsgNameByType_(EDI_MESSAGES_STRUCT *pMesTemp,
                                  edi_msg_types_t type)
{
    EDI_MSG_TYPE *pEType= GetEdiMsgTypeStrByType_(pMesTemp, type);

    if(pEType)
    {
        return pEType->code;
    }
    else
    {
        EdiTrace(TRACE3,"No such edifact message type: %d", type);
        return "";
    }
}

int GetEdiMsgTypeByType(edi_msg_types_t type, edi_mes_head *pHead)
{
    return GetEdiMsgTypeByType_(GetEdiTemplateMessages(), type, pHead);
}

int GetEdiMsgTypeByType_(EDI_MESSAGES_STRUCT *pMesTemp,
                         edi_msg_types_t type, edi_mes_head *pHead)
{
    EDI_MSG_TYPE *pEType= GetEdiMsgTypeStrByType_(pMesTemp, type);
    if(pEType){
        pHead->msg_type     = type;
        pHead->msg_type_req = pEType->query_type;
        pHead->answer_type  = pEType->answer_type;
        pHead->msg_type_str = pEType;
        strcpy(pHead->code,   pEType->code);
        return EDI_MES_OK;
    } else {
        EdiError(EDILOG,"No such edifact type: %d", type);
        return EDI_MES_NOT_FND;
    }
}

int GetEdiMsgTypeByCode(const char *code, edi_mes_head *pHead)
{
    return GetEdiMsgTypeByCode_(GetEdiTemplateMessages(), code, pHead);
}

int GetEdiMsgTypeByCode_(EDI_MESSAGES_STRUCT *pMesTemp,
                         const char *code, edi_mes_head *pHead)
{
    EDI_MSG_TYPE *pEType= GetEdiMsgTypeStrByName_(pMesTemp, code);
    if(pEType){
        pHead->msg_type     = pEType->type;
        pHead->msg_type_req = pEType->query_type;
        pHead->answer_type  = pEType->answer_type;
        pHead->msg_type_str = pEType;
        return EDI_MES_OK;
    } else {
        EdiError(EDILOG,"No such edifact message: %s", code);
        return EDI_MES_NOT_FND;
    }
}


/*
 По имени сообщения достаем его структуру
 return NULL - not found
 return EDI_MSG_TYPE * - Ok
 */
EDI_MSG_TYPE *GetEdiMsgTypeStrByName(const char *mname)
{
    return GetEdiMsgTypeStrByName_(GetEdiTemplateMessages(),mname);
}

EDI_MSG_TYPE *GetEdiMsgTypeStrByName_(EDI_MESSAGES_STRUCT *pMesTemp,
                                      const char *mname)
{
    EDI_MSG_TYPE key1, *res;

    if(pMesTemp == NULL) {
        return NULL;
    }

    key1.code=mname;

    res = static_cast<EDI_MSG_TYPE *>(
            bsearch(&key1, pMesTemp->HBNTypes, (size_t)pMesTemp->tnum, sizeof(EDI_MSG_TYPE),
                    (int(*)(const void *, const void *))cmp_edi_names));

    return res;
}

/*
 По типу сообщения достаем его структуру
 return NULL - not found
 return EDI_MSG_TYPE * - Ok
 */
EDI_MSG_TYPE *GetEdiMsgTypeStrByType(edi_msg_types_t type)
{
    return GetEdiMsgTypeStrByType_(GetEdiTemplateMessages(), type);
}

EDI_MSG_TYPE *GetEdiMsgTypeStrByType_(EDI_MESSAGES_STRUCT *pMesTemp,
                                      edi_msg_types_t type)
{
    EDI_MSG_TYPE key1, *res;

    if(pMesTemp==NULL){
        EdiError(EDILOG,"Template is NULL");
        return NULL;
    }

    key1.type=type;

    res = static_cast<EDI_MSG_TYPE *>(
            bsearch(&key1,pMesTemp->HBTTypes, (size_t)pMesTemp->tnum, sizeof(EDI_MSG_TYPE),
                    (int(*)(const void *, const void *))cmp_edi_types));

    return res;
}



void SetEdiTempServiceFunc(fp_edi_after_parse_err f_ape,
                           fp_edi_before_proc     f_bp,
                           fp_edi_after_proc      f_ap,
                           fp_edi_after_proc_err  f_apre,
                           fp_edi_before_send 	  f_bs,
                           fp_edi_after_send	  f_as,
                           fp_edi_after_send_err  f_ase)
{
    SetEdiTempServiceFunc_(GetEdiTemplateMessages(), f_ape,f_bp,f_ap,f_apre,f_bs,f_as,f_ase);
}

void SetEdiTempServiceFunc_(EDI_MESSAGES_STRUCT *pMesTemp,
                            fp_edi_after_parse_err f_ape,
                            fp_edi_before_proc     f_bp,
                            fp_edi_after_proc      f_ap,
                            fp_edi_after_proc_err  f_apre,
                            fp_edi_before_send 	  f_bs,
                            fp_edi_after_send	  f_as,
                            fp_edi_after_send_err  f_ase)
{
    pMesTemp->f_after_parse_err=f_ape;
    pMesTemp->f_before_proc=f_bp;
    pMesTemp->f_after_proc=f_ap;
    pMesTemp->f_after_proc_err=f_apre;
    pMesTemp->f_before_send=f_bs;
    pMesTemp->f_after_send=f_as;
    pMesTemp->f_after_send_err=f_ase;
}


/*
 Отправка сообщения заданного типа
*/
int SendEdiMessage(edi_msg_types_t type, edi_mes_head *pHead, void *udata, void *data, int *err)
{
    return SendEdiMessage_(GetEdiTemplateMessages(), GetEdiMesStruct(),
                           type, pHead, udata, data, err);
}

int SendEdiMessage_(EDI_MESSAGES_STRUCT *pMesTemp, EDI_REAL_MES_STRUCT *pMesStr,
                    edi_msg_types_t type, edi_mes_head *pHead,
                    void *udata, void *data, int *err)
{
    EDI_MSG_TYPE *pEType= GetEdiMsgTypeStrByType_(pMesTemp, type);

    if(pEType==NULL){
        SetEdiErrNum_(pMesStr, EDI_NO_TYPES);
        EdiError(EDILOG,"No types defined for type %d", type);
        return EDI_MES_TYPES_ERR;
    }

    return SendEdiMessage__(pMesTemp,pMesStr,pEType,pHead,udata,data,err);
}

int SendEdiMessage__(EDI_MESSAGES_STRUCT *pMesTemp, EDI_REAL_MES_STRUCT *pMesStr,
                     EDI_MSG_TYPE *pEType, edi_mes_head *pHead,
                     void *udata, void *data, int *err)
{
    const char *str_type;
    int ret;

    if(pEType->f_send == NULL){
        SetEdiErrNum_(pMesStr, EDI_NO_MES_SEND);
        EdiError(EDILOG,"Send func is not defined");
        return EDI_MES_TYPES_ERR;
    }

    str_type=(pEType->query_type == QUERY)?"Request":"Response";
    pHead->msg_type_req = pEType->query_type;
    pHead->answer_type  = pEType->answer_type;
    pHead->msg_type     = pEType->type;
    pHead->msg_type_str = pEType;
    strcpy(pHead->code,   pEType->code);

    if(pMesTemp->f_before_send){
        EdiTrace(TRACE3,"Call before_send function ...");
        ret = pMesTemp->f_before_send(pHead,udata,err);
        if(ret){
            return ret;
        }
    }

    EdiTrace(TRACE2,"Call send %s function for %s...",
             str_type, pEType->code);

    ret = pEType->f_send(pHead, udata, data, err);

    EdiTrace(TRACE2,"Send %s function for %s finished with retcode=%d, err=%d",
             str_type, pEType->code, ret, *err);

    if(ret){
        if(pMesTemp->f_after_send_err){
            EdiTrace(TRACE3,"Call after_send_err function ...");
            pMesTemp->f_after_send_err(pHead,ret,udata,err);
        }
        return ret;
    }

    if(pMesTemp->f_after_send){
        EdiTrace(TRACE2,"Call send_%s_after function for %s",
                 str_type, pEType->code);
        ret = pMesTemp->f_after_send(pHead, udata, err);
        EdiTrace(TRACE2,"Done with retcode=%d, err=%d",ret, *err);
    } else {
        EdiTrace(TRACE2, "There is no send_%s_after function for %s",
                 str_type, pEType->code);
    }

    return ret;
}

/*
 Обработка сообщения заданного типа
 */
int ProcEdiMessage(edi_msg_types_t type, edi_mes_head *pHead, void *udata, void *data, int *err)
{
    return ProcEdiMessage_(GetEdiTemplateMessages(), GetEdiMesStruct(),
                           type, pHead, udata, data, err);
}

int ProcEdiMessage_(EDI_MESSAGES_STRUCT *pMesTemp,EDI_REAL_MES_STRUCT *pMesStr,
                    edi_msg_types_t type, edi_mes_head *pHead,
                    void *udata, void *data, int *err)
{
    EDI_MSG_TYPE *pEType = GetEdiMsgTypeStrByType_(pMesTemp, type);

    if(pEType==NULL){
        SetEdiErrNum_(pMesStr, EDI_NO_TYPES);
        EdiError(EDILOG,"No types defined for the message");
        return EDI_MES_TYPES_ERR;
    }

    return ProcEdiMessage__(pMesTemp,pMesStr,pEType,pHead,udata,data,err);
}

int ProcEdiMessage__(EDI_MESSAGES_STRUCT *pMesTemp,EDI_REAL_MES_STRUCT *pMesStr,
                     EDI_MSG_TYPE *pEType, edi_mes_head *pHead,
                     void *udata, void *data, int *err)
{
    const char *str_type;
    int ret;

    if(pEType->f_proc==NULL){
        SetEdiErrNum_(pMesStr, EDI_NO_MES_PROC);
        EdiError(EDILOG,"Proc func is not defined");
        return EDI_MES_TYPES_ERR;
    }

    str_type=(pEType->query_type == QUERY)?"Request":"Response";

    if(pMesTemp->f_before_proc){
        EdiTrace(TRACE3,"Call proc_before function ...");
        ret = pMesTemp->f_before_proc(pHead,udata,err);
        EdiTrace(TRACE3,"Done with ret_code=%d, *err=%d", ret, *err);
        if(ret){
            return ret;
        }
    }

    EdiTrace(TRACE3,"Call proc %s function for %s...", str_type, pEType->code);
    ret = pEType->f_proc(pHead,udata,data,err);
    EdiTrace(TRACE3,"Done with ret_code=%d, *err=%d", ret, *err);

    if(ret){
        if(pMesTemp->f_after_proc_err){
            EdiTrace(TRACE3,"Call after_proc_%s_err function ...", str_type);
            pMesTemp->f_after_proc_err(pHead,ret,udata,err);
        } else {
            EdiTrace(TRACE2, "There is no proc_%s_after_err function for %s",
                     str_type, pEType->code);
        }
        return ret;
    } else {
        if(pMesTemp->f_after_proc){
            EdiTrace(TRACE3,"Call after_proc_%s function ...", str_type);
            ret = pMesTemp->f_after_proc(pHead,udata,err);
            EdiTrace(TRACE3,"Done with ret_code=%d, *err=%d", ret, *err);
            if(ret){
                return ret;
            }
        } else {
            EdiTrace(TRACE2, "There is no proc_%s_after function for %s",
                     str_type, pEType->code);
        }
    }

    return EDI_MES_OK;
}


int FullObrEdiMessage(const char *edi_tlg_text, edi_mes_head *pHead,
                      void *udata, int *err)
{
    if(!GetEdiMesStruct() && CreateEdiLocalMesStruct()){
        tst();
        return EDI_MES_ERR;
    }

    return FullObrEdiMessage_(GetEdiTemplateMessages(),GetEdiMesStruct(),
                              edi_tlg_text, pHead, udata, err);
}

int FullObrEdiMessage_(EDI_MESSAGES_STRUCT *pMesTemp,EDI_REAL_MES_STRUCT *pMesStr,
                       const char *edi_tlg_text, edi_mes_head *pHead,
                       void *udata, int *err)
{
    EDI_MSG_TYPE *pEType;
    int ret=ReadEdiMessage_(pMesTemp, pMesStr, edi_tlg_text);
    void *data=NULL;

    if(ret){
        if(ret == EDI_MES_STRUCT_ERR){ /*Ошибка синтаксиса*/
            const char *err_str = EdiErrGetString_(pMesStr);
            EdiError(EDILOG,"%s",err_str?err_str:"ReadEdiMessage: syntax error!");
            /*err_msg_error(EDILOG,*err=EDI_SYNTAX_ERR,1,1);*/
        }else {
            EdiError(EDILOG,"ReadEdiMessage failed whith code = %d", ret);
        }
        if(pMesTemp->f_after_parse_err){
            EdiTrace(TRACE3,"Call after_parse_err function ...");
            return pMesTemp->f_after_parse_err(ret,udata,err);
        }else {
            return ret;
        }
    }

    pEType = GetEdiMsgTypeStrByName_(pMesTemp, GetEdiMes_(pMesStr));


    if(SetMesHead_(pHead, pMesStr,1)){
        EdiError(EDILOG,"GetMesAttr failed");
        return EDI_MES_ERR;
    }

    if(pEType->str_sz){
        EdiTrace(TRACE3, "Calloc %zd bytes for message struct ...",
                 pEType->str_sz);
        data= edi_calloc(1,pEType->str_sz);
        if(data==NULL){
            EdiError(EDILOG,"Can't allocate %zd bytes", pEType->str_sz);
            return EDI_MES_ERR;
        }
    }

    ret = ProcEdiMessage__(pMesTemp,pMesStr,pEType,pHead,udata,data,err);
    if(ret){
        if(data){
            EdiTrace(TRACE3,"Freeing message struct ...");
            if(pEType->destruct){
                pEType->destruct(data);
            }
            edi_free(data);
        }
        return ret;
    }

    if(pEType->query_type==QUERY){
        if(pEType->answer_type==EDINOMES){
            if(data){
                EdiTrace(TRACE3,"Freeing message struct ...");
                if(pEType->destruct){
                    pEType->destruct(data);
                }
                edi_free(data);
            }

            SetEdiErrNum_(pMesStr, EDI_NO_ANSWER);
            EdiError(EDILOG,"Message %s is a QUERY and no answer type specified", GetEdiMes_(pMesStr));
            return EDI_MES_TYPES_ERR;
        }
        ret = SendEdiMessage_(pMesTemp,pMesStr,
                              pEType->answer_type,pHead,udata,data,err);
    }

    if(data){
        EdiTrace(TRACE3,"Freeing message struct ...");
        if(pEType->destruct){
            pEType->destruct(data);
        }
        edi_free(data);
    }

    return ret;
}

int cmp_edi_types(const EDI_MSG_TYPE *t1, const EDI_MSG_TYPE *t2)
{
    return (t1->type - t2->type);
}

int cmp_edi_names(const EDI_MSG_TYPE *t1, const EDI_MSG_TYPE *t2)
{
    return strcmp(t1->code, t2->code);
}
