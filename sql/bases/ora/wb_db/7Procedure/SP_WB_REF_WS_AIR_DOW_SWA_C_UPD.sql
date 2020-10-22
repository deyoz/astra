create or replace procedure SP_WB_REF_WS_AIR_DOW_SWA_C_UPD
(cXML_in in clob,
   cXML_out out clob)
as

P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

P_CODE_NAME_1 varchar2(200):='';
P_CODE_NAME_2 varchar2(200):='';

P_WEIGHT NUMBER:=null;
P_BALANCE_ARM NUMBER:=null;
P_INDEX_UNIT NUMBER:=null;

P_WEIGHT_INT_PART varchar2(50):='NULL';
P_WEIGHT_DEC_PART varchar2(50):='NULL';
P_BALANCE_ARM_INT_PART varchar2(50):='NULL';
P_BALANCE_ARM_DEC_PART varchar2(50):='NULL';
P_INDEX_UNIT_INT_PART varchar2(50):='NULL';
P_INDEX_UNIT_DEC_PART varchar2(50):='NULL';

P_REMARKS varchar2(1000):='';

P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';

P_ID number:=-1;
V_STR_MSG clob:=null;
V_R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CODE_NAME_1[1]') into P_CODE_NAME_1 from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CODE_NAME_2[1]') into P_CODE_NAME_2 from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_REMARKS[1]') into P_REMARKS from dual;


    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_WEIGHT_INT_PART[1]') into P_WEIGHT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_WEIGHT_DEC_PART[1]') into P_WEIGHT_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BALANCE_ARM_INT_PART[1]') into P_BALANCE_ARM_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BALANCE_ARM_DEC_PART[1]') into P_BALANCE_ARM_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_INDEX_UNIT_INT_PART[1]') into P_INDEX_UNIT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_INDEX_UNIT_DEC_PART[1]') into P_INDEX_UNIT_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    if P_WEIGHT_INT_PART<>'NULL' then P_WEIGHT:=to_number(P_WEIGHT_INT_PART||'.'||P_WEIGHT_DEC_PART); end if;
    if P_BALANCE_ARM_INT_PART<>'NULL' then P_BALANCE_ARM:=to_number(P_BALANCE_ARM_INT_PART||'.'||P_BALANCE_ARM_DEC_PART); end if;
    if P_INDEX_UNIT_INT_PART<>'NULL' then P_INDEX_UNIT:=to_number(P_INDEX_UNIT_INT_PART||'.'||P_INDEX_UNIT_DEC_PART); end if;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_DOW_SWA_CODES
    where id=P_ID;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='The selected record is removed!';
          end;
        else
          begin
            V_STR_MSG:='Выбранная запись удалена!';
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_RESERVED_PHRASE
        where PHRASE=P_CODE_NAME_1;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 V_STR_MSG:='Value '||P_CODE_NAME_1||' is a phrase reserved!';
              end;
            else
              begin
                V_STR_MSG:='Значение '||P_CODE_NAME_1||' является зарезервированной фразой!';
              end;
            end if;
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_DOW_SWA_CODES
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT and
              CODE_NAME_1=P_CODE_NAME_1 and
              id<>P_ID;

      if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 V_STR_MSG:='Recording with the value of the field "Adjustment Code Name" already exists!';
              end;
            else
              begin
                V_STR_MSG:='Запись с таким значением поля "Adjustment Code Name" уже существует!';
              end;
            end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        if P_CODE_NAME_2<>'EMPTY_STRING' then
          begin
            select count(id) into V_R_COUNT
            from WB_REF_RESERVED_PHRASE
            where PHRASE=P_CODE_NAME_2;

            if V_R_COUNT>0 then
              begin
                if P_LANG='ENG' then
                  begin
                     V_STR_MSG:='Value '||P_CODE_NAME_2||' is a phrase reserved!';
                  end;
                else
                  begin
                    V_STR_MSG:='Значение '||P_CODE_NAME_2||' является зарезервированной фразой!';
                  end;
                end if;
              end;
            end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        if P_REMARKS<>'EMPTY_STRING' then
          begin
            select count(id) into V_R_COUNT
            from WB_REF_RESERVED_PHRASE
            where PHRASE=P_REMARKS;

            if V_R_COUNT>0 then
              begin
                if P_LANG='ENG' then
                  begin
                     V_STR_MSG:='Value '||P_REMARKS||' is a phrase reserved!';
                  end;
                else
                  begin
                    V_STR_MSG:='Значение '||P_REMARKS||' является зарезервированной фразой!';
                  end;
                end if;
              end;
            end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------

    if (V_STR_MSG is null) then
      begin
        update WB_REF_WS_AIR_DOW_SWA_CODES
        set CODE_NAME_1=P_CODE_NAME_1,
	          CODE_NAME_2=P_CODE_NAME_2,
	          WEIGHT=P_WEIGHT,
	          BALANCE_ARM=P_BALANCE_ARM,
	          INDEX_UNIT=P_INDEX_UNIT,
            REMARKS=P_REMARKS,
	          U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where id=P_ID;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_DOW_SWA_C_UPD;
/
