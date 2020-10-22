create or replace procedure SP_WB_REF_WS_AIR_STLY_ADV_UPD
(cXML_in in clob,
   cXML_out out clob)
as
P_ADV_ID number:=-1;
P_LANG varchar2(50):='';

P_TABLE_NAME varchar2(200):='';
P_CH_BALANCE_ARM number:=null;
P_CH_INDEX_UNIT number:=null;
P_CABIN_ID number:=null;
P_NUMBER_OF_SEATS number:=null;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT number:=0;
V_CABIN_ID number:=null;
begin
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LANG[1]') into P_LANG from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ADV_ID[1]')) into P_ADV_ID from dual;

  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_TABLE_NAME[1]') into P_TABLE_NAME from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_BALANCE_ARM[1]')) into P_CH_BALANCE_ARM from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_INDEX_UNIT[1]')) into P_CH_INDEX_UNIT from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CABIN_ID[1]')) into P_CABIN_ID from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_NUMBER_OF_SEATS[1]')) into P_NUMBER_OF_SEATS from dual;

  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_NAME[1]') into P_U_NAME from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_IP[1]') into P_U_IP from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

  cXML_out:='<?xml version="1.0" ?><root>';


  insert into WB_TEMP_XML_ID (ID,
                                num)
  select f.ID,
           f.ID
  from (select to_number(extractValue(value(t),'seat_id_data/P_ID[1]')) as ID
        from table(xmlsequence(xmltype(cXML_in).extract('//seat_id_data'))) t) f;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_SEAT_LAY_ADV
  where id=P_ADV_ID;

  if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then V_STR_MSG:='The selected record is removed!'; end if;
        if P_LANG='RUS' then V_STR_MSG:='Выбранная запись удалена!'; end if;
      end;
    end if;
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(c.id) into V_R_COUNT
      from WB_REF_WS_AIR_SEAT_LAY_ADV a join WB_REF_WS_AIR_CABIN_IDN c
      on a.id=P_ADV_ID and
         c.id_ac=a.id_ac and
         c.id_ws=a.id_ws and
         c.id_bort=a.id_bort and
         c.id=P_CABIN_ID;

      if V_R_COUNT=0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='The selected record "Cabin" block deleted!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Выбранная запись блока "Cabin" удалена!'; end if;
        end;
      end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  /*if V_STR_MSG is null then
    begin
      select count(slps.id) into V_R_COUNT
      from WB_REF_WS_AIR_S_L_P_S slps
      where slps.adv_id=P_ADV_ID and
            not exists(select t.id
                       from WB_TEMP_XML_ID t
                       where t.id=slps.seat_id);

      if V_R_COUNT>0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='On the value of the block "Use Seat Identifiers" are referenced in the section "Seat Identifier"!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='На значения блока "Use Seat Identifiers" имеются ссылки в блоке "Seat Identifier"!'; end if;
        end;
      end if;

    end;
  end if;*/
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(id) into V_R_COUNT
      from WB_TEMP_XML_ID t
      where not exists(select 1
                       from WB_REF_WS_SEATS_NAMES s
                       where s.id=t.id);

      if V_R_COUNT>0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='Some block records "Use Seat Identifiers" are removed from the directory!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Некоторые записи блока "Use Seat Identifiers" удалены из справочника!'; end if;
        end;
      end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      if P_CABIN_ID>0 then
        begin
          select cabin_id into V_CABIN_ID
          from WB_REF_WS_AIR_SEAT_LAY_ADV
          where id=P_ADV_ID;

          if (V_CABIN_ID>0) and
               (V_CABIN_ID<>P_CABIN_ID) then
            begin
              select count(id) into V_R_COUNT
              from WB_REF_WS_AIR_S_L_PLAN
              where adv_id=P_ADV_ID;

              if V_R_COUNT>0 then
                begin
                  if P_LANG='ENG' then V_STR_MSG:='There are entries in the rows of the distribution table. Changing the value of the field "Cabin" it is prohibited!'; end if;
                  if P_LANG='RUS' then V_STR_MSG:='Имеются записи в таблице распределения рядов. Изменение значения поля "Cabin" запрещено!'; end if;
                end;
              end if;
            end;
          end if;
        end;
      end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(id) into V_R_COUNT
      from WB_REF_WS_AIR_S_L_P_S
      where adv_id=P_ADV_ID and
            is_seat=1;

      if P_NUMBER_OF_SEATS<V_R_COUNT then
        begin
          if P_LANG='ENG' then V_STR_MSG:='The total number of seats will exceed the number of places specified in the "Number of Seats"!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Итоговое число мест будет превышать число мест, заданное в поле "Number of Seats"! '; end if;

          V_STR_MSG:=V_STR_MSG||' '||to_char(V_R_COUNT)||'/'||to_char(P_NUMBER_OF_SEATS);
        end;
      end if;

    end;
  end if;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(s.id) into V_R_COUNT
      from WB_REF_WS_AIR_S_L_P_S s
      where s.adv_id=P_ADV_ID and
            not exists(select 1
                       from WB_TEMP_XML_ID t
                       where t.id=s.seat_id);

      if V_R_COUNT>0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='There are entries in the rows of the distribution table. Such a change in the combination of "Use Seat Identifiers" is forbidden!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Имеются записи в таблице распределения рядов. Такое изменение комбинации "Use Seat Identifiers" запрещено!'; end if;
        end;
      end if;

    end;
  end if;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(q.id) into V_R_COUNT
      from (select distinct a.id,
                            sum(tt.NUM_OF_SEATS) cnt
            from WB_REF_WS_AIR_S_L_C_ADV a join WB_REF_WS_AIR_SL_CAI_T t
            on a.adv_id=P_ADV_ID and
               a.id=t.adv_id join WB_REF_WS_AIR_SL_CAI_TT tt
               on tt.t_id=t.id
            group by a.id) q
      where q.cnt>P_NUMBER_OF_SEATS;

      if V_R_COUNT>0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='There are blocks "Configuration"->"Cabin Area Information", the number of places that will exceed the specified in the "Number of Seats"!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Имеются блоки "Configuration"->"Cabin Area Information", число мест в которых будет превышать заданное в поле "Number of Seats"!'; end if;
        end;
      end if;

    end;
  end if;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(q.id) into V_R_COUNT
      from (select distinct a.id,
                            sum(t.NUM_OF_SEATS) cnt
            from WB_REF_WS_AIR_S_L_C_ADV a join WB_REF_WS_AIR_SL_CI_T t
            on a.adv_id=P_ADV_ID and
               a.id=t.adv_id
            group by a.id) q
      where q.cnt>P_NUMBER_OF_SEATS;

      if V_R_COUNT>0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='There are blocks "Configuration"->"Class Information", the number of places that will exceed the specified in the "Number of Seats"!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Имеются блоки "Configuration"->"Class Information", число мест в которых будет превышать заданное в поле "Number of Seats"!'; end if;
        end;
      end if;

    end;
  end if;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if (V_STR_MSG is null) then
    begin
      update WB_REF_WS_AIR_SEAT_LAY_ADV
      set CH_BALANCE_ARM=P_CH_BALANCE_ARM,
	        CH_INDEX_UNIT=P_CH_INDEX_UNIT,
	        NUM_OF_SEATS=P_NUMBER_OF_SEATS,
	        CABIN_ID=P_CABIN_ID,
	        U_NAME=P_U_NAME,
	        U_IP=P_U_IP,
	        U_HOST_NAME=P_U_HOST_NAME,	
	        TABLE_NAME=P_TABLE_NAME,
          DATE_WRITE=sysdate()
      where id=P_ADV_ID;

      insert into WB_REF_WS_AIR_S_L_A_U_S_HST(ID_,
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
	                                                                      SEAT_ID,
		                                                                      U_NAME,
		                                                                        U_IP,
		                                                                          U_HOST_NAME,
		                                                                            DATE_WRITE)
      select SEC_WB_REF_WS_AIR_SLAUS_HST.nextval,
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
	                                     i.SEAT_ID,
                                         i.U_NAME,
		                                     	 i.U_IP,
		                                         i.U_HOST_NAME,
		                                 	         i.DATE_WRITE
      from WB_REF_WS_AIR_S_L_A_U_S i
      where i.ADV_ID=P_ADV_ID;

      delete from WB_REF_WS_AIR_S_L_A_U_S where ADV_ID=P_ADV_ID;

     insert into WB_REF_WS_AIR_S_L_A_U_S (ID,
	                                          ID_AC,
	                                            ID_WS,
		                                            ID_BORT,
                                                  IDN,
	                                                  ADV_ID,
	                                                    SEAT_ID,
		                                                    U_NAME,
		                                                      U_IP,
		                                                        U_HOST_NAME,
		                                                          DATE_WRITE)
     select SEC_WB_REF_WS_AIR_SLAUS.nextval,
              a.ID_AC,
	              a.ID_WS,
		              a.ID_BORT,
                    a.IDN,
                      a.id,
                        t.id,
                          a.U_NAME,
		                        a.U_IP,
		                          a.U_HOST_NAME,
                                a.DATE_WRITE
     from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SEAT_LAY_ADV a
     on a.id=P_ADV_ID;

      V_STR_MSG:='EMPTY_STRING';
    end;
  end if;

  cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';
  commit;
end SP_WB_REF_WS_AIR_STLY_ADV_UPD;
/
