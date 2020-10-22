create or replace procedure SP_WB_REF_WS_AIR_WCC_UPD
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;

P_CODE_NAME varchar2(200):='';
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

P_CREW_CODE_ID number:=null;
P_PANTRY_CODE_ID number:=null;
P_PORTABLE_WATER_CODE_ID number:=null;

P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;


    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CODE_NAME[1]') into P_CODE_NAME from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CREW_CODE_ID[1]')) into P_CREW_CODE_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_PANTRY_CODE_ID[1]')) into P_PANTRY_CODE_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_PORTABLE_WATER_CODE_ID[1]')) into P_PORTABLE_WATER_CODE_ID from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_WCC
    where ID=P_ID;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
             V_STR_MSG:='The selected record deleted!';
          end;
        else
          begin
            V_STR_MSG:='Выбранная запись удалена!';
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_DOW_CR_CODES
        where id=P_CREW_CODE_ID;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='The selected value of "Crew Code" is removed from the block "Crew Codes"!';
              end;
            else
              begin
                V_STR_MSG:='Выбранное значение "Crew Code" удалено из блока "Crew Codes"!';
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
        from WB_REF_WS_AIR_DOW_PT_CODES
        where ID=P_PANTRY_CODE_ID;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                 V_STR_MSG:='The selected value of "Pantry Code" is removed from the block "Pantry Codes"!';
              end;
            else
              begin
                V_STR_MSG:='Выбранное значение "Pantry Code" удалено из блока "Pantry Codes"!';
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
        from WB_REF_WS_AIR_DOW_PW_CODES
        where ID=P_PORTABLE_WATER_CODE_ID;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                 V_STR_MSG:='The selected value of "Portable Water Code" is removed from the block "Portable Water Codes"!';
              end;
            else
              begin
                V_STR_MSG:='Выбранное значение "Portable Water Code" удалено из блока "Portable Water Codes"!';
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
        from WB_REF_WS_AIR_WCC
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT and
              CODE_NAME=P_CODE_NAME and
              id<>P_ID;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 V_STR_MSG:='The value entered "Weight Configuration Code Name" is already in use!';
              end;
            else
              begin
                V_STR_MSG:='Введенное значение "Weight Configuration Code Name" уже используется!';
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
        from WB_REF_RESERVED_PHRASE
        where PHRASE=P_CODE_NAME;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 V_STR_MSG:='Value '||P_CODE_NAME||' is a phrase reserved!';
              end;
            else
              begin
                V_STR_MSG:='Значение '||P_CODE_NAME||' является зарезервированной фразой!';
              end;
            end if;
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

    if (V_STR_MSG is null) then
      begin
        update WB_REF_WS_AIR_WCC
	      set CODE_NAME=P_CODE_NAME,
	          CREW_CODE_ID=P_CREW_CODE_ID,
	          PANTRY_CODE_ID=P_PANTRY_CODE_ID,
	          PORTABLE_WATER_CODE_ID=P_PORTABLE_WATER_CODE_ID,
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
  end SP_WB_REF_WS_AIR_WCC_UPD;
/
