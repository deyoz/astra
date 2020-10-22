create or replace procedure SP_WB_REF_WS_AIR_S_L_PLAN_UPD
(cXML_in in clob,
   cXML_out out clob)
as
P_ADV_ID number:=-1;
P_ID number:=-1;

P_CABIN_SECTION varchar2(50):='';
P_ROW_NUMBER number:=null;
P_MAX_WEIGHT number:=null;
P_MAX_SEATS number:=null;
P_BALANCE_ARM number:=null;
P_INDEX_PER_WT_UNIT number:=null;

P_MAX_WEIGHT_INT_PART varchar2(50):='NULL';
P_MAX_WEIGHT_DEC_PART varchar2(50):='NULL';
P_BALANCE_ARM_INT_PART varchar2(50):='NULL';
P_BALANCE_ARM_DEC_PART varchar2(50):='NULL';
P_INDEX_PER_WT_UNIT_INT_PART varchar2(50):='NULL';
P_INDEX_PER_WT_UNIT_DEC_PART varchar2(50):='NULL';

P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';

cXML_data clob;
V_R_COUNT number:=0;
V_SEAT_COUNT number:=0;
V_STR_MSG clob:=null;
begin

  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LANG[1]') into P_LANG from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CABIN_SECTION[1]') into P_CABIN_SECTION from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ROW_NUMBER[1]')) into P_ROW_NUMBER from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ADV_ID[1]')) into P_ADV_ID from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ID[1]')) into P_ID from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_MAX_SEATS[1]')) into P_MAX_SEATS from dual;

  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_MAX_WEIGHT_INT_PART[1]') into P_MAX_WEIGHT_INT_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_MAX_WEIGHT_DEC_PART[1]') into P_MAX_WEIGHT_DEC_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_BALANCE_ARM_INT_PART[1]') into P_BALANCE_ARM_INT_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_BALANCE_ARM_DEC_PART[1]') into P_BALANCE_ARM_DEC_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_INDEX_PER_WT_UNIT_INT_PART[1]') into P_INDEX_PER_WT_UNIT_INT_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_INDEX_PER_WT_UNIT_DEC_PART[1]') into P_INDEX_PER_WT_UNIT_DEC_PART from dual;

  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_NAME[1]') into P_U_NAME from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_IP[1]') into P_U_IP from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

  cXML_out:='<?xml version="1.0" ?><root>';

  if P_MAX_WEIGHT_INT_PART<>'NULL' then P_MAX_WEIGHT:=to_number(P_MAX_WEIGHT_INT_PART||'.'||P_MAX_WEIGHT_DEC_PART); end if;
  if P_BALANCE_ARM_INT_PART<>'NULL' then P_BALANCE_ARM:=to_number(P_BALANCE_ARM_INT_PART||'.'||P_BALANCE_ARM_DEC_PART); end if;
  if P_INDEX_PER_WT_UNIT_INT_PART<>'NULL' then P_INDEX_PER_WT_UNIT:=to_number(P_INDEX_PER_WT_UNIT_INT_PART||'.'||P_INDEX_PER_WT_UNIT_DEC_PART); end if;

  insert into WB_TEMP_XML_ID (ID,
                                num)
  select distinct f.ID,
                    f.ID
  from (select to_number(extractValue(value(t),'seat_id_data/P_ID[1]')) as ID
        from table(xmlsequence(xmltype(cXML_in).extract('//seat_id_data'))) t) f;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_S_L_PLAN
  where id=P_ID;

  if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then V_STR_MSG:='The selected record is deleted!'; end if;
        if P_LANG='RUS' then V_STR_MSG:='Выбранная запись удалена!'; end if;
      end;
    end if;
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(cd.id) into V_R_COUNT
      from WB_REF_WS_AIR_SEAT_LAY_ADV a join WB_REF_WS_AIR_CABIN_CD cd
      on a.id=P_ADV_ID and
         a.cabin_id=cd.idn and
         cd.section=P_CABIN_SECTION;

      if V_R_COUNT=0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='The selected value of the field "Section" is not active in the block "Cabin" -> "Cabin Definitions"!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Выбранное значение поля "Section" более не существует в блоке "Cabin"->"Cabin Definitions"!'; end if;
        end;
      end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(p.id) into V_R_COUNT
      from WB_REF_WS_AIR_S_L_PLAN p
      where p.adv_id=P_ADV_ID and
            p.CABIN_SECTION=P_CABIN_SECTION and
            p.ROW_NUMBER=P_ROW_NUMBER and
            p.id<>P_ID;

      if V_R_COUNT>0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='For the field "Section" record already exists with the value of the field "Row Number"!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Для поля "Section" уже существует запись с таким значением поля "Row Number"!'; end if;
        end;
      end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(cd.id) into V_R_COUNT
      from WB_REF_WS_AIR_SEAT_LAY_ADV a join WB_REF_WS_AIR_CABIN_CD cd
      on a.id=P_ADV_ID and
         a.cabin_id=cd.idn and
         cd.section=P_CABIN_SECTION and
         P_ROW_NUMBER between cd.rows_from and cd.rows_to;

      if V_R_COUNT=0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='For the field "Section" field value "Row Number" outside the range specified in the block "Cabin" -> "Cabin Definitions"!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Для поля "Section" значение поля "Row Number" выходит за диапазон, заданный в блоке "Cabin"->"Cabin Definitions"!'; end if;
        end;
      end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(t.id) into V_R_COUNT
      from WB_TEMP_XML_ID t
      where not exists(select sls.id
                       from WB_REF_WS_AIR_S_L_A_U_S sls
                       where sls.seat_id=t.id and
                             sls.adv_id=P_ADV_ID);

      if V_R_COUNT>0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='Some values "Seat Identifier" are no longer in the block "Use Seat Identifiers"!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Некоторые значения "Seat Identifier" уже отсутствуют в блоке "Use Seat Identifiers"!'; end if;
        end;
      end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  /*
  if V_STR_MSG is null then
    begin
      select count(id) into V_R_COUNT
      from WB_REF_WS_AIR_S_L_P_S
      where adv_id=P_ADV_ID and
            is_seat=1 and
            plan_id<>P_ID;

      select count(id) into V_SEAT_COUNT
      from WB_TEMP_XML_ID;

      V_R_COUNT:=V_R_COUNT+V_SEAT_COUNT;

      V_SEAT_COUNT:=0;

      select NUM_OF_SEATS into V_SEAT_COUNT
      from WB_REF_WS_AIR_SEAT_LAY_ADV
      where id=P_ADV_ID;

      if (V_R_COUNT>V_SEAT_COUNT) then
        begin
          if P_LANG='ENG' then V_STR_MSG:='The total number of seats will exceed the number of places specified in the "Number of Seats"!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Итоговое число мест будет превышать число мест, заданное в поле "Number of Seats"! '; end if;

          V_STR_MSG:=V_STR_MSG||' '||to_char(V_R_COUNT)||'/'||to_char(V_SEAT_COUNT);
        end;
      end if;
    end;
  end if;*/
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(sls.id) into V_R_COUNT
      from WB_REF_WS_AIR_S_L_A_U_S sls
      where sls.adv_id=P_ADV_ID;

      if V_R_COUNT<P_MAX_SEATS then
        begin
          if P_LANG='ENG' then V_STR_MSG:='The field "Maximum Seats" exceeds the number of places specified in the "Use Seat Identifiers" block "!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Значение поля "Maximum Seats" превышает число мест, заданных в блоке "Use Seat Identifiers"! '; end if;

          V_STR_MSG:=V_STR_MSG||' '||to_char(P_MAX_SEATS)||'/'||to_char(V_R_COUNT);
        end;
      end if;

    end;
  end if;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if (V_STR_MSG is null) then
    begin
      savepoint sp_1;

      update WB_REF_WS_AIR_S_L_PLAN
      set CABIN_SECTION=P_CABIN_SECTION,
	        ROW_NUMBER=P_ROW_NUMBER,
          MAX_WEIGHT=P_MAX_WEIGHT,
	        MAX_SEATS=P_MAX_SEATS,
	        BALANCE_ARM=P_BALANCE_ARM,
	        INDEX_PER_WT_UNIT=P_INDEX_PER_WT_UNIT,
	        U_NAME=P_U_NAME,
	        U_IP=P_U_IP,
	        U_HOST_NAME=P_U_HOST_NAME,
	        DATE_WRITE=sysdate()
      where id=P_ID;

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
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
    on i.S_SEAT_ID=s.id and
       i.PLAN_ID=P_ID and
       not exists(select 1
                  from WB_TEMP_XML_ID t
                  where t.id=s.SEAT_ID);

    delete from WB_REF_WS_AIR_S_L_P_S_P
    where id in (select distinct i.id
                 from WB_REF_WS_AIR_S_L_P_S_P i join WB_REF_WS_AIR_S_L_P_S s
                 on i.S_SEAT_ID=s.id and
                    i.PLAN_ID=P_ID and
                    not exists(select 1
                               from WB_TEMP_XML_ID t
                               where t.id=s.SEAT_ID));
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_S_L_P_S_HST(ID_,
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
	                                                                    SEAT_ID,
	                                                                      IS_SEAT,
		                                                                      U_NAME,
		                                                                        U_IP,
		                                                                          U_HOST_NAME,
		                                                                            DATE_WRITE)
    select SEC_WB_REF_WS_AIR_S_L_P_S_HST.nextval,
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
	                                     i.SEAT_ID,
	                                       i.IS_SEAT,
                                           i.U_NAME,
		                                         i.U_IP,
		                                           i.U_HOST_NAME,
		                                 	           i.DATE_WRITE
    from WB_REF_WS_AIR_S_L_P_S i
    where i.plan_id=P_ID and
          not exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.id=i.SEAT_ID);

    delete from WB_REF_WS_AIR_S_L_P_S
    where plan_id=P_ID and
          not exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.id=WB_REF_WS_AIR_S_L_P_S.SEAT_ID);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
     insert into WB_REF_WS_AIR_S_L_P_S (ID,
	                                         ID_AC,
                                             ID_WS,
	                                             ID_BORT,
	                                               IDN,
                                                   ADV_ID,
                                                     PLAN_ID,
	                                                     SEAT_ID,
	                                                       IS_SEAT,
                                                           U_NAME,
	                                                           U_IP,
	                                                             U_HOST_NAME,
	                                                               DATE_WRITE,
                                                                   MAX_WEIGHT)
      select SEC_WB_REF_WS_AIR_S_L_P_S.nextval,
               a.ID_AC,
                 a.ID_WS,
	                 a.ID_BORT,
	                   a.IDN,
                       P_ADV_ID,
                         P_ID,
                           t.id,
                             1,
                               P_U_NAME,
	                               P_U_IP,
	                                 P_U_HOST_NAME,
                                     sysdate(),
                                       100
      from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SEAT_LAY_ADV a
      on a.id=P_ADV_ID and
         not exists(select 1
                    from WB_REF_WS_AIR_S_L_P_S s
                    where s.plan_id=P_ID and
                          s.seat_id=t.id);

      -----------------------------------------------------------------------------------------------------------
      -----------------------------------------------------------------------------------------------------------

      select count(q.id) into V_R_COUNT
      from (select p.id,
                   p.max_seats,
                   (select count(s.id)
                    from WB_REF_WS_AIR_S_L_P_S s
                    where s.plan_id=p.id and
                          s.is_seat=1) seat_count
           from WB_REF_WS_AIR_S_L_PLAN p
           where p.ID=P_ID) q
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
end SP_WB_REF_WS_AIR_S_L_PLAN_UPD;
/
