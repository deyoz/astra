#if HAVE_CONFIG_H
#endif

#include <string.h>
#include "errno.h"
#include <unistd.h>
#include <fcntl.h>
#include "tclmon.h"
#include "moncmd.h"
#include "lwriter.h"
#include "potok.h"
#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "KSE"
void add_zapr_to_potok_proc(int n_zapr, int type_zapr, int grp_zapr);
void get_potok_all_proc(char *potok_str, int potok_size);
void reset_potok_proc(void);
void reset_potok_head(void);

void reset_potok_item( _PotokItem_ &p_item,time_t tt_cur );
void timeout_potok(time_t tt_cur, int add_flag);
/* Перед вызовом след. функции необходимо вызвать timeout_potok() */
float calc_potok_by_struct(int potokstr_num, int n_items);
char *get_potok_by_que_num(int que_num, int head_fl, int tail_fl);
char *get_zapr_by_sec_Zapr(int potokstr_num, int n_items,
                           char *str_dest, int size_dest );
void add_zapr_to_potok_str(int potokstr_num, int n_zapr);

_PotokItem_ sPotokItem[MAX_POTOK_ITEM];
int curPotokItem;

_PotokHead_ sPotokHead[QUEHEAD_MAX];

int get_que_num_by_grp_name_check(char *grp_name)
{
  if(grp_name!=NULL)
  {
    for(int i_head=0;i_head<QUEGRP_MAX;i_head++)
    {
      _PotokHead_ &s_head=sPotokHead[i_head+1];
      if( (s_head.inp_potok_num>=0 || s_head.obr_potok_num>=0) &&
          strncmp(grp_name,s_head.potok_head,sizeof(s_head.potok_head))==0)
      {
        return i_head+1;
      }
    }
  }
return -1;
}

int get_que_num_by_grp_name_proc(char *grp_name)
{
 int que_num=-1;

  if(grp_name!=NULL)
  {
    que_num=get_que_num_by_grp_name_check(grp_name);

    if(que_num<0)
    {
      for(int i_head=0;i_head<QUEGRP_MAX;i_head++)
      {
        _PotokHead_ &s_head=sPotokHead[i_head+1];
        if( (s_head.inp_potok_num<0 && s_head.obr_potok_num<0) )
        {
          s_head.set_potok_head(grp_name);
          s_head.inp_potok_num=(i_head+1)*2;
          s_head.obr_potok_num=(i_head+1)*2+1;
          que_num=i_head+1;
          break;
        }
      }
    }
  }
  if(que_num<0)
    que_num=0;
return que_num;
}

void print_potok_head(void)
{
 int i;
  for(i=0;i<QUEHEAD_MAX;i++)
  {
     fprintf(stderr,"%2i %3i %3i '%s'\n",
             i,
             sPotokHead[i].inp_potok_num,
             sPotokHead[i].obr_potok_num,
             sPotokHead[i].potok_head);
  }
}

void reset_potok_head(void)
{
  for(int i=0;i<QUEHEAD_MAX;i++)
  {
    sPotokHead[i]._reset();
  }

  /*Tlg all*/
  sPotokHead[QUEHEAD_OFFSET_TLG]._set(NULL,QUESTRUCT_OFFSET_TLG,QUESTRUCT_OFFSET_TLG+1);


 /*Tlg EDI*/
 sPotokHead[QUEHEAD_OFFSET_TLG_EDI]._set("EDI:",-1,QUESTRUCT_OFFSET_TLG_EDI);

 /*Tlg AIR*/
 sPotokHead[QUEHEAD_OFFSET_TLG_AIR]._set("AIR:",-1,QUESTRUCT_OFFSET_TLG_AIR);

 /*Tlg RES*/
 sPotokHead[QUEHEAD_OFFSET_TLG_RES]._set("RES:",-1,QUESTRUCT_OFFSET_TLG_RES);

 /*Tlg OTH*/
 sPotokHead[QUEHEAD_OFFSET_TLG_OTH]._set("OTH:",-1,QUESTRUCT_OFFSET_TLG_OTH);
}

void reset_potok_proc(void)
{
 int i_item;
 time_t tt_cur;
 time_t tt_tmp;

  tt_cur = time(NULL);
  
  for(i_item=0;i_item<MAX_POTOK_ITEM;i_item++)
  {
    curPotokItem=i_item;
    tt_tmp=tt_cur-MAX_POTOK_ITEM+1+i_item;
    if(tt_tmp<0)
      tt_tmp=0;
    reset_potok_item( sPotokItem[i_item] ,tt_tmp );
  }
}

void reset_potok_item( _PotokItem_ &p_item,time_t tt_cur )
{
  p_item._reset();
  p_item.set_time(tt_cur);
}

void print_potok(char *head)
{
 int i_item;
 int i_potokstr;
  printf("%s: curPotokItem=%i\n",(head==NULL)?"??":head,curPotokItem);
  for(i_item=0;i_item<MAX_POTOK_ITEM;i_item++)
  {
    printf("%2d %9ld",i_item,sPotokItem[i_item].tt_potok);
    for(i_potokstr=0;i_potokstr<QUESTRUCT_MAX;i_potokstr++)
      printf(" %3d",sPotokItem[i_item].n_zapr[i_potokstr]);
    printf("\n");
  }
}


/* add_flag == 0 - use for show        */
/*          == 1 - use for change(add) */
void timeout_potok(time_t tt_cur, int add_flag)
{
 int i_item;
 int cur_item;
 double tt_diff;

  /* Если время в текущем элементе не совпадает с текущим временем */
  /* то пробуем сделать изменения */
  tt_diff=difftime(tt_cur,sPotokItem[curPotokItem].tt_potok);

/*
  if( add_flag==0 && tt_diff<2 )
     return;
  if( add_flag==1 && tt_diff<1 )
     return;
*/
  if(tt_diff<1)
     return;
     
  /* смещаем текущий номер и обнуляем информацию для него */
  curPotokItem = (curPotokItem+1) % MAX_POTOK_ITEM;
  reset_potok_item(  sPotokItem[curPotokItem] ,tt_cur );
  
  cur_item = curPotokItem;

  /* Посмотрим, есть ли у нас еще старые записи*/
  for(i_item=0 ;i_item<MAX_POTOK_ITEM-1;i_item++)
  {
    cur_item = (cur_item+1)%MAX_POTOK_ITEM;
    
    if( difftime(tt_cur,sPotokItem[cur_item].tt_potok) >= 
        ((add_flag==0)?(MAX_POTOK_ITEM+1):(MAX_POTOK_ITEM))  )
    {/*Если запись старая, то обнуляем информацию и делаем ее текущей записью*/
      curPotokItem = cur_item;
      reset_potok_item(  sPotokItem[curPotokItem], tt_cur );
    }
    else
    {/*Если запись еще свежая, то выходим из цикла*/
       break;
    }
  }
}


/* Перед вызовом данной функции необходимо вызвать timeout_potok() */
void add_zapr_to_potok_str(int potokstr_num, int n_zapr)
{
  if(potokstr_num>=0 && potokstr_num<QUESTRUCT_MAX)
  {
    sPotokItem[curPotokItem].n_zapr[potokstr_num] += n_zapr;
  }
}

void add_zapr_to_potok_tlg(int n_zapr, int type_zapr)
{
 time_t tt;
  tt = time(NULL);
  timeout_potok(tt,1);
  int potokstr_num=-1;
  if(type_zapr >=  QUEPOT_TLG_BEG    && 
     type_zapr < QUEPOT_TLG_BEG+QUEPOT_TLG_NUM  )
  {
    add_zapr_to_potok_str(sPotokHead[QUEHEAD_OFFSET_TLG].obr_potok_num, n_zapr);
  }

  switch(type_zapr)
  {
    case QUEPOT_TLG_INP:
      potokstr_num=sPotokHead[QUEHEAD_OFFSET_TLG].inp_potok_num;
      break;
    case QUEPOT_TLG_EDI:
      potokstr_num=sPotokHead[QUEHEAD_OFFSET_TLG_EDI].obr_potok_num;
      break;
    case QUEPOT_TLG_AIR:
      potokstr_num=sPotokHead[QUEHEAD_OFFSET_TLG_AIR].obr_potok_num;
      break;
    case QUEPOT_TLG_RES:
      potokstr_num=sPotokHead[QUEHEAD_OFFSET_TLG_RES].obr_potok_num;
      break;
    case QUEPOT_TLG_OTH:
      potokstr_num=sPotokHead[QUEHEAD_OFFSET_TLG_OTH].obr_potok_num;
      break;
  }
  add_zapr_to_potok_str(potokstr_num, n_zapr);
}

void add_zapr_to_potok_grp(int n_zapr, int type_zapr,int grp_zapr)
{
 time_t tt;
 int que_num;
 int potokstr_num;
  tt = time(NULL);
  timeout_potok(tt,1);
  que_num=grp_zapr;
  /* Если номер очереди не правильный - устанавливаем номер очереди = 0*/
  if(que_num<1 ||que_num>=QUEGRP_MAX+1 )
  {
    que_num=0;
  }
  /* Если в структуре заголовков переменные не инициализированы */
  /* то  устанавливаем номер очереди = 0 */
  if(que_num>0 && 
     (sPotokHead[que_num].inp_potok_num<0 ||
      sPotokHead[que_num].obr_potok_num<0  ) )
  {
    que_num=0;
  }
  /* Если у нас номер очереди=0 и */
  /* для нее в структуре заголовков переменные не инициализированы, */
  /* то инициализируем переменные */
  if(que_num==0 && 
     (sPotokHead[que_num].inp_potok_num<0 ||
      sPotokHead[que_num].obr_potok_num<0  ) )
  {
    sPotokHead[que_num].inp_potok_num=0;
    sPotokHead[que_num].obr_potok_num=1;
  }
  
  potokstr_num=(type_zapr==QUEPOT_LEVB)?sPotokHead[que_num].inp_potok_num:
                                        sPotokHead[que_num].obr_potok_num;
  add_zapr_to_potok_str(potokstr_num, n_zapr);
}

void add_zapr_to_potok_proc(int n_zapr, int type_zapr, int grp_zapr)
{
  if(type_zapr == QUEPOT_NULL)
  {
     return;
  }
  if(type_zapr < 0 || 
     type_zapr >= QUEPOT_MAX)
  {
     type_zapr = QUEPOT_ZAPR;
  }
  if(type_zapr==QUEPOT_ZAPR || type_zapr==QUEPOT_LEVB)
  {
    add_zapr_to_potok_grp(n_zapr,type_zapr, grp_zapr);
  }
  else if(type_zapr>=QUEPOT_TLG_INP &&
          type_zapr < QUEPOT_TLG_BEG+QUEPOT_TLG_NUM)
  {
    add_zapr_to_potok_tlg(n_zapr,type_zapr);
  }
}


void get_potok_all_tlg(char *potok_str, int potok_size)
{
  int potok_len;
  char tlg_potok_head[]="Tlg";
  int i_head;
  int inp_potokstr_num;
  int obr_potokstr_num;

  potok_len = strlen(potok_str);
  if(potok_size > potok_len)
  {
    inp_potokstr_num = sPotokHead[QUEHEAD_OFFSET_TLG].inp_potok_num;
    obr_potokstr_num = sPotokHead[QUEHEAD_OFFSET_TLG].obr_potok_num;
    snprintf(potok_str+potok_len,potok_size-potok_len,
             "%-30.30s %5.2f/%5.2f ( ",
             tlg_potok_head,
             calc_potok_by_struct( inp_potokstr_num,-1 ),
             calc_potok_by_struct( obr_potokstr_num,-1 ) );
  }  

  for(i_head=QUEHEAD_OFFSET_TLG_BEG;i_head<QUEHEAD_OFFSET_TLG_BEG+QUEPOT_TLG_NUM;i_head++)
  {
    potok_len = strlen(potok_str);
    if( potok_size > potok_len)
    {
       snprintf(potok_str+potok_len,potok_size-potok_len,
                "%s%.2f  ", 
                sPotokHead[i_head].potok_head,
                calc_potok_by_struct( sPotokHead[i_head].obr_potok_num,-1 ) );
    }
  }
  potok_len = strlen(potok_str);
  if(potok_size > potok_len)
  {
     snprintf(potok_str+potok_len,potok_size-potok_len,")\n" );
  }

}

void get_potok_all_grp(char *potok_str, int potok_size)
{
  int potok_len;
  char *str;
  int i_que;
  for(i_que=0;i_que<QUEGRP_MAX+1;i_que++)
  {
    str = get_potok_by_que_num(i_que,1,1);
    if(strlen(str)>0)
    {
      potok_len = strlen(potok_str);
      if(potok_size > potok_len)
      {
        snprintf(potok_str+potok_len,potok_size-potok_len,"%s\n",str );
      }
    }
  }
  /* Удалим последний перевод строки */
  if((potok_len=strlen(potok_str))>0)
    potok_str[potok_len-1]='\0';
}

void get_potok_all_proc(char *potok_str, int potok_size)
{
  time_t tt_cur;

  potok_str[0]=0;
  tt_cur = time(NULL);
  timeout_potok(tt_cur,0);
  /* TlgInp */
  get_potok_all_tlg(potok_str,potok_size);

  /* Zapr */
  get_potok_all_grp(potok_str,potok_size);
}



/*****************************************************************/

/* Перед вызовом данной функции необходимо вызвать timeout_potok() */
char *get_zapr_by_sec_Zapr(int potokstr_num, int n_items,
                           char *str_dest, int size_dest )
{
/* static char zapr_str[2000];
 int    zapr_size=sizeof(zapr_str);*/
 int    len_dest=0;
 int    i_item;
 int    cur_item;
 int    n_zapr=0;
 
  str_dest[0] = 0;
  if(potokstr_num>=0 && potokstr_num<QUESTRUCT_MAX)
  {
    len_dest = strlen(str_dest);
    if( size_dest > len_dest)
    {
      snprintf(str_dest+len_dest,size_dest-len_dest,"(");
    }

    if(n_items<=0 || n_items>MAX_POTOK_ITEM-1)
    {
      n_items = MAX_POTOK_ITEM-1;
    }
    cur_item=curPotokItem-n_items;
    if(cur_item<0)
      cur_item += MAX_POTOK_ITEM;

    /*Информацию за текущую секунду не рассматриваем*/
    for(i_item=0 ;i_item<n_items;i_item++)
    {
      n_zapr =  sPotokItem[cur_item].n_zapr[potokstr_num];
      len_dest = strlen(str_dest);
      if( size_dest > len_dest)
      {
        snprintf(str_dest+len_dest,size_dest-len_dest," %3d", n_zapr);
      }
      cur_item = (cur_item+1)%MAX_POTOK_ITEM;
    }
    len_dest = strlen(str_dest);
    if( size_dest > len_dest)
    {
      snprintf(str_dest+len_dest,size_dest-len_dest,")");
    }
  }
return str_dest;
}

char *get_potok_by_que_name_proc(char *que_name)
{
 int que_num=-1;
 time_t tt_cur;

  tt_cur = time(NULL);
  timeout_potok(tt_cur,0);

  que_num=get_que_num_by_grp_name_check(que_name);

return get_potok_by_que_num(que_num, 0, 1);
}

/* Перед вызовом данной функции необходимо вызвать timeout_potok() */
char *get_potok_by_que_num(int que_num, int head_fl, int tail_fl)
{
 static  char potok_str[2000];
 int potok_len;
 int potok_size;
 char str_inp[200];
 char str_obr[200];
 
  potok_str[0] = 0;
  potok_size=sizeof(potok_str);
  
  if(que_num>=0 && que_num<QUEHEAD_MAX)
  {
    _PotokHead_ &s_head=sPotokHead[que_num];
    if(s_head.inp_potok_num>=0 || s_head.obr_potok_num>=0)
    {
      if(head_fl==1)
      {
        potok_len=strlen(potok_str);
        if(potok_size > potok_len)
        {
          snprintf(potok_str+potok_len,potok_size-potok_len,
                   "%-30.30s ",
                   (que_num==0 && s_head.potok_head[0]=='\0')?"Daemons":s_head.potok_head);
        }
      }
      potok_len=strlen(potok_str);
      if(potok_size > potok_len)
      {
        snprintf(potok_str+potok_len,potok_size-potok_len,
               "%5.2f/%5.2f  %5.2f/%5.2f", 
               calc_potok_by_struct( s_head.inp_potok_num, -1 ),
               calc_potok_by_struct( s_head.obr_potok_num, -1 ),
               calc_potok_by_struct( s_head.inp_potok_num, SHOW_POTOK_ITEM ),
               calc_potok_by_struct( s_head.obr_potok_num, SHOW_POTOK_ITEM ));
      }
      if(tail_fl==1)
      {
        get_zapr_by_sec_Zapr( s_head.inp_potok_num, SHOW_POTOK_ITEM,
                              str_inp,sizeof(str_inp) );
        get_zapr_by_sec_Zapr( s_head.obr_potok_num, SHOW_POTOK_ITEM ,
                              str_obr,sizeof(str_obr));
        potok_len=strlen(potok_str);
        if(potok_size > potok_len)
        {
          snprintf(potok_str+potok_len,potok_size-potok_len,
                   " %s/%s",str_inp,str_obr);
        }
      }
    }
  }
return potok_str;
}

/* Перед вызовом данной функции необходимо вызвать timeout_potok() */
float calc_potok_by_struct(int potokstr_num, int n_items)
{
 float potok_fl;  
 int n_zapr;
 int i_item;
 int cur_item;

  if(potokstr_num<0 || potokstr_num>=QUESTRUCT_MAX)
  {
     return 0;
  }
  if(n_items<=0 || n_items>MAX_POTOK_ITEM-1)
  {
    n_items = MAX_POTOK_ITEM-1;
  }
  cur_item=curPotokItem-n_items;
  if(cur_item<0)
    cur_item += MAX_POTOK_ITEM;
    
  n_zapr = 0;
  /*Информацию за текущую секунду не рассматриваем*/
  for(i_item=0 ;i_item<n_items;i_item++)
  {
     n_zapr += sPotokItem[cur_item].n_zapr[potokstr_num];
     cur_item = (cur_item+1)%MAX_POTOK_ITEM;
  }

  potok_fl = (float)n_zapr*1000/((n_items)*1000); 
return potok_fl;
}
