#ifndef __POTOK_H__
 #define __POTOK_H__

/*Максимальное количество групп обработчиков */
#define QUEGRP_MAX           15

/*Количество структур потоков */
#define QUESTRUCT_MAX            ((1+QUEGRP_MAX)*2+2+QUEPOT_TLG_NUM)
#define QUESTRUCT_OFFSET_TLG     ((1+QUEGRP_MAX)*2)
#define QUESTRUCT_OFFSET_TLG_BEG (QUESTRUCT_OFFSET_TLG+2)
#define QUESTRUCT_OFFSET_TLG_EDI (QUESTRUCT_OFFSET_TLG_BEG)
#define QUESTRUCT_OFFSET_TLG_AIR (QUESTRUCT_OFFSET_TLG_BEG+1)
#define QUESTRUCT_OFFSET_TLG_RES (QUESTRUCT_OFFSET_TLG_BEG+2)
#define QUESTRUCT_OFFSET_TLG_OTH (QUESTRUCT_OFFSET_TLG_BEG+3)

/*Максимальное количество заголовков */
#define QUEHEAD_MAX            (1+QUEGRP_MAX+1+QUEPOT_TLG_NUM)
#define QUEHEAD_OFFSET_TLG     (1+QUEGRP_MAX)
#define QUEHEAD_OFFSET_TLG_BEG (QUEHEAD_OFFSET_TLG+1)
#define QUEHEAD_OFFSET_TLG_EDI (QUEHEAD_OFFSET_TLG_BEG)
#define QUEHEAD_OFFSET_TLG_AIR (QUEHEAD_OFFSET_TLG_BEG+1)
#define QUEHEAD_OFFSET_TLG_RES (QUEHEAD_OFFSET_TLG_BEG+2)
#define QUEHEAD_OFFSET_TLG_OTH (QUEHEAD_OFFSET_TLG_BEG+3)

#define MAX_POTOK_ITEM 50
#define SHOW_POTOK_ITEM 3

class _PotokItem_
{
public:
  time_t  tt_potok;
  int     n_zapr[QUESTRUCT_MAX];
  
  void _reset()
  {
    tt_potok=0;/*may be now???*/
    for(int i=0;i<QUESTRUCT_MAX;i++)
    {
      n_zapr[i]=0;
    }
  };
  _PotokItem_() {_reset();}
  void set_time(time_t tt_cur)
  {
    tt_potok=tt_cur;
  }
};

class _PotokHead_
{
public:
 int  inp_potok_num;
 int  obr_potok_num;
 char potok_head[32];
 _PotokHead_() {_reset();}
 void _reset()
 {
   inp_potok_num=-1;
   obr_potok_num=-1;
   memset(potok_head,0,sizeof(potok_head));
 }
 void _set(const char *head,int inp_num, int obr_num)
 {
   set_potok_head(head);
   set_inp_num(inp_num);
   set_obr_num(obr_num);
 }
 
 void set_potok_head(const char *grp_name)
 {
   if(grp_name!=NULL)
   {
     strncpy(potok_head, grp_name, sizeof(potok_head)-1);
     potok_head[sizeof(potok_head)-1]=0;
   }
 }
 void set_inp_num(int inp_num)
 {
   inp_potok_num=inp_num;
 }
 void set_obr_num(int obr_num)
 {
   obr_potok_num=obr_num;
 }
 
}/*_PotokHead_*/;

#endif /*  __POTOK_H__*/
