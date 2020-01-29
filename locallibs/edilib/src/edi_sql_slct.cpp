#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include "edi_test.h"

#include "edi_sql_insert.h"
#include "edi_all.h"
#include "edi_user_func.h"
#include "edi_sql_func.h"
#include "edilib_db_callbacks.h"

/******************************************************/
/*        Очищает базу данных сообщений               */
/* return 0 - SQL ERROR                               */
/* return 1 - Ok                                      */
/******************************************************/
int DeleteDBMesseges()
{
    edilib::EdilibDbCallbacks::instance()->clearMesTableData();
    return 1;
}

int insert_to_tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab)
{
    int Ret;

    switch(pCommStr->flag){
    /*****************************************************/
    /*Для файла MES_FILE*/
    case 0: /*Message text*/
        Ret = InsertMess2Tab(pCommStr, pTab);
        if(!Ret) return Ret;
        break;
    case 1: /* описание сегмента        */
    case 2: /* описание сегментн группы */
        Ret = InsertMesStr2Tab(pCommStr,pTab);
        if(!Ret) return Ret;
        break;
        /*****************************************************/
        /*Для файла SEG_FILE*/
    case 3:
        Ret = InsertSegment2Tab(pCommStr,pTab);
        if(!Ret) return Ret;
        break;
    case 4:
    case 5:
        Ret = InsertSegmentStr2Tab(pCommStr, pTab);
        if(!Ret) return Ret;
        break;
        /*****************************************************/
        /*Для файла COMP_FILE*/
    case 6:
        Ret = InsertComposite2Tab(pCommStr, pTab);
        if(!Ret) return Ret;
        break;
    case 7:
        Ret = InsertCompositeStr2Tab(pCommStr,pTab);
        if(!Ret) return Ret;
        break;
        /*****************************************************/
        /*Для файла DATA_FILE*/
    case 8:
        Ret = InsertDataElement2Tab(pCommStr,pTab);
        if(!Ret) return Ret;
        break;
    }

    return 1;
}

int insert_to_sql(Command_Struct *pCommStr)
{
    int Ret;

    switch(pCommStr->flag){
    /*****************************************************/
    /*Для файла MES_FILE*/
    case 0: /*Message text*/
        Ret = InsertMess(pCommStr);
        if(!Ret) return Ret;
        break;
    case 1: /* описание сегмента        */
    case 2: /* описание сегментн группы */
        Ret = InsertMesStr(pCommStr);
        if(!Ret) return Ret;
        break;
        /*****************************************************/
        /*Для файла SEG_FILE*/
    case 3:
        Ret = InsertSegment(pCommStr);
        if(!Ret) return Ret;
        break;
    case 4:
    case 5:
        Ret = InsertSegmentStr(pCommStr);
        if(!Ret) return Ret;
        break;
        /*****************************************************/
        /*Для файла COMP_FILE*/
    case 6:
        Ret = InsertComposite(pCommStr);
        if(!Ret) return Ret;
        break;
    case 7:
        Ret = InsertCompositeStr(pCommStr);
        if(!Ret) return Ret;
        break;
        /*****************************************************/
        /*Для файла DATA_FILE*/
    case 8:
        Ret = InsertDataElement(pCommStr);
        if(!Ret) return Ret;
        break;
    }
    return 1;
}

int InsertMess(Command_Struct *pCommStr)
{
    edilib::EdilibDbCallbacks::instance()->insertMesTableData(pCommStr);
    return 1;
}

int InsertMess2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab)
{
    MESSAGE_TABLE_STRUCT * pMesTab;

    if(pTab->MesTable == NULL){
        pTab->MesTable = (MESSAGE_TABLE_STRUCT*)calloc(1,sizeof(MESSAGE_TABLE_STRUCT));
        if(pTab->MesTable == NULL){
            EdiError(EDILOG,"Memory error");
            return 0;
        }
        pMesTab=pTab->MesTable;
        pTab->MesTable->Prev=pMesTab;
    } else {
        pTab->MesTable->Prev->Next=(MESSAGE_TABLE_STRUCT*)calloc(1,sizeof(MESSAGE_TABLE_STRUCT));
        if(pTab->MesTable->Prev->Next == NULL){
            EdiError(EDILOG,"Memory error");
            return 0;
        }
        pMesTab      =pTab->MesTable->Prev->Next;
        pMesTab->Prev=pTab->MesTable->Prev;
        pTab->MesTable->Prev=pMesTab;
        pMesTab->Prev->Next=pMesTab;
    }

    if(pCommStr->tlen) {
        strcpy(pMesTab->Message, pCommStr->Text);
    } else {
        EdiError(EDILOG,"Text for %s is null", pCommStr->Command[0]);
        return 0;
    }

    strcpy(pMesTab->Message, pCommStr->Command[0]);

    return 1;
}

int InsertMesStr(Command_Struct *pCommStr)
{
    edilib::EdilibDbCallbacks::instance()->insertMesStrTableData(pCommStr);
    return 1;
}

int InsertMesStr2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab)
{
    MES_STRUCT_TABLE_STRUCT * pMesSTab;

    if(pTab->MesStrTable == NULL){
        pTab->MesStrTable = (MES_STRUCT_TABLE_STRUCT*)calloc(1,sizeof(MES_STRUCT_TABLE_STRUCT));
        if(pTab->MesStrTable == NULL){
            EdiError(EDILOG,"Memory error");
            return 0;
        }
        pMesSTab=pTab->MesStrTable;
        pTab->MesStrTable->Prev=pMesSTab;
    } else {
        pTab->MesStrTable->Prev->Next=(MES_STRUCT_TABLE_STRUCT*)calloc(1,sizeof(MES_STRUCT_TABLE_STRUCT));
        if(pTab->MesStrTable->Prev->Next == NULL){
            EdiError(EDILOG,"Memory error");
            return 0;
        }
        pMesSTab=pTab->MesStrTable->Prev->Next;
        pMesSTab->Prev=pTab->MesStrTable->Prev;
        pTab->MesStrTable->Prev=pMesSTab;
        pMesSTab->Prev->Next=pMesSTab;
    }

    strcpy(pMesSTab->Message, pCommStr->Command[0]);
    pMesSTab->Pos=(int)strtol(pCommStr->Command[1],NULL,10);
    strcpy(pMesSTab->Tag,pCommStr->Command[2]);
    pMesSTab->MaxPos=(int)strtol(pCommStr->Command[3],NULL,10);
    strcpy(pMesSTab->S,pCommStr->Command[4]);
    pMesSTab->R=strtol(pCommStr->Command[5],NULL,10);
    pMesSTab->GrpNum = strtol(pCommStr->Command[6],NULL,10);

    return 1;
}

int InsertSegment(Command_Struct *pCommStr)
{
    edilib::EdilibDbCallbacks::instance()->insertSegTableData(pCommStr);
    return 1;
}

int InsertSegment2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab)
{
    SEGMENT_TABLE_STRUCT * pSegTab;

    if(pTab->SegTable == NULL){
        pTab->SegTable = (SEGMENT_TABLE_STRUCT*)calloc(1,sizeof(SEGMENT_TABLE_STRUCT));
        if(pTab->SegTable == NULL){
            EdiError(EDILOG,"Memory error");
            return 0;
        }
        pSegTab=pTab->SegTable;
        pTab->SegTable->Prev=pSegTab;
    } else {
        pTab->SegTable->Prev->Next=(SEGMENT_TABLE_STRUCT*)calloc(1,sizeof(SEGMENT_TABLE_STRUCT));
        if(pTab->SegTable->Prev->Next == NULL){
            EdiError(EDILOG,"Memory error");
            return 0;
        }
        pSegTab=pTab->SegTable->Prev->Next;
        pSegTab->Prev=pTab->SegTable->Prev;
        pTab->SegTable->Prev=pSegTab;
        pSegTab->Prev->Next=pSegTab;
    }

    if(pCommStr->tlen) {
        strcpy(pSegTab->Text, pCommStr->Text);
    } else {
        EdiError(EDILOG,"Text for %s is null",pCommStr->Command[0]);
        return 0;
    }

    strcpy(pSegTab->Tag, pCommStr->Command[0]);

    return 1;
}

int InsertSegmentStr(Command_Struct *pCommStr)
{
    edilib::EdilibDbCallbacks::instance()->insertSegStrTableData(pCommStr);
    return 1;
}

int InsertSegmentStr2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab)
{
    SEG_STRUCT_TABLE_STRUCT *pSegStrTab;

    if(pTab->SegStrTable == NULL){
        pTab->SegStrTable = (SEG_STRUCT_TABLE_STRUCT*)calloc(1,sizeof(SEG_STRUCT_TABLE_STRUCT));
        if(pTab->SegStrTable == NULL){
            EdiError(EDILOG,"Memory error");
            return 0;
        }
        pSegStrTab=pTab->SegStrTable;
        pTab->SegStrTable->Prev=pSegStrTab;
    } else {
        pTab->SegStrTable->Prev->Next=(SEG_STRUCT_TABLE_STRUCT*)calloc(1,sizeof(SEG_STRUCT_TABLE_STRUCT));
        if(pTab->SegStrTable->Prev->Next == NULL){
            EdiError(EDILOG,"Memory error");
            return 0;
        }
        pSegStrTab=pTab->SegStrTable->Prev->Next;
        pSegStrTab->Prev=pTab->SegStrTable->Prev;
        pTab->SegStrTable->Prev=pSegStrTab;
        pSegStrTab->Prev->Next=pSegStrTab;
    }

    strcpy(pSegStrTab->Tag, pCommStr->Command[0]);
    pSegStrTab->Pos=(int)strtol(pCommStr->Command[1],NULL,10);
    strcpy(pSegStrTab->S, pCommStr->Command[4]);
    pSegStrTab->R=(int)strtol(pCommStr->Command[5],NULL,10);
    strcpy(pSegStrTab->Composite, pCommStr->Command[2]);
    pSegStrTab->DataElem=(int)strtol(pCommStr->Command[3],NULL,10);

    return 1;
}

int InsertComposite(Command_Struct *pCommStr)
{
    edilib::EdilibDbCallbacks::instance()->insertCompTableData(pCommStr);
    return 1;
}

int InsertComposite2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab)
{
    COMPOSITE_TABLE_STRUCT * pCompTab;

    if(pTab->CompTable == NULL){
        pTab->CompTable = (COMPOSITE_TABLE_STRUCT*)calloc(1,sizeof(COMPOSITE_TABLE_STRUCT));
        if(pTab->CompTable == NULL){
            EdiError(EDILOG,"Memory error");
            return 0;
        }
        pCompTab=pTab->CompTable;
        pTab->CompTable->Prev=pCompTab;
    } else {
        pTab->CompTable->Prev->Next=(COMPOSITE_TABLE_STRUCT*)calloc(1,sizeof(COMPOSITE_TABLE_STRUCT));
        if(pTab->CompTable->Prev->Next == NULL){
            EdiError(EDILOG,"Memory error");
            return 0;
        }
        pCompTab=pTab->CompTable->Prev->Next;
        pCompTab->Prev=pTab->CompTable->Prev;
        pTab->CompTable->Prev=pCompTab;
        pCompTab->Prev->Next=pCompTab;
    }

    if(pCommStr->tlen) {
        strcpy(pCompTab->Text, pCommStr->Text);
    } else {
        EdiError(EDILOG,"Text for %s is null", pCommStr->Command[0]);
        return 0;
    }

    strcpy(pCompTab->Composite, pCommStr->Command[0]);
    return 1;
}

int InsertCompositeStr(Command_Struct *pCommStr)
{
    edilib::EdilibDbCallbacks::instance()->insertCompStrTableData(pCommStr);
    return 1;
}

int InsertCompositeStr2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab)
{
    COMP_STRUCT_TABLE_STRUCT *pCompSTab;

    if(pTab->CompStrTable == NULL){
        pTab->CompStrTable = (COMP_STRUCT_TABLE_STRUCT*)calloc(1,sizeof(COMP_STRUCT_TABLE_STRUCT));
        if(pTab->CompStrTable == NULL){
            EdiError(EDILOG,"Memory error");
            return 0;
        }
        pCompSTab=pTab->CompStrTable;
        pTab->CompStrTable->Prev=pCompSTab;
    } else {
        pTab->CompStrTable->Prev->Next=(COMP_STRUCT_TABLE_STRUCT*)calloc(1,sizeof(COMP_STRUCT_TABLE_STRUCT));
        if(pTab->CompStrTable->Prev->Next == NULL){
            EdiError(EDILOG,"Memory error");
            return 0;
        }
        pCompSTab=pTab->CompStrTable->Prev->Next;
        pCompSTab->Prev=pTab->CompStrTable->Prev;
        pTab->CompStrTable->Prev=pCompSTab;
        pCompSTab->Prev->Next=pCompSTab;
    }

    strcpy(pCompSTab->Composite, pCommStr->Command[0]);
    pCompSTab->Pos=(int)strtol(pCommStr->Command[1],NULL,10);
    strcpy(pCompSTab->S, pCommStr->Command[3]);
    pCompSTab->R=(int)strtol(pCommStr->Command[4],NULL,10);
    pCompSTab->DataElem=(int)strtol(pCommStr->Command[2],NULL,10);

    return 1;
}

int InsertDataElement(Command_Struct *pCommStr)
{
    edilib::EdilibDbCallbacks::instance()->insertDataElemTableData(pCommStr);
    return 1;
}

int InsertDataElement2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab)
{
    DATA_ELEM_TABLE_STRUCT *pDataTab;

    if(pTab->DataTable == NULL){
        pTab->DataTable = (DATA_ELEM_TABLE_STRUCT*)calloc(1,sizeof(DATA_ELEM_TABLE_STRUCT));
        if(pTab->DataTable == NULL){
            EdiError(EDILOG,"Memory error");
            return 0;
        }
        pDataTab=pTab->DataTable;
        pTab->DataTable->Prev=pDataTab;
    } else {
        pTab->DataTable->Prev->Next=(DATA_ELEM_TABLE_STRUCT*)calloc(1,sizeof(DATA_ELEM_TABLE_STRUCT));
        if(pTab->DataTable->Prev->Next == NULL){
            EdiError(EDILOG,"Memory error");
            return 0;
        }
        pDataTab=pTab->DataTable->Prev->Next;
        pDataTab->Prev=pTab->DataTable->Prev;
        pTab->DataTable->Prev=pDataTab;
        pDataTab->Prev->Next=pDataTab;
    }

    if(pCommStr->tlen){
        strcpy(pDataTab->Text, pCommStr->Text);
    } else {
        EdiError(EDILOG,"Text for %s is null",pCommStr->Command[0]);
        return 0;
    }

    pDataTab->DataElem=(int)strtol(pCommStr->Command[0],NULL,10);
    strcpy(pDataTab->Format, pCommStr->Command[1]);
    pDataTab->MinField=(int)strtol(pCommStr->Command[2],NULL,10);
    pDataTab->MaxField=(int)strtol(pCommStr->Command[3],NULL,10);

    return 1;
}
