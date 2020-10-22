create or replace procedure SP_WB_REF_WS_AIR_SL_CAI_T_UPD
(cXML_in in clob,
   cXML_out out clob)
as
P_ADV_ID number:=-1;
P_SL_ADV_ID number:=-1;
P_ID number:=-1;

P_CABIN_SECTION varchar2(50):='';


P_BA_CENTROID number:=null;
P_BA_FWD number:=null;
P_BA_AFT number:=null;
P_INDEX_PER_WT_UNIT number:=null;

P_BA_CENTROID_INT_PART varchar2(50):='NULL';
P_BA_CENTROID_DEC_PART varchar2(50):='NULL';
P_BA_FWD_INT_PART varchar2(50):='NULL';
P_BA_FWD_DEC_PART varchar2(50):='NULL';
P_BA_AFT_INT_PART varchar2(50):='NULL';
P_BA_AFT_DEC_PART varchar2(50):='NULL';
P_INDEX_PER_WT_UNIT_INT_PART varchar2(50):='NULL';
P_INDEX_PER_WT_UNIT_DEC_PART varchar2(50):='NULL';

P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';

cXML_data clob;
V_R_COUNT number:=0;
V_SEAT_COUNT_ADV number:=0;
V_SEAT_COUNT_CUR number:=0;
V_STR_MSG clob:=null;
V_ID number:=-1;
begin

  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LANG[1]') into P_LANG from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CABIN_SECTION[1]') into P_CABIN_SECTION from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ADV_ID[1]')) into P_ADV_ID from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_SL_ADV_ID[1]')) into P_SL_ADV_ID from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ID[1]')) into P_ID from dual;

  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_BA_CENTROID_INT_PART[1]') into P_BA_CENTROID_INT_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_BA_CENTROID_DEC_PART[1]') into P_BA_CENTROID_DEC_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_BA_FWD_INT_PART[1]') into P_BA_FWD_INT_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_BA_FWD_DEC_PART[1]') into P_BA_FWD_DEC_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_BA_AFT_INT_PART[1]') into P_BA_AFT_INT_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_BA_AFT_DEC_PART[1]') into P_BA_AFT_DEC_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_INDEX_PER_WT_UNIT_INT_PART[1]') into P_INDEX_PER_WT_UNIT_INT_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_INDEX_PER_WT_UNIT_DEC_PART[1]') into P_INDEX_PER_WT_UNIT_DEC_PART from dual;

  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_NAME[1]') into P_U_NAME from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_IP[1]') into P_U_IP from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

  cXML_out:='<?xml version="1.0" ?><root>';

  if P_BA_CENTROID_INT_PART<>'NULL' then P_BA_CENTROID:=to_number(P_BA_CENTROID_INT_PART||'.'||P_BA_CENTROID_DEC_PART); end if;
  if P_BA_FWD_INT_PART<>'NULL' then P_BA_FWD:=to_number(P_BA_FWD_INT_PART||'.'||P_BA_FWD_DEC_PART); end if;
  if P_BA_AFT_INT_PART<>'NULL' then P_BA_AFT:=to_number(P_BA_AFT_INT_PART||'.'||P_BA_AFT_DEC_PART); end if;
  if P_INDEX_PER_WT_UNIT_INT_PART<>'NULL' then P_INDEX_PER_WT_UNIT:=to_number(P_INDEX_PER_WT_UNIT_INT_PART||'.'||P_INDEX_PER_WT_UNIT_DEC_PART); end if;


  insert into WB_TEMP_XML_ID (ID,
                                num,
                                  STRING_VAL)
  select distinct f.P_SEATS_COUNT,
                    f.P_SEATS_COUNT,
                      f.P_CLASS_CODE
  from (select to_number(extractValue(value(t),'class_code_data/P_SEATS_COUNT[1]')) as P_SEATS_COUNT,
                 extractValue(value(t),'class_code_data/P_CLASS_CODE[1]') as P_CLASS_CODE
        from table(xmlsequence(xmltype(cXML_in).extract('//class_code_data'))) t) f;

  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_SL_CAI_T
  where id=P_ID;

  if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then V_STR_MSG:='The selected record "Configuration" block deleted!'; end if;
        if P_LANG='RUS' then V_STR_MSG:='Выбранная запись блока "Configuration" удалена!'; end if;
      end;
    end if;

  if V_STR_MSG is null then
    begin
      select count(id) into V_R_COUNT
      from WB_REF_WS_AIR_SL_CAI_T
      where adv_id=P_ADV_ID and
            CABIN_SECTION=P_CABIN_SECTION and
            id<>P_ID;

      if V_R_COUNT>0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='Recording with the value of the field "Section" already exists!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Запись с таким значением поля "Seсtion" уже существует!'; end if;
        end;
      end if;

    end;
  end if;
  if V_STR_MSG is null then
    begin
      select count(cd.id) into V_R_COUNT
      from WB_REF_WS_AIR_SL_CAI_T t join WB_REF_WS_AIR_S_L_C_ADV a
      on t.id=P_ID and
         t.adv_id=a.id join WB_REF_WS_AIR_SEAT_LAY_ADV aa
         on a.id=P_ADV_ID and
            a.adv_id=aa.id join WB_REF_WS_AIR_CABIN_CD cd
              on aa.cabin_id=cd.idn and
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
      select count(t.id) into V_R_COUNT
      from WB_TEMP_XML_ID t
      where not exists(select 1
                       from WB_REF_WS_AIR_SL_CAI_T a join WB_REF_AIRCO_CLASS_CODES cc
                       on a.id=P_ID and
                          cc.CLASS_CODE=t.STRING_VAL and
                          ((a.ID_AC<>-1 and cc.ID_AC=a.ID_AC) or
                           (a.ID_AC=-1)));

      if V_R_COUNT>0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='Some values "Class Codes" is not active in the block "Airline Standarts" -> "Units and Codes" -> "Class Codes"!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Некоторые значения "Class Codes" более не существует в блоке "Airline Standarts"->"Units and Codes"->"Class Codes"!'; end if;
        end;
      end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select NUM_OF_SEATS into V_SEAT_COUNT_ADV
      from WB_REF_WS_AIR_SL_CAI_T t join WB_REF_WS_AIR_S_L_C_ADV a
      on t.id=P_ID and
         t.adv_id=a.id join WB_REF_WS_AIR_SEAT_LAY_ADV aa
         on a.id=P_ADV_ID and
            a.adv_id=aa.id;

      select sum(NUM_OF_SEATS) into V_SEAT_COUNT_CUR
      from WB_REF_WS_AIR_SL_CAI_TT
      where ADV_ID=P_ADV_ID and
            T_ID<>P_ID;

      select sum(id) into V_R_COUNT
      from WB_TEMP_XML_ID;

      if V_SEAT_COUNT_ADV<V_SEAT_COUNT_CUR+V_R_COUNT then
        begin
          if P_LANG='ENG' then V_STR_MSG:='The total number of seats will exceed the number of places specified in the "Number of Seats"!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Итоговое число мест будет превышать число мест, заданное в поле "Number of Seats"! '; end if;

          V_STR_MSG:=V_STR_MSG||' '||to_char(V_SEAT_COUNT_CUR+V_R_COUNT)||'/'||to_char(V_SEAT_COUNT_ADV);
        end;
      end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if (V_STR_MSG is null) then
    begin
      update WB_REF_WS_AIR_SL_CAI_T
      set CABIN_SECTION=P_CABIN_SECTION,
	        BA_CENTROID=P_BA_CENTROID,
	        BA_FWD=P_BA_FWD,
	        BA_AFT=P_BA_AFT,
	        INDEX_PER_WT_UNIT=P_INDEX_PER_WT_UNIT,
          U_NAME=P_U_NAME,
	        U_IP=P_U_IP,
	        U_HOST_NAME=P_U_HOST_NAME,	
          DATE_WRITE=sysdate()
      where id=P_ID;

    insert into WB_REF_WS_AIR_SL_CAI_TT_HST(ID_,
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
	                                                                    T_ID,
	                                                                      CLASS_CODE,
	                                                                        NUM_OF_SEATS,
		                                                                        U_NAME,
		                                                                          U_IP,
		                                                                            U_HOST_NAME,
		                                                                              DATE_WRITE)
    select SEC_WB_REF_WS_AIR_SL_CAI_TT_HS.nextval,
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
	                                   i.T_ID,
	                                     i.CLASS_CODE,
	                                       i.NUM_OF_SEATS,
                                           i.U_NAME,
		                                         i.U_IP,
		                                           i.U_HOST_NAME,
		                                 	           i.DATE_WRITE
    from WB_REF_WS_AIR_SL_CAI_TT i
    where i.t_id=P_ID and
          not exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.string_val=i.class_code);

      delete from WB_REF_WS_AIR_SL_CAI_TT
      where t_id=P_ID and
            not exists(select 1
                       from WB_TEMP_XML_ID t
                       where t.string_val=WB_REF_WS_AIR_SL_CAI_TT.class_code);


     ---------------------------------------------------------------------------
     insert into WB_TEMP_XML_ID_KEY (ID,
                                       NUM_VAL_1)
     select distinct a.id,
                       t.id
     from  WB_TEMP_XML_ID t join  WB_REF_WS_AIR_SL_CAI_TT a
     on a.t_id=P_ID and
        a.class_code=t.string_val;

     update
      (select a.id,
              a.NUM_OF_SEATS,
              t.NUM_VAL_1 t_NUM_OF_SEATS
      from WB_TEMP_XML_ID_KEY t join  WB_REF_WS_AIR_SL_CAI_TT a
      on a.id=t.id) set NUM_OF_SEATS=t_NUM_OF_SEATS;

     /*
     for a in (select a.id,
                      t.id t_NUM_OF_SEATS
     from WB_TEMP_XML_ID t join  WB_REF_WS_AIR_SL_CAI_TT a
     on a.t_id=P_ID and
        a.class_code=t.string_val) loop
     update WB_REF_WS_AIR_SL_CAI_TT set NUM_OF_SEATS=a.t_NUM_OF_SEATS  where id=a.id;
     end loop;*/


      insert into WB_REF_WS_AIR_SL_CAI_TT (ID,
	                                           ID_AC,
	                                             ID_WS,
	                                               ID_BORT,
	                                                 IDN,
	                                                   ADV_ID,
	                                                     T_ID,
	                                                       CLASS_CODE,
	                                                         NUM_OF_SEATS,
	                                                           U_NAME,
	                                                             U_IP,
	                                                               U_HOST_NAME,
	                                                                 DATE_WRITE)
      select SEC_WB_REF_WS_AIR_SL_CAI_TT.nextval,
               a.ID_AC,
                 a.ID_WS,
	                 a.ID_BORT,
	                   a.IDN,
                       a.ADV_ID,
                         a.ID,
                           t.string_val,
                             t.id,
                               P_U_NAME,
	                               P_U_IP,
	                                 P_U_HOST_NAME,
                                     sysdate()
      from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SL_CAI_T a
      on a.id=P_ID
      where not exists(select 1
                       from WB_REF_WS_AIR_SL_CAI_TT tt
                       where tt.T_ID=a.ID and
                             tt.class_code=t.string_val);

      V_STR_MSG:='EMPTY_STRING';
    end;
  end if;

  cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

  commit;
end SP_WB_REF_WS_AIR_SL_CAI_T_UPD;
/
