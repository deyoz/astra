#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <sys/times.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#define  NICKNAME "ROMAN"
#include "edi_test.h"
#include "edi_tables.h"
#include "edi_all.h"
#include "edi_user_func.h"
#include "edi_func.h"
#include "edi_malloc.h"

static int edilib=0;
static EDI_REAL_MES_STRUCT *pMesGlobal=NULL;

int ReadEdiMessage(const char *Mes)
{
    EDI_REAL_MES_STRUCT *pCurMes=NULL;
    EDI_MESSAGES_STRUCT *pTempMes=NULL;

    if((pTempMes=GetEdiTemplateMessages())==NULL){
        EdiError(EDILOG,"GetEdiTemplateMessages()==NULL");
        return EDI_MES_ERR;
    }

    if(CreateEdiLocalMesStruct() || (pCurMes=GetEdiMesStruct())==NULL){
        EdiError(EDILOG,"Error in CreateEdiLocalMesStruct()");
        return EDI_MES_ERR;
    }

    return ReadEdiMessage_(pTempMes, pCurMes, Mes);
}

int CreateEdiLocalMesStruct(void)
{
    return CreateEdiMesStruct(&pMesGlobal);
}

/**
 * �� ���ᨨ SITA
IATA:
A to Z  Letters, upper case
0 to 9  Numerals
Space character
.   Full stop
,   Comma
-   Hyphen/minus sign
(   Opening Parenthesis
)   Closing parenthesis
/   Oblique stroke (slash)
=   Equals sign
'   Apostrophe  Used as segment terminator (X'7D')
+   Plus sign   Used as segment tag and data element separator (X'4E')
:   Colon   Used as component data element separator (X'7A')
?   Question mark   Used as a release character

IATB:
A to Z  Letters, upper case
a to z  Letters, lower case
0 to 9  Numerals
Space character
.   Full stop
,   Comma
-   Hyphen/minus sign
(   Opening Parenthesis
)   Closing parenthesis
/   Oblique stroke (slash)
'   Apostrophe
+   Plus sign
:   Colon
=   Equal sign
?   Question mark
%   Percentage sign
*   Asterisk
;   Semi-colon
<   Less than sign
>   Greater than sign
*/


static edi_loaded_char_sets edi_chrset[]=
{
    {"IATA", "\x3A\x2B,\x3F \x27" /* :+,? ' */},
    {"IATB", "\x1F\x1D,\x3F\x1D\x1C" /*��࣠ �����-�!*/},
    {"SIRE", "\x3A\x2B,\x3F \"\n"},
    {"UNOA", "\x3A\x2B.\x3F '" /* :+.? ' */}
};


int CreateEdiMesStruct(EDI_REAL_MES_STRUCT **pMes)
{
    int err=0;

    if(*pMes) {
        DeleteRealMes(*pMes);
        return 0;
    }

    if( (*pMes = (EDI_REAL_MES_STRUCT *)
         calloc(1,sizeof(EDI_REAL_MES_STRUCT))) == NULL)
    {
        EdiError(EDILOG,"Can't allocate %zd bytes\n",sizeof(EDI_REAL_MES_STRUCT));
        err=-1;
    }

    // load defaults
    if(InitEdiCharSet(edi_chrset, sizeof(edi_chrset)/sizeof(edi_chrset[0]))){
        EdiError(EDILOG,"InitEdiCharSet() failed");
        return -3;
    }


    if(err)
        EdiError(EDILOG,"Create edifact message struct - ERROR");
    else
        EdiTrace(TRACE2,"Create edifact message struct - Ok");
    return err;
}

int IsEdilib(void)
{
    EdiTrace(TRACE3,"edilib=%d",edilib);
    return edilib;
}
int SetEdilib(int Set)
{
    edilib=Set;
    EdiTrace(TRACE3,"edilib set to %d",edilib);
    return 1;
}

/*****************************************************************/
/*������ ��஥ ᮮ�饭��, ������ �⥪, ���뢠�� ����� ᮮ��.*/
/*input : *Mes - ᮮ�饭�� � �ଠ� EDIFACT                     */
/*output: *pMes - ��⠭��� � �������� ������ ᮮ�饭��         */
/*return EDI_MES_ERR        - �ணࠬ��� �訡��!!!               */
/*return EDI_MES_STRUCT_ERR - �訡�� ᨭ⠪��!!!               */
/*return EDI_MES_NOT_FND    - �������⭮� ᮮ�饭��              */
/*return EDI_MES_OK         - �� Ok!                            */
/*****************************************************************/
int ReadEdiMessage_(EDI_MESSAGES_STRUCT *pTempMes,
                    EDI_REAL_MES_STRUCT *pstMes, const char *Mes)
{
    int Err;
    struct  tms stm;
    clock_t tm1=times(&stm);
    edilib = 0;

    if(pstMes->Tag != NULL) { /*�᫨ ࠭�� 㦥 ࠧ��ࠫ� ᮮ�饭��*/
        DeleteRealMes(pstMes);  /*����塞 ࠭�� ��⠭��� ᮮ�饭��*/
    }

    Err = ReadEdiMes(Mes, pTempMes, pstMes);/*���뢠��� ᮮ�饭�� � �஢�મ� ᨭ⠪��*/
    switch(Err) {
    case EDI_MES_STRUCT_ERR:
	{
	    const char *err = EdiErrGetString();
	    WriteEdiLog(EDILOG,"Syntax error: %s", err?err:"No error message");
	    WriteEdiLog(EDILOG,"Segm %d, Comp %d, DataElem %d",
					EdiErrGetSegNum_(pstMes), EdiErrGetCompNum_(pstMes),
					EdiErrGetDataNum_(pstMes));
	}
	break;
    case EDI_MES_NOT_FND:
        EdiError(EDILOG,"Template for message not found ! \n");
        break;
    case EDI_MES_ERR:
        EdiError(EDILOG,"Program error ! \n");
        break;
    case EDI_MES_OK:
        EdiTrace(TRACE1,"Read message <%s> ok!",GetEdiMes_(pstMes));
        edilib=1;
        break;
    default:
        EdiError(EDILOG,"Unknown return from ReadEdiMes = %d", Err);
        Err = EDI_MES_ERR;
        break;
    }
    EdiTrace(TRACE1,"Err = %d, Execution time=%ldMs",
             Err, MSec(times(&stm)-tm1));
    return Err;
}

/*************************************************************/
/*�����頥� 㪠��⥫� �� �����䨪��� ᮮ�饭��.	     */
/*************************************************************/
const char *GetEdiMes_(EDI_REAL_MES_STRUCT *pMes)
{
    return pMes->Message;
}

const char *GetEdiMes(void)
{
    return GetEdiMes_(GetEdiMesStruct ());
}

edi_msg_types_t GetEdiMesType_(EDI_REAL_MES_STRUCT *pMes)
{
    return pMes->pTempMes->Type.type;
}

edi_msg_types_t GetEdiMesType(void)
{
    return GetEdiMesType_(GetEdiMesStruct());
}

EDI_REAL_MES_STRUCT *GetEdiMesStruct(void)
{
    return pMesGlobal;
}

/********************************************************************/
/*��⠭�������� ���. �����. �� 楫������ ᮮ�饭��(���㫥��� pPoint)*/
/*���樠������� pPoint                                             */
/*return -1 - �ணࠬ���� �訡��                                    */
/*return  0 - �� Ok                                                */
/********************************************************************/
int ResetEdiPoint_(EDI_REAL_MES_STRUCT *pM)
{
    if(pM->pPoint == NULL) {
        if((pM->pPoint = (Edi_Point *) calloc(1, sizeof( Edi_Point )))==NULL){
            EdiError(EDILOG,"Can't allocate %zd bytes\n", sizeof( Edi_Point ));
            return -1;
        }
        return 0;
    }
    memset(pM->pPoint, 0, sizeof(Edi_Point));
    //  EdiTrace(TRACE3,"Reset point ok!");
    return 0;
}
int ResetEdiPoint(void)
{
    return ResetEdiPoint_(GetEdiMesStruct());
}

/********************************************************************/
/*���࠭�� ���. �����. � �⥪                                      */
/*return  -1 - �ணࠬ���� �訡��                                    */
/*return   0 - �� Ok                                                */
/********************************************************************/
int PushEdiPoint_(EDI_REAL_MES_STRUCT *pM)
{
    char Str[30];
    Edi_Stack_Point *pStackTmp;

    if( (pM->pPoint == NULL) && (ResetEdiPoint_(pM) < 0) ) return 0;
    if((pStackTmp = (Edi_Stack_Point *)
        calloc(1, sizeof( Edi_Stack_Point ) ))==NULL){
        EdiError(EDILOG,"Can't allocate %zd bytes\n",sizeof( Edi_Stack_Point ));
        return -1;
    }
    memcpy(pStackTmp, pM->pPoint, sizeof(Edi_Point));
    if(pM->pStack == NULL){
        pM->pStack = pStackTmp;
    } else {
        pStackTmp->Next = pM->pStack;
        pM->pStack = pStackTmp;
    }
    PrintEdiPoint(pM,Str, sizeof(Str));
    //   EdiTrace(TRACE3,"Push point ok! It's at %s now.", Str);
    return 0;
}
int PushEdiPoint(void)
{
    return PushEdiPoint_(GetEdiMesStruct());
}

/********************************************************************/
/*��������� ���. �����. �� �⥪�                                    */
/*return -1 - ��祣� ���������                                      */
/*return  0 - �� Ok                                                */
/********************************************************************/
int PopEdiPoint_(EDI_REAL_MES_STRUCT *pM)
{
    Edi_Stack_Point *pStackTmp;
    char Str[30];

    if(pM->pStack == NULL)
        return -1;
    pStackTmp = pM->pStack;
    memcpy(pM->pPoint, pStackTmp, sizeof(Edi_Point));
    pM->pStack = pStackTmp->Next;
    free(pStackTmp);
    PrintEdiPoint(pM,Str, sizeof(Str));
    //   EdiTrace(TRACE3,"Pop point ok! It's at %s now.", Str);
    return 0;
}
int PopEdiPoint(void)
{
    return PopEdiPoint_(GetEdiMesStruct());
}

/********************************************************************/
/*��������� ���. �����. �� �⥪� �� 㤠��� �. �⥪�                */
/*return -1 - ��祣� ���������                                      */
/*return  0 - �� Ok                                                */
/********************************************************************/
int PopEdiPoint_wd_(EDI_REAL_MES_STRUCT *pM)
{
    char Str[30];
    if(pM->pStack == NULL)
        return -1;
    memcpy(pM->pPoint, pM->pStack, sizeof(Edi_Point));
    PrintEdiPoint(pM,Str,sizeof(Str));
    //     EdiTrace(TRACE3,"Pop point (wd) ok! It's at %s now.",Str);
    return 0;
}
int PopEdiPoint_wd(void)
{
    return PopEdiPoint_wd_(GetEdiMesStruct());
}

/********************************************************************/
/*������  �⥪                                                    */
/*return  0 - �� Ok                                                */
/********************************************************************/
int ResetStackPoint_(EDI_REAL_MES_STRUCT *pMes)
{
    Edi_Stack_Point *pStackTmp, *pStackDel;
    if(pMes->pStack == NULL)
        return 0;
    pStackTmp = pMes->pStack;
    pMes->pStack = NULL;
    while(pStackTmp != NULL){
        pStackDel = pStackTmp;
        pStackTmp = pStackDel->Next;
        free (pStackDel);
    }
    //  EdiTrace(TRACE3,"Reset stack point ok!");
    return 0;
}
int ResetStackPoint(void)
{
    return ResetStackPoint_(GetEdiMesStruct());
}

/********************************************************************/
/*�����頥� ⥪�饩 �஢��� ������ ��������                      */
/*return  0 - �� Ok                                                */
/*return -1 - �� Error                                             */
/********************************************************************/
#if 0
unsigned CurrEdiLevel(void)
{
    return CurrEdiLevel_(GetEdiMesStruct());
}

unsigned CurrEdiLevel_(EDI_REAL_MES_STRUCT *pMes)
{
    if(pMes->pPoint == NULL)
        return 0;
    if(pMes->pPoint->pGrStr != NULL){
        // ������⭠� ��㯯�
        //pMes->pPoint->pGrStr->ParTag;
    } else if(pMes->pPoint->pTag != NULL){

    } else if(pMes->pPoint->pComp != NULL){

    } else {
        return 0;
    }
}
#endif

/********************************************************************/
/*���樠������� pFind, ��� ������ , �᫨ ���樠������ 㦥 �뫠  */
/*return -1 - �ணࠬ���� �訡��                                    */
/*return  0 - �� Ok                                                */
/********************************************************************/
int ResetEdiFind(void)
{
    return ResetEdiFind_(GetEdiMesStruct());
}
int ResetEdiFind_(EDI_REAL_MES_STRUCT *pMes)
{
    Find_Data *pFind;
    int blok_s = sizeof(void *);

    if (pMes->pFind == NULL) {
        if((pMes->pFind = (Find_Data *) calloc(1, sizeof( Find_Data ) ))==NULL){
            EdiError(EDILOG,"Can't allocate %zd bytes\n",sizeof( Find_Data ));
            return -1;
        }
        pFind = pMes->pFind;
        if((pFind->pData = (char *) calloc(1, blok_s * MAX_FIND )) == NULL){
            EdiError(EDILOG,"Can't allocate %d bytes\n",blok_s*MAX_FIND);
            return -1;
        }
        pFind->MaxNum   = MAX_FIND;
    }
    else{
        pFind = pMes->pFind;
        if(pFind->MaxNum > MAX_FIND)/*���� ���᪮� ��諨 ������ ����� 祬 ��� �।��*/
        {
            auto pData = pFind->pData;
            if((pFind->pData = (char *)
                realloc(pFind->pData,blok_s*MAX_FIND))==NULL){
                EdiError(EDILOG,"Can't change memory size to %d bytes\n",blok_s*MAX_FIND);
                pFind->pData = pData;
                return -1;
            }
        }
        memset(pFind->pData, 0, blok_s * MAX_FIND);
        memset(pFind->Data , 0, edi_max);
        pFind->NumData  = 0;
        pFind->FindFlag = 0;
        pFind->MaxNum   = MAX_FIND;
        pFind->pPoint   = 0;
    }
    return 0;
}

/********************************************************************/
/*���࠭�� �������� ��᫥���� ���᪮� ����� ��� �����ᮬ,        */
/*㪠����� � ����⢥ �室���� ��ࠬ��� (ind < MAX_SAVE_FOUND)    */
/*return  -1  - ��㤠�                                              */
/*return   0  - Ok                                                   */
/********************************************************************/
int SaveEdiFound(int ind)
{
    return SaveEdiFound_(GetEdiMesStruct(), ind);
}
int SaveEdiFound_(EDI_REAL_MES_STRUCT *pMes, int ind)
{
    edi_save_found *save;
    char *pData;
    if((ind >= MAX_SAVE_FOUND) || (ind < 0)) {
        EdiError(EDILOG,"Bad index (%d max: %d)",ind,MAX_SAVE_FOUND);
        return -1;
    }

    if(pMes->pFound[ind]!=NULL)
        FreeEdiFound_(pMes, ind);/*�� ����� ���. ��-� ���९����*/

    if((save = (edi_save_found *)
        malloc( sizeof(edi_save_found) )) == NULL){
        EdiError(EDILOG,"Can't allocate %zd bytes\n",sizeof(edi_save_found));
        return -1;
    }

    if((save->pPoint = (Edi_Point *)
        malloc( sizeof(Edi_Point) )) == NULL){
        EdiError(EDILOG,"Can't allocate %zd bytes\n",sizeof(Edi_Point));
        return -1;
    }
    memcpy (save->pPoint,pMes->pPoint,sizeof(Edi_Point));

    if((save->pFind = (Find_Data *)
        malloc( sizeof(Find_Data) )) == NULL){
        EdiError(EDILOG,"Can't allocate %zd bytes\n",sizeof(Find_Data));
        return -1;
    }
    memcpy (save->pFind,pMes->pFind,sizeof(Find_Data));

    unsigned int size_d = pMes->pFind->MaxNum*sizeof(void *);
    if((pData = (char *)
        malloc( size_d )) == NULL){
        EdiError(EDILOG,"Can't allocate %u bytes\n",size_d);
        return -1;
    }
    memcpy(pData,pMes->pFind->pData,size_d);
    save->pFind->pData = pData;
    pMes->pFound[ind] = save;
    //   EdiTrace(TRACE3,"Save found edi data ok!");
    return 0;
}

/********************************************************************/
/*����⠭�������� ��࠭���� �����.                               */
/*��������: ���뢠���� ���. ��������,                             */
/*��� ���ன �� ����� ��࠭﫨��                                */
/*return -1   - ��㤠�                                             */
/*return  0   - �� Ok                                              */
/********************************************************************/
int LoadEdiFound(int ind)
{
    return LoadEdiFound_(GetEdiMesStruct(), ind);
}
int LoadEdiFound_(EDI_REAL_MES_STRUCT *pMes, int ind)
{
    if( (ind >= MAX_SAVE_FOUND) || (pMes->pFound[ind]==NULL) ) {
        EdiError(EDILOG,"Bad index %d",ind);
        return -1;
    }
    auto save = pMes->pFound[ind];
    /*�஢��塞, ��� �⮩ �� ���. �����. �� ��࠭﫨��*/
    if( memcmp(save->pPoint,pMes->pPoint,sizeof(Edi_Point)) ){
        EdiError(EDILOG,"Areas of visibility do not coincide");
        return -1;
    }

    /*����㦠�� ��࠭�����*/
    unsigned int size_d = save->pFind->MaxNum*sizeof(void *);
    char *pData;
    if((pData = (char *)malloc( size_d )) == NULL){
        EdiError(EDILOG,"Can't allocate %u bytes\n",size_d);
        return -1;
    }
    memcpy(pData,save->pFind->pData,size_d);
    memcpy(pMes->pFind,save->pFind,sizeof(Find_Data));
    /*㤠�塞 ����� ���. ��������*/
    free(pMes->pFind->pData);
    pMes->pFind->pData = pData;
    //   EdiTrace(TRACE3,"Load edi found data ok!");
    return 0;
}

/********************************************************************/
/*������ ��࠭���� � SaveEdiFound �����                         */
/********************************************************************/
void FreeEdiFound_(EDI_REAL_MES_STRUCT *pM, int ind)
{
    edi_save_found *save;

    if( (ind >= MAX_SAVE_FOUND) || (pM->pFound[ind]==NULL) ) {
        EdiError(EDILOG,"Bad index %d",ind);
        return;
    }
    save = pM->pFound[ind];
    free(save->pPoint);
    free(save->pFind->pData);
    free(save->pFind);
    free(save);
    pM->pFound[ind] = NULL;
}
void FreeEdiFound(int  ind)
{
    FreeEdiFound_(GetEdiMesStruct(), ind);
}


/********************************************************************/
/*������ �� ��࠭���� � SaveEdiFound �����                     */
/********************************************************************/
void ResetEdiFound_(EDI_REAL_MES_STRUCT  *pM)
{
    if(pM->pFound == NULL)
        return;

    int i;
    for(i = 0; i < MAX_SAVE_FOUND; i++)
        if(pM->pFound[i]!=NULL)
            FreeEdiFound_(pM, i);
}
void ResetEdiFound(void)
{
    return ResetEdiFound_(GetEdiMesStruct());
}

/************************************************************/
/*��室�� ����� ������ �� ��� ������� �����               */
/*Num - �᫨ ���� �ᯥ�� - ���-�� ��������� �. ������   */
/*�ணࠬ���� �訡�� ��� ���������筮���:		    */
/*if flag return  NULL					    */
/*     else return  '/0'				    */
/*return  char * - �����⥫� �� �. ������ � ������� ������*/
/************************************************************/
const char *GetDBFN(short SegGr , int SegGrNum,
                    const char *Segm, int SegmNum,
                    const char *Comp, int CompNum,
                    int Data  , int DataNum)
{
    return GetDBFN_(GetEdiMesStruct(), SegGr, SegGrNum,
                    Segm, SegmNum,
                    Comp, CompNum,
                    Data, DataNum);
}

const char *GetDBFN_(EDI_REAL_MES_STRUCT *pMes,
                     short SegGr , int SegGrNum,
                     const char *Segm, int SegmNum,
                     const char *Comp, int CompNum,
                     int Data  , int DataNum)
{
    return GetDBFN__(pMes, SegGr, SegGrNum,
                     Segm, SegmNum,
                     Comp, CompNum,
                     Data, DataNum, -1, 1);
}

const char *GetDBFNSeq_(EDI_REAL_MES_STRUCT *pMes,
                        short SegGr , int SegGrNum,
                        const char *Segm, int SegmNum,
                        const char *Comp, int CompNum,
                        int Data  , int DataNum, int NumSeq)
{
    return GetDBFN__(pMes, SegGr, SegGrNum,
                     Segm, SegmNum,
                     Comp, CompNum,
                     Data, DataNum, NumSeq, 1);
}

const char *GetDBFNz(short SegGr , int SegGrNum,
                     const char *Segm, int SegmNum,
                     const char *Comp, int CompNum,
                     int Data  , int DataNum )
{
    return GetDBFNz_(GetEdiMesStruct(), SegGr, SegGrNum,
                     Segm, SegmNum,
                     Comp, CompNum,
                     Data, DataNum);
}

const char *GetDBFNz_(EDI_REAL_MES_STRUCT *pMes,
                      short SegGr , int SegGrNum,
                      const char *Segm, int SegmNum,
                      const char *Comp, int CompNum,
                      int Data  , int DataNum)
{
    return GetDBFN__(pMes, SegGr, SegGrNum,
                     Segm, SegmNum,
                     Comp, CompNum,
                     Data, DataNum, -1, 0);
}

const char *GetDBFN__(EDI_REAL_MES_STRUCT *pMes,
                      short SegGr , int SegGrNum,
                      const char *Segm, int SegmNum,
                      const char *Comp, int CompNum,
                      int Data  , int DataNum , int NumSeq,
                      int flag)
{
    static const char NullStr[10]="";
    int Ret/*, Num*/;
    const char *Data_res;

    EdiTrace(TRACE3,"Getting data by full name :\n"
             "\t SegGr=<%d>, SegGrNum=<%d>\n"
             "\t Segm =<%s>, SegmNum =<%d>\n"
             "\t Comp =<%s>, CompNum =<%d>\n"
             "\t Data =<%d>, DataNum =<%d>, DataSequence =<%d>\n"
             "\t flag=%d",
             SegGr,SegGrNum,Segm?Segm:"Null",SegmNum,
             Comp?Comp:"Null",CompNum,Data,DataNum,NumSeq,flag);


    if(!Data) {
        EdiTrace(TRACE1,"No data cpecify! return ...");
        return (flag)?NULL:NullStr;
    }

    PushEdiPoint_(pMes); /*���࠭塞 ⥪��. ��� ��������*/

    if(SegGr && (Ret = SetEdiPointToSegGr_(pMes, SegGr,SegGrNum))<=0){
        EdiTrace(TRACE3,"SetEdiPointToSegGr(%d,%d) returned %d",SegGr,SegGrNum,Ret);
        PopEdiPoint_(pMes);
        return (flag)?NULL:NullStr;
    }

    if(Segm && *Segm && (Ret = SetEdiPointToSegment_(pMes, Segm,SegmNum))<=0){
        EdiTrace(TRACE3,"SetEdiPointToSegment('%s',%d) return %d",Segm,SegmNum,Ret);
        PopEdiPoint_(pMes);
        return (flag)?NULL:NullStr;
    }

    if(Comp && *Comp && (Ret = SetEdiPointToComposite_(pMes, Comp,CompNum))<=0){
        EdiTrace(TRACE3,"SetEdiPointToComposite('%s',%d) return %d",Comp,CompNum,Ret);
        PopEdiPoint_(pMes);
        return (flag)?NULL:NullStr;
    }

    /*   if ((Ret = GetDataByName(Data,&Num))<=0){
   	EdiTrace(TRACE3,"GetDataByName(%d,%d) return %d",Data,Num,Ret);
   	PopEdiPoint();
   	return (flag)?NULL:NullStr;
   }
*/
    if((Data_res=GetDBNum__(pMes, Data,DataNum, NumSeq, 0))==NULL){
        EdiTrace(TRACE3,"GetDBNum(%d,%d) return NULL",Data,DataNum);
        PopEdiPoint_(pMes);
        return (flag)?NULL:NullStr;
    }

#ifdef _DEBUG_DATA_
    EdiTrace(TRACE3,"Data for return <%s>",Data_res?Data_res:"NULL");
#endif /*_DEBUG_DATA_*/

    PopEdiPoint_(pMes); /*��������� ���. �����. �� �⥪�.*/

    return (Data_res)?Data_res:((flag)?NULL:NullStr);
}


/************************************************************/
/*��室�� ����� ������ �� ��� �����                       */
/*Num - �᫨ ���� �ᯥ�� - ���-�� ��������� �. ������   */
/*return -1 - �ணࠬ���� �訡��                            */
/*return  0 - ��祣� �� ��諨 (Num = 0)                     */
/*return  1 - ��諨 Num �. ������ � ������� ������        */
/************************************************************/
int GetDataByName__(EDI_REAL_MES_STRUCT *pMes, int Data, int *Num, int NumSeq);
int GetDataByName(int Data, int *Num)
{
    return GetDataByName_(GetEdiMesStruct(), Data, Num);
}

int GetDataByName_(EDI_REAL_MES_STRUCT *pMes, int Data, int *Num)
{
    return GetDataByName__(pMes, Data, Num, -1);
}

int GetDataByNum_(EDI_REAL_MES_STRUCT *pMes, int Data, int Seq, int *Num)
{
    return GetDataByName__(pMes, Data, Num, Seq);
}

int GetDataByNum(int Data, int Seq, int *Num)
{
    return GetDataByName__(GetEdiMesStruct(), Data, Num, Seq);
}

int GetDataByName__(EDI_REAL_MES_STRUCT *pMes, int Data, int *Num, int NumSeq)
{
    int Ret;
    Edi_Point *pPoint;
    Find_Data *pFind;
    /* int blok_s = sizeof(void *);*/

    if (pMes->pPoint == NULL) {
        Ret = ResetEdiPoint_(pMes);
        if(Ret < 0) return -1;
    }

    Ret = ResetEdiFind_(pMes);
    if(Ret < 0) return -1;

    pFind = pMes->pFind;
    pFind->FindFlag = 1;       /* ���� �� �. ������ */
    pPoint = pMes->pPoint;
#ifdef _POINT_WARNING_
    pFind->pPoint = pPoint;
#endif /*_POINT_WARNING_*/
    memcpy (pFind->Data, &Data, sizeof(Data));
    pFind->NumSeq = NumSeq;
    if(pPoint->pTag != NULL){
        Ret = FindIn1Tag(pFind,pPoint->pTag);
    } else {
        if(pPoint->pGrStr != NULL){
            EdiError(EDILOG,"Trying to find data element '%04d' in the SegmentGroup!", Data);
            Ret = -1;
		} else {
			if(pPoint->pComp != NULL){
				Ret = FindIn1Comp(pFind,pPoint->pComp);
			} else {
                WriteEdiLog(EDILOG,"Trying to find data element '%04d' whithout Point specification!" , Data);
                Ret = -1;
            }
        }
    }
    if(!Ret) return -1;
    *Num = pFind->NumData;
    if(pFind->NumData == 0) return 0;
    return 1;
}

int FindIn1Tag(Find_Data *pFind, TAG_REAL_STRUCT *pTag)
{
    int Ret;

    Ret = FindInComp(pFind,pTag->Comp);
    if(!Ret)   return Ret;
    return 1;
}

int FindInComp(Find_Data *pFind, COMP_REAL_STRUCT *pComp)
{
    int Ret;

    while( pComp != NULL ){
        Ret=1;
        /*EdiTrace(TRACE3,"SingleData=%d", pComp->SingleData);*/
        if(pComp->SingleData)
            Ret = FindInData(pFind, pComp->DataElem);
        if(!Ret) return Ret;
        /*EdiTrace(TRACE3,"SingleData=%d", pComp->SingleData);*/
        if(pComp->NextArr != NULL && pComp->SingleData){
            Ret = FindInComp(pFind, pComp->NextArr);
            if(!Ret) return Ret;
        }
        pComp = pComp->Next;
    }
    return 1;
}

int FindIn1Comp(Find_Data *pFind, COMP_REAL_STRUCT *pComp)
{
    int Ret;

    Ret = FindInData(pFind, pComp->DataElem);
    if(!Ret) return Ret;
    return 1;
}

int FindInData(Find_Data *pFind, DATA_REAL_STRUCT *pData)
{
    DATA_REAL_STRUCT *pDataArr;
    int Ret;

    while( pData != NULL ){
        if( !memcmp(&(pData->DataElem), pFind->Data, sizeof(int)) &&
                (pFind->NumSeq == -1 || pData->TemplateData->Num == pFind->NumSeq)){
            Ret = SaveData(pFind,(void *)pData);
            if(!Ret) return Ret;
            if(pData->NextArr != NULL){
                pDataArr = pData->NextArr;
                while(pDataArr != NULL){
                    Ret = SaveData(pFind,(void *)pDataArr);
                    if(!Ret) return Ret;
                    pDataArr = pDataArr->NextArr;
                }
            }
        }
        pData = pData->Next;
    }
    return 1;
}
/*****************************************************/
/*���࠭�� �������� � ������ � pFind              */
/*return 1 - Ok                                      */
/*return 0 - error allocated memory                  */
/*****************************************************/
int SaveData(Find_Data *pFind, void *pData)
{
    int Ret;
    void  **pSaveData;

    if(pFind->NumData == pFind->MaxNum){ /*���⨣�� ���ᨬ㬠*/
        Ret = Resize_pFind(pFind);
        if(!Ret) return Ret;
    }
    pSaveData = (void **)
            (pFind->pData + (pFind->NumData)*sizeof(void *) );
    *pSaveData = pData;
    pFind->NumData ++;
    return 1;
}

int Resize_pFind(Find_Data *pFind)
{
    char *pData, *pDelData;
    unsigned int old_size, add_size;
    int blok_s;

    blok_s = sizeof( char *);
    old_size = (pFind->MaxNum)*blok_s;
    add_size = MAX_FIND*blok_s;
    pFind->MaxNum += MAX_FIND;

    if(( pData = (char *)calloc(1, old_size+add_size ))==NULL){
        EdiError(EDILOG,"Can't allocate %i bytes\n", old_size+add_size);
        return 0;
    }
    memcpy(pData,pFind->pData,old_size);
    pDelData = pFind->pData;
    pFind->pData = pData;
    free(pDelData);
    return 1;
}
/**********************************************************/
/*�����頥� 㪠��⥫� �� �������� ����� �� ������      */
/*return NULL - ��㤠�                                   */
/*return char* - 㪠��⥫� �� �����                      */
/**********************************************************/
const char *GetFoundDataByNum(int Num)
{
    return GetFoundDataByNum_(GetEdiMesStruct(), Num);
}
const char *GetFoundDataByNum_(EDI_REAL_MES_STRUCT *pMes, int Num)
{
    Find_Data *pFind;
    DATA_REAL_STRUCT  **pData;

    pFind = pMes->pFind;
    if(pFind->FindFlag==1){
        if((Num >= pFind->NumData) || (Num < 0)){
            EdiError(EDILOG,"Incorrect number of the data: %d, %d- maximum!\n",
                     Num,pFind->NumData-1);
            return NULL;
        }
        pData = (DATA_REAL_STRUCT **)(pFind->pData + (Num*sizeof(void *) ));
        return (*pData)->Data;
    } else {
        /*���� �ந�������� �� �� �. ������*/
        EdiError(EDILOG,"Find error \n");
        return NULL;
    }
}

/**********************************************************/
/*�ந������ ���� ����� ������ (�᫨ ��� �� �� �뫮) */
/*�����頥� 㪠��⥫� �� ����� ������ 		  */
/*��� 㪠��⥫� �� ������ ��ப� � ��砥 ���������筮�� */
/*Input : data - �. ������                               */
/**********************************************************/
const char *GetDBNz(int data)
{
    return GetDBNz_(GetEdiMesStruct(), data);
}

const char *GetDBNz_(EDI_REAL_MES_STRUCT *pMes, int data)
{
    return GetDBN__(pMes, data, -1, 1);
}

/**********************************************************/
/*�ந������ ���� ����� ������ (�᫨ ��� �� �� �뫮) */
/*�����頥� 㪠��⥫� �� ����� ������ 		  */
/*��� 㪠��⥫� �� NULL � ��砥 ���������筮��          */
/*Input : data - �. ������                               */
/**********************************************************/
const char *GetDBN(int data)
{
    return GetDBN_(GetEdiMesStruct(), data);
}

const char *GetDBN_(EDI_REAL_MES_STRUCT *pMes, int data)
{
    return GetDBN__(pMes, data, -1, 1);
}

const char *GetDBN__(EDI_REAL_MES_STRUCT *pMes, int data, int NumSeq, int zero)
{
    static const char null_str[10]="";
    Find_Data *pFind;
    DATA_REAL_STRUCT  **pData;
    int Ret, Num=0;

    if ((pMes->pFind == NULL) && ResetEdiFind_(pMes))
        return zero?null_str:NULL; /*�ணࠬ���� �訡��*/

    pFind = pMes->pFind;

    if( (pFind->FindFlag==1) && (!memcmp(pFind->Data,&data,sizeof(int))) ){
        /*���� �� ������� �. ������ 㦥 �஢������*/
        if(pFind->NumData==1){ /*�� ������ �ᥣ� ���� �. ���.*/
            pData = (DATA_REAL_STRUCT **)(pFind->pData);
#ifdef _DEBUG_DATA_
            EdiTrace(TRACE3,"data for return <%s>",(*pData)->Data);
#endif /* _DEBUG_DATA_ */
            return (*pData)->Data;
        } else {
            EdiTrace(TRACE3,"Found %d", pFind->NumData);
            return zero?null_str:NULL; /*���������筮���!*/
        }
    }
    else { /*���᪠ �� �� �뫮*/
        Ret = GetDataByName__(pMes, data,&Num, NumSeq);
        if ((Ret <= 0)||(Num>1)){
            if(Ret > 0){
                EdiTrace(TRACE3,"Found %d", pFind->NumData);
            }
            return zero?null_str:NULL;
        }
        /* ^1)��祣� �� ��諨 2) �訡�� 3)���������筮���*/
        pData = (DATA_REAL_STRUCT **)(pFind->pData);
#ifdef _DEBUG_DATA_
        EdiTrace(TRACE3,"data for return <%s>",(*pData)->Data);
#endif /*_DEBUG_DATA_*/
        return (*pData)->Data;
    }
}


/**********************************************************/
/*�ந������ ���� ����� ������ (�᫨ ��� �� �� �뫮) */
/*�����頥� 㪠��⥫� �� ����� ������ �� ����� � ������*/
/*��� 㪠��⥫� �� NULL � ��砥 ��㤠�                  */
/*Input : data - �. ������                               */
/**********************************************************/
const char *GetDBNum(int data, int Num_d)
{
    return GetDBNum_(GetEdiMesStruct(), data, Num_d);
}

/**********************************************************/
/*�ந������ ���� ����� ������ (�᫨ ��� �� �� �뫮) */
/*�����頥� 㪠��⥫� �� ����� ������ �� ����� � ������*/
/*��� 㪠��⥫� �� ������ ��ப� � ��砥 ��㤠�         */
/*Input : data - �. ������                               */
/**********************************************************/
const char *GetDBNumz(int data, int Num_d)
{
    return GetDBNumz_(GetEdiMesStruct(), data, Num_d);
}

const char *GetDBNumSeq_(EDI_REAL_MES_STRUCT *pMes, int data, int Num_d, int NumSeq)
{
    return GetDBNum__(pMes, data, Num_d, NumSeq, 0);
}

const char *GetDBNum_(EDI_REAL_MES_STRUCT *pMes, int data, int Num_d)
{
    return GetDBNum__(pMes, data, Num_d, -1, 0);
}

const char *GetDBNumz_(EDI_REAL_MES_STRUCT *pMes, int data, int Num_d)
{
    return GetDBNum__(pMes, data, Num_d, -1, 1);
}

const char *GetDBNum__(EDI_REAL_MES_STRUCT *pMes, int data, int Num_d, int NumSeq, int zero)
{
    static const char null_str[10]="";
    Find_Data *pFind;
    DATA_REAL_STRUCT  **pData;
    int Ret, Num=0;

    if ((pMes->pFind == NULL) && ResetEdiFind_(pMes))
        return zero?null_str:NULL; /*�ணࠬ���� �訡��*/

    pFind = pMes->pFind;

    if( (pFind->FindFlag==1) && (!memcmp(pFind->Data,&data,sizeof(int)) && pFind->NumSeq == NumSeq) ){
        /*���� �� ������� �. ������ 㦥 �஢������*/
        if(pFind->NumData>Num_d){
            pData = (DATA_REAL_STRUCT **)(pFind->pData + (Num_d*sizeof(void *) ));
#ifdef _DEBUG_DATA_
            EdiTrace(TRACE3,"data for return <%s>",(*pData)->Data);
#endif /*_DEBUG_DATA_*/
            return (*pData)->Data;
        } else
            return zero?null_str:NULL; /*��㤠�*/
    }
    else { /*���᪠ �� �� �뫮*/
        Ret = GetDataByName__(pMes, data,&Num, NumSeq);
        if ((Ret <= 0)||(Num<=Num_d))
            return zero?null_str:NULL;
        /* ^1)��祣� �� ��諨 2) �訡�� 3)��諨 ����� 祬 ��������*/
        pData = (DATA_REAL_STRUCT **)(pFind->pData + (Num_d*sizeof(void *) ));
#ifdef _DEBUG_DATA_
        EdiTrace(TRACE3,"data for return <%s>",(*pData)->Data);
#endif /* _DEBUG_DATA_ */
        return (*pData)->Data;
    }
}

/********************************************************************/
/*      a) �᫨ ���� �� ���������  ����.  㦥 �ந��������,        */
/*         ��࠭�稢��� �� ���. ��������                           */
/*         return 1 - Ok                                            */
/*      b) ���᪠ �� �� �뫮.                                      */
/*         �ந������ ����;                                        */
/*         �믮��塞 �㭪� �);                                      */
/* return -1 - �ணࠬ���� �訡��                                   */
/* return  0 - ���  ᥣ�. � ������� ���浪��� ����஬             */
/* return  1 - �� Ok                                               */
/********************************************************************/
int SetEdiPointToComposite(const char *Comp, int Num)
{
    return SetEdiPointToComposite_(GetEdiMesStruct(), Comp, Num);
}
int SetEdiPointToComposite_(EDI_REAL_MES_STRUCT *pMes, const char *Comp, int Num)
{
    Find_Data *pFind;
    Edi_Point *pPoint;
    int Ret;

    if(Num < 0){
        EdiTrace(TRACE3,"Can't get composite: Num=%d comp=%s",Num,Comp);
        return 0;
    }
    if( GetNumComposite_(pMes, Comp)==-1){
        EdiTrace(TRACE3,"Can't get composite <%s>",Comp);
        return -1;
    }

    pPoint = pMes->pPoint;
    pFind = pMes->pFind;

    if(!pFind->NumData){
        EdiTrace(TRACE2,">>>Composite <%s:%d> is not found",Comp,Num);
        return 0;
    }

    if ( Num > pFind->NumData-1 ) {
        EdiTrace(TRACE2,">>>Too big number for composite <%s:%d> %d max",Comp,Num,pFind->NumData);
        return 0;/*��� ����. � ������� ���. ����஬*/
    }
    else {
        Ret = ResetEdiPoint_(pMes);
        if(Ret < 0){
            EdiTrace(TRACE1,"Can't reset edi_point!");
            return -1;
        }
        if( (pPoint->pComp = (COMP_REAL_STRUCT *)GetFoundByNum(pMes, Num)) == 0 ){
            /*������� �������� ��࠭�稢. �������⮬ � ����஬ Num*/
            EdiTrace(TRACE2,">>>Can't get composite <%s:%d>",Comp,Num);
            return -1;
        }
        //           EdiTrace(TRACE3,"Set point to composite <%s:%d>",Comp,Num);
    }
    return 1;
}
/************************************************************/
/*                 ���� ��������                          */
/* return -1 - �ணࠬ��� �訡��                            */
/* return  0 - ��祣� �� ��諨                              */
/* return  1 - OK                                           */
/************************************************************/
int FindComp_(EDI_REAL_MES_STRUCT *pMes, const char *Comp, int FindReset)
{
    Edi_Point *pPoint;
    Find_Data *pFind ;
    int Ret;

    if(!FindReset){ /**/
        Ret = ResetEdiFind_(pMes);
        if(Ret < 0) return -1;
    }
    pPoint = pMes->pPoint;
    pFind  = pMes->pFind ;
    memcpy(pFind->Data, Comp, COMP_LEN+1);/*�� �᪠��*/
    pFind->FindFlag = 2; /*�饬 ��������!*/

    if(pPoint->pTag != NULL){
        Ret = FindCompIn1Tag(pFind,pPoint->pTag);
    } else {
        EdiError(EDILOG,"Trying to search composite not in a segment");
        Ret = -1;
    }
    if(!Ret)
        return -1;
    if(pFind->NumData == 0) return 0;
    return 1;
}

int FindCompIn1Tag(Find_Data *pFind, TAG_REAL_STRUCT *pTag)
{
    int Ret;

    Ret = FindCompInComp(pFind, pTag->Comp);
    if(!Ret) return Ret;
    return 1;
}

int FindCompInComp(Find_Data *pFind, COMP_REAL_STRUCT *pComp)
{
    int Ret;
    /*  COMPOSITE_STRUCT *pCompTmp;*/
    COMP_REAL_STRUCT *pCompArr;

    while(pComp != NULL){
        if(!pComp->SingleData){
            if(!memcmp((pComp->TemplateComp)->Composite, pFind->Data, COMP_LEN+1) ) {
                Ret = SaveData(pFind,(void *)pComp);
                if(!Ret) return Ret;
                if(pComp->NextArr != NULL){
                    pCompArr = pComp->NextArr;
                    while(pCompArr != NULL){
                        Ret = SaveData(pFind,(void *)pCompArr);
                        if(!Ret) return Ret;
                        pCompArr = pCompArr->NextArr;
                    }
                }
            }
        }
        pComp = pComp->Next;
    }
    return 1;
}
/**********************************************************/
/*�����頥� 㪠��⥫� �� �������� ����� (��㯯�       */
/* ᥣ���⮢, ᥣ����, ��������, �. ������ ) �� ������   */
/*return void * - 㪠��⥫� �� �����                    */
/**********************************************************/
void *GetFoundByNum(EDI_REAL_MES_STRUCT *pMes, int Num)
{
    Find_Data *pFind;
    void  **ppPoint;

    pFind = pMes->pFind;
    ppPoint = (void **)(pFind->pData + (Num*sizeof(void *) ));
    return (*ppPoint);

}

/********************************************************************/
/*      a) �᫨ ���� �� ���������  ᥣ�.  㦥 �ந��������,        */
/*         ��࠭�稢��� �� ���. ��������                           */
/*         return 1 - Ok                                            */
/*      b) ���᪠ �� �� �뫮.                                      */
/*         �ந������ ����;                                        */
/*         �믮��塞 �㭪� �);                                      */
/* return -1 - �ணࠬ���� �訡��                                   */
/* return  0 - ���  ᥣ�. � ������� ���浪��� ����஬             */
/* return  1 - �� Ok                                               */
/********************************************************************/
int SetEdiPointToSegment(const char *Segm, int Num)
{
    return SetEdiPointToSegment_(GetEdiMesStruct(), Segm, Num);
}
int SetEdiPointToSegment_(EDI_REAL_MES_STRUCT *pMes, const char *Segm, int Num)
{
    Find_Data *pFind;
    Edi_Point *pPoint;
    int Ret;

    if(Num < 0){
        EdiTrace(TRACE3,"Can't get segment: Num=%d, Segm=%s",Num,Segm);
        return 0;
    }
    if( GetNumSegment_(pMes, Segm)==-1){
        EdiTrace(TRACE3,"Can't get segment <%s>",Segm);
        return -1;
    }

    pPoint = pMes->pPoint;
    pFind = pMes->pFind;
    if(!pFind->NumData){
        EdiTrace(TRACE2,">>>Segment <%s:%d> is not found", Segm,Num);
        return 0;
    }
    if ( Num > pFind->NumData-1 ){
        EdiTrace(TRACE2,">>>Too big number for segment <%s:%d> %d max",Segm,Num,pFind->NumData-1);
        return 0;/*��� ᥣ�. � ������� ���. ����஬*/
    }
    else {
        Ret = ResetEdiPoint_(pMes);
        if(Ret < 0) return -1;
        if( (pPoint->pTag = (TAG_REAL_STRUCT *)GetFoundByNum(pMes, Num)) == 0 ){
            /*������� �������� ��࠭�稢. ᥣ�. � ����஬ Num*/
            EdiTrace(TRACE2,">>>Can't get segment <%s:%d>",Segm,Num);
            return -1;
        }
        //           EdiTrace(TRACE3,"Set point to segment <%s:%d>",Segm,Num);
    }
    return 1;

}
/************************************************************/
/*                 ���� ᥣ����                           */
/* return -1 - �ணࠬ��� �訡��                            */
/* return  0 - ��祣� �� ��諨                              */
/* return  1 - OK                                           */
/************************************************************/
int FindSegm_(EDI_REAL_MES_STRUCT *pMes, const char *Segm, int FindReset)
{
    Edi_Point *pPoint;
    Find_Data *pFind ;
    int Ret;

    if(!FindReset){ /**/
        Ret = ResetEdiFind_(pMes);
        if(Ret < 0) return -1;
    }
    pPoint = pMes->pPoint;
    pFind  = pMes->pFind ;
    memcpy(pFind->Data, Segm, TAG_LEN+1);/*�� �᪠��*/
    pFind->FindFlag = 3; /*�饬 ᥣ����!*/

    if(pPoint->pGrStr != NULL){
        Ret = FindSegmInTag (pFind,pPoint->pGrStr);
    } else if (pPoint->pTag || pPoint->pComp){
        EdiError(EDILOG,"Trying to find Segment %s in the Segment (Composite)! It's BAD!", Segm);
        Ret = 0;
    } else {
        Ret = FindSegmInTag (pFind,pMes->Tag);
    }
    if(!Ret)
        return -1;
    if(pFind->NumData == 0) return 0;
    return 1;
}

int FindSegmInTag(Find_Data *pFind, TAG_REAL_STRUCT *pTag)
{
    int Ret;
    TAG_REAL_STRUCT *pTagArr;

    while(pTag){
        if( !memcmp(pFind->Data, (pTag->TemplateTag)->TagName, TAG_LEN+1) ){
            Ret = SaveData(pFind, (void *)pTag);
            if(!Ret) return Ret;
            if(pTag->NextArr != NULL){
                pTagArr = pTag->NextArr;
                while(pTagArr != NULL){
                    Ret = SaveData(pFind, (void *)pTagArr);
                    if(!Ret) return Ret;
                    pTagArr = pTagArr->NextArr;
                }
            }
        }

        pTag = pTag->NextTag;
    }
    return 1;
}

/********************************************************************/
/*      a) �᫨ ���� �� �������� ��. ᥣ�.  㦥 �ந��������,      */
/*          ��࠭�稢��� ���. �������� ��㯯��.� ������� ���. ���.*/
/*         return 1 - Ok                                            */
/*      b) ���᪠ �� �� �뫮.                                      */
/*         �ந������ ����;                                        */
/*         �믮��塞 �㭪� �);                                      */
/* return -1 - �ணࠬ���� �訡��                                   */
/* return  0 - ��� ��. ᥣ�. � ������� ���浪��� ����஬          */
/* return  1 - �� Ok                                               */
/********************************************************************/
int SetEdiPointToSegGr(short SegGr, int Num)
{
    return SetEdiPointToSegGr_(GetEdiMesStruct(), SegGr, Num);
}

int SetEdiPointToSegGr_(EDI_REAL_MES_STRUCT *pMes, short SegGr, int Num)
{
    Find_Data *pFind;
    Edi_Point *pPoint;
    int Ret;

    if(Num < 0){
        EdiTrace(TRACE3,"Can't get segment group: Num=%d, SegGr=%d",Num,SegGr);
        return 0;
    }
    if( GetNumSegGr_(pMes, SegGr)==-1){
        EdiTrace(TRACE3,"Can't get segment group <%d>",SegGr);
        return -1;
    }

    pPoint = pMes->pPoint;
    pFind = pMes->pFind;

    if(!pFind->NumData){
        EdiTrace(TRACE2,">>>Segment group <%d:%d> is not found",SegGr,Num);
        return 0;
    }

    if ( Num > pFind->NumData-1 ) {
        EdiTrace(TRACE2,">>>Too big number for segment group <%d:%d> %d max",SegGr,Num,pFind->NumData-1);
        return 0;/*��� ᥣ�. ��. � ������� ���. ����஬*/
    }
    else {
        Ret = ResetEdiPoint_(pMes);
        if(Ret < 0) return -1;
        if( (pPoint->pGrStr = (TAG_REAL_STRUCT *)GetFoundByNum(pMes, Num)) == 0 ){
            /*������� �������� ��࠭�稢. ᥣ�. ��. � ����஬ Num*/
            EdiTrace(TRACE2,">>>Can't get segment group <%d:%d>",SegGr,Num);
            return -1;
        }
        //           EdiTrace(TRACE3,"Set point to segment group <%d:%d>",SegGr,Num);
    }
    return 1;
}
/************************************************************/
/*             ���� ᥣ���⭮� ��㯯�                      */
/* return -1 - �ணࠬ��� �訡��                            */
/* return  0 - ��祣� �� ��諨                              */
/* return  1 - OK                                           */
/************************************************************/
int FindSegGr(EDI_REAL_MES_STRUCT *pMes, short SegGr, int FindReset)
{
    Edi_Point *pPoint;
    Find_Data *pFind ;
    int Ret;

    if(!FindReset){ /**/
        Ret = ResetEdiFind_(pMes);
        if(Ret < 0) return -1;
    }
    pPoint = pMes->pPoint;
    pFind  = pMes->pFind ;
    memcpy(pFind->Data, &SegGr, sizeof(short));/*�� �᪠��*/
    pFind->FindFlag = 4; /*�饬 ᥣ������ ��㯯� !*/
    if(pPoint->pGrStr != NULL){
        Ret = FindSegGrInTag (pFind,pPoint->pGrStr);
    } else if(pPoint->pTag || pPoint->pComp) {
        EdiError(EDILOG,"Trying to find SegGr%d in the Segment (Composite)! It's BAD!", SegGr);
        Ret = 0;
    } else {
        Ret = FindSegGrInTag (pFind,pMes->Tag);
    }
    if(!Ret)
        return -1;
    if(pFind->NumData == 0) return 0;
    return 1;
}

int FindSegGrInTag(Find_Data *pFind, TAG_REAL_STRUCT *pTag)
{
    int Ret;
    TAG_REAL_STRUCT *pTagArr;

    while(pTag != NULL){
        if(pTag->GrStr != NULL){
            if( !memcmp(pFind->Data, &((pTag->TemplateTag)->Num), sizeof(short)) ){
                Ret = SaveData(pFind, (void *)pTag->GrStr);
                if(!Ret) return Ret;
                if(pTag->NextArr != NULL){
                    pTagArr = pTag->NextArr;
                    while(pTagArr != NULL){
                        Ret = SaveData(pFind, (void *)pTagArr->GrStr);
                        if(!Ret) return Ret;
                        pTagArr = pTagArr->NextArr;
                    }
                }
            }/*if ( !memcmp(...) )*/
        }

        pTag = pTag->NextTag;
    }
    return 1;
}

/********************************************************************/
/* �ந������ ����, �᫨ ���᪠ �� ���뫮                         */
/* �����頥� ���-�� ᥣ������ ��㯯 � ᮮ�饭��                   */
/* return -1 - �ணࠬ���� �訡��                                   */
/* return  N - ���-��                                               */
/********************************************************************/
int GetNumSegGr(short SegGr)
{
    return GetNumSegGr_(GetEdiMesStruct(), SegGr);
}
int GetNumSegGr_(EDI_REAL_MES_STRUCT *pMes, short SegGr)
{
    int FindReset = 0;

    if(pMes->pPoint == NULL){
        int Ret = ResetEdiPoint_(pMes);
        if(Ret < 0) return -1;
    }

    if(pMes->pFind == NULL){
        int Ret = ResetEdiFind_(pMes);
        if(Ret < 0) return -1;
        FindReset = 1;
    }

    if( (pMes->pFind->FindFlag != 4) || (memcmp(pMes->pFind->Data, &SegGr, sizeof(short))!=0) ||
            (pMes->pFind->pPoint != pMes->pPoint) ){
        /*���� �� �������� ᥣ. ��. �� �஢������*/
        pMes->pFind->pPoint = pMes->pPoint;
        int Ret = FindSegGr(pMes,SegGr,FindReset);
        if( Ret == -1 ) return Ret; /*�ணࠬ��� �訡��*/
    } else {
        //      EdiTrace(TRACE3,"Already found");
    }
    /*���� �ந������*/
    return pMes->pFind->NumData;
}

/********************************************************************/
/* �ந������ ����, �᫨ ���᪠ �� ���뫮                         */
/* �����頥� ���-�� ᥣ���⮢ � ᮮ�饭�� ���뢠� ���. ��������  */
/* return -1 - �ணࠬ���� �訡��                                   */
/* return  N - ���-��                                               */
/********************************************************************/
int GetNumSegment(const char *Segm)
{
    return GetNumSegment_(GetEdiMesStruct(), Segm);
}
int GetNumSegment_(EDI_REAL_MES_STRUCT *pMes, const char *Segm)
{
    Find_Data *pFind;
    Edi_Point *pPoint;
    int Ret, FindReset = 0;

    if(pMes->pPoint == NULL){
        Ret = ResetEdiPoint_(pMes);
        if(Ret < 0) return -1;
    }
    pPoint = pMes->pPoint;

    if(pMes->pFind == NULL){
        Ret = ResetEdiFind_(pMes);
        if(Ret < 0) return -1;
        FindReset = 1;
    }
    pFind = pMes->pFind;

    if( (pFind->FindFlag != 3) || (memcmp(pFind->Data, Segm, TAG_LEN+1)!=0) ||
            (pFind->pPoint != pMes->pPoint)){
        /*���� �� ��������� ᥣ����� �� �஢������*/
        pFind->pPoint = pMes->pPoint;
        Ret = FindSegm_(pMes, Segm, FindReset);
        if( Ret == -1 ) return Ret; /*�ணࠬ��� �訡��*/
    } else {
        //      EdiTrace(TRACE3,"Already found");
    }
    /*���� �ந������*/
    return (pFind->NumData);
}

/********************************************************************/
/* �ந������ ����, �᫨ ���᪠ �� ���뫮                         */
/* �����頥� ���-�� ᥣ���⮢ � ᮮ�饭�� ���뢠� ���. ��������  */
/* return -1 - �ணࠬ���� �訡��                                   */
/* return  N - ���-��                                               */
/********************************************************************/
int GetNumComposite(const char *Comp)
{
    return GetNumComposite_(GetEdiMesStruct(), Comp);
}
int GetNumComposite_(EDI_REAL_MES_STRUCT *pMes, const char *Comp)
{
    Find_Data *pFind;
    Edi_Point *pPoint;
    int Ret, FindReset = 0;

    if(pMes->pPoint == NULL){
        Ret = ResetEdiPoint_(pMes);
        if(Ret < 0) return -1;
    }
    pPoint = pMes->pPoint;

    if(pMes->pFind == NULL){
        Ret = ResetEdiFind_(pMes);
        if(Ret < 0) return -1;
        FindReset = 1;
    }
    pFind = pMes->pFind;

    if( (pFind->FindFlag != 2) || (memcmp(pFind->Data, Comp, COMP_LEN+1)!=0) ||
            (pFind->pPoint != pMes->pPoint)){
        /*���� �� ��������� ��������� �� �஢������*/
        pFind->pPoint = pMes->pPoint;
        Ret = FindComp_(pMes, Comp,FindReset);
        if( Ret == -1 ) return Ret; /*�ணࠬ��� �訡��*/
    } else {
        //      EdiTrace(TRACE3,"Already found");
    }
    /*���� �ந������*/
    return (pFind->NumData);
}

void PrintRealEdiMes(FILE *fl)
{
    PrintRealEdiMes_(GetEdiMesStruct(),fl);
    return ;
}

void PrintAllEdiTemplateMessages(FILE *fl)
{
    PrintAllEdiMessages(GetEdiTemplateMessages(),fl);
    return ;
}

int InsertEdiMaskChars2(EDI_REAL_MES_STRUCT *pMes, DATA_REAL_STRUCT *pData)
{
    if(pData->len == 0)
    {
        return 0;
    }

    char *data_str = (char *)pData->Data;
    pData->len = maskSpecialChars_capp(pMes->pCharSet, pData->Data, (char **)&pData->Data);
    if(pData->len >= 0)
    {
        edi_free(data_str);
        return 0;
    }
    else
        return -1;
}

int DeleteEdiMaskChars(const Edi_CharSet *Chars, char *str, int *len_)
{
    int i, j;
    int len = *len_;

    for(i = 0, j = 0; i < len; i++)
    {
        int k = i + 1;
        int str_flag = 0;

        if(str[i] == Chars->Release &&
                (str[k] == Chars->EndData || str[k] == Chars->EndComp || str[k] == Chars->Release ||
                 (len - k >= Chars->EndSegLen &&
                  !memcmp(&str[k], Chars->EndSegStr, Chars->EndSegLen) &&
                  (str_flag=Chars->EndSegLen))))
        {

            str_flag ? (i += str_flag) : (i++);
            if(str_flag)
            {
                memcpy(&str[j], &str[k], str_flag);
            }
            else
            {
                str[j] = str[k];
            }
        }
        else
        {
            str[j] = str[i];
        }
        str_flag ? (j += str_flag) : (j++);
    }

    str[j]='\0';
    *len_ = j;

    return 0;
}

int DeleteEdiMaskChars2(EDI_REAL_MES_STRUCT *pMes, DATA_REAL_STRUCT *pData)
{
    return DeleteEdiMaskChars(pMes->pCharSet, (char *)pData->Data, &pData->len);
}

int ChangeEdiCharSetStat(const char *edi_text, int len,
                         const char *for_coding, char *dest, size_t dest_size)
{
    char *dest_tmp=NULL;
    size_t len_tmp=0;
    int ret=ChangeEdiCharSet(edi_text,len,for_coding,&dest_tmp, &len_tmp);

    if(ret==EDI_MES_OK && dest_tmp){
        if(len_tmp<dest_size){
            memcpy(dest,dest_tmp,len_tmp+1);
        } else {
            WriteEdiLog(EDILOG,"Input buffer is too small");
            ret = EDI_MES_ERR;
        }
        edi_free(dest_tmp);
    }
    return ret;
}

int ChangeEdiCharSet(const char *edi_text, int len,
                     const char *for_coding, char **dest, size_t *dest_len)
{
    char mes_name[MESSAGE_LEN+1];
    int ret=EDI_MES_OK;
    EDI_REAL_MES_STRUCT *pCurMes=NULL, *pCurMes2=NULL;
    const char *chs_str;

    if(strlen(for_coding)!=EDI_CHSET_LEN){
        WriteEdiLog(EDILOG,"Wrong length of input charset: %s(%zd)/must be %d",
                    for_coding,strlen(for_coding), EDI_CHSET_LEN);
        return EDI_MES_ERR;
    }

    if(len>TAG_LEN && !memcmp(edi_text, CUSTOM_CHAR_SET_TAG, TAG_LEN)){
        edi_text = edi_text+TAG_LEN+CUSTOM_CHAR_SET_LEN;
        len -= TAG_LEN + CUSTOM_CHAR_SET_LEN;
    }

    if(CreateEdiMesStruct(&pCurMes) || CreateEdiMesStruct(&pCurMes2)) {
        DeleteRealMesStruct(pCurMes);
        DeleteRealMesStruct(pCurMes2);
        ret = EDI_MES_ERR;
        return ret;
    }

    if((chs_str=GetEdiCharSetByName(GetEdiTemplateMessages(),for_coding,0)) &&
            !SetCharSetOnType(pCurMes2, chs_str, LoadedEdiCharSet)){
        tst();
    } else {
        EdiTrace(TRACE0,"Unknown character set in the message");
        ret = EDI_MES_STRUCT_ERR;
    }

    if (ret == EDI_MES_OK){
        ret = GetMesNameCharSet(pCurMes, GetEdiTemplateMessages(),
                                edi_text, len, mes_name, CHECK_LOG_LEVEL);
    }

    if(ret == EDI_MES_OK) {
#if _DUAL_CHAR_SET_SUPPORT_
		if(pCurMes->pCharSet->EndComp==pCurMes2->pCharSet->EndComp &&
				pCurMes->pCharSet->EndData==pCurMes2->pCharSet->EndData &&
				pCurMes->pCharSet->Release==pCurMes2->pCharSet->Release &&
				!strcmp(pCurMes->pCharSet->EndSegStr,pCurMes2->pCharSet->EndSegStr)){
            EdiTrace(TRACE2,"Equal charset. Nothing to do!");
        }
#else /* _DUAL_CHAR_SET_SUPPORT_ */
        if(ret==EDI_MES_OK && !memcmp(chs_str,for_coding,EDI_CHSET_LEN)){
            EdiTrace(TRACE2,"src charset=%.*s, dest charset=%.*s. Nothing to do!",
                     EDI_CHSET_LEN, chs_str, EDI_CHSET_LEN, for_coding);
        }
#endif
        else {
            ret = ChangeEdiCharSet_(edi_text, len, dest, dest_len,
                                    pCurMes->pCharSet,pCurMes2->pCharSet);
            if(ret == EDI_MES_OK){
                char *chs=(char *)GetCharSetNameFromText(*dest, *dest_len, 0);
                memcpy(chs, for_coding, EDI_CHSET_LEN);
            }

        }
    }

    DeleteRealMesStruct(pCurMes);
    DeleteRealMesStruct(pCurMes2);

    return ret;
}


int ChangeEdiCharSet_(const char *src, size_t srclen,
					  char **dest, size_t *destlen,
					  Edi_CharSet *CharSrc, Edi_CharSet *CharDest)
{
    MesEdiStruct Dest;
    size_t i;
    int err=EDI_MES_ERR;

    tst();

    memset(&Dest, 0, sizeof(Dest));
    if(realloc_edi_buff(&Dest, srclen)){
        return -1;
    }

    /*
     1)������� ��᪨� ᨬ����
     2)�������� �� ���� �ࠢ���騥
     3)�᫠���� ���� ��᪨� ᨬ����
     */

    for(i=0,Dest.len=0; i<srclen; i++,Dest.len++){
        int str_flag=0,str_flag2=0;

        /*1)*/
        if(src[i] == CharSrc->Release &&
                (src[i+1] == CharSrc->EndData || src[i+1] == CharSrc->EndComp ||
                 src[i+1] == CharSrc->Release ||
                 (srclen-i+1>=CharSrc->EndSegLen &&
                  !memcmp(&src[i+1], CharSrc->EndSegStr, CharSrc->EndSegLen) &&
                  (str_flag=CharSrc->EndSegLen)))){

            i++;
        } else {
            /*2)*/
            /*���� ����᪨஢��, � ����� �ࠢ���騩!*/
            if(realloc_edi_buff(&Dest, 1)){
                break;
            }
            if(src[i] == CharSrc->EndData){
                Dest.mes[Dest.len]=CharDest->EndData;
                continue;
            }else if(src[i] == CharSrc->EndComp){
                Dest.mes[Dest.len]=CharDest->EndComp;
                continue;
            }else if(src[i] == CharSrc->Release){
                Dest.mes[Dest.len]=CharDest->Release;
                continue;
            }else if((srclen-i>=CharSrc->EndSegLen &&
                      !memcmp(&src[i], CharSrc->EndSegStr, CharSrc->EndSegLen)&&
                      (str_flag=CharSrc->EndSegLen))){

                if(realloc_edi_buff(&Dest, CharDest->EndSegLen)){
                    break;
                }
                memcpy(&Dest.mes[Dest.len], CharDest->EndSegStr, CharDest->EndSegLen);
                Dest.len+=CharDest->EndSegLen-1;
                i+=CharSrc->EndSegLen-1;
                continue;
            }

        }

        if(realloc_edi_buff(&Dest, 1)){
            break;
        }
        /*����� ᨬ���*/
        /*3)*/
        /*����� ���� ����᪨஢���*/
        if(src[i] == CharDest->EndData ||
                src[i] == CharDest->EndComp ||
                src[i] == CharDest->Release ||
                (srclen-i>=CharDest->EndSegLen &&
                 !memcmp(&src[i], CharDest->EndSegStr, CharDest->EndSegLen) &&
                 (str_flag2=CharDest->EndSegLen))){

            Dest.mes[Dest.len]=CharDest->Release;
            Dest.len++;
            if(realloc_edi_buff(&Dest, str_flag2?str_flag2:1)){
                break;
            }
        }
        if(str_flag2){
            memcpy(&Dest.mes[Dest.len], &src[i], CharDest->EndSegLen);
            Dest.len += CharDest->EndSegLen-1;
            i+=CharDest->EndSegLen-1;
        } else {
            Dest.mes[Dest.len]=src[i];
        }
    } /* for (...) */

    if(i==srclen){
        err=EDI_MES_OK;
        Dest.mes[Dest.len]='\0';
        *dest=Dest.mes;
        *destlen=Dest.len;
    } else {
        WriteEdiLog(EDILOG,"something wrong ...");
    }

    return err;
}

