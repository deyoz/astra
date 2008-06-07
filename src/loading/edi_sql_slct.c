#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include "setup.h"
#include "test.h"
#include "ocilocal.h"

#ifdef _EDI_MES_NO_SIR_
#include "edi_sql_insert.h"
#include "airimp/edi_all.h"
#include "airimp/edi_user_func.h"
#include "edi_sql_func.h"


/******************************************************/
/*        Очищает базу данных сообщений               */
/* return 0 - SQL ERROR                               */
/* return 1 - Ok                                      */
/******************************************************/
int DeleteDBMesseges()
{
    const char *OciStr=
	"begin\n"
	"delete from edi_messages;\n"
	"delete from edi_str_message;\n"
	"delete from edi_segment;\n"
	"delete from edi_str_segment;\n"
        "delete from edi_composite;\n"
	"delete from edi_str_composite;\n"
	"delete from edi_data_elem;\n"
	"end;";


    if ( Oparse( CU, OciStr) ||
	 Oexec(CU) ){
	ProgError( EDILOG,"Error of cleaning EDI_DATA tables ");
	oci_error(CU);
	return 0;
    }
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
    const char *Text =
        "INSERT INTO EDI_MESSAGES(MESSAGE, TEXT) "
	"VALUES (:M,:T)";
 if ( Oparse( CU, Text) ||
      Obndrs( CU,":M", pCommStr->Command[0]   ) ||
      Obndrs( CU,":T", pCommStr->Text         ) ||
      Oexec(  CU                              ) )
 {
   if(cda_err( CU ) == SQL_DUBLICATE){
      ProgTrace( EDITRACE,"Dublicate %s in EDI_MESSAGES", pCommStr->Command[0]);
      return 1;
   }
   ProgError( EDILOG,
              "Error: INSERT INTO EDI_MESSAGES (<%s>,<%s>)",
               pCommStr->Command[0], pCommStr->Text);
   oci_error(CU);
   return 0;
 }
 return 1;
}

int InsertMess2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab)
{
    MESSAGE_TABLE_STRUCT * pMesTab;

    if(pTab->MesTable == NULL){
	pTab->MesTable = (MESSAGE_TABLE_STRUCT*)calloc(1,sizeof(MESSAGE_TABLE_STRUCT));
	if(pTab->MesTable == NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
        pMesTab=pTab->MesTable;
    } else {
	pTab->MesTable->Next=(MESSAGE_TABLE_STRUCT*)calloc(1,sizeof(MESSAGE_TABLE_STRUCT));
        if(pTab->MesTable->Next == NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
        pMesTab=pTab->MesTable->Next;
    }

    if(pCommStr->tlen){
	pMesTab->Text = (char *)malloc(pCommStr->tlen+1);
	if(pMesTab->Text==NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
	memcpy(pMesTab->Text, pCommStr->Text, pCommStr->tlen);
        pMesTab->Text[pCommStr->tlen]='\0';
    }

    strcpy(pMesTab->Message, pCommStr->Command[0]);

    return 1;
}

int InsertMesStr(Command_Struct *pCommStr)
{
    const char *Text =
        "INSERT INTO EDI_STR_MESSAGE"
	"(MESSAGE, POS, TAG, MAXPOS, S, R) "
	"VALUES (:M,TO_NUMBER(:P),:T,TO_NUMBER(:MP),:S,TO_NUMBER(:R))";

 if ( Oparse( CU, Text) ||
      Obndrs( CU,":M" , pCommStr->Command[0]   ) ||
      Obndrs( CU,":P" , pCommStr->Command[1]   ) ||
      Obndrs( CU,":T" , pCommStr->Command[2]   ) ||
      Obndrs( CU,":MP", pCommStr->Command[3]   ) ||
      Obndrs( CU,":S" , pCommStr->Command[4]   ) ||
      Obndrs( CU,":R" , pCommStr->Command[5]   ) ||
      Oexec(  CU                               ) )
 {
   if(cda_err( CU ) == SQL_DUBLICATE){
     ProgTrace( EDITRACE,"Dublicate %s -> %s in EDI_STR_MESSAGE",
                pCommStr->Command[0], pCommStr->Command[2]);
     return 1;
   }
   ProgError( EDILOG,
              "Error: INSERT INTO EDI_STR_MESSAGE "
              "(<%s>,<%s>,<%s>,<%s>,<%s>,<%s>)",
               pCommStr->Command[0], pCommStr->Command[1],pCommStr->Command[2],
               pCommStr->Command[3], pCommStr->Command[4],pCommStr->Command[5]);
   oci_error(CU);
   return 0;
 }
 return 1;
}

int InsertMesStr2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab)
{
    MES_STRUCT_TABLE_STRUCT * pMesSTab;

    if(pTab->MesStrTable == NULL){
	pTab->MesStrTable = (MES_STRUCT_TABLE_STRUCT*)calloc(1,sizeof(MES_STRUCT_TABLE_STRUCT));
	if(pTab->MesStrTable == NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
        pMesSTab=pTab->MesStrTable;
    } else {
	pTab->MesStrTable->Next=(MES_STRUCT_TABLE_STRUCT*)calloc(1,sizeof(MES_STRUCT_TABLE_STRUCT));
        if(pTab->MesStrTable->Next == NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
        pMesSTab=pTab->MesStrTable->Next;
    }

    strcpy(pMesSTab->Message, pCommStr->Command[0]);
    pMesSTab->Pos=(int)strtol(pCommStr->Command[1],NULL,10);
    strcpy(pMesSTab->Tag,pCommStr->Command[2]);
    pMesSTab->MaxPos=(int)strtol(pCommStr->Command[3],NULL,10);
    strcpy(pMesSTab->S,pCommStr->Command[4]);
    pMesSTab->R=strtol(pCommStr->Command[5],NULL,10);

    return 1;
}

int InsertSegment(Command_Struct *pCommStr)
{
    const char *Text =
        "INSERT INTO EDI_SEGMENT (TAG, TEXT) "
	"VALUES (:T, :Txt)";
 if ( Oparse( CU, Text) ||
      Obndrs( CU,":T"  , pCommStr->Command[0]   ) ||
      Obndrs( CU,":Txt", pCommStr->Text         ) ||
      Oexec(  CU                              ) )
 {
   if(cda_err( CU ) == SQL_DUBLICATE){
      ProgTrace( EDITRACE,"Dublicate %s in EDI_SEGMENT", pCommStr->Command[0]);
      return 1;
   }
   ProgError( EDILOG,
              "Error: INSERT INTO EDI_SEGMENT (<%s>,<%s>)",
               pCommStr->Command[0], pCommStr->Text);
   oci_error(CU);
   return 0;
 }
 return 1;
}

int InsertSegment2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab)
{
    SEGMENT_TABLE_STRUCT * pSegTab;

    if(pTab->SegTable == NULL){
	pTab->SegTable = (SEGMENT_TABLE_STRUCT*)calloc(1,sizeof(SEGMENT_TABLE_STRUCT));
	if(pTab->SegTable == NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
        pSegTab=pTab->SegTable;
    } else {
	pTab->SegTable->Next=(SEGMENT_TABLE_STRUCT*)calloc(1,sizeof(SEGMENT_TABLE_STRUCT));
        if(pTab->SegTable->Next == NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
        pSegTab=pTab->SegTable->Next;
    }

    if(pCommStr->tlen){
	pSegTab->Text=malloc(pCommStr->tlen+1);
	if(pSegTab->Text==NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
	memcpy(pSegTab->Text,pCommStr->Text,pCommStr->tlen);
	pSegTab->Text[pCommStr->tlen]='\0';
        pSegTab->TextOwner=1;
    }

    strcpy(pSegTab->Tag, pCommStr->Command[0]);

    return 1;
}


int InsertSegmentStr(Command_Struct *pCommStr)
{
    const char *Text =
        "INSERT INTO EDI_STR_SEGMENT"
	"(TAG, POS, S, R, COMPOSITE, DATAELEMENT) "
	"VALUES (:TAG, TO_NUMBER(:POS), :S, TO_NUMBER(:R), "
	":COMP, TO_NUMBER(:DE))";
 if ( Oparse( CU, Text) ||
      Obndrs( CU,":TAG"  , pCommStr->Command[0]   ) ||
      Obndrs( CU,":POS"  , pCommStr->Command[1]   ) ||
      Obndrs( CU,":S"    , pCommStr->Command[4]   ) ||
      Obndrs( CU,":R"    , pCommStr->Command[5]   ) ||
      Obndrs( CU,":COMP" , pCommStr->Command[2]   ) ||
      Obndrs( CU,":DE"   , pCommStr->Command[3]   ) ||
      Oexec(  CU                               ) )
 {
   if(cda_err( CU ) == SQL_DUBLICATE){
      ProgTrace( EDITRACE,"Dublicate %s -> %s in EDI_STR_SEGMENT",
      pCommStr->Command[0], pCommStr->Command[2]);
      return 1;
   }
   ProgError( EDILOG,
              "Error: INSERT INTO EDI_STR_SEGMENT "
              "(<%s>,<%s>,<%s>,<%s>,<%s>,<%s>)",
               pCommStr->Command[0], pCommStr->Command[1],pCommStr->Command[4],
               pCommStr->Command[5], pCommStr->Command[2],pCommStr->Command[3]);
   oci_error(CU);
   return 0;
 }
 return 1;
}

int InsertSegmentStr2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab)
{
    SEG_STRUCT_TABLE_STRUCT *pSegStrTab;

    if(pTab->SegStrTable == NULL){
	pTab->SegStrTable = (SEG_STRUCT_TABLE_STRUCT*)calloc(1,sizeof(SEG_STRUCT_TABLE_STRUCT));
	if(pTab->SegStrTable == NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
        pSegStrTab=pTab->SegStrTable;
    } else {
	pTab->SegStrTable->Next=(SEG_STRUCT_TABLE_STRUCT*)calloc(1,sizeof(SEG_STRUCT_TABLE_STRUCT));
        if(pTab->SegStrTable->Next == NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
        pSegStrTab=pTab->SegStrTable->Next;
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
    const char *Text=
	"INSERT INTO EDI_COMPOSITE"
	"(COMPOSITE, TEXT) VALUES (:COMP, :TXT)";

 if(Oparse( CU, Text) ||
    Obndrs( CU,":COMP"  , pCommStr->Command[0]   ) ||
    Obndrs( CU,":TXT"   , pCommStr->Text         ) ||
    Oexec(  CU                                   ) )
 {
   if(cda_err( CU ) == SQL_DUBLICATE){
       ProgTrace( EDITRACE,"Dublicate %s in EDI_COMPOSITE", pCommStr->Command[0]);
       return 1;
   }
   ProgError( EDILOG,
                    "Error: INSERT INTO EDI_COMPOSITE"
                    "(<%s>,<%s>)",
                    pCommStr->Command[0], pCommStr->Text);
    oci_error(CU);
    return 0;
 }
 return 1;
}

int InsertComposite2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab)
{
    COMPOSITE_TABLE_STRUCT * pCompTab;

    if(pTab->CompTable == NULL){
	pTab->CompTable = (COMPOSITE_TABLE_STRUCT*)calloc(1,sizeof(COMPOSITE_TABLE_STRUCT));
	if(pTab->CompTable == NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
        pCompTab=pTab->CompTable;
    } else {
	pTab->CompTable->Next=(COMPOSITE_TABLE_STRUCT*)calloc(1,sizeof(COMPOSITE_TABLE_STRUCT));
        if(pTab->CompTable->Next == NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
        pCompTab=pTab->CompTable->Next;
    }

    if(pCommStr->tlen){
	pCompTab->Text=malloc(pCommStr->tlen+1);
	if(pCompTab->Text==NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
	memcpy(pCompTab->Text,pCommStr->Text,pCommStr->tlen);
	pCompTab->Text[pCommStr->tlen]='\0';
        pCompTab->TextOwner=1;
    }

    strcpy(pCompTab->Composite, pCommStr->Command[0]);

    return 1;
}

int InsertCompositeStr(Command_Struct *pCommStr)
{
    const char *Text=
	"INSERT INTO EDI_STR_COMPOSITE"
	"(COMPOSITE, POS, S, R, DATAELEMENT) "
	"VALUES (:COMP, TO_NUMBER( :POS ), :S, "
	"TO_NUMBER( :R ), TO_NUMBER( :DE ) )";

    if ( Oparse( CU, Text) ||
      Obndrs( CU,":COMP" , pCommStr->Command[0]   ) ||
      Obndrs( CU,":POS"  , pCommStr->Command[1]   ) ||
      Obndrs( CU,":S"    , pCommStr->Command[3]   ) ||
      Obndrs( CU,":R"    , pCommStr->Command[4]   ) ||
      Obndrs( CU,":DE"   , pCommStr->Command[2]   ) ||
      Oexec(  CU                               ) )
 {
   if(cda_err( CU ) == SQL_DUBLICATE){
      ProgTrace( EDITRACE,"Dublicate %s -> %s in EDI_STR_COMPOSITE",
         pCommStr->Command[0], pCommStr->Command[2]);
      return 1;
   }
   ProgError( EDILOG,
              "Error: INSERT INTO EDI_STR_COMPOSITE "
              "(<%s>,<%s>,<%s>,<%s>,<%s>)",
               pCommStr->Command[0], pCommStr->Command[1],pCommStr->Command[3],
               pCommStr->Command[4], pCommStr->Command[2]);
   oci_error(CU);
   return 0;
 }
 return 1;
}

int InsertCompositeStr2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab)
{
    COMP_STRUCT_TABLE_STRUCT *pCompSTab;

    if(pTab->CompStrTable == NULL){
	pTab->CompStrTable = (COMP_STRUCT_TABLE_STRUCT*)calloc(1,sizeof(COMP_STRUCT_TABLE_STRUCT));
	if(pTab->CompStrTable == NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
        pCompSTab=pTab->CompStrTable;
    } else {
	pTab->CompStrTable->Next=(COMP_STRUCT_TABLE_STRUCT*)calloc(1,sizeof(COMP_STRUCT_TABLE_STRUCT));
        if(pTab->CompStrTable->Next == NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
        pCompSTab=pTab->CompStrTable->Next;
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
    const char *Text=
        "INSERT INTO EDI_DATA_ELEM"
	"(DATAELEMENT, TEXT, FORMAT, MINFIELD, MAXFIELD) "
	"VALUES (TO_NUMBER(:DE), :TXT, :F, TO_NUMBER(:MINF), TO_NUMBER(:MAXF))";
 if ( Oparse( CU, Text) ||
      Obndrs( CU,":DE"  , pCommStr->Command[0]   ) ||
      Obndrs( CU,":TXT" , pCommStr->Text         ) ||
      Obndrs( CU,":F"   , pCommStr->Command[1]   ) ||
      Obndrs( CU,":MINF", pCommStr->Command[2]   ) ||
      Obndrs( CU,":MAXF", pCommStr->Command[3]   ) ||
      Oexec(  CU                               ) )
 {
   if(cda_err( CU ) == SQL_DUBLICATE){
      ProgTrace( EDITRACE,"Dublicate %s in EDI_DATA_ELEM", pCommStr->Command[0]);
      return 1;
   }
   ProgError( EDILOG,
              "Error: INSERT INTO EDI_DATA_ELEM "
              "(<%s>,<%s>,<%s>,<%s>,<%s>)",
               pCommStr->Command[0], pCommStr->Text, pCommStr->Command[1],
               pCommStr->Command[2], pCommStr->Command[3]);
   oci_error(CU);
   return 0;
 }
 return 1;
}

int InsertDataElement2Tab(Command_Struct *pCommStr, EDI_TABLES_STRUCT *pTab)
{
    DATA_ELEM_TABLE_STRUCT *pDataTab;

    if(pTab->DataTable == NULL){
	pTab->DataTable = (DATA_ELEM_TABLE_STRUCT*)calloc(1,sizeof(DATA_ELEM_TABLE_STRUCT));
	if(pTab->DataTable == NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
        pDataTab=pTab->DataTable;
    } else {
	pTab->DataTable->Next=(DATA_ELEM_TABLE_STRUCT*)calloc(1,sizeof(DATA_ELEM_TABLE_STRUCT));
        if(pTab->DataTable->Next == NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
        pDataTab=pTab->DataTable->Next;
    }

    if(pCommStr->tlen){
        pDataTab->Text=malloc(pCommStr->tlen+1);
	if(pDataTab->Text==NULL){
            ProgError(STDLOG,"Memory error");
            return 0;
	}
	memcpy(pDataTab->Text,pCommStr->Text,pCommStr->tlen);
	pDataTab->Text[pCommStr->tlen]='\0';
        pDataTab->TextOwner=1;
    }

    pDataTab->DataElem=(int)strtol(pCommStr->Command[0],NULL,10);
    strcpy(pDataTab->Format, pCommStr->Command[1]);
    pDataTab->MinField=(int)strtol(pCommStr->Command[2],NULL,10);
    pDataTab->MaxField=(int)strtol(pCommStr->Command[3],NULL,10);

    return 1;
}


/*****************************************************************/
/* return 0     - ошибка !!!!                                    */
/* return 1     - Ok                                             */
/*****************************************************************/
int CommitLD()
{
 if(ocom(LD)) {
    ProgError(EDILOG, "Commit error");
    return 0;
 }
 return 1;
}

/*****************************************************************/
/* return 0     - ошибка !!!!                                    */
/* return 1     - Ok                                             */
/*****************************************************************/
int EdiSavePoint()
{
 if (Oparse (CU, "savepoint edi_before_insert") ||
     Oexec  (CU)) {
   oci_error (CU);
   ProgError(EDILOG,"SAVEPOINT ERROR!!!");
   return 0;
 }
 return 1;
}
/*****************************************************************/
/* return 0     - ошибка !!!!                                    */
/* return 1     - Ok                                             */
/*****************************************************************/
int EdiRollBack()
{
 if (Oparse (CU, "rollback to savepoint edi_before_insert")||
     Oexec  (CU)) {
    oci_error (CU);
    ProgError(EDILOG,"ROLLBACK ERROR!!!");
    return 0;
 }
 return 1;
}
#endif
