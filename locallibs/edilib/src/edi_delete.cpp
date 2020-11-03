#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include "edi_tables.h"
#include "edi_all.h"
#include <stdlib.h>
#include <string.h>
#include "edi_func.h"
#include "edi_user_func.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include "edi_test.h"
#include "edi_malloc.h"

/****************************************************************************/
/******************Delete functions******************************************/
/****************************************************************************/

void DeleteAllEdiMessages( EDI_MESSAGES_STRUCT **ppEdiMes )
{
    EDI_MESSAGES_STRUCT *pEdiMes;
    EDI_TEMP_MESSAGE *pTemp, *pDelEdiMes;
    pEdiMes = *ppEdiMes;
    if (pEdiMes == NULL) return;
    *ppEdiMes = NULL;

    edi_free(pEdiMes->HBNTypes);
    edi_free(pEdiMes->HBTTypes);

    pTemp=pEdiMes->pTemps;
    while( pTemp ){
        DeleteTags(pTemp->Tag);
        pDelEdiMes = pTemp;
		pTemp = pTemp->Next;
		free(pDelEdiMes);
    }/*end while*/
    return;
}

void DeleteTags(TAG_STRUCT *pTag)
{
    TAG_STRUCT *pTagTmp, *pDelTagTmp;

    pTagTmp = pTag;
    while( pTagTmp ){
        if(pTagTmp->GrStr)
            DeleteTags(pTagTmp->GrStr);
        if(pTagTmp->Comp)
            DeleteComp(pTagTmp->Comp);
        pDelTagTmp = pTagTmp;
        pTagTmp = pTagTmp->NextTag;
        free(pDelTagTmp);
    }
    return;
}

void DeleteComp(COMPOSITE_STRUCT *pComp)
{
    COMPOSITE_STRUCT *pCompTmp, *pDelComp;

    pCompTmp = pComp;
    while( pCompTmp ){
        DeleteDataElem(pCompTmp->DataElem);
        pDelComp = pCompTmp;
        pCompTmp = pCompTmp->Next;
        free(pDelComp);
    }
    return;
}

void DeleteDataElem(DATA_ELEM_STRUCT *pDataElem)
{
    DATA_ELEM_STRUCT *pDataTmp, *pDelData;

    pDataTmp = pDataElem;
    while( pDataTmp ){
        pDelData = pDataTmp;
        pDataTmp = pDataTmp->Next;
        free(pDelData);
    }
    return;
}

/****************************************************************************/

void DeleteAllEdiTabMessages(EDI_TABLES_STRUCT **ppTab, int delflag)
{
    EDI_TABLES_STRUCT *pTab;
    /**********************************************/
    /*delflag = 0 - удаляем все, вместе с текстом!*/
    /*delflag = 1 - текст не трогаем!             */
    /**********************************************/
    pTab = *ppTab;
    if (pTab == NULL) return;
    *ppTab = NULL;
    DeleteMesTable    ( pTab , delflag );
    DeleteMesStrTable ( pTab , delflag );
    DeleteSegStrTable ( pTab , delflag );
    DeleteCompStrTable( pTab , delflag );
    DeleteDataTable   ( pTab , delflag );
    DeleteSegTable    ( pTab , delflag );
    DeleteCompTable   ( pTab , delflag );
    return;
}

void DeleteMesTable(EDI_TABLES_STRUCT *pTab, int delflag)
{
    MESSAGE_TABLE_STRUCT *MesTabTmp, *DelMesTabTmp;
    if(pTab->MesTable == NULL) return;
    MesTabTmp = pTab->MesTable;
    pTab->MesTable = NULL;
    while( 1 ){
        if(MesTabTmp == NULL) return;
        DelMesTabTmp = MesTabTmp;
        MesTabTmp = MesTabTmp->Next;
        free(DelMesTabTmp);
    }
}

void DeleteMesStrTable(EDI_TABLES_STRUCT *pTab, int delflag)
{
    MES_STRUCT_TABLE_STRUCT   *MesStrTabTmp, *DelMesStrTab;
    if(pTab->MesStrTable == NULL) return;
    MesStrTabTmp = pTab->MesStrTable;
    pTab->MesStrTable = NULL;
    while( 1 ){
        if(MesStrTabTmp == NULL) return;
        DelMesStrTab = MesStrTabTmp;
        MesStrTabTmp = MesStrTabTmp->Next;
        free(DelMesStrTab);
    }
}

void DeleteSegStrTable(EDI_TABLES_STRUCT *pTab, int delflag)
{
    SEG_STRUCT_TABLE_STRUCT   *SegStrTabTmp, *DelSegStrTabTmp;
    if(pTab->SegStrTable == NULL) return;
    SegStrTabTmp = pTab->SegStrTable;
    pTab->SegStrTable = NULL;
    while( 1 ){
        if(SegStrTabTmp == NULL) return;
        DelSegStrTabTmp = SegStrTabTmp;
        SegStrTabTmp = SegStrTabTmp->Next;
        free(DelSegStrTabTmp);
    }
}

void DeleteCompStrTable(EDI_TABLES_STRUCT *pTab, int delflag)
{
    COMP_STRUCT_TABLE_STRUCT  *CompStrTabTmp , *DelCompStrTabTmp;
    if(pTab->CompStrTable == NULL) return;
    CompStrTabTmp = pTab->CompStrTable;
    pTab->CompStrTable = NULL;
    while( 1 ){
        if(CompStrTabTmp == NULL) return;
        DelCompStrTabTmp = CompStrTabTmp;
        CompStrTabTmp = CompStrTabTmp->Next;
        free(DelCompStrTabTmp);
    }
}

void DeleteDataTable(EDI_TABLES_STRUCT *pTab, int delflag)
{
    DATA_ELEM_TABLE_STRUCT    *DataTabTmp, *DelDataTabTmp;
    if(pTab->DataTable == NULL) return;
    DataTabTmp = pTab->DataTable;
    pTab->DataTable = NULL;
    while( 1 ){
        if(DataTabTmp == NULL) return;
        DelDataTabTmp = DataTabTmp;
        DataTabTmp = DataTabTmp->Next;
        free(DelDataTabTmp);
    }
}

void DeleteSegTable( EDI_TABLES_STRUCT *pTab ,int delflag)
{
    SEGMENT_TABLE_STRUCT *SegTabTmp1,*SegTabTmp2;
    SegTabTmp1 = pTab->SegTable;
    if(SegTabTmp1 == NULL) return;
    pTab->SegTable = NULL;
    while( 1 )
    {
        if( SegTabTmp1 == NULL ) break;
        SegTabTmp2 = SegTabTmp1->Next;
        free(SegTabTmp1);
        SegTabTmp1 = SegTabTmp2;
    }
    return ;
}

void DeleteCompTable( EDI_TABLES_STRUCT *pTab ,int delflag)
{
    COMPOSITE_TABLE_STRUCT *CompTabTmp1,*CompTabTmp2;
    CompTabTmp1 = pTab->CompTable;
    if(CompTabTmp1 == NULL) return;
    pTab->CompTable = NULL;
    while( 1 )
    {
        if( CompTabTmp1 == NULL ) break;
        CompTabTmp2 = CompTabTmp1->Next;
        free(CompTabTmp1);
        CompTabTmp1 = CompTabTmp2;
    }
    return;
}

/****************************************************************************/
/****************************************************************************/

void DeleteMesIncoming(void)
{
    DeleteRealMes(GetEdiMesStruct());
}

void DeleteMesOutgoing(void)
{
    DeleteRealMes(GetEdiMesStructW());
}

/*Удаляет стр-ру целиком*/
void DeleteRealMesStruct(EDI_REAL_MES_STRUCT *pEdiMes)
{
    DeleteRealMes(pEdiMes);
    free(pEdiMes);
}

void DeleteRealMes(EDI_REAL_MES_STRUCT *pEdiMes)
{
    if (pEdiMes == NULL)
        return;

    DelRealTags(pEdiMes->Tag);

    free_edi_buff(&pEdiMes->MesTxt);

    if(pEdiMes->pPoint)
        ResetEdiPoint_(pEdiMes);
    ResetStackPoint_(pEdiMes);    /*Обнуляем стек (удаляем все сохраненные в нем эл.)*/
    ResetEdiFound_(pEdiMes);      /**/
    if(pEdiMes->pFind)
        ResetEdiFind_(pEdiMes);   /**/
    ResetEdiErr_(pEdiMes);

    free(pEdiMes->pPoint);
    if(pEdiMes->pFind)
    {
        free(pEdiMes->pFind->pData);
        free(pEdiMes->pFind);
    }
    memset(pEdiMes, 0, sizeof(EDI_REAL_MES_STRUCT));
    return;
}

void DelRealTags(TAG_REAL_STRUCT *pTag)
{
    TAG_REAL_STRUCT *pTagTmp;

    while( pTag ){
        if(pTag->Comp    != NULL)  DelRealComp(pTag->Comp   );
        if(pTag->GrStr   != NULL)  DelRealTags(pTag->GrStr  );
        if(pTag->NextArr != NULL)  DelRealTags(pTag->NextArr);
        pTagTmp = pTag;
        pTag = pTag->NextTag;
        free(pTagTmp);
    }
}

void DelRealComp(COMP_REAL_STRUCT *pComp)
{
    COMP_REAL_STRUCT *pCompTmp;

    while( pComp ){
        if(pComp->DataElem != NULL)
            DelRealDataElem(pComp->DataElem);
        if(pComp->NextArr != NULL)
            DelRealComp(pComp->NextArr);
        pCompTmp = pComp;
        pComp = pComp->Next;
        free(pCompTmp);
    }
}

void DelRealDataElem(DATA_REAL_STRUCT *pDataElem)
{
    DATA_REAL_STRUCT *pDataTmp;

    while( pDataElem ){
        if(pDataElem->NextArr != NULL)
            DelRealDataElem(pDataElem->NextArr);
        pDataTmp = pDataElem;
        pDataElem = pDataElem->Next;
        if(pDataTmp->malloc)
            edi_free ((char *)pDataTmp->Data);
        free(pDataTmp);
    }
    return;
}


