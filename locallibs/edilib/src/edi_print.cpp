#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include "edi_tables.h"
#include "edi_all.h"
#include "edi_func.h"
#include "edi_user_func.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include "edi_test.h"
/****************************************************************************/
/******************Print functions*******************************************/
/****************************************************************************/

void PrintAllEdiMessages( EDI_MESSAGES_STRUCT *pEdiTemps, FILE *fl )
{
    int Vloj = -1;
    EDI_TEMP_MESSAGE *pEdiMes=pEdiTemps?pEdiTemps->pTemps:NULL;

    if (pEdiMes == NULL) return;
    while( 1 ){
        fprintf(fl,"\n\n%s  %s\n", pEdiMes->Type.code, pEdiMes->Text);
        PrintTags(pEdiMes->Tag,&Vloj,fl);
        pEdiMes = pEdiMes->Next;
        if(pEdiMes == NULL){
            break;
        }
        Vloj = -1;
    }/*end while*/
    return;
}

void PrintTags(TAG_STRUCT *pTag, int *Vloj, FILE *fl)
{
    TAG_STRUCT *pTagTmp;
    int i;

    (*Vloj)++;
    pTagTmp = pTag;
    while( 1 ){
        if(pTagTmp == NULL) return;
        if(pTagTmp->TagName[0] != '\0'){
            for (i=0;i<(*Vloj);i++)
                fprintf(fl,"+");
            fprintf(fl,"%4d %s  %c  %d",pTagTmp->Pos,pTagTmp->TagName,
                    pTagTmp->S,pTagTmp->MaxR);
            fprintf(fl,"  %s\n",pTagTmp->Text);
        }
        else {
            for (i=0;i<(*Vloj);i++)
                fprintf(fl,"+");
            fprintf(fl,"%d  %s  %c  %d\n",pTagTmp->Pos,pTagTmp->Text,
                    pTagTmp->S, pTagTmp->MaxR);
        }
        if(pTagTmp->Comp != NULL) PrintComp(pTagTmp->Comp,fl);
        if(pTagTmp->GrStr != NULL){
            PrintTags(pTagTmp->GrStr,Vloj,fl);
            (*Vloj)--;
        }
        pTagTmp = pTagTmp->NextTag;
    }
    return;
}

void PrintComp(COMPOSITE_STRUCT *pComp, FILE *fl)
{
    COMPOSITE_STRUCT *pCompTmp;
    DATA_ELEM_STRUCT *pData;
    /*int pos=7; */
    pCompTmp = pComp;
    while( 1 ){
        if(pCompTmp == NULL) break;
        if(pCompTmp->Composite[0] != '\0') {
            fprintf(fl,"%7d  %s  %c  %d  %s\n",pCompTmp->Pos,pCompTmp->Composite,
                    pCompTmp->S,pCompTmp->MaxR,pCompTmp->Text);
            PrintDataElem(pCompTmp->DataElem,fl);
        } else  {
            pData = pCompTmp->DataElem;
            fprintf(fl,"%7d  %4d  %c%d..%2d  %c  %d  %s\n",
                    pData->Pos,pData->DataElem,pData->Format,pData->MinField,
                    pData->MaxField,pData->S,pData->MaxR,pData->Text);
        }
        pCompTmp = pCompTmp->Next;
    }
    return;
}

void PrintDataElem(DATA_ELEM_STRUCT *pDataElem, FILE *fl)
{
    DATA_ELEM_STRUCT *pDataTmp;

    pDataTmp = pDataElem;
    while( 1 ){
        if(pDataTmp == NULL) break;
        fprintf(fl,"%10d  %04d  %c%d..%2d  %c  %d  %s\n",
                pDataTmp->Pos,pDataTmp->DataElem,pDataTmp->Format,pDataTmp->MinField,
                pDataTmp->MaxField,pDataTmp->S,pDataTmp->MaxR,pDataTmp->Text);
        pDataTmp = pDataTmp->Next;
    }
    return;
}
/****************************************************************************/
/****************************************************************************/

void PrintRealEdiMes_( EDI_REAL_MES_STRUCT *pMes, FILE *fl)
{
    int Vloj = -1;
    char str[50];
    Edi_Point *pPoint;
    if (pMes == NULL) return;
    fprintf(fl,"%s  %50s\n",pMes->Message,pMes->Text);

    if(pMes->pPoint == NULL)
        ResetEdiPoint_(pMes);
    pPoint = pMes->pPoint;
    PrintEdiPoint(pMes, str, sizeof(str));
    fprintf(fl,"%s\n",str);

    if(pPoint->pTag != NULL){
        PrintRealComp((pPoint->pTag)->Comp,fl);
    } else if(pPoint->pGrStr != NULL){
        PrintRealTags(pPoint->pGrStr,&Vloj,fl);
    } else if(pPoint->pComp != NULL){
        PrintRealDataElem((pPoint->pComp)->DataElem,fl);
    } else {
        PrintRealTags(pMes->Tag,&Vloj,fl);
    }
    return;
}

void PrintRealTags(TAG_REAL_STRUCT *pTag, int *Vloj, FILE *fl)
{
    TAG_REAL_STRUCT *pTagTmp;
    int i;

    (*Vloj)++;
    pTagTmp = pTag;
    while( 1 ){
        if(pTagTmp == NULL) return;
        if(pTagTmp->GrStr == NULL){
            for (i=0;i<(*Vloj);i++)
                fprintf(fl,"+");
            fprintf(fl,"%4s:%d\n",(pTagTmp->TemplateTag)->TagName, pTagTmp->R);
            if(pTagTmp->ParTag)
                fprintf(fl,"[%s:%d]\n",(pTagTmp->ParTag)->TemplateTag->Text, pTagTmp->ParTag->R);
        }
        else {
            for (i=0;i<(*Vloj);i++)
                fprintf(fl,"+");
            fprintf(fl,"%4s:%d \n",(pTagTmp->TemplateTag)->Text, pTagTmp->R);
            if(pTagTmp->ParTag)
                fprintf(fl,"[%s:%d]\n",(pTagTmp->ParTag)->TemplateTag->Text, pTagTmp->ParTag->R);
        }
        if(pTagTmp->Comp != NULL) PrintRealComp(pTagTmp->Comp,fl);
        if(pTagTmp->GrStr != NULL){
            PrintRealTags(pTagTmp->GrStr,Vloj,fl);
            (*Vloj)--;
        }
        if(pTagTmp->NextArr!=NULL){
            (*Vloj)--;
            PrintRealTags(pTagTmp->NextArr,Vloj,fl);
        }
        pTagTmp = pTagTmp->NextTag;
    }
    return;
}

void PrintRealComp(COMP_REAL_STRUCT *pComp,FILE *fl)
{
    DATA_REAL_STRUCT *pData;
    while( 1 ){
        if(pComp == NULL) break;
        if(!pComp->SingleData) {
            fprintf(fl,"%10s:%d\n",(pComp->TemplateComp)->Composite, pComp->R);
            fprintf(fl,"[%s:%d]\n", pComp->ParTag->TemplateTag->TagName, pComp->ParTag->R);
            PrintRealDataElem(pComp->DataElem,fl);
        } else  {
            pData = pComp->DataElem;
            fprintf(fl,"%6s%04d:%d        %s\n","",pData->DataElem,pData->R,pData->Data);
            fprintf(fl,"[%s:%d]\n", pData->ParComp->TemplateComp->Composite, pData->ParComp->R);
            if(pData->NextArr != NULL)
                PrintRealDataElem(pData->NextArr,fl);
        }
        if(pComp->NextArr != NULL) PrintRealComp(pComp->NextArr,fl);
        pComp = pComp->Next;
    }
    return;
}

void PrintRealDataElem(DATA_REAL_STRUCT *pData,FILE *fl)
{
    while( 1 ){
        if(pData == NULL) break;
        fprintf(fl,"%14s%04d:%d  %s\n","",pData->DataElem,pData->R,pData->Data);
        if(pData->NextArr != NULL)
            PrintRealDataElem(pData->NextArr,fl);
        pData = pData->Next;
    }
    return;
}
/****************************************************************************/
/****************************************************************************/

void PrintMesStrTabMes(MES_STRUCT_TABLE_STRUCT *MesStrTab,FILE *fl)
{
    fprintf(fl,"Printing MesStrTab :\n\n");
    while( MesStrTab != NULL ){
        fprintf(fl,"Pos :%d  MaxPos:%d\n",MesStrTab->Pos,MesStrTab->MaxPos);
        MesStrTab = MesStrTab->Next;
    }
    return;
}

/* Печатает текущую область видимости */
void PrintEdiPoint(EDI_REAL_MES_STRUCT *pMes, char *str, size_t size)
{
#if 0
    strcpy(str,"???");
#else
    if(pMes->pPoint){
        if(pMes->pPoint->pGrStr){
            if(!pMes->pPoint->pGrStr->ParTag){
                /*WriteEdiLog(STDLOG,"pMes->pPoint->pGrStr->ParTag is NULL!!!");*/
                snprintf(str, size, "%s:%d",
                         pMes->pPoint->pGrStr->TemplateTag->Text,
                         pMes->pPoint->pGrStr->R);
            } else {
                snprintf(str, size, "%s:%d",
                         pMes->pPoint->pGrStr->ParTag->TemplateTag->Text,
                         pMes->pPoint->pGrStr->ParTag->R);
            }
            /*              snprintf(str, size, "Segment group");*/
        } else if(pMes->pPoint->pTag){
            snprintf(str, size, "%s:%d",
                     pMes->pPoint->pTag->TemplateTag->TagName,
                     pMes->pPoint->pTag->R);
        } else if(pMes->pPoint->pComp){
            snprintf(str, size, "%s:%d",
                     pMes->pPoint->pComp->TemplateComp->Composite,
                     pMes->pPoint->pComp->R);
        } else {
            snprintf(str, size, "Message root");
        }
    } else {
        snprintf(str,size,"Message root");
    }
#endif
}
