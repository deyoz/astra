create or replace procedure SP_WB_REF_WS_AIR_S_L_P_S_P_UPD
(cXML_in in clob,
   cXML_out out clob)
as

p_ADV_ID number:=-1;
P_MAX_WEIGHT number:=null;
P_INSERT_ONLY number:=0;
P_MAX_WEIGHT_INT_PART varchar2(50):='NULL';
P_MAX_WEIGHT_DEC_PART varchar2(50):='NULL';

P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';

cXML_data clob;
V_R_COUNT number:=0;
V_SEAT_COUNT number:=0;
V_STR_MSG clob:=null;
V_ACTION_NAME varchar2(200):='SP_WB_REF_WS_AIR_S_L_P_S_P_UPD';
V_ACTION_DATE date:=sysdate();
begin
  cXML_out:='<?xml version="1.0" ?><root>';

  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ADV_ID[1]')) into P_ADV_ID from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_INSERT_ONLY[1]')) into P_INSERT_ONLY from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_MAX_WEIGHT_INT_PART[1]') into P_MAX_WEIGHT_INT_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_MAX_WEIGHT_DEC_PART[1]') into P_MAX_WEIGHT_DEC_PART from dual;

  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LANG[1]') into P_LANG from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_NAME[1]') into P_U_NAME from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_IP[1]') into P_U_IP from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

  if P_MAX_WEIGHT_INT_PART<>'NULL' then P_MAX_WEIGHT:=to_number(P_MAX_WEIGHT_INT_PART||'.'||P_MAX_WEIGHT_DEC_PART); end if;

  insert into WB_TEMP_XML_ID (ID,
                                num)
  select distinct f.ID,
                    f.ID
  from (select to_number(extractValue(value(t),'param_id_data/P_ID[1]')) as ID
        from table(xmlsequence(xmltype(cXML_in).extract('//param_id_data'))) t) f;

  insert into WB_TEMP_XML_ID_EX (ID,
                                   F_FLT_1,
                                     ACTION_NAME,
                                       ACTION_DATE)
  select distinct f.plan_id,
                    f.seat_id,
                      V_ACTION_NAME,
                        V_ACTION_DATE
    from (select to_number(extractValue(value(t),'records_id_data/P_PLAN_ID[1]')) as plan_id,
                   to_number(extractValue(value(t),'records_id_data/P_SEAT_ID[1]')) as seat_id
          from table(xmlsequence(xmltype(cXML_in).extract('//records_id_data'))) t) f;

  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  select count(distinct(t.id)) into V_R_COUNT
  from WB_TEMP_XML_ID_EX t
  where t.ACTION_NAME=V_ACTION_NAME and
        t.ACTION_DATE=V_ACTION_DATE and
        not exists(select 1
                   from WB_REF_WS_AIR_S_L_PLAN p
                   where p.id=t.id);
  if V_R_COUNT>0 then
    begin
      if P_LANG='ENG' then V_STR_MSG:='From the ranks of the distribution table deleted records containing the selected seats/seat - '||to_char(V_R_COUNT)||' The operation is prohibited!'; end if;
      if P_LANG='RUS' then V_STR_MSG:='Из таблицы распределения рядов удалены записи, содержащие выбранные места/место - '||to_char(V_R_COUNT)||'! Операция запрещена!'; end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(distinct(t.id)) into V_R_COUNT
      from WB_TEMP_XML_ID_EX t
      where t.ACTION_NAME=V_ACTION_NAME and
            t.ACTION_DATE=V_ACTION_DATE and
            not exists(select 1
                       from WB_REF_WS_AIR_S_L_P_S s
                       where s.plan_id=t.id and
                             s.seat_id=t.f_flt_1);

      if V_R_COUNT>0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='From the ranks of the distribution table deleted records of selected seats/seat - '||to_char(V_R_COUNT)||' The operation is prohibited!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Из таблицы распределения рядов удалены записи о выбранных местах/месте - '||to_char(V_R_COUNT)||'! Операция запрещена!'; end if;
        end;
      end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(t.id) into V_R_COUNT
      from WB_TEMP_XML_ID t
      where not exists(select 1
                       from WB_REF_WS_SEATS_PARAMS sp
                       where sp.id=t.id);

      if V_R_COUNT>0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='From the directory seats parameters by remove selected records - '||to_char(V_R_COUNT)||' The operation is prohibited!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Из справочника параметров мест удалено выбранных записей - '||to_char(V_R_COUNT)||'! Операция запрещена!'; end if;
        end;
      end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if (V_STR_MSG is null) then
    begin


      savepoint sp_1;

      update WB_REF_WS_AIR_S_L_P_S
      set MAX_WEIGHT=P_MAX_WEIGHT,
          U_NAME=P_U_NAME,
	        U_IP=P_U_IP,
	        U_HOST_NAME=P_U_HOST_NAME,
	        DATE_WRITE=sysdate()
      where id in (select s.id
                   from WB_REF_WS_AIR_S_L_P_S s join WB_TEMP_XML_ID_EX t
                   on t.ID=s.plan_id and
                      t.f_flt_1=s.seat_id and
                      t.ACTION_NAME=V_ACTION_NAME and
                      t.ACTION_DATE=V_ACTION_DATE);

      if P_INSERT_ONLY<>1 then
        begin
          insert into WB_REF_WS_AIR_S_L_P_S_P_HST(ID_,
    	                                              U_NAME_,
	                                                    U_IP_,
	                                                      U_HOST_NAME_,
	                                                        DATE_WRITE_,
                                                            OPERATION_,
	                                                            ACTION_,
	                                                              ID,
	                                                                ID_AC,
	                                                                  ID_WS,
		                                                                  ID_BORT,
                                                                        IDN,
	                                                                        ADV_ID,
	                                                                          PLAN_ID,
	                                                                            S_SEAT_ID,
	                                                                              PARAM_ID,
		                                                                              U_NAME,
		                                                                                U_IP,
		                                                                                  U_HOST_NAME,
		                                                                                    DATE_WRITE)
          select SEC_WB_REF_WS_AIR_S_L_P_S_P_HS.nextval,
                   P_U_NAME,
	                   P_U_IP,
	                     P_U_HOST_NAME,
                         SYSDATE(),
                           'delete',
                             'delete',
                               i.id,
                                 i.ID_AC,
	                                 i.ID_WS,
		                                 i.ID_BORT,
                                       i.IDN,
	                                       i.ADV_ID,
	                                         i.PLAN_ID,
	                                           i.S_SEAT_ID,
	                                             i.PARAM_ID,
                                                 i.U_NAME,
		                                               i.U_IP,
		                                                 i.U_HOST_NAME,
		                                 	                 i.DATE_WRITE
          from WB_REF_WS_AIR_S_L_P_S_P i join WB_REF_WS_AIR_S_L_P_S s
          on i.S_SEAT_ID=s.id join WB_TEMP_XML_ID_EX t
             on t.ID=s.plan_id and
                t.f_flt_1=s.seat_id and
                t.ACTION_NAME=V_ACTION_NAME and
                t.ACTION_DATE=V_ACTION_DATE;

          delete from WB_REF_WS_AIR_S_L_P_S_P
          where id in (select i.id
                       from WB_REF_WS_AIR_S_L_P_S_P i join WB_REF_WS_AIR_S_L_P_S s
                       on i.S_SEAT_ID=s.id join WB_TEMP_XML_ID_EX t
                          on t.ID=s.plan_id and
                             t.f_flt_1=s.seat_id and
                             t.ACTION_NAME=V_ACTION_NAME and
                             t.ACTION_DATE=V_ACTION_DATE);

          insert into WB_REF_WS_AIR_S_L_P_S_P (ID,
	                                               ID_AC,
	                                                 ID_WS,
	                                                   ID_BORT,
	                                                     IDN,
	                                                       ADV_ID,
	                                                         PLAN_ID,
	                                                           S_SEAT_ID,
	                                                             PARAM_ID,
	                                                               U_NAME,
	                                                                 U_IP,
	                                                                   U_HOST_NAME,
	                                                                     DATE_WRITE)
          select /*distinct*/ SEC_WB_REF_WS_AIR_S_L_P_S_P.nextval,
                            s.ID_AC,
	                            s.ID_WS,
	                              s.ID_BORT,
	                                s.IDN,
	                                  s.ADV_ID,
                                      s.PLAN_ID,
                                        s.id,
                                          tt.id,
                                            P_U_NAME,
	                                            P_U_IP,
	                                              P_U_HOST_NAME,
                                                  SYSDATE()
          from WB_REF_WS_AIR_S_L_P_S s join WB_TEMP_XML_ID_EX t
          on t.ID=s.plan_id and
             t.f_flt_1=s.seat_id and
             t.ACTION_NAME=V_ACTION_NAME and
             t.ACTION_DATE=V_ACTION_DATE join WB_TEMP_XML_ID tt
             on 1=1;
     end;
   else
     begin
       insert into WB_REF_WS_AIR_S_L_P_S_P (ID,
	                                               ID_AC,
	                                                 ID_WS,
	                                                   ID_BORT,
	                                                     IDN,
	                                                       ADV_ID,
	                                                         PLAN_ID,
	                                                           S_SEAT_ID,
	                                                             PARAM_ID,
	                                                               U_NAME,
	                                                                 U_IP,
	                                                                   U_HOST_NAME,
	                                                                     DATE_WRITE)
          select /*distinct*/ SEC_WB_REF_WS_AIR_S_L_P_S_P.nextval,
                            s.ID_AC,
	                            s.ID_WS,
	                              s.ID_BORT,
	                                s.IDN,
	                                  s.ADV_ID,
                                      s.PLAN_ID,
                                        s.id,
                                          tt.id,
                                            P_U_NAME,
	                                            P_U_IP,
	                                              P_U_HOST_NAME,
                                                  SYSDATE()
          from WB_REF_WS_AIR_S_L_P_S s join WB_TEMP_XML_ID_EX t
          on t.ID=s.plan_id and
             t.f_flt_1=s.seat_id and
             t.ACTION_NAME=V_ACTION_NAME and
             t.ACTION_DATE=V_ACTION_DATE join WB_TEMP_XML_ID tt
             on 1=1
           where not exists(select 1
                            from WB_REF_WS_AIR_S_L_P_S_P pp
                            where pp.s_seat_id=s.id and
                                  pp.param_id=tt.id);
     end;
   end if;

     update WB_REF_WS_AIR_S_L_P_S
      set IS_SEAT=0,
          U_NAME=P_U_NAME,
	        U_IP=P_U_IP,
	        U_HOST_NAME=P_U_HOST_NAME,
	        DATE_WRITE=sysdate()
      where id in (select distinct s.id
                   from WB_REF_WS_AIR_S_L_P_S s join WB_TEMP_XML_ID_EX t
                   on t.ID=s.plan_id and
                      t.f_flt_1=s.seat_id and
                      t.ACTION_NAME=V_ACTION_NAME and
                      t.ACTION_DATE=V_ACTION_DATE join WB_REF_WS_AIR_S_L_P_S_P p
                      on p.s_seat_id=s.id join WB_REF_WS_SEATS_PARAMS sp
                         on sp.id=p.param_id and
                            sp.IS_SEAT=0);

      update WB_REF_WS_AIR_S_L_P_S
      set IS_SEAT=1,
          U_NAME=P_U_NAME,
	        U_IP=P_U_IP,
	        U_HOST_NAME=P_U_HOST_NAME,
	        DATE_WRITE=sysdate()
      where id in (select distinct s.id
                   from WB_REF_WS_AIR_S_L_P_S s join WB_TEMP_XML_ID_EX t
                   on t.ID=s.plan_id and
                      t.f_flt_1=s.seat_id and
                      t.ACTION_NAME=V_ACTION_NAME and
                      t.ACTION_DATE=V_ACTION_DATE
                   where not exists(select 1
                                    from WB_REF_WS_AIR_S_L_P_S_P p join WB_REF_WS_SEATS_PARAMS sp
                                    on p.s_seat_id=s.id and
                                       sp.id=p.param_id and
                                       sp.IS_SEAT=0));



      select count(q.id) into V_R_COUNT
      from (select p.id,
                   p.max_seats,
                   (select count(s.id)
                    from WB_REF_WS_AIR_S_L_P_S s
                    where s.plan_id=p.id and
                          s.is_seat=1) seat_count
           from WB_REF_WS_AIR_S_L_PLAN p
           where p.adv_id=P_ADV_ID) q
      where q.max_seats<q.seat_count;

      if V_R_COUNT>0 then
        begin
          rollback to sp_1;

          if P_LANG='ENG' then V_STR_MSG:='The operation would exceed the number of seats on the value of the field "Maximum Seats" - '||to_char(V_R_COUNT)||' The operation is prohibited!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Выполнение операции приведет к превышению числа мест над значением поля "Maximum Seats" - '||to_char(V_R_COUNT)||'! Операция запрещена!'; end if;
        end;
      end if;

      if V_STR_MSG is null then
        begin
          select NUM_OF_SEATS into V_SEAT_COUNT
          from WB_REF_WS_AIR_SEAT_LAY_ADV
          where id=P_ADV_ID;

          select count(s.id) into V_R_COUNT
          from WB_REF_WS_AIR_S_L_P_S s
          where s.adv_id=P_ADV_ID and
                s.is_seat=1;

          if V_R_COUNT>V_SEAT_COUNT then
            begin
              rollback to sp_1;

              if P_LANG='ENG' then V_STR_MSG:='The total number of seats will exceed the number of places specified in the "Number of Seats"!'; end if;
              if P_LANG='RUS' then V_STR_MSG:='Итоговое число мест будет превышать число мест, заданное в поле "Number of Seats"! '; end if;

              V_STR_MSG:=V_STR_MSG||' '||to_char(V_R_COUNT)||'/'||to_char(V_SEAT_COUNT);
            end;
          end if;
        end;
      end if;

      if V_STR_MSG is null then
        begin
          V_STR_MSG:='EMPTY_STRING';
          commit;
        end;
      end if;
    end;
  end if;

  cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';
end SP_WB_REF_WS_AIR_S_L_P_S_P_UPD;
/
