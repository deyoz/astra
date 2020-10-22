create or replace procedure SP_WB_SHED_REIS_INS
(cXML_in in clob,
   cXML_out out clob)
as

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

P_SHED_MVL_TYPE int;

P_DATE_FROM date:=null;
P_DATE_TO date:=null;

P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);

P_DATE_TO_D varchar2(50);
P_DATE_TO_M varchar2(50);
P_DATE_TO_Y varchar2(50);

P_DAY_1 number:=0;
P_DAY_2 number:=0;
P_DAY_3 number:=0;
P_DAY_4 number:=0;
P_DAY_5 number:=0;
P_DAY_6 number:=0;
P_DAY_7 number:=0;


P_TIME_OUT_LOC_MIN int;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';

cXML_data clob;
V_R_COUNT number:=0;
V_STR_MSG clob;
V_RESULT number:=0;
V_CAN_INSERT number:=0;
begin
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

   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DAY_1[1]')) into P_DAY_1 from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DAY_2[1]')) into P_DAY_2 from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DAY_3[1]')) into P_DAY_3 from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DAY_4[1]')) into P_DAY_4 from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DAY_5[1]')) into P_DAY_5 from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DAY_6[1]')) into P_DAY_6 from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DAY_7[1]')) into P_DAY_7 from dual;

   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_TIME_OUT_LOC_MIN[1]')) into P_TIME_OUT_LOC_MIN from dual;

   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;

   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_TO_D[1]') into P_DATE_TO_D from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_TO_M[1]') into P_DATE_TO_M from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_TO_Y[1]') into P_DATE_TO_Y from dual;

   P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y, 'dd.mm.yyyy');
   P_DATE_TO:=to_date(P_DATE_TO_D||'.'||P_DATE_TO_M||'.'||P_DATE_TO_Y, 'dd.mm.yyyy');

   cXML_out:='<?xml version="1.0" ?><root>';

   -----------------------------------------------------------------------------
   ------------------------ПРОВЕРКИ---------------------------------------------
   select count(id) into V_R_COUNT
   from WB_REF_AIRCOMPANY_KEY
   where id=P_ID_AC;

   if V_R_COUNT=0 then
     begin
       if P_LANG='ENG' then
         begin
           V_STR_MSG:='This airline is removed!';
         end;
       else
         begin
           V_STR_MSG:='Этa авиакомпания удалена!';
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
      end;
    end if;
   ------------------------ПРОВЕРКИ---------------------------------------------
   -----------------------------------------------------------------------------

   if V_STR_MSG is null then
     begin

       while P_DATE_FROM<=P_DATE_TO
         loop
           V_CAN_INSERT:=1;

           select count(s.id) into V_R_COUNT
           from WB_SHED s
           where s.id_ac=P_ID_AC and
                 s.nr=P_NR and
                 s.S_DTL_1=P_DATE_FROM+(P_TIME_OUT_LOC_MIN/1440);

           if V_R_COUNT>0 then
             begin
               V_CAN_INSERT:=0;

               if P_LANG='RUS' then
                 begin
                   V_STR_MSG:=
                     V_STR_MSG||
                       Chr(13)||
                         'Рейс '||P_NR||'  '||
                           to_char(P_DATE_FROM+(P_TIME_OUT_LOC_MIN/1440), 'dd.mm.yyyy hh24:mi')||
                             ' Loc '||
                               'уже существует!';
                  end;
                end if;

               if P_LANG='ENG' then
                 begin
                   V_STR_MSG:=
                     V_STR_MSG||
                       Chr(13)||
                         'Flight № '||P_NR||'  '||
                           to_char(P_DATE_FROM+(P_TIME_OUT_LOC_MIN/1440), 'dd.mm.yyyy hh24:mi')||
                             ' Loc '||
                               'already exists!';
                  end;
                end if;
             end;
           end if;

           select to_number(to_char(P_DATE_FROM, 'd', 'nls_date_language=russian')) into V_R_COUNT from dual;

           if (V_R_COUNT=1 and P_DAY_1=0) or
                (V_R_COUNT=2 and P_DAY_2=0) or
                  (V_R_COUNT=3 and P_DAY_3=0) or
                    (V_R_COUNT=4 and P_DAY_4=0) or
                      (V_R_COUNT=5 and P_DAY_5=0) or
                        (V_R_COUNT=6 and P_DAY_6=0) or
                          (V_R_COUNT=7 and P_DAY_7=0) then
             begin

               V_CAN_INSERT:=0;
             end;
           end if;


           if V_CAN_INSERT=1 then
             begin
               insert into WB_SHED (ID,
	                                    ID_AC,
                                      	ID_WS,
                                       	  ID_BORT,
	                                          BORT_NUM,
                                	            NR,
	                                              ID_AP_1,
	                                                ID_AP_2,
	                                                  TERM_1,
	                                                    TERM_2,
	                                                      MVL_TYPE,
	                                                        S_DTU_1,
	                                                          S_DTU_2,
	                                                            S_DTL_1,
	                                                              S_DTL_2,
	                                                                E_DTU_1,
	                                                                  E_DTU_2,
	                                                                    E_DTL_1,
	                                                                      E_DTL_2,
	                                                                        F_DTU_1,
	                                                                          F_DTU_2,
                                                                           	  F_DTL_1,
	                                                                              F_DTL_2,
	                                                                                U_NAME,
                                                                        	          U_IP,
                                                                        	            U_HOST_NAME,
	                                                                                      DATE_WRITE)
              select SEC_WB_SHED.nextval,
                       P_ID_AC,
                         P_ID_WS,
                           P_ID_BORT,
                             NULL,
                               P_NR,
                                 P_ID_AP_1,
	                                 P_ID_AP_2,
	                                   P_TERM_1,
	                                     P_TERM_2,
	                                       P_SHED_MVL_TYPE,
                                           NULL,
                                             NULL,
                                               P_DATE_FROM+(P_TIME_OUT_LOC_MIN/1440),
                                                 NULL,
                                                   NULL,
                                                     NULL,
                                                       NULL,
                                                         NULL,
                                                           NULL,
                                                             NULL,
                                                               NULL,
                                                                 NULL,
                                                                   P_U_NAME,
	                                                                   P_U_IP,
	                                                                     P_U_HOST_NAME,
                                                                         sysdate()
              from dual;

               V_RESULT:=V_RESULT+1;
               commit;
             end;
          end if;

           P_DATE_FROM:=P_DATE_FROM+1;
         end loop;

       if V_STR_MSG is null then
         begin
           V_STR_MSG:='EMPTY_STRING';
         end;
       end if;
     end;
   end if;


  cXML_out:=cXML_out||'<list res="'||to_char(V_RESULT)||'" str_msg="'||V_STR_MSG||'"/>'||'</root>';
end SP_WB_SHED_REIS_INS;
/
