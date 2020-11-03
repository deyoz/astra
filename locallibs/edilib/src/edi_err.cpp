#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <stdlib.h>
#include <string.h>

#define NICKNAME "ROMAN"
#include "edi_test.h"
#include "edi_tables.h"
#include "edi_func.h"
#include "edi_user_func.h"
#include "edi_err_msg.h"

static char err_str[200]="";

const char *EdiErrGetString()
{
    return EdiErrGetString_(GetEdiMesStruct());
}

const char *EdiErrGetString_(EDI_REAL_MES_STRUCT *pMes)
{
    const char *e;
    int len=0;

    if(pMes->ErrInfo.ErrNum == 0){
        return NULL;
    }
    if(*err_str){
        return err_str;
    }

    e = get_edi_msg_by_num(pMes->ErrInfo.ErrNum, pMes->ErrInfo.ErrLang);
    if(!e){
        WriteEdiLog(EDILOG,"No msg, code = %d", pMes->ErrInfo.ErrNum);
	return NULL;
    }

    if(*GetEdiMes_(pMes)){
	len += sprintf(err_str+len,"%s: ", GetEdiMes_(pMes));
    }

    if(pMes->ErrInfo.SegGr){
	len += sprintf(err_str+len,"%d", pMes->ErrInfo.SegGr);
	if(pMes->ErrInfo.SegGr_num>-1){
	    len += sprintf(err_str+len,":%d", pMes->ErrInfo.SegGr_num);
	}
        len += sprintf(err_str+len,",");
    }

    if(pMes->ErrInfo.Segment[0]!='\0'){
	len += sprintf(err_str + len, "%s", pMes->ErrInfo.Segment);
	if(pMes->ErrInfo.Segm_num>-1){
            len += sprintf(err_str+len,":%d", pMes->ErrInfo.Segm_num);
	}
        len += sprintf(err_str+len,",");
    }

    if(pMes->ErrInfo.Composite[0]!='\0'){
        len += sprintf(err_str + len, "%s", pMes->ErrInfo.Composite);
	if(pMes->ErrInfo.Comp_num>-1){
            len += sprintf(err_str+len,":%d", pMes->ErrInfo.Comp_num);
	}
        len += sprintf(err_str+len,",");
    }

    if(pMes->ErrInfo.DataElem){
        len += sprintf(err_str + len, "%d", pMes->ErrInfo.DataElem);
	if(pMes->ErrInfo.DataElemNum>-1){
            len += sprintf(err_str+len,":%d", pMes->ErrInfo.DataElemNum);
	}
        len += sprintf(err_str+len,",");
    }

    if(len){
	err_str[len-1]=':';
    }

    sprintf(err_str+len, "%s", e);
    return err_str;
}

void ResetEdiErr(void)
{
    ResetEdiErr_(GetEdiMesStruct());
}

void ResetEdiErr_(EDI_REAL_MES_STRUCT *pMes)
{
    memset(&pMes->ErrInfo, 0, sizeof(pMes->ErrInfo));
    *err_str = 0;
    EdiTrace(TRACE3,"Reset edi error info ok!");
}

void SetEdiErrDataElemArr(int DataElem, int num)
{
    SetEdiErrDataElem_(GetEdiMesStruct(), DataElem, num);
}

void SetEdiErrDataElem(int DataElem)
{
    SetEdiErrDataElem_(GetEdiMesStruct(), DataElem, -1);
}

void SetEdiErrDataElem_(EDI_REAL_MES_STRUCT *pMes, int DataElem, int num)
{
    pMes->ErrInfo.DataElem = DataElem;
    pMes->ErrInfo.DataElemNum = num;
}


void SetEdiErrCompArr(const char *Comp, int num)
{
    SetEdiErrCompArr_(GetEdiMesStruct(), Comp, num);
}

void SetEdiErrCompArr_(EDI_REAL_MES_STRUCT *pMes, const char *Comp, int num)
{
    SetEdiErrComp__(pMes, Comp, num);
}

void SetEdiErrComp(const char *Comp)
{
    SetEdiErrComp_(GetEdiMesStruct(), Comp);
}

void SetEdiErrComp_(EDI_REAL_MES_STRUCT *pMes, const char *Comp)
{
    SetEdiErrComp__(pMes, Comp, -1);
}

void SetEdiErrComp__(EDI_REAL_MES_STRUCT *pMes, const char *Comp, int num)
{
    memcpy(pMes->ErrInfo.Composite, Comp, COMP_LEN);
    pMes->ErrInfo.Composite[COMP_LEN] = '\0';
    pMes->ErrInfo.Comp_num = num;
}

void SetEdiErrSegmArr(const char *Segm, int num)
{
    SetEdiErrSegmArr_(GetEdiMesStruct(), Segm, num);
}

void SetEdiErrSegmArr_(EDI_REAL_MES_STRUCT *pMes, const char *Segm, int num)
{
    SetEdiErrSegm__(pMes, Segm, num);
}

void SetEdiErrSegm(const char *Segm)
{
    SetEdiErrSegm_(GetEdiMesStruct(), Segm);
}

void SetEdiErrSegm_(EDI_REAL_MES_STRUCT *pMes, const char *Segm)
{
    SetEdiErrSegm__(pMes, Segm, -1);
}

void SetEdiErrSegm__(EDI_REAL_MES_STRUCT *pMes, const char *Segm, int num)
{
    memcpy(pMes->ErrInfo.Segment, Segm, TAG_LEN);
    pMes->ErrInfo.Segment[TAG_LEN] = '\0';
    pMes->ErrInfo.Segm_num = num;
}

void SetEdiErrSegGr(int SegGr, int num)
{
    SetEdiErrSegGr_(GetEdiMesStruct(), SegGr, num);
}

void SetEdiErrSegGr_(EDI_REAL_MES_STRUCT *pMes, int SegGr, int num)
{
    pMes->ErrInfo.SegGr = SegGr;
    pMes->ErrInfo.SegGr_num = num;
}

void SetEdiErrNum(unsigned num)
{
    SetEdiErrNum_(GetEdiMesStruct(), num);
}

void SetEdiErrNum_(EDI_REAL_MES_STRUCT *pMes, unsigned num)
{
    if(!pMes)
        return;
    pMes->ErrInfo.ErrNum = num;
    pMes->ErrInfo.ErrLang = EDI_ENGLISH;
}

int isEdiError(void)
{
    return isEdiError_(GetEdiMesStruct());
}

int isEdiError_(EDI_REAL_MES_STRUCT *pMes)
{
    return (pMes->ErrInfo.ErrNum)?1:0;
}

int GetEdiErrNum(void)
{
    return GetEdiErrNum_(GetEdiMesStruct());
}

int GetEdiErrNum_(EDI_REAL_MES_STRUCT *pMes)
{
    return pMes->ErrInfo.ErrNum;
}

void SetEdiErrLang(int lang)
{
    SetEdiErrLang_(GetEdiMesStruct(),lang);
}
void SetEdiErrLang_(EDI_REAL_MES_STRUCT *pMes, int lang)
{
    if(!pMes)
	return;
    pMes->ErrInfo.ErrLang = lang;
}

/* Counter functions */
void IncEdiSegment(EDI_REAL_MES_STRUCT *pMes)
{
    pMes->ErrInfo.SegmCount ++;
    pMes->ErrInfo.CompCount = 0;
    pMes->ErrInfo.DataCount = 0;
}

void IncEdiComposite(EDI_REAL_MES_STRUCT *pMes)
{
    pMes->ErrInfo.CompCount ++;
    pMes->ErrInfo.DataCount = 0;
}

void IncEdiDataElem(EDI_REAL_MES_STRUCT *pMes)
{
    pMes->ErrInfo.DataCount ++;
}

const char *EdiErrGetSegName(EDI_REAL_MES_STRUCT *pMes)
{
    return pMes->ErrInfo.Segment;
}

int EdiErrGetSegNum(void)
{
    return EdiErrGetSegNum_(GetEdiMesStruct());
}

int EdiErrGetSegNum_(EDI_REAL_MES_STRUCT *pMes)
{
    return pMes->ErrInfo.SegmCount;
}

int EdiErrGetCompNum(void)
{
    return EdiErrGetCompNum_(GetEdiMesStruct());
}

int EdiErrGetCompNum_(EDI_REAL_MES_STRUCT *pMes)
{
    return pMes->ErrInfo.CompCount;
}

int EdiErrGetDataNum(void)
{
    return EdiErrGetDataNum_(GetEdiMesStruct());
}

int EdiErrGetDataNum_(EDI_REAL_MES_STRUCT *pMes)
{
    return pMes->ErrInfo.DataCount;
}

