#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <serverlib/cursctl.h>
#include "edi_sql_insert.h"
#include "edi_sql_func.h"
#include "edi_user_func.h"
#include "edi_all.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include "edi_test.h"

unsigned int FileLine;

int insert_to_ora_from_dir(const char *dir)
{
    const int ret = insert_to_ora_from_dir_cur(dir);
    return ret;
}

int insert_to_ora_from_dir_cur(const char *dir)
{
    int ret = EDI_MES_ERR;
    EdiFilePoints Fp = {};

    if(open_edi_files(dir,&Fp)){
        return EDI_READ_BASE_ERR;
    }

    ret= insert_to_ora_from_files(Fp.mes, Fp.seg, Fp.comp, Fp.data);

    close_edi_files(&Fp);

    if(ret<=0) {
        rollback();
    }
    return ret;
}

int insert_to_ora_from_files(FILE *fp_mes,FILE *fp_seg,FILE *fp_comp,FILE *fp_data)
{
    int ret;
    Command_Struct CommStr, *pCommStr = &CommStr;

    if((ret=insert_to_from_file(fp_mes, insert_comm_str_message, pCommStr, NULL))<=0) {
        return ret;
    }

    if((ret=insert_to_from_file(fp_seg, insert_comm_str_segment, pCommStr, NULL))<=0) {
        return ret;
    }

    if((ret=insert_to_from_file(fp_comp, insert_comm_str_composite, pCommStr, NULL))<=0) {
        return ret;
    }

    if((ret=insert_to_from_file(fp_data, insert_comm_str_data_element, pCommStr, NULL))<=0) {
        return ret;
    }

    return 1;
}


int insert_to_tab_from_mes_file(EDI_TABLES_STRUCT *pTab,FILE *fpoint)
{
    int ret;
    Command_Struct CommStr, *pCommStr = &CommStr;

    if((ret=insert_to_from_file(fpoint, insert_comm_str_message, pCommStr, pTab))<=0) {
        return ret;
    }
    return 1;
}

int insert_to_tab_from_seg_file(EDI_TABLES_STRUCT *pTab,FILE *fpoint)
{
    int ret;
    Command_Struct CommStr, *pCommStr = &CommStr;

    if((ret=insert_to_from_file(fpoint, insert_comm_str_segment, pCommStr, pTab))<=0) {
        return ret;
    }
    return 1;
}

int insert_to_tab_from_comp_file(EDI_TABLES_STRUCT *pTab,FILE *fpoint)
{
    int ret;
    Command_Struct CommStr, *pCommStr = &CommStr;

    if((ret=insert_to_from_file(fpoint, insert_comm_str_composite, pCommStr, pTab))<=0) {
        return ret;
    }
    return 1;
}

int insert_to_tab_from_data_file(EDI_TABLES_STRUCT *pTab,FILE *fpoint)
{
    int ret;
    Command_Struct CommStr, *pCommStr = &CommStr;

    if((ret=insert_to_from_file(fpoint, insert_comm_str_data_element, pCommStr, pTab))<=0) {
        return ret;
    }

    return 1;
}


int insert_to_from_file(FILE *fpoint,
                        edi_insert_func ifunc, Command_Struct *pCommStr,
                        EDI_TABLES_STRUCT *pTab)
{
    char buffer[BUFFER_SIZE];
    char *bufftmp;
    int  Ret;

    FileLine=0;
    memset(pCommStr,0,sizeof(Command_Struct));

    while(fgets(buffer,BUFFER_SIZE,fpoint) != NULL) {
        FileLine ++;
        bufftmp = &buffer[0];
        Ret = ifunc(pCommStr,&bufftmp);
        if(Ret < 0)
            continue; /* Пропускаем пустую или закоммент. строку */
        if(!Ret) {
            return Ret;
        }
        if(pTab==0){
            Ret=insert_to_sql(pCommStr);
        }else {
            Ret=insert_to_tab(pCommStr,pTab);
        }
        if(!Ret)
            return Ret;
    }

    return 1;
}

/*********************************************************************/
/* Пропускает символы пробела и табуляции                            */
/* return 1 - все Ok  						     */
/*********************************************************************/
int skiptabs(char **buffer)
{
    while (( (*buffer)[0] == TABCHAR ) || ( (*buffer)[0] == ANYKEYCHAR) )
        (*buffer) ++;
    return 1;
}
/*********************************************************************/
/*	 Возвращает в word следущее слово			     */
/* return -1 - закомментированная или пропущ строка                  */
/* return  0 - конец строки	                                     */
/* return  1 - все Ok						     */
/*********************************************************************/
int get_next_word(char **buffer, char *word, int flag)
{
    int i=0;

    skiptabs(buffer);
    if (flag && ( ( (*buffer)[0] == COMMENTCHAR) ||
                  ( (*buffer)[0] == ENTERCHAR  ) ||
                  ( (*buffer)[0] == '\0'       ) ))
        return -1;
    else
        if(!flag && ( ( (*buffer)[0] == ENTERCHAR  ) ||
                      ( (*buffer)[0] == '\0'       ) ))
            return 0;

    while(( (*buffer)[0] != TABCHAR   ) && ( (*buffer)[0] != ANYKEYCHAR ) &&
          ( (*buffer)[0] != ENTERCHAR ) && ( (*buffer)[0] != '\0'       )  ) {
        word[i] = (*buffer)[0];
        i++;
        (*buffer) ++;
    }
    word[i] = '\0';
    return 1;
}

/*********************************************************************/
/*       Возвращает в text символы в кавычках                        */
/* return 0 - неудача                                                */
/* return 1 - все Ok                                                 */
/*********************************************************************/
int get_text(char **buffer, char *text)
{
    int i=0;
    skiptabs(buffer);
    if(( (*buffer)[0] == ENTERCHAR ) || ( (*buffer)[0] == '\0') ||
            ( (*buffer)[0] != TEXTCHAR  ) )
        return 0;
    (*buffer) ++;
    while((*buffer)[0] != TEXTCHAR){
        if(( (*buffer)[0] == ENTERCHAR ) || ( (*buffer)[0] == '\0'))
            return 0;
        text[i] = (*buffer)[0];
        (*buffer) ++;
        i++;
    }
    (*buffer) ++;
    text[i] = '\0';
    return 1;
}

static int isSegGrCommand(const char *word)
{
    if (strlen(word) >= sizeof(SGCOMMAND) + 1
            && !memcmp(word, SGCOMMAND, sizeof(SGCOMMAND) - 1)
            && word[strlen(word) - 1] == ':') {
        // формат с номером группы: Sg2:
        return 1;
    } else {
        return 0;
    }
}

static int GetSegGrSeqNum(const char *word)
{
    const size_t length = strlen(word);
    if(length <= sizeof SGCOMMAND) {
        return -1;
    } else {
        int num = 0;
        int nscanf = sscanf(word, "Sg%d:", &num);
        if(nscanf == 1) {
            return num;
        }
    }
    return -1;
}

/************************************************************************/
/* Записывает считанную строку в стр-ру                                 */
/* return 0 - неудача                                                   */
/* return 1 - Ok                                                        */
/************************************************************************/
int insert_comm_str_message(Command_Struct *pCommStr, char **bufftmp)
{
    int Ret;
    char word[TEXT_SIZE];
    char str[100];
    int len;

    Ret = get_next_word(bufftmp,word,1);
    if(Ret < 0) return Ret;
    if(!Ret)
    {
        error(MES_FILE_," ");
        return Ret;
    }
    memset(pCommStr->Command[1], 0, sizeof pCommStr->Command - sizeof pCommStr->Command[0]);
    if( !memcmp( word,MESCOMMAND,sizeof(MESCOMMAND) ) ){/*описание сообщения*/
        /*format : {Message: MesName "Text about this message"}*/
        pCommStr->flag = 0;
        Ret = get_next_word(bufftmp,word,0);
        if(!Ret) {
            error(MES_FILE_, "Message: MesName \"Text about this message\"");
            return Ret;
        }
        len=strlen(word);
        if(len>MESSAGE_LEN) {
            error(MES_FILE_, "Message length is too long");
            return 0;
        } else if(len<MESSAGE_LEN) {
            error(MES_FILE_, "Message length is too short");
            return 0;
        }
        memcpy(pCommStr->Command[0],word,len);
        pCommStr->Command[0][len]='\0';
        Ret = get_text(bufftmp,word);
        if(!Ret) {
            error(MES_FILE_,"Cant get text");
            return Ret;
        }
        len=strlen(word);
        memcpy(pCommStr->Text,word,len);
        pCommStr->Text[len]='\0';
        pCommStr->tlen=len;
    } else if(pCommStr->Command[0][0] == '\0') {
        error (MES_FILE_, "You should enter message name");
        return 0;/*сообщение не определено*/
    } else {
        /*******************************************************/
        if(isSegGrCommand(word)) {
            /*Описание сегм. гр.*/
            int segGrSeqNum = GetSegGrSeqNum(word);
            if(segGrSeqNum < 0) {
                error(MES_FILE_,"Invalid segment group sequence number");
                return 0;
            }

            int n = snprintf(pCommStr->Command[6], sizeof pCommStr->Command[6], "%d", segGrSeqNum);
            if(n < 0 || n >= 4 || size_t(n) >= sizeof pCommStr->Command[6]) {
                error(MES_FILE_,"Segment group number is too big");
                return 0;
            }

            Ret = get_next_word(bufftmp, word, 0);
            if(!Ret) {
                error (MES_FILE_,"Cant find position");
                return Ret;
            }
            pCommStr->flag = 2;/*Описание сегм. гр.*/
        }
        else
        {
            pCommStr->flag = 1;/*Описание сегм.*/
        }
        if(strlen(word) > POS_LEN){
            error(MES_FILE_,"Position number is too long");
            return 0;
        }
        if(!TestForDigit(word)){
            error(MES_FILE_,"Position must be digit");
            return 0;
        }
        strcpy(pCommStr->Command[1],word);              /* Pos */
        /*******************************************************/
        Ret = get_next_word(bufftmp,word,0);
        if(!Ret) {
            error(MES_FILE_,"TAG or MAXPOS is not found");
            return Ret;
        }
        if(pCommStr->flag == 1) {
            if(strlen(word) != TAG_LEN){
                snprintf(str, sizeof str,"Tag - %.70s : bad length",word);
                error(MES_FILE_,str);
                return 0;
            }
            strcpy(pCommStr->Command[2],word);/* TAG */
            pCommStr->Command[3][0] = '\0';
        } else {
            if (strlen(word) > POS_LEN){
                snprintf(str, sizeof str,"MAXPOS - %.70s : bad length",word);
                error(MES_FILE_,str);
                return 0;
            }
            if(!TestForDigit(word)){
                error(MES_FILE_,"MaxPosition must be digit");
                return 0;
            }
            strcpy(pCommStr->Command[3],word);/* MaxPos */
            pCommStr->Command[2][0] = '\0';
        }
        /* MaxPos or TAG */
        /***********************************************************/
        Ret = get_next_word(bufftmp,word,0);
        if(!Ret) {
            error(MES_FILE_,"C/M - not found");
            return Ret;
        }
        if( strlen(word) > 1 ){
            error(MES_FILE_,"C/M - bad length");
            return 0;
        }
        if((word[0] != MANDATORY) && (word[0] != CONDITIONAL))
        {
            snprintf(str, sizeof str,"C/M must be %c or %c",MANDATORY,CONDITIONAL);
            error (MES_FILE_,str);
            return 0;
        }
        strcpy(pCommStr->Command[4],word);    /* C/M */
        /***********************************************************/
        Ret = get_next_word(bufftmp,word,0);
        if(!Ret) {
            error(MES_FILE_,"R - not found");
            return Ret;
        }
        if(strlen(word) > MES_R) {
            error(MES_FILE_,"R - bad length");
            return 0;
        }
        if(!TestForDigit(word)){
            error(MES_FILE_,"R  must be digit");
            return 0;
        }
        strcpy(pCommStr->Command[5],word);    /* R */
        /***********************************************************/
    }

    return 1;
}

/************************************************************************/
/* Записывает считанную строку в стр-ру                                 */
/* return 0 - неудача                                                   */
/* return 1 - Ok                                                        */
/************************************************************************/
int insert_comm_str_segment(Command_Struct *pCommStr, char **bufftmp)
{
    int Ret;
    char word[TEXT_SIZE];
    char str[100];
    int len;

    Ret = get_next_word(bufftmp,word,1);
    if(Ret < 0) return Ret;
    if(!Ret){
        error(SEG_FILE_," ");
        return Ret;
    }
    if( !memcmp( word,SEGCOMMAND,sizeof(SEGCOMMAND) ) ){/*описание сегмента*/
        /*format : {Segment: SegName "Text about this segment"}*/
        pCommStr->flag = 3;
        Ret = get_next_word(bufftmp,word,0);
        if(!Ret) {
            error (SEG_FILE_, "Segment: SegName \"Text about this segment\"");
            return Ret;
        }
        len = strlen(word);
        if(len>TAG_LEN) {
            error(SEG_FILE_, "Tag length is too long");
            return 0;
        } else if(len<TAG_LEN) {
            error(SEG_FILE_, "Tag length is too short");
            return 0;
        }
        strcpy(pCommStr->Command[0],word);
        Ret = get_text(bufftmp,word);
        if(!Ret) {
            error(SEG_FILE_,"Cant get text");
            return Ret;
        }
        len = strlen(word);
        memcpy(pCommStr->Text,word,len);
        pCommStr->Text[len] = '\0';
        pCommStr->tlen=len;
    } else
        if(pCommStr->Command[0][0] == '\0'){
            error (SEG_FILE_, "You must entering segment name");
            return 0;/*сегмент не определен*/
        }else {
            /*******************************************************/
            if(!memcmp(word,DECOMMAND,sizeof(DECOMMAND))){/*Описание эл. данных*/
                Ret = get_next_word(bufftmp,word,0);
                if(!Ret) {
                    error (SEG_FILE_, "Cant find position");
                    return Ret;
                }
                pCommStr->flag = 5;/*Описание эл.д*/
            }  else
            {
                pCommStr->flag = 4;/*Описание композита*/
            }
            if(strlen(word) > POS_LEN){
                error(SEG_FILE_,"Position number is too long");
                return 0;
            }
            if(!TestForDigit(word)){
                error(SEG_FILE_,"Position must be digit");
                return 0;
            }
            strcpy(pCommStr->Command[1],word);              /* Pos */
            /*******************************************************/
            Ret = get_next_word(bufftmp,word,0);
            if(!Ret) {
                error(SEG_FILE_,"Composite or DataElement is not found");
                return Ret;
            }
            if(pCommStr->flag == 4){
                if(strlen(word) != COMP_LEN){
                    snprintf(str, sizeof str,"Composite - %.70s : bad length",word);
                    error(SEG_FILE_,str);
                    return 0;
                }
                strcpy(pCommStr->Command[2],word);/* Composite */
                pCommStr->Command[3][0] = '\0';
            } else{
                if (strlen(word) > DE_LEN){
                    snprintf(str, sizeof str,"DataElement - %.70s : bad length",word);
                    error(SEG_FILE_,str);
                    return 0;
                }
                if(!TestForDigit(word)){
                    error(SEG_FILE_,"Data element must be digit");
                    return 0;
                }
                strcpy(pCommStr->Command[3],word);/* Data element */
                pCommStr->Command[2][0] = '\0';
            }
            /* Composite or Data element */
            /***********************************************************/
            Ret = get_next_word(bufftmp,word,0);
            if(!Ret) {
                error(SEG_FILE_,"C/M - not found");
                return Ret;
            }
            if( strlen(word) > 1 ){
                error(SEG_FILE_,"C/M - bad length");
                return 0;
            }
            if((word[0] != MANDATORY) && (word[0] != CONDITIONAL))
            {
                snprintf(str, sizeof str,"C/M must be %c or %c",MANDATORY,CONDITIONAL);
                error (SEG_FILE_,str);
                return 0;
            }
            strcpy(pCommStr->Command[4],word);    /* C/M */
            /***********************************************************/
            Ret = get_next_word(bufftmp,word,0);
            if(!Ret) {
                error(SEG_FILE_,"R - not found");
                return Ret;
            }
            if(strlen(word) > COMP_R) {
                error(SEG_FILE_,"R - bad length");
                return 0;
            }
            if(!TestForDigit(word)){
                error(SEG_FILE_,"R  must be digit");
                return 0;
            }
            strcpy(pCommStr->Command[5],word);    /* R */
            /***********************************************************/
        }
    return 1;
}

/************************************************************************/
/* Записывает считанную строку в стр-ру                                 */
/* return 0 - неудача                                                   */
/* return 1 - Ok                                                        */
/************************************************************************/
int insert_comm_str_composite(Command_Struct *pCommStr, char **bufftmp)
{
    int Ret;
    char word[TEXT_SIZE];
    char str[100];
    int len;

    Ret = get_next_word(bufftmp,word,1);
    if(Ret < 0) return Ret;
    if(!Ret){
        error(COMP_FILE_," ");
        return Ret;
    }
    if( !memcmp( word,COMPCOMMAND,sizeof(COMPCOMMAND) ) ){/*описание композита*/
        /*format : {Composite: CompName "Text about this composite"}*/
        pCommStr->flag = 6;
        Ret = get_next_word(bufftmp,word,0);
        if(!Ret) {
            error (COMP_FILE_, "Composite: CompName \"Text about this composite\"");
            return Ret;
        }
        len=strlen(word);
        if(len>COMP_LEN) {
            error(COMP_FILE_, "Composite length is too long");
            return 0;
        } else if(len<COMP_LEN) {
            error(COMP_FILE_, "Composite length is too short");
            return 0;
        }
        strcpy(pCommStr->Command[0],word);
        Ret = get_text(bufftmp,word);
        if(!Ret) {
            error(COMP_FILE_,"Cant get text");
            return Ret;
        }
        len = strlen(word);
        memcpy(pCommStr->Text,word,len);
        pCommStr->Text[len] = '\0';
        pCommStr->tlen=len;
    } else
        if(pCommStr->Command[0][0] == '\0'){
            error (COMP_FILE_, "You must entering composite name");
            return 0;/*сегмент не определен*/
        }else {
            /*******************************************************/
            pCommStr->flag = 7;/* описание стр-ры комп-та */
            if(strlen(word) > POS_LEN){
                error(COMP_FILE_,"Position number is too long");
                return 0;
            }
            if(!TestForDigit(word)){
                error(COMP_FILE_,"Position must be digit");
                return 0;
            }
            strcpy(pCommStr->Command[1],word);              /* Pos */
            /*******************************************************/
            Ret = get_next_word(bufftmp,word,0);
            if(!Ret) {
                error(COMP_FILE_,"Data element is not found");
                return Ret;
            }
            if (strlen(word) > DE_LEN){
                snprintf(str, sizeof str,"Data element - %.60s : bad length",word);
                error(COMP_FILE_,str);
                return 0;
            }
            if(!TestForDigit(word)){
                error(COMP_FILE_,"Data element must be digit");
                return 0;
            }
            strcpy(pCommStr->Command[2],word);/* Data element */
            /* Data element */
            /***********************************************************/
            Ret = get_next_word(bufftmp,word,0);
            if(!Ret) {
                error(COMP_FILE_,"C/M - not found");
                return Ret;
            }
            if( strlen(word) > 1 ){
                error(COMP_FILE_,"C/M - bad length");
                return 0;
            }
            if((word[0] != MANDATORY) && (word[0] != CONDITIONAL))
            {
                snprintf(str, sizeof str,"C/M must be %c or %c",MANDATORY,CONDITIONAL);
                error (COMP_FILE_,str);
                return 0;
            }
            strcpy(pCommStr->Command[3],word);    /* C/M */
            /***********************************************************/
            Ret = get_next_word(bufftmp,word,0);
            if(!Ret) {
                error(COMP_FILE_,"R - not found");
                return Ret;
            }
            if(strlen(word) > COMP_R) {
                error(COMP_FILE_,"R - bad length");
                return 0;
            }
            if(!TestForDigit(word)){
                error(COMP_FILE_,"R  must be digit");
                return 0;
            }
            strcpy(pCommStr->Command[4],word);    /* R */
            /***********************************************************/
        }
    return 1;
}

int insert_comm_str_data_element(Command_Struct *pCommStr, char **bufftmp)
{
    int Ret;
    char word[TEXT_SIZE];
    char str[100];
    int len;

    Ret = get_next_word(bufftmp,word,1);
    if(Ret < 0) return Ret;
    if(!Ret) {
        error(DATA_FILE_,"Data element is not found");
        return Ret;
    }
    if (strlen(word) > DE_LEN){
        snprintf(str, sizeof str,"Data element - %.70s : bad length",word);
        error(DATA_FILE_,str);
        return 0;
    }
    if(!TestForDigit(word)){
        error(DATA_FILE_,"Data element must be digit");
        return 0;
    }
    strcpy(pCommStr->Command[0],word);/* Data element */
    /* Data element */
    /*******************************************************/
    Ret = get_next_word(bufftmp,word,0);
    if(!Ret) {
        error(DATA_FILE_,"Format of the data element is not found");
        return Ret;
    }
    if (strlen(word) > 1){
        snprintf(str, sizeof str,"Data element format - %.60s : bad length",word);
        error(DATA_FILE_,str);
        return 0;
    }
    if( (word[0] != DATA_FORMAT1) &&
            (word[0] != DATA_FORMAT2) &&
            (word[0] != DATA_FORMAT3) ){
        snprintf(str, sizeof str,"Format of the data element must be <%c> or <%c> or <%c>",
                DATA_FORMAT1, DATA_FORMAT2, DATA_FORMAT3);
        error(DATA_FILE_,str);
        return 0;
    }
    strcpy(pCommStr->Command[1],word);/* Data element format */
    /***********************************************************/
    Ret = get_next_word(bufftmp,word,0);
    if(!Ret) {
        error(DATA_FILE_,"MinField of the data element is not found");
        return Ret;
    }
    if( strlen(word) > FIELD_LEN ){
        error(DATA_FILE_,"MinField - bad length");
        return 0;
    }
    if(!TestForDigit(word)){
        error(DATA_FILE_,"Data element MinField must be digit");
        return 0;
    }
    strcpy(pCommStr->Command[2],word);    /* MinField */
    /***********************************************************/
    Ret = get_next_word(bufftmp,word,0);
    if(!Ret) {
        error(DATA_FILE_,"MaxField of the data element is not found");
        return Ret;
    }
    if( strlen(word) > FIELD_LEN ){
        error(DATA_FILE_,"MaxField - bad length");
        return 0;
    }
    if(!TestForDigit(word)){
        error(DATA_FILE_,"Data element MaxField must be digit");
        return 0;
    }
    strcpy(pCommStr->Command[3],word);    /* MaxField */
    /***********************************************************/
    Ret = get_text(bufftmp,word);
    if(!Ret) {
        error(DATA_FILE_,"Cant get text");
        return Ret;
    }
    len = strlen(word);
    memcpy(pCommStr->Text,word,len);        /* Text */
    pCommStr->Text[len]='\0';
    pCommStr->tlen=len;
    /***********************************************************/
    pCommStr->flag = 8;/* описание элемента данных */
    return 1;
}

void error(const char *filename, const char *errorline)
{
    printf("%s:%d %s\n",filename,FileLine,errorline);
    EdiError(EDILOG,"%s:%d %s\n",filename,FileLine,errorline);
    return ;
}

int TestForDigit(char *Str)
{
    int i=0;
    while(Str[i] != '\0'){
        if(!ISDIGIT((int)Str[i])){
            return 0;
        }
        i++;
    }
    return 1;/*Ok*/
}

int create_one_file(const char *dir, const char *file, const char *help)
{
    char name[512];
    FILE *fp;

    if(create_file_name(dir,file,name,sizeof(name))){
        return EDI_MES_ERR;
    }

    if((fp = fopen(name,"r")) == NULL){
        if((fp = fopen(name,"w")) == NULL){
            EdiError(EDILOG,"Can't create file : %s : %s\n\a",name,strerror(errno));
            return EDI_MES_ERR;
        }
        fprintf(fp,"%s",help);
        fclose(fp);
    } else
        EdiTrace(TRACE1, "File %s is already exists\n",name);

    return EDI_MES_OK;
}

int create_edi_empty_files(const char *dir)
{
    int ret = EDI_MES_ERR;
    char help[1024];

    do {
        snprintf(help,sizeof(help),
                 "# File for inserting message structure \n"
                 "# %s  message_name %cText about this message %c    \n"
                 "#          Pos  Tag    %c/%c R             \n"
                 "# %s[n]:   Pos  MaxPos %c/%c R              \n",
                 MESCOMMAND,TEXTCHAR,TEXTCHAR,MANDATORY,CONDITIONAL,
                 SGCOMMAND,MANDATORY,CONDITIONAL);

        if(create_one_file(dir,MES_FILE_,help)!=EDI_MES_OK){
            break;
        }

        snprintf(help,sizeof(help),
                 "# File for inserting segment structure \n"
                 "# %s  segment_name %cText about this segment %c    \n"
                 "#      Pos  Composite  %c/%c R           \n"
                 "# %s   Pos  Data_Elem  %c/%c R           \n",
                 SEGCOMMAND,TEXTCHAR,TEXTCHAR,MANDATORY,CONDITIONAL,
                 DECOMMAND,MANDATORY,CONDITIONAL);

        if(create_one_file(dir,SEG_FILE_,help)!=EDI_MES_OK){
            break;
        }

        snprintf(help,sizeof(help),
                 "# File for inserting composite structure \n"
                 "# %s composite_name  %cText about this composite %c    \n"
                 "#      Pos  Data_Elem  %c/%c R           \n",
                 COMPCOMMAND,TEXTCHAR,TEXTCHAR,MANDATORY,CONDITIONAL);
        if(create_one_file(dir,COMP_FILE_,help)!=EDI_MES_OK){
            break;
        }

        snprintf(help,sizeof(help),
                 "# File for inserting data elements \n"
                 "#%c - Alpha-Numeric\n"
                 "#%c - Numeric      \n"
                 "#%c - Alphabetic   \n"
                 "# data_elem Format(%c/%c/%c) min_field max_field %cText about this data element%c\n",
                 DATA_FORMAT1,DATA_FORMAT2,DATA_FORMAT3,
                 DATA_FORMAT1,DATA_FORMAT2,DATA_FORMAT3,
                 TEXTCHAR,TEXTCHAR);

        if(create_one_file(dir,DATA_FILE_,help)!=EDI_MES_OK){
            break;
        }

        ret = EDI_MES_OK;
    }while(0);

    return ret;
}

int create_file_name(const char *dir, const char *file, char *res, size_t size)
{
    int ret=snprintf(res,size,"%s/%s",dir,file);
    if(ret<0|| size_t(ret)>size){
        EdiError(EDILOG,"Too long dir+file names for a buffer. (%s/%s)",dir,file);
        return -1;
    }
    return 0;
}

int open_edi_files(const char *dir, EdiFilePoints *pFp)
{
    char name[512];
    int ret=-1;

    do{
        if(create_file_name(dir,MES_FILE_,name, sizeof(name))){
            break;
        }
        if((pFp->mes=fopen(name,"r"))==NULL){
            EdiError(EDILOG,"Open %s: %s",name,strerror(errno));
            break;
        }

        if(create_file_name(dir,SEG_FILE_,name, sizeof(name))){
            break;
        }
        if((pFp->seg=fopen(name,"r"))==NULL){
            EdiError(EDILOG,"Open %s: %s",name,strerror(errno));
            break;
        }

        if(create_file_name(dir,COMP_FILE_,name, sizeof(name))){
            break;
        }
        if((pFp->comp=fopen(name,"r"))==NULL){
            EdiError(EDILOG,"Open %s: %s",name,strerror(errno));
            break;
        }

        if(create_file_name(dir,DATA_FILE_,name, sizeof(name))){
            break;
        }
        if((pFp->data=fopen(name,"r"))==NULL){
            EdiError(EDILOG,"Open %s: %s",name,strerror(errno));
            break;
        }
        ret=0;
    }while(0);

    if(ret){
        close_edi_files(pFp);
    }
    return ret;
}

void close_edi_files(EdiFilePoints *pFp)
{
    if(pFp->mes){
        fclose(pFp->mes);
    }

    if(pFp->seg){
        fclose(pFp->seg);
    }

    if(pFp->comp){
        fclose(pFp->comp);
    }

    if(pFp->data){
        fclose(pFp->data);
    }
}
