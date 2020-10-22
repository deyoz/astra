create or replace procedure SP_WB_REF_WS_AIR_DOW_SWACDRUPD
(cXML_in in clob,
   cXML_out out clob)
as

P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

P_IS_BALANCE_ARM NUMBER:=null;
P_IS_INDEX_UNIT NUMBER:=null;

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


    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_BALANCE_ARM[1]')) into P_IS_BALANCE_ARM from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_INDEX_UNIT[1]')) into P_IS_INDEX_UNIT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;


    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_AIRCO_WS_TYPES
    where id_ac=P_ID_AC and
          id_ws=P_ID_WS;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='This type of aircraft is removed from the selected airline!';
          end;
        else
          begin
            V_STR_MSG:='Этот тип ВС удален из выбранной авиакомпании!';
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------

    if (V_STR_MSG is null) then
      begin
        update WB_REF_WS_AIR_DOW_SWA_CDER
        set IS_BALANCE_ARM=P_IS_BALANCE_ARM,
	          IS_INDEX_UNIT=P_IS_INDEX_UNIT,
	          U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_DOW_SWACDRUPD;
/
