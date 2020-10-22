create or replace procedure SP_WB_SHED_REIS_UPD
(cXML_in in clob,
   cXML_out out clob)
as

P_ID number:=-1;
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
P_LANG varchar2(50):='ENG';

P_NR varchar2(50);
P_ID_AP_1 number;
P_ID_AP_2 number;

P_TERM_1 varchar2(50);
P_TERM_2 varchar2(50);
P_IS_TERM_1_EMPTY number:=0;
P_IS_TERM_2_EMPTY number:=0;

P_SHED_MVL_TYPE number:=0;

P_SHED_DATE_DEP date:=null;
P_ESTM_DATE_DEP date:=null;
P_ACTL_DATE_DEP date:=null;

P_SHED_DATE_ARR date:=null;
P_ESTM_DATE_ARR date:=null;
P_ACTL_DATE_ARR date:=null;

P_SHED_DATE_DEP_D varchar2(50);
P_SHED_DATE_DEP_M varchar2(50);
P_SHED_DATE_DEP_Y varchar2(50);

P_ESTM_DATE_DEP_D varchar2(50);
P_ESTM_DATE_DEP_M varchar2(50);
P_ESTM_DATE_DEP_Y varchar2(50);

P_ACTL_DATE_DEP_D varchar2(50);
P_ACTL_DATE_DEP_M varchar2(50);
P_ACTL_DATE_DEP_Y varchar2(50);

P_SHED_DATE_ARR_D varchar2(50);
P_SHED_DATE_ARR_M varchar2(50);
P_SHED_DATE_ARR_Y varchar2(50);

P_ESTM_DATE_ARR_D varchar2(50);
P_ESTM_DATE_ARR_M varchar2(50);
P_ESTM_DATE_ARR_Y varchar2(50);

P_ACTL_DATE_ARR_D varchar2(50);
P_ACTL_DATE_ARR_M varchar2(50);
P_ACTL_DATE_ARR_Y varchar2(50);

P_SHED_DATE_DEP_MIN number:=0;
P_ESTM_DATE_DEP_MIN number:=0;
P_ACTL_DATE_DEP_MIN number:=0;

P_SHED_DATE_ARR_MIN number:=0;
P_ESTM_DATE_ARR_MIN number:=0;
P_ACTL_DATE_ARR_MIN number:=0;

P_DATE_TIME_UTC int;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';

cXML_data clob;
V_R_COUNT number:=0;
V_STR_MSG clob;
begin
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AP_1[1]')) into P_ID_AP_1 from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AP_2[1]')) into P_ID_AP_2 from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SHED_MVL_TYPE[1]')) into P_SHED_MVL_TYPE from dual;

   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_NR[1]') into P_NR from dual;

   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_TERM_1[1]') into P_TERM_1 from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_TERM_2[1]') into P_TERM_2 from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_TERM_1_EMPTY[1]')) into P_IS_TERM_1_EMPTY from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_TERM_2_EMPTY[1]')) into P_IS_TERM_2_EMPTY from dual;

   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;


   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SHED_DATE_DEP_D[1]') into P_SHED_DATE_DEP_D from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SHED_DATE_DEP_M[1]') into P_SHED_DATE_DEP_M from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SHED_DATE_DEP_Y[1]') into P_SHED_DATE_DEP_Y from dual;

   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ESTM_DATE_DEP_D[1]') into P_ESTM_DATE_DEP_D from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ESTM_DATE_DEP_M[1]') into P_ESTM_DATE_DEP_M from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ESTM_DATE_DEP_Y[1]') into P_ESTM_DATE_DEP_Y from dual;

   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ACTL_DATE_DEP_D[1]') into P_ACTL_DATE_DEP_D from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ACTL_DATE_DEP_M[1]') into P_ACTL_DATE_DEP_M from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ACTL_DATE_DEP_Y[1]') into P_ACTL_DATE_DEP_Y from dual;


   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SHED_DATE_ARR_D[1]') into P_SHED_DATE_ARR_D from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SHED_DATE_ARR_M[1]') into P_SHED_DATE_ARR_M from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SHED_DATE_ARR_Y[1]') into P_SHED_DATE_ARR_Y from dual;

   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ESTM_DATE_ARR_D[1]') into P_ESTM_DATE_ARR_D from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ESTM_DATE_ARR_M[1]') into P_ESTM_DATE_ARR_M from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ESTM_DATE_ARR_Y[1]') into P_ESTM_DATE_ARR_Y from dual;

   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ACTL_DATE_ARR_D[1]') into P_ACTL_DATE_ARR_D from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ACTL_DATE_ARR_M[1]') into P_ACTL_DATE_ARR_M from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ACTL_DATE_ARR_Y[1]') into P_ACTL_DATE_ARR_Y from dual;

   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SHED_DATE_DEP_MIN[1]')) into P_SHED_DATE_DEP_MIN from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ESTM_DATE_DEP_MIN[1]')) into P_ESTM_DATE_DEP_MIN from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ACTL_DATE_DEP_MIN[1]')) into P_ACTL_DATE_DEP_MIN from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SHED_DATE_ARR_MIN[1]')) into P_SHED_DATE_ARR_MIN from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ESTM_DATE_ARR_MIN[1]')) into P_ESTM_DATE_ARR_MIN from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ACTL_DATE_ARR_MIN[1]')) into P_ACTL_DATE_ARR_MIN from dual;

   P_SHED_DATE_DEP:=to_date(P_SHED_DATE_DEP_D||'.'||P_SHED_DATE_DEP_M||'.'||P_SHED_DATE_DEP_Y, 'dd.mm.yyyy');

   if P_ESTM_DATE_DEP_D<>'NULL' then
     begin
       P_ESTM_DATE_DEP:=to_date(P_ESTM_DATE_DEP_D||'.'||P_ESTM_DATE_DEP_M||'.'||P_ESTM_DATE_DEP_Y, 'dd.mm.yyyy');
     end;
   end if;

   if P_ACTL_DATE_DEP_D<>'NULL' then
     begin
       P_ACTL_DATE_DEP:=to_date(P_ACTL_DATE_DEP_D||'.'||P_ACTL_DATE_DEP_M||'.'||P_ACTL_DATE_DEP_Y, 'dd.mm.yyyy');
     end;
   end if;

   if P_SHED_DATE_ARR_D<>'NULL' then
     begin
       P_SHED_DATE_ARR:=to_date(P_SHED_DATE_ARR_D||'.'||P_SHED_DATE_ARR_M||'.'||P_SHED_DATE_ARR_Y, 'dd.mm.yyyy');
     end;
   end if;

   if P_ESTM_DATE_ARR_D<>'NULL' then
     begin
       P_ESTM_DATE_ARR:=to_date(P_ESTM_DATE_ARR_D||'.'||P_ESTM_DATE_ARR_M||'.'||P_ESTM_DATE_ARR_Y, 'dd.mm.yyyy');
     end;
   end if;

   if P_ACTL_DATE_ARR_D<>'NULL' then
     begin
       P_ACTL_DATE_ARR:=to_date(P_ACTL_DATE_ARR_D||'.'||P_ACTL_DATE_ARR_M||'.'||P_ACTL_DATE_ARR_Y, 'dd.mm.yyyy');
     end;
   end if;

   cXML_out:='<?xml version="1.0" ?><root>';

   -----------------------------------------------------------------------------
   ------------------------ПРОВЕРКИ---------------------------------------------
   select count(id) into V_R_COUNT
   from WB_SHED
   where id=P_ID;

   if V_R_COUNT=0 then
     begin
       if P_LANG='ENG' then
         begin
           V_STR_MSG:='This record has been deleted!';
         end;
       else
         begin
           V_STR_MSG:='Этa запись удалена!';
         end;
       end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
         select count(id) into V_R_COUNT
         from WB_REF_AIRCO_WS_TYPES
         where id_ac=P_ID_AC and
               id_ws=P_ID_WS;

         if V_R_COUNT=0 then
           begin
             if P_LANG='ENG' then
               begin
                 V_STR_MSG:='This type of aircraft is no longer used in this airline!';
               end;
             else
               begin
                 V_STR_MSG:='Такой тип воздушного судна уже не используется в этой авиакомпании!';
               end;
             end if;

           end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        if P_ID_BORT>-1 then
          begin
            select count(id) into V_R_COUNT
            from WB_REF_AIRCO_WS_BORTS
            where id=P_ID_BORT;

            if V_R_COUNT=0 then
              begin
                if P_LANG='ENG' then
                  begin
                    V_STR_MSG:='Aircraft with such a number is no longer used in this airline!';
                  end;
                else
                  begin
                    V_STR_MSG:='Воздушное судно с таким бортовым номером уже не используется в этой авиакомпании!';
                  end;
              end if;
              end;
            end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_AIRPORTS
        where id=P_ID_AP_1;

        if V_R_COUNT=0 then
           begin
             if P_LANG='ENG' then
               begin
                 V_STR_MSG:='Departure airport was deleted from the directory!';
               end;
             else
               begin
                 V_STR_MSG:='Аэропорт отправления удален из справочника!';
               end;
             end if;

           end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_AIRPORTS
        where id=P_ID_AP_2;

        if V_R_COUNT=0 then
           begin
             if P_LANG='ENG' then
               begin
                 V_STR_MSG:='Arrival airport is removed from the directory!';
               end;
             else
               begin
                 V_STR_MSG:='Аэропорт прибытия удален из справочника!';
               end;
             end if;

           end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_SHED_MVL_TYPE
        where id=P_SHED_MVL_TYPE;

        if V_R_COUNT=0 then
           begin
             if P_LANG='ENG' then
               begin
                 V_STR_MSG:='Selected flight type is removed from the directory!';
               end;
             else
               begin
                 V_STR_MSG:='Выбранный тип рейса удален из справочника!';
               end;
             end if;

           end;
        end if;
   -----------------------------------------------------------------------------
   -----------------------------------------------------------------------------
   if V_STR_MSG is null then
      begin
        select count(s.id) into V_R_COUNT
        from WB_SHED s
        where s.id<>P_ID and
              s.id_ac=P_ID_AC and
              s.nr=P_NR and
              s.S_DTL_1=P_SHED_DATE_DEP+(P_SHED_DATE_DEP_MIN/1440);

        if V_R_COUNT>0 then
             begin
               if P_LANG='RUS' then
                 begin
                   V_STR_MSG:=
                         'Рейс '||P_NR||'  '||
                           to_char(P_SHED_DATE_DEP+(P_SHED_DATE_DEP_MIN/1440), 'dd.mm.yyyy hh24:mi')||
                             ' Loc '||
                               'уже существует!';
                  end;
                end if;

               if P_LANG='ENG' then
                 begin
                   V_STR_MSG:=
                         'Flight № '||P_NR||'  '||
                           to_char(P_SHED_DATE_DEP+(P_SHED_DATE_DEP_MIN/1440), 'dd.mm.yyyy hh24:mi')||
                             ' Loc '||
                               'already exists!';
                  end;
                end if;
             end;
           end if;
         end;
        end if;
      end;
    end if;
   ------------------------ПРОВЕРКИ---------------------------------------------
   -----------------------------------------------------------------------------

   if V_STR_MSG is null then
     begin
       UPDATE WB_SHED
       set ID_WS=P_ID_WS,
           ID_BORT=P_ID_BORT,	
           NR=P_NR,
	         ID_AP_1=P_ID_AP_1,
	         ID_AP_2=P_ID_AP_2,
	         TERM_1=P_TERM_1,
	         TERM_2=P_TERM_2,
	         MVL_TYPE=P_SHED_MVL_TYPE,
	         S_DTU_1=null,
	         S_DTU_2=null,
	         S_DTL_1=P_SHED_DATE_DEP+(P_SHED_DATE_DEP_MIN/1440),
	         S_DTL_2=P_SHED_DATE_ARR+(P_SHED_DATE_ARR_MIN/1440),
	         E_DTU_1=null,
	         E_DTU_2=null,
	         E_DTL_1=P_ESTM_DATE_DEP+(P_ESTM_DATE_DEP_MIN/1440),
	         E_DTL_2=P_ESTM_DATE_ARR+(P_ESTM_DATE_ARR_MIN/1440),
	         F_DTU_1=null,
	         F_DTU_2=null,
           F_DTL_1=P_ACTL_DATE_DEP+(P_ACTL_DATE_DEP_MIN/1440),
	         F_DTL_2=P_ACTL_DATE_ARR+(P_ACTL_DATE_ARR_MIN/1440),
	         U_NAME=P_U_NAME,
           U_IP=P_U_IP,
           U_HOST_NAME=P_U_HOST_NAME,
	         DATE_WRITE=sysdate()
       where ID=P_ID;

       V_STR_MSG:='EMPTY_STRING';
     end;
   end if;

  cXML_out:=cXML_out||'<list str_msg="'||V_STR_MSG||'"/>'||'</root>';
  commit;
end SP_WB_SHED_REIS_UPD;
/
