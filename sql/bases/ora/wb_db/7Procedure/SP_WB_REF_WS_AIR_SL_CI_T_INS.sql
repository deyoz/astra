create or replace procedure SP_WB_REF_WS_AIR_SL_CI_T_INS
(cXML_in in clob,
   cXML_out out clob)
as
P_ADV_ID number:=-1;
P_SL_ADV_ID number:=-1;

P_CLASS_CODE varchar2(50):='';

P_FIRST_ROW number:=null;
P_LAST_ROW number:=null;
P_NUM_OF_SEATS number:=null;
P_BA_CENTROID number:=null;
P_LA_FROM number:=null;
P_LA_TO number:=null;
P_BA_FWD number:=null;
P_BA_AFT number:=null;
P_INDEX_PER_WT_UNIT number:=null;

P_LA_FROM_INT_PART varchar2(50):='NULL';
P_LA_FROM_DEC_PART varchar2(50):='NULL';
P_LA_TO_INT_PART varchar2(50):='NULL';
P_LA_TO_DEC_PART varchar2(50):='NULL';
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
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CLASS_CODE[1]') into P_CLASS_CODE from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ADV_ID[1]')) into P_ADV_ID from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_SL_ADV_ID[1]')) into P_SL_ADV_ID from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_FIRST_ROW[1]')) into P_FIRST_ROW from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LAST_ROW[1]')) into P_LAST_ROW from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_NUM_OF_SEATS[1]')) into P_NUM_OF_SEATS from dual;

  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LA_FROM_INT_PART[1]') into P_LA_FROM_INT_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LA_FROM_DEC_PART[1]') into P_LA_FROM_DEC_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LA_TO_INT_PART[1]') into P_LA_TO_INT_PART from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LA_TO_DEC_PART[1]') into P_LA_TO_DEC_PART from dual;
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

  if P_LA_FROM_INT_PART<>'NULL' then P_LA_FROM:=to_number(P_LA_FROM_INT_PART||'.'||P_LA_FROM_DEC_PART); end if;
  if P_LA_TO_INT_PART<>'NULL' then P_LA_TO:=to_number(P_LA_TO_INT_PART||'.'||P_LA_TO_DEC_PART); end if;
  if P_BA_CENTROID_INT_PART<>'NULL' then P_BA_CENTROID:=to_number(P_BA_CENTROID_INT_PART||'.'||P_BA_CENTROID_DEC_PART); end if;
  if P_BA_FWD_INT_PART<>'NULL' then P_BA_FWD:=to_number(P_BA_FWD_INT_PART||'.'||P_BA_FWD_DEC_PART); end if;
  if P_BA_AFT_INT_PART<>'NULL' then P_BA_AFT:=to_number(P_BA_AFT_INT_PART||'.'||P_BA_AFT_DEC_PART); end if;
  if P_INDEX_PER_WT_UNIT_INT_PART<>'NULL' then P_INDEX_PER_WT_UNIT:=to_number(P_INDEX_PER_WT_UNIT_INT_PART||'.'||P_INDEX_PER_WT_UNIT_DEC_PART); end if;

  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_S_L_C_ADV
  where id=P_ADV_ID;

  if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then V_STR_MSG:='The selected record "Seating Layout"->"Configuration" block deleted!'; end if;
        if P_LANG='RUS' then V_STR_MSG:='Выбранная запись блока "Seating Layout"->"Configuration" удалена!'; end if;
      end;
    end if;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      if P_FIRST_ROW>P_LAST_ROW then
        begin
          if P_LANG='ENG' then V_STR_MSG:='Value "First Row" longer "Last Row"!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Значение "First Row" превосходит "Last Row"!'; end if;
        end;
      end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(id) into V_R_COUNT
      from WB_REF_WS_AIR_SL_CI_T
      where adv_id=p_ADV_ID and
            (FIRST_ROW between P_FIRST_ROW and P_LAST_ROW or
             LAST_ROW between P_FIRST_ROW and P_LAST_ROW or
             P_FIRST_ROW between FIRST_ROW and LAST_ROW or
             P_LAST_ROW between FIRST_ROW and LAST_ROW);

      if V_R_COUNT>0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='There are records with overlapping rows range!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Существуют записи с перекрывающимся диапазоном рядов!'; end if;
        end;
      end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(cc.id) into V_R_COUNT
      from WB_REF_WS_AIR_S_L_C_ADV a join WB_REF_AIRCO_CLASS_CODES cc
      on a.id=P_ADV_ID and
         cc.CLASS_CODE=P_CLASS_CODE and
         ((a.ID_AC<>-1 and cc.ID_AC=a.ID_AC) or
          (a.ID_AC=-1));

      if V_R_COUNT=0 then
        begin
          if P_LANG='ENG' then V_STR_MSG:='Value "Class Code" is not active in the block "Airline Standarts" -> "Units and Codes" -> "Class Codes"!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Значение "Class Code" более не существует в блоке "Airline Standarts"->"Units and Codes"->"Class Codes"!'; end if;
        end;
      end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select NUM_OF_SEATS into V_SEAT_COUNT_ADV
      from WB_REF_WS_AIR_S_L_C_ADV a join WB_REF_WS_AIR_SEAT_LAY_ADV aa
      on a.id=P_ADV_ID and
         a.adv_id=aa.id;

      select sum(NUM_OF_SEATS) into V_SEAT_COUNT_CUR
      from WB_REF_WS_AIR_SL_CI_T
      where ADV_ID=P_ADV_ID;

      if V_SEAT_COUNT_ADV<V_SEAT_COUNT_CUR+P_NUM_OF_SEATS then
        begin
          if P_LANG='ENG' then V_STR_MSG:='The total number of seats will exceed the number of places specified in the "Number of Seats"!'; end if;
          if P_LANG='RUS' then V_STR_MSG:='Итоговое число мест будет превышать число мест, заданное в поле "Number of Seats"! '; end if;

          V_STR_MSG:=V_STR_MSG||' '||to_char(V_SEAT_COUNT_CUR+P_NUM_OF_SEATS)||'/'||to_char(V_SEAT_COUNT_ADV);
        end;
      end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if (V_STR_MSG is null) then
    begin
      V_ID:=SEC_WB_REF_WS_AIR_SL_CI_T.nextval();

      insert into WB_REF_WS_AIR_SL_CI_T (ID,
	                                         ID_AC,
                                             ID_WS,
	                                             ID_BORT,
	                                               IDN,
                                                   ADV_ID,
	                                                   CLASS_CODE,
	                                                     FIRST_ROW,
	                                                       LAST_ROW,
	                                                         NUM_OF_SEATS,
	                                                           LA_FROM,
	                                                             LA_TO,	
	                                                               BA_CENTROID,
	                                                                 BA_FWD,
	                                                                   BA_AFT,
	                                                                     INDEX_PER_WT_UNIT,
	                                                                       U_NAME,
	                                                                         U_IP,
	                                                                           U_HOST_NAME,
	                                                                             DATE_WRITE)
      select V_ID,
               ID_AC,
                 ID_WS,
	                 ID_BORT,
	                   IDN,
                       P_ADV_ID,
                         P_CLASS_CODE,
	                         P_FIRST_ROW,
	                           P_LAST_ROW,
	                             P_NUM_OF_SEATS,
	                               P_LA_FROM,
	                                 P_LA_TO,	
	                                   P_BA_CENTROID,
                                       P_BA_FWD,
	                                       P_BA_AFT,	
	                                         P_INDEX_PER_WT_UNIT,
	                                           P_U_NAME,
	                                             P_U_IP,
	                                               P_U_HOST_NAME,
                                                   sysdate()
      from WB_REF_WS_AIR_S_L_C_ADV
      where id=P_ADV_ID;

      V_STR_MSG:='EMPTY_STRING';
    end;
  end if;

  cXML_out:=cXML_out||'<list id="'||to_number(V_ID)||'" str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

  commit;
end SP_WB_REF_WS_AIR_SL_CI_T_INS;
/
