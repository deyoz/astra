#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <string.h>
#include <stdlib.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE

#include <vector>
#include "edilib_db_callbacks.h"
#include "edi_tables.h"
#include "edi_all.h"
#include "edi_func.h"
#include "edi_test.h"
#include "edi_sql_insert.h"
#include "edi_sql_func.h"

/***********************************************************/
/*Считывает в ОП файлы, описывающие форматы edi-сообщений*/
/*return EDI_READ_BASE_OK - считывание прошло успешно          */
/*return EDI_READ_BASE_ERR - считывание завершено с ошибкой    */
/***********************************************************/
int ReadTablesFf(const char *directory, EDI_TABLES_STRUCT **ppTab)
{
    EdiFilePoints Fp;
    int ret;

    EdiTrace(TRACE3,"directory is %s",directory);
    memset(&Fp,0,sizeof(Fp));

    if(open_edi_files(directory,&Fp)){
        return EDI_READ_BASE_ERR;
    }

    ret= ReadTablesType(ppTab, &Fp);
    close_edi_files(&Fp);

    return ret;
}

/***********************************************************/
/*Считывает в ОП таблицы, описывающие форматы edi-сообщений*/
/*return EDI_READ_BASE_OK - считывание прошло успешно          */
/*return EDI_READ_BASE_ERR - считывание завершено с ошибкой    */
/***********************************************************/
int ReadTablesOra(EDI_TABLES_STRUCT **ppTab)
{
    return ReadTablesType(ppTab,NULL);
}

int ReadTablesType(EDI_TABLES_STRUCT **ppTab, EdiFilePoints *pFp)
{
    EDI_TABLES_STRUCT   *pTab;

    if( ( *ppTab = (EDI_TABLES_STRUCT *)
          calloc(1, sizeof( EDI_TABLES_STRUCT )) ) == NULL){
        return EDI_READ_BASE_ERR ;
    }
    pTab = *ppTab;

    if(pFp && ReadMesFileTable(pTab,pFp->mes)!=EDI_READ_BASE_OK){
        return EDI_READ_BASE_ERR;
    }
    else if(!pFp){
        if( ReadMesTable(pTab) != EDI_READ_BASE_OK)
            return EDI_READ_BASE_ERR ;

        if( ReadMesStrTable(pTab) != EDI_READ_BASE_OK)
            return EDI_READ_BASE_ERR ;
    }

    if(pFp && ReadSegFileTable(pTab,pFp->seg)!=EDI_READ_BASE_OK){
        return EDI_READ_BASE_ERR;
    }
    else if(!pFp){
        if( ReadSegTable(pTab) != EDI_READ_BASE_OK)
            return EDI_READ_BASE_ERR ;

        if( ReadSegStrTable(pTab) != EDI_READ_BASE_OK)
            return EDI_READ_BASE_ERR ;
    }

    if(pFp && ReadCompFileTable(pTab,pFp->comp)!=EDI_READ_BASE_OK)
    {
        return EDI_READ_BASE_ERR;
    }
    else if(!pFp){
        if( ReadCompTable(pTab) != EDI_READ_BASE_OK)
            return EDI_READ_BASE_ERR ;

        if( ReadCompStrTable(pTab) != EDI_READ_BASE_OK)
            return EDI_READ_BASE_ERR ;
    }


    if(pFp && ReadDataFileTable(pTab,pFp->data)!=EDI_READ_BASE_OK){
        return EDI_READ_BASE_ERR;
    }
    else if(!pFp && ReadDataTable(pTab) != EDI_READ_BASE_OK)
        return EDI_READ_BASE_ERR ;

    return EDI_READ_BASE_OK;
}

int ReadMesFileTable(EDI_TABLES_STRUCT *pTab,FILE *fp_mes)
{
    if(insert_to_tab_from_mes_file(pTab,fp_mes)<=0){
        return EDI_READ_BASE_ERR;
    }
    return EDI_READ_BASE_OK;
}

void ReadMesTableData(std::vector<MESSAGE_TABLE_STRUCT> &vMesTable)
{
    edilib::EdilibDbCallbacks::instance()->readMesTableData(vMesTable);
}

int ReadMesTable(EDI_TABLES_STRUCT *pTab)
{
    MESSAGE_TABLE_STRUCT *pMesTable1,*pMesTable2;

    if((pTab->MesTable=(MESSAGE_TABLE_STRUCT *)
        calloc(1, sizeof(MESSAGE_TABLE_STRUCT)))==NULL )
        return EDI_READ_BASE_ERR ;

    pMesTable1 = pMesTable2 = pTab->MesTable;

    std::vector<MESSAGE_TABLE_STRUCT> vMesTable;
    ReadMesTableData(vMesTable);

    for(const MESSAGE_TABLE_STRUCT &MesTabTmp: vMesTable) {
        memcpy(pMesTable1, &MesTabTmp, sizeof(MesTabTmp));

        if((pMesTable1->Next=(MESSAGE_TABLE_STRUCT *)
            calloc(1, sizeof(MESSAGE_TABLE_STRUCT)))==NULL )
            return EDI_READ_BASE_ERR ;

        pMesTable2 = pMesTable1;
        pMesTable1 = pMesTable2->Next;
    }

    if(pMesTable1 == pMesTable2)
        pTab->MesTable = NULL;
    else
        pMesTable2->Next = NULL;
    free(pMesTable1);

    return EDI_READ_BASE_OK;
}

/******************************************************************/
void ReadMesStrTableData(std::vector<MES_STRUCT_TABLE_STRUCT> &vMesStrTable)
{
    edilib::EdilibDbCallbacks::instance()->readMesStrTableData(vMesStrTable);
}

int ReadMesStrTable(EDI_TABLES_STRUCT *pTab)
{
    MES_STRUCT_TABLE_STRUCT *pMesStrTable1,*pMesStrTable2;

    if((pTab->MesStrTable=(MES_STRUCT_TABLE_STRUCT *)
        calloc(1, sizeof(MES_STRUCT_TABLE_STRUCT)))==NULL )
        return EDI_READ_BASE_ERR ;

    pMesStrTable1 = pMesStrTable2 = pTab->MesStrTable;
    std::vector<MES_STRUCT_TABLE_STRUCT> vMesStrTable;
    ReadMesStrTableData(vMesStrTable);

    for(const MES_STRUCT_TABLE_STRUCT &MesStrTabTmp: vMesStrTable){
        memcpy(pMesStrTable1, &MesStrTabTmp, sizeof(MES_STRUCT_TABLE_STRUCT));

        if((pMesStrTable1->Next=(MES_STRUCT_TABLE_STRUCT *)
            calloc(1, sizeof(MES_STRUCT_TABLE_STRUCT)))==NULL )
            return EDI_READ_BASE_ERR ;

        pMesStrTable2 = pMesStrTable1;
        pMesStrTable1 = pMesStrTable2->Next;
    }

    if(pMesStrTable1 == pMesStrTable2)
        pTab->MesStrTable = NULL;
    else
        pMesStrTable2->Next = NULL;
    free(pMesStrTable1);

    return EDI_READ_BASE_OK;
}
/***********************************************************/

int ReadSegFileTable(EDI_TABLES_STRUCT *pTab,FILE *fp_seg)
{
    if(insert_to_tab_from_seg_file(pTab,fp_seg)<=0){
        return EDI_READ_BASE_ERR;
    }
    return EDI_READ_BASE_OK;
}

void ReadSegStrTableData(std::vector<SEG_STRUCT_TABLE_STRUCT> &vSegStrTable)
{
    edilib::EdilibDbCallbacks::instance()->readSegStrTableData(vSegStrTable);
}

int ReadSegStrTable(EDI_TABLES_STRUCT *pTab)
{
    SEG_STRUCT_TABLE_STRUCT *pSegStrTable1,*pSegStrTable2;
    
    if((pTab->SegStrTable=(SEG_STRUCT_TABLE_STRUCT *)
        calloc(1, sizeof(SEG_STRUCT_TABLE_STRUCT)))==NULL )
        return EDI_READ_BASE_ERR ;

    pSegStrTable1 = pSegStrTable2 = pTab->SegStrTable;

    std::vector<SEG_STRUCT_TABLE_STRUCT> vSegStrTable;
    ReadSegStrTableData(vSegStrTable);

    for(auto SegStrTabTmp: vSegStrTable){
        memcpy(pSegStrTable1, &SegStrTabTmp, sizeof(SegStrTabTmp));

        if((pSegStrTable1->Next=(SEG_STRUCT_TABLE_STRUCT *)
            calloc(1, sizeof(SEG_STRUCT_TABLE_STRUCT)))==NULL )
            return EDI_READ_BASE_ERR ;

        pSegStrTable2 = pSegStrTable1;
        pSegStrTable1 = pSegStrTable2->Next;
    }
    
    if(pSegStrTable1 == pSegStrTable2)
        pTab->SegStrTable = NULL;
    else
        pSegStrTable2->Next = NULL;
    free(pSegStrTable1);

    return EDI_READ_BASE_OK;
}

/*************************************************************************/
int ReadCompFileTable(EDI_TABLES_STRUCT *pTab,FILE *fp_comp)
{
    if(insert_to_tab_from_comp_file(pTab,fp_comp)<=0){
        return EDI_READ_BASE_ERR;
    }
    return EDI_READ_BASE_OK;
}

void ReadCompStrTableData(std::vector<COMP_STRUCT_TABLE_STRUCT> &vCompStrTable)
{
    edilib::EdilibDbCallbacks::instance()->readCompStrTableData(vCompStrTable);
}


int ReadCompStrTable(EDI_TABLES_STRUCT *pTab)
{
    COMP_STRUCT_TABLE_STRUCT *pCompStrTable1,*pCompStrTable2;

    if((pTab->CompStrTable=(COMP_STRUCT_TABLE_STRUCT *)
        calloc(1, sizeof(COMP_STRUCT_TABLE_STRUCT)))==NULL )
        return EDI_READ_BASE_ERR ;

    pCompStrTable1 = pCompStrTable2 = pTab->CompStrTable;
    std::vector<COMP_STRUCT_TABLE_STRUCT> vCompStrTable;
    ReadCompStrTableData(vCompStrTable);

    for(const auto &CompStr: vCompStrTable) {
        memcpy(pCompStrTable1, &CompStr, sizeof(COMP_STRUCT_TABLE_STRUCT));

        if((pCompStrTable1->Next=(COMP_STRUCT_TABLE_STRUCT *)
            calloc(1, sizeof(COMP_STRUCT_TABLE_STRUCT)))==NULL )
            return EDI_READ_BASE_ERR ;

        pCompStrTable2 = pCompStrTable1;
        pCompStrTable1 = pCompStrTable2->Next;
    }

    if(pCompStrTable1 == pCompStrTable2)
        pTab->CompStrTable = NULL;
    else
        pCompStrTable2->Next = NULL;
    free(pCompStrTable1);

    return EDI_READ_BASE_OK;
}

/******************************************************************/
int ReadDataFileTable(EDI_TABLES_STRUCT *pTab,FILE *fp_data)
{
    if(insert_to_tab_from_data_file(pTab,fp_data)<=0){
        return EDI_READ_BASE_ERR;
    }
    return EDI_READ_BASE_OK;
}

void ReadDataElemTableData(std::vector<DATA_ELEM_TABLE_STRUCT> &vDataTable)
{
    edilib::EdilibDbCallbacks::instance()->readDataElemTableData(vDataTable);
}

int ReadDataTable(EDI_TABLES_STRUCT *pTab)
{
    DATA_ELEM_TABLE_STRUCT *pDataElemTable1,*pDataElemTable2;
    
    if((pTab->DataTable=(DATA_ELEM_TABLE_STRUCT *)
        calloc(1, sizeof(DATA_ELEM_TABLE_STRUCT)))==NULL )
        return EDI_READ_BASE_ERR ;

    pDataElemTable1 = pDataElemTable2 = pTab->DataTable;
    std::vector<DATA_ELEM_TABLE_STRUCT> vDataTable;
    ReadDataElemTableData(vDataTable);

    for(const auto &DataElem: vDataTable) {
        memcpy(pDataElemTable1, &DataElem, sizeof(DataElem));

        if((pDataElemTable1->Next=(DATA_ELEM_TABLE_STRUCT *)
            calloc(1, sizeof(DATA_ELEM_TABLE_STRUCT)))==NULL )
            return EDI_READ_BASE_ERR ;

        pDataElemTable2 = pDataElemTable1;
        pDataElemTable1 = pDataElemTable2->Next;
    }

    if(pDataElemTable1 == pDataElemTable2)
        pTab->DataTable = NULL;
    else
        pDataElemTable2->Next = NULL;
    free(pDataElemTable1);

    return EDI_READ_BASE_OK;
}

/**********************************************************************/
void ReadSegTableData(std::vector<SEGMENT_TABLE_STRUCT> &vSegTable)
{
    edilib::EdilibDbCallbacks::instance()->readSegTableData(vSegTable);
}

int ReadSegTable(EDI_TABLES_STRUCT *pTab)
{
    SEGMENT_TABLE_STRUCT *pSegmentTable1, *pSegmentTable2;

    if((pTab->SegTable=(SEGMENT_TABLE_STRUCT *)
        calloc(1, sizeof(SEGMENT_TABLE_STRUCT)))==NULL )
        return EDI_READ_BASE_ERR ;

    pSegmentTable1 = pSegmentTable2 = pTab->SegTable;
    std::vector<SEGMENT_TABLE_STRUCT> vSegTable;
    ReadSegTableData(vSegTable);

    for(const auto &seg: vSegTable) {
        memcpy(pSegmentTable1, &seg, sizeof(seg));

        if((pSegmentTable1->Next=(SEGMENT_TABLE_STRUCT *)
            calloc(1, sizeof(SEGMENT_TABLE_STRUCT)))==NULL )
            return EDI_READ_BASE_ERR ;

        pSegmentTable2 = pSegmentTable1;
        pSegmentTable1 = pSegmentTable2->Next;
    }

    if(pSegmentTable1 == pSegmentTable2)
        pTab->SegTable = NULL;
    else
        pSegmentTable2->Next = NULL;
    free(pSegmentTable1);

    return EDI_READ_BASE_OK;
}

/*********************************************************/
void ReadCompTableData(std::vector<COMPOSITE_TABLE_STRUCT> &vCompTable)
{
    edilib::EdilibDbCallbacks::instance()->readCompTableData(vCompTable);
}

int ReadCompTable(EDI_TABLES_STRUCT *pTab)
{
    COMPOSITE_TABLE_STRUCT *pCompTable1,*pCompTable2;

    if((pTab->CompTable=(COMPOSITE_TABLE_STRUCT *)
        calloc(1, sizeof(COMPOSITE_TABLE_STRUCT)))==NULL )
        return EDI_READ_BASE_ERR ;

    pCompTable1 = pCompTable2 = pTab->CompTable;

    std::vector<COMPOSITE_TABLE_STRUCT> vCompTable;
    ReadCompTableData(vCompTable);

    for(const auto &Comp: vCompTable) {
        memcpy(pCompTable1, &Comp, sizeof(Comp));

        if((pCompTable1->Next=(COMPOSITE_TABLE_STRUCT *)
            calloc(1, sizeof(COMPOSITE_TABLE_STRUCT)))==NULL )
            return EDI_READ_BASE_ERR ;

        pCompTable2 = pCompTable1;
        pCompTable1 = pCompTable2->Next;
    }

    if(pCompTable1 == pCompTable2)
        pTab->CompTable = NULL;
    else
        pCompTable2->Next = NULL;
    free(pCompTable1);

    return EDI_READ_BASE_OK;
}
