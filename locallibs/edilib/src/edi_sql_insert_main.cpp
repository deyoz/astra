#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE

#include <string.h>
#include <errno.h>

#include <serverlib/test.h>
#include <serverlib/cursctl.h>
#include "edilib/edi_user_func.h"
#include <serverlib/monitor_ctl.h>
#include "edilib/edilib_db_callbacks.h"

int edi_load_messages_main (int argc, char *argv[])
{
    int Ret=1;
    int delete_falg;

    if((argc == 1)||(argc > 3)){
        printf("Format : %s -c  [PATH] -to ceate used files\n",
               argv[0]);
        printf("         %s -i  [PATH] -for inserting files to oracle\n", argv[0]);
        printf("         %s -n  [PATH] -for inserting files to oracle whithout deleting old database messages\n", argv[0]);
        printf("         %s -p         -print all data base messages\n", argv[0]);
        printf("         %s -p  [PATH] -print all messages from directory\n", argv[0]);

        printf("		   [PATH] -path whith .edi files\n");
        return 0;
    }

    InitEdiLogger(ProgError,WriteLog,ProgTrace);

    if(!memcmp(argv[1],"-c",2) ){
        create_edi_empty_files((argc == 3)?argv[2]:"./");
        return 0;
    }

    if(!memcmp(argv[1],"-p",2)){
        /* ����஢���� ᮮ�饭�� , ��室����� � ����*/
        int ret;
        if(argc == 3){
            ret=CreateTemplateMessagesFf(argv[2]);
        }else {
            ret=CreateTemplateMessages();
        }

        if(ret) {
            printf("Testing data base message error!\n");
            ProgError(STDLOG,"Testing data base message error!\n");
            return -1;
        }
        PrintAllEdiTemplateMessages(stdout);
        return 0;
    }

    if(!memcmp(argv[1],"-n",2))
        delete_falg=0;
    else if(!memcmp(argv[1],"-i",2))
        delete_falg=1;
    else {
        printf("Parameter list error \n Use : %s for help\n", argv[0]);
        return 0;
    }


    if(delete_falg && (Ret != -1)){
        Ret = DeleteDBMesseges();
        if (Ret != 1)
        {
            printf("Delete old database messages - error!\n\n");
            ProgError(STDLOG,"Delete old database messages - error!\n\n");
            Ret = -1;
        } else  ProgTrace (TRACE5,"Delete old database messages - Ok!");
    }

    if(Ret == 1)
        Ret = insert_to_ora_from_dir_cur((argc == 3)?argv[2]:"./");
    if(!Ret) {
        printf("Inserting messages to oracle failed !\n");
        ProgError(STDLOG,"Inserting messages to oracle failed !");
        Ret = -1;
    }
    /* ����஢���� ���ᥭ��� ���ଠ樨 */

    if(Ret == 1){
        if(!CreateTemplateMessages()) {
            printf("Inserting message Ok\n");
            ProgTrace(TRACE3,"Inserting message Ok\n");
            Ret = 0;
        } else {
            printf("Testing message error! \n");
            ProgError(STDLOG,"Testing message error! \n");
            Ret = -1;
        }
    }

    return Ret;
}

