#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <string.h>
#include <stdlib.h>

#include "edi_tables.h"
#include "edi_all.h"
#include "edi_func.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE

#include "edi_test.h"
/*#include "edi_user_func.h"*/

/****************************************************************************/
/* Проверяет правильность ссылок в считанных таблицах                       */
/* Устанавливает ссылки в структурах, содержащих инфу из таблиц             */
/* return EDI_READ_BASE_OK  - все щл                                        */
/* return EDI_READ_BASE_ERR - ошибка в ссылках                              */
/****************************************************************************/
int TestEdiTbl(EDI_TABLES_STRUCT *pTab)
{
    int Ret;
    MESSAGE_TABLE_STRUCT *MesTabTmp;

    MesTabTmp = pTab->MesTable;
    if(MesTabTmp == NULL) {
        EdiError(EDILOG,"pTab->MesTable == NULL\n");
        return EDI_READ_BASE_ERR;
    }
    while (MesTabTmp){
        Ret = TestMesStrTbl(MesTabTmp,pTab);
        if(Ret != EDI_READ_BASE_OK) {
            EdiError(EDILOG,"TestEdiTbl finished whith error!\n");
            return Ret;
        }
        MesTabTmp = MesTabTmp->Next;
    }
    DeleteSegTable(pTab,1);  /*Удаляем стр., содержащие описание сегментов */
    DeleteCompTable(pTab,1); /*и композитов. Они больше не нужны!*/
    /*flag == 1 - удаление без текста. Текст нам от них и нужен был!*/
    return EDI_READ_BASE_OK;
}

int TestMesStrTbl(MESSAGE_TABLE_STRUCT *MesTab, EDI_TABLES_STRUCT *pTab)
{
    int Ret;
    MES_STRUCT_TABLE_STRUCT   *MesStrTabTmp;

    MesStrTabTmp = pTab->MesStrTable;
    if(MesStrTabTmp == NULL) {
        EdiError(EDILOG,"pTab->MesStrTable == NULL\n");
        return EDI_READ_BASE_ERR;
    }
    if((MesStrTabTmp = GetMesStrByMes(MesTab->Message, pTab))==NULL) {
        EdiError(EDILOG,"Message %s in EDI_STR_MESSAGE is not found!\n",MesTab->Message);
        return EDI_READ_BASE_ERR;
    }
    MesTab->First = MesStrTabTmp;
    while( MesStrTabTmp )
    {
        if(memcmp(MesStrTabTmp->Message, MesTab->Message, MESSAGE_LEN+1) != 0)
            break;
        if(MesStrTabTmp->Tag[0] != '\0') {
            Ret = TestSegStrTbl(MesStrTabTmp, pTab);
            if(Ret != EDI_READ_BASE_OK)
                return Ret;
            const char *text = GetSegText(MesStrTabTmp->Tag,pTab);
            if( text == NULL) {
                TST();
                return EDI_READ_BASE_ERR;
            }
            strcpy(MesStrTabTmp->Text, text);
        }
        MesStrTabTmp = MesStrTabTmp->Next;
    }
    return EDI_READ_BASE_OK;
}

int TestSegStrTbl(MES_STRUCT_TABLE_STRUCT *MesStrTab, EDI_TABLES_STRUCT *pTab)
{
    int Ret;
    SEG_STRUCT_TABLE_STRUCT   *SegStrTabTmp;
    SegStrTabTmp = pTab->SegStrTable;
    if(SegStrTabTmp == NULL) {
        EdiError(EDILOG,"pTab->SegStrTable == NULL\n");
        return EDI_READ_BASE_ERR;
    }
    if((SegStrTabTmp = GetSegStrByTag(MesStrTab->Tag,pTab))==NULL){
        EdiError(EDILOG,"Segment %s in EDI_STR_SEGMENT is not found!\n",MesStrTab->Tag);
        return EDI_READ_BASE_ERR;
    }
    MesStrTab->First = SegStrTabTmp;
    while( SegStrTabTmp )
    {
        if(memcmp( SegStrTabTmp->Tag,MesStrTab->Tag,TAG_LEN+1 ) != 0) break;
        if( SegStrTabTmp->Composite[0] != '\0' ){
            Ret = TestCompStrTbl(SegStrTabTmp,pTab);
            if(Ret != EDI_READ_BASE_OK) {
                TST();
                return Ret;
            }

            const char *text = GetCompText(SegStrTabTmp->Composite, pTab);
            if(text == NULL) {
                TST();
                return EDI_READ_BASE_ERR;
            }
            strcpy(SegStrTabTmp->Text, text);
        }
        else {
            Ret = TestDataStrTbl(&(SegStrTabTmp->FirstElem),SegStrTabTmp->DataElem,pTab);
            if(Ret != EDI_READ_BASE_OK){
                TST();
                return Ret;
            }
        }
        SegStrTabTmp = SegStrTabTmp->Next;
    }
    return EDI_READ_BASE_OK;
}

int TestCompStrTbl(SEG_STRUCT_TABLE_STRUCT *SegStrTab, 
                   EDI_TABLES_STRUCT *pTab )
{
    int Ret;
    COMP_STRUCT_TABLE_STRUCT *CompStrTabTmp;
    CompStrTabTmp = pTab->CompStrTable;
    if(CompStrTabTmp == NULL) {
        EdiError(EDILOG,"EDI_STR_COMPOSITE == NULL\n");
        return EDI_READ_BASE_ERR;
    }
    if((CompStrTabTmp = GetCompStrByComp(SegStrTab->Composite,pTab))==NULL){
        EdiError(EDILOG,"Composite %s in EDI_STR_COMPOSITE is not found!\n",SegStrTab->Composite);
        return EDI_READ_BASE_ERR;
    }
    SegStrTab->FirstComp = CompStrTabTmp;
    while( CompStrTabTmp )
    {
        if(memcmp( CompStrTabTmp->Composite,SegStrTab->Composite,COMP_LEN+1 ) != 0)
            break;
        Ret = TestDataStrTbl(&(CompStrTabTmp->First),CompStrTabTmp->DataElem,pTab);
        if(Ret != EDI_READ_BASE_OK){
            TST();
            return Ret;
        }
        CompStrTabTmp = CompStrTabTmp->Next;
    }
    return EDI_READ_BASE_OK;
}

int TestDataStrTbl(DATA_ELEM_TABLE_STRUCT **Point,int DataElem,
                   EDI_TABLES_STRUCT *pTab )
{
    /* int Ret;*/
    DATA_ELEM_TABLE_STRUCT *DataElemTabTmp;
    DataElemTabTmp = pTab->DataTable;
    if( DataElemTabTmp == NULL ) {
        EdiError(EDILOG,"EDI_DATAELEMENT table == NULL!\n");
        return EDI_READ_BASE_ERR;
    }
    if((DataElemTabTmp = GetDataElem(DataElem,pTab))==NULL){
        EdiError(EDILOG,"DataElem %d in EDI_DATAELEMENT is not found!\n",DataElem);
        return EDI_READ_BASE_ERR;
    }

    *Point = DataElemTabTmp;
    return EDI_READ_BASE_OK;
}

MES_STRUCT_TABLE_STRUCT *GetMesStrByMes(const char *Mes, EDI_TABLES_STRUCT *pTab)
{
    MES_STRUCT_TABLE_STRUCT   *MesStrTabTmp;
    MesStrTabTmp = pTab->MesStrTable;
    while(MesStrTabTmp)
    {
        if( memcmp(MesStrTabTmp->Message,Mes,MESSAGE_LEN+1) == 0 )
            return MesStrTabTmp;
        MesStrTabTmp = MesStrTabTmp->Next;
    }
    return NULL;
}

SEG_STRUCT_TABLE_STRUCT *GetSegStrByTag(const char *Tag, EDI_TABLES_STRUCT *pTab)
{
    SEG_STRUCT_TABLE_STRUCT   *SegStrTabTmp;
    SegStrTabTmp = pTab->SegStrTable;
    while(SegStrTabTmp)
    {
        if( memcmp( SegStrTabTmp->Tag,Tag,TAG_LEN+1 ) == 0 )
            return SegStrTabTmp;
        SegStrTabTmp = SegStrTabTmp->Next;
    }
    return NULL;
}

COMP_STRUCT_TABLE_STRUCT *GetCompStrByComp(const char *Composite,
                                           EDI_TABLES_STRUCT *pTab)
{
    COMP_STRUCT_TABLE_STRUCT *CompStrTabTmp;
    CompStrTabTmp = pTab->CompStrTable;
    while( CompStrTabTmp )
    {
        if( memcmp( CompStrTabTmp->Composite,Composite,COMP_LEN+1 ) == 0 )
            return CompStrTabTmp;
        CompStrTabTmp = CompStrTabTmp->Next;
    }
    return NULL;
}

DATA_ELEM_TABLE_STRUCT *GetDataElem(int DataElem, EDI_TABLES_STRUCT *pTab)
{
    DATA_ELEM_TABLE_STRUCT *DataElemTabTmp;
    DataElemTabTmp = pTab->DataTable;
    while( DataElemTabTmp )
    {
        if( DataElem == DataElemTabTmp->DataElem)
            return DataElemTabTmp;
        DataElemTabTmp = DataElemTabTmp->Next;
    }
    return NULL;
}

const char *GetSegText(const char *Tag, const EDI_TABLES_STRUCT *pTab)
{
    const SEGMENT_TABLE_STRUCT *SegTabTmp;
    SegTabTmp = pTab->SegTable;
    while( SegTabTmp )
    {
        if( memcmp( SegTabTmp->Tag, Tag, TAG_LEN + 1 ) == 0 )
            return SegTabTmp->Text;
        SegTabTmp = SegTabTmp->Next;
    }
    EdiError(EDILOG,"Text for tag %s is not found!\n", Tag);
    return NULL;
}

const char *GetCompText(const char *Comp, EDI_TABLES_STRUCT *pTab)
{
    COMPOSITE_TABLE_STRUCT *CompTabTmp;
    CompTabTmp = pTab->CompTable;
    while( CompTabTmp )
    {
        if( memcmp( CompTabTmp->Composite, Comp, COMP_LEN+1) == 0)
            return CompTabTmp->Text;
        CompTabTmp = CompTabTmp->Next;
    }
    EdiError(EDILOG,"Text for composite %s is not found!\n", Comp);
    return NULL;
}
